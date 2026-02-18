#pragma once

#include "aifr3d/analyzer.hpp"
#include "aifr3d/compare.hpp"
#include "aifr3d/issues.hpp"
#include "aifr3d/reference_compare.hpp"
#include "aifr3d/scoring.hpp"

#include <JuceHeader.h>

#include <optional>

namespace aifr3d::plugin {

enum class MeterMode {
  TrueVsProPool = 0,
  TrueVsUserRef,
  UserRefVsProPool,
};

struct AnalysisSnapshot {
  bool valid{false};
  juce::String errorMessage;
  juce::String sourceLabel{"captured_buffer"};
  juce::String trackName{"Untitled"};
  juce::Time completedAt;
  double sampleRateHz{0.0};
  double durationSeconds{0.0};

  aifr3d::AnalysisResult analysis;
  std::optional<aifr3d::BenchmarkCompareResult> benchmarkCompare;
  std::optional<aifr3d::ReferenceCompareResult> referenceCompare;
  std::optional<aifr3d::ScoreBreakdown> score;
  std::optional<aifr3d::IssueReport> issues;
};

}  // namespace aifr3d::plugin
