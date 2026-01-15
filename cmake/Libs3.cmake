#-------------------------------------------------------------------------------
# Libs3 Library Configuration
#
# This module provides functions and variables for building Libs3 S3 library
# as part of NetBox project.
#
# NOTE: Paths are relative to libs/libs3/ directory where this file is included.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Function: libs3_get_sources
#
# Returns the list of Libs3 source files
#
# Output:
#   LIBS3_SOURCES - List of source files (relative to libs/libs3/)
#-------------------------------------------------------------------------------
function(libs3_get_sources LIBS3_SOURCES)
    set(${LIBS3_SOURCES}
        src/bucket.c
        src/bucket_metadata.c
        src/error_parser.c
        src/general.c
        src/mingw_functions.c
        src/multipart.c
        src/object.c
        src/request.c
        src/request_context.c
        src/response_headers_handler.c
        src/service.c
        src/service_access_logging.c
        src/simplexml.c
        src/util.c
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: libs3_get_headers
#
# Returns the list of Libs3 header files
#
# Output:
#   LIBS3_HEADERS - List of header files (relative to libs/libs3/)
#-------------------------------------------------------------------------------
function(libs3_get_headers LIBS3_HEADERS)
    set(${LIBS3_HEADERS}
        inc/string_buffer.h
        inc/util.h
        inc/simplexml.h
        inc/response_headers_handler.h
        inc/request.h
        inc/request_context.h
        inc/libs3.h
        inc/error_parser.h
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: libs3_apply_compile_options
#
# Applies Libs3 compile options to a target
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(libs3_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${NETBOX_C_FLAGS}
        ${NETBOX_UNICODE_FLAGS}
    )

    target_compile_definitions(${TARGET} PRIVATE
        ${LIBEXPAT_DEFS}
        -DS3_EXPORT
        -DWIN32
        -DWINSCP
        -DNE_LFS
        -DNEED_LIBS3
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: libs3_get_include_dirs
#
# Returns Libs3 include directories
#
# Output:
#   LIBS3_INCLUDE_DIRS - List of include directories
#-------------------------------------------------------------------------------
function(libs3_get_include_dirs LIBS3_INCLUDE_DIRS)
    set(${LIBS3_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/libs
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/mingw
        ${CMAKE_SOURCE_DIR}/libs/expat/lib
        ${CMAKE_SOURCE_DIR}/libs/neon/src
        ${CMAKE_SOURCE_DIR}/libs/openssl-3/include
        PARENT_SCOPE
    )
endfunction()
