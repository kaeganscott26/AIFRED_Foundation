#include "aifr3d/fft.hpp"

#include <cmath>
#include <stdexcept>

namespace aifr3d {

std::size_t next_power_of_two(std::size_t v) {
  if (v == 0) {
    return 1;
  }
  std::size_t p = 1;
  while (p < v) {
    p <<= 1U;
  }
  return p;
}

void fft_inplace(std::vector<Complex>& data) {
  const std::size_t n = data.size();
  if (n == 0 || (n & (n - 1U)) != 0U) {
    throw std::invalid_argument("fft_inplace requires power-of-two non-empty input");
  }

  for (std::size_t i = 1, j = 0; i < n; ++i) {
    std::size_t bit = n >> 1U;
    for (; j & bit; bit >>= 1U) {
      j ^= bit;
    }
    j ^= bit;
    if (i < j) {
      std::swap(data[i], data[j]);
    }
  }

  for (std::size_t len = 2; len <= n; len <<= 1U) {
    const double ang = -2.0 * std::acos(-1.0) / static_cast<double>(len);
    const Complex wlen(std::cos(ang), std::sin(ang));
    for (std::size_t i = 0; i < n; i += len) {
      Complex w(1.0, 0.0);
      for (std::size_t j = 0; j < len / 2U; ++j) {
        const Complex u = data[i + j];
        const Complex v = data[i + j + len / 2U] * w;
        data[i + j] = u + v;
        data[i + j + len / 2U] = u - v;
        w *= wlen;
      }
    }
  }
}

}  // namespace aifr3d
