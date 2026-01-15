#-------------------------------------------------------------------------------
# Expat XML Library Build Configuration
#
# Builds expat static library
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Libraries/Expat.cmake)

#-------------------------------------------------------------------------------
# Build expat library
#-------------------------------------------------------------------------------

expat_get_sources(EXPAT_SOURCES)

add_library(expat STATIC ${EXPAT_SOURCES})

expat_get_include_dirs(EXPAT_INCLUDE_DIRS)
target_include_directories(expat PRIVATE ${EXPAT_INCLUDE_DIRS})

expat_apply_compile_options(expat)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "Expat Configuration:")
# message(STATUS "  expat sources: ${EXPAT_SOURCES}")
# message(STATUS "  expat include dirs: ${EXPAT_INCLUDE_DIRS}")
