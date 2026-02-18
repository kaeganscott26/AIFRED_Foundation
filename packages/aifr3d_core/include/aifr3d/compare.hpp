#pragma once

#include "aifr3d/analyzer.hpp"
#include "aifr3d/benchmark_profile.hpp"

#include <optional>
#include <string>

namespace aifr3d {

enum class InRangeClass {
  UNKNOWN = 0,
  IN_RANGE,
  SLIGHTLY_OFF,
  NEEDS_ATTENTION,
};

struct MetricDelta {
  std::optional<double> value;
  std::optional<double> mean;
  std::optional<double> delta;
  std::optional<double> z;
  InRangeClass in_range{InRangeClass::UNKNOWN};
};

struct CompareBasic {
  MetricDelta peak_dbfs;
  MetricDelta rms_dbfs;
  MetricDelta crest_db;
};

struct CompareLoudness {
  MetricDelta integrated_lufs;
  MetricDelta short_term_lufs;
  MetricDelta loudness_range_lu;
};

struct CompareSpectral {
  MetricDelta sub;
  MetricDelta low;
  MetricDelta lowmid;
  MetricDelta mid;
  MetricDelta highmid;
  MetricDelta high;
  MetricDelta air;
};

struct CompareStereo {
  MetricDelta correlation;
  MetricDelta lr_balance_db;
  MetricDelta width_proxy;
};

struct CompareDynamics {
  MetricDelta peak_dbfs;
  MetricDelta rms_dbfs;
  MetricDelta crest_db;
  MetricDelta dr_proxy_db;
};

struct BenchmarkCompareSummary {
  int in_range_count{0};
  int slightly_off_count{0};
  int needs_attention_count{0};
  int unknown_count{0};
};

struct BenchmarkCompareResult {
  std::string profile_id;
  std::string genre;
  CompareBasic basic;
  CompareLoudness loudness;
  CompareSpectral spectral;
  CompareStereo stereo;
  CompareDynamics dynamics;
  BenchmarkCompareSummary summary;
};

BenchmarkCompareResult compareAgainstBenchmark(const AnalysisResult& analysis,
                                              const BenchmarkProfile& benchmark);

}  // namespace aifr3d
