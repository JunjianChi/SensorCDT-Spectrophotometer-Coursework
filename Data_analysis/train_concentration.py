# train_concentration.py
from __future__ import annotations

import argparse
from pathlib import Path
import numpy as np
import pandas as pd
from joblib import dump

from sklearn.model_selection import StratifiedKFold, train_test_split
from sklearn.preprocessing import LabelEncoder, OneHotEncoder
from sklearn.metrics import accuracy_score, f1_score

from sklearn.linear_model import LogisticRegression
from sklearn.svm import LinearSVC, SVC
from sklearn.ensemble import RandomForestClassifier, HistGradientBoostingClassifier

from dataset import load_concentration_data


def get_models(seed: int):
    return {
        "logistic regression": LogisticRegression(max_iter=2000, random_state=seed),
        "svm_lin": LinearSVC(dual="auto", random_state=seed),
        "svm_rbf": SVC(kernel="rbf", C=10.0, gamma="scale", random_state=seed),
        "random forest": RandomForestClassifier(n_estimators=500, random_state=seed, n_jobs=-1),
        "gradient boosting": HistGradientBoostingClassifier(max_iter=300, learning_rate=0.05, random_state=seed),
    }


def make_raw_features(X_raw: np.ndarray, preprocess: str) -> np.ndarray:
    preprocess = str(preprocess).strip().lower()
    X_raw = X_raw.astype(float)

    if preprocess == "raw":
        return X_raw
    if preprocess == "mean":
        # one value per sample
        return X_raw.mean(axis=1, keepdims=True)

    raise ValueError(f"Unknown preprocess: {preprocess}. Use 'raw' or 'mean'.")


def make_features(X_raw: np.ndarray, juice_base: np.ndarray, juice_enc: OneHotEncoder, preprocess: str) -> np.ndarray:
    X_feat = make_raw_features(X_raw, preprocess=preprocess)
    juice_oh = juice_enc.transform(juice_base.reshape(-1, 1))
    return np.hstack([X_feat, juice_oh])


def cv_score_with_encoder(
    model,
    X_raw_tr: np.ndarray,
    y_tr: np.ndarray,
    juice_tr: np.ndarray,
    cv: StratifiedKFold,
    preprocess: str,
):
    accs, f1s = [], []
    for tr_idx, va_idx in cv.split(X_raw_tr, y_tr):
        # fit encoder on fold-train only (no leakage)
        enc = OneHotEncoder(sparse=False, handle_unknown="ignore")
        enc.fit(juice_tr[tr_idx].reshape(-1, 1))

        Xtr = make_features(X_raw_tr[tr_idx], juice_tr[tr_idx], enc, preprocess=preprocess)
        Xva = make_features(X_raw_tr[va_idx], juice_tr[va_idx], enc, preprocess=preprocess)

        model.fit(Xtr, y_tr[tr_idx])
        pred = model.predict(Xva)

        accs.append(accuracy_score(y_tr[va_idx], pred))
        f1s.append(f1_score(y_tr[va_idx], pred, average="macro"))

    return float(np.mean(accs)), float(np.std(accs)), float(np.mean(f1s)), float(np.std(f1s))


def main():
    ap = argparse.ArgumentParser("Concentration - Train(CV on TRAIN split) + hold-out TEST + save best model")
    ap.add_argument("--data_dir", type=str, default="../Data")
    ap.add_argument("--models_dir", type=str, default="models")
    ap.add_argument("--preprocess", choices=["raw", "mean"], default="mean")
    ap.add_argument("--folds", type=int, default=5)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--test_size", type=float, default=0.2)
    args = ap.parse_args()

    DATA_DIR = Path(args.data_dir)
    MODELS_DIR = Path(args.models_dir)
    MODELS_DIR.mkdir(parents=True, exist_ok=True)

    # load full dataset
    X_raw_all, y_conc_str, juice_base_all, feat_cols, df = load_concentration_data(DATA_DIR)

    # encode y
    le = LabelEncoder()
    y_all = le.fit_transform(y_conc_str)
    conc_classes = list(le.classes_)

    n_all = len(y_all)
    all_idx = np.arange(n_all)

    # fixed train/test split (save indices for evaluate to reuse exactly)
    tr_idx, te_idx = train_test_split(
        all_idx,
        test_size=args.test_size,
        random_state=args.seed,
        stratify=y_all
    )
    tr_idx = np.array(tr_idx, dtype=int)
    te_idx = np.array(te_idx, dtype=int)

    X_raw_tr, X_raw_te = X_raw_all[tr_idx], X_raw_all[te_idx]
    y_tr, y_te = y_all[tr_idx], y_all[te_idx]
    juice_tr, juice_te = juice_base_all[tr_idx], juice_base_all[te_idx]

    cv = StratifiedKFold(n_splits=args.folds, shuffle=True, random_state=args.seed)
    models = get_models(seed=args.seed)

    print("=" * 70)
    print("CONCENTRATION TRAIN: CV on TRAIN split ONLY + hold-out TEST")
    print("=" * 70)
    print(f"All: {n_all} | Train: {len(y_tr)} | Test: {len(y_te)}")
    print(f"Classes: {conc_classes}")
    print(f"Preprocess: {args.preprocess} | Folds: {args.folds} | Seed: {args.seed} | Test size: {args.test_size}")
    print("-" * 70)

    rows = []
    for name, model in models.items():
        mean_acc, std_acc, mean_f1, std_f1 = cv_score_with_encoder(
            model, X_raw_tr, y_tr, juice_tr, cv, preprocess=args.preprocess
        )
        rows.append({
            "model": name,
            "preprocess": args.preprocess,
            "folds": args.folds,
            "seed": args.seed,
            "test_size": args.test_size,
            "mean_accuracy": mean_acc,
            "std_accuracy": std_acc,
            "mean_macro_f1": mean_f1,
            "std_macro_f1": std_f1,
            "n_train": len(y_tr),
            "n_test": len(y_te),
            "n_all": n_all,
        })
        print(f"{name:18s} | CV acc={mean_acc:.3f}±{std_acc:.3f} | CV macroF1={mean_f1:.3f}±{std_f1:.3f}")

    res = pd.DataFrame(rows).sort_values("mean_macro_f1", ascending=False).reset_index(drop=True)
    results_csv = MODELS_DIR / "results_concentration_cv.csv"
    res.to_csv(results_csv, index=False)

    best_name = str(res.loc[0, "model"])
    print("-" * 70)
    print("Best by CV mean_macro_f1 (on TRAIN split):", best_name)
    print("Saved CV results:", results_csv.resolve())

    # fit encoder on FULL TRAIN split, then train best model on FULL TRAIN split
    juice_enc = OneHotEncoder(sparse=False, handle_unknown="ignore")
    juice_enc.fit(juice_tr.reshape(-1, 1))

    Xtr = make_features(X_raw_tr, juice_tr, juice_enc, preprocess=args.preprocess)
    Xte = make_features(X_raw_te, juice_te, juice_enc, preprocess=args.preprocess)

    best_model = models[best_name]
    best_model.fit(Xtr, y_tr)

    bundle = {
        "best_model_name": best_name,
        "model": best_model,
        "label_encoder": le,
        "juice_encoder": juice_enc,
        "preprocess": args.preprocess,
        "folds": args.folds,
        "seed": args.seed,
        "test_size": args.test_size,
        "classes": conc_classes,
        "cv_summary": res.to_dict(orient="records"),
        # save split indices so evaluate uses the exact same split
        "train_indices": tr_idx,
        "test_indices": te_idx,
        # save test features/labels for convenience
        "X_test": Xte,
        "y_test": y_te,
    }

    best_path = MODELS_DIR / "best_concentration_model.joblib"
    dump(bundle, best_path)

    print("Saved best model bundle:", best_path.resolve())
    print("=" * 70)


if __name__ == "__main__":
    main()
