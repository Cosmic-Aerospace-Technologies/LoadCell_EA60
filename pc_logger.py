"""THRUST // LAB wireless PC logger.

Connect the computer to the ESP32 Wi-Fi network, then run:
    py -3 pc_logger.py

No third-party Python packages are required.
"""

from __future__ import annotations

import argparse
import csv
from datetime import datetime
import json
from pathlib import Path
import time
from urllib.error import HTTPError, URLError
from urllib.request import urlopen

COLUMNS = (
    "pc_timestamp",
    "elapsed_ms",
    "esp_uptime_ms",
    "raw_counts",
    "thrust_gf",
    "thrust_kgf",
    "thrust_lbf",
    "thrust_N",
    "motor_temp_C",
    "motor_temp_F",
    "quality",
)


class Logger:
    def __init__(self, base: str, directory: Path, poll_seconds: float = 0.12):
        self.base = base.rstrip("/")
        self.directory = directory
        self.poll_seconds = poll_seconds

        self.directory.mkdir(parents=True, exist_ok=True)

        self.boot = None
        self.last_run = None
        self.active_run = None
        self.cursor = 0
        self.start_ms = 0

        self.file_handle = None
        self.writer = None
        self.file_path = None

        self.rows = 0
        self.gaps = 0

        self.last_ping = 0.0
        self.last_flush = 0.0
        self.last_error = None

    def request(self, path: str) -> dict:
        with urlopen(self.base + path, timeout=2.0) as response:
            return json.load(response)

    def begin(self, status: dict) -> None:
        if self.file_handle is not None:
            raise RuntimeError("A CSV file is already open.")

        self.active_run = int(status["run"])
        self.cursor = max(0, int(status["runStartSeq"]) - 1)
        self.start_ms = int(status["runStartMs"])

        name = datetime.now().strftime("run_%Y%m%d_%H%M%S_%f.csv")
        self.file_path = self.directory / name

        self.file_handle = self.file_path.open(
            "x",
            newline="",
            encoding="utf-8",
        )

        self.writer = csv.writer(self.file_handle)
        self.writer.writerow(COLUMNS)
        self.file_handle.flush()

        self.rows = 0
        self.gaps = 0
        self.last_flush = time.monotonic()

        print(
            f"[START] Run {self.active_run} -> {self.file_path}",
            flush=True,
        )

    def finish(self, reason: str) -> None:
        if self.file_handle is not None:
            self.file_handle.flush()
            self.file_handle.close()

            print(
                f"[SAVED] {self.file_path} | "
                f"{self.rows} samples | "
                f"{self.gaps} missing | "
                f"{reason}",
                flush=True,
            )

        self.file_handle = None
        self.writer = None
        self.file_path = None
        self.active_run = None
        self.rows = 0
        self.gaps = 0

    def tick(self) -> None:
        now = time.monotonic()

        if now - self.last_ping >= 0.8:
            self.request("/api/logger/ping")
            self.last_ping = now

        status = self.request("/api/status")

        boot = int(status["boot"])
        run = int(status["run"])

        if self.boot != boot:
            if self.active_run is not None:
                self.finish("ESP32 rebooted; recording may be incomplete.")

            self.boot = boot
            self.last_run = None
            print("[LINK] ESP32 connected over standalone Wi-Fi", flush=True)

        if self.last_run is None:
            self.last_run = run

            if status["recording"]:
                self.begin(status)

        elif run != self.last_run:
            if self.active_run is not None:
                self.finish("A new run began before the previous CSV finalized.")

            self.last_run = run

            if status["recording"]:
                self.begin(status)

        if self.active_run is None:
            return

        end_seq = (
            None
            if status["recording"]
            else int(status["runEndSeq"])
        )

        batch = self.request(
            f"/api/samples?after={self.cursor}&limit=96"
        )

        if int(batch["boot"]) != self.boot:
            self.finish("ESP32 rebooted during sample transfer.")
            return

        points = batch.get("points", [])

        for record in points:
            if len(record) < 4:
                continue

            seq, timestamp_ms, gf, raw = record[:4]
            temp_c = record[4] if len(record) >= 5 else None

            seq = int(seq)
            timestamp_ms = int(timestamp_ms)
            gf = float(gf)
            raw = int(raw)

            if seq <= self.cursor:
                continue

            if end_seq is not None and seq > end_seq:
                break

            missing = seq - self.cursor - 1

            if missing > 0:
                self.gaps += missing
                print(
                    f"[GAP] Missing {missing} sample(s) before sequence {seq}",
                    flush=True,
                )

            self.cursor = seq
            elapsed_ms = (timestamp_ms - self.start_ms) & 0xFFFFFFFF

            if temp_c is None:
                temp_c_text = ""
                temp_f_text = ""
            else:
                temp_c_value = float(temp_c)
                temp_c_text = f"{temp_c_value:.2f}"
                temp_f_text = f"{temp_c_value * 9.0 / 5.0 + 32.0:.2f}"

            self.writer.writerow(
                (
                    datetime.now()
                    .astimezone()
                    .isoformat(timespec="milliseconds"),
                    elapsed_ms,
                    timestamp_ms,
                    raw,
                    f"{gf:.3f}",
                    f"{gf / 1000:.6f}",
                    f"{gf / 453.59237:.6f}",
                    f"{gf * 0.00980665:.6f}",
                    temp_c_text,
                    temp_f_text,
                    "gap_before" if missing else "ok",
                )
            )

            self.rows += 1

        if (
            self.file_handle is not None
            and time.monotonic() - self.last_flush >= 0.5
        ):
            self.file_handle.flush()
            self.last_flush = time.monotonic()

        if end_seq is not None:
            if self.cursor >= end_seq:
                self.finish("Completed")
                return

            oldest = int(batch["oldestSeq"])

            if oldest > self.cursor + 1:
                missing = oldest - self.cursor - 1
                self.gaps += missing
                self.cursor = oldest - 1

                print(
                    f"[GAP] {missing} sample(s) were overwritten in ESP32 RAM",
                    flush=True,
                )

            if self.cursor >= end_seq:
                self.finish("Incomplete: ESP32 RAM history was overwritten.")

    def run(self) -> None:
        print("THRUST // LAB - Wi-Fi PC Logger", flush=True)
        print(f"ESP32: {self.base}", flush=True)
        print(f"Recording folder: {self.directory}", flush=True)
        print("Leave this terminal running during tests. Ctrl+C exits.", flush=True)

        try:
            while True:
                try:
                    self.tick()

                    if self.last_error is not None:
                        print("[LINK] Connection restored", flush=True)

                    self.last_error = None
                    time.sleep(self.poll_seconds)

                except (
                    OSError,
                    ValueError,
                    KeyError,
                    TypeError,
                    RuntimeError,
                    HTTPError,
                    URLError,
                ) as exc:
                    error_text = f"{type(exc).__name__}: {exc}"

                    if error_text != self.last_error:
                        print("[LINK] " + error_text, flush=True)

                    self.last_error = error_text
                    time.sleep(0.6)

        except KeyboardInterrupt:
            print("\n[EXIT] Logger stopped.", flush=True)

        finally:
            if self.file_handle is not None:
                self.finish("Logger exited before run completion.")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Record ESP32 HX711 thrust and motor temperature samples over Wi-Fi."
    )

    parser.add_argument(
        "--url",
        default="http://192.168.4.1",
        help="ESP32 dashboard/API URL",
    )

    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).resolve().parent / "recordings",
        help="Directory for CSV recordings",
    )

    args = parser.parse_args()
    Logger(args.url, args.output).run()


if __name__ == "__main__":
    main()
