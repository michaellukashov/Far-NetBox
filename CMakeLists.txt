#===============================================================================
# NetBox: SFTP/FTP/SCP/WebDAV/S3 client for Far Manager 3.0
#===============================================================================

cmake_minimum_required(VERSION 3.15)

project(NetBox
    LANGUAGES C CXX
    DESCRIPTION "Far-NetBox SFTP/FTP/SCP/WebDAV/S3 client for Far Manager"
    HOMEPAGE_URL "https://github.com/michaellukashov/Far-NetBox"
)

#===============================================================================
# Global Configuration
#===============================================================================

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include custom modules
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/NetBox.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Libraries.cmake)

#===============================================================================
# Build Options and Platform Detection
#===============================================================================

# Build type selection (Debug, Release, RelWithDebInfo, MinSizeRel)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
endif()

# Platform auto-detection (x86, x64, ARM64)
include(${CMAKE_SOURCE_DIR}/cmake/PlatformDetection.cmake)
netbox_detect_platform()

#-------------------------------------------------------------------------------
# CMake Options
#-------------------------------------------------------------------------------

# OPT_USE_UNITY_BUILD: Enable unity builds for faster compilation on x86 Release
if(PROJECT_PLATFORM STREQUAL "x86" AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(OPT_USE_UNITY_BUILD ON CACHE BOOL "Enable unity builds for faster compilation")
else()
    set(OPT_USE_UNITY_BUILD OFF CACHE BOOL "Enable unity builds for faster compilation")
endif()

# OPT_CREATE_PLUGIN_DIR: Create Far3_<platform>/Plugins/NetBox/ directory structure
set(OPT_CREATE_PLUGIN_DIR OFF CACHE BOOL "Create plugin directory structure")

# OPT_COMPILE_COMMANDS: Generate compile_commands.json for IDE support
set(OPT_COMPILE_COMMANDS OFF CACHE BOOL "Generate compile_commands.json for IDE support")
if(OPT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

#-------------------------------------------------------------------------------
# Standard Include Directories
#-------------------------------------------------------------------------------

# Define standard include directories for the project
set(NETBOX_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/base
    ${CMAKE_SOURCE_DIR}/src/resource
)

# Status messages
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}, PROJECT_PLATFORM: ${PROJECT_PLATFORM}")

#===============================================================================
# Third-Party Libraries
#===============================================================================

# Configure all libraries using centralized module
netbox_configure_libraries()

#===============================================================================
# Main Plugin Targe
#===============================================================================

add_subdirectory(src)
