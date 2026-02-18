# AIFR3D Known Issues (v1.0.0-beta)

## Platform/runtime
- Full plugin load/reload validation in FL Studio is Windows-only and must be performed on target host.
- Unsigned installer binaries may trigger SmartScreen prompts in beta.

## Analysis behavior
- Timeout protects UI responsiveness but can terminate very large/slow offline analyses.
- Cancellation is cooperative: already-computed intermediate steps may complete before stale result is discarded.

## Packaging/distribution
- Linux/macOS do not generate the Windows `.exe` installer directly in this repo workflow.
- Final Windows full-install zip is produced by GitHub Actions Windows runner (or local Windows machine with Inno Setup).

## Non-goals in this phase
- No DawAI Phase 8 integration included.
- No signed installer/notarization process included in beta scope.
