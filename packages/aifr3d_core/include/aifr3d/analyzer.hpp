#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace aifr3d {

struct BasicMetrics {
  std::optional<double> peak_dbfs;
  std::optional<double> rms_dbfs;
  std::optional<double> crest_db;
};

struct LoudnessMetrics {
  std::optional<double> integrated_lufs;
  std::optional<double> short_term_lufs;
  std::optional<double> loudness_range_lu;
};

struct TruePeakMetrics {
  std::optional<double> true_peak_dbfs;
  int oversample_factor{4};
};

struct SpectralBands {
  std::optional<double> sub;
  std::optional<double> low;
  std::optional<double> lowmid;
  std::optional<double> mid;
  std::optional<double> highmid;
  std::optional<double> high;
  std::optional<double> air;
};

struct StereoMetrics {
  std::optional<double> correlation;
  std::optional<double> lr_balance_db;
  std::optional<double> width_proxy;
};

struct DynamicsMetrics {
  std::optional<double> peak_dbfs;
  std::optional<double> rms_dbfs;
  std::optional<double> crest_db;
  std::optional<double> dr_proxy_db;
};

struct AnalysisResult {
  int schema_version{1};
  std::optional<std::string> analysis_id;
  std::size_t frame_count{0};
  double sample_rate_hz{0.0};
  std::string generated_at_utc;
  BasicMetrics basic;
  LoudnessMetrics loudness;
  TruePeakMetrics true_peak;
  SpectralBands spectral;
  StereoMetrics stereo;
  DynamicsMetrics dynamics;
};

class Analyzer {
 public:
  AnalysisResult analyzeInterleavedStereo(const float* interleaved_stereo,
                                          std::size_t frame_count,
                                          double sample_rate_hz) const;
};

}  // namespace aifr3d
