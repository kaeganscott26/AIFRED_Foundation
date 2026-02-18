#pragma once

#include <optional>
#include <string>

namespace aifr3d {

struct BenchmarkMetricTarget {
  double mean{0.0};
  std::optional<double> stddev;
  std::optional<double> target_min;
  std::optional<double> target_max;
};

struct BenchmarkBasicTargets {
  std::optional<BenchmarkMetricTarget> peak_dbfs;
  std::optional<BenchmarkMetricTarget> rms_dbfs;
  std::optional<BenchmarkMetricTarget> crest_db;
};

struct BenchmarkLoudnessTargets {
  std::optional<BenchmarkMetricTarget> integrated_lufs;
  std::optional<BenchmarkMetricTarget> short_term_lufs;
  std::optional<BenchmarkMetricTarget> loudness_range_lu;
};

struct BenchmarkSpectralTargets {
  std::optional<BenchmarkMetricTarget> sub;
  std::optional<BenchmarkMetricTarget> low;
  std::optional<BenchmarkMetricTarget> lowmid;
  std::optional<BenchmarkMetricTarget> mid;
  std::optional<BenchmarkMetricTarget> highmid;
  std::optional<BenchmarkMetricTarget> high;
  std::optional<BenchmarkMetricTarget> air;
};

struct BenchmarkStereoTargets {
  std::optional<BenchmarkMetricTarget> correlation;
  std::optional<BenchmarkMetricTarget> lr_balance_db;
  std::optional<BenchmarkMetricTarget> width_proxy;
};

struct BenchmarkDynamicsTargets {
  std::optional<BenchmarkMetricTarget> peak_dbfs;
  std::optional<BenchmarkMetricTarget> rms_dbfs;
  std::optional<BenchmarkMetricTarget> crest_db;
  std::optional<BenchmarkMetricTarget> dr_proxy_db;
};

struct BenchmarkMetrics {
  BenchmarkBasicTargets basic;
  BenchmarkLoudnessTargets loudness;
  BenchmarkSpectralTargets spectral;
  BenchmarkStereoTargets stereo;
  BenchmarkDynamicsTargets dynamics;
};

struct BenchmarkProfile {
  std::string schema_version;
  std::string genre;
  std::string profile_id;
  std::string created_at_utc;
  int track_count{0};
  std::optional<std::string> source_notes;
  BenchmarkMetrics metrics;
};

BenchmarkProfile loadBenchmarkProfileFromJson(const std::string& json_path);

}  // namespace aifr3d
