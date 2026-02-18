# ENGINE_CONTRACT

## Determinism
- Core analysis routines must be stateless and reentrant.
- Given identical input samples, frame counts, sample rate, and build settings, numeric outputs must be repeatable within documented tolerances.

## Numeric policy for silence and non-finite values
- JSON outputs must not contain `Infinity`, `-Infinity`, or `NaN`.
- Values that would mathematically be `-inf` in dB space are encoded as `null` in JSON-facing structures.
- Internal calculations may use finite sentinel floors (for example `-300.0 dBFS`) but serialization must honor the non-infinity policy.

## Tolerances
- Peak/RMS/Crest comparisons in tests use strict absolute tolerances appropriate for `float` accumulation paths.
- Tolerance values must be explicitly declared in tests and documents to preserve determinism expectations.
