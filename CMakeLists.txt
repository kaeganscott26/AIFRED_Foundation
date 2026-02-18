cmake_minimum_required(VERSION 3.20)
project(AIFRED_Foundation LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerSettings)

enable_testing()

add_subdirectory(packages/aifr3d_core)
add_subdirectory(apps/dawai_desktop)

option(AIFR3D_ENABLE_VST3 "Enable Phase 6 JUCE VST3 wrapper build" OFF)
if(AIFR3D_ENABLE_VST3)
  add_subdirectory(plugins/aifr3d_vst3)
endif()
