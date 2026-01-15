#-------------------------------------------------------------------------------
# TinyXML2 XML Library Build Configuration
#
# Builds tinyxml2 static library
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/TinyXML2.cmake)

#-------------------------------------------------------------------------------
# Build tinyxml2 library
#-------------------------------------------------------------------------------

tinyxml2_get_sources(TINYXML2_SOURCES)

add_library(tinyxml2 STATIC ${TINYXML2_SOURCES})

tinyxml2_get_include_dirs(TINYXML2_INCLUDE_DIRS)
target_include_directories(tinyxml2 PRIVATE ${TINYXML2_INCLUDE_DIRS})

tinyxml2_apply_compile_options(tinyxml2)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "TinyXML2 Configuration:")
# message(STATUS "  tinyxml2 sources: ${TINYXML2_SOURCES}")
# message(STATUS "  tinyxml2 include dirs: ${TINYXML2_INCLUDE_DIRS}")
