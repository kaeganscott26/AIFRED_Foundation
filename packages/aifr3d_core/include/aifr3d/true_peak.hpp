#pragma once

#include "aifr3d/analyzer.hpp"

#include <cstddef>

namespace aifr3d {

TruePeakMetrics compute_true_peak_interleaved_stereo(const float* interleaved_stereo,
                                                     std::size_t frame_count,
                                                     int oversample_factor);

}  // namespace aifr3d
