#-------------------------------------------------------------------------------
# Expat Library Configuration
#
# This module provides functions and variables for building Expat XML library
# as part of NetBox project.
#
# NOTE: Paths are relative to libs/expat/ directory where this file is included.
#-------------------------------------------------------------------------------

# Expat compile flags (already defined in main CMakeLists.txt)
# LIBEXPAT_DEFS: -DCOMPILED_FROM_DSP -DXML_STATIC

#-------------------------------------------------------------------------------
# Function: expat_get_sources
#
# Returns the list of Expat source files
#
# Output:
#   EXPAT_SOURCES - List of source files (relative to libs/expat/)
#-------------------------------------------------------------------------------
function(expat_get_sources EXPAT_SOURCES)
    set(${EXPAT_SOURCES}
        lib/xmlparse.c
        lib/xmlrole.c
        lib/xmltok.c
        lib/xmltok_impl.c
        lib/loadlibrary.c
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: expat_apply_compile_options
#
# Applies Expat compile options to a target
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(expat_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${NETBOX_C_FLAGS}
        ${NETBOX_UNICODE_FLAGS}
        ${LIBEXPAT_DEFS}
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: expat_get_include_dirs
#
# Returns Expat include directories
#
# Output:
#   EXPAT_INCLUDE_DIRS - List of include directories
#-------------------------------------------------------------------------------
function(expat_get_include_dirs EXPAT_INCLUDE_DIRS)
    set(${EXPAT_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/libs/expat
        PARENT_SCOPE
    )
endfunction()
