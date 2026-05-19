# TinyLog Library Build Configuration
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/TinyLog.cmake)

tinylog_get_sources(TINYLOG_SOURCES)
tinylog_get_headers(TINYLOG_HEADERS)

add_library(tinylog STATIC ${TINYLOG_SOURCES} ${TINYLOG_HEADERS})
tinylog_get_include_dirs(TINYLOG_INCLUDE_DIRS)
target_include_directories(tinylog PRIVATE ${TINYLOG_INCLUDE_DIRS})
tinylog_apply_compile_options(tinylog)

# message(STATUS "TinyLog Configuration:")
