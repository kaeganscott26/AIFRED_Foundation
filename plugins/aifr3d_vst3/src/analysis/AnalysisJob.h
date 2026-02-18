#pragma once

#include <JuceHeader.h>

#include <cstdint>

namespace aifr3d::plugin {

enum class AnalysisSourceKind {
  OfflineWav,
  CapturedBuffer,
};

struct AnalysisJob {
  std::uint64_t generation{0};
  AnalysisSourceKind sourceKind{AnalysisSourceKind::CapturedBuffer};
  juce::String trackName{"Captured Buffer"};
  juce::File offlineFile;
  juce::AudioBuffer<float> stereoBuffer;
  double sampleRateHz{48000.0};
  juce::String benchmarkProfilePath;
  juce::String referenceWavPath;
};

}  // namespace aifr3d::plugin
