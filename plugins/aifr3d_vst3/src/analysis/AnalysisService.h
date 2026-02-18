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

  struct Config {
    int timeoutMs{8000};
  };

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

  void cancelPendingAndInFlight();
  void setRingCapacitySamples(std::uint64_t samples);

  std::shared_ptr<const AnalysisSnapshot> latestSnapshot() const;
  AnalysisPerfCounters perfCounters() const;

 private:
  void run() override;

  AnalysisSnapshot runJob(const AnalysisJob& job);
  bool loadWavToStereoBuffer(const juce::File& file,
                             juce::AudioBuffer<float>& out,
                             double& sampleRateHz,
                             juce::String& err) const;

  static std::vector<float> interleaveStereo(const juce::AudioBuffer<float>& buf);

  Config config_;
  mutable std::mutex jobMutex_;
  AnalysisJob pendingJob_;
  bool hasPendingJob_{false};

  std::atomic<std::uint64_t> generation_{0};
  std::atomic<std::uint64_t> latestRequestedGeneration_{0};

  mutable std::mutex perfMutex_;
  AnalysisPerfCounters perf_;

  std::shared_ptr<AnalysisSnapshot> latestSnapshot_;
  mutable std::mutex latestMutex_;
};

}  // namespace aifr3d::plugin
