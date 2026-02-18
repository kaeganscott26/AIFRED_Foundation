#include "aifr3d/analyzer.hpp"
#include "aifr3d/reference_compare.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

void require(bool cond, const std::string& msg) {
  if (!cond) {
    throw std::runtime_error(msg);
  }
}

std::vector<float> makeStereoSine(double amp, double hz, std::size_t frames, double sr) {
  std::vector<float> out(frames * 2U, 0.0f);
  for (std::size_t i = 0; i < frames; ++i) {
    const double t = static_cast<double>(i) / sr;
    const float s = static_cast<float>(amp * std::sin(2.0 * kPi * hz * t));
    out[i * 2U] = s;
    out[i * 2U + 1U] = s;
  }
  return out;
}

}  // namespace

int main() {
  try {
    constexpr std::size_t frames = 48000;
    constexpr double sr = 48000.0;
    aifr3d::Analyzer analyzer;

    const auto mix_buf = makeStereoSine(0.60, 1000.0, frames, sr);
    const auto ref1_buf = makeStereoSine(0.30, 1000.0, frames, sr);
    const auto ref2_buf = makeStereoSine(0.45, 1000.0, frames, sr);

    const auto mix = analyzer.analyzeInterleavedStereo(mix_buf.data(), frames, sr);
    auto ref1 = analyzer.analyzeInterleavedStereo(ref1_buf.data(), frames, sr);
    const auto ref2 = analyzer.analyzeInterleavedStereo(ref2_buf.data(), frames, sr);

    const auto out = aifr3d::compareToReferences(mix, {ref1, ref2});
    require(out.reference_count == 2U, "reference count mismatch");
    require(out.per_reference.size() == 2U, "per_reference size mismatch");

    require(out.per_reference[0].basic.rms_dbfs.delta.has_value(), "rms delta missing for ref1");
    require(*out.per_reference[0].basic.rms_dbfs.delta > 0.0,
            "mix RMS should be greater than weaker reference RMS");

    require(out.reference_average.basic.rms_dbfs.has_value(), "average RMS missing");
    require(mix.basic.rms_dbfs.has_value(), "mix RMS missing");
    require(*mix.basic.rms_dbfs > *out.reference_average.basic.rms_dbfs,
            "mix RMS expected above reference average RMS");

    require(out.distance.overall_closeness_0_100 >= 0.0 && out.distance.overall_closeness_0_100 <= 100.0,
            "overall closeness out of bounds");

    const auto out2 = aifr3d::compareToReferences(mix, {ref1, ref2});
    require(std::fabs(out.distance.overall_closeness_0_100 - out2.distance.overall_closeness_0_100) < 1e-12,
            "reference compare must be deterministic across runs");

    bool threw_empty = false;
    try {
      (void)aifr3d::compareToReferences(mix, {});
    } catch (const std::invalid_argument&) {
      threw_empty = true;
    }
    require(threw_empty, "empty reference list must throw");

    bool threw_schema = false;
    try {
      ref1.schema_version = mix.schema_version + 1;
      (void)aifr3d::compareToReferences(mix, {ref1});
    } catch (const std::invalid_argument&) {
      threw_schema = true;
    }
    require(threw_schema, "schema mismatch must throw");
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_reference_compare\n";
  return 0;
}
