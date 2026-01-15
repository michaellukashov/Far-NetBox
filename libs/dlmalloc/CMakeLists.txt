#-------------------------------------------------------------------------------
# DLMalloc Memory Allocator Build Configuration
#
# Builds dlmalloc static library
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Libraries/DLMalloc.cmake)

#-------------------------------------------------------------------------------
# Build dlmalloc library
#-------------------------------------------------------------------------------

dlmalloc_get_sources(DLMALLOC_SOURCES)

add_library(dlmalloc STATIC ${DLMALLOC_SOURCES})

dlmalloc_apply_compile_options(dlmalloc)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "DLMalloc Configuration:")
# message(STATUS "  dlmalloc sources: ${DLMALLOC_SOURCES}")
