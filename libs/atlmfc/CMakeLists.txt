#-------------------------------------------------------------------------------
# ATLMFC Library Build Configuration
#
# Builds atlmfc static library (minimal MFC subset)
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Libraries/ATLMFC.cmake)

#-------------------------------------------------------------------------------
# Build atlmfc library
#-------------------------------------------------------------------------------

atlmfc_get_sources(ATLMFC_SOURCES)

add_library(atlmfc STATIC ${ATLMFC_SOURCES})

atlmfc_get_include_dirs(ATLMFC_INCLUDE_DIRS)
target_include_directories(atlmfc PRIVATE ${ATLMFC_INCLUDE_DIRS})

atlmfc_apply_compile_options(atlmfc)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "ATLMFC Configuration:")
# message(STATUS "  atlmfc sources: ${ATLMFC_SOURCES}")
# message(STATUS "  atlmfc include dirs: ${ATLMFC_INCLUDE_DIRS}")
