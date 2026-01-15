# Expat XML Library Build Configuration
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Expat.cmake)

expat_get_sources(EXPAT_SOURCES)
expat_get_include_dirs(EXPAT_INCLUDE_DIRS)

add_library(expat STATIC ${EXPAT_SOURCES})
target_include_directories(expat PRIVATE ${EXPAT_INCLUDE_DIRS})
expat_apply_compile_options(expat)

# message(STATUS "Expat Configuration:")
