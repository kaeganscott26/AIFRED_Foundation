add_library(aifr3d_core STATIC
  src/analyzer.cpp
  src/benchmark_profile.cpp
  src/compare.cpp
  src/dynamics.cpp
  src/fft.cpp
  src/loudness.cpp
  src/scoring.cpp
  src/spectral.cpp
  src/stereo.cpp
  src/true_peak.cpp
)

target_include_directories(aifr3d_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(aifr3d_core PUBLIC cxx_std_20)

add_subdirectory(tests)
