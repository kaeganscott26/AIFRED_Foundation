#include "aifr3d/compare.hpp"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace aifr3d {

namespace {

constexpr double kStddevEps = 1e-12;

InRangeClass classify(double value,
                      const BenchmarkMetricTarget& target,
                      std::optional<double> z) {
  if (target.target_min.has_value() && target.target_max.has_value()) {
    if (value >= *target.target_min && value <= *target.target_max) {
      return InRangeClass::IN_RANGE;
    }
    const double span = std::max(1e-9, *target.target_max - *target.target_min);
    const double dist = (value < *target.target_min) ? (*target.target_min - value)
                                                      : (value - *target.target_max);
    return (dist <= 0.10 * span) ? InRangeClass::SLIGHTLY_OFF : InRangeClass::NEEDS_ATTENTION;
  }

  if (z.has_value()) {
    const double az = std::fabs(*z);
    if (az < 1.0) {
      return InRangeClass::IN_RANGE;
    }
    if (az < 2.0) {
      return InRangeClass::SLIGHTLY_OFF;
    }
    return InRangeClass::NEEDS_ATTENTION;
  }

  return InRangeClass::UNKNOWN;
}

MetricDelta compareMetric(const std::optional<double>& value,
                          const std::optional<BenchmarkMetricTarget>& target) {
  MetricDelta out;
  out.value = value;
  if (!value.has_value() || !target.has_value()) {
    out.in_range = InRangeClass::UNKNOWN;
    return out;
  }

  out.mean = target->mean;
  out.delta = *value - target->mean;
  if (target->stddev.has_value() && *target->stddev > kStddevEps) {
    out.z = *out.delta / *target->stddev;
  }
  out.in_range = classify(*value, *target, out.z);
  return out;
}

void tally(const MetricDelta& m, BenchmarkCompareSummary& s) {
  switch (m.in_range) {
    case InRangeClass::IN_RANGE:
      ++s.in_range_count;
      break;
    case InRangeClass::SLIGHTLY_OFF:
      ++s.slightly_off_count;
      break;
    case InRangeClass::NEEDS_ATTENTION:
      ++s.needs_attention_count;
      break;
    case InRangeClass::UNKNOWN:
    default:
      ++s.unknown_count;
      break;
  }
}

}  // namespace

BenchmarkCompareResult compareAgainstBenchmark(const AnalysisResult& analysis,
                                              const BenchmarkProfile& benchmark) {
  BenchmarkCompareResult out;
  out.profile_id = benchmark.profile_id;
  out.genre = benchmark.genre;

  out.basic.peak_dbfs = compareMetric(analysis.basic.peak_dbfs, benchmark.metrics.basic.peak_dbfs);
  out.basic.rms_dbfs = compareMetric(analysis.basic.rms_dbfs, benchmark.metrics.basic.rms_dbfs);
  out.basic.crest_db = compareMetric(analysis.basic.crest_db, benchmark.metrics.basic.crest_db);

  out.loudness.integrated_lufs =
      compareMetric(analysis.loudness.integrated_lufs, benchmark.metrics.loudness.integrated_lufs);
  out.loudness.short_term_lufs =
      compareMetric(analysis.loudness.short_term_lufs, benchmark.metrics.loudness.short_term_lufs);
  out.loudness.loudness_range_lu = compareMetric(analysis.loudness.loudness_range_lu,
                                                 benchmark.metrics.loudness.loudness_range_lu);

  out.spectral.sub = compareMetric(analysis.spectral.sub, benchmark.metrics.spectral.sub);
  out.spectral.low = compareMetric(analysis.spectral.low, benchmark.metrics.spectral.low);
  out.spectral.lowmid = compareMetric(analysis.spectral.lowmid, benchmark.metrics.spectral.lowmid);
  out.spectral.mid = compareMetric(analysis.spectral.mid, benchmark.metrics.spectral.mid);
  out.spectral.highmid = compareMetric(analysis.spectral.highmid, benchmark.metrics.spectral.highmid);
  out.spectral.high = compareMetric(analysis.spectral.high, benchmark.metrics.spectral.high);
  out.spectral.air = compareMetric(analysis.spectral.air, benchmark.metrics.spectral.air);

  out.stereo.correlation =
      compareMetric(analysis.stereo.correlation, benchmark.metrics.stereo.correlation);
  out.stereo.lr_balance_db =
      compareMetric(analysis.stereo.lr_balance_db, benchmark.metrics.stereo.lr_balance_db);
  out.stereo.width_proxy =
      compareMetric(analysis.stereo.width_proxy, benchmark.metrics.stereo.width_proxy);

  out.dynamics.peak_dbfs = compareMetric(analysis.dynamics.peak_dbfs, benchmark.metrics.dynamics.peak_dbfs);
  out.dynamics.rms_dbfs = compareMetric(analysis.dynamics.rms_dbfs, benchmark.metrics.dynamics.rms_dbfs);
  out.dynamics.crest_db = compareMetric(analysis.dynamics.crest_db, benchmark.metrics.dynamics.crest_db);
  out.dynamics.dr_proxy_db =
      compareMetric(analysis.dynamics.dr_proxy_db, benchmark.metrics.dynamics.dr_proxy_db);

  const auto tally_group = [&](const auto& g) {
    using T = std::decay_t<decltype(g)>;
    if constexpr (std::is_same_v<T, CompareBasic>) {
      tally(g.peak_dbfs, out.summary);
      tally(g.rms_dbfs, out.summary);
      tally(g.crest_db, out.summary);
    } else if constexpr (std::is_same_v<T, CompareLoudness>) {
      tally(g.integrated_lufs, out.summary);
      tally(g.short_term_lufs, out.summary);
      tally(g.loudness_range_lu, out.summary);
    } else if constexpr (std::is_same_v<T, CompareSpectral>) {
      tally(g.sub, out.summary);
      tally(g.low, out.summary);
      tally(g.lowmid, out.summary);
      tally(g.mid, out.summary);
      tally(g.highmid, out.summary);
      tally(g.high, out.summary);
      tally(g.air, out.summary);
    } else if constexpr (std::is_same_v<T, CompareStereo>) {
      tally(g.correlation, out.summary);
      tally(g.lr_balance_db, out.summary);
      tally(g.width_proxy, out.summary);
    } else if constexpr (std::is_same_v<T, CompareDynamics>) {
      tally(g.peak_dbfs, out.summary);
      tally(g.rms_dbfs, out.summary);
      tally(g.crest_db, out.summary);
      tally(g.dr_proxy_db, out.summary);
    }
  };

  tally_group(out.basic);
  tally_group(out.loudness);
  tally_group(out.spectral);
  tally_group(out.stereo);
  tally_group(out.dynamics);

  return out;
}

}  // namespace aifr3d
