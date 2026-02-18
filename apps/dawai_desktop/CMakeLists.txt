add_executable(dawai_desktop_stub
  src/main.cpp
)

target_link_libraries(dawai_desktop_stub
  PRIVATE
    aifr3d_core
)

target_compile_features(dawai_desktop_stub PRIVATE cxx_std_20)
