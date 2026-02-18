#include "aifr3d/spectral.hpp"

#include "aifr3d/fft.hpp"

#include <array>
#include <cmath>
#include <vector>

namespace aifr3d {

namespace {

struct BandDef {
  double lo;
  double hi;
};

constexpr std::array<BandDef, 7> kBands{{
    {20.0, 60.0}, {60.0, 200.0}, {200.0, 500.0}, {500.0, 2000.0},
    {2000.0, 6000.0}, {6000.0, 12000.0}, {12000.0, 20000.0},
}};

std::optional<double> energy_to_db(double e) {
  if (!(e > 0.0)) {
    return std::nullopt;
  }
  return 10.0 * std::log10(e);
}

}  // namespace

SpectralBands compute_spectral_bands_interleaved_stereo(const float* interleaved_stereo,
                                                        std::size_t frame_count,
                                                        double sample_rate_hz) {
  SpectralBands out;
  if (interleaved_stereo == nullptr || frame_count == 0 || !(sample_rate_hz > 0.0)) {
    return out;
  }

  const std::size_t fft_size = 1024;
  const std::size_t hop = 512;
  if (frame_count < fft_size) {
    return out;
  }

  std::array<double, 7> accum{};
  std::size_t windows = 0;
  std::vector<Complex> buf(fft_size);

  for (std::size_t start = 0; start + fft_size <= frame_count; start += hop) {
    for (std::size_t i = 0; i < fft_size; ++i) {
      const std::size_t idx = (start + i) * 2U;
      const double l = static_cast<double>(interleaved_stereo[idx]);
      const double r = static_cast<double>(interleaved_stereo[idx + 1U]);
      const double mono = 0.5 * (l + r);
      const double w = 0.5 * (1.0 - std::cos((2.0 * std::acos(-1.0) * static_cast<double>(i)) /
                                             static_cast<double>(fft_size - 1U)));
      buf[i] = Complex(mono * w, 0.0);
    }

    fft_inplace(buf);

    const std::size_t bins = fft_size / 2U;
    for (std::size_t b = 1; b < bins; ++b) {
      const double freq = static_cast<double>(b) * sample_rate_hz / static_cast<double>(fft_size);
      const double mag2 = std::norm(buf[b]);
      for (std::size_t bi = 0; bi < kBands.size(); ++bi) {
        if (freq >= kBands[bi].lo && freq < kBands[bi].hi) {
          accum[bi] += mag2;
          break;
        }
      }
    }
    ++windows;
  }

  if (windows == 0) {
    return out;
  }

  for (double& v : accum) {
    v /= static_cast<double>(windows);
  }

  out.sub = energy_to_db(accum[0]);
  out.low = energy_to_db(accum[1]);
  out.lowmid = energy_to_db(accum[2]);
  out.mid = energy_to_db(accum[3]);
  out.highmid = energy_to_db(accum[4]);
  out.high = energy_to_db(accum[5]);
  out.air = energy_to_db(accum[6]);
  return out;
}

}  // namespace aifr3d
