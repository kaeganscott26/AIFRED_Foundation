#include "aifr3d/dynamics.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace aifr3d {

namespace {

std::optional<double> to_db(double linear) {
  if (!(linear > 0.0)) {
    return std::nullopt;
  }
  return 20.0 * std::log10(linear);
}

}  // namespace

DynamicsMetrics compute_dynamics_interleaved_stereo(const float* interleaved_stereo,
                                                    std::size_t frame_count) {
  DynamicsMetrics out;
  if (interleaved_stereo == nullptr || frame_count == 0) {
    return out;
  }

  const std::size_t sample_count = frame_count * 2U;
  double peak = 0.0;
  double sum_sq = 0.0;
  std::vector<double> abs_values;
  abs_values.reserve(sample_count);

  for (std::size_t i = 0; i < sample_count; ++i) {
    const double s = static_cast<double>(interleaved_stereo[i]);
    const double a = std::fabs(s);
    peak = std::max(peak, a);
    sum_sq += s * s;
    abs_values.push_back(a);
  }

  const double rms = std::sqrt(sum_sq / static_cast<double>(sample_count));
  out.peak_dbfs = to_db(peak);
  out.rms_dbfs = to_db(rms);
  if (out.peak_dbfs.has_value() && out.rms_dbfs.has_value()) {
    out.crest_db = *out.peak_dbfs - *out.rms_dbfs;
  }

  std::sort(abs_values.begin(), abs_values.end());
  if (!abs_values.empty()) {
    const std::size_t p10_idx = static_cast<std::size_t>((abs_values.size() - 1U) * 0.10);
    const std::size_t p95_idx = static_cast<std::size_t>((abs_values.size() - 1U) * 0.95);
    const auto p10_db = to_db(abs_values[p10_idx]);
    const auto p95_db = to_db(abs_values[p95_idx]);
    if (p10_db.has_value() && p95_db.has_value()) {
      out.dr_proxy_db = *p95_db - *p10_db;
    }
  }

  return out;
}

}  // namespace aifr3d
