#pragma once

#include <string>

namespace aifr3d {

struct RuleEngineConfigV1 {
  std::string version{"v1"};

  double loudness_hot_delta_lu{1.0};
  double loudness_cold_delta_lu{-1.0};
  double true_peak_risk_dbfs{-1.0};
  double low_end_buildup_delta_db{2.0};
  double harsh_mids_delta_db{1.5};
  double dull_top_delta_db{-1.5};
  double stereo_width_delta_limit{0.15};
  double stereo_correlation_low{-0.10};
  double dynamics_crest_low_delta_db{-2.0};
  double dynamics_crest_high_delta_db{2.0};
};

RuleEngineConfigV1 getDefaultRuleEngineConfigV1();

}  // namespace aifr3d
