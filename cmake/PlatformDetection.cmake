#===============================================================================
# NetBox Platform Detection Module
#
# Purpose: Auto-detect target platform and set PROJECT_PLATFORM variable
#
# Usage:
#   include(${CMAKE_SOURCE_DIR}/cmake/PlatformDetection.cmake)
#   netbox_detect_platform()
#===============================================================================

function(netbox_detect_platform)
    if(NOT DEFINED PROJECT_PLATFORM OR PROJECT_PLATFORM STREQUAL "")
        if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
            set(PROJECT_PLATFORM "ARM64" CACHE STRING "Target platform (x86, x64, ARM64)")
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(PROJECT_PLATFORM "x64" CACHE STRING "Target platform (x86, x64, ARM64)")
        else()
            set(PROJECT_PLATFORM "x86" CACHE STRING "Target platform (x86, x64, ARM64)")
        endif()
    endif()
endfunction()
