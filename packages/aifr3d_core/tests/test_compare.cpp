#include "aifr3d/compare.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool cond, const std::string& msg) {
  if (!cond) {
    throw std::runtime_error(msg);
  }
}

}  // namespace

int main() {
  try {
    const auto profile = aifr3d::loadBenchmarkProfileFromJson(
        "/home/north3rnlight3r/Projects/AIFRED_Foundation/data/benchmarks/test_pop_profile.json");

    aifr3d::AnalysisResult analysis;
    analysis.basic.peak_dbfs = -1.0;
    analysis.basic.rms_dbfs = -10.5;
    analysis.basic.crest_db = 9.5;
    analysis.loudness.integrated_lufs = -10.0;
    analysis.loudness.short_term_lufs = -9.2;
    analysis.loudness.loudness_range_lu = 1.8;
    analysis.spectral.mid = -15.0;
    analysis.stereo.correlation = 0.50;
    analysis.stereo.width_proxy = 0.33;
    analysis.stereo.lr_balance_db = 0.2;
    analysis.dynamics.peak_dbfs = -1.0;
    analysis.dynamics.rms_dbfs = -10.5;
    analysis.dynamics.crest_db = 9.5;
    analysis.dynamics.dr_proxy_db = 8.0;

    const auto cmp = aifr3d::compareAgainstBenchmark(analysis, profile);
    require(cmp.profile_id == "test_pop_v1", "profile id not propagated");

    require(cmp.basic.peak_dbfs.delta.has_value(), "missing basic peak delta");
    require(std::fabs(*cmp.basic.peak_dbfs.delta - 0.2) < 1e-9, "basic peak delta mismatch");
    require(cmp.basic.peak_dbfs.z.has_value(), "missing basic peak z");
    require(std::fabs(*cmp.basic.peak_dbfs.z - 0.25) < 1e-9, "basic peak z mismatch");
    require(cmp.basic.peak_dbfs.in_range == aifr3d::InRangeClass::IN_RANGE,
            "basic peak should be in range");

    require(cmp.spectral.mid.in_range == aifr3d::InRangeClass::IN_RANGE,
            "spectral mid should be in range by z-threshold");

    require(cmp.summary.unknown_count >= 0, "summary counts invalid");
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_compare\n";
  return 0;
}
