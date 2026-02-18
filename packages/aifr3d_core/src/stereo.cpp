#include "aifr3d/stereo.hpp"

#include <algorithm>
#include <cmath>

namespace aifr3d {

StereoMetrics compute_stereo_metrics_interleaved_stereo(const float* interleaved_stereo,
                                                        std::size_t frame_count) {
  StereoMetrics out;
  if (interleaved_stereo == nullptr || frame_count == 0) {
    return out;
  }

  double sum_l = 0.0;
  double sum_r = 0.0;
  double sum_ll = 0.0;
  double sum_rr = 0.0;
  double sum_lr = 0.0;
  double sum_mid2 = 0.0;
  double sum_side2 = 0.0;

  for (std::size_t i = 0; i < frame_count; ++i) {
    const double l = static_cast<double>(interleaved_stereo[i * 2U]);
    const double r = static_cast<double>(interleaved_stereo[i * 2U + 1U]);
    sum_l += l;
    sum_r += r;
    sum_ll += l * l;
    sum_rr += r * r;
    sum_lr += l * r;
    const double mid = 0.5 * (l + r);
    const double side = 0.5 * (l - r);
    sum_mid2 += mid * mid;
    sum_side2 += side * side;
  }

  const double n = static_cast<double>(frame_count);
  const double mean_l = sum_l / n;
  const double mean_r = sum_r / n;
  const double var_l = std::max(0.0, (sum_ll / n) - mean_l * mean_l);
  const double var_r = std::max(0.0, (sum_rr / n) - mean_r * mean_r);
  const double cov = (sum_lr / n) - mean_l * mean_r;
  const double denom = std::sqrt(var_l * var_r);

  if (denom > 1e-15) {
    out.correlation = std::clamp(cov / denom, -1.0, 1.0);
  }

  const double rms_l = std::sqrt(sum_ll / n);
  const double rms_r = std::sqrt(sum_rr / n);
  if (rms_l > 0.0 && rms_r > 0.0) {
    out.lr_balance_db = 20.0 * std::log10(rms_l / rms_r);
  }

  const double rms_mid = std::sqrt(sum_mid2 / n);
  const double rms_side = std::sqrt(sum_side2 / n);
  const double denom_width = rms_mid + rms_side;
  if (denom_width > 0.0) {
    out.width_proxy = std::clamp(rms_side / denom_width, 0.0, 1.0);
  }

  return out;
}

}  // namespace aifr3d
