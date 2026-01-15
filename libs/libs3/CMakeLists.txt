#-------------------------------------------------------------------------------
# Libs3 S3 Library Build Configuration
#
# Builds s3 static library
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Libraries/Libs3.cmake)

#-------------------------------------------------------------------------------
# Build s3 library
#-------------------------------------------------------------------------------

libs3_get_sources(LIBS3_SOURCES)
libs3_get_headers(LIBS3_HEADERS)

add_library(s3 STATIC
    ${LIBS3_SOURCES}
    ${LIBS3_HEADERS}
)

libs3_get_include_dirs(LIBS3_INCLUDE_DIRS)
target_include_directories(s3 PRIVATE ${LIBS3_INCLUDE_DIRS})

libs3_apply_compile_options(s3)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "Libs3 Configuration:")
# message(STATUS "  libs3 sources: ${LIBS3_SOURCES}")
# message(STATUS "  libs3 include dirs: ${LIBS3_INCLUDE_DIRS}")
