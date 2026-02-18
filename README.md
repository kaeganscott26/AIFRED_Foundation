# AIFRED Foundation

## Phase 7 Beta (AIFR3D VST3)

### One-step Windows installer download
Use the latest GitHub release asset:

- `AIFR3D_v1.0.0-beta_Windows_FullInstall.zip`

It contains:
- `AIFR3D_Installer_v1.0.0-beta.exe`
- install instructions (`README_install.md`)

After install, scan in FL Studio plugin manager (see `dist/README_install.md`).

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Phase 7 verification (includes TestKit)

```bash
RUN_TESTKIT=1 bash scripts/verify_phase_build.sh
```
