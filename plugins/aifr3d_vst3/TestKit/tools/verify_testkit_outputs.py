#!/usr/bin/env python3
import json
from pathlib import Path


def get_path(obj, path):
    cur = obj
    for p in path.split("."):
        if not isinstance(cur, dict) or p not in cur:
            return None, False
        cur = cur[p]
    return cur, True


def fail(msg):
    raise AssertionError(msg)


def main():
    root = Path(__file__).resolve().parents[1]
    expected = json.loads((root / "expected" / "ranges.json").read_text())
    out_dir = root / "out"

    if not out_dir.exists():
        fail(f"Output directory missing: {out_dir}. Run run_testkit_analysis.py first.")

    checked = 0
    for wav_name, rules in expected.items():
        out_json = out_dir / f"{Path(wav_name).stem}.json"
        if not out_json.exists():
            fail(f"Missing output JSON for {wav_name}: {out_json}")

        data = json.loads(out_json.read_text())
        checked += 1

        expected_sr = rules.get("sample_rate_hz")
        if expected_sr is not None:
            got = int(round(float(data.get("sample_rate_hz", -1))))
            if got != int(expected_sr):
                fail(f"{wav_name}: sample_rate_hz expected {expected_sr}, got {got}")

        frame_count = int(data.get("frame_count", -1))
        fmin = rules.get("frame_count_min")
        fmax = rules.get("frame_count_max")
        if fmin is not None and frame_count < int(fmin):
            fail(f"{wav_name}: frame_count {frame_count} < min {fmin}")
        if fmax is not None and frame_count > int(fmax):
            fail(f"{wav_name}: frame_count {frame_count} > max {fmax}")

        for p in rules.get("expect_null", []):
            v, ok = get_path(data, p)
            if (not ok) or v is not None:
                fail(f"{wav_name}: expected null at '{p}', got {v!r}")

        for p, r in rules.get("ranges", {}).items():
            v, ok = get_path(data, p)
            if not ok:
                fail(f"{wav_name}: missing path '{p}'")
            if v is None:
                fail(f"{wav_name}: null value at '{p}' but expected range {r}")
            lo, hi = float(r[0]), float(r[1])
            fv = float(v)
            if fv < lo or fv > hi:
                fail(f"{wav_name}: '{p}'={fv:.6f} outside [{lo:.6f}, {hi:.6f}]")

    print(f"[OK] Verified {checked} TestKit outputs against expected ranges")


if __name__ == "__main__":
    main()
