#-------------------------------------------------------------------------------
# TinyLog Library Build Configuration
#
# Builds tinylog static library
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/TinyLog.cmake)

#-------------------------------------------------------------------------------
# Build tinylog library
#-------------------------------------------------------------------------------

tinylog_get_sources(TINYLOG_SOURCES)
tinylog_get_headers(TINYLOG_HEADERS)

add_library(tinylog STATIC
    ${TINYLOG_SOURCES}
    ${TINYLOG_HEADERS}
)

tinylog_get_include_dirs(TINYLOG_INCLUDE_DIRS)
target_include_directories(tinylog PRIVATE ${TINYLOG_INCLUDE_DIRS})

tinylog_apply_compile_options(tinylog)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "TinyLog Configuration:")
# message(STATUS "  tinylog sources: ${TINYLOG_SOURCES}")
# message(STATUS "  tinylog include dirs: ${TINYLOG_INCLUDE_DIRS}")
