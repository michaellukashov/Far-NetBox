#-------------------------------------------------------------------------------
# TinyXML2 Library Configuration
#
# This module provides functions and variables for building TinyXML2 XML library
# as part of NetBox project.
#
# NOTE: Paths are relative to libs/tinyxml2/ directory where this file is included.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Function: tinyxml2_get_sources
#
# Returns the list of TinyXML2 source files
#
# Output:
#   TINYXML2_SOURCES - List of source files (relative to libs/tinyxml2/)
#-------------------------------------------------------------------------------
function(tinyxml2_get_sources TINYXML2_SOURCES)
    set(${TINYXML2_SOURCES}
        tinyxml2.cpp
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: tinyxml2_apply_compile_options
#
# Applies TinyXML2 compile options to a targe
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(tinyxml2_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${NETBOX_CXX_FLAGS}
        ${NETBOX_UNICODE_FLAGS}
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: tinyxml2_get_include_dirs
#
# Returns TinyXML2 include directories
#
# Output:
#   TINYXML2_INCLUDE_DIRS - List of include directories
#-------------------------------------------------------------------------------
function(tinyxml2_get_include_dirs TINYXML2_INCLUDE_DIRS)
    set(${TINYXML2_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/libs
        ${CMAKE_SOURCE_DIR}/libs/GSL/include
        ${CMAKE_SOURCE_DIR}/src/include
        PARENT_SCOPE
    )
endfunction()
