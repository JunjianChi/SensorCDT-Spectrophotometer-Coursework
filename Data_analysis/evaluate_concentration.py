# evaluate_concentration.py
from __future__ import annotations

from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from joblib import load

from sklearn.metrics import confusion_matrix, classification_report, accuracy_score, f1_score

from sklearn.linear_model import LogisticRegression
from sklearn.svm import LinearSVC, SVC
from sklearn.ensemble import RandomForestClassifier, HistGradientBoostingClassifier

from sklearn.preprocessing import OneHotEncoder, LabelEncoder

from dataset import load_concentration_data


def get_model(name: str, seed: int):
    name = str(name).strip().lower()
    if name == "logistic regression":
        return LogisticRegression(max_iter=2000, random_state=seed)
    if name == "svm_lin":
        return LinearSVC(dual="auto", random_state=seed)
    if name == "svm_rbf":
        return SVC(kernel="rbf", C=10.0, gamma="scale", random_state=seed)
    if name == "random forest":
        return RandomForestClassifier(n_estimators=500, random_state=seed, n_jobs=-1)
    if name == "gradient boosting":
        return HistGradientBoostingClassifier(max_iter=300, learning_rate=0.05, random_state=seed)
    raise ValueError(f"Unknown model name: {name}")


def make_raw_features(X_raw: np.ndarray, preprocess: str) -> np.ndarray:
    preprocess = str(preprocess).strip().lower()
    X_raw = X_raw.astype(float)
    if preprocess == "raw":
        return X_raw
    if preprocess == "mean":
        return X_raw.mean(axis=1, keepdims=True)
    raise ValueError(f"Unknown preprocess: {preprocess}. Use 'raw' or 'mean'.")


def make_features(X_raw: np.ndarray, juice_base: np.ndarray, juice_enc: OneHotEncoder, preprocess: str) -> np.ndarray:
    X_feat = make_raw_features(X_raw, preprocess=preprocess)
    juice_oh = juice_enc.transform(juice_base.reshape(-1, 1))
    return np.hstack([X_feat, juice_oh])


def plot_confusion(cm: np.ndarray, class_names: list[str], title: str, out_path: Path, normalised: bool):
    if normalised:
        cm_plot = cm.astype(float)
        row_sum = cm_plot.sum(axis=1, keepdims=True)
        row_sum[row_sum == 0] = 1.0
        cm_plot = cm_plot / row_sum
    else:
        cm_plot = cm

    fig, ax = plt.subplots(figsize=(5.4, 4.8))
    im = ax.imshow(cm_plot, cmap="Blues", vmin=0.0 if normalised else None, vmax=1.0 if normalised else None)

    ax.set_title(title)
    ax.set_xlabel("Predicted label")
    ax.set_ylabel("True label")
    ax.set_xticks(range(len(class_names)))
    ax.set_yticks(range(len(class_names)))
    ax.set_xticklabels(class_names)
    ax.set_yticklabels(class_names)

    for i in range(len(class_names)):
        for j in range(len(class_names)):
            if normalised:
                txt = f"{cm_plot[i, j]:.2f}"
                color = "white" if cm_plot[i, j] > 0.5 else "black"
            else:
                txt = str(int(cm_plot[i, j]))
                color = "white" if cm_plot[i, j] > (cm_plot.max() * 0.5) else "black"
            ax.text(j, i, txt, ha="center", va="center", color=color)

    # fig.colorbar(im, ax=ax)
    plt.tight_layout()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=200)
    plt.close(fig)


def main():
    DATA_DIR = Path("../Data")
    MODELS_DIR = Path("models")

    results_path = MODELS_DIR / "results_concentration_cv.csv"
    bundle_path = MODELS_DIR / "best_concentration_model.joblib"
    if not results_path.exists():
        raise FileNotFoundError(f"Missing {results_path}. Run train_concentration.py first.")
    if not bundle_path.exists():
        raise FileNotFoundError(f"Missing {bundle_path}. Run train_concentration.py first.")

    results = pd.read_csv(results_path)
    model_names = results["model"].tolist()

    bundle = load(bundle_path)
    preprocess = str(bundle["preprocess"])
    seed = int(bundle["seed"])
    tr_idx = np.array(bundle["train_indices"], dtype=int)
    te_idx = np.array(bundle["test_indices"], dtype=int)

    # load full dataset again, then apply the exact saved split
    X_raw_all, y_conc_str, juice_all, feat_cols, df = load_concentration_data(DATA_DIR)

    le: LabelEncoder = bundle["label_encoder"]
    y_all = le.transform(y_conc_str)
    class_names = list(bundle["classes"])

    X_raw_tr, X_raw_te = X_raw_all[tr_idx], X_raw_all[te_idx]
    y_tr, y_te = y_all[tr_idx], y_all[te_idx]
    juice_tr, juice_te = juice_all[tr_idx], juice_all[te_idx]

    # fit ONE encoder on full train split (no leakage)
    juice_enc = OneHotEncoder(sparse=False, handle_unknown="ignore")
    juice_enc.fit(juice_tr.reshape(-1, 1))

    X_tr = make_features(X_raw_tr, juice_tr, juice_enc, preprocess=preprocess)
    X_te = make_features(X_raw_te, juice_te, juice_enc, preprocess=preprocess)

    fig_dir = MODELS_DIR / "figures_concentration_test"
    fig_dir.mkdir(parents=True, exist_ok=True)

    summary_rows = []
    per_class_rows = []

    for name in model_names:
        model = get_model(name, seed=seed)
        model.fit(X_tr, y_tr)
        pred = model.predict(X_te)

        acc = accuracy_score(y_te, pred)
        macro_f1 = f1_score(y_te, pred, average="macro")
        weighted_f1 = f1_score(y_te, pred, average="weighted")

        cm = confusion_matrix(y_te, pred)
        title = f"Concentration confusion matrix ({str(name).upper()})"

        safe_name = str(name).replace(" ", "_")
        plot_confusion(cm, class_names, title, fig_dir / f"cm_{safe_name}.png", normalised=False)
        plot_confusion(cm, class_names, title, fig_dir / f"cm_{safe_name}_norm.png", normalised=True)

        rpt = classification_report(y_te, pred, target_names=class_names, output_dict=True, digits=6)

        summary_rows.append({
            "model": name,
            "preprocess": preprocess,
            "seed": seed,
            "n_test": int(len(y_te)),
            "test_accuracy": acc,
            "test_macro_f1": macro_f1,
            "test_weighted_f1": weighted_f1,
        })

        for cls in class_names:
            per_class_rows.append({
                "model": name,
                "preprocess": preprocess,
                "seed": seed,
                "class": cls,
                "precision": rpt[cls]["precision"],
                "recall": rpt[cls]["recall"],
                "f1": rpt[cls]["f1-score"],
                "support": int(rpt[cls]["support"]),
            })

        print(f"{name:18s} | test acc={acc:.3f} | macroF1={macro_f1:.3f} | weightedF1={weighted_f1:.3f}")

    summary_df = pd.DataFrame(summary_rows).sort_values("test_macro_f1", ascending=False)
    per_class_df = pd.DataFrame(per_class_rows).sort_values(["model", "class"])

    summary_csv = MODELS_DIR / "concentration_summary_table.csv"
    per_class_csv = MODELS_DIR / "concentration_per_class_table.csv"
    summary_df.to_csv(summary_csv, index=False)
    per_class_df.to_csv(per_class_csv, index=False)

    print("\nSaved tables:")
    print(" -", summary_csv.resolve())
    print(" -", per_class_csv.resolve())
    print("Saved figures to:")
    print(" -", fig_dir.resolve())


if __name__ == "__main__":
    main()
