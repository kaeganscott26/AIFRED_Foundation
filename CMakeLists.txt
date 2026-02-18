cmake_minimum_required(VERSION 3.20)
project(AIFRED_Foundation LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerSettings)

enable_testing()

add_subdirectory(packages/aifr3d_core)
