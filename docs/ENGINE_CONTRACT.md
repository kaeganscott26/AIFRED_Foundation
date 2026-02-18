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

## Numeric policy for silence and non-finite values
- JSON outputs must not contain `Infinity`, `-Infinity`, or `NaN`.
- Values that would mathematically be `-inf` in dB space are encoded as `null` in JSON-facing structures.
- Internal calculations may use finite sentinel floors (for example `-300.0 dBFS`) but serialization must honor the non-infinity policy.
- Silence policy: when all samples are zero (or `frame_count == 0`), `peak_dbfs`, `rms_dbfs`, and `crest_db` are encoded as `null`.

## Validation
- `sample_rate_hz` must be strictly greater than zero.
- `interleaved_stereo` must be non-null whenever `frame_count > 0`.

## Tolerances
- Peak/RMS/Crest comparisons in tests use strict absolute tolerances appropriate for `float` accumulation paths.
- Tolerance values must be explicitly declared in tests and documents to preserve determinism expectations.
