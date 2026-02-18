# MASTER_SPEC (Append-Only Governance)

## Governance Rule
This document is append-only. Existing sections are never deleted or rewritten in-place; updates are added as dated amendments.

## Phase 0 baseline
- Monorepo root established at `AIFRED_Foundation`.
- Source repo `Free_API` remains immutable and is copied into `apps/aifred_chat`.
- Build scaffold introduced for deterministic core development without altering copied chatbot behavior.

## Change control
- Any policy or architecture changes must be appended as a new section with timestamp and rationale.
- No destructive rewrites of prior decisions.

## 2026-02-18 — Phase 3 record
- Introduced deterministic benchmark profile loading, metric delta comparison, and scoring v1 in `packages/aifr3d_core`.
- Added strict benchmark profile schema and extended analysis result schema with optional benchmark compare and score output blocks.
- Added derived-only benchmark fixture JSON under `data/benchmarks/` (no source audio content).
- Added Phase 3 tests for benchmark loading, compare logic, and scoring determinism/monotonicity.

## 2026-02-18 — Phase 4 record
- Added deterministic user reference comparison support in `packages/aifr3d_core` via `compareToReferences`.
- Added per-reference metric deltas, aggregate reference means, and normalized block/overall closeness scores.
- Added guardrails for empty reference sets and schema version compatibility checks.
- Added synthetic-buffer test coverage for reference comparison behavior and repeatability.

## 2026-02-18 — Phase 5 record
- Added deterministic issue detection + fix list rule engine v1 in `packages/aifr3d_core`.
- Added versioned rule thresholds/config and typed issue/evidence/fix-step outputs.
- Implemented top-5 ranking based on severity, normalized distance, and deterministic confidence.
- Added deterministic test coverage for issue triggering, evidence population, and repeat-run stability.

## 2026-02-18 — Phase 7 record
- Hardened VST3 async analysis path for beta shipping: cancellation, stale-job supersession, timeout handling, and UI-safe status propagation.
- Added optional non-Release profiling counters and explicit bounded-memory ring buffer telemetry.
- Introduced beta packaging workflow for Windows full-install artifact (`AIFR3D_v1.0.0-beta_Windows_FullInstall.zip`) with one-click installer flow.
- Added version record file (`VERSION = 1.0.0-beta`) and install documentation under `dist/README_install.md`.
- Added deterministic TestKit (synthetic WAV generation, analysis run script, output range verification).

## 2026-02-18 — Phase 8 record
- Added `apps/dawai_desktop` executable stub (`dawai_desktop_stub`) linked to shared `aifr3d_core`.
- Implemented session bundle loader flow for exported JSON (`analysis.json`, `issues.json`) with lightweight required-field checks.
- Implemented optional offline WAV analysis path using `aifr3d::Analyzer` to prove shared core reuse outside plugin runtime.
- Kept implementation intentionally lightweight (CLI-style dashboard) with no JUCE/plugin hosting and no full DAW behavior.
