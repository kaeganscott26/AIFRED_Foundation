#pragma once

#include "aifr3d/analyzer.hpp"

#include <cstddef>

namespace aifr3d {

StereoMetrics compute_stereo_metrics_interleaved_stereo(const float* interleaved_stereo,
                                                        std::size_t frame_count);

}  // namespace aifr3d
