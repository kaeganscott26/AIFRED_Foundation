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
