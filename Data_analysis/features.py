# features.py
from __future__ import annotations
import numpy as np
from sklearn.preprocessing import OneHotEncoder

def l1_normalise(X: np.ndarray, eps: float = 1e-12) -> np.ndarray:
    X = np.asarray(X, dtype=float)
    s = np.sum(X, axis=1, keepdims=True)
    s = np.where(s == 0, eps, s)
    return X / s


def make_features(X: np.ndarray, preprocess: str = "l1") -> np.ndarray:
    X = np.asarray(X, dtype=float)
    if preprocess == "raw":
        return X
    if preprocess == "l1":
        return l1_normalise(X)
    raise ValueError("preprocess must be 'raw' or 'l1'")


def make_concentration_features(X_raw, juice_base):
    """
    Build features for concentration model:
    - RAW spectrum
    - + juice_base one-hot
    """
    # one-hot encode juice type
    enc = OneHotEncoder(sparse=False)
    juice_oh = enc.fit_transform(juice_base.reshape(-1, 1))

    X = np.hstack([X_raw, juice_oh])
    return X, enc