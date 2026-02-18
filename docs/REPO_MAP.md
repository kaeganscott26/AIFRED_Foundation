# AIFRED Foundation Repository Map

## Top-level layout
- `apps/` application entrypoints and product-facing executables
  - `apps/aifred_chat/` immutable runtime copy of `Free_API` for transition safety
  - `apps/dawai_desktop/` Phase 8 lightweight desktop stub using shared `aifr3d_core`
- `packages/` shared libraries, contracts, and schemas
  - `packages/aifr3d_core/` deterministic audio analysis core (Phase 0 scaffold)
  - `packages/aifr3d_ai/` AI orchestration contracts only (Phase 0)
  - `packages/schemas/` JSON schema placeholders and stubs
- `plugins/` plugin wrappers and delivery targets
- `data/` benchmark datasets and corpus manifests
- `docs/` governance, contracts, architecture, and specs
- `scripts/` verification and automation helpers
- `cmake/` centralized toolchain/compiler settings
- `archive/` non-destructive parking area when historical moves are required

## Windows VST3 builds from GitHub Actions
- Workflow: `.github/workflows/windows-build.yml`
- Trigger options:
  - Manual run from **Actions → windows-vst3-build → Run workflow**
  - Automatic on pushed tags matching `v*`
- Build configuration:
  - Runner: `windows-latest`
  - CMake generator: `Visual Studio 17 2022` x64
  - VST3 enabled via `-DAIFR3D_ENABLE_VST3=ON`
- Output artifact:
  - Artifact name: `AIFR3D_v<version>_Windows`
  - Zip filename: `AIFR3D_v<version>_Windows.zip`
  - Zip contents: `AIFR3D.vst3` bundle + `README_install.md`
- Release behavior:
  - On tag builds, the same zip is attached to the GitHub Release.
