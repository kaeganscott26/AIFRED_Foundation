#include "aifr3d/loudness.hpp"

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

std::vector<float> make_sine(double amp, double hz, std::size_t frames, double sr) {
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

    const auto quiet = make_sine(0.1, 1000.0, frames, sr);
    const auto loud = make_sine(0.5, 1000.0, frames, sr);

    const auto quiet_m = aifr3d::compute_loudness_interleaved_stereo(quiet.data(), frames, sr);
    const auto loud_m = aifr3d::compute_loudness_interleaved_stereo(loud.data(), frames, sr);

    require(quiet_m.integrated_lufs.has_value(), "quiet integrated_lufs missing");
    require(loud_m.integrated_lufs.has_value(), "loud integrated_lufs missing");
    require(*loud_m.integrated_lufs > *quiet_m.integrated_lufs, "loudness must be monotonic with amplitude");
    require(*loud_m.integrated_lufs < 0.0 && *loud_m.integrated_lufs > -60.0,
            "loudness should be in coarse expected range");

    std::vector<float> silence(frames * 2U, 0.0f);
    const auto sil = aifr3d::compute_loudness_interleaved_stereo(silence.data(), frames, sr);
    require(!sil.integrated_lufs.has_value(), "silence integrated_lufs should be nullopt");
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_loudness\n";
  return 0;
}
