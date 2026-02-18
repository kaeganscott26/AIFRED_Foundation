#include "aifr3d/true_peak.hpp"

#include <algorithm>
#include <cmath>

namespace aifr3d {

TruePeakMetrics compute_true_peak_interleaved_stereo(const float* interleaved_stereo,
                                                     std::size_t frame_count,
                                                     int oversample_factor) {
  TruePeakMetrics out;
  out.oversample_factor = oversample_factor > 1 ? oversample_factor : 1;

  if (interleaved_stereo == nullptr || frame_count == 0) {
    return out;
  }

  double max_abs = 0.0;
  const std::size_t channels = 2U;

  for (std::size_t c = 0; c < channels; ++c) {
    for (std::size_t f = 0; f < frame_count; ++f) {
      const std::size_t idx = f * channels + c;
      const double s0 = static_cast<double>(interleaved_stereo[idx]);
      max_abs = std::max(max_abs, std::fabs(s0));

      if (f + 1U < frame_count) {
        const std::size_t idx_next = (f + 1U) * channels + c;
        const double s1 = static_cast<double>(interleaved_stereo[idx_next]);
        for (int k = 1; k < out.oversample_factor; ++k) {
          const double t = static_cast<double>(k) / static_cast<double>(out.oversample_factor);
          const double interp = s0 + (s1 - s0) * t;
          max_abs = std::max(max_abs, std::fabs(interp));
        }
      }
    }
  }

  if (max_abs > 0.0) {
    out.true_peak_dbfs = 20.0 * std::log10(max_abs);
  }
  return out;
}

}  // namespace aifr3d
