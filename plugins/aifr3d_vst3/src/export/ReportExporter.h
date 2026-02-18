#pragma once

#include "../analysis/AnalysisTypes.h"

#include <JuceHeader.h>

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
