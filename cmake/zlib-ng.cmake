#-------------------------------------------------------------------------------
# zlib-ng Library Configuration
#
# This module provides functions and variables for building zlib-ng library
# as part of NetBox project.
#
# NOTE: Paths are relative to libs/zlib-ng/ directory where this file is included.
#-------------------------------------------------------------------------------

# Set zlib-ng compile flags (platform-specific logic is in the CMakeLists.txt)

#-------------------------------------------------------------------------------
# Function: zlib_get_sources
#
# Returns the list of zlib-ng source files (base sources, platform-specific)
#
# Output:
#   ZLIB_SOURCES - List of source files (relative to libs/zlib-ng/)
#-------------------------------------------------------------------------------
function(zlib_get_sources ZLIB_SOURCES)
    set(${ZLIB_SOURCES}
        adler32.c
        infback.c
        inflate.c
        inftrees.c
        trees.c
        zutil.c
        deflate.c
        deflate_slow.c
        deflate_medium.c
        deflate_fast.c
        crc32.c
        insert_string.c
        arch/generic/crc32_fold_c.c
        arch/generic/slide_hash_c.c
        arch/generic/chunkset_c.c
        arch/generic/compare256_c.c
        arch/generic/crc32_braid_c.c
        arch/generic/adler32_c.c
        arch/generic/adler32_fold_c.c
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: zlib_get_platform_sources
#
# Returns platform-specific zlib-ng source files
#
# Arguments:
#   PLATFORM_SOURCES - Variable to store platform-specific sources
#-------------------------------------------------------------------------------
function(zlib_get_platform_sources PLATFORM_SOURCES)
    if(PROJECT_PLATFORM STREQUAL "ARM64")
        set(${PLATFORM_SOURCES}
            arch/arm/slide_hash_neon.c
            arch/arm/crc32_acle.c
            arch/arm/slide_hash_armv6.c
            arch/arm/chunkset_neon.c
            arch/arm/compare256_neon.c
            arch/arm/adler32_neon.c
            arch/arm/arm_features.c
            PARENT_SCOPE
        )
    else()
        set(${PLATFORM_SOURCES}
            arch/x86/x86_features.c
            arch/x86/slide_hash_avx2.c
            arch/x86/slide_hash_sse2.c
            arch/x86/crc32_pclmulqdq.c
            arch/x86/crc32_vpclmulqdq.c
            arch/x86/compare256_avx2.c
            arch/x86/compare256_sse2.c
            arch/x86/chunkset_sse2.c
            arch/x86/chunkset_ssse3.c
            arch/x86/chunkset_avx2.c
            arch/x86/chunkset_avx512.c
            arch/x86/adler32_avx512_vnni.c
            arch/x86/adler32_sse42.c
            arch/x86/adler32_ssse3.c
            arch/x86/adler32_avx512.c
            arch/x86/adler32_avx2.c
            PARENT_SCOPE
        )
    endif()
endfunction()

#-------------------------------------------------------------------------------
# Function: zlib_get_include_dirs
#
# Returns zlib-ng include directories (base + platform-specific)
#
# Arguments:
#   INCLUDE_DIRS - Variable to store include directories
#-------------------------------------------------------------------------------
function(zlib_get_include_dirs INCLUDE_DIRS)
    set(${INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/generic
        PARENT_SCOPE
    )

    if(PROJECT_PLATFORM STREQUAL "ARM64")
        list(APPEND ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/arch/arm)
    else()
        list(APPEND ${INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86)
    endif()

endfunction()

#-------------------------------------------------------------------------------
# Function: zlib_apply_compile_options
#
# Applies zlib-ng compile options to a targe
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(zlib_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${NETBOX_C_FLAGS}
        ${NETBOX_UNICODE_FLAGS}
    )

    target_compile_definitions(${TARGET} PRIVATE
        -DZLIB_COMPA
        -DZLIB_NAME_MANGLING_H
        -DMEMCPY=memcpy
        -DMEMSET=memse
    )

    if(PROJECT_PLATFORM STREQUAL "ARM64")
        target_compile_definitions(${TARGET} PRIVATE
            -D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE=1
            -D_CRT_SECURE_NO_DEPRECATE
            -D_CRT_NONSTDC_NO_DEPRECATE
            -DARM_FEATURES
            -DARM_NEON_HASLD4
        )
    endif()
endfunction()
