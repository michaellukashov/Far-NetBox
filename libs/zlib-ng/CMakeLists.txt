# zlib-ng Library Build Configuration
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/zlib-ng.cmake)

zlib_get_sources(ZLIB_SOURCES)
zlib_get_platform_sources(ZLIB_PLATFORM_SOURCES)

add_library(zlib STATIC ${ZLIB_SOURCES} ${ZLIB_PLATFORM_SOURCES})
zlib_get_include_dirs(ZLIB_INCLUDE_DIRS)
target_include_directories(zlib PRIVATE ${ZLIB_INCLUDE_DIRS})
zlib_apply_compile_options(zlib)

# message(STATUS "zlib-ng Configuration:")
