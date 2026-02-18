# AIFR3D Phase 7 TestKit

This TestKit provides deterministic synthetic WAV fixtures and expected output checks for beta validation.

## Contents
- `wav/` generated non-copyrighted fixtures
- `expected/ranges.json` expected metric ranges and basic assertions
- `out/` generated analysis output (gitignored)
- `tools/` scripts for generation + analysis + verification

## Scenarios covered
- Silence edge case
- Mono-like sine tone duplicated to stereo
- Dual-tone content
- Transient train (spiky dynamics)
- Broadband pseudo-noise (deterministic RNG)

## Quick run
From repo root:

```bash
python plugins/aifr3d_vst3/TestKit/tools/generate_test_wavs.py
python plugins/aifr3d_vst3/TestKit/tools/run_testkit_analysis.py --build-dir build_phase7
python plugins/aifr3d_vst3/TestKit/tools/verify_testkit_outputs.py
```

If your build directory differs, pass `--build-dir` accordingly.
