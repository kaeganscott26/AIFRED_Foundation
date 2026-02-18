#pragma once

#include "aifr3d/analyzer.hpp"
#include "aifr3d/compare.hpp"
#include "aifr3d/reference_compare.hpp"

#include <optional>
#include <string>
#include <vector>

namespace aifr3d {

enum class Severity {
  Info = 0,
  Minor,
  Moderate,
  Severe,
};

struct EvidencePoint {
  std::string metric_path;
  std::optional<double> value;
  std::optional<double> target_mean;
  std::optional<double> delta;
  std::optional<double> z;
  std::optional<std::string> in_range;
};

struct FixStep {
  std::string title;
  std::string action;
  std::string when;
  std::string how;
  std::string recheck;
};

struct IssueItem {
  std::string id;
  std::string title;
  Severity severity{Severity::Info};
  double confidence_0_100{0.0};
  std::string summary;
  std::vector<EvidencePoint> evidence;
  std::vector<FixStep> fix_steps;
};

struct IssueReport {
  std::string schema_version{"0.5.0"};
  std::vector<IssueItem> top_issues;
  std::string generated_at_utc{"1970-01-01T00:00:00Z"};
};

struct ImprovementPlanDay {
  int day_index{1};
  std::string focus;
  std::vector<FixStep> steps;
};

struct ImprovementPlan3Day {
  std::vector<ImprovementPlanDay> days;
};

IssueReport generateIssues(const AnalysisResult& mix,
                           const BenchmarkCompareResult* bench,
                           const ReferenceCompareResult* refs);

}  // namespace aifr3d
