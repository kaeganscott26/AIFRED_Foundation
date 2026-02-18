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
