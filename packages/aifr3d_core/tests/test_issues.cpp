#include "aifr3d/issues.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool cond, const std::string& msg) {
  if (!cond) {
    throw std::runtime_error(msg);
  }
}

bool hasIssueId(const aifr3d::IssueReport& report, const std::string& id) {
  for (const auto& i : report.top_issues) {
    if (i.id == id) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  try {
    aifr3d::AnalysisResult mix;
    mix.true_peak.true_peak_dbfs = 0.8;  // stronger risk trigger to ensure top-5 ranking

    aifr3d::BenchmarkCompareResult bench;
    bench.loudness.integrated_lufs.value = -3.0;
    bench.loudness.integrated_lufs.mean = -10.5;
    bench.loudness.integrated_lufs.delta = 7.5;
    bench.loudness.integrated_lufs.z = 6.25;
    bench.loudness.integrated_lufs.in_range = aifr3d::InRangeClass::NEEDS_ATTENTION;

    bench.spectral.sub.value = -18.0;
    bench.spectral.sub.mean = -24.0;
    bench.spectral.sub.delta = 6.0;
    bench.spectral.sub.z = 2.0;
    bench.spectral.sub.in_range = aifr3d::InRangeClass::NEEDS_ATTENTION;

    bench.spectral.low.value = -17.0;
    bench.spectral.low.mean = -20.0;
    bench.spectral.low.delta = 3.0;
    bench.spectral.low.z = 1.5;
    bench.spectral.low.in_range = aifr3d::InRangeClass::SLIGHTLY_OFF;

    bench.spectral.highmid.value = -13.0;
    bench.spectral.highmid.mean = -17.0;
    bench.spectral.highmid.delta = 4.0;
    bench.spectral.highmid.z = 1.8;
    bench.spectral.highmid.in_range = aifr3d::InRangeClass::NEEDS_ATTENTION;

    bench.dynamics.crest_db.value = 4.0;
    bench.dynamics.crest_db.mean = 8.8;
    bench.dynamics.crest_db.delta = -4.8;
    bench.dynamics.crest_db.z = -3.0;
    bench.dynamics.crest_db.in_range = aifr3d::InRangeClass::NEEDS_ATTENTION;

    bench.stereo.width_proxy.value = 0.7;
    bench.stereo.width_proxy.mean = 0.3;
    bench.stereo.width_proxy.delta = 0.4;
    bench.stereo.width_proxy.z = 2.4;
    bench.stereo.width_proxy.in_range = aifr3d::InRangeClass::NEEDS_ATTENTION;

    bench.stereo.correlation.value = -0.2;
    bench.stereo.correlation.mean = 0.55;
    bench.stereo.correlation.delta = -0.75;
    bench.stereo.correlation.z = -2.0;
    bench.stereo.correlation.in_range = aifr3d::InRangeClass::NEEDS_ATTENTION;

    const auto report = aifr3d::generateIssues(mix, &bench, nullptr);
    require(!report.top_issues.empty(), "expected issues to be generated");
    require(report.top_issues.size() <= 5U, "top issues must be capped at 5");

    require(hasIssueId(report, "TRUE_PEAK_RISK"), "expected TRUE_PEAK_RISK issue");
    require(hasIssueId(report, "LOUDNESS_TOO_HOT"), "expected LOUDNESS_TOO_HOT issue");
    require(hasIssueId(report, "LOW_END_BUILDUP"), "expected LOW_END_BUILDUP issue");

    for (const auto& issue : report.top_issues) {
      require(!issue.evidence.empty(), "each issue must include evidence");
      require(!issue.fix_steps.empty(), "each issue must include deterministic fix steps");
      require(issue.confidence_0_100 >= 0.0 && issue.confidence_0_100 <= 100.0,
              "confidence out of range");
      for (const auto& ev : issue.evidence) {
        require(!ev.metric_path.empty(), "evidence metric_path missing");
      }
    }

    const auto report2 = aifr3d::generateIssues(mix, &bench, nullptr);
    require(report.top_issues.size() == report2.top_issues.size(), "determinism size mismatch");
    for (std::size_t i = 0; i < report.top_issues.size(); ++i) {
      require(report.top_issues[i].id == report2.top_issues[i].id, "determinism id mismatch");
      require(std::fabs(report.top_issues[i].confidence_0_100 - report2.top_issues[i].confidence_0_100) < 1e-12,
              "determinism confidence mismatch");
    }
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_issues\n";
  return 0;
}
