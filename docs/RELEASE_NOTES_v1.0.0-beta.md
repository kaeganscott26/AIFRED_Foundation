# AIFR3D Release Notes — v1.0.0-beta (Phase 7)

## Summary
This beta promotes the Phase 6 VST3 wrapper to a shippable package with stability guardrails, performance instrumentation, packaging automation, and a deterministic TestKit.

## Included in this beta

### Stability
- Async analysis remains non-blocking for UI.
- Analysis cancellation support added (queued/in-flight supersession behavior).
- Analysis timeout behavior added to prevent runaway analysis jobs.
- State and lifecycle guardrails improved for transport/reload/sample-rate transition scenarios.

### Performance
- Bounded capture ring-buffer remains enforced and telemetry surfaced.
- Optional analysis profiling counters/timing integrated and compile-gated off in Release.

### Packaging
- `VERSION` introduced: `1.0.0-beta`.
- Windows installer pipeline added using Inno Setup.
- Full-install zip contract defined:
  - `AIFR3D_v1.0.0-beta_Windows_FullInstall.zip`
  - contains `AIFR3D_Installer_v1.0.0-beta.exe` + install docs.
- CI workflow added to build/package on Windows and upload artifact/release asset.

### TestKit
- Deterministic synthetic WAV fixture generator added.
- CLI analysis runner added for fixture set.
- Range-based verification script added with pass/fail assertions.

## Validation matrix (Phase 7)
- Root CMake configure/build/test path
- TestKit generation → analysis → verification
- Windows packaging workflow (build + Inno Setup + zip artifact)

## Upgrade/install guidance
- See `dist/README_install.md` for FL Studio plugin path + scan instructions.
