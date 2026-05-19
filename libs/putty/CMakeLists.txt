# PuTTY Library Build Configuration
cmake_minimum_required(VERSION 3.15)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/PuTTY.cmake)

putty_get_sources(PUTTY_SOURCES)
add_library(putty STATIC ${PUTTY_SOURCES})
target_include_directories(putty PRIVATE ${CMAKE_SOURCE_DIR}/libs/putty ${CMAKE_SOURCE_DIR}/libs/putty/windows)
putty_apply_compile_options(putty)

puttyvs_get_sources(PUTTYVS_SOURCES)
add_library(puttyvs STATIC ${PUTTYVS_SOURCES})
target_include_directories(puttyvs PRIVATE ${CMAKE_SOURCE_DIR}/libs/putty ${CMAKE_SOURCE_DIR}/libs/putty/windows)
puttyvs_apply_compile_options(puttyvs)
target_compile_options(puttyvs PRIVATE -DWINSCP_VS ${NETBOX_UNICODE_FLAGS})

# message(STATUS "PuTTY Configuration:")
