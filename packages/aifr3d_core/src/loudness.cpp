#include "aifr3d/loudness.hpp"

#include <cmath>

namespace aifr3d {

LoudnessMetrics compute_loudness_interleaved_stereo(const float* interleaved_stereo,
                                                    std::size_t frame_count,
                                                    double sample_rate_hz) {
  (void)sample_rate_hz;

  LoudnessMetrics out;
  if (interleaved_stereo == nullptr || frame_count == 0) {
    return out;
  }

  double mean_square = 0.0;
  double max_short_term_ms = 0.0;
  const std::size_t sample_count = frame_count * 2U;

  for (std::size_t i = 0; i < sample_count; ++i) {
    const double s = static_cast<double>(interleaved_stereo[i]);
    mean_square += s * s;
  }
  mean_square /= static_cast<double>(sample_count);

  constexpr std::size_t window_frames = 4800;  // ~100ms @ 48kHz proxy window
  if (frame_count >= window_frames) {
    for (std::size_t start = 0; start + window_frames <= frame_count; start += window_frames / 2U) {
      double wms = 0.0;
      for (std::size_t f = 0; f < window_frames; ++f) {
        const std::size_t idx = (start + f) * 2U;
        const double l = static_cast<double>(interleaved_stereo[idx]);
        const double r = static_cast<double>(interleaved_stereo[idx + 1U]);
        const double mono = 0.5 * (l + r);
        wms += mono * mono;
      }
      wms /= static_cast<double>(window_frames);
      if (wms > max_short_term_ms) {
        max_short_term_ms = wms;
      }
    }
  } else {
    max_short_term_ms = mean_square;
  }

  // Phase 2 deterministic LUFS proxy (not full EBU R128)
  if (mean_square > 0.0) {
    out.integrated_lufs = -0.691 + 10.0 * std::log10(mean_square);
  }
  if (max_short_term_ms > 0.0) {
    out.short_term_lufs = -0.691 + 10.0 * std::log10(max_short_term_ms);
  }
  if (out.short_term_lufs.has_value() && out.integrated_lufs.has_value()) {
    out.loudness_range_lu = *out.short_term_lufs - *out.integrated_lufs;
  }

  return out;
}

}  // namespace aifr3d
