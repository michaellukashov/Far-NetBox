#-------------------------------------------------------------------------------
# TinyLog Library Configuration
#
# This module provides functions and variables for building TinyLog logging
# library as part of NetBox project.
#
# NOTE: Paths are relative to libs/tinylog/ directory where this file is included.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Function: tinylog_get_sources
#
# Returns the list of TinyLog source files
#
# Output:
#   TINYLOG_SOURCES - List of source files (relative to libs/tinylog/)
#-------------------------------------------------------------------------------
function(tinylog_get_sources TINYLOG_SOURCES)
    set(${TINYLOG_SOURCES}
        src/TinyLog.cpp
        src/Utils.cpp
        src/LogStream.cpp
        src/Buffer.cpp
        src/LockFreeQueue.cpp
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: tinylog_get_headers
#
# Returns the list of TinyLog header files
#
# Output:
#   TINYLOG_HEADERS - List of header files (relative to libs/tinylog/)
#-------------------------------------------------------------------------------
function(tinylog_get_headers TINYLOG_HEADERS)
    set(${TINYLOG_HEADERS}
        tinylog/TinyLog.h
        tinylog/Utils.h
        tinylog/LogStream.h
        tinylog/Config.h
        tinylog/Buffer.h
        tinylog/LockFreeQueue.h
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: tinylog_apply_compile_options
#
# Applies TinyLog compile options to a targe
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(tinylog_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${NETBOX_CXX_FLAGS}
        ${NETBOX_UNICODE_FLAGS}
        ${NETBOX_PLATFORM_FLAGS}
    )

    target_compile_definitions(${TARGET} PRIVATE
        -DTINYLOG_EXPOR
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: tinylog_get_include_dirs
#
# Returns TinyLog include directories
#
# Output:
#   TINYLOG_INCLUDE_DIRS - List of include directories
#-------------------------------------------------------------------------------
function(tinylog_get_include_dirs TINYLOG_INCLUDE_DIRS)
    set(${TINYLOG_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/libs
        ${CMAKE_SOURCE_DIR}/libs/fmt
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libs/GSL/include
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_SOURCE_DIR}/src/base
        PARENT_SCOPE
    )
endfunction()
