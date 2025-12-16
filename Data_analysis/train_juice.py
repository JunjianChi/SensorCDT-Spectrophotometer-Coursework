# train_juice.py
from __future__ import annotations

import argparse
from pathlib import Path
import numpy as np
import pandas as pd
from joblib import dump

from sklearn.model_selection import StratifiedKFold, train_test_split
from sklearn.preprocessing import LabelEncoder
from sklearn.metrics import accuracy_score, f1_score

from sklearn.linear_model import LogisticRegression
from sklearn.svm import LinearSVC, SVC
from sklearn.ensemble import RandomForestClassifier, HistGradientBoostingClassifier

from dataset import load_juice_Xy
from features import make_features


def get_models(seed: int):
    return {
        "logistic regression": LogisticRegression(max_iter=2000, random_state=seed),
        "svm_lin": LinearSVC(dual="auto", random_state=seed),
        "svm_rbf": SVC(kernel="rbf", C=10.0, gamma="scale", random_state=seed),
        "random forest": RandomForestClassifier(n_estimators=500, random_state=seed, n_jobs=-1),
        "gradient boosting": HistGradientBoostingClassifier(max_iter=300, learning_rate=0.05, random_state=seed),
    }


def cv_score(model, X: np.ndarray, y: np.ndarray, cv: StratifiedKFold):
    accs, f1s = [], []
    for tr, va in cv.split(X, y):
        model.fit(X[tr], y[tr])
        pred = model.predict(X[va])
        accs.append(accuracy_score(y[va], pred))
        f1s.append(f1_score(y[va], pred, average="macro"))
    return float(np.mean(accs)), float(np.std(accs)), float(np.mean(f1s)), float(np.std(f1s))


def main():
    ap = argparse.ArgumentParser("Juice classification - Train(CV) + Hold-out Test + save best model bundle")
    ap.add_argument("--data_dir", type=str, default="../Data")
    ap.add_argument("--preprocess", choices=["raw", "l1"], default="l1")
    ap.add_argument("--folds", type=int, default=5)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--models_dir", type=str, default="models")
    ap.add_argument("--test_size", type=float, default=0.2)
    args = ap.parse_args()

    models_dir = Path(args.models_dir)
    models_dir.mkdir(parents=True, exist_ok=True)

    # load full data
    X_raw, y_str, feat_cols, df = load_juice_Xy(Path(args.data_dir))
    X_all = make_features(X_raw, preprocess=args.preprocess)

    le = LabelEncoder()
    y_all = le.fit_transform(y_str)
    class_names = list(le.classes_)

    # hold-out split (used ONLY at final evaluation)
    X_tr, X_te, y_tr, y_te = train_test_split(
        X_all, y_all,
        test_size=args.test_size,
        random_state=args.seed,
        stratify=y_all
    )

    cv = StratifiedKFold(n_splits=args.folds, shuffle=True, random_state=args.seed)
    models = get_models(seed=args.seed)

    print("=" * 70)
    print("TRAIN (CV on TRAIN SPLIT ONLY) + HOLD-OUT TEST")
    print("=" * 70)
    print(f"Data: {Path(args.data_dir).resolve()}")
    print(f"All samples: {len(y_all)} | Train: {len(y_tr)} | Test: {len(y_te)}")
    print(f"Classes: {class_names}")
    print(f"Preprocess: {args.preprocess} | Folds: {args.folds} | Seed: {args.seed} | Test size: {args.test_size}")
    print("-" * 70)

    rows = []
    for name, model in models.items():
        mean_acc, std_acc, mean_f1, std_f1 = cv_score(model, X_tr, y_tr, cv)
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
            "n_all": len(y_all),
        })
        print(f"{name:18s} | CV acc={mean_acc:.3f}±{std_acc:.3f} | CV macroF1={mean_f1:.3f}±{std_f1:.3f}")

    res = pd.DataFrame(rows).sort_values("mean_macro_f1", ascending=False).reset_index(drop=True)
    results_csv = models_dir / "results_cv.csv"
    res.to_csv(results_csv, index=False)

    best_name = str(res.loc[0, "model"])
    print("-" * 70)
    print("Best by CV mean_macro_f1 (on TRAIN split):", best_name)
    print("Saved CV results:", results_csv.resolve())

    # fit best model on TRAIN split only
    best_model = models[best_name]
    best_model.fit(X_tr, y_tr)

    bundle = {
        "model": best_model,
        "label_encoder": le,
        "feature_cols": feat_cols,
        "preprocess": args.preprocess,
        "folds": args.folds,
        "seed": args.seed,
        "test_size": args.test_size,
        "classes": class_names,
        "cv_summary": res.to_dict(orient="records"),
        # store hold-out test for evaluation script
        "X_test": X_te,
        "y_test": y_te,
    }

    best_path = models_dir / "best_juice_model.joblib"
    dump(bundle, best_path)

    print("Saved best model bundle:", best_path.resolve())
    print("=" * 70)


if __name__ == "__main__":
    main()
