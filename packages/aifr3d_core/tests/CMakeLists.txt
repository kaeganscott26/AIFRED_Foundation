add_executable(test_analyzer_basic
  test_analyzer_basic.cpp
)

target_link_libraries(test_analyzer_basic
  PRIVATE
    aifr3d_core
)

target_compile_features(test_analyzer_basic PRIVATE cxx_std_20)

add_test(NAME aifr3d_core.test_analyzer_basic COMMAND test_analyzer_basic)

add_executable(test_loudness
  test_loudness.cpp
)
target_link_libraries(test_loudness PRIVATE aifr3d_core)
target_compile_features(test_loudness PRIVATE cxx_std_20)
add_test(NAME aifr3d_core.test_loudness COMMAND test_loudness)

add_executable(test_true_peak
  test_true_peak.cpp
)
target_link_libraries(test_true_peak PRIVATE aifr3d_core)
target_compile_features(test_true_peak PRIVATE cxx_std_20)
add_test(NAME aifr3d_core.test_true_peak COMMAND test_true_peak)

add_executable(test_spectral
  test_spectral.cpp
)
target_link_libraries(test_spectral PRIVATE aifr3d_core)
target_compile_features(test_spectral PRIVATE cxx_std_20)
add_test(NAME aifr3d_core.test_spectral COMMAND test_spectral)

add_executable(test_stereo
  test_stereo.cpp
)
target_link_libraries(test_stereo PRIVATE aifr3d_core)
target_compile_features(test_stereo PRIVATE cxx_std_20)
add_test(NAME aifr3d_core.test_stereo COMMAND test_stereo)

add_executable(test_dynamics
  test_dynamics.cpp
)
target_link_libraries(test_dynamics PRIVATE aifr3d_core)
target_compile_features(test_dynamics PRIVATE cxx_std_20)
add_test(NAME aifr3d_core.test_dynamics COMMAND test_dynamics)

add_executable(test_benchmark_load
  test_benchmark_load.cpp
)
target_link_libraries(test_benchmark_load PRIVATE aifr3d_core)
target_compile_features(test_benchmark_load PRIVATE cxx_std_20)
add_test(NAME aifr3d_core.test_benchmark_load COMMAND test_benchmark_load)

add_executable(test_compare
  test_compare.cpp
)
target_link_libraries(test_compare PRIVATE aifr3d_core)
target_compile_features(test_compare PRIVATE cxx_std_20)
add_test(NAME aifr3d_core.test_compare COMMAND test_compare)

add_executable(test_scoring
  test_scoring.cpp
)
target_link_libraries(test_scoring PRIVATE aifr3d_core)
target_compile_features(test_scoring PRIVATE cxx_std_20)
add_test(NAME aifr3d_core.test_scoring COMMAND test_scoring)
