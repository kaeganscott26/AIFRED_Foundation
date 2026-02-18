#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace aifr3d::plugin {

namespace {
constexpr const char* kStateType = "AIFR3D_STATE";
constexpr const char* kBypassParam = "bypass";
constexpr const char* kMeterModeParam = "meterMode";
}  // namespace

Aifr3dAudioProcessor::Aifr3dAudioProcessor()
    : juce::AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "PARAMS", createParameterLayout()) {
  analysisService_.start();
}

Aifr3dAudioProcessor::~Aifr3dAudioProcessor() { analysisService_.stop(); }

void Aifr3dAudioProcessor::prepareToPlay(double sampleRate, int) {
  ringCapacitySamples_ = juce::jmax(1, static_cast<int>(sampleRate * static_cast<double>(kCaptureSeconds)));
  const juce::SpinLock::ScopedLockType lock(ringLock_);
  ringBuffer_.setSize(2, ringCapacitySamples_, false, true, true);
  ringBuffer_.clear();
  ringWritePos_ = 0;
  ringValidSamples_ = 0;
  analysisService_.setRingCapacitySamples(static_cast<std::uint64_t>(ringCapacitySamples_));
}

void Aifr3dAudioProcessor::releaseResources() { analysisService_.cancelPendingAndInFlight(); }

void Aifr3dAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals noDenormals;

  const auto totalIn = getTotalNumInputChannels();
  const auto totalOut = getTotalNumOutputChannels();
  for (auto i = totalIn; i < totalOut; ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }

  const bool bypass = apvts_.getRawParameterValue(kBypassParam)->load() > 0.5f;
  if (bypass) {
    pushToRingBuffer(buffer);
    return;
  }

  // Pass-through unchanged by design for Phase 6 safety.
  pushToRingBuffer(buffer);
}

juce::AudioProcessorEditor* Aifr3dAudioProcessor::createEditor() { return new Aifr3dAudioProcessorEditor(*this); }

void Aifr3dAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  juce::ValueTree state = apvts_.copyState();
  state.setType(kStateType);
  state.setProperty("lastSessionId", lastSessionId_, nullptr);
  state.setProperty("lastGenre", lastGenre_, nullptr);
  state.setProperty("lastExportPath", lastExportPath_, nullptr);
  state.setProperty("benchmarkProfilePath", benchmarkProfilePath_, nullptr);
  state.setProperty("referenceWavPath", referenceWavPath_, nullptr);

  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void Aifr3dAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
  if (xmlState == nullptr) {
    return;
  }

  const juce::ValueTree tree = juce::ValueTree::fromXml(*xmlState);
  if (!tree.isValid()) {
    return;
  }

  apvts_.replaceState(tree);
  lastSessionId_ = tree.getProperty("lastSessionId").toString();
  lastGenre_ = tree.getProperty("lastGenre").toString();
  lastExportPath_ = tree.getProperty("lastExportPath").toString();
  benchmarkProfilePath_ = tree.getProperty("benchmarkProfilePath").toString();
  referenceWavPath_ = tree.getProperty("referenceWavPath").toString();
}

void Aifr3dAudioProcessor::triggerAnalysisFromCapturedBuffer() {
  juce::AudioBuffer<float> snapshot;
  copyRingBufferSnapshot(snapshot);
  analysisService_.submitCapturedBuffer(snapshot,
                                        getSampleRate() > 0.0 ? getSampleRate() : 48000.0,
                                        "Captured Buffer",
                                        benchmarkProfilePath_,
                                        referenceWavPath_);
}

void Aifr3dAudioProcessor::triggerAnalysisFromFile(const juce::File& wavFile) {
  analysisService_.submitOfflineFile(wavFile, benchmarkProfilePath_, referenceWavPath_);
}

void Aifr3dAudioProcessor::cancelAnalysisJobs() { analysisService_.cancelPendingAndInFlight(); }

juce::AudioProcessorValueTreeState::ParameterLayout Aifr3dAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
  params.push_back(std::make_unique<juce::AudioParameterBool>(kBypassParam, "Bypass", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      kMeterModeParam,
      "Meter Mode",
      juce::StringArray{"True Meter vs Pro Pool", "True Meter vs User Ref", "User Ref vs Pro Pool"},
      0));
  return {params.begin(), params.end()};
}

void Aifr3dAudioProcessor::pushToRingBuffer(const juce::AudioBuffer<float>& in) {
  if (ringCapacitySamples_ <= 0 || in.getNumSamples() <= 0) {
    return;
  }

  const juce::SpinLock::ScopedTryLockType lock(ringLock_);
  if (!lock.isLocked()) {
    return;
  }

  const int n = in.getNumSamples();
  for (int i = 0; i < n; ++i) {
    const int dst = ringWritePos_;
    ringBuffer_.setSample(0, dst, in.getSample(0, i));
    ringBuffer_.setSample(1, dst, in.getNumChannels() > 1 ? in.getSample(1, i) : in.getSample(0, i));
    ringWritePos_ = (ringWritePos_ + 1) % ringCapacitySamples_;
  }
  ringValidSamples_ = juce::jmin(ringCapacitySamples_, ringValidSamples_ + n);
}

void Aifr3dAudioProcessor::copyRingBufferSnapshot(juce::AudioBuffer<float>& out) const {
  const juce::SpinLock::ScopedLockType lock(ringLock_);
  if (ringValidSamples_ <= 0 || ringCapacitySamples_ <= 0) {
    out.setSize(2, 0);
    return;
  }

  out.setSize(2, ringValidSamples_, false, false, true);
  const int start = (ringWritePos_ - ringValidSamples_ + ringCapacitySamples_) % ringCapacitySamples_;
  for (int i = 0; i < ringValidSamples_; ++i) {
    const int src = (start + i) % ringCapacitySamples_;
    out.setSample(0, i, ringBuffer_.getSample(0, src));
    out.setSample(1, i, ringBuffer_.getSample(1, src));
  }
}

}  // namespace aifr3d::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new aifr3d::plugin::Aifr3dAudioProcessor();
}
