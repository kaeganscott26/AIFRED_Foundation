#pragma once

#include "AnalysisJob.h"
#include "AnalysisTypes.h"

#include <JuceHeader.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace aifr3d::plugin {

class AnalysisService final : private juce::Thread {
 public:
  AnalysisService();
  ~AnalysisService() override;

  void start();
  void stop();

  void submitCapturedBuffer(const juce::AudioBuffer<float>& stereoBuffer,
                            double sampleRateHz,
                            juce::String trackName,
                            juce::String benchmarkProfilePath,
                            juce::String referenceWavPath);

  void submitOfflineFile(const juce::File& wavFile,
                         juce::String benchmarkProfilePath,
                         juce::String referenceWavPath);

  std::shared_ptr<const AnalysisSnapshot> latestSnapshot() const;

 private:
  void run() override;

  AnalysisSnapshot runJob(const AnalysisJob& job);
  bool loadWavToStereoBuffer(const juce::File& file,
                             juce::AudioBuffer<float>& out,
                             double& sampleRateHz,
                             juce::String& err) const;

  static std::vector<float> interleaveStereo(const juce::AudioBuffer<float>& buf);

  mutable std::mutex jobMutex_;
  AnalysisJob pendingJob_;
  bool hasPendingJob_{false};

  std::atomic<std::uint64_t> generation_{0};
  std::shared_ptr<AnalysisSnapshot> latestSnapshot_;
  mutable std::mutex latestMutex_;
};

}  // namespace aifr3d::plugin
