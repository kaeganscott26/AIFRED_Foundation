#include "aifr3d/scoring.hpp"

#include <algorithm>
#include <cmath>

namespace aifr3d {

namespace {

double scoreMetric(const MetricDelta& m) {
  switch (m.in_range) {
    case InRangeClass::IN_RANGE:
      return 100.0;
    case InRangeClass::SLIGHTLY_OFF: {
      if (m.z.has_value()) {
        return std::max(60.0, 100.0 - 20.0 * std::fabs(*m.z));
      }
      return 70.0;
    }
    case InRangeClass::NEEDS_ATTENTION: {
      if (m.z.has_value()) {
        return std::max(0.0, 60.0 - 12.5 * std::fabs(*m.z));
      }
      return 35.0;
    }
    case InRangeClass::UNKNOWN:
    default:
      return 50.0;
  }
}

double average(std::initializer_list<double> vals) {
  double s = 0.0;
  std::size_t n = 0;
  for (double v : vals) {
    s += v;
    ++n;
  }
  return (n > 0U) ? (s / static_cast<double>(n)) : 0.0;
}

}  // namespace

ScoreBreakdown computeScore(const BenchmarkCompareResult& compare, const ScoreConfig& config) {
  ScoreBreakdown out;
  out.version = config.version;
  out.weights = config.weights;

  const double loudness = average({
      scoreMetric(compare.loudness.integrated_lufs),
      scoreMetric(compare.loudness.short_term_lufs),
      scoreMetric(compare.loudness.loudness_range_lu),
  });

  const double dynamics = average({
      scoreMetric(compare.dynamics.peak_dbfs),
      scoreMetric(compare.dynamics.rms_dbfs),
      scoreMetric(compare.dynamics.crest_db),
      scoreMetric(compare.dynamics.dr_proxy_db),
  });

  const double tonal_balance = average({
      scoreMetric(compare.spectral.sub),
      scoreMetric(compare.spectral.low),
      scoreMetric(compare.spectral.lowmid),
      scoreMetric(compare.spectral.mid),
      scoreMetric(compare.spectral.highmid),
      scoreMetric(compare.spectral.high),
      scoreMetric(compare.spectral.air),
  });

  const double stereo = average({
      scoreMetric(compare.stereo.correlation),
      scoreMetric(compare.stereo.lr_balance_db),
      scoreMetric(compare.stereo.width_proxy),
  });

  out.subscores["loudness"] = loudness;
  out.subscores["dynamics"] = dynamics;
  out.subscores["tonal_balance"] = tonal_balance;
  out.subscores["stereo"] = stereo;

  double total_weight = 0.0;
  double weighted_sum = 0.0;
  for (const auto& [name, subscore] : out.subscores) {
    const auto it = config.weights.find(name);
    const double w = (it != config.weights.end()) ? it->second : 0.0;
    weighted_sum += w * subscore;
    total_weight += w;
  }

  out.overall_0_100 = (total_weight > 0.0) ? (weighted_sum / total_weight) : 0.0;
  out.overall_0_100 = std::max(0.0, std::min(100.0, out.overall_0_100));

  out.notes.push_back("scoring_version=" + out.version);
  out.notes.push_back("profile_id=" + compare.profile_id);
  out.notes.push_back("classification_counts=in_range:" + std::to_string(compare.summary.in_range_count) +
                      ",slightly_off:" + std::to_string(compare.summary.slightly_off_count) +
                      ",needs_attention:" + std::to_string(compare.summary.needs_attention_count) +
                      ",unknown:" + std::to_string(compare.summary.unknown_count));

  return out;
}

}  // namespace aifr3d
