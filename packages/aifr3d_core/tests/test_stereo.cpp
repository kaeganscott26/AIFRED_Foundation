#include "aifr3d/stereo.hpp"

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

}  // namespace

int main() {
  try {
    constexpr std::size_t frames = 4096;
    constexpr double sr = 48000.0;
    constexpr double hz = 997.0;

    std::vector<float> same(frames * 2U);
    for (std::size_t i = 0; i < frames; ++i) {
      const double t = static_cast<double>(i) / sr;
      const float s = static_cast<float>(0.5 * std::sin(2.0 * kPi * hz * t));
      same[i * 2U] = s;
      same[i * 2U + 1U] = s;
    }
    const auto ms = aifr3d::compute_stereo_metrics_interleaved_stereo(same.data(), frames);
    require(ms.correlation.has_value(), "same corr missing");
    require(*ms.correlation > 0.999, "L=R should be highly correlated");
    require(ms.width_proxy.has_value(), "same width missing");
    require(*ms.width_proxy < 1e-6, "L=R should have near-zero width");

    std::vector<float> opp(frames * 2U);
    for (std::size_t i = 0; i < frames; ++i) {
      const double t = static_cast<double>(i) / sr;
      const float s = static_cast<float>(0.5 * std::sin(2.0 * kPi * hz * t));
      opp[i * 2U] = s;
      opp[i * 2U + 1U] = -s;
    }
    const auto mo = aifr3d::compute_stereo_metrics_interleaved_stereo(opp.data(), frames);
    require(mo.correlation.has_value(), "opp corr missing");
    require(*mo.correlation < -0.999, "L=-R should be highly anti-correlated");
    require(mo.width_proxy.has_value(), "opp width missing");
    require(*mo.width_proxy > 0.99, "L=-R should have high width");
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_stereo\n";
  return 0;
}
