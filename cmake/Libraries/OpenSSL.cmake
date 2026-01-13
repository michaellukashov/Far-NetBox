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
    -DOPENSSL_NO_DEPRECATED_1_1_0
    -DOPENSSL_NO_DEPRECATED_3_0
    -DOPENSSL_NO_PINSHARED
    -DOPENSSL_API_COMPAT=11000
)

if(MSVC)
    set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
        /Zi /Zl
        -wd4090
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
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/openssl-3
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/openssl-3/include
        # ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/openssl-3/include/internal
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/openssl-3/include/crypto
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/openssl-3/crypto/modes
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/openssl-3/crypto/ec/curve448
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/openssl-3/libs/openssl-3
    )
endfunction()
