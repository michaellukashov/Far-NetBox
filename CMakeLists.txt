message(STATUS "CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.15)

project(NetBox)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Remove for compiler-specific features
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type")
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")
# message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Zi")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /Zi")
  # /MP Multi-processor compilation
  # /Zi Generate debug info

  # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
# message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

if(NOT DEFINED PROJECT_PLATFORM OR PROJECT_PLATFORM STREQUAL "")
if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
  set(PROJECT_PLATFORM "ARM64" CACHE STRING "Platform")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(PROJECT_PLATFORM "x64" CACHE STRING "Platform")
else()
  set(PROJECT_PLATFORM "x86" CACHE STRING "Platform")
endif()
endif()

if(PROJECT_PLATFORM STREQUAL "x86" AND CMAKE_BUILD_TYPE STREQUAL "Release")
  set(OPT_USE_UNITY_BUILD ON CACHE STRING "Default use unity build")
else()
  set(OPT_USE_UNITY_BUILD OFF CACHE STRING "Default use unity build")
endif()
set(OPT_CREATE_PLUGIN_DIR OFF CACHE STRING "Create plugin dir")

#-------------------------------------------------------------------------------

include(CMakeParseArguments)
include(${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
# include(CMakeDetermineCXXCompiler)
message("System: ${CMAKE_SYSTEM_NAME}, PROJECT_PLATFORM: ${PROJECT_PLATFORM}")
# message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
# message("MSVC_VERSION: ${MSVC_VERSION}")
# message("CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
# message("CMAKE_COMPILER_IS_MSVC: ${CMAKE_COMPILER_IS_MSVC}")
# message("CMAKE_COMPILER_IS_MINGW: ${CMAKE_COMPILER_IS_MINGW}")
# message("CMAKE_COMPILER_IS_CLANG: ${CMAKE_COMPILER_IS_CLANG}")
# message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
# message("CMAKE_CXX_COMPILER_ARCHITECTURE_ID: ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")

#-------------------------------------------------------------------------------

set(LIBNEON_DEFS -DNE_LFS -DNOCRYPT -DNE_HAVE_SSL -DHAVE_OPENSSL -DNE_HAVE_DAV -DNE_HAVE_ZLIB -DZLIB_COMPAT -DHAVE_EXPAT -DHAVE_EXPAT_H) #" -DUSE_GETADDRINFO")
set(LIBEXPAT_DEFS -DCOMPILED_FROM_DSP -DXML_STATIC)
set(NETBOX_DEFS -DNOMINMAX -DMPEXT -DWINSCP -DFARPLUGIN)
set(NETBOX_DEFS ${NETBOX_DEFS} -DFASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX)
set(NETBOX_DEFS ${NETBOX_DEFS} -DUSE_DLMALLOC -DUSE_DL_PREFIX -DNO_MALLINFO -DUSE_LOCKS=1)
set(NETBOX_DEFS ${NETBOX_DEFS} -DHIDE_TODO)
set(NETBOX_DEFS ${NETBOX_DEFS} -DNEED_LIBS3)
set(NETBOX_DEFS ${NETBOX_DEFS} -DNO_GZIP -DSTRICT -DNOCRYPT)
set(NETBOX_DEFS ${NETBOX_DEFS} -D_WIN32_WINNT=0x0501 -D_LIB -D_WINDOWS -DWIN32 -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
set(NETBOX_DEFS ${NETBOX_DEFS} -D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS -D_DISABLE_VECTOR_ANNOTATION)
set(NETBOX_DEFS ${NETBOX_DEFS} -DCOM_NO_WINDOWS_H)
#-D_WINDLL -D_USRDLL
if (MSVC_VERSION GREATER 1899)
set(NETBOX_DEFS ${NETBOX_DEFS} /Zc:threadSafeInit- /bigobj)
endif()

set(NETBOX_C_FLAGS )

if(PROJECT_PLATFORM STREQUAL "x64")
set(NETBOX_PLATFORM_FLAGS ${NETBOX_PLATFORM_FLAGS} -DWIN64 -D_WIN64 -D_AMD64_)
endif()

if(PROJECT_PLATFORM STREQUAL "x86")
# set(NETBOX_PLATFORM_FLAGS ${NETBOX_PLATFORM_FLAGS} -DWIN32 -D_WIN32)
endif()

if(CMAKE_COMPILER_IS_MINGW)
set(NETBOX_C_FLAGS ${NETBOX_C_FLAGS} -x c --std=c99)
endif()

set(NETBOX_FLAGS_RELEASE -U_DEBUG -DNDEBUG)
set(NETBOX_FLAGS_DEBUG -D_DEBUG -DDEBUG)

set(NETBOX_UNICODE_FLAGS -D_UNICODE -DUNICODE)

if(MSVC)

set(NETBOX_C_WARNING_FLAGS "")
set(NETBOX_CXX_WARNING_FLAGS "")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)

elseif(CMAKE_COMPILER_IS_MINGW)

set(NETBOX_C_WARNING_FLAGS -Wno-unknown-pragmas)
set(NETBOX_CXX_WARNING_FLAGS ${NETBOX_C_WARNING_FLAGS} -Wno-attributes -Wno-write-strings -Wno-old-style-cast -Wno-effc++ -Wno-unknown-pragmas -Wno-unused-value)
set(NETBOX_CXX_FLAGS ${NETBOX_CXX_FLAGS} -Woverloaded-virtual -Wnon-virtual-dtor)
set(NETBOX_C_FLAGS ${NETBOX_C_FLAGS} -static -s -Wall -Wextra -pedantic -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code)
set(NETBOX_FLAGS_RELEASE ${NETBOX_FLAGS_RELEASE})
set(NETBOX_FLAGS_DEBUG ${NETBOX_FLAGS_DEBUG})

endif()

set(NETBOX_C_FLAGS ${NETBOX_DEFS} ${NETBOX_C_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_C_WARNING_FLAGS})
set(NETBOX_CXX_FLAGS ${NETBOX_DEFS} ${NETBOX_CXX_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_CXX_WARNING_FLAGS})

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
  /DELAYLOAD:ws2_32.dll
  /DELAYLOAD:oleaut32.dll
  /DELAYLOAD:shlwapi.dll
  /DELAYLOAD:crypt32.dll
  /MANIFEST:NO
  /SUBSYSTEM:WINDOWS
  /NOLOGO
  /ignore:4099
  /ignore:4197
  /ignore:4199
)
#/DELAYLOAD:gdi32.dll

set(NETBOX_DLL_LINK_FLAGS_RELEASE ${NETBOX_DLL_LINK_FLAGS} /RELEASE /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG)
set(NETBOX_DLL_LINK_FLAGS_DEBUG ${NETBOX_DLL_LINK_FLAGS} /DEBUG)

elseif(CMAKE_COMPILER_IS_MINGW)
set(NETBOX_DLL_LINK_FLAGS )
set(NETBOX_DLL_LINK_FLAGS_RELEASE ${NETBOX_DLL_LINK_FLAGS})
set(NETBOX_DLL_LINK_FLAGS_DEBUG ${NETBOX_DLL_LINK_FLAGS})
endif()

if(MSVC)

#find_program(NASM_EXECUTABLE nasm)
# message("nasm: ${NASM_EXECUTABLE}")
set(NASM_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/buildtools/tools/nasm.exe)

macro(compile_asm_files)
  cmake_parse_arguments(ARG "" "RESULT" "ASM_FILES" ${ARGN})
  if(NOT ARG_ASM_FILES)
      set(ARG_ASM_FILES " ")
  endif()
#  message("ARG_ASM_FILES: ${ARG_ASM_FILES}")
  if(PROJECT_PLATFORM STREQUAL "x64")
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

# OpenSSL - now built from libs/openssl-3/CMakeLists.txt
# See cmake/Libraries/OpenSSL.cmake for configuration
#-------------------------------------------------------------------------------

# Add OpenSSL subdirectory
add_subdirectory(libs/openssl-3)

# OpenSSL libraries are now built from libs/openssl-3/CMakeLists.txt
# See cmake/Libraries/OpenSSL.cmake for configuration

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
  libs/atlmfc/mfc1.cpp
  libs/atlmfc/mfc2.cpp
#  libs/atlmfc/mfc3.cpp
  libs/atlmfc/mfc4.cpp
  libs/atlmfc/mfc5.cpp
  libs/atlmfc/src/mfc/thrdcore.cpp
)

target_compile_options(atlmfc PRIVATE
  ${ATLMFC_COMPILE_FLAGS}
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}

)

if(MSVC)
target_compile_options(atlmfc PRIVATE
  -wd4995 -wd4996 -wd4101 -wd4290
)
endif()

target_include_directories(atlmfc PRIVATE
  libs
  libs/atlmfc/include
  libs/GSL/include
  src/include
  src/base
)

#-------------------------------------------------------------------------------
#putty

add_library(putty STATIC
  libs/putty/be_list.c
  libs/putty/callback.c
#  libs/putty/misc.c
  libs/putty/crypto/aes-common.c
  libs/putty/crypto/aesold.c
  libs/putty/crypto/aes-select.c
  libs/putty/crypto/aes-sw.c
  libs/putty/crypto/arcfour.c
  libs/putty/crypto/argon2.c
  libs/putty/crypto/bcrypt.c
  libs/putty/crypto/blake2.c
  libs/putty/crypto/blowfish.c
  libs/putty/crypto/chacha20-poly1305.c
  libs/putty/crypto/des.c
  libs/putty/crypto/diffie-hellman.c
  libs/putty/crypto/dsa.c
  libs/putty/crypto/ecc-arithmetic.c
  libs/putty/crypto/ecc-ssh.c
  libs/putty/crypto/hash_simple.c
  libs/putty/crypto/hmac.c
  libs/putty/crypto/mac.c
  libs/putty/crypto/mac_simple.c
  libs/putty/crypto/md5.c
  libs/putty/crypto/mpint.c
  libs/putty/crypto/prng.c
  libs/putty/crypto/pubkey-pem.c
  libs/putty/crypto/pubkey-ppk.c
  libs/putty/crypto/rfc6979.c
  libs/putty/crypto/rsa.c
  libs/putty/crypto/sha1-common.c
  libs/putty/crypto/sha1-select.c
  libs/putty/crypto/sha1-sw.c
  libs/putty/crypto/sha256-common.c
  libs/putty/crypto/sha256-select.c
  libs/putty/crypto/sha256-sw.c
  libs/putty/crypto/sha3.c
  libs/putty/crypto/sha512-common.c
  libs/putty/crypto/sha512-select.c
  libs/putty/crypto/sha512-sw.c
  libs/putty/crypto/kex-hybrid.c
  libs/putty/crypto/mlkem.c
  libs/putty/errsock.c
  libs/putty/import.c
  libs/putty/logging.c
  libs/putty/proxy/cproxy.c
  libs/putty/proxy/http_p.c
  libs/putty/proxy/interactor.c
  libs/putty/proxy/local.c
  libs/putty/proxy/nosshproxy.c
  libs/putty/proxy/proxy.c
  libs/putty/proxy/socks4.c
  libs/putty/proxy/socks5.c
  libs/putty/proxy/telnet.c
  libs/putty/settings.c
  libs/putty/ssh/agentf.c
  libs/putty/ssh/bpp2.c
  libs/putty/ssh/bpp-bare.c
  libs/putty/ssh/censor2.c
  libs/putty/ssh/common_p.c
  libs/putty/ssh/connection2.c
  libs/putty/ssh/connection2-client.c
  libs/putty/ssh/gssc.c
  libs/putty/ssh/kex2-client.c
  libs/putty/ssh/mainchan.c
  libs/putty/ssh/nosharing.c
  libs/putty/ssh/pgssapi.c
  libs/putty/ssh/portfwd.c
  libs/putty/ssh/sharing.c
  libs/putty/ssh/ssh.c
  libs/putty/ssh/transient-hostkey-cache.c
  libs/putty/ssh/transport2.c
  libs/putty/ssh/userauth2-client.c
  libs/putty/ssh/verstring.c
  libs/putty/ssh/zlib.c
  libs/putty/sshpubk.c
  libs/putty/sshrand.c
  libs/putty/utils/antispoof.c
  libs/putty/utils/backend_socket_log.c
  libs/putty/utils/base64_decode_atom.c
  libs/putty/utils/base64_encode_atom.c
  libs/putty/utils/bufchain.c
  libs/putty/utils/burnstr.c
  libs/putty/utils/conf.c
  libs/putty/utils/conf_launchable.c
  libs/putty/utils/ctrlparse.c
  libs/putty/utils/default_description.c
  libs/putty/utils/dup_mb_to_wc.c
  libs/putty/utils/dupcat.c
  libs/putty/utils/dupprintf.c
  libs/putty/utils/dupstr.c
  libs/putty/utils/host_strchr.c
  libs/putty/utils/host_strchr_internal.c
  libs/putty/utils/host_strcspn.c
  libs/putty/utils/host_strduptrim.c
  libs/putty/utils/host_strrchr.c
  libs/putty/utils/log_proxy_stderr.c
  libs/putty/utils/make_spr_sw_abort_static.c
  libs/putty/utils/marshal.c
  libs/putty/utils/memory.c
  libs/putty/utils/memxor.c
  libs/putty/stubs/null-lp.c
  libs/putty/stubs/null-mac.c
  libs/putty/stubs/null-opener.c
  libs/putty/stubs/null-plug.c
  libs/putty/stubs/null-seat.c
  libs/putty/stubs/null-cipher.c
  libs/putty/stubs/null-key.c
  libs/putty/stubs/null-socket.c
  libs/putty/utils/nullstrcmp.c
  libs/putty/utils/out_of_memory.c
  libs/putty/utils/parse_blocksize.c
  libs/putty/utils/prompts.c
  libs/putty/utils/ptrlen.c
  libs/putty/utils/seat_connection_fatal.c
  libs/putty/utils/sk_free_peer_info.c
  libs/putty/utils/smemclr.c
  libs/putty/utils/smemeq.c
  libs/putty/utils/spr_get_error_message.c
  libs/putty/utils/ssh2_pick_fingerprint.c
  libs/putty/utils/sshutils.c
  libs/putty/utils/strbuf.c
  libs/putty/utils/string_length_for_printf.c
  libs/putty/utils/stripctrl.c
  libs/putty/utils/tempseat.c
  libs/putty/utils/tree234.c
  libs/putty/utils/version.c
  libs/putty/utils/wildcard.c
  libs/putty/utils/write_c_string_literal.c
  libs/putty/utils/conf_dest.c
  libs/putty/utils/base64_valid.c
  libs/putty/utils/decode_utf8.c
  libs/putty/utils/decode_utf8_to_wchar.c
  libs/putty/utils/decode_utf8_to_wide_string.c
  libs/putty/utils/dup_wc_to_mb.c
  libs/putty/utils/dupwcs.c
  libs/putty/utils/encode_utf8.c
  libs/putty/utils/encode_wide_string_as_utf8.c
  libs/putty/utils/logeventf.c
  libs/putty/utils/conf_data.c
  libs/putty/utils/burnwcs.c
  libs/putty/windows/agent-client.c
  libs/putty/windows/gss.c
  libs/putty/windows/handle-io.c
  libs/putty/windows/handle-socket.c
  libs/putty/windows/handle-wait.c
  libs/putty/windows/local-proxy.c
  libs/putty/windows/named-pipe-client.c
  libs/putty/windows/network.c
  libs/putty/windows/noise.c
  libs/putty/windows/no-jump-list.c
  libs/putty/windows/storage.c
  libs/putty/windows/unicode.c
  libs/putty/windows/utils/agent_named_pipe_name.c
  libs/putty/windows/utils/cryptoapi.c
  libs/putty/windows/utils/defaults.c
  libs/putty/windows/utils/escape_registry_key.c
  libs/putty/windows/utils/filename.c
  libs/putty/windows/utils/fontspec.c
  libs/putty/windows/utils/get_system_dir.c
  libs/putty/windows/utils/get_username.c
  libs/putty/windows/utils/load_system32_dll.c
  libs/putty/windows/utils/ltime.c
  libs/putty/windows/utils/open_for_write_would_lose_data.c
  libs/putty/windows/utils/security_p.c
  libs/putty/windows/utils/win_strerror.c
  # libs/putty/windows/utils/dll_hijacking_protection.c
#  libs/putty/ldisc.c
#  libs/putty/crypto/aesgcm-common.c
  libs/putty/crypto/aesgcm-footer.h
  libs/putty/crypto/aesgcm-ref-poly.c
  libs/putty/crypto/aesgcm-select.c
  libs/putty/crypto/aesgcm-sw.c
  libs/putty/crypto/aesgcm.h
  libs/putty/crypto/ntru.c
  libs/putty/crypto/openssh-certs.c
  libs/putty/utils/base64_decode.c
  libs/putty/utils/base64_encode.c
  libs/putty/utils/cert-expr.c
  libs/putty/utils/host_ca_new_free.c
  libs/putty/utils/key_components.c
#  libs/putty/utils/percent_decode.c
#  libs/putty/utils/percent_encode.c
  libs/putty/utils/seat_dialog_text.c
  libs/putty/windows/utils/registry.c
)

target_include_directories(putty PRIVATE
  libs/putty
  libs/putty/windows
)
# set_source_files_properties(${NEON_LIB_SOURCES} PROPERTIES LANGUAGE CXX)
set(PUTTY_COMPILE_FLAGS
  -DLibrary -D_MT -DMPEXT -DNET_SETUP_DIAGNOSTICS
  -DSECURITY_WIN32 -DWINSCP -DMPEXT -DPLATFORM_HAS_SMEMCLR -D_WINDOWS
)
if(MSVC)
set(PUTTY_COMPILE_FLAGS ${PUTTY_COMPILE_FLAGS}
  -wd4996
  -wd4068 # warning C4068: unknown pragma 'warn'
)
endif()

target_compile_options(putty PRIVATE
  ${PUTTY_COMPILE_FLAGS}
  -Dssize_t=size_t
  ${NETBOX_C_FLAGS}
  #  ${NETBOX_UNICODE_FLAGS}
)

#puttyvs

set(puttyvs_SOURCES
  libs/putty/crypto/aes-sw.c
  libs/putty/crypto/argon2.c
  libs/putty/crypto/sha256-sw.c
)

if(NOT PROJECT_PLATFORM STREQUAL "ARM64")

set(puttyvs_SOURCES ${puttyvs_SOURCES}
  libs/putty/crypto/aes-ni.c
)

endif(NOT PROJECT_PLATFORM STREQUAL "ARM64")

add_library(puttyvs STATIC
 ${puttyvs_SOURCES}
)

target_include_directories(puttyvs PRIVATE
  libs/putty
  libs/putty/windows
)

target_compile_options(puttyvs PRIVATE
  ${PUTTY_COMPILE_FLAGS}
  -Dssize_t=size_t
  -DWINSCP_VS
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

#-------------------------------------------------------------------------------
#tinyxml2

add_library(tinyxml2 STATIC
  libs/tinyxml2/tinyxml2.cpp
)

target_include_directories(tinyxml2 PRIVATE
  libs
  libs/GSL/include
  src/include
)

target_compile_options(tinyxml2 PRIVATE
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

target_compile_definitions(tinyxml2 PRIVATE
#  -DTINYXML2_EXPORT
)


#-------------------------------------------------------------------------------
#neon

set(NEON_LIB_SOURCES
  libs/neon/src/ne_alloc.c
  libs/neon/src/ne_auth.c
  libs/neon/src/ne_basic.c
  libs/neon/src/ne_compress.c
  libs/neon/src/ne_dates.c
  libs/neon/src/ne_i18n.c
  libs/neon/src/ne_md5.c
  libs/neon/src/ne_pkcs11.c
  libs/neon/src/ne_redirect.c
  libs/neon/src/ne_request.c
  libs/neon/src/ne_session.c
  libs/neon/src/ne_socket.c
  libs/neon/src/ne_socks.c
  libs/neon/src/ne_sspi.c
  libs/neon/src/ne_string.c
  libs/neon/src/ne_uri.c
  libs/neon/src/ne_utils.c
  libs/neon/src/ne_207.c
  libs/neon/src/ne_xml.c
  libs/neon/src/ne_xmlreq.c
  libs/neon/src/ne_oldacl.c
  libs/neon/src/ne_acl3744.c
  libs/neon/src/ne_props.c
  libs/neon/src/ne_locks.c
  libs/neon/src/ne_openssl.c
)

add_library(neon STATIC
  ${NEON_LIB_SOURCES}
)

target_include_directories(neon PRIVATE
  libs/zlib-ng
  libs/expat/lib
  libs/openssl-3/include
)

#set_source_files_properties(${NEON_LIB_SOURCES} PROPERTIES LANGUAGE CXX)

set(NEON_COMPILE_FLAGS ${LIBNEON_DEFS} ${LIBEXPAT_DEFS})
if(MSVC)
set(NEON_COMPILE_FLAGS ${NEON_COMPILE_FLAGS}
  -wd4996
)
endif()

target_compile_options(neon PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  ${NEON_COMPILE_FLAGS}
)

target_compile_definitions(neon PRIVATE
  -DWINSCP
  -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS
  # -U_UNICODE -UUNICODE
  -DZLIB_NAME_MANGLING_H
)

#-------------------------------------------------------------------------------
#expat

add_library(expat STATIC
  libs/expat/lib/xmlparse.c
  libs/expat/lib/xmlrole.c
  libs/expat/lib/xmltok.c
  libs/expat/lib/xmltok_impl.c
  libs/expat/lib/loadlibrary.c
#  libs/expat/lib/xmltok_ns.c
  libs/expat/lib/loadlibrary.c
)

target_include_directories(expat PRIVATE
  libs/expat
)

target_compile_options(expat PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  ${LIBEXPAT_DEFS}
)


#-------------------------------------------------------------------------------
#zlib-ng

set(ZLIB_SOURCES
  libs/zlib-ng/adler32.c
  libs/zlib-ng/infback.c
  libs/zlib-ng/inflate.c
  libs/zlib-ng/inftrees.c
  libs/zlib-ng/trees.c
  libs/zlib-ng/zutil.c
  libs/zlib-ng/deflate.c
  libs/zlib-ng/deflate_slow.c
  libs/zlib-ng/deflate_medium.c
  libs/zlib-ng/deflate_fast.c
  libs/zlib-ng/crc32.c
  libs/zlib-ng/insert_string.c
  libs/zlib-ng/arch/generic/crc32_fold_c.c
  libs/zlib-ng/arch/generic/slide_hash_c.c
  libs/zlib-ng/arch/generic/chunkset_c.c
  libs/zlib-ng/arch/generic/compare256_c.c
  libs/zlib-ng/arch/generic/crc32_braid_c.c
  libs/zlib-ng/arch/generic/adler32_c.c
  libs/zlib-ng/arch/generic/adler32_fold_c.c
)

if(PROJECT_PLATFORM STREQUAL "ARM64")
set(ZLIB_SOURCES ${ZLIB_SOURCES}
  libs/zlib-ng/arch/arm/slide_hash_neon.c
  libs/zlib-ng/arch/arm/crc32_acle.c
  libs/zlib-ng/arch/arm/slide_hash_armv6.c
  libs/zlib-ng/arch/arm/chunkset_neon.c
  libs/zlib-ng/arch/arm/compare256_neon.c
  libs/zlib-ng/arch/arm/adler32_neon.c
  libs/zlib-ng/arch/arm/arm_features.c
)
else()
set(ZLIB_SOURCES ${ZLIB_SOURCES}
  libs/zlib-ng/arch/x86/x86_features.c
  # libs/zlib-ng/arch/x86/insert_string_sse42.c
  libs/zlib-ng/arch/x86/slide_hash_avx2.c
  libs/zlib-ng/arch/x86/slide_hash_sse2.c
  libs/zlib-ng/arch/x86/crc32_pclmulqdq.c
  libs/zlib-ng/arch/x86/crc32_vpclmulqdq.c
  libs/zlib-ng/arch/x86/compare256_avx2.c
  libs/zlib-ng/arch/x86/compare256_sse2.c
  libs/zlib-ng/arch/x86/chunkset_sse2.c
  libs/zlib-ng/arch/x86/chunkset_ssse3.c
  libs/zlib-ng/arch/x86/chunkset_avx2.c
  libs/zlib-ng/arch/x86/chunkset_avx512.c
  libs/zlib-ng/arch/x86/adler32_avx512_vnni.c
  libs/zlib-ng/arch/x86/adler32_sse42.c
  libs/zlib-ng/arch/x86/adler32_ssse3.c
  libs/zlib-ng/arch/x86/adler32_avx512.c
  libs/zlib-ng/arch/x86/adler32_avx2.c
)
endif()

add_library(zlib STATIC
  ${ZLIB_SOURCES}
)

target_include_directories(zlib PRIVATE
  libs/zlib-ng
  libs/zlib-ng/arch/generic
)

target_compile_options(zlib PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

target_compile_definitions(zlib PRIVATE
  # -DUNALIGNED_OK
  # -DZLIB_INTERNAL=
  -DZLIB_COMPAT
  -DZLIB_NAME_MANGLING_H
  -DMEMCPY=memcpy
  -DMEMSET=memset
)
if(PROJECT_PLATFORM STREQUAL "ARM64")
target_compile_definitions(zlib PRIVATE
  -D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE=1
  -D_CRT_SECURE_NO_DEPRECATE
  -D_CRT_NONSTDC_NO_DEPRECATE
  -DARM_FEATURES
  -DARM_NEON_HASLD4
)
target_include_directories(zlib PRIVATE
  libs/zlib-ng/arch/arm
)
else()
target_include_directories(zlib PRIVATE
  libs/zlib-ng/arch/x86
)
endif()

#-------------------------------------------------------------------------------
#dlmalloc

add_library(dlmalloc STATIC
  libs/dlmalloc/dlmalloc.cc
  libs/dlmalloc/dlmalloc-2.8.6.h
)

target_compile_options(dlmalloc PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

#-------------------------------------------------------------------------------
#tinylog

add_library(tinylog STATIC
  libs/tinylog/src/TinyLog.cpp
  libs/tinylog/src/Utils.cpp
  libs/tinylog/src/LogStream.cpp
  libs/tinylog/src/Buffer.cpp
  libs/tinylog/src/LockFreeQueue.cpp

  libs/tinylog/tinylog/TinyLog.h
  libs/tinylog/tinylog/Utils.h
  libs/tinylog/tinylog/LogStream.h
  libs/tinylog/tinylog/Config.h
  libs/tinylog/tinylog/Buffer.h
  libs/tinylog/tinylog/LockFreeQueue.h
)

target_include_directories(tinylog PRIVATE
  libs
  libs/fmt
  libs/tinylog
  libs/GSL/include
  src/include
  src/base
  # src/resource
)

target_compile_options(tinylog PRIVATE
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  ${NETBOX_PLATFORM_FLAGS}
)

target_compile_definitions(tinylog PRIVATE
  -DTINYLOG_EXPORT
)

#-------------------------------------------------------------------------------
#fmt

set(fmt_DEFINES
  #-DFMT_EXPORT
  #-DFMT_HEADER_ONLY
)

#if(FALSE)
add_library(fmt STATIC
  libs/fmt/fmt/format.cc
  libs/fmt/fmt/format.h
  libs/fmt/fmt/printf.cc
  libs/fmt/fmt/printf.h
#  libs/fmt/fmt/ostream.cc
#  libs/fmt/fmt/ostream.h
  libs/fmt/fmt/string.h
)

target_include_directories(fmt PRIVATE
  libs
  libs/GSL/include
  src/include
)

target_compile_options(fmt PRIVATE
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  ${NETBOX_PLATFORM_FLAGS}
)

target_compile_definitions(fmt PRIVATE
  ${fmt_DEFINES}
)

#endif(FALSE)

#-------------------------------------------------------------------------------
#libs3

add_library(s3 STATIC
  libs/libs3/src/bucket.c
  libs/libs3/src/bucket_metadata.c
  libs/libs3/src/error_parser.c
  libs/libs3/src/general.c
  libs/libs3/src/mingw_functions.c
  libs/libs3/src/multipart.c
  libs/libs3/src/object.c
  libs/libs3/src/request.c
  libs/libs3/src/request_context.c
  libs/libs3/src/response_headers_handler.c
  libs/libs3/src/service.c
  libs/libs3/src/service_access_logging.c
  libs/libs3/src/simplexml.c
  libs/libs3/src/util.c
  libs/libs3/inc/string_buffer.h
  libs/libs3/inc/util.h
  libs/libs3/inc/simplexml.h
  libs/libs3/inc/response_headers_handler.h
  libs/libs3/inc/request.h
  libs/libs3/inc/request_context.h
  libs/libs3/inc/libs3.h
  libs/libs3/inc/error_parser.h

)

target_include_directories(s3 PRIVATE
  libs
  src/include
  libs/libs3/inc
  libs/libs3/inc/mingw
  libs/expat/lib
  libs/neon/src
  libs/openssl-3/include
  # libs/openssl-3/providers/common/include
)

target_compile_options(s3 PRIVATE
  ${NETBOX_C_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

target_compile_definitions(s3 PRIVATE
  ${LIBEXPAT_DEFS}
  -DS3_EXPORT -DWIN32 -DWINSCP -DNE_LFS -DNEED_LIBS3
)

#-------------------------------------------------------------------------------
#NetBox

if(OPT_USE_UNITY_BUILD)

set(NETBOX_SOURCES
  src/NetBox/UnityBuildCore.cpp
  src/NetBox/UnityBuildFilezilla.cpp
  src/NetBox/UnityBuildMain.cpp
)

else(OPT_USE_UNITY_BUILD)

set(Base_SOURCES
  src/nbcore/nbstring.cpp
  src/nbcore/nbmemory.cpp
  src/nbcore/nbutils.cpp

  src/base/UnicodeString.cpp
  src/base/Classes.cpp
  src/base/Masks.cpp
  src/base/Sysutils.cpp
  src/base/StrUtils.cpp
  src/base/WideStrUtils.cpp
  src/base/LibraryLoader.cpp
)

set(Base_HEADERS
  src/nbcore/stdafx.h

  src/base/Classes.hpp
  src/base/Sysutils.hpp
  src/base/Property.hpp
  src/base/function2.hpp
  src/base/UnicodeString.hpp
  src/base/Masks.hpp
  src/base/System.StrUtils.hpp
  src/base/StrUtils.hpp
  src/base/DirectoryMonitor.hpp
  src/base/System.SyncObjs.hpp
  src/base/Windows.hpp
  src/base/Xml.XMLIntf.hpp
  src/base/LibraryLoader.hpp
  src/base/DynamicQueue.hpp
  src/base/System.IOUtils.hpp
  src/base/System.ShlObj.hpp
  src/base/WideStrUtils.hpp
  src/base/Math.hpp
  src/base/PasTools.hpp
  src/base/System.DateUtils.hpp
  src/base/ComCtrls.hpp
  src/base/registry.hpp
  src/base/contnrs.hpp
  src/base/SysInit.hpp
  src/base/System.hpp
  src/base/LanguagesDEPfix.hpp
  src/base/FileCtrl.hpp
  src/base/CompThread.hpp
  src/base/DateUtils.hpp
  src/base/DiscMon.hpp
)

set(Filezilla_HEADERS
  src/filezilla/AsyncSslSocketLayer.h
  src/filezilla/structures.h
  src/filezilla/stdafx.h
  src/filezilla/TransferSocket.h
  src/filezilla/ServerPath.h
  src/filezilla/MainThread.h
  src/filezilla/MFC64bitFix.h
  src/filezilla/FzApiStructures.h
  src/filezilla/FtpListResult.h
  src/filezilla/FtpControlSocket.h
  src/filezilla/FilezillaTools.h
  src/filezilla/FileZillaOpt.h
  src/filezilla/FileZillaIntf.h
  src/filezilla/FileZillaIntern.h
  src/filezilla/FileZillaApi.h
  src/filezilla/AsyncSocketExLayer.h
  src/filezilla/AsyncSocketEx.h
  src/filezilla/AsyncProxySocketLayer.h
  src/filezilla/ApiLog.h
  src/filezilla/afxdll.h
)

set(Filezilla_SOURCES
  src/filezilla/afxdll.cpp
  src/filezilla/FileZillaIntf.cpp
  src/filezilla/FileZillaIntern.cpp
  src/filezilla/ApiLog.cpp
  src/filezilla/ServerPath.cpp
  src/filezilla/AsyncSslSocketLayer.cpp
  src/filezilla/AsyncSocketExLayer.cpp
  src/filezilla/AsyncSocketEx.cpp
  src/filezilla/FileZillaApi.cpp
  src/filezilla/FzApiStructures.cpp
  src/filezilla/FtpControlSocket.cpp
  src/filezilla/MainThread.cpp
  src/filezilla/TransferSocket.cpp
  src/filezilla/FtpListResult.cpp
  src/filezilla/AsyncProxySocketLayer.cpp
  src/filezilla/structures.cpp
  src/filezilla/MFC64bitFix.cpp
  src/filezilla/misc/CBase64Coding.cpp
)

set(WinSCP_SOURCES
  src/base/Common.cpp
  src/base/Exceptions.cpp
  src/base/FileBuffer.cpp
  src/base/System.IOUtils.cpp
  src/base/Global.cpp
  src/base/System.SyncObjs.cpp
  src/base/FormatUtils.cpp
  src/base/ObjIDs.cpp

  src/core/RemoteFiles.cpp
  src/core/Terminal.cpp
  src/core/FileOperationProgress.cpp
  src/core/Queue.cpp
  src/core/SecureShell.cpp
  src/core/SessionInfo.cpp
  src/core/Script.cpp
  src/core/CoreMain.cpp
  src/core/FileMasks.cpp
  src/core/CopyParam.cpp
  src/core/SessionData.cpp
  src/core/Usage.cpp
  src/core/Configuration.cpp
  src/core/ScpFileSystem.cpp
  src/core/FtpFileSystem.cpp
  src/core/SftpFileSystem.cpp
  src/core/WebDAVFileSystem.cpp
  src/core/S3FileSystem.cpp
  src/core/PuttyIntf.cpp
  src/core/Cryptography.cpp
  src/core/NamedObjs.cpp
  src/core/HierarchicalStorage.cpp
  src/core/Option.cpp
  src/core/FileInfo.cpp
  src/core/FileSystems.cpp
  src/core/Bookmarks.cpp
  src/core/WinSCPSecurity.cpp
  src/core/Http.cpp
  src/core/NeonIntf.cpp

  src/windows/SynchronizeController.cpp
  src/windows/GUITools.cpp
  src/windows/GUIConfiguration.cpp
  src/windows/Tools.cpp
  src/windows/ProgParams.cpp
  src/windows/UserInterface.cpp
  src/windows/WinInterface.cpp
)

set(WinSCP_HEADERS
  src/core/FileMasks.h
  src/core/Bookmarks.h
  src/core/HierarchicalStorage.h
  src/core/Configuration.h
  src/core/Usage.h
  src/core/Terminal.h
  src/core/SessionData.h
  src/core/NamedObjs.h
  src/core/SessionInfo.h
  src/core/CopyParam.h
  src/core/SecureShell.h
  src/core/FileSystems.h
  src/core/ScpFileSystem.h
  src/core/WebDAVFileSystem.h
  src/core/SftpFileSystem.h
  src/core/FtpFileSystem.h
  src/core/S3FileSystem.h
  src/core/Queue.h
  src/core/RemoteFiles.h
  src/core/Cryptography.h
  src/core/Http.h
  src/core/FileOperationProgress.h
  src/core/Option.h
  src/core/Interface.h
  src/core/NeonIntf.h
  src/core/Script.h
  src/core/PuttyTools.h
  src/core/WinSCPSecurity.h
  src/core/PuttyIntf.h
  src/core/FileInfo.h
  src/core/CoreMain.h

  src/windows/WinInterface.h
  src/windows/GUIConfiguration.h
  src/windows/TerminalManager.h
  src/windows/Tools.h
  src/windows/GUITools.h
  src/windows/VCLCommon.h
  src/windows/Setup.h
  src/windows/ProgParams.h
  src/windows/CustomWinConfiguration.h
  src/windows/WinConfiguration.h
  src/windows/SynchronizeController.h
)

set(NetBox_SOURCES
  src/NetBox/NetBox.cpp
  src/NetBox/FarDialog.cpp
  src/NetBox/FarPlugin.cpp
  src/NetBox/FarUtils.cpp
  src/NetBox/WinSCPFileSystem.cpp
  src/NetBox/WinSCPDialogs.cpp
  src/NetBox/WinSCPPlugin.cpp
  src/NetBox/FarConfiguration.cpp
  src/NetBox/FarInterface.cpp
  src/NetBox/XmlStorage.cpp
  src/NetBox/Far3Storage.cpp
  src/NetBox/FarPluginStrings.cpp
  src/NetBox/MessageDlg.cpp
)

set(NetBox_HEADERS
  src/NetBox/FarUtils.h
  src/NetBox/FarPlugin.h
  src/NetBox/FarConfiguration.h
  src/NetBox/WinSCPPlugin.h
  src/NetBox/WinSCPFileSystem.h
  src/NetBox/FarDialog.h
  src/NetBox/XmlStorage.h
  src/NetBox/FarInterface.h
  src/NetBox/resource.h
  src/NetBox/FarPluginStrings.h
)

set(NETBOX_SOURCES
#  UnityBuildCore.cpp
  # ${Base_HEADERS}
  # ${Base_SOURCES}

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

#-------------------------------------------------------------------------------
#baselib

add_library(baselib STATIC
  ${Base_HEADERS}
  ${Base_SOURCES}
)

target_include_directories(baselib PRIVATE
  libs
  libs/fmt
  libs/tinylog
  libs/GSL/include
  src/include
  src/base
  src/resource
)

target_compile_options(baselib PRIVATE
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
  ${NETBOX_PLATFORM_FLAGS}
)

set(NETBOX_LIBRARIES
  baselib
)

endif(OPT_USE_UNITY_BUILD)

set(NETBOX_SOURCES
  ${NETBOX_SOURCES}
  ${NetBox_HEADERS}

  src/include/FastDelegateBind.h
  src/include/FastDelegate.h
  src/include/type_traits.h
  src/include/rtti.hpp
  src/include/nbtypes.h
  src/include/nbglobals.h
  src/include/nbsystem.h
  src/include/nbcore.h
  src/include/nbstring.h
  src/include/nbstring.inl
  src/include/nbutils.h

  src/base/Property.hpp
  src/base/Global.h
  src/base/Common.h
  src/base/UnicodeString.hpp
  src/base/Exceptions.h
  src/base/Sysutils.hpp
  src/base/StrUtils.hpp
  src/base/MsgIDs.h
  src/base/vcl.h
  src/base/FileBuffer.h
  src/base/System.IOUtils.hpp
  src/base/FormatUtils.h
  src/base/ObjIDs.h

  src/core/WebDAVFileSystem.h
  src/core/S3FileSystem.h
  src/core/Queue.h
  src/core/Terminal.h
  src/core/FileSystems.h
  src/core/SessionInfo.h
  src/core/SessionData.h
  src/core/RemoteFiles.h
  src/core/Http.h
  src/core/FtpFileSystem.h
  src/core/FileMasks.h
  src/core/Configuration.h
  src/core/WinSCPSecurity.h
  src/core/SecureShell.h
  src/core/ScpFileSystem.h
  src/core/NeonIntf.h
  src/core/Interface.h
  src/core/FileInfo.h
  src/core/CopyParam.h
  src/core/SftpFileSystem.h
  src/core/PuttyIntf.h
  src/core/Option.h
  src/core/HierarchicalStorage.h
  src/core/Cryptography.h
  src/core/PuttyTools.h
  src/core/NamedObjs.h
  src/core/FileOperationProgress.h
  src/core/Bookmarks.h
  src/core/CoreMain.h

  src/windows/WinInterface.h
  src/windows/GUITools.h
  src/windows/GUIConfiguration.h
  src/windows/Tools.h
  src/windows/ProgParams.h
  src/windows/SynchronizeController.h

  src/resource/TextsCore.h
  src/resource/rtlconsts.h
  src/resource/HelpCore.h
  src/resource/TextsFileZilla.h
  src/PluginSDK/Far3/plugin.hpp
  src/PluginSDK/Far3/farcolor.hpp
  src/PluginSDK/Far3/PluginSettings.hpp

  src/resource/TextsFileZilla.rc
  src/resource/TextsCore1.rc
  src/resource/rtlconsts.rc
  src/resource/TextsWin1.rc
  src/NetBox/NetBox.rc
  src/NetBox/NetBox.def
)

add_custom_target(NetBoxResources SOURCES
  src/NetBox/NetBoxEng.lng
  src/NetBox/NetBoxRus.lng
  src/NetBox/NetBoxPol.lng
  src/NetBox/NetBoxSpa.lng
  src/NetBox/resource.h.template
  src/NetBox/NetBox.rc
  src/NetBox/NetBox.rc.template
  .github/workflows/release.yml
  appveyor.yml
  appveyor.bat
  ChangeLog
  README.md
)

set(NETBOX_LIBRARIES ${NETBOX_LIBRARIES}
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
  fmt
  s3
  tinylog

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
set(NETBOX_LIBRARIES ${NETBOX_LIBRARIES}
  debug libcmtd.lib
  debug libcpmtd.lib
  optimized libcmt.lib
  optimized libcpmt.lib
)
endif(MSVC)

add_library(NetBox SHARED
  ${NETBOX_SOURCES}
)

target_include_directories(NetBox PRIVATE
  libs
  libs/openssl-3/include
  # libs/openssl-3/providers/common/include
  libs/putty
  libs/putty/windows
  libs/neon/src
  libs/tinyxml2
  libs/expat/lib
  libs/atlmfc/include
  libs/fmt
  libs/tinylog
  libs/libs3/inc
  libs/GSL/include
  src/include
  src/filezilla
  src/base
  src/core
  src/windows
  src/resource
  src/include
  src/PluginSDK/Far3
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
  ${NETBOX_PLATFORM_FLAGS}
  -DGSL_THROW_ON_CONTRACT_VIOLATION
  -DNOMINMAX
  -D_CRTIMP=
)

if(OPT_CREATE_PLUGIN_DIR)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
string(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})
set(NETBOX_PLUGIN_DIR ${PROJECT_ROOT}/Far3_${PROJECT_PLATFORM}/Plugins/NetBox)

file(GLOB DIST_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/NetBox/*.lng
  ${CMAKE_CURRENT_SOURCE_DIR}/src/NetBox/cacert.pem
  ${CMAKE_CURRENT_SOURCE_DIR}/README*.md
  ${CMAKE_CURRENT_SOURCE_DIR}/ChangeLog
  ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt
)

set(NB_TARGET_FILES
  NetBox.dll
  NetBox.pdb
  NetBox.map
)

foreach(NB_TARGET ${NB_TARGET_FILES})
  add_custom_command(TARGET NetBox POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    "-DFILE_NAME=${NB_TARGET}"
    "-DFULL_FILE_NAME=$<TARGET_FILE_DIR:NetBox>//${NB_TARGET}"
    "-DNETBOX_PLUGIN_DIR=${NETBOX_PLUGIN_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_file.cmake"
    VERBATIM
  )
endforeach()

foreach(DIST_FILE ${DIST_FILES})
  get_filename_component(FILE_NAME ${DIST_FILE} NAME)
  add_custom_command(TARGET NetBox POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    "-DFILE_NAME=${FILE_NAME}"
    "-DFULL_FILE_NAME=${DIST_FILE}"
    "-DNETBOX_PLUGIN_DIR=${NETBOX_PLUGIN_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_file.cmake"
  )
endforeach()

endif(OPT_CREATE_PLUGIN_DIR)

#-------------------------------------------------------------------------------
ucm_set_runtime(STATIC)
ucm_add_flags(CXX /EHs)
# /O1 - minimize size,  implies /Og /Oy /Ob2 /GF /Gy /Os
# /O2 - maximize speed, implies /Og /Oy /Ob2 /GF /Gy /Oi /Ot
# /Gw - Enables whole program global data optimization
# /GL - Enables whole program optimization
ucm_remove_flags(CXX C /Ob1 /Ob2 CONFIG Release MinSizeRel RelWithDebInfo)
ucm_add_flags(CXX C /fp:fast /Gw /GL CONFIG Release RelWithDebInfo MinSizeRel)
ucm_add_flags(CXX C /RTC1 /DDEBUG CONFIG Debug)
# ucm_add_flags(CXX /fsanitize=address CONFIG Debug) #/sdl
ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_RELEASE} CONFIG Release RelWithDebInfo MinSizeRel)
ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_DEBUG} CONFIG Debug)
ucm_add_linker_flags(MODULE SHARED /DEBUG /MAP CONFIG RelWithDebInfo)
#ucm_print_flags()
#-------------------------------------------------------------------------------
