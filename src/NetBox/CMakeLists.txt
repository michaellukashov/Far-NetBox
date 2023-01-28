message(STATUS "CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.15)

set(OPT_USE_UNITY_BUILD OFF CACHE STRING "Default use unity build")

project(NetBox)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type to Debug")
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")
message("CMAKE_CL_64: ${CMAKE_CL_64}")

if(NOT DEFINED FAR_VERSION OR FAR_VERSION STREQUAL "")
  set(FAR_VERSION Far3)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(x64 1)
  set(PROJECT_PLATFORM "x64")
else()
  set(x64 0)
  set(PROJECT_PLATFORM "x86")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(BUILD_DEBUG 1)
else()
  set(BUILD_DEBUG 0)
endif()

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(NETBOX_DIR ${CMAKE_CURRENT_SOURCE_DIR})
string(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})

if(NOT DEFINED BUILD_DIR OR BUILD_DIR STREQUAL "")
string(REPLACE "/" "\\" BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(MSVC)
  set(OPENSSL_LIB_DIR "${PROJECT_PLATFORM}-${CMAKE_BUILD_TYPE}")
endif(MSVC)

if(NOT DEFINED OPENSSL_LIBRARYDIR)
  set(OPENSSL_LIBRARYDIR ${PROJECT_ROOT}/libs/openssl-1.1.1/${OPENSSL_LIB_DIR})
endif()

message("OPENSSL_LIBRARYDIR: ${OPENSSL_LIBRARYDIR}")

#-------------------------------------------------------------------------------

include(CMakeParseArguments)
include(${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
#include(CMakeDetermineCXXCompiler)
message("System: ${CMAKE_SYSTEM_NAME}, PROJECT_PLATFORM: ${PROJECT_PLATFORM}, BUILD_DIR: ${BUILD_DIR}, CMAKE_COMPILER_IS_MINGW = ${CMAKE_COMPILER_IS_MINGW}")
message("MSVC_VERSION: ${MSVC_VERSION}")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

#-------------------------------------------------------------------------------

set(LIBNEON_DEFS -DNE_LFS -DNOCRYPT -DNE_HAVE_SSL -DHAVE_OPENSSL -DNE_HAVE_DAV -DNE_HAVE_ZLIB -DZLIB_COMPAT -DHAVE_EXPAT -DHAVE_EXPAT_H) #" -DUSE_GETADDRINFO")
set(LIBEXPAT_DEFS -DCOMPILED_FROM_DSP -DXML_STATIC)
set(NETBOX_DEFS -DNOMINMAX -DMPEXT -DWINSCP -DFARPLUGIN)
set(NETBOX_DEFS ${NETBOX_DEFS} -DFASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX)
set(NETBOX_DEFS ${NETBOX_DEFS} -DUSE_DLMALLOC -DUSE_DL_PREFIX -DNO_MALLINFO -DUSE_LOCKS=1)
set(NETBOX_DEFS ${NETBOX_DEFS} -DHIDE_TODO)
set(NETBOX_DEFS ${NETBOX_DEFS} -DNEED_LIBS3)
set(NETBOX_DEFS ${NETBOX_DEFS} -DNO_GZIP -DSTRICT -DNOCRYPT -DOPENSSL_NO_LOCKING)
set(NETBOX_DEFS ${NETBOX_DEFS} -D_WIN32_WINNT=0x0501 -D_LIB -D_WINDOWS -DWIN32 -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
set(NETBOX_DEFS ${NETBOX_DEFS} -DCOM_NO_WINDOWS_H)
#-D_WINDLL -D_USRDLL
if (MSVC_VERSION GREATER 1899)
set(NETBOX_DEFS ${NETBOX_DEFS} /Zc:threadSafeInit- /bigobj)
endif()

set(NETBOX_C_FLAGS )

if(x64)
set(NETBOX_PLATFORM_FLAGS ${NETBOX_PLATFORM_FLAGS} -DWIN64 -D_WIN64 -D_AMD64_)
else()
if (MSVC_VERSION GREATER 1699)
set(NETBOX_PLATFORM_FLAGS ${NETBOX_PLATFORM_FLAGS} /arch:IA32)
endif()
endif()

if(MSVC)
set(NETBOX_C_FLAGS ${NETBOX_C_FLAGS} /TC)
elseif(CMAKE_COMPILER_IS_MINGW)
set(NETBOX_C_FLAGS ${NETBOX_C_FLAGS} -x c --std=c99)
endif()

set(NETBOX_FLAGS_RELEASE -U_DEBUG -DNDEBUG)
set(NETBOX_FLAGS_DEBUG -D_DEBUG -DDEBUG)

set(NETBOX_UNICODE_FLAGS -D_UNICODE -DUNICODE)

if(MSVC)

set(NETBOX_C_WARNING_FLAGS "")
set(NETBOX_CXX_WARNING_FLAGS "")
set(CMAKE_CXX_STANDARD 17)
#set(NETBOX_CXX_FLAGS /std:c++17)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)

elseif(CMAKE_COMPILER_IS_MINGW)

set(NETBOX_C_WARNING_FLAGS -Wno-unknown-pragmas)
set(NETBOX_CXX_WARNING_FLAGS ${NETBOX_C_WARNING_FLAGS} -Wno-attributes -Wno-write-strings -Wno-old-style-cast -Wno-effc++ -Wno-unknown-pragmas -Wno-unused-value)
set(NETBOX_CXX_FLAGS ${NETBOX_CXX_FLAGS} -Woverloaded-virtual -Wnon-virtual-dtor)
set(NETBOX_C_FLAGS ${NETBOX_C_FLAGS} -static -s -Wall -Wextra -pedantic -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code)
#-O2
#-Weffc++
set(NETBOX_FLAGS_RELEASE ${NETBOX_FLAGS_RELEASE})
set(NETBOX_FLAGS_DEBUG ${NETBOX_FLAGS_DEBUG})

endif()

set(NETBOX_C_FLAGS ${NETBOX_DEFS} ${NETBOX_C_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_C_WARNING_FLAGS})
set(NETBOX_CXX_FLAGS ${NETBOX_DEFS} ${NETBOX_CXX_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_CXX_WARNING_FLAGS})

function(nb_replace_compiler_option var old new)
  # Replaces a compiler option or switch `old' in `var' by `new'.
  # If `old' is not in `var', appends `new' to `var'.
  # Example: nb_replace_compiler_option(CMAKE_CXX_FLAGS_RELEASE "-O3" "-O2")
  # If the option already is on the variable, don't add it:
  if( "${${var}}" MATCHES "(^| )${new}($| )" )
    set(n "")
  else()
    set(n "${new}")
  endif()
  if( "${${var}}" MATCHES "(^| )${old}($| )" )
    string( REGEX REPLACE "(^| )${old}($| )" " ${n} " ${var} "${${var}}" )
  else()
    set( ${var} "${${var}} ${n}" )
  endif()
  set( ${var} "${${var}}" PARENT_SCOPE )
endfunction(nb_replace_compiler_option)

if(MSVC)

set(NETBOX_DLL_LINK_FLAGS
  /NODEFAULTLIB:MSVCURT.LIB
  /NODEFAULTLIB:MSVCURTD.LIB
  /NODEFAULTLIB:MSVCPRT.LIB
  /NODEFAULTLIB:MSVCPRTD.LIB
  /NODEFAULTLIB:LIBC.LIB
  /NODEFAULTLIB:LIBCMT.LIB
  /NODEFAULTLIB:LIBCMTD.LIB
  /NODEFAULTLIB:MFC100U.LIB
  /NODEFAULTLIB:MFC100UD.LIB
  /NODEFAULTLIB:ATL.LIB
  /NODEFAULTLIB:LIBCRT.LIB
  /NODEFAULTLIB:LIBCRTD.LIB
  /NODEFAULTLIB:MSVCRT.LIB
  /NODEFAULTLIB:MSVCRTD.LIB
  /NODEFAULTLIB:MFCS100U.LIB
  /NODEFAULTLIB:MFCS100UD.LIB
  /NODEFAULTLIB:LIBCPMT.LIB
  /NODEFAULTLIB:LIBCPMTD.LIB
  /NODEFAULTLIB:LIBCRYPTO.LIB
  /NODEFAULTLIB:LIBSSL.LIB
  /NODEFAULTLIB:PUTTY.LIB
  /NODEFAULTLIB:KERNEL32.LIB
  /NODEFAULTLIB:USER32.LIB
  /NODEFAULTLIB:VERSION.LIB
  /DELAYLOAD:version.dll
  /DELAYLOAD:ws2_32.dll
  /DELAYLOAD:oleaut32.dll
  /DELAYLOAD:shell32.dll
  /DELAYLOAD:shlwapi.dll
  /DELAYLOAD:crypt32.dll
  /DELAYLOAD:secur32.dll
  /MANIFEST:NO
  /TLBID:1 /DYNAMICBASE /SUBSYSTEM:WINDOWS
  /TLBID:1 /DYNAMICBASE /SUBSYSTEM:WINDOWS /NOLOGO
  /MAP
  /INCREMENTAL:NO
  /ignore:4099
  /ignore:4197
  /ignore:4199
)
#/DELAYLOAD:gdi32.dll

set(NETBOX_DLL_LINK_FLAGS_RELEASE /OPT:REF)
set(NETBOX_DLL_LINK_FLAGS_DEBUG /DEBUG)

elseif(CMAKE_COMPILER_IS_MINGW)
set(NETBOX_DLL_LINK_FLAGS )
set(NETBOX_DLL_LINK_FLAGS_RELEASE ${NETBOX_DLL_LINK_FLAGS})
set(NETBOX_DLL_LINK_FLAGS_DEBUG ${NETBOX_DLL_LINK_FLAGS})
endif()

if(MSVC)

#find_program(NASM_EXECUTABLE nasm)
# message("nasm: ${NASM_EXECUTABLE}")
set(NASM_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/../../buildtools/tools/nasm.exe)

macro(compile_asm_files)
  cmake_parse_arguments(ARG "" "RESULT" "ASM_FILES" ${ARGN})
  if(NOT ARG_ASM_FILES)
      set(ARG_ASM_FILES " ")
  endif()
#  message("ARG_ASM_FILES: ${ARG_ASM_FILES}")
  if(x64)
    set(_nasm_args win64)
  else()
    set(_nasm_args win32)
  endif()
  set(${ARG_RESULT} )
  foreach(_asm_file ${ARG_ASM_FILES})
#    message("asm_file: ${_asm_file}")
    set(_asm_source ${CMAKE_CURRENT_SOURCE_DIR}/${_asm_file})
    get_filename_component(_asm_source_fn ${_asm_source} NAME)
#    message("_asm_source_fn: ${_asm_source_fn}")
    # file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${_asm_source_fn}" _asm_file_name)
    # message("_asm_file_name: ${_asm_file_name}")
    set(_asm_object ${CMAKE_CURRENT_BINARY_DIR}/${_asm_source_fn}.obj)
    add_custom_command(
      OUTPUT ${_asm_object}
      COMMAND ${NASM_EXECUTABLE}
      ARGS -f ${_nasm_args} -o ${_asm_object} ${_asm_source}
      DEPENDS ${_asm_source}
    )
    set(${ARG_RESULT} ${${ARG_RESULT}} ${_asm_object})
  endforeach(_asm_file)
endmacro()

#[[set(NETBOX_SOURCES ${NETBOX_SOURCES}
  vc_crt_fix_impl.cpp
  ${ASM_OBJECTS}
)]]

endif(MSVC)

#-------------------------------------------------------------------------------
#openssl

if(x64)
set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
  -DOPENSSL_SYSNAME_WIN32 -DOPENSSL_SYSNAME_WIN64
  -DMK1MF_PLATFORM_VC_WIN64A
)
else()
set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
  -DOPENSSL_SYSNAME_WIN32
  -DMK1MF_PLATFORM_VC_WIN32
)
endif()
set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
  -DMK1MF_BUILD -DWIN32_LEAN_AND_MEAN
  -DVC_EXTRALEAN -DSECURITY_WIN32 -DOPENSSL_NO_CAPIENG -DNO_CHMOD -DOPENSSL_NO_DGRAM -DOPENSSL_NO_RIJNDAEL
  -DDSO_WIN32
)
set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
  -DOPENSSL_THREADS -DOPENSSL_NO_DYNAMIC_ENGINE
  -DOPENSSL_PIC -DOPENSSL_NO_ERR
)
set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
  -DNB_COMPILE_OPENSSL
)
set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
  -DWIN32_LEAN_AND_MEAN
  -DL_ENDIAN -DDSO_WIN32 -D_stricmp=stricmp -D_strnicmp=strnicmp
  -DBN_ASM -DMD5_ASM -DSHA1_ASM -DOPENSSL_NO_CAMELLIA -DOPENSSL_NO_SEED -DOPENSSL_NO_RC5 -DOPENSSL_NO_MDC2
  -DOPENSSL_NO_CAPIENG -DOPENSSL_NO_KRB5 -DOPENSSL_NO_ENGINE -DOPENSSL_NO_DYNAMIC_ENGINE
  -DOPENSSL_DISABLE_OLD_DES_SUPPORT -DNO_CHMOD -DOPENSSL_NO_DGRAM -DDOPENSSL_NO_EC_NISTP_64_GCC_128
  -DOPENSSL_NO_WHIRLPOOL -DWINSCP -DMK1MF_BUILD -DMK1MF_PLATFORM_BC_NT
  -DOPENSSL_NO_CRYPTO_MDEBUG_BACKTRACE
)
#-O2
if(MSVC)
set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
  /Zi /Zl
  -wd4090
)
endif()

add_library(ssleay32 STATIC
  ../../libs/openssl-1.1.1/ssl/s3_lib.c
  ../../libs/openssl-1.1.1/ssl/s3_enc.c
  ../../libs/openssl-1.1.1/ssl/s3_cbc.c
  ../../libs/openssl-1.1.1/ssl/s3_msg.c
  ../../libs/openssl-1.1.1/ssl/t1_lib.c
  ../../libs/openssl-1.1.1/ssl/t1_enc.c
  ../../libs/openssl-1.1.1/ssl/tls_srp.c
  ../../libs/openssl-1.1.1/ssl/tls13_enc.c
  ../../libs/openssl-1.1.1/ssl/d1_srtp.c
  ../../libs/openssl-1.1.1/ssl/d1_lib.c
  ../../libs/openssl-1.1.1/ssl/d1_msg.c
  ../../libs/openssl-1.1.1/ssl/ssl_lib.c
  ../../libs/openssl-1.1.1/ssl/ssl_cert.c
  ../../libs/openssl-1.1.1/ssl/ssl_sess.c
  ../../libs/openssl-1.1.1/ssl/ssl_ciph.c
  ../../libs/openssl-1.1.1/ssl/ssl_stat.c
  ../../libs/openssl-1.1.1/ssl/ssl_rsa.c
  ../../libs/openssl-1.1.1/ssl/ssl_asn1.c
  ../../libs/openssl-1.1.1/ssl/ssl_txt.c
  ../../libs/openssl-1.1.1/ssl/ssl_err.c
  ../../libs/openssl-1.1.1/ssl/ssl_init.c
  ../../libs/openssl-1.1.1/ssl/ssl_mcnf.c
  ../../libs/openssl-1.1.1/ssl/bio_ssl.c
  ../../libs/openssl-1.1.1/ssl/pqueue.c
  ../../libs/openssl-1.1.1/ssl/methods.c
  ../../libs/openssl-1.1.1/ssl/packet.c
  ../../libs/openssl-1.1.1/ssl/ssl_conf.c
  ../../libs/openssl-1.1.1/ssl/statem/statem.c
  ../../libs/openssl-1.1.1/ssl/statem/statem_clnt.c
  ../../libs/openssl-1.1.1/ssl/statem/statem_dtls.c
  ../../libs/openssl-1.1.1/ssl/statem/statem_lib.c
  ../../libs/openssl-1.1.1/ssl/statem/statem_srvr.c
  ../../libs/openssl-1.1.1/ssl/statem/extensions.c
  ../../libs/openssl-1.1.1/ssl/statem/extensions_clnt.c
  ../../libs/openssl-1.1.1/ssl/statem/extensions_cust.c
  ../../libs/openssl-1.1.1/ssl/statem/extensions_srvr.c
  ../../libs/openssl-1.1.1/ssl/record/rec_layer_s3.c
  ../../libs/openssl-1.1.1/ssl/record/rec_layer_d1.c
  ../../libs/openssl-1.1.1/ssl/record/ssl3_record.c
  ../../libs/openssl-1.1.1/ssl/record/ssl3_record_tls13.c
  ../../libs/openssl-1.1.1/ssl/record/ssl3_buffer.c
  ../../libs/openssl-1.1.1/ssl/record/dtls1_bitmap.c
)

target_compile_options(ssleay32 PRIVATE
  ${OPENSSL_COMPILE_FLAGS}
  ${NETBOX_PLATFORM_FLAGS}
)

target_include_directories(ssleay32 SYSTEM BEFORE PRIVATE
  ../../libs/openssl-1.1.1/include
  ../../libs/openssl-1.1.1/include/crypto
  ../../libs/openssl-1.1.1/crypto/modes
  ../../libs/openssl-1.1.1/crypto/ec/curve448/arch_32
  ../../libs/openssl-1.1.1/crypto/ec/curve448
  ../../libs/openssl-1.1.1
)

set(libeay32_asm_files
  ../../libs/openssl-1.1.1/crypto/des/asm/d_win32.asm
  ../../libs/openssl-1.1.1/crypto/des/asm/y_win32.asm
)

set(libeay32_obj_files )
if(x64)
compile_asm_files(RESULT libeay32_obj_files ASM_FILES
  ../../libs/openssl-1.1.1/crypto/sha/asm/sha1-x86_64.asm
  ../../libs/openssl-1.1.1/crypto/md5/asm/md5-x86_64.asm
)
else()
compile_asm_files(RESULT libeay32_obj_files ASM_FILES
  ../../libs/openssl-1.1.1/crypto/sha/asm/sha1-586.asm
  ../../libs/openssl-1.1.1/crypto/md5/asm/md5-586.asm
)
endif()
#message("libeay32_obj_files: ${libeay32_obj_files}")

add_library(libeay32 STATIC
  ../../libs/openssl-1.1.1/crypto/cryptlib.c
  ../../libs/openssl-1.1.1/crypto/ctype.c
  ../../libs/openssl-1.1.1/crypto/init.c
  ../../libs/openssl-1.1.1/crypto/mem.c
  ../../libs/openssl-1.1.1/crypto/mem_clr.c
  ../../libs/openssl-1.1.1/crypto/mem_dbg.c
  ../../libs/openssl-1.1.1/crypto/ex_data.c
  ../../libs/openssl-1.1.1/crypto/cpt_err.c
  ../../libs/openssl-1.1.1/crypto/ebcdic.c
  ../../libs/openssl-1.1.1/crypto/uid.c
  ../../libs/openssl-1.1.1/crypto/o_time.c
  ../../libs/openssl-1.1.1/crypto/o_str.c
  ../../libs/openssl-1.1.1/crypto/o_dir.c
  ../../libs/openssl-1.1.1/crypto/o_fopen.c
  ../../libs/openssl-1.1.1/crypto/getenv.c
  ../../libs/openssl-1.1.1/crypto/mem_sec.c
  ../../libs/openssl-1.1.1/crypto/threads_win.c
  ../../libs/openssl-1.1.1/crypto/md4/md4_dgst.c
  ../../libs/openssl-1.1.1/crypto/md4/md4_one.c
  ../../libs/openssl-1.1.1/crypto/md5/md5_dgst.c
  ../../libs/openssl-1.1.1/crypto/md5/md5_one.c
  ../../libs/openssl-1.1.1/crypto/modes/cbc128.c
  ../../libs/openssl-1.1.1/crypto/modes/cfb128.c
  ../../libs/openssl-1.1.1/crypto/modes/ofb128.c
  ../../libs/openssl-1.1.1/crypto/modes/ccm128.c
  ../../libs/openssl-1.1.1/crypto/modes/gcm128.c
  ../../libs/openssl-1.1.1/crypto/modes/xts128.c
  ../../libs/openssl-1.1.1/crypto/modes/wrap128.c
  ../../libs/openssl-1.1.1/crypto/modes/ctr128.c
  ../../libs/openssl-1.1.1/crypto/modes/ocb128.c
  ../../libs/openssl-1.1.1/crypto/sha/sha1dgst.c
  ../../libs/openssl-1.1.1/crypto/sha/sha1_one.c
  ../../libs/openssl-1.1.1/crypto/sha/sha256.c
  ../../libs/openssl-1.1.1/crypto/sha/sha512.c
  ../../libs/openssl-1.1.1/crypto/sha/keccak1600.c
  ../../libs/openssl-1.1.1/crypto/hmac/hmac.c
  ../../libs/openssl-1.1.1/crypto/ripemd/rmd_dgst.c
  ../../libs/openssl-1.1.1/crypto/hmac/hm_ameth.c
  ../../libs/openssl-1.1.1/crypto/hmac/hm_pmeth.c
  ../../libs/openssl-1.1.1/crypto/ripemd/rmd_one.c
  ../../libs/openssl-1.1.1/crypto/des/set_key.c
  ../../libs/openssl-1.1.1/crypto/des/ecb_enc.c
  ../../libs/openssl-1.1.1/crypto/des/cbc_enc.c
  ../../libs/openssl-1.1.1/crypto/des/ecb3_enc.c
  ../../libs/openssl-1.1.1/crypto/des/cfb64enc.c
  ../../libs/openssl-1.1.1/crypto/des/cfb64ede.c
  ../../libs/openssl-1.1.1/crypto/des/cfb_enc.c
  ../../libs/openssl-1.1.1/crypto/des/ofb64ede.c
  ../../libs/openssl-1.1.1/crypto/des/ofb64enc.c
  ../../libs/openssl-1.1.1/crypto/des/ofb_enc.c
  ../../libs/openssl-1.1.1/crypto/des/str2key.c
  ../../libs/openssl-1.1.1/crypto/des/pcbc_enc.c
  ../../libs/openssl-1.1.1/crypto/des/qud_cksm.c
  ../../libs/openssl-1.1.1/crypto/des/rand_key.c
  ../../libs/openssl-1.1.1/crypto/des/fcrypt.c
  ../../libs/openssl-1.1.1/crypto/des/xcbc_enc.c
  ../../libs/openssl-1.1.1/crypto/des/cbc_cksm.c
  ../../libs/openssl-1.1.1/crypto/rc2/rc2_ecb.c
  ../../libs/openssl-1.1.1/crypto/rc2/rc2_skey.c
  ../../libs/openssl-1.1.1/crypto/rc2/rc2_cbc.c
  ../../libs/openssl-1.1.1/crypto/rc2/rc2cfb64.c
  ../../libs/openssl-1.1.1/crypto/rc2/rc2ofb64.c
  ../../libs/openssl-1.1.1/crypto/rc4/rc4_skey.c
  ../../libs/openssl-1.1.1/crypto/rc4/rc4_enc.c
  ../../libs/openssl-1.1.1/crypto/idea/i_cbc.c
  ../../libs/openssl-1.1.1/crypto/idea/i_cfb64.c
  ../../libs/openssl-1.1.1/crypto/idea/i_ofb64.c
  ../../libs/openssl-1.1.1/crypto/idea/i_ecb.c
  ../../libs/openssl-1.1.1/crypto/idea/i_skey.c
  ../../libs/openssl-1.1.1/crypto/bf/bf_skey.c
  ../../libs/openssl-1.1.1/crypto/bf/bf_ecb.c
  ../../libs/openssl-1.1.1/crypto/bf/bf_cfb64.c
  ../../libs/openssl-1.1.1/crypto/bf/bf_ofb64.c
  ../../libs/openssl-1.1.1/crypto/cast/c_skey.c
  ../../libs/openssl-1.1.1/crypto/cast/c_ecb.c
  ../../libs/openssl-1.1.1/crypto/cast/c_cfb64.c
  ../../libs/openssl-1.1.1/crypto/cast/c_ofb64.c
  ../../libs/openssl-1.1.1/crypto/aes/aes_misc.c
  ../../libs/openssl-1.1.1/crypto/aes/aes_ecb.c
  ../../libs/openssl-1.1.1/crypto/aes/aes_cfb.c
  ../../libs/openssl-1.1.1/crypto/aes/aes_ofb.c
  ../../libs/openssl-1.1.1/crypto/aes/aes_ige.c
  ../../libs/openssl-1.1.1/crypto/aes/aes_wrap.c
  ../../libs/openssl-1.1.1/crypto/aes/aes_core.c
  ../../libs/openssl-1.1.1/crypto/aes/aes_cbc.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_add.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_div.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_exp.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_lib.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_ctx.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_mul.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_mod.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_print.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_rand.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_shift.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_word.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_blind.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_kron.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_sqrt.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_gcd.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_prime.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_err.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_sqr.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_asm.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_recp.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_mont.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_mpi.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_exp2.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_gf2m.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_nist.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_depr.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_const.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_intern.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_dh.c
  ../../libs/openssl-1.1.1/crypto/bn/bn_srp.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_ossl.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_gen.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_lib.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_sign.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_saos.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_err.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_pk1.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_ssl.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_none.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_oaep.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_chk.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_pss.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_x931.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_asn1.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_depr.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_pmeth.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_crpt.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_ameth.c
  ../../libs/openssl-1.1.1/crypto/rsa/rsa_mp.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_gen.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_key.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_lib.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_asn1.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_vrf.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_sign.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_err.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_ossl.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_depr.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_pmeth.c
  ../../libs/openssl-1.1.1/crypto/dsa/dsa_ameth.c
  ../../libs/openssl-1.1.1/crypto/dso/dso_dl.c
  ../../libs/openssl-1.1.1/crypto/dso/dso_dlfcn.c
  ../../libs/openssl-1.1.1/crypto/dso/dso_err.c
  ../../libs/openssl-1.1.1/crypto/dso/dso_lib.c
  ../../libs/openssl-1.1.1/crypto/dso/dso_openssl.c
  ../../libs/openssl-1.1.1/crypto/dso/dso_win32.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_asn1.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_gen.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_key.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_lib.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_check.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_err.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_depr.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_pmeth.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_ameth.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_rfc5114.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_kdf.c
  ../../libs/openssl-1.1.1/crypto/dh/dh_rfc7919.c
  ../../libs/openssl-1.1.1/crypto/buffer/buffer.c
  ../../libs/openssl-1.1.1/crypto/buffer/buf_err.c
  ../../libs/openssl-1.1.1/crypto/bio/bio_lib.c
  ../../libs/openssl-1.1.1/crypto/bio/bio_cb.c
  ../../libs/openssl-1.1.1/crypto/bio/bio_err.c
  ../../libs/openssl-1.1.1/crypto/bio/bss_mem.c
  ../../libs/openssl-1.1.1/crypto/bio/bss_null.c
  ../../libs/openssl-1.1.1/crypto/bio/bss_fd.c
  ../../libs/openssl-1.1.1/crypto/bio/bss_file.c
  ../../libs/openssl-1.1.1/crypto/bio/bss_sock.c
  ../../libs/openssl-1.1.1/crypto/bio/bf_null.c
  ../../libs/openssl-1.1.1/crypto/bio/bf_buff.c
  ../../libs/openssl-1.1.1/crypto/bio/b_print.c
  ../../libs/openssl-1.1.1/crypto/bio/b_dump.c
  ../../libs/openssl-1.1.1/crypto/bio/b_sock.c
  ../../libs/openssl-1.1.1/crypto/bio/b_sock2.c
  ../../libs/openssl-1.1.1/crypto/bio/bss_acpt.c
  ../../libs/openssl-1.1.1/crypto/bio/bf_nbio.c
  ../../libs/openssl-1.1.1/crypto/bio/bss_log.c
  ../../libs/openssl-1.1.1/crypto/bio/bss_bio.c
  ../../libs/openssl-1.1.1/crypto/bio/bss_conn.c
  ../../libs/openssl-1.1.1/crypto/bio/b_addr.c
  ../../libs/openssl-1.1.1/crypto/bio/bio_meth.c
  ../../libs/openssl-1.1.1/crypto/stack/stack.c
  ../../libs/openssl-1.1.1/crypto/lhash/lhash.c
  ../../libs/openssl-1.1.1/crypto/lhash/lh_stats.c
  ../../libs/openssl-1.1.1/crypto/rand/rand_lib.c
  ../../libs/openssl-1.1.1/crypto/rand/rand_err.c
  ../../libs/openssl-1.1.1/crypto/rand/rand_egd.c
  ../../libs/openssl-1.1.1/crypto/rand/rand_win.c
  ../../libs/openssl-1.1.1/crypto/rand/drbg_lib.c
  ../../libs/openssl-1.1.1/crypto/rand/drbg_ctr.c
  ../../libs/openssl-1.1.1/crypto/err/err.c
  ../../libs/openssl-1.1.1/crypto/err/err_all.c
  ../../libs/openssl-1.1.1/crypto/err/err_prn.c
  ../../libs/openssl-1.1.1/crypto/objects/o_names.c
  ../../libs/openssl-1.1.1/crypto/objects/obj_dat.c
  ../../libs/openssl-1.1.1/crypto/objects/obj_lib.c
  ../../libs/openssl-1.1.1/crypto/objects/obj_err.c
  ../../libs/openssl-1.1.1/crypto/objects/obj_xref.c
  ../../libs/openssl-1.1.1/crypto/evp/encode.c
  ../../libs/openssl-1.1.1/crypto/evp/digest.c
  ../../libs/openssl-1.1.1/crypto/evp/evp_enc.c
  ../../libs/openssl-1.1.1/crypto/evp/evp_key.c
  ../../libs/openssl-1.1.1/crypto/evp/e_des.c
  ../../libs/openssl-1.1.1/crypto/evp/e_bf.c
  ../../libs/openssl-1.1.1/crypto/evp/e_idea.c
  ../../libs/openssl-1.1.1/crypto/evp/e_des3.c
  ../../libs/openssl-1.1.1/crypto/evp/e_rc4.c
  ../../libs/openssl-1.1.1/crypto/evp/e_aes.c
  ../../libs/openssl-1.1.1/crypto/evp/e_aes_cbc_hmac_sha1.c
  ../../libs/openssl-1.1.1/crypto/evp/e_aes_cbc_hmac_sha256.c
  ../../libs/openssl-1.1.1/crypto/evp/names.c
  ../../libs/openssl-1.1.1/crypto/evp/e_xcbc_d.c
  ../../libs/openssl-1.1.1/crypto/evp/e_rc2.c
  ../../libs/openssl-1.1.1/crypto/evp/e_cast.c
  ../../libs/openssl-1.1.1/crypto/evp/m_null.c
  ../../libs/openssl-1.1.1/crypto/evp/m_md4.c
  ../../libs/openssl-1.1.1/crypto/evp/m_md5.c
  ../../libs/openssl-1.1.1/crypto/evp/m_sha1.c
  ../../libs/openssl-1.1.1/crypto/evp/m_ripemd.c
  ../../libs/openssl-1.1.1/crypto/evp/p_open.c
  ../../libs/openssl-1.1.1/crypto/evp/p_seal.c
  ../../libs/openssl-1.1.1/crypto/evp/p_sign.c
  ../../libs/openssl-1.1.1/crypto/evp/p_verify.c
  ../../libs/openssl-1.1.1/crypto/evp/p_lib.c
  ../../libs/openssl-1.1.1/crypto/evp/p_enc.c
  ../../libs/openssl-1.1.1/crypto/evp/p_dec.c
  ../../libs/openssl-1.1.1/crypto/evp/bio_md.c
  ../../libs/openssl-1.1.1/crypto/evp/bio_b64.c
  ../../libs/openssl-1.1.1/crypto/evp/bio_enc.c
  ../../libs/openssl-1.1.1/crypto/evp/evp_err.c
  ../../libs/openssl-1.1.1/crypto/evp/e_null.c
  ../../libs/openssl-1.1.1/crypto/evp/c_allc.c
  ../../libs/openssl-1.1.1/crypto/evp/c_alld.c
  ../../libs/openssl-1.1.1/crypto/evp/evp_lib.c
  ../../libs/openssl-1.1.1/crypto/evp/bio_ok.c
  ../../libs/openssl-1.1.1/crypto/evp/evp_pkey.c
  ../../libs/openssl-1.1.1/crypto/evp/evp_pbe.c
  ../../libs/openssl-1.1.1/crypto/evp/p5_crpt.c
  ../../libs/openssl-1.1.1/crypto/evp/p5_crpt2.c
  ../../libs/openssl-1.1.1/crypto/evp/e_old.c
  ../../libs/openssl-1.1.1/crypto/evp/pmeth_lib.c
  ../../libs/openssl-1.1.1/crypto/evp/pmeth_fn.c
  ../../libs/openssl-1.1.1/crypto/evp/pmeth_gn.c
  ../../libs/openssl-1.1.1/crypto/evp/m_sigver.c
  ../../libs/openssl-1.1.1/crypto/evp/e_rc4_hmac_md5.c
  ../../libs/openssl-1.1.1/crypto/evp/m_sha3.c
  ../../libs/openssl-1.1.1/crypto/evp/pbe_scrypt.c
  ../../libs/openssl-1.1.1/crypto/evp/evp_cnf.c
  ../../libs/openssl-1.1.1/crypto/evp/e_aria.c
  ../../libs/openssl-1.1.1/crypto/evp/e_chacha20_poly1305.c
  ../../libs/openssl-1.1.1/crypto/evp/e_sm4.c
  ../../libs/openssl-1.1.1/crypto/evp/m_md5_sha1.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_object.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_bitstr.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_utctm.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_gentm.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_time.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_int.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_octet.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_print.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_type.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_dup.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_d2i_fp.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_i2d_fp.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_utf8.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_sign.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_digest.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_verify.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_mbstr.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_strex.c
  ../../libs/openssl-1.1.1/crypto/asn1/ameth_lib.c
  ../../libs/openssl-1.1.1/crypto/asn1/x_algor.c
  ../../libs/openssl-1.1.1/crypto/asn1/x_val.c
  ../../libs/openssl-1.1.1/crypto/asn1/x_sig.c
  ../../libs/openssl-1.1.1/crypto/asn1/x_bignum.c
  ../../libs/openssl-1.1.1/crypto/asn1/x_long.c
  ../../libs/openssl-1.1.1/crypto/asn1/x_info.c
  ../../libs/openssl-1.1.1/crypto/asn1/x_spki.c
  ../../libs/openssl-1.1.1/crypto/asn1/nsseq.c
  ../../libs/openssl-1.1.1/crypto/asn1/d2i_pu.c
  ../../libs/openssl-1.1.1/crypto/asn1/d2i_pr.c
  ../../libs/openssl-1.1.1/crypto/asn1/i2d_pu.c
  ../../libs/openssl-1.1.1/crypto/asn1/i2d_pr.c
  ../../libs/openssl-1.1.1/crypto/asn1/t_pkey.c
  ../../libs/openssl-1.1.1/crypto/asn1/t_spki.c
  ../../libs/openssl-1.1.1/crypto/asn1/t_bitst.c
  ../../libs/openssl-1.1.1/crypto/asn1/tasn_new.c
  ../../libs/openssl-1.1.1/crypto/asn1/tasn_fre.c
  ../../libs/openssl-1.1.1/crypto/asn1/tasn_enc.c
  ../../libs/openssl-1.1.1/crypto/asn1/tasn_dec.c
  ../../libs/openssl-1.1.1/crypto/asn1/tasn_utl.c
  ../../libs/openssl-1.1.1/crypto/asn1/tasn_typ.c
  ../../libs/openssl-1.1.1/crypto/asn1/tasn_prn.c
  ../../libs/openssl-1.1.1/crypto/asn1/f_int.c
  ../../libs/openssl-1.1.1/crypto/asn1/f_string.c
  ../../libs/openssl-1.1.1/crypto/asn1/n_pkey.c
  ../../libs/openssl-1.1.1/crypto/asn1/x_pkey.c
  ../../libs/openssl-1.1.1/crypto/asn1/asn_mime.c
  ../../libs/openssl-1.1.1/crypto/asn1/asn1_gen.c
  ../../libs/openssl-1.1.1/crypto/asn1/asn1_par.c
  ../../libs/openssl-1.1.1/crypto/asn1/asn1_lib.c
  ../../libs/openssl-1.1.1/crypto/asn1/asn1_err.c
  ../../libs/openssl-1.1.1/crypto/asn1/bio_asn1.c
  ../../libs/openssl-1.1.1/crypto/asn1/bio_ndef.c
  ../../libs/openssl-1.1.1/crypto/asn1/a_strnid.c
  ../../libs/openssl-1.1.1/crypto/asn1/evp_asn1.c
  ../../libs/openssl-1.1.1/crypto/asn1/asn_pack.c
  ../../libs/openssl-1.1.1/crypto/asn1/p5_pbe.c
  ../../libs/openssl-1.1.1/crypto/asn1/p5_pbev2.c
  ../../libs/openssl-1.1.1/crypto/asn1/p8_pkey.c
  ../../libs/openssl-1.1.1/crypto/asn1/asn_moid.c
  ../../libs/openssl-1.1.1/crypto/asn1/p5_scrypt.c
  ../../libs/openssl-1.1.1/crypto/asn1/asn_mstbl.c
  ../../libs/openssl-1.1.1/crypto/asn1/x_int64.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_sign.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_info.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_lib.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_all.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_err.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_x509.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_xaux.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_oth.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_pk8.c
  ../../libs/openssl-1.1.1/crypto/pem/pem_pkey.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_def.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_d2.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_r2x.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_cmp.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_obj.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_req.c
  ../../libs/openssl-1.1.1/crypto/x509/x509spki.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_vfy.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_set.c
  ../../libs/openssl-1.1.1/crypto/x509/x509cset.c
  ../../libs/openssl-1.1.1/crypto/x509/x509rset.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_err.c
  ../../libs/openssl-1.1.1/crypto/x509/x509name.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_v3.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_ext.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_att.c
  ../../libs/openssl-1.1.1/crypto/x509/x509type.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_lu.c
  ../../libs/openssl-1.1.1/crypto/x509/x_all.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_txt.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_trs.c
  ../../libs/openssl-1.1.1/crypto/x509/by_file.c
  ../../libs/openssl-1.1.1/crypto/x509/by_dir.c
  ../../libs/openssl-1.1.1/crypto/x509/x509_vpm.c
  ../../libs/openssl-1.1.1/crypto/x509/x_pubkey.c
  ../../libs/openssl-1.1.1/crypto/x509/x_req.c
  ../../libs/openssl-1.1.1/crypto/x509/x_attrib.c
  ../../libs/openssl-1.1.1/crypto/x509/x_name.c
  ../../libs/openssl-1.1.1/crypto/x509/x_x509.c
  ../../libs/openssl-1.1.1/crypto/x509/x_x509a.c
  ../../libs/openssl-1.1.1/crypto/x509/x_crl.c
  ../../libs/openssl-1.1.1/crypto/x509/t_req.c
  ../../libs/openssl-1.1.1/crypto/x509/t_x509.c
  ../../libs/openssl-1.1.1/crypto/x509/t_crl.c
  ../../libs/openssl-1.1.1/crypto/x509/x_exten.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_bcons.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_bitst.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_conf.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_extku.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_ia5.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_lib.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_prn.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_utl.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3err.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_genn.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_alt.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_skey.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_akey.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_pku.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_int.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_enum.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_sxnet.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_cpols.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_crld.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_purp.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_info.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_akeya.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_pmaps.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_pcons.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_ncons.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_pcia.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_pci.c
  ../../libs/openssl-1.1.1/crypto/x509v3/pcy_cache.c
  ../../libs/openssl-1.1.1/crypto/x509v3/pcy_node.c
  ../../libs/openssl-1.1.1/crypto/x509v3/pcy_data.c
  ../../libs/openssl-1.1.1/crypto/x509v3/pcy_map.c
  ../../libs/openssl-1.1.1/crypto/x509v3/pcy_tree.c
  ../../libs/openssl-1.1.1/crypto/x509v3/pcy_lib.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_asid.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_addr.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_tlsf.c
  ../../libs/openssl-1.1.1/crypto/x509v3/v3_admis.c
  ../../libs/openssl-1.1.1/crypto/conf/conf_err.c
  ../../libs/openssl-1.1.1/crypto/conf/conf_lib.c
  ../../libs/openssl-1.1.1/crypto/conf/conf_api.c
  ../../libs/openssl-1.1.1/crypto/conf/conf_def.c
  ../../libs/openssl-1.1.1/crypto/conf/conf_mod.c
  ../../libs/openssl-1.1.1/crypto/conf/conf_mall.c
  ../../libs/openssl-1.1.1/crypto/conf/conf_sap.c
  ../../libs/openssl-1.1.1/crypto/conf/conf_ssl.c
  ../../libs/openssl-1.1.1/crypto/pkcs7/pk7_asn1.c
  ../../libs/openssl-1.1.1/crypto/pkcs7/pk7_lib.c
  ../../libs/openssl-1.1.1/crypto/pkcs7/pkcs7err.c
  ../../libs/openssl-1.1.1/crypto/pkcs7/pk7_doit.c
  ../../libs/openssl-1.1.1/crypto/pkcs7/pk7_smime.c
  ../../libs/openssl-1.1.1/crypto/pkcs7/pk7_attr.c
  ../../libs/openssl-1.1.1/crypto/pkcs7/pk7_mime.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_add.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_asn.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_attr.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_crpt.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_crt.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_decr.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_init.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_key.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_kiss.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_mutl.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_utl.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_npas.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/pk12err.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_p8d.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_p8e.c
  ../../libs/openssl-1.1.1/crypto/pkcs12/p12_sbag.c
  ../../libs/openssl-1.1.1/crypto/comp/comp_lib.c
  ../../libs/openssl-1.1.1/crypto/comp/comp_err.c
  ../../libs/openssl-1.1.1/crypto/comp/c_zlib.c
  ../../libs/openssl-1.1.1/crypto/cmac/cm_ameth.c
  ../../libs/openssl-1.1.1/crypto/cmac/cm_pmeth.c
  ../../libs/openssl-1.1.1/crypto/cmac/cmac.c
  ../../libs/openssl-1.1.1/crypto/ocsp/ocsp_asn.c
  ../../libs/openssl-1.1.1/crypto/ocsp/ocsp_ext.c
  ../../libs/openssl-1.1.1/crypto/ocsp/ocsp_ht.c
  ../../libs/openssl-1.1.1/crypto/ocsp/ocsp_lib.c
  ../../libs/openssl-1.1.1/crypto/ocsp/ocsp_cl.c
  ../../libs/openssl-1.1.1/crypto/ocsp/ocsp_srv.c
  ../../libs/openssl-1.1.1/crypto/ocsp/ocsp_prn.c
  ../../libs/openssl-1.1.1/crypto/ocsp/ocsp_vfy.c
  ../../libs/openssl-1.1.1/crypto/ocsp/ocsp_err.c
  ../../libs/openssl-1.1.1/crypto/ocsp/v3_ocsp.c
  ../../libs/openssl-1.1.1/crypto/ui/ui_err.c
  ../../libs/openssl-1.1.1/crypto/ui/ui_lib.c
  ../../libs/openssl-1.1.1/crypto/ui/ui_openssl.c
  ../../libs/openssl-1.1.1/crypto/ui/ui_util.c
  ../../libs/openssl-1.1.1/crypto/ui/ui_null.c
  ../../libs/openssl-1.1.1/crypto/o_init.c
  ../../libs/openssl-1.1.1/crypto/srp/srp_lib.c
  ../../libs/openssl-1.1.1/crypto/srp/srp_vfy.c
  ../../libs/openssl-1.1.1/crypto/txt_db/txt_db.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_ameth.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_asn1.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_curve.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_cvt.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_err.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_check.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_key.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_lib.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_mult.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_oct.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_pmeth.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_print.c
  ../../libs/openssl-1.1.1/crypto/ec/ec2_oct.c
  ../../libs/openssl-1.1.1/crypto/ec/ec2_smpl.c
  ../../libs/openssl-1.1.1/crypto/ec/eck_prn.c
  ../../libs/openssl-1.1.1/crypto/ec/ecp_mont.c
  ../../libs/openssl-1.1.1/crypto/ec/ecp_nist.c
  ../../libs/openssl-1.1.1/crypto/ec/ecp_nistp224.c
  ../../libs/openssl-1.1.1/crypto/ec/ecp_nistp256.c
  ../../libs/openssl-1.1.1/crypto/ec/ecp_nistp521.c
  ../../libs/openssl-1.1.1/crypto/ec/ecp_nistputil.c
  ../../libs/openssl-1.1.1/crypto/ec/ecp_oct.c
  ../../libs/openssl-1.1.1/crypto/ec/ecp_smpl.c
  ../../libs/openssl-1.1.1/crypto/ec/ecdh_ossl.c
  ../../libs/openssl-1.1.1/crypto/ec/ec_kmeth.c
  ../../libs/openssl-1.1.1/crypto/ec/ecx_meth.c
  ../../libs/openssl-1.1.1/crypto/ec/ecdsa_ossl.c
  ../../libs/openssl-1.1.1/crypto/ec/curve25519.c
  ../../libs/openssl-1.1.1/crypto/ec/ecdsa_sign.c
  ../../libs/openssl-1.1.1/crypto/ec/curve448/eddsa.c
  ../../libs/openssl-1.1.1/crypto/ec/curve448/curve448.c
  ../../libs/openssl-1.1.1/crypto/ec/curve448/scalar.c
  ../../libs/openssl-1.1.1/crypto/ec/curve448/curve448_tables.c
  ../../libs/openssl-1.1.1/crypto/ec/curve448/f_generic.c
  ../../libs/openssl-1.1.1/crypto/ec/curve448/arch_32/f_impl.c
  ../../libs/openssl-1.1.1/crypto/ec/ecdsa_vrf.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_sd.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_err.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_lib.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_env.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_asn1.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_kari.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_dd.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_enc.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_io.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_pwri.c
  ../../libs/openssl-1.1.1/crypto/cms/cms_att.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_oct.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_prn.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_err.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_x509v3.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_sct.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_log.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_policy.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_sct_ctx.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_b64.c
  ../../libs/openssl-1.1.1/crypto/ct/ct_vfy.c
  ../../libs/openssl-1.1.1/crypto/async/async.c
  ../../libs/openssl-1.1.1/crypto/async/async_err.c
  ../../libs/openssl-1.1.1/crypto/async/async_wait.c
  ../../libs/openssl-1.1.1/crypto/async/arch/async_win.c
  ../../libs/openssl-1.1.1/crypto/kdf/hkdf.c
  ../../libs/openssl-1.1.1/crypto/kdf/tls1_prf.c
  ../../libs/openssl-1.1.1/crypto/kdf/kdf_err.c
  ../../libs/openssl-1.1.1/crypto/kdf/scrypt.c
  ../../libs/openssl-1.1.1/crypto/ec/ecdh_kdf.c
  ../../libs/openssl-1.1.1/crypto/store/store_lib.c
  ../../libs/openssl-1.1.1/crypto/store/store_init.c
  ../../libs/openssl-1.1.1/crypto/store/store_register.c
  ../../libs/openssl-1.1.1/crypto/store/store_err.c
  ../../libs/openssl-1.1.1/crypto/store/loader_file.c
  ../../libs/openssl-1.1.1/crypto/aria/aria.c
  ../../libs/openssl-1.1.1/crypto/blake2/blake2b.c
  ../../libs/openssl-1.1.1/crypto/blake2/blake2s.c
  ../../libs/openssl-1.1.1/crypto/blake2/m_blake2b.c
  ../../libs/openssl-1.1.1/crypto/blake2/m_blake2s.c
  ../../libs/openssl-1.1.1/crypto/sm3/sm3.c
  ../../libs/openssl-1.1.1/crypto/sm3/m_sm3.c
  ../../libs/openssl-1.1.1/crypto/sm4/sm4.c
  ../../libs/openssl-1.1.1/crypto/poly1305/poly1305.c
  ../../libs/openssl-1.1.1/crypto/poly1305/poly1305_ameth.c
  ../../libs/openssl-1.1.1/crypto/poly1305/poly1305_pmeth.c
  ../../libs/openssl-1.1.1/crypto/siphash/siphash.c
  ../../libs/openssl-1.1.1/crypto/siphash/siphash_ameth.c
  ../../libs/openssl-1.1.1/crypto/siphash/siphash_pmeth.c
  ../../libs/openssl-1.1.1/crypto/sm2/sm2_pmeth.c
  ../../libs/openssl-1.1.1/crypto/sm2/sm2_crypt.c
  ../../libs/openssl-1.1.1/crypto/sm2/sm2_sign.c
  ../../libs/openssl-1.1.1/crypto/chacha/chacha_enc.c
  ../../libs/openssl-1.1.1/crypto/des/des_enc.c
  ../../libs/openssl-1.1.1/crypto/bf/bf_enc.c
  ../../libs/openssl-1.1.1/crypto/cast/c_enc.c

  ${libeay32_obj_files}
)

target_compile_options(libeay32 PRIVATE
  ${OPENSSL_COMPILE_FLAGS}
  ${NETBOX_PLATFORM_FLAGS}
)

target_include_directories(libeay32 SYSTEM BEFORE PRIVATE
  ../../libs/openssl-1.1.1/include
  ../../libs/openssl-1.1.1/include/crypto
  ../../libs/openssl-1.1.1/crypto/modes
  ../../libs/openssl-1.1.1/crypto/ec/curve448/arch_32
  ../../libs/openssl-1.1.1/crypto/ec/curve448
  ../../libs/openssl-1.1.1
)

#-------------------------------------------------------------------------------
#libmemcpy

#add_subdirectory(libs/libmemcpy)

set(LIBMEMCPY_LIB "libmemcpy")

set(LIBMEMCPY_LIB_SOURCES
  ../../libs/libmemcpy/memcpy.c
)

add_library(${LIBMEMCPY_LIB}
  ${LIBMEMCPY_LIB_SOURCES}
)

target_compile_definitions(${LIBMEMCPY_LIB} PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

set_source_files_properties(${LIBMEMCPY_LIB_SOURCES} PROPERTIES LANGUAGE C)
#-------------------------------------------------------------------------------
#atlmfc


set(ATLMFC_COMPILE_FLAGS
  -D_ATL_NO_DEBUG_CRT -D_ATL_NO_UUIDOF -D_ATL_NO_CONNECTION_POINTS -D_ATL_NO_DATETIME_RESOURCES_
  -D_ATL_NO_DEFAULT_LIBS -D_ATL_NO_PERF_SUPPORT -D_AFX_PORTABLE -D_ATL_NO_COMMODULE
  -D_AFX_NO_SOCKET_SUPPORT
  -DNOMINMAX
  -D_MFC_BLD -DAFX_DATA= -DAFX_DATA_IMPORT= -DAFX_CORE_DATA= -DAFX_CORE_DATADEF= -DAFX_CLASS_IMPORT= -DAFX_API_IMPORT=
  -D_ATL_STATIC_REGISTRY -DAFXAPI= -DAFXOLEAPI=
)

if(CMAKE_COMPILER_IS_MINGW)

set(ATLMFC_COMPILE_FLAGS ${ATLMFC_COMPILE_FLAGS}
  -fpermissive
  -Wno-int-to-pointer-cast
  -Wno-pragmas
  -Wno-non-virtual-dtor
  -Wno-unused-parameter
  -Wno-conversion-null
  -Wno-sign-conversion
  -Wno-deprecated
  -Wno-overloaded-virtual
  -Wno-sign-conversion
  -Wno-conversion
  -Wno-unused-result
)

endif(CMAKE_COMPILER_IS_MINGW)

add_library(atlmfc STATIC
  ../../libs/atlmfc/mfc1.cpp
  ../../libs/atlmfc/mfc2.cpp
#  ../../libs/atlmfc/mfc3.cpp
  ../../libs/atlmfc/mfc4.cpp
  ../../libs/atlmfc/mfc5.cpp
  ../../libs/atlmfc/src/mfc/thrdcore.cpp
)

target_compile_options(atlmfc PRIVATE
  ${ATLMFC_COMPILE_FLAGS}
)
if(MSVC)
target_compile_options(atlmfc PRIVATE
  -wd4995 -wd4996 -wd4101 -wd4290
)
endif()

target_compile_definitions(atlmfc PRIVATE
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

target_include_directories(atlmfc PRIVATE
  ../../libs
  ../include
  ../base
  ../../libs/atlmfc/include
  ../../libs/GSL/include
)

#-------------------------------------------------------------------------------
#putty

add_library(putty STATIC
  ../../libs/putty/be_list.c
  ../../libs/putty/callback.c
  ../../libs/putty/crypto/aes-common.c
  ../../libs/putty/crypto/aesold.c
  ../../libs/putty/crypto/aes-select.c
  ../../libs/putty/crypto/aes-sw.c
  ../../libs/putty/crypto/arcfour.c
  ../../libs/putty/crypto/argon2.c
  ../../libs/putty/crypto/bcrypt.c
  ../../libs/putty/crypto/blake2.c
  ../../libs/putty/crypto/blowfish.c
  ../../libs/putty/crypto/chacha20-poly1305.c
  ../../libs/putty/crypto/des.c
  ../../libs/putty/crypto/diffie-hellman.c
  ../../libs/putty/crypto/dsa.c
  ../../libs/putty/crypto/ecc-arithmetic.c
  ../../libs/putty/crypto/ecc-ssh.c
  ../../libs/putty/crypto/hash_simple.c
  ../../libs/putty/crypto/hmac.c
  ../../libs/putty/crypto/mac.c
  ../../libs/putty/crypto/mac_simple.c
  ../../libs/putty/crypto/md5.c
  ../../libs/putty/crypto/mpint.c
  ../../libs/putty/crypto/prng.c
  ../../libs/putty/crypto/pubkey-pem.c
  ../../libs/putty/crypto/pubkey-ppk.c
  ../../libs/putty/crypto/rsa.c
  ../../libs/putty/crypto/sha1-common.c
  ../../libs/putty/crypto/sha1-select.c
  ../../libs/putty/crypto/sha1-sw.c
  ../../libs/putty/crypto/sha256-common.c
  ../../libs/putty/crypto/sha256-select.c
  ../../libs/putty/crypto/sha256-sw.c
  ../../libs/putty/crypto/sha3.c
  ../../libs/putty/crypto/sha512-common.c
  ../../libs/putty/crypto/sha512-select.c
  ../../libs/putty/crypto/sha512-sw.c
  ../../libs/putty/errsock.c
  ../../libs/putty/import.c
  ../../libs/putty/logging.c
  ../../libs/putty/proxy/cproxy.c
  ../../libs/putty/proxy/http_p.c
  ../../libs/putty/proxy/interactor.c
  ../../libs/putty/proxy/local.c
  ../../libs/putty/proxy/nosshproxy.c
  ../../libs/putty/proxy/proxy.c
  ../../libs/putty/proxy/socks4.c
  ../../libs/putty/proxy/socks5.c
  ../../libs/putty/proxy/telnet.c
  ../../libs/putty/settings.c
  ../../libs/putty/ssh/agentf.c
  ../../libs/putty/ssh/bpp2.c
  ../../libs/putty/ssh/bpp-bare.c
  ../../libs/putty/ssh/censor2.c
  ../../libs/putty/ssh/common_p.c
  ../../libs/putty/ssh/connection2.c
  ../../libs/putty/ssh/connection2-client.c
  ../../libs/putty/ssh/gssc.c
  ../../libs/putty/ssh/kex2-client.c
  ../../libs/putty/ssh/mainchan.c
  ../../libs/putty/ssh/nosharing.c
  ../../libs/putty/ssh/pgssapi.c
  ../../libs/putty/ssh/portfwd.c
  ../../libs/putty/ssh/sharing.c
  ../../libs/putty/ssh/ssh.c
  ../../libs/putty/ssh/transient-hostkey-cache.c
  ../../libs/putty/ssh/transport2.c
  ../../libs/putty/ssh/userauth2-client.c
  ../../libs/putty/ssh/verstring.c
  ../../libs/putty/ssh/zlib.c
  ../../libs/putty/sshpubk.c
  ../../libs/putty/sshrand.c
  ../../libs/putty/stubs/nullplug.c
  ../../libs/putty/utils/antispoof.c
  ../../libs/putty/utils/backend_socket_log.c
  ../../libs/putty/utils/base64_decode_atom.c
  ../../libs/putty/utils/base64_encode_atom.c
  ../../libs/putty/utils/bufchain.c
  ../../libs/putty/utils/burnstr.c
  ../../libs/putty/utils/conf.c
  ../../libs/putty/utils/conf_launchable.c
  ../../libs/putty/utils/ctrlparse.c
  ../../libs/putty/utils/default_description.c
  ../../libs/putty/utils/dup_mb_to_wc.c
  ../../libs/putty/utils/dupcat.c
  ../../libs/putty/utils/dupprintf.c
  ../../libs/putty/utils/dupstr.c
  ../../libs/putty/utils/host_strchr.c
  ../../libs/putty/utils/host_strchr_internal.c
  ../../libs/putty/utils/host_strcspn.c
  ../../libs/putty/utils/host_strduptrim.c
  ../../libs/putty/utils/host_strrchr.c
  ../../libs/putty/utils/log_proxy_stderr.c
  ../../libs/putty/utils/make_spr_sw_abort_static.c
  ../../libs/putty/utils/marshal.c
  ../../libs/putty/utils/memory.c
  ../../libs/putty/utils/memxor.c
  ../../libs/putty/utils/null_lp.c
  ../../libs/putty/utils/nullseat.c
  ../../libs/putty/utils/nullstrcmp.c
  ../../libs/putty/utils/out_of_memory.c
  ../../libs/putty/utils/parse_blocksize.c
  ../../libs/putty/utils/prompts.c
  ../../libs/putty/utils/ptrlen.c
  ../../libs/putty/utils/seat_connection_fatal.c
  ../../libs/putty/utils/sk_free_peer_info.c
  ../../libs/putty/utils/smemclr.c
  ../../libs/putty/utils/smemeq.c
  ../../libs/putty/utils/spr_get_error_message.c
  ../../libs/putty/utils/ssh2_pick_fingerprint.c
  ../../libs/putty/utils/sshutils.c
  ../../libs/putty/utils/strbuf.c
  ../../libs/putty/utils/string_length_for_printf.c
  ../../libs/putty/utils/stripctrl.c
  ../../libs/putty/utils/tempseat.c
  ../../libs/putty/utils/tree234.c
  ../../libs/putty/utils/version.c
  ../../libs/putty/utils/wildcard.c
  ../../libs/putty/utils/write_c_string_literal.c
  ../../libs/putty/windows/agent-client.c
  ../../libs/putty/windows/gss.c
  ../../libs/putty/windows/handle-io.c
  ../../libs/putty/windows/handle-socket.c
  ../../libs/putty/windows/handle-wait.c
  ../../libs/putty/windows/local-proxy.c
  ../../libs/putty/windows/named-pipe-client.c
  ../../libs/putty/windows/network.c
  ../../libs/putty/windows/noise.c
  ../../libs/putty/windows/no-jump-list.c
  ../../libs/putty/windows/storage.c
  ../../libs/putty/windows/unicode.c
  ../../libs/putty/windows/utils/agent_named_pipe_name.c
  ../../libs/putty/windows/utils/cryptoapi.c
  ../../libs/putty/windows/utils/defaults.c
  ../../libs/putty/windows/utils/dll_hijacking_protection.c
  ../../libs/putty/windows/utils/escape_registry_key.c
  ../../libs/putty/windows/utils/filename.c
  ../../libs/putty/windows/utils/fontspec.c
  ../../libs/putty/windows/utils/get_system_dir.c
  ../../libs/putty/windows/utils/get_username.c
  ../../libs/putty/windows/utils/load_system32_dll.c
  ../../libs/putty/windows/utils/ltime.c
  ../../libs/putty/windows/utils/open_for_write_would_lose_data.c
  ../../libs/putty/windows/utils/security_p.c
  ../../libs/putty/windows/utils/win_strerror.c
#  ../../libs/putty/ldisc.c
)

target_include_directories(putty PRIVATE
  ../../libs/putty
  ../../libs/putty/windows
)

set(PUTTY_COMPILE_FLAGS
  -DLibrary -D_MT -DMPEXT -DNET_SETUP_DIAGNOSTICS
  -DSECURITY_WIN32 -DWINSCP -DMPEXT -DPLATFORM_HAS_SMEMCLR -D_WINDOWS
)
if(MSVC)
set(PUTTY_COMPILE_FLAGS ${PUTTY_COMPILE_FLAGS}
  -wd4996
)
endif()

target_compile_options(putty PRIVATE
  ${PUTTY_COMPILE_FLAGS}
  -Dssize_t=size_t
)

target_compile_definitions(putty PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

#puttyvs

add_library(puttyvs STATIC
  ../../libs/putty/crypto/aes-ni.c
  ../../libs/putty/crypto/aes-sw.c
  ../../libs/putty/crypto/argon2.c
  ../../libs/putty/crypto/sha256-sw.c
)

target_include_directories(puttyvs PRIVATE
  ../../libs/putty
  ../../libs/putty/windows
)

target_compile_options(puttyvs PRIVATE
  ${PUTTY_COMPILE_FLAGS}
  -Dssize_t=size_t
  -DWINSCP_VS
)

target_compile_definitions(puttyvs PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

#-------------------------------------------------------------------------------
#tinyxml2

add_library(tinyxml2 STATIC
  ../../libs/tinyxml2/tinyxml2.cpp
)

target_include_directories(tinyxml2 PRIVATE
  ../../libs
  ../include
  ../../libs/GSL/include
)

target_compile_options(tinyxml2 PRIVATE
)

target_compile_definitions(tinyxml2 PRIVATE
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  -DTINYXML2_EXPORT
)


#-------------------------------------------------------------------------------
#neon

set(NEON_LIB_SOURCES
  ../../libs/neon/src/ne_alloc.c
  ../../libs/neon/src/ne_auth.c
  ../../libs/neon/src/ne_basic.c
  ../../libs/neon/src/ne_compress.c
  ../../libs/neon/src/ne_dates.c
  ../../libs/neon/src/ne_i18n.c
  ../../libs/neon/src/ne_md5.c
  ../../libs/neon/src/ne_pkcs11.c
  ../../libs/neon/src/ne_redirect.c
  ../../libs/neon/src/ne_request.c
  ../../libs/neon/src/ne_session.c
  ../../libs/neon/src/ne_socket.c
  ../../libs/neon/src/ne_socks.c
  ../../libs/neon/src/ne_sspi.c
  ../../libs/neon/src/ne_string.c
  ../../libs/neon/src/ne_uri.c
  ../../libs/neon/src/ne_utils.c
  ../../libs/neon/src/ne_207.c
  ../../libs/neon/src/ne_xml.c
  ../../libs/neon/src/ne_xmlreq.c
  ../../libs/neon/src/ne_oldacl.c
  ../../libs/neon/src/ne_acl3744.c
  ../../libs/neon/src/ne_props.c
  ../../libs/neon/src/ne_locks.c
  ../../libs/neon/src/ne_openssl.c
)

add_library(neon STATIC
  ${NEON_LIB_SOURCES}
)

target_include_directories(neon PRIVATE
  ../../libs/zlib/src
  ../../libs/expat/lib
  ../../libs/openssl-1.1.1/include
#  ${OPENSSL_LIBRARYDIR}
)

#set_source_files_properties(${NEON_LIB_SOURCES} PROPERTIES LANGUAGE CXX)

set(NEON_COMPILE_FLAGS ${LIBNEON_DEFS} ${LIBEXPAT_DEFS})
if(MSVC)
set(NEON_COMPILE_FLAGS ${NEON_COMPILE_FLAGS}
  -wd4996
)
endif()

#target_compile_options(neon PRIVATE
#)

target_compile_definitions(neon PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  ${NEON_COMPILE_FLAGS}
  -DWINSCP
)

#-------------------------------------------------------------------------------
#expat

add_library(expat STATIC
  ../../libs/expat/lib/xmlparse.c
  ../../libs/expat/lib/xmlrole.c
  ../../libs/expat/lib/xmltok.c
  ../../libs/expat/lib/xmltok_impl.c
  ../../libs/expat/lib/loadlibrary.c
#  ../../libs/expat/lib/xmltok_ns.c
  ../../libs/expat/lib/loadlibrary.c
)

target_include_directories(expat PRIVATE
  ../../libs/expat
)

target_compile_definitions(expat PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  ${LIBEXPAT_DEFS}
)


#-------------------------------------------------------------------------------
#zlib

set(ZLIB_SOURCES
  ../../libs/zlib/src/adler32.c
  ../../libs/zlib/src/crc32.c
  ../../libs/zlib/src/infback.c
  ../../libs/zlib/src/inffast.c
  ../../libs/zlib/src/inflate.c
  ../../libs/zlib/src/inftrees.c
  ../../libs/zlib/src/trees.c
  ../../libs/zlib/src/zutil.c
  ../../libs/zlib/src/match.c
  ../../libs/zlib/src/deflate.c
  ../../libs/zlib/src/deflate_slow.c
  ../../libs/zlib/src/deflate_medium.c
  ../../libs/zlib/src/deflate_fast.c
)

set(ZLIB_SOURCES ${ZLIB_SOURCES}
  ../../libs/zlib/src/arch/x86/crc_folding.c
  ../../libs/zlib/src/arch/x86/x86.c
)

add_library(zlib STATIC
  ${ZLIB_SOURCES}
)

target_include_directories(zlib PRIVATE
  ../../libs/zlib/src
)

target_compile_options(zlib PRIVATE
)

target_compile_definitions(zlib PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  -DUNALIGNED_OK -DZLIB_COMPAT
)

#-------------------------------------------------------------------------------
#dlmalloc

add_library(dlmalloc STATIC
  ../../libs/dlmalloc/dlmalloc.cc
  ../../libs/dlmalloc/dlmalloc-2.8.6.h
)

target_compile_definitions(dlmalloc PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

#-------------------------------------------------------------------------------
#rdestl

add_library(rdestl STATIC
#  ../../libs/rdestl/allocator.cpp
  ../../libs/rdestl/intrusive_list.cpp
  ../../libs/rdestl/list.cpp
#  ../../libs/rdestl/intrusive_slist.cpp
#  ../../libs/rdestl/slist.cpp
  ../../libs/rdestl/sort.h
  ../../libs/rdestl/string_utils.h
#  ../../libs/rdestl/intrusive_list.h
  ../../libs/rdestl/sorted_vector.h
  ../../libs/rdestl/allocator.h
  ../../libs/rdestl/hash_map.h
  ../../libs/rdestl/map.h
  ../../libs/rdestl/basic_string.h
  ../../libs/rdestl/list.h
  ../../libs/rdestl/type_traits.h
#  ../../libs/rdestl/intrusive_slist.h
  ../../libs/rdestl/radix_sorter.h
  ../../libs/rdestl/stack_allocator.h
  ../../libs/rdestl/utility.h
  ../../libs/rdestl/vector.h
#  ../../libs/rdestl/slist.h
  ../../libs/rdestl/fixed_array.h
  ../../libs/rdestl/pair.h
  ../../libs/rdestl/algorithm.h
  ../../libs/rdestl/fixed_list.h
  ../../libs/rdestl/hash.h
  ../../libs/rdestl/alignment.h
  ../../libs/rdestl/fixed_vector.h
  ../../libs/rdestl/rb_tree.h
  ../../libs/rdestl/buffer_allocator.h
  ../../libs/rdestl/sstream.h
  ../../libs/rdestl/rdestl_common.h
  ../../libs/rdestl/stack.h
  ../../libs/rdestl/cow_string_storage.h
  ../../libs/rdestl/fixed_sorted_vector.h
  ../../libs/rdestl/functional.h
  ../../libs/rdestl/simple_string_storage.h
  ../../libs/rdestl/fixed_substring.h
  ../../libs/rdestl/int_to_type.h
  ../../libs/rdestl/rde_string.h
  ../../libs/rdestl/rdestl.h
  ../../libs/rdestl/set.h
  ../../libs/rdestl/iterator.h
)

target_include_directories(rdestl PRIVATE
  ../../libs
  ../include
  ../base
  ../../libs/GSL/include
)

target_compile_definitions(rdestl PRIVATE
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

#-------------------------------------------------------------------------------
#tinylog

add_library(tinylog STATIC
  ../../libs/tinylog/tinylog/TinyLog.cpp
  ../../libs/tinylog/tinylog/Utils.cpp
  ../../libs/tinylog/tinylog/LogStream.cpp
  ../../libs/tinylog/tinylog/Buffer.cpp

  ../../libs/tinylog/tinylog/TinyLog.h
  ../../libs/tinylog/tinylog/Utils.h
  ../../libs/tinylog/tinylog/LogStream.h
  ../../libs/tinylog/tinylog/Config.h
  ../../libs/tinylog/tinylog/Buffer.h
)

target_include_directories(tinylog PRIVATE
  ../../libs
  ../include
  ../base
  ../../libs/GSL/include
)

target_compile_definitions(tinylog PRIVATE
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  -DTINYLOG_EXPORT
)

#-------------------------------------------------------------------------------
#fmt

set(fmt_DEFINES
  -DFMT_EXPORT #-DFMT_HEADER_ONLY
)

#if(FALSE)
add_library(fmt STATIC
  ../../libs/fmt/fmt/format.cc
  ../../libs/fmt/fmt/format.h
  ../../libs/fmt/fmt/printf.cc
  ../../libs/fmt/fmt/printf.h
#  ../../libs/fmt/fmt/ostream.cc
#  ../../libs/fmt/fmt/ostream.h
  ../../libs/fmt/fmt/string.h
)

target_include_directories(fmt PRIVATE
  ../../libs
  ../include
  ../../libs/GSL/include
)

target_compile_definitions(fmt PRIVATE
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  ${fmt_DEFINES}
)

#endif(FALSE)

#-------------------------------------------------------------------------------
#libs3

add_library(s3 STATIC
  ../../libs/libs3/src/bucket.c
  ../../libs/libs3/src/bucket_metadata.c
  ../../libs/libs3/src/error_parser.c
  ../../libs/libs3/src/general.c
  ../../libs/libs3/src/mingw_functions.c
  ../../libs/libs3/src/multipart.c
  ../../libs/libs3/src/object.c
  ../../libs/libs3/src/request.c
  ../../libs/libs3/src/request_context.c
  ../../libs/libs3/src/response_headers_handler.c
  ../../libs/libs3/src/service.c
  ../../libs/libs3/src/service_access_logging.c
  ../../libs/libs3/src/simplexml.c
  ../../libs/libs3/src/util.c
  ../../libs/libs3/inc/string_buffer.h
  ../../libs/libs3/inc/util.h
  ../../libs/libs3/inc/simplexml.h
  ../../libs/libs3/inc/response_headers_handler.h
  ../../libs/libs3/inc/request.h
  ../../libs/libs3/inc/request_context.h
  ../../libs/libs3/inc/libs3.h
  ../../libs/libs3/inc/error_parser.h

)

target_include_directories(s3 PRIVATE
  ../../libs
  ../include
  ../../libs/libs3/inc
  ../../libs/libs3/inc/mingw
  ../../libs/expat/lib
  ../../libs/neon/src
  ../../libs/openssl-1.1.1/include
  ${OPENSSL_LIBRARYDIR}
)

target_compile_definitions(s3 PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  ${LIBEXPAT_DEFS}
  -DS3_EXPORT -DWIN32 -DWINSCP -DNE_LFS -DNEED_LIBS3
)

#-------------------------------------------------------------------------------
#NetBox

if(OPT_USE_UNITY_BUILD)

set(NETBOX_SOURCES
  UnityBuildCore.cpp
  UnityBuildFilezilla.cpp
  UnityBuildMain.cpp
)

else(OPT_USE_UNITY_BUILD)

set(Base_SOURCES
  ../nbcore/nbstring.cpp
  ../nbcore/nbmemory.cpp
  ../nbcore/nbutils.cpp

  ../base/UnicodeString.cpp
  ../base/Classes.cpp
  ../base/Masks.cpp
  ../base/Sysutils.cpp
  ../base/StrUtils.cpp
  ../base/WideStrUtils.cpp
  ../base/LibraryLoader.cpp
  ../base/Common.cpp
  ../base/Exceptions.cpp
  ../base/FileBuffer.cpp
  ../base/Global.cpp
  ../base/System.SyncObjs.cpp
  ../base/FormatUtils.cpp
)

set(Base_HEADERS
  ../nbcore/stdafx.h

  ../base/Classes.hpp
  ../base/Sysutils.hpp
  ../base/Property.hpp
  ../base/function2.hpp
  ../base/UnicodeString.hpp
  ../base/Masks.hpp
  ../base/System.StrUtils.hpp
  ../base/StrUtils.hpp
  ../base/DirectoryMonitor.hpp
  ../base/System.SyncObjs.hpp
  ../base/Windows.hpp
  ../base/Xml.XMLIntf.hpp
  ../base/LibraryLoader.hpp
  ../base/DynamicQueue.hpp
  ../base/System.IOUtils.hpp
  ../base/System.ShlObj.hpp
  ../base/WideStrUtils.hpp
  ../base/Math.hpp
  ../base/PasTools.hpp
  ../base/System.DateUtils.hpp
  ../base/ComCtrls.hpp
  ../base/registry.hpp
  ../base/contnrs.hpp
  ../base/SysInit.hpp
  ../base/System.hpp
  ../base/LanguagesDEPfix.hpp
  ../base/FileCtrl.hpp
  ../base/CompThread.hpp
  ../base/DateUtils.hpp
  ../base/DiscMon.hpp
)

set(Filezilla_HEADERS
  ../filezilla/AsyncSslSocketLayer.h
  ../filezilla/structures.h
  ../filezilla/stdafx.h
  ../filezilla/TransferSocket.h
  ../filezilla/ServerPath.h
  ../filezilla/MainThread.h
  ../filezilla/MFC64bitFix.h
  ../filezilla/FzApiStructures.h
  ../filezilla/FtpListResult.h
  ../filezilla/FtpControlSocket.h
  ../filezilla/FilezillaTools.h
  ../filezilla/FileZillaOpt.h
  ../filezilla/FileZillaIntf.h
  ../filezilla/FileZillaIntern.h
  ../filezilla/FileZillaApi.h
  ../filezilla/AsyncSocketExLayer.h
  ../filezilla/AsyncSocketEx.h
  ../filezilla/AsyncProxySocketLayer.h
  ../filezilla/ApiLog.h
  ../filezilla/afxdll.h
)

set(Filezilla_SOURCES
  ../filezilla/afxdll.cpp
  ../filezilla/FileZillaIntf.cpp
  ../filezilla/FileZillaIntern.cpp
  ../filezilla/ApiLog.cpp
  ../filezilla/ServerPath.cpp
  ../filezilla/AsyncSslSocketLayer.cpp
  ../filezilla/AsyncSocketExLayer.cpp
  ../filezilla/AsyncSocketEx.cpp
  ../filezilla/FileZillaApi.cpp
  ../filezilla/FzApiStructures.cpp
  ../filezilla/FtpControlSocket.cpp
  ../filezilla/MainThread.cpp
  ../filezilla/TransferSocket.cpp
  ../filezilla/FtpListResult.cpp
  ../filezilla/AsyncProxySocketLayer.cpp
  ../filezilla/structures.cpp
  ../filezilla/MFC64bitFix.cpp
  ../filezilla/misc/CBase64Coding.cpp
)

set(WinSCP_SOURCES
  ../core/RemoteFiles.cpp
  ../core/Terminal.cpp
  ../core/FileOperationProgress.cpp
  ../core/Queue.cpp
  ../core/SecureShell.cpp
  ../core/SessionInfo.cpp
  ../core/Script.cpp
  ../core/CoreMain.cpp
  ../core/FileMasks.cpp
  ../core/CopyParam.cpp
  ../core/SessionData.cpp
  ../core/Usage.cpp
  ../core/Configuration.cpp
  ../core/ScpFileSystem.cpp
  ../core/FtpFileSystem.cpp
  ../core/SftpFileSystem.cpp
  ../core/WebDAVFileSystem.cpp
  ../core/S3FileSystem.cpp
  ../core/PuttyIntf.cpp
  ../core/Cryptography.cpp
  ../core/NamedObjs.cpp
  ../core/HierarchicalStorage.cpp
  ../core/Option.cpp
  ../core/FileInfo.cpp
  ../core/FileSystems.cpp
  ../core/Bookmarks.cpp
  ../core/WinSCPSecurity.cpp
  ../core/Http.cpp
  ../core/NeonIntf.cpp
  ../windows/SynchronizeController.cpp
  ../windows/GUITools.cpp
  ../windows/GUIConfiguration.cpp
  ../windows/Tools.cpp
  ../windows/ProgParams.cpp
  ../windows/UserInterface.cpp
  ../windows/WinInterface.cpp
)

set(WinSCP_HEADERS
  ../core/FileMasks.h
  ../core/Bookmarks.h
  ../core/HierarchicalStorage.h
  ../core/Configuration.h
  ../core/Usage.h
  ../core/Terminal.h
  ../core/SessionData.h
  ../core/NamedObjs.h
  ../core/SessionInfo.h
  ../core/CopyParam.h
  ../core/SecureShell.h
  ../core/FileSystems.h
  ../core/ScpFileSystem.h
  ../core/WebDAVFileSystem.h
  ../core/SftpFileSystem.h
  ../core/FtpFileSystem.h
  ../core/S3FileSystem.h
  ../core/Queue.h
  ../core/RemoteFiles.h
  ../core/Cryptography.h
  ../core/Http.h
  ../core/FileOperationProgress.h
  ../core/Option.h
  ../core/Interface.h
  ../core/NeonIntf.h
  ../core/Script.h
  ../core/PuttyTools.h
  ../core/WinSCPSecurity.h
  ../core/PuttyIntf.h
  ../core/FileInfo.h
  ../core/CoreMain.h
  ../windows/WinInterface.h
  ../windows/GUIConfiguration.h
  ../windows/TerminalManager.h
  ../windows/Tools.h
  ../windows/GUITools.h
  ../windows/VCLCommon.h
  ../windows/Setup.h
  ../windows/ProgParams.h
  ../windows/CustomWinConfiguration.h
  ../windows/WinConfiguration.h
  ../windows/SynchronizeController.h
)

set(NetBox_SOURCES
  NetBox.cpp
  FarDialog.cpp
  FarPlugin.cpp
  FarUtils.cpp
  WinSCPFileSystem.cpp
  WinSCPDialogs.cpp
  WinSCPPlugin.cpp
  FarConfiguration.cpp
  FarInterface.cpp
  XmlStorage.cpp
  Far3Storage.cpp
  FarPluginStrings.cpp
  MessageDlg.cpp
)

set(NetBox_HEADERS
  FarUtils.h
  FarPlugin.h
  FarConfiguration.h
  WinSCPPlugin.h
  WinSCPFileSystem.h
  FarDialog.h
  XmlStorage.h
  FarInterface.h
  resource.h
  FarPluginStrings.h
)

set(NETBOX_SOURCES
#  UnityBuildCore.cpp
  ${Base_HEADERS}
  ${Base_SOURCES}

  ${WinSCP_HEADERS}
  ${WinSCP_SOURCES}

#  UnityBuildFilezilla.cpp
  ${Filezilla_HEADERS}
  ${Filezilla_SOURCES}

#  UnityBuildMain.cpp
  ${NetBox_HEADERS}
  ${NetBox_SOURCES}
)

source_group("Filezilla headers" FILES ${Filezilla_HEADERS})
source_group("Filezilla sources" FILES ${Filezilla_SOURCES})
source_group("WinSCP headers" FILES ${WinSCP_HEADERS})
source_group("WinSCP sources" FILES ${WinSCP_SOURCES})
source_group("Base headers" FILES ${Base_HEADERS})
source_group("Base sources" FILES ${Base_SOURCES})
source_group("NetBox headers" FILES ${NetBox_HEADERS})
source_group("NetBox sources" FILES ${NetBox_SOURCES})

endif(OPT_USE_UNITY_BUILD)

set(NETBOX_SOURCES
  ${NETBOX_SOURCES}

  FarPlugin.h
  WinSCPPlugin.h
  FarDialog.h
  resource.h
  WinSCPFileSystem.h
  FarInterface.h
  FarConfiguration.h
  XmlStorage.h
  Far3Storage.h
  FarUtils.h
  FarPluginStrings.h

  ../include/FastDelegateBind.h
  ../include/FastDelegate.h
  ../include/type_traits.h
  ../include/rtti.hpp
  ../include/nbtypes.h
  ../include/nbglobals.h
  ../include/nbsystem.h
  ../include/nbcore.h
  ../include/nbstring.h
  ../include/nbstring.inl
  ../include/nbutils.h

  ../base/Property.hpp
  ../base/Global.h
  ../base/Common.h
  ../base/UnicodeString.hpp
  ../base/Exceptions.h
  ../base/Sysutils.hpp
  ../base/StrUtils.hpp
  ../base/MsgIDs.h
  ../base/vcl.h
  ../base/FileBuffer.h
  ../base/FormatUtils.h

  ../core/WebDAVFileSystem.h
  ../core/S3FileSystem.h
  ../core/Queue.h
  ../core/Terminal.h
  ../core/FileSystems.h
  ../core/SessionInfo.h
  ../core/SessionData.h
  ../core/RemoteFiles.h
  ../core/Http.h
  ../core/FtpFileSystem.h
  ../core/FileMasks.h
  ../core/Configuration.h
  ../core/WinSCPSecurity.h
  ../core/SecureShell.h
  ../core/ScpFileSystem.h
  ../core/NeonIntf.h
  ../core/Interface.h
  ../core/FileInfo.h
  ../core/CopyParam.h
  ../core/SftpFileSystem.h
  ../core/PuttyIntf.h
  ../core/Option.h
  ../core/HierarchicalStorage.h
  ../core/Cryptography.h
  ../core/PuttyTools.h
  ../core/NamedObjs.h
  ../core/FileOperationProgress.h
  ../core/Bookmarks.h
  ../core/CoreMain.h

  ../windows/WinInterface.h
  ../windows/GUITools.h
  ../windows/GUIConfiguration.h
  ../windows/Tools.h
  ../windows/ProgParams.h
  ../windows/SynchronizeController.h

  ../resource/TextsCore.h
  ../resource/rtlconsts.h
  ../resource/HelpCore.h
  ../resource/TextsFileZilla.h
  ../PluginSDK/${FAR_VERSION}/plugin.hpp
  ../PluginSDK/${FAR_VERSION}/farcolor.hpp
  ../PluginSDK/${FAR_VERSION}/PluginSettings.hpp

  ../resource/TextsFileZilla.rc
  NetBox.rc
  NetBox.def
)

add_custom_target(NETBOX_SOURCES SOURCES
  NetBoxEng.lng
  NetBoxRus.lng
  resource.h.template
  NetBox.rc.template
)

if(MSVC)
if(NOT x64)

#[[find_program(MASM_EXECUTABLE ml)
# message("masm: ${MASM_EXECUTABLE}")
set(ASM_OBJECTS)
foreach(src vc_crt_fix)
  set(ASM_SOURCE ${NETBOX_DIR}/${src}.asm)
  set(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${src}.obj)
  set(ASM_OBJECTS ${ASM_OBJECTS} ${ASM_OBJECT})
  add_custom_command(
    OUTPUT ${ASM_OBJECT}
    COMMAND ${MASM_EXECUTABLE}
    ARGS   /c /Fo ${ASM_OBJECT} ${ASM_SOURCE}
    DEPENDS ${ASM_SOURCE}
  )
endforeach(src)

set(NETBOX_SOURCES ${NETBOX_SOURCES}
  vc_crt_fix_impl.cpp
  ${ASM_OBJECTS}
)]]

endif(NOT x64)
endif(MSVC)

set(NETBOX_LIBRARIES
  putty
  puttyvs
  atlmfc
  zlib
  tinyxml2
  ssleay32
  libeay32
  neon
  expat
  dlmalloc
  rdestl
  fmt
  s3
  tinylog
  ${LIBMEMCPY_LIB}

  Ws2_32.lib
  kernel32.lib
  user32.lib
  advapi32.lib
  Version.lib
  winhttp.lib
  winspool.lib
  Crypt32.lib
  secur32.lib
  shell32.lib
  shlwapi.lib
  delayimp.lib
)

if(MSVC)
if(BUILD_DEBUG)
set(NETBOX_LIBRARIES PRIVATE
  ${NETBOX_LIBRARIES}
  libcmtd.lib
  libcpmtd.lib
)
else()
set(NETBOX_LIBRARIES PRIVATE
  ${NETBOX_LIBRARIES}
  libcmt.lib
  libcpmt.lib
)
endif(BUILD_DEBUG)
endif(MSVC)

add_library(NetBox SHARED
  ${NETBOX_SOURCES}
)

target_include_directories(NetBox PRIVATE
  ../../libs
  ../include
  ../../libs
  ../../libs/openssl-1.1.1/include
  ${OPENSSL_LIBRARYDIR}
  ../../libs/putty
  ../../libs/putty/windows
  ../../libs/neon/src
  ../../libs/tinyxml2
  ../../libs/rdestl
  ../../libs/expat/lib
  ../../libs/atlmfc/include
  ../../libs/fmt
  ../../libs/tinylog
  ../../libs/libs3/inc
  ../../libs/GSL/include
  ../filezilla
  ../base
  ../core
  ../windows
  ../resource
  ../include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ../PluginSDK/${FAR_VERSION}
)

#[[set_target_properties(NetBox
  PROPERTIES
  LINK_FLAGS ${NETBOX_DLL_LINK_FLAGS}
  LINK_FLAGS_RELEASE ${NETBOX_DLL_LINK_FLAGS_RELEASE}
  LINK_FLAGS_DEBUG ${NETBOX_DLL_LINK_FLAGS_DEBUG}
)]]

#TODO: COMMAND create_ver.py

target_link_libraries(NetBox PRIVATE
  ${NETBOX_LIBRARIES}
)

target_compile_options(NetBox PRIVATE
  -DBUILD_OFFICIAL
  -wd4251 -wd4275 -wd4661
  ${LIBNEON_DEFS}
  ${LIBEXPAT_DEFS}
  ${ATLMFC_COMPILE_FLAGS}
  ${PUTTY_COMPILE_FLAGS}
  ${NETBOX_DEFS}
  ${NETBOX_UNICODE_FLAGS}
  -DGSL_THROW_ON_CONTRACT_VIOLATION
  -DNOMINMAX
  -D_CRTIMP=
)

set(NETBOX_PLUGIN_DIR ${PROJECT_ROOT}/${FAR_VERSION}_${PROJECT_PLATFORM}/Plugins/NetBox)
string(REPLACE "/" "\\" NETBOX_PLUGIN_DIR ${NETBOX_PLUGIN_DIR})

add_custom_target(create_plugin_dir)
add_custom_command(TARGET create_plugin_dir POST_BUILD
  COMMAND if not exist "${NETBOX_PLUGIN_DIR}" ( ${CMAKE_COMMAND} -E make_directory "${NETBOX_PLUGIN_DIR}" )
  WORKING_DIRECTORY ${BUILD_DIR}
  VERBATIM
)

file(GLOB DIST_FILES
  ${NETBOX_DIR}\\*.lng
)

add_custom_target(copy_to_plugin_dir)
foreach(DIST_FILE ${DIST_FILES})
  add_custom_command(TARGET copy_to_plugin_dir POST_BUILD
#    COMMAND echo DIST_FILE: ${DIST_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DIST_FILE} ${NETBOX_PLUGIN_DIR}
  )
endforeach()

add_custom_command(TARGET NetBox POST_BUILD
  COMMAND if exist "$<TARGET_FILE_DIR:NetBox>\\NetBox.dll" ( ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:NetBox>\\NetBox.dll" ${NETBOX_PLUGIN_DIR} )
  COMMAND if exist "$<TARGET_FILE_DIR:NetBox>\\NetBox.pdb" ( ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:NetBox>\\NetBox.pdb" ${NETBOX_PLUGIN_DIR} )
  COMMAND if exist "$<TARGET_FILE_DIR:NetBox>\\NetBox.map" ( ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:NetBox>\\NetBox.map" ${NETBOX_PLUGIN_DIR} )
)

add_dependencies(copy_to_plugin_dir create_plugin_dir )
add_dependencies(NetBox copy_to_plugin_dir)
#-------------------------------------------------------------------------------
ucm_set_runtime(STATIC)
ucm_add_flags(CXX /EHsc)
ucm_add_flags(CXX C /Zi CONFIG Debug)
ucm_add_flags(CXX C /MP /Gy /Zi CONFIG Release)
#/Ob2 /GR
ucm_add_flags(CXX C /GS- /fp:fast /Oy /GA /GF /Gw /GL CONFIG Release)
#/O1 release min size
#/O2 release
#/Oy - omit frame pointers
#/Oi - enable intrinsic functions
ucm_add_flags(CXX C /Gm- -Ox -Oi -Os CONFIG Release)
#-Ob1
# static runtime settings
#ucm_add_flags(CXX C /MT CONFIG Release)
#ucm_add_flags(CXX C /MTd CONFIG Debug)
# linker settings
ucm_add_linker_flags(MODULE SHARED EXE ${NETBOX_DLL_LINK_FLAGS} /OPT:REF /OPT:ICF=6 /NXCOMPAT /DYNAMICBASE /MANIFEST:NO CONFIG Release)
ucm_add_linker_flags(MODULE SHARED EXE ${NETBOX_DLL_LINK_FLAGS} /DEBUG CONFIG Debug)
ucm_add_linker_flags(MODULE STATIC SHARED EXE /LTCG CONFIG Release)
ucm_print_flags()
#-------------------------------------------------------------------------------
