# evaluate_juice_test_all_models.py
from __future__ import annotations

from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
from sklearn.metrics import confusion_matrix, classification_report, accuracy_score, f1_score

from sklearn.linear_model import LogisticRegression
from sklearn.svm import LinearSVC, SVC
from sklearn.ensemble import RandomForestClassifier, HistGradientBoostingClassifier

from dataset import load_juice_Xy
from features import make_features


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

    # 固定一次 split，保证每个模型用同一份 test
    seed = 42
    test_size = 0.2

    results_path = MODELS_DIR / "results_cv.csv"
    if not results_path.exists():
        raise FileNotFoundError(f"Missing {results_path}. Run train_juice.py first.")

    results = pd.read_csv(results_path)
    model_names = results["model"].tolist()

    # preprocess：优先用 results_cv.csv 里写的（也可以自己改成固定 l1）
    preprocess = str(results.loc[0, "preprocess"]) if "preprocess" in results.columns else "l1"

    # Load full dataset
    X_raw, y_str, feat_cols, df = load_juice_Xy(DATA_DIR)
    X_all = make_features(X_raw, preprocess=preprocess)

    le = LabelEncoder()
    y_all = le.fit_transform(y_str)
    class_names = list(le.classes_)

    # Split once, same for all models
    X_tr, X_te, y_tr, y_te = train_test_split(
        X_all, y_all,
        test_size=test_size,
        random_state=seed,
        stratify=y_all
    )

    fig_dir = MODELS_DIR / "figures_juice_test"
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

        title = f"Juice confusion matrix ({str(name).upper()})"

        # Save plots: raw + norm
        safe_name = str(name).replace(" ", "_")
        plot_confusion(cm, class_names, title, fig_dir / f"cm_{safe_name}.png", normalised=False)
        plot_confusion(cm, class_names, title, fig_dir / f"cm_{safe_name}_norm.png", normalised=True)

        # Per-class metrics
        rpt = classification_report(y_te, pred, target_names=class_names, output_dict=True, digits=6)

        summary_rows.append({
            "model": name,
            "preprocess": preprocess,
            "seed": seed,
            "test_size": test_size,
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
                "test_size": test_size,
                "class": cls,
                "precision": rpt[cls]["precision"],
                "recall": rpt[cls]["recall"],
                "f1": rpt[cls]["f1-score"],
                "support": int(rpt[cls]["support"]),
            })

        print(f"{name:18s} | test acc={acc:.3f} | macroF1={macro_f1:.3f} | weightedF1={weighted_f1:.3f}")

    summary_df = pd.DataFrame(summary_rows).sort_values("test_macro_f1", ascending=False)
    per_class_df = pd.DataFrame(per_class_rows).sort_values(["model", "class"])

    summary_csv = MODELS_DIR / "juice_summary_table.csv"
    per_class_csv = MODELS_DIR / "juice_per_class_table.csv"
    summary_df.to_csv(summary_csv, index=False)
    per_class_df.to_csv(per_class_csv, index=False)

    print("\nSaved tables:")
    print(" -", summary_csv.resolve())
    print(" -", per_class_csv.resolve())
    print("Saved figures to:")
    print(" -", fig_dir.resolve())


if __name__ == "__main__":
    main()
