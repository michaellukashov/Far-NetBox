#-------------------------------------------------------------------------------
# PuTTY Library Build Configuration
#
# Builds putty and puttyvs static libraries
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

# Include parent's ucm.cmake for utility functions
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Libraries/PuTTY.cmake)

#-------------------------------------------------------------------------------
# Build putty library
#-------------------------------------------------------------------------------

putty_get_sources(PUTTY_SOURCES)

add_library(putty STATIC ${PUTTY_SOURCES})

target_include_directories(putty PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/putty
    ${CMAKE_SOURCE_DIR}/libs/putty/windows
)

putty_apply_compile_options(putty)

#-------------------------------------------------------------------------------
# Build puttyvs library (vectorized AES/SHA)
#-------------------------------------------------------------------------------

puttyvs_get_sources(PUTTYVS_SOURCES)

add_library(puttyvs STATIC ${PUTTYVS_SOURCES})

target_include_directories(puttyvs PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/putty
    ${CMAKE_SOURCE_DIR}/libs/putty/windows
)

puttyvs_apply_compile_options(puttyvs)
target_compile_options(puttyvs PRIVATE
    -DWINSCP_VS
    ${NETBOX_UNICODE_FLAGS}
)

#-------------------------------------------------------------------------------
# Debug output (optional, for verification)
#-------------------------------------------------------------------------------

message(STATUS "PuTTY Configuration:")
# message(STATUS "  putty sources: ${PUTTY_SOURCES}")
message(STATUS "  puttyvs sources: ${PUTTYVS_SOURCES}")
