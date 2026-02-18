#include "aifr3d/issues.hpp"

#include "aifr3d/rules.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace aifr3d {

namespace {

double clamp(double x, double lo, double hi) {
  return std::max(lo, std::min(hi, x));
}

std::string inRangeToString(InRangeClass c) {
  switch (c) {
    case InRangeClass::IN_RANGE:
      return "IN_RANGE";
    case InRangeClass::SLIGHTLY_OFF:
      return "SLIGHTLY_OFF";
    case InRangeClass::NEEDS_ATTENTION:
      return "NEEDS_ATTENTION";
    case InRangeClass::UNKNOWN:
    default:
      return "UNKNOWN";
  }
}

double severityWeight(Severity s) {
  switch (s) {
    case Severity::Info:
      return 0.8;
    case Severity::Minor:
      return 1.0;
    case Severity::Moderate:
      return 1.5;
    case Severity::Severe:
      return 2.0;
    default:
      return 1.0;
  }
}

Severity severityFromNormDistance(double norm_distance) {
  if (norm_distance >= 2.5) {
    return Severity::Severe;
  }
  if (norm_distance >= 1.5) {
    return Severity::Moderate;
  }
  if (norm_distance >= 0.7) {
    return Severity::Minor;
  }
  return Severity::Info;
}

double confidenceFromNormDistance(double norm_distance, bool has_reference_support) {
  double base = 55.0 + 15.0 * clamp(norm_distance, 0.0, 3.0);
  if (has_reference_support) {
    base += 10.0;
  }
  return clamp(base, 0.0, 100.0);
}

EvidencePoint evidenceFromMetric(const std::string& path, const MetricDelta& m) {
  EvidencePoint e;
  e.metric_path = path;
  e.value = m.value;
  e.target_mean = m.mean;
  e.delta = m.delta;
  e.z = m.z;
  e.in_range = inRangeToString(m.in_range);
  return e;
}

bool refSupportsDirection(const ReferenceCompareResult* refs,
                          const std::optional<double>& mix_minus_target,
                          const std::optional<double>& mix_minus_ref_mean) {
  if (refs == nullptr || !mix_minus_target.has_value() || !mix_minus_ref_mean.has_value()) {
    return false;
  }
  return ((*mix_minus_target >= 0.0 && *mix_minus_ref_mean >= 0.0) ||
          (*mix_minus_target <= 0.0 && *mix_minus_ref_mean <= 0.0));
}

std::optional<double> optionalDelta(const std::optional<double>& a, const std::optional<double>& b) {
  if (!a.has_value() || !b.has_value()) {
    return std::nullopt;
  }
  return *a - *b;
}

struct RankedIssue {
  IssueItem item;
  double ranking_score{0.0};
};

void addFixTemplate(std::vector<FixStep>& out,
                    const std::string& title,
                    const std::string& action,
                    const std::string& when,
                    const std::string& how,
                    const std::string& recheck) {
  out.push_back(FixStep{title, action, when, how, recheck});
}

}  // namespace

IssueReport generateIssues(const AnalysisResult& mix,
                           const BenchmarkCompareResult* bench,
                           const ReferenceCompareResult* refs) {
  const RuleEngineConfigV1 cfg = getDefaultRuleEngineConfigV1();
  std::vector<RankedIssue> ranked;

  if (bench != nullptr) {
    // Rule 1: Loudness hot/cold
    if (bench->loudness.integrated_lufs.delta.has_value()) {
      const double d = *bench->loudness.integrated_lufs.delta;
      if (d >= cfg.loudness_hot_delta_lu || d <= cfg.loudness_cold_delta_lu) {
        const bool is_hot = d > 0.0;
        const double norm_distance = std::fabs(d) / std::max(1e-9, std::fabs(cfg.loudness_hot_delta_lu));
        const std::optional<double> mix_minus_ref_mean =
            refs ? optionalDelta(bench->loudness.integrated_lufs.value,
                                 refs->reference_average.loudness.integrated_lufs)
                 : std::nullopt;
        const bool ref_support =
            refSupportsDirection(refs, bench->loudness.integrated_lufs.delta, mix_minus_ref_mean);
        IssueItem item;
        item.id = is_hot ? "LOUDNESS_TOO_HOT" : "LOUDNESS_TOO_COLD";
        item.title = is_hot ? "Integrated loudness is above target" : "Integrated loudness is below target";
        item.severity = severityFromNormDistance(norm_distance);
        item.confidence_0_100 = confidenceFromNormDistance(norm_distance, ref_support);
        item.summary = is_hot ? "Why: integrated LUFS exceeds benchmark mean/target; What: reduce loudness with gain/limiter trim."
                              : "Why: integrated LUFS trails benchmark mean/target; What: raise controlled loudness."
                              ;
        item.evidence.push_back(evidenceFromMetric("loudness.integrated_lufs", bench->loudness.integrated_lufs));
        addFixTemplate(item.fix_steps,
                       "Set loudness direction",
                       is_hot ? "Reduce integrated loudness by lowering bus gain or limiter input."
                              : "Increase integrated loudness with gentle bus gain and controlled limiting.",
                       "When final gain staging before export",
                       "Adjust in 0.5 dB steps, render short loop, avoid clipping side effects.",
                       is_hot ? "Recheck loudness.integrated_lufs moves down toward target range."
                              : "Recheck loudness.integrated_lufs moves up toward target range.");

        const double ranking = severityWeight(item.severity) * norm_distance * (item.confidence_0_100 / 100.0);
        ranked.push_back(RankedIssue{item, ranking});
      }
    }

    // Rule 3: Low-end buildup
    if (bench->spectral.sub.delta.has_value() && bench->spectral.low.delta.has_value()) {
      const double dsub = *bench->spectral.sub.delta;
      const double dlow = *bench->spectral.low.delta;
      const double maxd = std::max(dsub, dlow);
      if (maxd >= cfg.low_end_buildup_delta_db) {
        const double norm_distance = maxd / cfg.low_end_buildup_delta_db;
        IssueItem item;
        item.id = "LOW_END_BUILDUP";
        item.title = "Low-end buildup above benchmark";
        item.severity = severityFromNormDistance(norm_distance);
        item.confidence_0_100 = confidenceFromNormDistance(norm_distance, false);
        item.summary = "Why: sub/low bands are elevated versus benchmark. What: tighten low-end EQ/dynamics.";
        item.evidence.push_back(evidenceFromMetric("spectral.sub", bench->spectral.sub));
        item.evidence.push_back(evidenceFromMetric("spectral.low", bench->spectral.low));
        addFixTemplate(item.fix_steps,
                       "Control low-end energy",
                       "Apply gentle low-shelf or dynamic EQ in sub/low region.",
                       "When balancing rhythm section",
                       "Cut 0.5-2.0 dB in offending band; prefer dynamic control if pumping occurs.",
                       "Recheck spectral.sub and spectral.low deltas decrease toward zero.");
        const double ranking = severityWeight(item.severity) * norm_distance * (item.confidence_0_100 / 100.0);
        ranked.push_back(RankedIssue{item, ranking});
      }
    }

    // Rule 4: Harsh mids
    if (bench->spectral.highmid.delta.has_value() && *bench->spectral.highmid.delta >= cfg.harsh_mids_delta_db) {
      const double norm_distance = *bench->spectral.highmid.delta / cfg.harsh_mids_delta_db;
      IssueItem item;
      item.id = "HARSH_MIDS";
      item.title = "High-mid harshness above target";
      item.severity = severityFromNormDistance(norm_distance);
      item.confidence_0_100 = confidenceFromNormDistance(norm_distance, false);
      item.summary = "Why: high-mid band exceeds target. What: soften high-mid content with EQ or dynamic EQ.";
      item.evidence.push_back(evidenceFromMetric("spectral.highmid", bench->spectral.highmid));
      addFixTemplate(item.fix_steps,
                     "Reduce high-mid harshness",
                     "Apply narrow attenuation in high-mid range or dynamic suppression.",
                     "When taming vocal/instrument bite",
                     "Start with 1 dB reduction and widen Q until harshness drops without dulling.",
                     "Recheck spectral.highmid delta trends down toward target.");
      const double ranking = severityWeight(item.severity) * norm_distance * (item.confidence_0_100 / 100.0);
      ranked.push_back(RankedIssue{item, ranking});
    }

    // Rule 5: Dull top
    const std::optional<double> dull_metric =
        (bench->spectral.air.delta.has_value() ? bench->spectral.air.delta : bench->spectral.high.delta);
    if (dull_metric.has_value() && *dull_metric <= cfg.dull_top_delta_db) {
      const double norm_distance = std::fabs(*dull_metric) / std::fabs(cfg.dull_top_delta_db);
      IssueItem item;
      item.id = "DULL_TOP";
      item.title = "Top-end energy below target";
      item.severity = severityFromNormDistance(norm_distance);
      item.confidence_0_100 = confidenceFromNormDistance(norm_distance, false);
      item.summary = "Why: high/air spectral energy is below benchmark. What: restore top-end presence carefully.";
      if (bench->spectral.air.delta.has_value()) {
        item.evidence.push_back(evidenceFromMetric("spectral.air", bench->spectral.air));
      }
      if (bench->spectral.high.delta.has_value()) {
        item.evidence.push_back(evidenceFromMetric("spectral.high", bench->spectral.high));
      }
      addFixTemplate(item.fix_steps,
                     "Lift top-end",
                     "Use gentle high-shelf or exciters conservatively.",
                     "When final tonal polish",
                     "Boost in small increments and verify no brittle side effects.",
                     "Recheck spectral.high/air deltas rise toward zero.");
      const double ranking = severityWeight(item.severity) * norm_distance * (item.confidence_0_100 / 100.0);
      ranked.push_back(RankedIssue{item, ranking});
    }

    // Rule 6: Stereo width
    bool width_issue = false;
    double width_norm = 0.0;
    IssueItem width_item;
    if (bench->stereo.width_proxy.delta.has_value() &&
        std::fabs(*bench->stereo.width_proxy.delta) >= cfg.stereo_width_delta_limit) {
      width_issue = true;
      width_norm = std::fabs(*bench->stereo.width_proxy.delta) / cfg.stereo_width_delta_limit;
      width_item.evidence.push_back(evidenceFromMetric("stereo.width_proxy", bench->stereo.width_proxy));
    }
    if (bench->stereo.correlation.value.has_value() && *bench->stereo.correlation.value < cfg.stereo_correlation_low) {
      width_issue = true;
      width_norm = std::max(width_norm,
                            std::fabs(*bench->stereo.correlation.value - cfg.stereo_correlation_low) /
                                std::max(1e-9, std::fabs(cfg.stereo_correlation_low)));
      width_item.evidence.push_back(evidenceFromMetric("stereo.correlation", bench->stereo.correlation));
    }
    if (width_issue) {
      width_item.id = "STEREO_WIDTH_IMBALANCE";
      width_item.title = "Stereo field outside target behavior";
      width_item.severity = severityFromNormDistance(width_norm);
      width_item.confidence_0_100 = confidenceFromNormDistance(width_norm, false);
      width_item.summary =
          "Why: stereo width/correlation is outside expected range. What: adjust width processing and mono compatibility.";
      addFixTemplate(width_item.fix_steps,
                     "Normalize stereo image",
                     "Reduce over-wide side gain or increase side width if too narrow.",
                     "When spatial balance is finalized",
                     "Adjust M/S balance incrementally; validate mono compatibility each change.",
                     "Recheck stereo.width_proxy and stereo.correlation move toward benchmark range.");
      const double ranking =
          severityWeight(width_item.severity) * width_norm * (width_item.confidence_0_100 / 100.0);
      ranked.push_back(RankedIssue{width_item, ranking});
    }

    // Rule 7: Dynamics crushed/spiky
    if (bench->dynamics.crest_db.delta.has_value()) {
      const double d = *bench->dynamics.crest_db.delta;
      if (d <= cfg.dynamics_crest_low_delta_db || d >= cfg.dynamics_crest_high_delta_db) {
        const bool crushed = d < 0.0;
        const double norm_distance = crushed ? std::fabs(d / cfg.dynamics_crest_low_delta_db)
                                             : std::fabs(d / cfg.dynamics_crest_high_delta_db);
        IssueItem item;
        item.id = crushed ? "DYNAMICS_TOO_CRUSHED" : "DYNAMICS_TOO_SPIKY";
        item.title = crushed ? "Dynamics are over-compressed" : "Dynamics are overly spiky";
        item.severity = severityFromNormDistance(norm_distance);
        item.confidence_0_100 = confidenceFromNormDistance(norm_distance, false);
        item.summary = crushed ? "Why: crest factor is below target. What: restore transient/dynamic range."
                               : "Why: crest factor is above target. What: control transient peaks.";
        item.evidence.push_back(evidenceFromMetric("dynamics.crest_db", bench->dynamics.crest_db));
        addFixTemplate(item.fix_steps,
                       "Rebalance dynamics",
                       crushed ? "Relax bus compression/limiting and recover transient headroom."
                               : "Apply moderate compression/limiting to tame peak excursions.",
                       "When setting bus dynamics",
                       "Adjust threshold/ratio in small steps while preserving tone.",
                       crushed ? "Recheck dynamics.crest_db increases toward target range."
                               : "Recheck dynamics.crest_db decreases toward target range.");
        const double ranking = severityWeight(item.severity) * norm_distance * (item.confidence_0_100 / 100.0);
        ranked.push_back(RankedIssue{item, ranking});
      }
    }
  }

  // Rule 2: True peak risk (mix-level check, with/without benchmark)
  if (mix.true_peak.true_peak_dbfs.has_value() && *mix.true_peak.true_peak_dbfs > cfg.true_peak_risk_dbfs) {
    const double norm_distance =
        (*mix.true_peak.true_peak_dbfs - cfg.true_peak_risk_dbfs) / std::max(1e-9, std::fabs(cfg.true_peak_risk_dbfs));
    IssueItem item;
    item.id = "TRUE_PEAK_RISK";
    item.title = "True peak exceeds safe headroom";
    item.severity = severityFromNormDistance(norm_distance + 1.0);
    item.confidence_0_100 = confidenceFromNormDistance(norm_distance + 1.0, false);
    item.summary = "Why: true peak is above safety threshold. What: reduce ceiling/input to avoid clipping risk.";
    EvidencePoint e;
    e.metric_path = "true_peak.true_peak_dbfs";
    e.value = mix.true_peak.true_peak_dbfs;
    e.target_mean = cfg.true_peak_risk_dbfs;
    e.delta = *mix.true_peak.true_peak_dbfs - cfg.true_peak_risk_dbfs;
    e.in_range = (*mix.true_peak.true_peak_dbfs <= cfg.true_peak_risk_dbfs) ? "IN_RANGE" : "NEEDS_ATTENTION";
    item.evidence.push_back(e);
    addFixTemplate(item.fix_steps,
                   "Reduce true peak",
                   "Lower limiter ceiling or bus gain to restore headroom.",
                   "When final loudness limiting",
                   "Trim 0.3-1.0 dB and verify transient overs stay contained.",
                   "Recheck true_peak.true_peak_dbfs moves down to <= threshold.");
    const double ranking = severityWeight(item.severity) * (norm_distance + 1.0) * (item.confidence_0_100 / 100.0);
    ranked.push_back(RankedIssue{item, ranking});
  }

  std::stable_sort(ranked.begin(), ranked.end(), [](const RankedIssue& a, const RankedIssue& b) {
    if (a.ranking_score == b.ranking_score) {
      return a.item.id < b.item.id;
    }
    return a.ranking_score > b.ranking_score;
  });

  IssueReport out;
  out.schema_version = "0.5.0";
  out.generated_at_utc = "1970-01-01T00:00:00Z";
  for (std::size_t i = 0; i < ranked.size() && i < 5U; ++i) {
    out.top_issues.push_back(ranked[i].item);
  }
  return out;
}

}  // namespace aifr3d
