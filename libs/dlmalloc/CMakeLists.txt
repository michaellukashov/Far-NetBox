# DLMalloc Memory Allocator Build Configuration
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/DLMalloc.cmake)

dlmalloc_get_sources(DLMALLOC_SOURCES)

add_library(dlmalloc STATIC ${DLMALLOC_SOURCES})
dlmalloc_apply_compile_options(dlmalloc)

# message(STATUS "DLMalloc Configuration:")
