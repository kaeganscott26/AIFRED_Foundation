#include "aifr3d/spectral.hpp"

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
  std::vector<float> out(frames * 2U);
  for (std::size_t i = 0; i < frames; ++i) {
    const double t = static_cast<double>(i) / sr;
    const float s = static_cast<float>(amp * std::sin(2.0 * kPi * hz * t));
    out[i * 2U] = s;
    out[i * 2U + 1U] = s;
  }
  return out;
}

double value(const std::optional<double>& v) {
  if (!v.has_value()) {
    return -1e18;
  }
  return *v;
}

}  // namespace

int main() {
  try {
    constexpr std::size_t frames = 16384;
    constexpr double sr = 48000.0;

    {
      const auto wave = make_sine(0.5, 100.0, frames, sr);
      const auto b = aifr3d::compute_spectral_bands_interleaved_stereo(wave.data(), frames, sr);
      require(value(b.low) > value(b.mid), "100Hz should favor low band over mid");
    }
    {
      const auto wave = make_sine(0.5, 1000.0, frames, sr);
      const auto b = aifr3d::compute_spectral_bands_interleaved_stereo(wave.data(), frames, sr);
      require(value(b.mid) > value(b.lowmid), "1kHz should favor mid over lowmid");
      require(value(b.mid) > value(b.highmid), "1kHz should favor mid over highmid");
    }
    {
      const auto wave = make_sine(0.5, 8000.0, frames, sr);
      const auto b = aifr3d::compute_spectral_bands_interleaved_stereo(wave.data(), frames, sr);
      require(value(b.high) > value(b.mid), "8kHz should favor high over mid");
    }
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_spectral\n";
  return 0;
}
