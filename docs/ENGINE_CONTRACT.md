# ENGINE_CONTRACT

## Determinism
- Core analysis routines must be stateless and reentrant.
- Given identical input samples, frame counts, sample rate, and build settings, numeric outputs must be repeatable within documented tolerances.

## Core formulas (Phase 1)
- Input format: interleaved stereo float samples in `[-1.0, 1.0]`, total samples = `frame_count * 2`.
- Peak amplitude: `peak = max(abs(sample_i))`.
- RMS amplitude: `rms = sqrt(sum(sample_i^2) / sample_count)` for `sample_count > 0`, else `0`.
- dBFS mapping:
  - `peak_dbfs = 20 * log10(peak)` when `peak > 0`
  - `rms_dbfs = 20 * log10(rms)` when `rms > 0`
  - `crest_db = peak_dbfs - rms_dbfs` when both are defined.

## Phase 2 expanded deterministic metrics

### Loudness (LUFS proxy)
- Phase 2 implements a deterministic integrated loudness proxy (not full EBU R128 gating).
- Signal energy basis:
  - `mean_square = average(sample_i^2)` over interleaved stereo samples.
  - `integrated_lufs = -0.691 + 10 * log10(mean_square)` when `mean_square > 0`.
- Short-term proxy:
  - fixed analysis window (~100 ms at 48 kHz equivalent frame count) with deterministic overlap.
  - `short_term_lufs` computed from maximum short-term window energy.
  - `loudness_range_lu = short_term_lufs - integrated_lufs` when both are defined.

### True peak (oversampled estimate)
- Oversample factor default: `4`.
- Phase 2 estimator uses linear interpolation between adjacent samples at fractional steps.
- `true_peak_dbfs = 20 * log10(max_abs_oversampled)` when peak amplitude > 0.
- This is a deterministic true-peak estimate proxy and is planned for later filter-based refinement.

### Spectral balance bands
- Analysis path: mono sum `(L + R) / 2`.
- STFT proxy:
  - Hann window
  - fixed FFT size/hop in core implementation
  - average band energies accumulated across windows
- Band boundaries (Hz):
  - `sub`: 20–60
  - `low`: 60–200
  - `lowmid`: 200–500
  - `mid`: 500–2000
  - `highmid`: 2000–6000
  - `high`: 6000–12000
  - `air`: 12000–20000
- Band values encoded as dB energy: `10 * log10(energy)` when energy > 0, else `null`.

### Stereo metrics
- Correlation:
  - `corr = cov(L,R) / (std(L)*std(R))` with epsilon guard and clamp to `[-1, 1]`.
- LR balance (optional):
  - `lr_balance_db = 20 * log10(rms(L)/rms(R))` when both RMS values are positive.
- Width proxy:
  - `mid = (L+R)/2`, `side = (L-R)/2`
  - `width_proxy = rms(side) / (rms(mid) + rms(side))`
  - bounded to `[0, 1]`.

### Dynamics proxy
- Reuses core peak/rms/crest definitions.
- Adds deterministic percentile dynamic range proxy from absolute sample distribution:
  - `dr_proxy_db = P95_dB - P10_dB` when both percentiles are defined.

### Determinism notes for Phase 2
- All metric modules are pure/stateless functions over provided buffers.
- No global mutable state and no external model/web/API dependencies.
- Deterministic tolerances in tests are explicit per metric; comparisons are exact or strict absolute bounds.

## Numeric policy for silence and non-finite values
- JSON outputs must not contain `Infinity`, `-Infinity`, or `NaN`.
- Values that would mathematically be `-inf` in dB space are encoded as `null` in JSON-facing structures.
- Internal calculations may use finite sentinel floors (for example `-300.0 dBFS`) but serialization must honor the non-infinity policy.
- Silence policy: when all samples are zero (or `frame_count == 0`), `peak_dbfs`, `rms_dbfs`, and `crest_db` are encoded as `null`.
- Phase 2 policy extension: loudness/true-peak/spectral/stereo/dynamics fields also use `null` for undefined values in silence or degenerate conditions.

## Validation
- `sample_rate_hz` must be strictly greater than zero.
- `interleaved_stereo` must be non-null whenever `frame_count > 0`.

## Tolerances
- Peak/RMS/Crest comparisons in tests use strict absolute tolerances appropriate for `float` accumulation paths.
- Tolerance values must be explicitly declared in tests and documents to preserve determinism expectations.

## Phase 3 benchmark comparison and scoring

### Benchmark profile contract
- Benchmark profiles are derived-metrics metadata only; no copyrighted audio is stored or required.
- Profile fields:
  - `schema_version`, `genre`, `profile_id`, `created_at_utc`, `track_count`, optional `source_notes`.
  - `metrics` grouped by analysis blocks (`basic`, `loudness`, `spectral`, `stereo`, `dynamics`).
- Per-metric target model:
  - required `mean`
  - optional `stddev`
  - optional `target_min`, `target_max`

### Delta and z-score rules
- For each available metric:
  - `delta = value - mean`
  - `z = delta / stddev` when `stddev` exists and `stddev > eps`
- Classification precedence:
  1. If `[target_min, target_max]` exists: range check is primary.
  2. Otherwise use z-thresholds:
     - `|z| < 1.0` => `IN_RANGE`
     - `|z| < 2.0` => `SLIGHTLY_OFF`
     - otherwise => `NEEDS_ATTENTION`
  3. Missing value/target => `UNKNOWN`.

### Scoring v1 (deterministic)
- Category subscores (0..100):
  - `loudness`, `dynamics`, `tonal_balance`, `stereo`
- Overall score:
  - weighted average of category subscores
  - default weights:
    - loudness: 0.30
    - dynamics: 0.25
    - tonal_balance: 0.25
    - stereo: 0.20
- Penalty mapping for individual metrics is deterministic and versioned (v1), based on classification and optional z-magnitude.
- Output includes transparent `ScoreBreakdown` with `overall_0_100`, per-category `subscores`, `weights`, and deterministic notes.

### Stability promise
- For identical input analysis values + benchmark profile + scoring config version, compare/scoring outputs are deterministic and repeatable for the same build/toolchain.
