import csv
import tempfile
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from pc_logger import Logger


class MockLogger(Logger):
    def __init__(self, path):
        super().__init__("http://test.local", path)
        self.mock_run = 0
        self.recording = False
        self.seq = 0
        self.endseq = 0
        self.missing = False

    def request(self, path):
        if path.startswith("/api/logger/ping"):
            return {"ok": True}

        if path.startswith("/api/status"):
            return dict(
                boot=1234,
                run=self.mock_run,
                recording=self.recording,
                runStartSeq=1,
                runStartMs=100,
                runEndSeq=self.endseq,
            )

        if path.startswith("/api/samples"):
            cursor = int(path.split("after=")[1].split("&")[0])
            records = [
                [i, 100 + i * 10, float(i * 2), 100000 + i, 25.0 + i]
                for i in range(cursor + 1, self.seq + 1)
                if i != 3 or not self.missing
            ]

            return dict(
                boot=1234,
                epoch=1,
                lastSeq=self.seq,
                oldestSeq=1,
                points=records[:96],
            )

        raise AssertionError(path)


class TestLogger(unittest.TestCase):
    def test_two_runs_csv_units_and_temperature(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = MockLogger(Path(tmp))
            log.tick()

            log.mock_run = 1
            log.recording = True
            log.seq = 5
            log.tick()
            log.tick()

            log.endseq = 5
            log.recording = False
            log.tick()

            self.assertIsNone(log.active_run)

            files = list(Path(tmp).glob("*.csv"))
            self.assertEqual(len(files), 1)

            with files[0].open() as handle:
                rows = list(csv.DictReader(handle))

            self.assertEqual(len(rows), 5)
            self.assertEqual(rows[0]["elapsed_ms"], "10")
            self.assertAlmostEqual(
                float(rows[0]["thrust_lbf"]),
                2 / 453.59237,
                places=6,
            )
            self.assertEqual(rows[0]["motor_temp_C"], "26.00")
            self.assertEqual(rows[0]["motor_temp_F"], "78.80")
            self.assertEqual(rows[0]["quality"], "ok")

            log.mock_run = 2
            log.recording = True
            log.seq = 6
            log.tick()
            log.tick()

            log.endseq = 6
            log.recording = False
            log.tick()

            self.assertEqual(len(list(Path(tmp).glob("*.csv"))), 2)

    def test_missing_sequence_flagged(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = MockLogger(Path(tmp))
            log.tick()

            log.mock_run = 1
            log.recording = True
            log.seq = 5
            log.missing = True
            log.tick()
            log.tick()

            log.endseq = 5
            log.recording = False
            log.tick()

            with list(Path(tmp).glob("*.csv"))[0].open() as handle:
                rows = list(csv.DictReader(handle))

            self.assertEqual(len(rows), 4)
            self.assertEqual(rows[2]["quality"], "gap_before")
            self.assertEqual(rows[2]["raw_counts"], "100004")


if __name__ == "__main__":
    unittest.main()
