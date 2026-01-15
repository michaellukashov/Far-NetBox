message(STATUS "CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.15)

project(NetBox)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Remove for compiler-specific features
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type")
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")
# message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

if(MSVC)
  # Set the desired runtime library for all configurations (Debug, Release, etc.)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:>$<IF:$<CONFIG:Debug>,,>")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Zi")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /Zi")
  # /MP Multi-processor compilation
  # /Zi Generate debug info

  # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
# message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

if(NOT DEFINED PROJECT_PLATFORM OR PROJECT_PLATFORM STREQUAL "")
if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
  set(PROJECT_PLATFORM "ARM64" CACHE STRING "Platform")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(PROJECT_PLATFORM "x64" CACHE STRING "Platform")
else()
  set(PROJECT_PLATFORM "x86" CACHE STRING "Platform")
endif()
endif()

if(PROJECT_PLATFORM STREQUAL "x86" AND CMAKE_BUILD_TYPE STREQUAL "Release")
  set(OPT_USE_UNITY_BUILD ON CACHE STRING "Default use unity build")
else()
  set(OPT_USE_UNITY_BUILD OFF CACHE STRING "Default use unity build")
endif()
set(OPT_CREATE_PLUGIN_DIR OFF CACHE STRING "Create plugin dir")

#-------------------------------------------------------------------------------

include(CMakeParseArguments)
include(${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
# include(CMakeDetermineCXXCompiler)
message("System: ${CMAKE_SYSTEM_NAME}, PROJECT_PLATFORM: ${PROJECT_PLATFORM}")
# message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
# message("MSVC_VERSION: ${MSVC_VERSION}")
# message("CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
# message("CMAKE_COMPILER_IS_MSVC: ${CMAKE_COMPILER_IS_MSVC}")
# message("CMAKE_COMPILER_IS_MINGW: ${CMAKE_COMPILER_IS_MINGW}")
# message("CMAKE_COMPILER_IS_CLANG: ${CMAKE_COMPILER_IS_CLANG}")
# message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
# message("CMAKE_CXX_COMPILER_ARCHITECTURE_ID: ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")

#-------------------------------------------------------------------------------
# Compiler Flags and Options (modularized in Phase 5)
# See cmake/NetBox.cmake for configuration
#-------------------------------------------------------------------------------

include(${CMAKE_SOURCE_DIR}/cmake/NetBox.cmake)

#-------------------------------------------------------------------------------

# OpenSSL - now built from libs/openssl-3/CMakeLists.txt
# See cmake/OpenSSL.cmake for configuration
#-------------------------------------------------------------------------------

# Add OpenSSL subdirectory
add_subdirectory(libs/openssl-3)

# OpenSSL libraries are now built from libs/openssl-3/CMakeLists.txt
# See cmake/OpenSSL.cmake for configuration

#-------------------------------------------------------------------------------
# Third-party Libraries (modularized in Phase 3)
# Each library is now built from its own libs/[library]/CMakeLists.txt
# See cmake/[Library].cmake for configuration
#-------------------------------------------------------------------------------

add_subdirectory(libs/atlmfc)
add_subdirectory(libs/putty)
add_subdirectory(libs/tinyxml2)
add_subdirectory(libs/neon)
add_subdirectory(libs/expat)
add_subdirectory(libs/zlib-ng)
add_subdirectory(libs/dlmalloc)
add_subdirectory(libs/tinylog)
add_subdirectory(libs/fmt)
add_subdirectory(libs/libs3)

#===============================================================================
# NetBox Main Plugin (src/CMakeLists.txt)
#===============================================================================

add_subdirectory(src)

#===============================================================================
# Compiler and Linker Flags (using ucm.cmake)
#===============================================================================

if(MSVC)
  ucm_set_runtime(STATIC)
  ucm_add_flags(CXX /EHs)
  ucm_remove_flags(CXX C /Ob1 /Ob2 CONFIG Release MinSizeRel RelWithDebInfo)
  ucm_add_flags(CXX C /fp:fast /Gw /GL CONFIG Release RelWithDebInfo MinSizeRel)
  ucm_add_flags(CXX C /RTC1 /DDEBUG CONFIG Debug)
  ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_RELEASE} CONFIG Release RelWithDebInfo MinSizeRel)
  ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_DEBUG} CONFIG Debug)
  ucm_add_linker_flags(MODULE SHARED /DEBUG /MAP CONFIG RelWithDebInfo)
endif()
# ucm_print_flags()
