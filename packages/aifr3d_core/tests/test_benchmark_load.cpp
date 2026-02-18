#include "aifr3d/benchmark_profile.hpp"

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

    require(profile.schema_version == "0.3.0", "schema_version mismatch");
    require(profile.genre == "pop", "genre mismatch");
    require(profile.profile_id == "test_pop_v1", "profile_id mismatch");
    require(profile.track_count == 32, "track_count mismatch");

    require(profile.metrics.loudness.integrated_lufs.has_value(), "missing loudness target");
    require(profile.metrics.stereo.width_proxy.has_value(), "missing stereo width target");
    require(profile.metrics.dynamics.dr_proxy_db.has_value(), "missing dr_proxy target");
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_benchmark_load\n";
  return 0;
}
