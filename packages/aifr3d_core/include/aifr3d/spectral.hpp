#pragma once

#include "aifr3d/analyzer.hpp"

#include <cstddef>

namespace aifr3d {

SpectralBands compute_spectral_bands_interleaved_stereo(const float* interleaved_stereo,
                                                        std::size_t frame_count,
                                                        double sample_rate_hz);

}  // namespace aifr3d
