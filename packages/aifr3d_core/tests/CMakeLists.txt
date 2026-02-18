add_executable(test_analyzer_basic
  test_analyzer_basic.cpp
)

target_link_libraries(test_analyzer_basic
  PRIVATE
    aifr3d_core
)

target_compile_features(test_analyzer_basic PRIVATE cxx_std_20)

add_test(NAME aifr3d_core.test_analyzer_basic COMMAND test_analyzer_basic)
