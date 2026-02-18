add_library(aifr3d_core STATIC
  src/analyzer.cpp
  src/benchmark_profile.cpp
  src/compare.cpp
  src/dynamics.cpp
  src/fft.cpp
  src/issues.cpp
  src/loudness.cpp
  src/reference_compare.cpp
  src/rules.cpp
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

option(AIFR3D_BUILD_CORE_TOOLS "Build AIFR3D core CLI/tools" ON)
if(AIFR3D_BUILD_CORE_TOOLS)
  add_executable(aifr3d_core_cli
    tools/analyze_wav.cpp
  )
  target_link_libraries(aifr3d_core_cli PRIVATE aifr3d_core)
  target_compile_features(aifr3d_core_cli PRIVATE cxx_std_20)
endif()

add_subdirectory(tests)
