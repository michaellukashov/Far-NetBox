#-------------------------------------------------------------------------------
# DLMalloc Library Configuration
#
# This module provides functions and variables for building DLMalloc memory
# allocator as part of NetBox project.
#
# NOTE: Paths are relative to libs/dlmalloc/ directory where this file is included.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Function: dlmalloc_get_sources
#
# Returns the list of DLMalloc source files
#
# Output:
#   DLMALLOC_SOURCES - List of source files (relative to libs/dlmalloc/)
#-------------------------------------------------------------------------------
function(dlmalloc_get_sources DLMALLOC_SOURCES)
    set(${DLMALLOC_SOURCES}
        dlmalloc.cc
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: dlmalloc_apply_compile_options
#
# Applies DLMalloc compile options to a target
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(dlmalloc_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${NETBOX_C_FLAGS}
        ${NETBOX_UNICODE_FLAGS}
    )
endfunction()
