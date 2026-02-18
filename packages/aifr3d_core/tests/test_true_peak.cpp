#include "aifr3d/true_peak.hpp"

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

std::optional<double> sample_peak_db(const std::vector<float>& interleaved) {
  double peak = 0.0;
  for (float s : interleaved) {
    peak = std::max(peak, std::fabs(static_cast<double>(s)));
  }
  if (peak <= 0.0) {
    return std::nullopt;
  }
  return 20.0 * std::log10(peak);
}

}  // namespace

int main() {
  try {
    constexpr std::size_t frames = 1024;
    std::vector<float> interleaved(frames * 2U, 0.0f);

    for (std::size_t i = 0; i < frames; ++i) {
      const float l = (i % 2U == 0U) ? 0.9f : -0.9f;
      const float r = (i % 2U == 0U) ? -0.9f : 0.9f;
      interleaved[i * 2U] = l;
      interleaved[i * 2U + 1U] = r;
    }

    const auto tp = aifr3d::compute_true_peak_interleaved_stereo(interleaved.data(), frames, 4);
    const auto sp = sample_peak_db(interleaved);
    require(tp.true_peak_dbfs.has_value(), "true peak should exist");
    require(sp.has_value(), "sample peak should exist");
    require(*tp.true_peak_dbfs + 1e-12 >= *sp, "true peak must be >= sample peak");
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_true_peak\n";
  return 0;
}
