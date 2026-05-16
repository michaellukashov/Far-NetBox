#-------------------------------------------------------------------------------
# Neon Library Configuration
#
# This module provides functions and variables for building Neon WebDAV library
# as part of NetBox project.
#
# NOTE: Paths are relative to libs/neon/ directory where this file is included.
#-------------------------------------------------------------------------------

# Set Neon compile flags (already defined in main CMakeLists.txt)
# LIBNEON_DEFS: -DNE_LFS -DNOCRYPT -DNE_HAVE_SSL -DHAVE_OPENSSL -DNE_HAVE_DAV -DNE_HAVE_ZLIB -DZLIB_COMPAT -DHAVE_EXPAT -DHAVE_EXPAT_H
# LIBEXPAT_DEFS: -DCOMPILED_FROM_DSP -DXML_STATIC

# Additional Neon-specific compile flags
set(NEON_COMPILE_FLAGS
    ${LIBNEON_DEFS}
    ${LIBEXPAT_DEFS}
)

if(MSVC)
    set(NEON_COMPILE_FLAGS ${NEON_COMPILE_FLAGS}
        -wd4996
    )
endif()

#-------------------------------------------------------------------------------
# Function: neon_apply_compile_options
#
# Applies common Neon compile options to a target
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(neon_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${NETBOX_C_FLAGS}
        ${NETBOX_UNICODE_FLAGS}
        ${NEON_COMPILE_FLAGS}
    )

    target_compile_definitions(${TARGET} PRIVATE
        -DWINSCP
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_WARNINGS
        -D_WINSOCK_DEPRECATED_NO_WARNINGS
        -DZLIB_NAME_MANGLING_H
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: neon_get_sources
#
# Returns the list of Neon source files
#
# Output:
#   NEON_SOURCES - List of source files (relative to libs/neon/)
#-------------------------------------------------------------------------------
function(neon_get_sources NEON_SOURCES)
    set(${NEON_SOURCES}
        src/ne_alloc.c
        src/ne_auth.c
        src/ne_basic.c
        src/ne_compress.c
        src/ne_dates.c
        src/ne_i18n.c
        src/ne_md5.c
        src/ne_pkcs11.c
        src/ne_redirect.c
        src/ne_request.c
        src/ne_session.c
        src/ne_socket.c
        src/ne_socks.c
        src/ne_sspi.c
        src/ne_string.c
        src/ne_uri.c
        src/ne_utils.c
        src/ne_207.c
        src/ne_xml.c
        src/ne_xmlreq.c
        src/ne_oldacl.c
        src/ne_acl3744.c
        src/ne_props.c
        src/ne_locks.c
        src/ne_openssl.c
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: neon_get_include_dirs
#
# Returns the list of Neon include directories
#
# Output:
#   NEON_INCLUDE_DIRS - List of include directories
#-------------------------------------------------------------------------------
function(neon_get_include_dirs NEON_INCLUDE_DIRS)
    set(${NEON_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/libs/zlib-ng
        ${CMAKE_SOURCE_DIR}/libs/expat/lib
        ${CMAKE_SOURCE_DIR}/libs/openssl-3/include
        PARENT_SCOPE
    )
endfunction()
