#include "AnalysisService.h"

#include "aifr3d/analyzer.hpp"
#include "aifr3d/benchmark_profile.hpp"
#include "aifr3d/compare.hpp"
#include "aifr3d/issues.hpp"
#include "aifr3d/reference_compare.hpp"
#include "aifr3d/scoring.hpp"

#include <utility>

namespace aifr3d::plugin {

AnalysisService::AnalysisService() : juce::Thread("AIFR3DAnalysisWorker") {}

AnalysisService::~AnalysisService() { stop(); }

void AnalysisService::start() {
  if (!isThreadRunning()) {
    startThread();
  }
}

void AnalysisService::stop() { stopThread(1500); }

void AnalysisService::submitCapturedBuffer(const juce::AudioBuffer<float>& stereoBuffer,
                                           double sampleRateHz,
                                           juce::String trackName,
                                           juce::String benchmarkProfilePath,
                                           juce::String referenceWavPath) {
  AnalysisJob j;
  j.generation = ++generation_;
  j.sourceKind = AnalysisSourceKind::CapturedBuffer;
  j.trackName = trackName;
  j.stereoBuffer.makeCopyOf(stereoBuffer);
  j.sampleRateHz = sampleRateHz;
  j.benchmarkProfilePath = std::move(benchmarkProfilePath);
  j.referenceWavPath = std::move(referenceWavPath);

  {
    const std::scoped_lock lock(jobMutex_);
    pendingJob_ = std::move(j);
    hasPendingJob_ = true;
  }
  notify();
}

void AnalysisService::submitOfflineFile(const juce::File& wavFile,
                                        juce::String benchmarkProfilePath,
                                        juce::String referenceWavPath) {
  AnalysisJob j;
  j.generation = ++generation_;
  j.sourceKind = AnalysisSourceKind::OfflineWav;
  j.trackName = wavFile.getFileNameWithoutExtension();
  j.offlineFile = wavFile;
  j.benchmarkProfilePath = std::move(benchmarkProfilePath);
  j.referenceWavPath = std::move(referenceWavPath);

  {
    const std::scoped_lock lock(jobMutex_);
    pendingJob_ = std::move(j);
    hasPendingJob_ = true;
  }
  notify();
}

std::shared_ptr<const AnalysisSnapshot> AnalysisService::latestSnapshot() const {
  const std::scoped_lock lock(latestMutex_);
  return latestSnapshot_;
}

void AnalysisService::run() {
  while (!threadShouldExit()) {
    AnalysisJob job;
    bool hasJob = false;

    {
      const std::scoped_lock lock(jobMutex_);
      if (hasPendingJob_) {
        job = pendingJob_;
        hasPendingJob_ = false;
        hasJob = true;
      }
    }

    if (!hasJob) {
      wait(100);
      continue;
    }

    auto snapshot = std::make_shared<AnalysisSnapshot>(runJob(job));
    const std::scoped_lock lock(latestMutex_);
    latestSnapshot_ = std::move(snapshot);
  }
}

AnalysisSnapshot AnalysisService::runJob(const AnalysisJob& job) {
  AnalysisSnapshot out;
  out.completedAt = juce::Time::getCurrentTime();
  out.trackName = job.trackName;
  out.sourceLabel = (job.sourceKind == AnalysisSourceKind::OfflineWav) ? "offline_wav" : "captured_buffer";

  try {
    juce::AudioBuffer<float> stereo;
    double sampleRate = job.sampleRateHz;

    if (job.sourceKind == AnalysisSourceKind::OfflineWav) {
      juce::String err;
      if (!loadWavToStereoBuffer(job.offlineFile, stereo, sampleRate, err)) {
        out.valid = false;
        out.errorMessage = err;
        return out;
      }
    } else {
      stereo.makeCopyOf(job.stereoBuffer);
    }

    if (stereo.getNumChannels() < 2 || stereo.getNumSamples() <= 0) {
      out.valid = false;
      out.errorMessage = "No valid stereo samples available for analysis.";
      return out;
    }

    const auto interleaved = interleaveStereo(stereo);
    const std::size_t frameCount = static_cast<std::size_t>(stereo.getNumSamples());

    aifr3d::Analyzer analyzer;
    auto analysis = analyzer.analyzeInterleavedStereo(interleaved.data(), frameCount, sampleRate);
    analysis.generated_at_utc = out.completedAt.toISO8601(true).toStdString();

    out.analysis = analysis;
    out.sampleRateHz = sampleRate;
    out.durationSeconds = sampleRate > 0.0 ? static_cast<double>(stereo.getNumSamples()) / sampleRate : 0.0;

    std::optional<aifr3d::BenchmarkProfile> benchmarkProfile;
    if (job.benchmarkProfilePath.isNotEmpty()) {
      const juce::File benchmarkFile(job.benchmarkProfilePath);
      if (benchmarkFile.existsAsFile()) {
        benchmarkProfile = aifr3d::loadBenchmarkProfileFromJson(benchmarkFile.getFullPathName().toStdString());
      }
    }

    if (benchmarkProfile.has_value()) {
      auto bench = aifr3d::compareAgainstBenchmark(analysis, *benchmarkProfile);
      auto score = aifr3d::computeScore(bench);
      out.benchmarkCompare = bench;
      out.score = score;
    }

    if (job.referenceWavPath.isNotEmpty()) {
      juce::String refErr;
      juce::AudioBuffer<float> refBuf;
      double refRate = 0.0;
      if (loadWavToStereoBuffer(juce::File(job.referenceWavPath), refBuf, refRate, refErr) &&
          refBuf.getNumSamples() > 0 && refRate > 0.0) {
        const auto refInterleaved = interleaveStereo(refBuf);
        auto refAnalysis = analyzer.analyzeInterleavedStereo(refInterleaved.data(),
                                                             static_cast<std::size_t>(refBuf.getNumSamples()),
                                                             refRate);
        refAnalysis.schema_version = analysis.schema_version;
        std::vector<aifr3d::AnalysisResult> refs{refAnalysis};
        out.referenceCompare = aifr3d::compareToReferences(analysis, refs);
      }
    }

    const aifr3d::BenchmarkCompareResult* benchPtr =
        out.benchmarkCompare.has_value() ? &(*out.benchmarkCompare) : nullptr;
    const aifr3d::ReferenceCompareResult* refPtr =
        out.referenceCompare.has_value() ? &(*out.referenceCompare) : nullptr;
    out.issues = aifr3d::generateIssues(analysis, benchPtr, refPtr);

    out.valid = true;
  } catch (const std::exception& e) {
    out.valid = false;
    out.errorMessage = e.what();
  }

  return out;
}

bool AnalysisService::loadWavToStereoBuffer(const juce::File& file,
                                            juce::AudioBuffer<float>& out,
                                            double& sampleRateHz,
                                            juce::String& err) const {
  if (!file.existsAsFile()) {
    err = "File does not exist: " + file.getFullPathName();
    return false;
  }

  juce::AudioFormatManager fm;
  fm.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
  if (reader == nullptr) {
    err = "Unsupported audio file format: " + file.getFullPathName();
    return false;
  }

  const int numSamples = static_cast<int>(reader->lengthInSamples);
  if (numSamples <= 0) {
    err = "Audio file is empty: " + file.getFullPathName();
    return false;
  }

  juce::AudioBuffer<float> temp(static_cast<int>(juce::jmax(2u, reader->numChannels)), numSamples);
  if (!reader->read(&temp, 0, numSamples, 0, true, true)) {
    err = "Failed to read audio data: " + file.getFullPathName();
    return false;
  }

  out.setSize(2, numSamples, false, false, true);
  out.copyFrom(0, 0, temp, 0, 0, numSamples);
  out.copyFrom(1, 0, temp, temp.getNumChannels() > 1 ? 1 : 0, 0, numSamples);
  sampleRateHz = reader->sampleRate;
  return true;
}

std::vector<float> AnalysisService::interleaveStereo(const juce::AudioBuffer<float>& buf) {
  const int n = buf.getNumSamples();
  std::vector<float> out(static_cast<std::size_t>(n) * 2U, 0.0f);
  const float* l = buf.getReadPointer(0);
  const float* r = buf.getReadPointer(buf.getNumChannels() > 1 ? 1 : 0);
  for (int i = 0; i < n; ++i) {
    out[static_cast<std::size_t>(i) * 2U] = l[i];
    out[static_cast<std::size_t>(i) * 2U + 1U] = r[i];
  }
  return out;
}

}  // namespace aifr3d::plugin
