#pragma once

#include "../analysis/AnalysisTypes.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace aifr3d::plugin {

class ChartRenderer {
 public:
  static juce::Image renderSpectralBands(const AnalysisSnapshot& snapshot, int width, int height);
  static juce::Image renderLoudnessTruePeak(const AnalysisSnapshot& snapshot, int width, int height);
  static juce::Image renderStereoCorrelation(const AnalysisSnapshot& snapshot, int width, int height);

 private:
  static void drawPanelBackground(juce::Graphics& g, juce::Rectangle<int> area);
  static juce::Colour accentColor();
};

}  // namespace aifr3d::plugin
