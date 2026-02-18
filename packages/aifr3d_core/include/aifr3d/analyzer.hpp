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

struct AnalysisResult {
  int schema_version{1};
  std::optional<std::string> analysis_id;
  std::size_t frame_count{0};
  double sample_rate_hz{0.0};
  std::string generated_at_utc;
  BasicMetrics basic;
};

class Analyzer {
 public:
  AnalysisResult analyzeInterleavedStereo(const float* interleaved_stereo,
                                          std::size_t frame_count,
                                          double sample_rate_hz) const;
};

}  // namespace aifr3d
