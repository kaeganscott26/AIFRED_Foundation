add_library(aifr3d_core STATIC
  src/placeholder.cpp
)

target_include_directories(aifr3d_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(tests)
