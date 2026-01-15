#-------------------------------------------------------------------------------
# zlib-ng Library Build Configuration
#
# Builds zlib static library
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/zlib-ng.cmake)

#-------------------------------------------------------------------------------
# Build zlib library
#-------------------------------------------------------------------------------

zlib_get_sources(ZLIB_SOURCES)
zlib_get_platform_sources(ZLIB_PLATFORM_SOURCES)

add_library(zlib STATIC
    ${ZLIB_SOURCES}
    ${ZLIB_PLATFORM_SOURCES}
)

zlib_get_include_dirs(ZLIB_INCLUDE_DIRS)
target_include_directories(zlib PRIVATE ${ZLIB_INCLUDE_DIRS})

zlib_apply_compile_options(zlib)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "zlib-ng Configuration:")
# message(STATUS "  zlib sources: ${ZLIB_SOURCES}")
# message(STATUS "  zlib platform sources: ${ZLIB_PLATFORM_SOURCES}")
# message(STATUS "  zlib include dirs: ${ZLIB_INCLUDE_DIRS}")
