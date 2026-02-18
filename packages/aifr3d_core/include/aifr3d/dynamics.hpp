#pragma once

#include "aifr3d/analyzer.hpp"

#include <cstddef>

namespace aifr3d {

DynamicsMetrics compute_dynamics_interleaved_stereo(const float* interleaved_stereo,
                                                    std::size_t frame_count);

}  // namespace aifr3d
