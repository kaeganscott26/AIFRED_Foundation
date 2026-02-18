#include "aifr3d/analyzer.hpp"

#include "aifr3d/dynamics.hpp"
#include "aifr3d/loudness.hpp"
#include "aifr3d/spectral.hpp"
#include "aifr3d/stereo.hpp"
#include "aifr3d/true_peak.hpp"

#include <cmath>
#include <stdexcept>

namespace aifr3d {

namespace {

constexpr double kDbFloorAmplitude = 0.0;

std::optional<double> toDbFs(double linear_amplitude) {
  if (!(linear_amplitude > kDbFloorAmplitude)) {
    return std::nullopt;
  }
  return 20.0 * std::log10(linear_amplitude);
}

}  // namespace

AnalysisResult Analyzer::analyzeInterleavedStereo(const float* interleaved_stereo,
                                                  std::size_t frame_count,
                                                  double sample_rate_hz) const {
  if (!(sample_rate_hz > 0.0)) {
    throw std::invalid_argument("sample_rate_hz must be > 0");
  }
  if (frame_count > 0 && interleaved_stereo == nullptr) {
    throw std::invalid_argument("interleaved_stereo must be non-null when frame_count > 0");
  }

  double peak = 0.0;
  double energy_sum = 0.0;
  const std::size_t sample_count = frame_count * 2U;

  for (std::size_t i = 0; i < sample_count; ++i) {
    const double s = static_cast<double>(interleaved_stereo[i]);
    const double abs_s = std::fabs(s);
    if (abs_s > peak) {
      peak = abs_s;
    }
    energy_sum += s * s;
  }

  const double rms = (sample_count > 0U) ? std::sqrt(energy_sum / static_cast<double>(sample_count)) : 0.0;

  AnalysisResult out;
  out.schema_version = 1;
  out.analysis_id = std::nullopt;
  out.frame_count = frame_count;
  out.sample_rate_hz = sample_rate_hz;
  out.generated_at_utc = "1970-01-01T00:00:00Z";
  out.basic.peak_dbfs = toDbFs(peak);
  out.basic.rms_dbfs = toDbFs(rms);

  if (out.basic.peak_dbfs.has_value() && out.basic.rms_dbfs.has_value()) {
    out.basic.crest_db = *out.basic.peak_dbfs - *out.basic.rms_dbfs;
  } else {
    out.basic.crest_db = std::nullopt;
  }

  out.loudness = compute_loudness_interleaved_stereo(interleaved_stereo, frame_count, sample_rate_hz);
  out.true_peak = compute_true_peak_interleaved_stereo(interleaved_stereo, frame_count, 4);
  out.spectral = compute_spectral_bands_interleaved_stereo(interleaved_stereo, frame_count, sample_rate_hz);
  out.stereo = compute_stereo_metrics_interleaved_stereo(interleaved_stereo, frame_count);
  out.dynamics = compute_dynamics_interleaved_stereo(interleaved_stereo, frame_count);

  out.dynamics.peak_dbfs = out.basic.peak_dbfs;
  out.dynamics.rms_dbfs = out.basic.rms_dbfs;
  out.dynamics.crest_db = out.basic.crest_db;

  return out;
}

}  // namespace aifr3d
