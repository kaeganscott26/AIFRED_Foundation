#!/usr/bin/env python3
import math
import random
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
WAV_DIR = ROOT / "wav"
SR = 48000
DUR_SEC = 1.0
N = int(SR * DUR_SEC)


def to_i16(x: float) -> int:
    x = max(-1.0, min(1.0, x))
    return int(x * 32767.0)


def write_stereo_wav(path: Path, left, right):
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(SR)
        frames = bytearray()
        for l, r in zip(left, right):
            li = to_i16(l)
            ri = to_i16(r)
            frames.extend((li & 0xFF, (li >> 8) & 0xFF, ri & 0xFF, (ri >> 8) & 0xFF))
        w.writeframes(frames)


def gen_silence():
    z = [0.0] * N
    write_stereo_wav(WAV_DIR / "silence.wav", z, z)


def gen_sine_1khz():
    a = 0.5
    l = [a * math.sin(2.0 * math.pi * 1000.0 * i / SR) for i in range(N)]
    write_stereo_wav(WAV_DIR / "sine_1khz.wav", l, l)


def gen_dual_tone():
    a1, a2 = 0.35, 0.25
    l = [a1 * math.sin(2.0 * math.pi * 220.0 * i / SR) + a2 * math.sin(2.0 * math.pi * 1760.0 * i / SR)
         for i in range(N)]
    write_stereo_wav(WAV_DIR / "dual_tone.wav", l, l)


def gen_transient_train():
    x = [0.0] * N
    period = SR // 8
    for start in range(0, N, period):
        for k in range(min(500, N - start)):
            env = math.exp(-k / 70.0)
            x[start + k] += 0.9 * env * math.sin(2.0 * math.pi * 2000.0 * k / SR)
    write_stereo_wav(WAV_DIR / "transient_train.wav", x, x)


def gen_noise_det():
    rng = random.Random(1337)
    x = [0.35 * (rng.random() * 2.0 - 1.0) for _ in range(N)]
    # light smoothing for deterministic "colored" character
    for i in range(1, N):
        x[i] = 0.9 * x[i - 1] + 0.1 * x[i]
    write_stereo_wav(WAV_DIR / "noise_det.wav", x, x)


def main():
    gen_silence()
    gen_sine_1khz()
    gen_dual_tone()
    gen_transient_train()
    gen_noise_det()
    print(f"Generated fixtures in: {WAV_DIR}")


if __name__ == "__main__":
    main()
