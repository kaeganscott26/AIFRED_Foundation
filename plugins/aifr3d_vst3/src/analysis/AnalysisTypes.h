#pragma once

#include "aifr3d/analyzer.hpp"
#include "aifr3d/compare.hpp"
#include "aifr3d/issues.hpp"
#include "aifr3d/reference_compare.hpp"
#include "aifr3d/scoring.hpp"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <cstdint>
#include <optional>

namespace aifr3d::plugin {

enum class MeterMode {
  TrueVsProPool = 0,
  TrueVsUserRef,
  UserRefVsProPool,
};

struct AnalysisSnapshot {
  bool valid{false};
  bool canceled{false};
  bool timedOut{false};
  juce::String errorMessage;
  juce::String sourceLabel{"captured_buffer"};
  juce::String trackName{"Untitled"};
  juce::Time completedAt;
  std::uint64_t generation{0};
  double processingMs{0.0};
  double sampleRateHz{0.0};
  double durationSeconds{0.0};

  aifr3d::AnalysisResult analysis;
  std::optional<aifr3d::BenchmarkCompareResult> benchmarkCompare;
  std::optional<aifr3d::ReferenceCompareResult> referenceCompare;
  std::optional<aifr3d::ScoreBreakdown> score;
  std::optional<aifr3d::IssueReport> issues;
};

struct AnalysisPerfCounters {
  std::uint64_t submittedJobs{0};
  std::uint64_t completedJobs{0};
  std::uint64_t canceledJobs{0};
  std::uint64_t timedOutJobs{0};
  std::uint64_t droppedPendingJobs{0};
  double lastJobMs{0.0};
  double avgJobMs{0.0};
  std::uint64_t ringCapacitySamples{0};
};

}  // namespace aifr3d::plugin
