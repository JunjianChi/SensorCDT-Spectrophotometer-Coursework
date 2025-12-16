# dataset.py
from __future__ import annotations

from pathlib import Path
import glob
import re
import numpy as np
import pandas as pd
from typing import List, Tuple


def load_all_csvs(data_dir: Path) -> pd.DataFrame:
    """Read all csv under data_dir recursively, add metadata like your notebook."""
    data_dir = Path(data_dir)
    csv_paths = sorted([Path(p) for p in glob.glob(str(data_dir / "**" / "*.csv"), recursive=True)])

    frames = []
    for path in csv_paths:
        df = pd.read_csv(path)
        df["__file__"] = path.name
        df["__path__"] = str(path)
        df["cuvette_id"] = path.stem
        frames.append(df)

    if not frames:
        raise ValueError(f"No CSV files found under: {data_dir}")
    return pd.concat(frames, ignore_index=True)


def add_labels(df: pd.DataFrame) -> pd.DataFrame:
    """Create juice_base + brand + cuvette, standardise concentration."""
    df = df.copy()
    jt = df["juice_type"].astype(str).str.strip().str.lower()
    df["juice_base"] = jt.str.extract(r"^([a-z]+)", expand=False).fillna(jt)
    df["brand"] = jt.str.extract(r"(\d+)$", expand=False).fillna("unknown")
    df["cuvette"] = df["cuvette_id"].astype(str).str.strip().str.lower()

    df["concentration"] = (
        df["concentration"].astype(str).str.strip().str.lower()
        .replace({"med": "medium", "mid": "medium", "hi": "high", "lo": "low", "h": "high", "l": "low"})
    )
    return df


def feature_cols(df: pd.DataFrame) -> List[str]:
    cols = [c for c in df.columns if re.match(r"avg_ch\d+$", str(c))]
    cols = sorted(cols, key=lambda x: int(re.findall(r"\d+", x)[0]))
    if not cols:
        raise ValueError("No feature columns like avg_ch1..avg_ch12 found.")
    return cols


def load_juice_Xy(data_dir: Path) -> Tuple[np.ndarray, np.ndarray, List[str], pd.DataFrame]:
    """
    Return full dataset for CV only:
      X: (n_samples, 12)
      y: juice_base strings
    """
    df = load_all_csvs(Path(data_dir))
    df = add_labels(df)
    cols = feature_cols(df)

    df = df.dropna(subset=cols + ["juice_base"])
    X = df[cols].to_numpy(dtype=float)
    y = df["juice_base"].astype(str).to_numpy()
    return X, y, cols, df


def load_concentration_data(data_dir: Path):
    """
    Load full dataset for concentration task.

    Returns:
        X_raw : np.ndarray (N, 12)
        y_conc : np.ndarray (N,)         # low / medium / high
        juice_base : np.ndarray (N,)     # apple / grape / orange
        feature_cols : list[str]
        df : original dataframe
    """
    csv_paths = sorted(glob.glob(str(data_dir / "**" / "*.csv"), recursive=True))
    frames = []

    for p in csv_paths:
        df = pd.read_csv(p)
        df["cuvette_id"] = Path(p).stem
        frames.append(df)

    df = pd.concat(frames, ignore_index=True)

    # extract juice_base (apple1 -> apple)
    df["juice_base"] = df["juice_type"].str.extract(r"^([a-zA-Z]+)", expand=False)

    feature_cols = [c for c in df.columns if c.startswith("avg_ch")]
    X_raw = df[feature_cols].values.astype(float)

    y_conc = df["concentration"].values
    juice_base = df["juice_base"].values

    return X_raw, y_conc, juice_base, feature_cols, df

if __name__ == "__main__":
    X, y, cols, df = load_juice_Xy(Path("../Data"))

    print("Total samples:", len(df))
    print("Feature columns:", cols)
    print("\nSamples per class:")
    print(df["juice_base"].value_counts())

    print("\nSamples per (juice_base, brand):")
    print(df.groupby(["juice_base", "brand"]).size())

    print("\nSamples per cuvette:")
    print(df["cuvette"].value_counts())

    print("\nConcentration distribution:")
    print(df["concentration"].value_counts())
