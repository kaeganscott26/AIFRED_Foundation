# aifr3d_vst3 (Phase 6)

JUCE-based VST3 wrapper for AIFR3D core analysis.

## Scope
- VST3 plugin shell only (no duplicated analysis algorithms).
- Links against `packages/aifr3d_core`.
- Pass-through audio path.
- Async analysis (captured ring buffer + offline WAV).
- Tabs: AIFRED / Analysis / Compare / Report / Settings.
- Session export to JSON + PNG charts under user Documents.

## Build system
- CMake + JUCE (`FetchContent`) in `plugins/aifr3d_vst3/CMakeLists.txt`.
- Root gate: `-DAIFR3D_ENABLE_VST3=ON`.

## Windows build steps (Visual Studio 2022)

```bash
cmake -S . -B build_win_vst3 -G "Visual Studio 17 2022" -A x64 -DAIFR3D_ENABLE_VST3=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build_win_vst3 --config Release --target aifr3d_vst3_VST3
```

Expected bundle path pattern:

```text
build_win_vst3/plugins/aifr3d_vst3/aifr3d_vst3_artefacts/Release/VST3/AIFR3D.vst3
```

## FL Studio sanity checklist (manual)
1. Copy/symlink `AIFR3D.vst3` to your scanned VST3 directory.
2. Rescan plugins in FL Studio.
3. Insert AIFR3D on a mixer slot.
4. Confirm audio passes through unchanged.
5. Open/close/reopen project and verify plugin state persists:
   - meter mode
   - last export path
   - benchmark/reference paths
   - last session ID

## Session export layout

```text
<Documents>/AIFR3D/sessions/<timestamp_uuid>/
  analysis.json
  benchmark_compare.json
  reference_compare.json   (when user reference is loaded)
  issues.json
  charts/
    spectral_bands.png
    loudness_true_peak.png
    stereo_correlation.png
```

## Notes / current limitations
- Linux CI build in this repo validates core only unless VST3 is explicitly enabled.
- Plugin compile/link and FL Studio load test must be executed on Windows.
