#-------------------------------------------------------------------------------
# FMT Library Configuration
#
# This module provides functions and variables for building FMT string
# formatting library as part of NetBox project.
#
# NOTE: Paths are relative to libs/fmt/ directory where this file is included.
#
# Note: FMT is currently disabled in the build (wrapped in if(FALSE) in main CMakeLists.txt)
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Function: fmt_get_sources
#
# Returns the list of FMT source files
#
# Output:
#   FMT_SOURCES - List of source files (relative to libs/fmt/)
#-------------------------------------------------------------------------------
function(fmt_get_sources FMT_SOURCES)
    set(${FMT_SOURCES}
        fmt/format.cc
        fmt/format.h
        fmt/printf.cc
        fmt/printf.h
        fmt/string.h
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: fmt_apply_compile_options
#
# Applies FMT compile options to a target
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(fmt_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${NETBOX_CXX_FLAGS}
        ${NETBOX_UNICODE_FLAGS}
        ${NETBOX_PLATFORM_FLAGS}
    )

    target_compile_definitions(${TARGET} PRIVATE
        ${fmt_DEFINES}
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: fmt_get_include_dirs
#
# Returns FMT include directories
#
# Output:
#   FMT_INCLUDE_DIRS - List of include directories
#-------------------------------------------------------------------------------
function(fmt_get_include_dirs FMT_INCLUDE_DIRS)
    set(${FMT_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/libs
        ${CMAKE_SOURCE_DIR}/libs/GSL/include
        ${CMAKE_SOURCE_DIR}/src/include
        PARENT_SCOPE
    )
endfunction()
