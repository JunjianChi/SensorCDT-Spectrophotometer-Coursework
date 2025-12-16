import csv
import sys
import time
from pathlib import Path
from datetime import datetime

import serial


PREFIX = "SORTED(405-855nm):"


def parse_sorted_line(line: str):
    """
    Parse: SORTED(405-855nm): 914,4652,6628,...
    Return list[int] or None.
    """
    line = line.strip()
    if not line.startswith(PREFIX):
        return None
    try:
        data_part = line.split(":", 1)[1].strip()
        vals = [int(x.strip()) for x in data_part.split(",") if x.strip() != ""]
        return vals if vals else None
    except Exception:
        return None


def read_one_measurement(ser: serial.Serial, timeout_s: float = 3.0):
    """
    Read until one valid SORTED line is received or timeout.
    """
    t0 = time.time()
    while time.time() - t0 < timeout_s:
        raw = ser.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", errors="ignore").strip()
        vals = parse_sorted_line(line)
        if vals is not None:
            return vals, line
    return None, None


def sum_measurements(meas_list):
    """
    Channel-wise exact sums (lossless).
    """
    n = len(meas_list)
    m = len(meas_list[0])
    sums = [0] * m
    for i in range(n):
        for j in range(m):
            sums[j] += meas_list[i][j]
    return sums


def ensure_csv_header(path: Path, n_channels: int):
    if path.exists():
        return

    header = ["timestamp", "juice_type", "concentration"]
    header += [f"avg_ch{i+1}" for i in range(n_channels)]

    with path.open("w", newline="", encoding="utf-8") as f:
        csv.writer(f).writerow(header)


def append_row(path: Path, row):
    with path.open("a", newline="", encoding="utf-8") as f:
        csv.writer(f).writerow(row)


def main():
    # -------- CONFIG --------
    port = 'COM3'        # Windows
    # port = '/dev/ttyUSB0'  # Linux
    # port = '/dev/tty.usbserial-XXXX'  # macOS 
    baud = 115200        # MUST match Arduino Serial.begin(...)
    N_READS = 5          # Number of measurements to average in one sample
    # ------------------------

    # Dataset path: ../Data/dataset.csv
    base_dir = Path(__file__).resolve().parent
    data_dir = (base_dir / ".." / "Data").resolve()
    data_dir.mkdir(parents=True, exist_ok=True)
    out_csv = data_dir / "ref.csv"

    try:
        ser = serial.Serial(port, baud, timeout=1)
    except Exception as e:
        print(f"Failed to open serial port: {e}")
        sys.exit(1)

    print(f"Connected: {port} @ {baud}")
    print(f"Dataset file: {out_csv}")
    print("Commands: 'r' + Enter = record one sample (5 reads), 'q' + Enter = quit")

    time.sleep(1.0)
    ser.reset_input_buffer()

    n_channels = None

    while True:
        cmd = input("\nCommand (r=record, q=quit): ").strip().lower()
        if cmd == "q":
            break
        if cmd != "r":
            print("Unknown command.")
            continue

        juice_type = input("Juice type (orange/apple/grape): ").strip()
        concentration = input("Concentration (low/medium/high): ").strip()

         
        ser.reset_input_buffer()
        time.sleep(0.05)

        measurements = []
        print(f"Sampling {N_READS} valid SORTED frames:")

        while len(measurements) < N_READS:
            
            vals, line = read_one_measurement(ser, timeout_s=5.0)
            if vals is None:
                print("Timeout: no valid SORTED line. Sample aborted.")
                break

            if n_channels is None:
                n_channels = len(vals)
                ensure_csv_header(out_csv, n_channels)
                print(f"Detected {n_channels} channels. CSV header created.")

            if len(vals) != n_channels:
                print("Discarded frame (channel count mismatch).")
                continue

            # Optional sanity check
            if any(v < 0 or v > 65535 for v in vals):
                print("Warning: value outside uint16 range detected.")

            measurements.append(vals)
            print(f"  {len(measurements)}/{N_READS}: {line}")

        if len(measurements) != N_READS:
            continue

        sums = sum_measurements(measurements)
        ts = datetime.now().isoformat(timespec="seconds")

        row = [ts, juice_type, concentration] + [s / N_READS for s in sums]
        append_row(out_csv, row)

        means_preview = [s / N_READS for s in sums]
        print("Sample saved.")
        print("Mean preview:", means_preview)

    ser.close()
    print("Exited.")


if __name__ == "__main__":
    main()
