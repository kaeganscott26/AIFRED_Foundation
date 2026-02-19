#pragma once

#include "../analysis/AnalysisTypes.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace aifr3d::plugin {

class ReportExporter {
 public:
  struct ExportResult {
    bool ok{false};
    juce::String sessionId;
    juce::File sessionDir;
    juce::String error;
  };

  static ExportResult exportSnapshot(const AnalysisSnapshot& snapshot, const juce::File& baseDocumentsDir);
  static juce::String makeSessionId();
  static juce::var optionalNumber(const std::optional<double>& value);
  static juce::String severityToString(aifr3d::Severity sev);
};

}  // namespace aifr3d::plugin
