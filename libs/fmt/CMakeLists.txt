#-------------------------------------------------------------------------------
# FMT Library Build Configuration
#
# Builds fmt static library
#
# Note: FMT is currently disabled in the build (wrapped in if(FALSE) in main CMakeLists.txt)
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Libraries/FMT.cmake)

#-------------------------------------------------------------------------------
# Build fmt library
#-------------------------------------------------------------------------------

fmt_get_sources(FMT_SOURCES)

add_library(fmt STATIC ${FMT_SOURCES})

fmt_get_include_dirs(FMT_INCLUDE_DIRS)
target_include_directories(fmt PRIVATE ${FMT_INCLUDE_DIRS})

fmt_apply_compile_options(fmt)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "FMT Configuration:")
# message(STATUS "  fmt sources: ${FMT_SOURCES}")
# message(STATUS "  fmt include dirs: ${FMT_INCLUDE_DIRS}")
