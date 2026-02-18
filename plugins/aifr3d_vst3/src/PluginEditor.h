#pragma once

#include "PluginProcessor.h"

#include <JuceHeader.h>

#include <cstdint>
#include <memory>

namespace aifr3d::plugin {

class MetricBarsComponent;
class DualSourceMeterComponent;

class Aifr3dAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer,
                                         private juce::Button::Listener {
 public:
  explicit Aifr3dAudioProcessorEditor(Aifr3dAudioProcessor& processor);
  ~Aifr3dAudioProcessorEditor() override;

  void paint(juce::Graphics& g) override;
  void resized() override;

 private:
  void timerCallback() override;
  void buttonClicked(juce::Button* button) override;

  void refreshFromSnapshot();
  void appendSessionHistory(const juce::String& line);

  Aifr3dAudioProcessor& processor_;

  juce::Label titleLabel_;
  juce::Label subtitleLabel_;
  juce::Label statusLabel_;

  juce::TabbedComponent tabs_{juce::TabbedButtonBar::TabsAtTop};

  juce::Component tabAifred_;
  juce::Component tabAnalysis_;
  juce::Component tabCompare_;
  juce::Component tabReport_;
  juce::Component tabSettings_;

  juce::TextButton analyzeBufferButton_{"Analyze Captured Buffer"};
  juce::TextButton analyzeFileButton_{"Analyze WAV File"};
  juce::TextButton cancelAnalysisButton_{"Cancel Analysis"};
  juce::Label headerCard_;

  juce::Label metricBarsLabel_;
  std::unique_ptr<MetricBarsComponent> metricBarsComponent_;
  juce::Label compareLabel_;
  std::unique_ptr<DualSourceMeterComponent> dualMeterComponent_;
  juce::Label issuesLabel_;
  juce::Label meterModeLabel_;
  juce::ComboBox meterModeCombo_;
  juce::Label reportStatusLabel_;
  juce::TextButton exportButton_{"Export Session Bundle"};
  juce::TextEditor sessionHistory_;

  juce::Label benchmarkPathLabel_;
  juce::TextButton pickBenchmarkButton_{"Pick Benchmark JSON"};
  juce::Label referencePathLabel_;
  juce::TextButton pickReferenceButton_{"Pick User Ref WAV"};

  std::shared_ptr<const AnalysisSnapshot> lastSnapshot_;
  std::uint64_t lastSeenGeneration_{0};

  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> meterModeAttachment_;
};

}  // namespace aifr3d::plugin
