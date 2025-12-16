from __future__ import annotations

import time
from pathlib import Path
import numpy as np
import serial
from joblib import load

from features import make_features as make_juice_features  # 复用你 juice 的特征函数


# -------- CONFIG --------
port = "COM3"        # Windows
# port = "/dev/ttyUSB0"  # Linux
# port = "/dev/tty.usbserial-XXXX"  # macOS
baud = 115200        # MUST match Arduino Serial.begin(...)
N_READS = 5          # Number of measurements to average in one sample
# ------------------------


JUICE_BUNDLE_PATH = Path("../Data_analysis/models/best_juice_model.joblib")
CONC_BUNDLE_PATH  = Path("../Data_analysis/models/best_concentration_model.joblib")


def parse_12_floats(line: str) -> np.ndarray | None:
    """
    Expect a line like:
      1.23,4.56,....,7.89   (12 values)
    or with spaces:
      1.23, 4.56, ..., 7.89
    Returns np.array shape (12,) or None if parse fails.
    """
    s = line.strip()
    if not s:
        return None

    # Allow Arduino to send extra tokens like "RAW:" prefix
    # Example: "RAW:1,2,3,..."
    if ":" in s:
        s = s.split(":", 1)[1].strip()

    parts = [p.strip() for p in s.split(",") if p.strip() != ""]
    if len(parts) != 12:
        return None

    try:
        vals = np.array([float(p) for p in parts], dtype=float)
    except ValueError:
        return None

    return vals


def make_conc_raw_features(X_raw: np.ndarray, preprocess: str) -> np.ndarray:
    """
    Must match your train_concentration.py:
      - raw: use as-is
      - mean: mean over channels (axis=1)
    """
    preprocess = str(preprocess).strip().lower()
    X_raw = np.asarray(X_raw, dtype=float)
    if X_raw.ndim == 1:
        X_raw = X_raw.reshape(1, -1)

    if preprocess == "raw":
        return X_raw
    if preprocess == "mean":
        return X_raw.mean(axis=1, keepdims=True)

    raise ValueError(f"Unknown concentration preprocess: {preprocess}")


def make_conc_features(X_raw: np.ndarray, juice_label: str, juice_enc, preprocess: str) -> np.ndarray:
    X_feat = make_conc_raw_features(X_raw, preprocess=preprocess)
    juice_oh = juice_enc.transform(np.array([juice_label], dtype=object).reshape(-1, 1))
    return np.hstack([X_feat, juice_oh])


def maybe_proba(model, X: np.ndarray):
    if hasattr(model, "predict_proba"):
        try:
            return model.predict_proba(X)
        except Exception:
            return None
    return None


def main():
    # ---- load bundles once ----
    if not JUICE_BUNDLE_PATH.exists():
        raise FileNotFoundError(f"Missing {JUICE_BUNDLE_PATH}. Run train_juice.py first.")
    if not CONC_BUNDLE_PATH.exists():
        raise FileNotFoundError(f"Missing {CONC_BUNDLE_PATH}. Run train_concentration.py first.")

    juice_bundle = load(JUICE_BUNDLE_PATH)
    conc_bundle = load(CONC_BUNDLE_PATH)

    juice_model = juice_bundle["model"]
    juice_le = juice_bundle["label_encoder"]
    juice_preprocess = juice_bundle.get("preprocess", "raw")

    conc_model = conc_bundle["model"]
    conc_le = conc_bundle["label_encoder"]
    conc_juice_enc = conc_bundle["juice_encoder"]
    conc_preprocess = conc_bundle.get("preprocess", "mean")

    print("Loaded models:")
    print(" - Juice preprocess:", juice_preprocess)
    print(" - Concentration preprocess:", conc_preprocess)

    # ---- open serial ----
    ser = serial.Serial(port, baudrate=baud, timeout=1)
    time.sleep(2.0)  # give Arduino time to reset
    ser.reset_input_buffer()
    print(f"Serial connected: {port} @ {baud}")
    print("Waiting for 12-channel lines...")

    # ---- main loop ----
    buf = []
    while True:
        try:
            raw_line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not raw_line:
                continue

            vals = parse_12_floats(raw_line)
            if vals is None:
                # You can uncomment to debug unexpected lines:
                # print("Skip line:", raw_line)
                continue

            buf.append(vals)
            if len(buf) < N_READS:
                continue

            # average N_READS measurements => one sample
            sample = np.mean(np.stack(buf, axis=0), axis=0)  # shape (12,)
            buf.clear()

            X_raw = sample.reshape(1, -1)

            # ---- JUICE inference ----
            Xj = make_juice_features(X_raw, preprocess=juice_preprocess)
            j_id = int(juice_model.predict(Xj)[0])
            j_label = str(juice_le.inverse_transform([j_id])[0])

            # ---- CONCENTRATION inference (uses predicted juice) ----
            Xc = make_conc_features(X_raw, juice_label=j_label, juice_enc=conc_juice_enc, preprocess=conc_preprocess)
            c_id = int(conc_model.predict(Xc)[0])
            c_label = str(conc_le.inverse_transform([c_id])[0])

            # optional probabilities (if supported)
            j_p = maybe_proba(juice_model, Xj)
            c_p = maybe_proba(conc_model, Xc)

            # ---- format reply ----
            # Simple, Arduino-friendly one-line response:
            #   JUICE=<label>;CONC=<label>
            # If proba available, also send max confidence:
            msg = f"JUICE={j_label};CONC={c_label}"

            if j_p is not None:
                j_conf = float(np.max(j_p[0]))
                msg += f";JUICE_CONF={j_conf:.3f}"
            if c_p is not None:
                c_conf = float(np.max(c_p[0]))
                msg += f";CONC_CONF={c_conf:.3f}"

            msg += "\n"

            ser.write(msg.encode("utf-8"))
            print("IN:", sample)
            print("OUT:", msg.strip())

        except KeyboardInterrupt:
            print("\nStopping.")
            break
        except Exception as e:
            # don't crash the loop on a single bad line / transient error
            err = f"ERROR={type(e).__name__}:{e}\n"
            try:
                ser.write(err.encode("utf-8"))
            except Exception:
                pass
            print(err.strip())
            buf.clear()

    ser.close()


if __name__ == "__main__":
    main()
