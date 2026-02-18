#pragma once

#include <complex>
#include <cstddef>
#include <vector>

namespace aifr3d {

using Complex = std::complex<double>;

std::size_t next_power_of_two(std::size_t v);
void fft_inplace(std::vector<Complex>& data);

}  // namespace aifr3d
