# Neon WebDAV Library Build Configuration
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Neon.cmake)

neon_get_sources(NEON_SOURCES)
neon_get_include_dirs(NEON_INCLUDE_DIRS)

add_library(neon STATIC ${NEON_SOURCES})
target_include_directories(neon PRIVATE ${NEON_INCLUDE_DIRS})
neon_apply_compile_options(neon)

# message(STATUS "Neon Configuration:")
