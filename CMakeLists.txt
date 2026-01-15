cmake_minimum_required(VERSION 3.15)

#-------------------------------------------------------------------------------
# Project Configuration
#-------------------------------------------------------------------------------

project(NetBox
    LANGUAGES C CXX
    DESCRIPTION "Far-NetBox SFTP/FTP/SCP/WebDAV/S3 client for Far Manager"
    HOMEPAGE_URL "https://github.com/michaellukashov/Far-NetBox"
)

# C++ Standard - C++17 required, no compiler-specific extensions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# IDE support - generate compile_commands.json
set(OPT_COMPILE_COMMANDS OFF CACHE BOOL "Generate compile_commands.json for IDE support")

#-------------------------------------------------------------------------------
# Build Options
#-------------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(NOT DEFINED PROJECT_PLATFORM OR PROJECT_PLATFORM STREQUAL "")
    if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
        set(PROJECT_PLATFORM "ARM64" CACHE STRING "Target platform (x86, x64, ARM64)")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PROJECT_PLATFORM "x64" CACHE STRING "Target platform (x86, x64, ARM64)")
    else()
        set(PROJECT_PLATFORM "x86" CACHE STRING "Target platform (x86, x64, ARM64)")
    endif()
endif()

# Unity builds for faster x86 Release compilation
set(OPT_USE_UNITY_BUILD OFF CACHE BOOL "Enable unity builds for faster compilation")
if(PROJECT_PLATFORM STREQUAL "x86" AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(OPT_USE_UNITY_BUILD ON)
endif()

set(OPT_CREATE_PLUGIN_DIR OFF CACHE BOOL "Create plugin directory structure")

#-------------------------------------------------------------------------------
# MSVC-Specific Configuration
#-------------------------------------------------------------------------------

if(MSVC)
    # Static runtime for all configurations (avoids DLL dependency issues)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:>$<IF:$<CONFIG:Debug>,,>")
endif()

message(STATUS "System: ${CMAKE_SYSTEM_NAME}, PROJECT_PLATFORM: ${PROJECT_PLATFORM}")

#-------------------------------------------------------------------------------
# Utility and Configuration Modules
#-------------------------------------------------------------------------------

include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/NetBox.cmake)

#-------------------------------------------------------------------------------
# Third-party Libraries
#-------------------------------------------------------------------------------

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

#-------------------------------------------------------------------------------
# Main Plugin Target
#-------------------------------------------------------------------------------

add_subdirectory(src)

#-------------------------------------------------------------------------------
# MSVC Compiler and Linker Flags
#-------------------------------------------------------------------------------

if(MSVC)
    # Static runtime for all configurations
    ucm_set_runtime(STATIC)

    # Multi-processor compilation and debug info
    # string(APPEND CMAKE_CXX_FLAGS " /MP /Zi")
    # string(APPEND CMAKE_C_FLAGS " /MP /Zi")
    ucm_add_flags(CXX C /MP /Zi)

    # Optimization flags for non-Debug builds
    ucm_add_flags(CXX /EHs)
    ucm_add_flags(CXX C /fp:fast /Gw /GL)
    ucm_remove_flags(CXX C /Ob1 /Ob2 CONFIG Release MinSizeRel RelWithDebInfo)

    # Debug flags
    ucm_add_flags(CXX C /RTC1 /DDEBUG CONFIG Debug)

    # Linker flags
    ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_RELEASE} CONFIG Release RelWithDebInfo MinSizeRel)
    ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_DEBUG} CONFIG Debug)
    ucm_add_linker_flags(MODULE SHARED /DEBUG /MAP CONFIG RelWithDebInfo)
endif()

# Export compile_commands.json if requested
if(OPT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()
