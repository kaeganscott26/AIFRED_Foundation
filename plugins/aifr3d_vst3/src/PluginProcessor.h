#pragma once

#include "analysis/AnalysisService.h"
#include "analysis/AnalysisTypes.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <memory>

namespace aifr3d::plugin {

class Aifr3dAudioProcessor final : public juce::AudioProcessor {
 public:
  Aifr3dAudioProcessor();
  ~Aifr3dAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  [[nodiscard]] juce::AudioProcessorEditor* createEditor() override;
  [[nodiscard]] bool hasEditor() const override { return true; }

  [[nodiscard]] const juce::String getName() const override { return "AIFR3D"; }
  [[nodiscard]] bool acceptsMidi() const override { return false; }
  [[nodiscard]] bool producesMidi() const override { return false; }
  [[nodiscard]] bool isMidiEffect() const override { return false; }
  [[nodiscard]] double getTailLengthSeconds() const override { return 0.0; }

  [[nodiscard]] int getNumPrograms() override { return 1; }
  [[nodiscard]] int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  [[nodiscard]] const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState& state() { return apvts_; }

  void setLastSessionId(const juce::String& value) { lastSessionId_ = value; }
  void setLastGenre(const juce::String& value) { lastGenre_ = value; }
  void setLastExportPath(const juce::String& value) { lastExportPath_ = value; }
  void setBenchmarkProfilePath(const juce::String& value) { benchmarkProfilePath_ = value; }
  void setReferenceWavPath(const juce::String& value) { referenceWavPath_ = value; }

  [[nodiscard]] const juce::String& lastSessionId() const { return lastSessionId_; }
  [[nodiscard]] const juce::String& lastGenre() const { return lastGenre_; }
  [[nodiscard]] const juce::String& lastExportPath() const { return lastExportPath_; }
  [[nodiscard]] const juce::String& benchmarkProfilePath() const { return benchmarkProfilePath_; }
  [[nodiscard]] const juce::String& referenceWavPath() const { return referenceWavPath_; }

  [[nodiscard]] std::shared_ptr<const AnalysisSnapshot> latestSnapshot() const {
    return analysisService_.latestSnapshot();
  }
  [[nodiscard]] AnalysisPerfCounters perfCounters() const { return analysisService_.perfCounters(); }

  void triggerAnalysisFromCapturedBuffer();
  void triggerAnalysisFromFile(const juce::File& wavFile);
  void cancelAnalysisJobs();

 private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
  void pushToRingBuffer(const juce::AudioBuffer<float>& in);
  void copyRingBufferSnapshot(juce::AudioBuffer<float>& out) const;

  juce::AudioProcessorValueTreeState apvts_;
  AnalysisService analysisService_;

  mutable juce::SpinLock ringLock_;
  juce::AudioBuffer<float> ringBuffer_;
  int ringWritePos_{0};
  int ringValidSamples_{0};
  int ringCapacitySamples_{0};
  static constexpr int kCaptureSeconds = 10;

  juce::String lastSessionId_;
  juce::String lastGenre_;
  juce::String lastExportPath_;
  juce::String benchmarkProfilePath_;
  juce::String referenceWavPath_;
};

}  // namespace aifr3d::plugin
