# TinyXML2 XML Library Build Configuration
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/TinyXML2.cmake)

tinyxml2_get_sources(TINYXML2_SOURCES)

add_library(tinyxml2 STATIC ${TINYXML2_SOURCES})
tinyxml2_get_include_dirs(TINYXML2_INCLUDE_DIRS)
target_include_directories(tinyxml2 PRIVATE ${TINYXML2_INCLUDE_DIRS})
tinyxml2_apply_compile_options(tinyxml2)

# message(STATUS "TinyXML2 Configuration:")
