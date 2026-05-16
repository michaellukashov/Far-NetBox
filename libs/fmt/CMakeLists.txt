# FMT Library Build Configuration
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/FMT.cmake)

fmt_get_sources(FMT_SOURCES)

add_library(fmt STATIC ${FMT_SOURCES})
fmt_get_include_dirs(FMT_INCLUDE_DIRS)
target_include_directories(fmt PRIVATE ${FMT_INCLUDE_DIRS})
fmt_apply_compile_options(fmt)

# message(STATUS "FMT Configuration:")
