#pragma once

#include "aifr3d/analyzer.hpp"

#include <optional>
#include <string>
#include <vector>

namespace aifr3d {

struct ReferenceSet {
  std::vector<AnalysisResult> references;
};

struct ReferenceMetricDelta {
  std::optional<double> mix;
  std::optional<double> reference;
  std::optional<double> delta;
};

struct ReferenceBasicDeltas {
  ReferenceMetricDelta peak_dbfs;
  ReferenceMetricDelta rms_dbfs;
  ReferenceMetricDelta crest_db;
};

struct ReferenceLoudnessDeltas {
  ReferenceMetricDelta integrated_lufs;
  ReferenceMetricDelta short_term_lufs;
  ReferenceMetricDelta loudness_range_lu;
};

struct ReferenceSpectralDeltas {
  ReferenceMetricDelta sub;
  ReferenceMetricDelta low;
  ReferenceMetricDelta lowmid;
  ReferenceMetricDelta mid;
  ReferenceMetricDelta highmid;
  ReferenceMetricDelta high;
  ReferenceMetricDelta air;
};

struct ReferenceStereoDeltas {
  ReferenceMetricDelta correlation;
  ReferenceMetricDelta lr_balance_db;
  ReferenceMetricDelta width_proxy;
};

struct ReferenceDynamicsDeltas {
  ReferenceMetricDelta peak_dbfs;
  ReferenceMetricDelta rms_dbfs;
  ReferenceMetricDelta crest_db;
  ReferenceMetricDelta dr_proxy_db;
};

struct ReferenceDeltaPerTrack {
  std::size_t reference_index{0};
  ReferenceBasicDeltas basic;
  ReferenceLoudnessDeltas loudness;
  ReferenceSpectralDeltas spectral;
  ReferenceStereoDeltas stereo;
  ReferenceDynamicsDeltas dynamics;
};

struct ReferenceDistanceSummary {
  double basic_closeness_0_100{0.0};
  double loudness_closeness_0_100{0.0};
  double spectral_closeness_0_100{0.0};
  double stereo_closeness_0_100{0.0};
  double dynamics_closeness_0_100{0.0};
  double overall_closeness_0_100{0.0};
};

struct ReferenceCompareResult {
  int schema_version{0};
  std::size_t reference_count{0};
  std::vector<ReferenceDeltaPerTrack> per_reference;
  AnalysisResult reference_average;
  ReferenceDistanceSummary distance;
};

ReferenceCompareResult compareToReferences(const AnalysisResult& mix,
                                           const std::vector<AnalysisResult>& refs);

}  // namespace aifr3d
