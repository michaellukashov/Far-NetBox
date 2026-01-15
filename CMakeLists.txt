cmake_minimum_required(VERSION 3.15)

#===============================================================================
# Project Configuration
#===============================================================================

project(NetBox
    LANGUAGES C CXX
    DESCRIPTION "Far-NetBox SFTP/FTP/SCP/WebDAV/S3 client for Far Manager"
    HOMEPAGE_URL "https://github.com/michaellukashov/Far-NetBox"
)

#-------------------------------------------------------------------------------
# C++ Standard Configuration
#-------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#-------------------------------------------------------------------------------
# Build Options
#-------------------------------------------------------------------------------

# Build type selection
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Platform selection (auto-detect if not specified)
if(NOT DEFINED PROJECT_PLATFORM OR PROJECT_PLATFORM STREQUAL "")
    if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
        set(PROJECT_PLATFORM "ARM64" CACHE STRING "Target platform (x86, x64, ARM64)")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PROJECT_PLATFORM "x64" CACHE STRING "Target platform (x86, x64, ARM64)")
    else()
        set(PROJECT_PLATFORM "x86" CACHE STRING "Target platform (x86, x64, ARM64)")
    endif()
endif()

# Unity builds: enabled for x86 Release builds by default
set(OPT_USE_UNITY_BUILD OFF CACHE BOOL "Enable unity builds for faster compilation")
if(PROJECT_PLATFORM STREQUAL "x86" AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(OPT_USE_UNITY_BUILD ON)
endif()

# Plugin directory creation
set(OPT_CREATE_PLUGIN_DIR OFF CACHE BOOL "Create plugin directory structure")

# IDE support: generate compile_commands.json
set(OPT_COMPILE_COMMANDS OFF CACHE BOOL "Generate compile_commands.json for IDE support")

message(STATUS "System: ${CMAKE_SYSTEM_NAME}, PROJECT_PLATFORM: ${PROJECT_PLATFORM}")

#===============================================================================
# CMake Modules
#===============================================================================

include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/NetBox.cmake)

#===============================================================================
# Third-Party Libraries
#===============================================================================

add_subdirectory(libs/openssl-3)   # OpenSSL 3.0 (ssleay32, libeay32)
add_subdirectory(libs/atlmfc)      # Minimal MFC subset
add_subdirectory(libs/putty)       # SSH/SCP library
add_subdirectory(libs/tinyxml2)    # XML parser
add_subdirectory(libs/neon)        # WebDAV protocol library
add_subdirectory(libs/expat)       # XML parsing library
add_subdirectory(libs/zlib-ng)     # Compression library
add_subdirectory(libs/dlmalloc)    # Memory allocator
add_subdirectory(libs/tinylog)     # Logging library
add_subdirectory(libs/fmt)         # String formatting library
add_subdirectory(libs/libs3)       # S3 protocol library

#===============================================================================
# Main Plugin Target
#===============================================================================

add_subdirectory(src)

#===============================================================================
# Export compile_commands.json (Optional)
#===============================================================================

if(OPT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()
