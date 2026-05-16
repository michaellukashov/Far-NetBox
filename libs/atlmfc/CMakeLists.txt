# ATLMFC Library Build Configuration
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/ATLMFC.cmake)

atlmfc_get_sources(ATLMFC_SOURCES)

add_library(atlmfc STATIC ${ATLMFC_SOURCES})
atlmfc_get_include_dirs(ATLMFC_INCLUDE_DIRS)
target_include_directories(atlmfc PRIVATE ${ATLMFC_INCLUDE_DIRS})
atlmfc_apply_compile_options(atlmfc)

# message(STATUS "ATLMFC Configuration:")
