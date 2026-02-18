#!/usr/bin/env python3
import argparse
import subprocess
from pathlib import Path


def run(cmd):
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


def main():
    parser = argparse.ArgumentParser(description="Run AIFR3D TestKit analysis via aifr3d_core_cli")
    parser.add_argument("--build-dir", default="build_phase7", help="CMake build directory containing aifr3d_core_cli")
    parser.add_argument("--benchmark", default="data/benchmarks/test_pop_profile.json", help="Benchmark profile JSON")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[4]
    testkit = repo / "plugins" / "aifr3d_vst3" / "TestKit"
    wav_dir = testkit / "wav"
    out_dir = testkit / "out"
    out_dir.mkdir(parents=True, exist_ok=True)

    cli = repo / args.build_dir / "packages" / "aifr3d_core" / "aifr3d_core_cli"
    if not cli.exists():
        raise FileNotFoundError(f"CLI binary not found: {cli}. Build with CMake first.")

    benchmark = repo / args.benchmark
    if not benchmark.exists():
        raise FileNotFoundError(f"Benchmark profile not found: {benchmark}")

    for wav in sorted(wav_dir.glob("*.wav")):
        out_json = out_dir / f"{wav.stem}.json"
        run([str(cli), str(wav), str(out_json), str(benchmark)])

    print(f"Generated analysis JSON files in: {out_dir}")


if __name__ == "__main__":
    main()
