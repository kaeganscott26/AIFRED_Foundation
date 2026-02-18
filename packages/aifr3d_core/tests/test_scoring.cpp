#include "aifr3d/scoring.hpp"

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

aifr3d::BenchmarkCompareResult makeCompare(double z_magnitude) {
  aifr3d::BenchmarkCompareResult c;
  c.profile_id = "test_pop_v1";
  c.genre = "pop";

  auto set_metric = [&](aifr3d::MetricDelta& m) {
    m.value = 0.0;
    m.mean = 0.0;
    m.delta = z_magnitude;
    m.z = z_magnitude;
    if (std::fabs(z_magnitude) < 1.0) {
      m.in_range = aifr3d::InRangeClass::IN_RANGE;
    } else if (std::fabs(z_magnitude) < 2.0) {
      m.in_range = aifr3d::InRangeClass::SLIGHTLY_OFF;
    } else {
      m.in_range = aifr3d::InRangeClass::NEEDS_ATTENTION;
    }
  };

  set_metric(c.loudness.integrated_lufs);
  set_metric(c.loudness.short_term_lufs);
  set_metric(c.loudness.loudness_range_lu);
  set_metric(c.dynamics.peak_dbfs);
  set_metric(c.dynamics.rms_dbfs);
  set_metric(c.dynamics.crest_db);
  set_metric(c.dynamics.dr_proxy_db);
  set_metric(c.spectral.sub);
  set_metric(c.spectral.low);
  set_metric(c.spectral.lowmid);
  set_metric(c.spectral.mid);
  set_metric(c.spectral.highmid);
  set_metric(c.spectral.high);
  set_metric(c.spectral.air);
  set_metric(c.stereo.correlation);
  set_metric(c.stereo.lr_balance_db);
  set_metric(c.stereo.width_proxy);
  return c;
}

}  // namespace

int main() {
  try {
    const auto near_cmp = makeCompare(0.5);
    const auto far_cmp = makeCompare(2.5);

    const auto s_near = aifr3d::computeScore(near_cmp);
    const auto s_far = aifr3d::computeScore(far_cmp);

    require(s_near.overall_0_100 > s_far.overall_0_100,
            "score must decrease as metrics move away from targets");

    const auto s1 = aifr3d::computeScore(near_cmp);
    const auto s2 = aifr3d::computeScore(near_cmp);
    require(std::fabs(s1.overall_0_100 - s2.overall_0_100) < 1e-12,
            "scoring must be deterministic across runs");

    require(s1.subscores.count("loudness") == 1, "missing loudness subscore");
    require(s1.subscores.count("dynamics") == 1, "missing dynamics subscore");
    require(s1.subscores.count("tonal_balance") == 1, "missing tonal_balance subscore");
    require(s1.subscores.count("stereo") == 1, "missing stereo subscore");
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_scoring\n";
  return 0;
}
