#include "aifr3d/analyzer.hpp"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDbTol = 1e-6;
constexpr double kExactTol = 0.0;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

double mustValue(const std::optional<double>& v, const std::string& name) {
  require(v.has_value(), name + " should have value");
  return *v;
}

void testSineWaveMetrics() {
  constexpr std::size_t frames = 48000;
  constexpr double sample_rate = 48000.0;
  constexpr double amplitude = 0.5;
  constexpr double frequency = 1000.0;

  std::vector<float> interleaved(frames * 2U);
  for (std::size_t i = 0; i < frames; ++i) {
    const double t = static_cast<double>(i) / sample_rate;
    const float sample = static_cast<float>(amplitude * std::sin(2.0 * kPi * frequency * t));
    interleaved[i * 2U] = sample;
    interleaved[i * 2U + 1U] = sample;
  }

  const aifr3d::Analyzer analyzer;
  const auto result = analyzer.analyzeInterleavedStereo(interleaved.data(), frames, sample_rate);

  const double peak_dbfs = mustValue(result.basic.peak_dbfs, "peak_dbfs");
  const double rms_dbfs = mustValue(result.basic.rms_dbfs, "rms_dbfs");
  const double crest_db = mustValue(result.basic.crest_db, "crest_db");

  const double expected_peak_dbfs = 20.0 * std::log10(amplitude);
  const double expected_rms_dbfs = 20.0 * std::log10(amplitude / std::sqrt(2.0));
  const double expected_crest_db = expected_peak_dbfs - expected_rms_dbfs;

  require(std::fabs(peak_dbfs - expected_peak_dbfs) <= kDbTol, "peak_dbfs out of tolerance");
  require(std::fabs(rms_dbfs - expected_rms_dbfs) <= kDbTol, "rms_dbfs out of tolerance");
  require(std::fabs(crest_db - expected_crest_db) <= kDbTol, "crest_db out of tolerance");
}

void testSilenceContract() {
  constexpr std::size_t frames = 1024;
  constexpr double sample_rate = 44100.0;

  std::vector<float> interleaved(frames * 2U, 0.0f);
  const aifr3d::Analyzer analyzer;
  const auto result = analyzer.analyzeInterleavedStereo(interleaved.data(), frames, sample_rate);

  require(!result.basic.peak_dbfs.has_value(), "silence peak_dbfs must be nullopt");
  require(!result.basic.rms_dbfs.has_value(), "silence rms_dbfs must be nullopt");
  require(!result.basic.crest_db.has_value(), "silence crest_db must be nullopt");
}

void testDeterminismRepeatedRun() {
  constexpr std::size_t frames = 4096;
  constexpr double sample_rate = 48000.0;

  std::vector<float> interleaved(frames * 2U);
  for (std::size_t i = 0; i < frames; ++i) {
    const float l = static_cast<float>(0.25 * std::sin(2.0 * kPi * 220.0 * static_cast<double>(i) / sample_rate));
    const float r = static_cast<float>(0.10 * std::cos(2.0 * kPi * 330.0 * static_cast<double>(i) / sample_rate));
    interleaved[i * 2U] = l;
    interleaved[i * 2U + 1U] = r;
  }

  const aifr3d::Analyzer analyzer;
  const auto baseline = analyzer.analyzeInterleavedStereo(interleaved.data(), frames, sample_rate);
  constexpr int runs = 32;
  for (int i = 0; i < runs; ++i) {
    const auto current = analyzer.analyzeInterleavedStereo(interleaved.data(), frames, sample_rate);
    require(current.schema_version == baseline.schema_version, "schema_version drift");
    require(current.frame_count == baseline.frame_count, "frame_count drift");
    require(std::fabs(current.sample_rate_hz - baseline.sample_rate_hz) <= kExactTol, "sample_rate drift");
    require(current.generated_at_utc == baseline.generated_at_utc, "generated_at_utc drift");
    require(current.analysis_id == baseline.analysis_id, "analysis_id drift");

    require(current.basic.peak_dbfs.has_value() == baseline.basic.peak_dbfs.has_value(), "peak presence drift");
    require(current.basic.rms_dbfs.has_value() == baseline.basic.rms_dbfs.has_value(), "rms presence drift");
    require(current.basic.crest_db.has_value() == baseline.basic.crest_db.has_value(), "crest presence drift");

    if (baseline.basic.peak_dbfs.has_value()) {
      require(std::fabs(*current.basic.peak_dbfs - *baseline.basic.peak_dbfs) <= kExactTol, "peak value drift");
    }
    if (baseline.basic.rms_dbfs.has_value()) {
      require(std::fabs(*current.basic.rms_dbfs - *baseline.basic.rms_dbfs) <= kExactTol, "rms value drift");
    }
    if (baseline.basic.crest_db.has_value()) {
      require(std::fabs(*current.basic.crest_db - *baseline.basic.crest_db) <= kExactTol, "crest value drift");
    }
  }
}

void testInvalidInputHandling() {
  const aifr3d::Analyzer analyzer;
  bool threw = false;

  try {
    (void)analyzer.analyzeInterleavedStereo(nullptr, 16, 48000.0);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  require(threw, "expected throw for null pointer with nonzero frame_count");

  threw = false;
  std::vector<float> interleaved(2U, 0.0f);
  try {
    (void)analyzer.analyzeInterleavedStereo(interleaved.data(), 1, 0.0);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  require(threw, "expected throw for non-positive sample_rate_hz");
}

}  // namespace

int main() {
  try {
    testSineWaveMetrics();
    testSilenceContract();
    testDeterminismRepeatedRun();
    testInvalidInputHandling();
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_analyzer_basic\n";
  return 0;
}
