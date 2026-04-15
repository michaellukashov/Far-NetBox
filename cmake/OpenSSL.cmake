#-------------------------------------------------------------------------------
# OpenSSL Library Configuration
#
# This module provides functions and variables for building OpenSSL libraries
# (libeay32 and ssleay32) as part of NetBox project.
#-------------------------------------------------------------------------------

# Set OpenSSL compile flags
set(OPENSSL_COMPILE_FLAGS
    -DWINSCP
    -DMK1MF_BUILD -DWIN32_LEAN_AND_MEAN -DVC_EXTRALEAN -DSECURITY_WIN32
    -DDSO_WIN32
    -DL_ENDIAN -D_stricmp=stricmp -D_strnicmp=strnicmp
    -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS
    -DOPENSSL_NO_MDC2
    -DOPENSSL_NO_WHIRLPOOL
    -DOPENSSL_NO_SEED
    -DOPENSSL_NO_ENGINE -DOPENSSL_NO_DYNAMIC_ENGINE
    -DOPENSSL_NO_QUIC -DOPENSSL_NO_BROTLI -DOPENSSL_NO_COMP -DOPENSSL_NO_COMP_ALG
    -DOPENSSL_NO_TFO -DOPENSSL_NO_ASYNC
    -DOPENSSL_NO_ERR
    -DOPENSSL_NO_CAMELLIA
    -DOPENSSL_NO_DGRAM
    -DOPENSSL_NO_DEPRECATED_1_1_0
    -DOPENSSL_NO_DEPRECATED_3_0
    -DOPENSSL_NO_PINSHARED
    -DOPENSSL_API_COMPAT=11000
)

 if(MSVC)
     set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
         /Zi /Zl
         -wd4090 -wd4133 -wd4319 -wd4005 -wd4311
     )
 endif()

#-------------------------------------------------------------------------------
# Function: openssl_apply_compile_options
#
# Applies common OpenSSL compile options to a target
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(openssl_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${OPENSSL_COMPILE_FLAGS}
        ${NETBOX_PLATFORM_FLAGS}
    )

    target_include_directories(${TARGET} SYSTEM BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/openssl-3/include
        ${CMAKE_SOURCE_DIR}/libs/openssl-3/include/crypto
        ${CMAKE_SOURCE_DIR}/libs/openssl-3/crypto/modes
        ${CMAKE_SOURCE_DIR}/libs/openssl-3/crypto/ec/curve448
        ${CMAKE_SOURCE_DIR}/libs/openssl-3
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: openssl_setup_asm_files
#
# Sets up platform-specific ASM files for OpenSSL
#
# Arguments:
#   RESULT_VAR - Variable name to store ASM file list
#
# Output:
#   Sets RESULT_VAR with list of ASM object files
#-------------------------------------------------------------------------------
function(openssl_setup_asm_files RESULT_VAR)
    set(_asm_files )

    if(PROJECT_PLATFORM STREQUAL "x64")
        set(_nasm_args win64)
        set(_asm_file_list
            crypto/sha/sha512-x86_64.asm
            crypto/sha/sha256-x86_64.asm
            crypto/sha/sha256-mb-x86_64.asm
            crypto/sha/sha1-x86_64.asm
            crypto/sha/sha1-mb-x86_64.asm
            crypto/sha/keccak1600-x86_64.asm
            crypto/md5/md5-x86_64.asm
            crypto/ec/x25519-x86_64.asm
            crypto/ec/ecp_nistz256-x86_64.asm
            crypto/poly1305/poly1305-x86_64.asm
            crypto/aes/vpaes-x86_64.asm
            crypto/aes/bsaes-x86_64.asm
            crypto/aes/aesni-x86_64.asm
            crypto/aes/aesni-sha256-x86_64.asm
            crypto/aes/aesni-sha1-x86_64.asm
            crypto/aes/aesni-mb-x86_64.asm
            crypto/aes/aes-x86_64.asm
            crypto/bn/x86_64-mont5.asm
            crypto/bn/x86_64-mont.asm
            crypto/bn/x86_64-gf2m.asm
            crypto/bn/rsaz-x86_64.asm
            crypto/bn/rsaz-avx2.asm
            crypto/bn/rsaz-4k-avx512.asm
            crypto/bn/rsaz-3k-avx512.asm
            crypto/bn/rsaz-2k-avx512.asm
            crypto/modes/ghash-x86_64.asm
            crypto/modes/aesni-gcm-x86_64.asm
            crypto/modes/aes-gcm-avx512.asm
        )
    elseif(PROJECT_PLATFORM STREQUAL "x86")
        set(_nasm_args win32)
        set(_asm_file_list
            crypto/aes/libcrypto-lib-vpaes-x86.obj.asm
            crypto/aes/libcrypto-lib-aesni-x86.obj.asm
            crypto/aes/libcrypto-lib-aes-586.obj.asm
            crypto/bf/libcrypto-lib-bf-586.obj.asm
            crypto/bn/libcrypto-lib-x86-mont.obj.asm
            crypto/bn/libcrypto-lib-x86-gf2m.obj.asm
            crypto/camellia/libcrypto-lib-cmll-x86.obj.asm
            crypto/cast/libcrypto-lib-cast-586.obj.asm
            crypto/des/libcrypto-lib-des-586.obj.asm
            crypto/des/libcrypto-lib-crypt586.obj.asm
            crypto/ec/libcrypto-lib-ecp_nistz256-x86.obj.asm
            crypto/md5/libcrypto-lib-md5-586.obj.asm
            crypto/modes/libcrypto-lib-ghash-x86.obj.asm
            crypto/poly1305/libcrypto-lib-poly1305-x86.obj.asm
            crypto/rc4/libcrypto-lib-rc4-586.obj.asm
            crypto/ripemd/libcrypto-lib-rmd-586.obj.asm
            crypto/sha/libcrypto-lib-sha512-586.obj.asm
            crypto/sha/libcrypto-lib-sha256-586.obj.asm
            crypto/sha/libcrypto-lib-sha1-586.obj.asm
            crypto/whrlpool/libcrypto-lib-wp-mmx.obj.asm
        )
    elseif(PROJECT_PLATFORM STREQUAL "ARM64")
        # ARM64 has no ASM files in this configuration
        set(_nasm_args "")
        set(_asm_file_list )
    endif()

    # Compile ASM files
    if(MSVC AND NOT PROJECT_PLATFORM STREQUAL "ARM64" AND _asm_file_list)
        set(NASM_EXECUTABLE ${CMAKE_SOURCE_DIR}/buildtools/tools/nasm.exe)

        foreach(_asm_file ${_asm_file_list})
            get_filename_component(_asm_file_fn ${_asm_file} NAME)
            # message("_asm_file: ${_asm_file}")
            set(_asm_object ${CMAKE_CURRENT_BINARY_DIR}/${_asm_file_fn}.obj)
            add_custom_command(
                OUTPUT ${_asm_object}
                COMMAND ${NASM_EXECUTABLE}
                ARGS -f ${_nasm_args} -o ${_asm_object} ${CMAKE_CURRENT_SOURCE_DIR}/${_asm_file}
                DEPENDS ${_asm_file}
            )
            list(APPEND _asm_files ${_asm_object})
        endforeach()
    endif()

    set(${RESULT_VAR} ${_asm_files} PARENT_SCOPE)
endfunction()
