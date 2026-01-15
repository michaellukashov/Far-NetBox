#-------------------------------------------------------------------------------
# Neon WebDAV Library Build Configuration
#
# Builds neon static library
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Libraries/Neon.cmake)

#-------------------------------------------------------------------------------
# Build neon library
#-------------------------------------------------------------------------------

neon_get_sources(NEON_SOURCES)

add_library(neon STATIC ${NEON_SOURCES})

neon_get_include_dirs(NEON_INCLUDE_DIRS)
target_include_directories(neon PRIVATE ${NEON_INCLUDE_DIRS})

neon_apply_compile_options(neon)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "Neon Configuration:")
# message(STATUS "  neon sources: ${NEON_SOURCES}")
# message(STATUS "  neon include dirs: ${NEON_INCLUDE_DIRS}")
