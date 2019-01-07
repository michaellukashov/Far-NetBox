message(STATUS "CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 2.8)

set(OPT_BUILD_OPENSSL ON CACHE STRING "Default build openSSL")
set(OPT_USE_UNITY_BUILD ON CACHE STRING "Default use unity build")

project(NetBox)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type to Debug" FORCE)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")
message("CMAKE_CL_64: ${CMAKE_CL_64}")
message("MSVC10: ${MSVC10}")

if(NOT DEFINED FAR_VERSION OR FAR_VERSION STREQUAL "")
  set(FAR_VERSION Far2)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(x64 1)
  set(PROJECT_PLATFORM "x64")
else()
  set(x64 0)
  set(PROJECT_PLATFORM "x86")
endif()

if(MSVC)
  if(MSVC14)
    set(OPENSSL_LIB_DIR "${PROJECT_PLATFORM}-${CMAKE_BUILD_TYPE}")
  else()
    set(OPENSSL_LIB_DIR "${PROJECT_PLATFORM}-${CMAKE_BUILD_TYPE}")
  endif()
else()
  set(OPENSSL_LIB_DIR "${PROJECT_PLATFORM}-${CMAKE_BUILD_TYPE}")
endif(MSVC)
#message("OPENSSL_LIB_DIR: ${OPENSSL_LIB_DIR}")

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

#-------------------------------------------------------------------------------

include(${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake)
#include(CMakeDetermineCXXCompiler)
message("System: ${CMAKE_SYSTEM_NAME}, PROJECT_PLATFORM: ${PROJECT_PLATFORM}, BUILD_DIR: ${BUILD_DIR}, CMAKE_COMPILER_IS_MINGW = ${CMAKE_COMPILER_IS_MINGW}")
message("MSVC_VERSION: ${MSVC_VERSION}")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

#-------------------------------------------------------------------------------

set(LIBNEON_DEFS "-DNE_LFS -DNE_HAVE_SSL -DHAVE_OPENSSL -DHAVE_EXPAT -DHAVE_EXPAT_H -DNE_HAVE_DAV -DNE_HAVE_ZLIB") #" -DUSE_GETADDRINFO")
set(LIBEXPAT_DEFS "-DCOMPILED_FROM_DSP -DXML_STATIC")
set(DLMALLOC_DEFS "-DUSE_DLMALLOC -DUSE_DL_PREFIX -DNO_MALLINFO -DUSE_LOCKS=1")
set(NETBOX_DEFS "-DMPEXT -DWINSCP -DFARPLUGIN")
set(NETBOX_DEFS "${NETBOX_DEFS} -DNO_GZIP -DSTRICT -DNOCRYPT -DOPENSSL_NO_LOCKING")
set(NETBOX_DEFS "${NETBOX_DEFS} -D_CRTIMP= -D_LIB -D_WINDOWS -DWIN32 -D_WIN32_WINNT=0x0501")
set(NETBOX_DEFS "${NETBOX_DEFS} -D_SCL_SECURE_NO_WARNINGS  -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS")
set(NETBOX_DEFS "${NETBOX_DEFS} -DCOM_NO_WINDOWS_H")
set(NETBOX_DEFS "${NETBOX_DEFS} ${LIBEXPAT_DEFS} ${LIBNEON_DEFS} ${DLMALLOC_DEFS}")
#-D_WINDLL -D_USRDLL
if (MSVC_VERSION GREATER 1899)
set(NETBOX_DEFS "${NETBOX_DEFS} /Zc:threadSafeInit- /bigobj")
endif()

set(NETBOX_C_FLAGS "")

if(x64)
set(NETBOX_PLATFORM_FLAGS "${NETBOX_PLATFORM_FLAGS} -DWIN64")
else()
if (MSVC_VERSION GREATER 1699)
set(NETBOX_PLATFORM_FLAGS "${NETBOX_PLATFORM_FLAGS} /arch:IA32")
endif()
endif()

if(MSVC)
set(NETBOX_C_FLAGS "${NETBOX_C_FLAGS} /TC")
elseif(CMAKE_COMPILER_IS_MINGW)
set(NETBOX_C_FLAGS "${NETBOX_C_FLAGS} -x c --std=c99")
endif()

set(NETBOX_FLAGS_RELEASE "-U_DEBUG -DNDEBUG")
set(NETBOX_FLAGS_DEBUG "-D_DEBUG -DDEBUG")

set(NETBOX_UNICODE_FLAGS -D_UNICODE -DUNICODE)

if(MSVC)

set(NETBOX_C_WARNING_FLAGS "")
set(NETBOX_CXX_WARNING_FLAGS "")
set(NETBOX_CXX_FLAGS "/MP2 /Zi")
set(NETBOX_FLAGS_RELEASE "${NETBOX_FLAGS_RELEASE} /Gm- /MT -Ox -Ob1 -Oi -Os -GF -GS- -Gy /fp:fast")
set(NETBOX_FLAGS_DEBUG "${NETBOX_FLAGS_DEBUG} /Gm- /Od /MTd /GS /RTC1")

elseif(CMAKE_COMPILER_IS_MINGW)

set(NETBOX_C_WARNING_FLAGS "-Wno-unknown-pragmas")
set(NETBOX_CXX_WARNING_FLAGS "${NETBOX_C_WARNING_FLAGS} -Wno-attributes -Wno-write-strings -Wno-old-style-cast -Wno-effc++ -Wno-unknown-pragmas -Wno-unused-value")
set(NETBOX_CXX_FLAGS "${NETBOX_CXX_FLAGS} -std=c++11 -Woverloaded-virtual -Wnon-virtual-dtor")
set(NETBOX_C_FLAGS "${NETBOX_C_FLAGS} -static -s -O2 -Wall -Wextra -pedantic -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code")
#-Weffc++
set(NETBOX_FLAGS_RELEASE "${NETBOX_FLAGS_RELEASE}")
set(NETBOX_FLAGS_DEBUG "${NETBOX_FLAGS_DEBUG}")

endif()

set(NETBOX_C_FLAGS "${NETBOX_DEFS} ${NETBOX_C_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_C_WARNING_FLAGS}")
set(NETBOX_CXX_FLAGS "${NETBOX_DEFS} ${NETBOX_CXX_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_CXX_WARNING_FLAGS}")

set(CMAKE_CXX_FLAGS_RELEASE "${NETBOX_CXX_FLAGS} ${NETBOX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELEASE "${NETBOX_C_FLAGS} ${NETBOX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${NETBOX_CXX_FLAGS} ${NETBOX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG "${NETBOX_C_FLAGS} ${NETBOX_FLAGS_DEBUG}")

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

nb_replace_compiler_option(CMAKE_CXX_FLAGS "/GR-?" "/GR-")
#nb_replace_compiler_option(CMAKE_CXX_FLAGS "/MD[d]?" "")
nb_replace_compiler_option(CMAKE_CXX_FLAGS "/EH[-cs]+" "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
#string(REPLACE "/MD" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(NETBOX_DLL_LINK_FLAGS
  "
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
  /MANIFEST:NO
  /TLBID:1 /DYNAMICBASE /SUBSYSTEM:WINDOWS /NOLOGO
  /MAP
  /INCREMENTAL:NO
  /ignore:4099
  /ignore:4197
  /ignore:4199
"
)
#/DELAYLOAD:gdi32.dll

set(NETBOX_DLL_LINK_FLAGS_RELEASE "/OPT:REF")
set(NETBOX_DLL_LINK_FLAGS_DEBUG "/DEBUG")

elseif(CMAKE_COMPILER_IS_MINGW)
set(NETBOX_DLL_LINK_FLAGS "")
set(NETBOX_DLL_LINK_FLAGS_RELEASE "${NETBOX_DLL_LINK_FLAGS}")
set(NETBOX_DLL_LINK_FLAGS_DEBUG "${NETBOX_DLL_LINK_FLAGS}")
endif()

include_directories(
  ../../libs
  ../include
)

#-------------------------------------------------------------------------------
#openssl

if(OPT_BUILD_OPENSSL)

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
  -DOPENSSLDIR="" -DENGINESDIR=""
  -DNB_COMPILE_OPENSSL
)
if(MSVC)
set(OPENSSL_COMPILE_FLAGS ${OPENSSL_COMPILE_FLAGS}
  /Zi /Zl
  -wd4090
)
endif()

add_library(libcrypto STATIC
  ../../libs/openssl/engines/e_capi.c
  ../../libs/openssl/engines/e_padlock.c
  ../../libs/openssl/engines/e_ubsec.c
  ../../libs/openssl/engines/e_sureware.c
  ../../libs/openssl/engines/e_nuron.c
  ../../libs/openssl/engines/e_chil.c
  ../../libs/openssl/engines/e_gmp.c
  ../../libs/openssl/engines/e_cswift.c
  ../../libs/openssl/engines/e_atalla.c
  ../../libs/openssl/engines/e_aep.c
  ../../libs/openssl/engines/e_4758cca.c
  ../../libs/openssl/crypto/ts/ts_asn1.c
  ../../libs/openssl/crypto/ts/ts_conf.c
  ../../libs/openssl/crypto/ts/ts_lib.c
  ../../libs/openssl/crypto/ts/ts_verify_ctx.c
  ../../libs/openssl/crypto/ts/ts_rsp_verify.c
  ../../libs/openssl/crypto/ts/ts_rsp_sign.c
  ../../libs/openssl/crypto/ts/ts_rsp_print.c
  ../../libs/openssl/crypto/ts/ts_rsp_utils.c
  ../../libs/openssl/crypto/ts/ts_req_print.c
  ../../libs/openssl/crypto/ts/ts_req_utils.c
  ../../libs/openssl/crypto/ts/ts_err.c
  ../../libs/openssl/crypto/pqueue/pqueue.c
  ../../libs/openssl/crypto/krb5/krb5_asn.c
  ../../libs/openssl/crypto/ui/ui_compat.c
  ../../libs/openssl/crypto/ui/ui_util.c
  ../../libs/openssl/crypto/ui/ui_openssl.c
  ../../libs/openssl/crypto/ui/ui_lib.c
  ../../libs/openssl/crypto/ui/ui_err.c
  ../../libs/openssl/crypto/ocsp/ocsp_err.c
  ../../libs/openssl/crypto/ocsp/ocsp_vfy.c
  ../../libs/openssl/crypto/ocsp/ocsp_prn.c
  ../../libs/openssl/crypto/ocsp/ocsp_srv.c
  ../../libs/openssl/crypto/ocsp/ocsp_cl.c
  ../../libs/openssl/crypto/ocsp/ocsp_lib.c
  ../../libs/openssl/crypto/ocsp/ocsp_ht.c
  ../../libs/openssl/crypto/ocsp/ocsp_ext.c
  ../../libs/openssl/crypto/ocsp/ocsp_asn.c
  ../../libs/openssl/crypto/engine/eng_rdrand.c
  ../../libs/openssl/crypto/engine/eng_cryptodev.c
  ../../libs/openssl/crypto/engine/eng_dyn.c
  ../../libs/openssl/crypto/engine/eng_cnf.c
  ../../libs/openssl/crypto/engine/eng_openssl.c
  ../../libs/openssl/crypto/engine/tb_asnmth.c
  ../../libs/openssl/crypto/engine/tb_pkmeth.c
  ../../libs/openssl/crypto/engine/tb_digest.c
  ../../libs/openssl/crypto/engine/tb_cipher.c
  ../../libs/openssl/crypto/engine/tb_store.c
  ../../libs/openssl/crypto/engine/tb_rand.c
  ../../libs/openssl/crypto/engine/tb_ecdh.c
  ../../libs/openssl/crypto/engine/tb_dh.c
  ../../libs/openssl/crypto/engine/tb_ecdsa.c
  ../../libs/openssl/crypto/engine/tb_dsa.c
  ../../libs/openssl/crypto/engine/tb_rsa.c
  ../../libs/openssl/crypto/engine/eng_all.c
  ../../libs/openssl/crypto/engine/eng_fat.c
  ../../libs/openssl/crypto/engine/eng_pkey.c
  ../../libs/openssl/crypto/engine/eng_table.c
  ../../libs/openssl/crypto/engine/eng_ctrl.c
  ../../libs/openssl/crypto/engine/eng_init.c
  ../../libs/openssl/crypto/engine/eng_list.c
  ../../libs/openssl/crypto/engine/eng_lib.c
  ../../libs/openssl/crypto/engine/eng_err.c
  ../../libs/openssl/crypto/comp/c_zlib.c
  ../../libs/openssl/crypto/comp/c_rle.c
  ../../libs/openssl/crypto/comp/comp_err.c
  ../../libs/openssl/crypto/comp/comp_lib.c
  ../../libs/openssl/crypto/pkcs12/p12_p8e.c
  ../../libs/openssl/crypto/pkcs12/p12_p8d.c
  ../../libs/openssl/crypto/pkcs12/pk12err.c
  ../../libs/openssl/crypto/pkcs12/p12_npas.c
  ../../libs/openssl/crypto/pkcs12/p12_utl.c
  ../../libs/openssl/crypto/pkcs12/p12_mutl.c
  ../../libs/openssl/crypto/pkcs12/p12_kiss.c
  ../../libs/openssl/crypto/pkcs12/p12_key.c
  ../../libs/openssl/crypto/pkcs12/p12_init.c
  ../../libs/openssl/crypto/pkcs12/p12_decr.c
  ../../libs/openssl/crypto/pkcs12/p12_crt.c
  ../../libs/openssl/crypto/pkcs12/p12_crpt.c
  ../../libs/openssl/crypto/pkcs12/p12_attr.c
  ../../libs/openssl/crypto/pkcs12/p12_asn.c
  ../../libs/openssl/crypto/pkcs12/p12_add.c
  ../../libs/openssl/crypto/pkcs7/bio_pk7.c
  ../../libs/openssl/crypto/pkcs7/pk7_mime.c
  ../../libs/openssl/crypto/pkcs7/pk7_attr.c
  ../../libs/openssl/crypto/pkcs7/pk7_smime.c
  ../../libs/openssl/crypto/pkcs7/pk7_doit.c
  ../../libs/openssl/crypto/pkcs7/pkcs7err.c
  ../../libs/openssl/crypto/pkcs7/pk7_lib.c
  ../../libs/openssl/crypto/pkcs7/pk7_asn1.c
  ../../libs/openssl/crypto/txt_db/txt_db.c
  ../../libs/openssl/crypto/conf/conf_sap.c
  ../../libs/openssl/crypto/conf/conf_mall.c
  ../../libs/openssl/crypto/conf/conf_mod.c
  ../../libs/openssl/crypto/conf/conf_def.c
  ../../libs/openssl/crypto/conf/conf_api.c
  ../../libs/openssl/crypto/conf/conf_lib.c
  ../../libs/openssl/crypto/conf/conf_err.c
  ../../libs/openssl/crypto/cms/cms_kari.c
  ../../libs/openssl/crypto/cms/cms_pwri.c
  ../../libs/openssl/crypto/cms/cms_ess.c
  ../../libs/openssl/crypto/cms/cms_enc.c
  ../../libs/openssl/crypto/cms/cms_env.c
  ../../libs/openssl/crypto/cms/cms_cd.c
  ../../libs/openssl/crypto/cms/cms_dd.c
  ../../libs/openssl/crypto/cms/cms_sd.c
  ../../libs/openssl/crypto/cms/cms_err.c
  ../../libs/openssl/crypto/cms/cms_smime.c
  ../../libs/openssl/crypto/cms/cms_io.c
  ../../libs/openssl/crypto/cms/cms_att.c
  ../../libs/openssl/crypto/cms/cms_asn1.c
  ../../libs/openssl/crypto/cms/cms_lib.c
  ../../libs/openssl/crypto/x509v3/v3_scts.c
  ../../libs/openssl/crypto/x509v3/v3_addr.c
  ../../libs/openssl/crypto/x509v3/v3_asid.c
  ../../libs/openssl/crypto/x509v3/pcy_lib.c
  ../../libs/openssl/crypto/x509v3/pcy_tree.c
  ../../libs/openssl/crypto/x509v3/pcy_map.c
  ../../libs/openssl/crypto/x509v3/pcy_data.c
  ../../libs/openssl/crypto/x509v3/pcy_node.c
  ../../libs/openssl/crypto/x509v3/pcy_cache.c
  ../../libs/openssl/crypto/x509v3/v3_pci.c
  ../../libs/openssl/crypto/x509v3/v3_pcia.c
  ../../libs/openssl/crypto/x509v3/v3_ncons.c
  ../../libs/openssl/crypto/x509v3/v3_pcons.c
  ../../libs/openssl/crypto/x509v3/v3_pmaps.c
  ../../libs/openssl/crypto/x509v3/v3_akeya.c
  ../../libs/openssl/crypto/x509v3/v3_ocsp.c
  ../../libs/openssl/crypto/x509v3/v3_info.c
  ../../libs/openssl/crypto/x509v3/v3_purp.c
  ../../libs/openssl/crypto/x509v3/v3_crld.c
  ../../libs/openssl/crypto/x509v3/v3_cpols.c
  ../../libs/openssl/crypto/x509v3/v3_sxnet.c
  ../../libs/openssl/crypto/x509v3/v3_enum.c
  ../../libs/openssl/crypto/x509v3/v3_int.c
  ../../libs/openssl/crypto/x509v3/v3_pku.c
  ../../libs/openssl/crypto/x509v3/v3_akey.c
  ../../libs/openssl/crypto/x509v3/v3_skey.c
  ../../libs/openssl/crypto/x509v3/v3_alt.c
  ../../libs/openssl/crypto/x509v3/v3_genn.c
  ../../libs/openssl/crypto/x509v3/v3err.c
  ../../libs/openssl/crypto/x509v3/v3_utl.c
  ../../libs/openssl/crypto/x509v3/v3_prn.c
  ../../libs/openssl/crypto/x509v3/v3_lib.c
  ../../libs/openssl/crypto/x509v3/v3_ia5.c
  ../../libs/openssl/crypto/x509v3/v3_extku.c
  ../../libs/openssl/crypto/x509v3/v3_conf.c
  ../../libs/openssl/crypto/x509v3/v3_bitst.c
  ../../libs/openssl/crypto/x509v3/v3_bcons.c
  ../../libs/openssl/crypto/x509/x509_vpm.c
  ../../libs/openssl/crypto/x509/by_dir.c
  ../../libs/openssl/crypto/x509/by_file.c
  ../../libs/openssl/crypto/x509/x509_trs.c
  ../../libs/openssl/crypto/x509/x509_txt.c
  ../../libs/openssl/crypto/x509/x_all.c
  ../../libs/openssl/crypto/x509/x509_lu.c
  ../../libs/openssl/crypto/x509/x509type.c
  ../../libs/openssl/crypto/x509/x509_att.c
  ../../libs/openssl/crypto/x509/x509_ext.c
  ../../libs/openssl/crypto/x509/x509_v3.c
  ../../libs/openssl/crypto/x509/x509name.c
  ../../libs/openssl/crypto/x509/x509_err.c
  ../../libs/openssl/crypto/x509/x509rset.c
  ../../libs/openssl/crypto/x509/x509cset.c
  ../../libs/openssl/crypto/x509/x509_set.c
  ../../libs/openssl/crypto/x509/x509_vfy.c
  ../../libs/openssl/crypto/x509/x509spki.c
  ../../libs/openssl/crypto/x509/x509_req.c
  ../../libs/openssl/crypto/x509/x509_obj.c
  ../../libs/openssl/crypto/x509/x509_cmp.c
  ../../libs/openssl/crypto/x509/x509_r2x.c
  ../../libs/openssl/crypto/x509/x509_d2.c
  ../../libs/openssl/crypto/x509/x509_def.c
  ../../libs/openssl/crypto/pem/pvkfmt.c
  ../../libs/openssl/crypto/pem/pem_pkey.c
  ../../libs/openssl/crypto/pem/pem_pk8.c
  ../../libs/openssl/crypto/pem/pem_oth.c
  ../../libs/openssl/crypto/pem/pem_xaux.c
  ../../libs/openssl/crypto/pem/pem_x509.c
  ../../libs/openssl/crypto/pem/pem_err.c
  ../../libs/openssl/crypto/pem/pem_all.c
  ../../libs/openssl/crypto/pem/pem_lib.c
  ../../libs/openssl/crypto/pem/pem_info.c
  ../../libs/openssl/crypto/pem/pem_seal.c
  ../../libs/openssl/crypto/pem/pem_sign.c
  ../../libs/openssl/crypto/asn1/asn_moid.c
  ../../libs/openssl/crypto/asn1/p8_pkey.c
  ../../libs/openssl/crypto/asn1/p5_pbev2.c
  ../../libs/openssl/crypto/asn1/p5_pbe.c
  ../../libs/openssl/crypto/asn1/asn_pack.c
  ../../libs/openssl/crypto/asn1/evp_asn1.c
  ../../libs/openssl/crypto/asn1/a_strnid.c
  ../../libs/openssl/crypto/asn1/a_bytes.c
  ../../libs/openssl/crypto/asn1/asn1_err.c
  ../../libs/openssl/crypto/asn1/asn1_lib.c
  ../../libs/openssl/crypto/asn1/asn1_par.c
  ../../libs/openssl/crypto/asn1/asn1_gen.c
  ../../libs/openssl/crypto/asn1/asn_mime.c
  ../../libs/openssl/crypto/asn1/bio_ndef.c
  ../../libs/openssl/crypto/asn1/bio_asn1.c
  ../../libs/openssl/crypto/asn1/x_exten.c
  ../../libs/openssl/crypto/asn1/a_bool.c
  ../../libs/openssl/crypto/asn1/x_pkey.c
  ../../libs/openssl/crypto/asn1/f_enum.c
  ../../libs/openssl/crypto/asn1/f_string.c
  ../../libs/openssl/crypto/asn1/f_int.c
  ../../libs/openssl/crypto/asn1/ameth_lib.c
  ../../libs/openssl/crypto/asn1/tasn_prn.c
  ../../libs/openssl/crypto/asn1/tasn_typ.c
  ../../libs/openssl/crypto/asn1/tasn_utl.c
  ../../libs/openssl/crypto/asn1/tasn_dec.c
  ../../libs/openssl/crypto/asn1/tasn_enc.c
  ../../libs/openssl/crypto/asn1/tasn_fre.c
  ../../libs/openssl/crypto/asn1/tasn_new.c
  ../../libs/openssl/crypto/asn1/t_bitst.c
  ../../libs/openssl/crypto/asn1/t_spki.c
  ../../libs/openssl/crypto/asn1/t_pkey.c
  ../../libs/openssl/crypto/asn1/t_crl.c
  ../../libs/openssl/crypto/asn1/t_x509a.c
  ../../libs/openssl/crypto/asn1/t_x509.c
  ../../libs/openssl/crypto/asn1/t_req.c
  ../../libs/openssl/crypto/asn1/i2d_pr.c
  ../../libs/openssl/crypto/asn1/i2d_pu.c
  ../../libs/openssl/crypto/asn1/d2i_pr.c
  ../../libs/openssl/crypto/asn1/d2i_pu.c
  ../../libs/openssl/crypto/asn1/x_nx509.c
  ../../libs/openssl/crypto/asn1/nsseq.c
  ../../libs/openssl/crypto/asn1/x_spki.c
  ../../libs/openssl/crypto/asn1/x_info.c
  ../../libs/openssl/crypto/asn1/x_crl.c
  ../../libs/openssl/crypto/asn1/x_x509a.c
  ../../libs/openssl/crypto/asn1/x_x509.c
  ../../libs/openssl/crypto/asn1/x_name.c
  ../../libs/openssl/crypto/asn1/x_long.c
  ../../libs/openssl/crypto/asn1/x_bignum.c
  ../../libs/openssl/crypto/asn1/x_attrib.c
  ../../libs/openssl/crypto/asn1/x_req.c
  ../../libs/openssl/crypto/asn1/x_sig.c
  ../../libs/openssl/crypto/asn1/x_pubkey.c
  ../../libs/openssl/crypto/asn1/x_val.c
  ../../libs/openssl/crypto/asn1/x_algor.c
  ../../libs/openssl/crypto/asn1/a_strex.c
  ../../libs/openssl/crypto/asn1/a_mbstr.c
  ../../libs/openssl/crypto/asn1/a_verify.c
  ../../libs/openssl/crypto/asn1/a_digest.c
  ../../libs/openssl/crypto/asn1/a_sign.c
  ../../libs/openssl/crypto/asn1/a_utf8.c
  ../../libs/openssl/crypto/asn1/a_enum.c
  ../../libs/openssl/crypto/asn1/a_i2d_fp.c
  ../../libs/openssl/crypto/asn1/a_d2i_fp.c
  ../../libs/openssl/crypto/asn1/a_dup.c
  ../../libs/openssl/crypto/asn1/a_set.c
  ../../libs/openssl/crypto/asn1/a_type.c
  ../../libs/openssl/crypto/asn1/a_print.c
  ../../libs/openssl/crypto/asn1/a_octet.c
  ../../libs/openssl/crypto/asn1/a_int.c
  ../../libs/openssl/crypto/asn1/a_time.c
  ../../libs/openssl/crypto/asn1/a_gentm.c
  ../../libs/openssl/crypto/asn1/a_utctm.c
  ../../libs/openssl/crypto/asn1/a_bitstr.c
  ../../libs/openssl/crypto/asn1/a_object.c
  ../../libs/openssl/crypto/evp/e_rc4_hmac_md5.c
  ../../libs/openssl/crypto/evp/e_aes_cbc_hmac_sha256.c
  ../../libs/openssl/crypto/evp/e_aes_cbc_hmac_sha1.c
  ../../libs/openssl/crypto/evp/m_sigver.c
  ../../libs/openssl/crypto/evp/pmeth_gn.c
  ../../libs/openssl/crypto/evp/pmeth_fn.c
  ../../libs/openssl/crypto/evp/pmeth_lib.c
  ../../libs/openssl/crypto/evp/e_old.c
  ../../libs/openssl/crypto/evp/p5_crpt2.c
  ../../libs/openssl/crypto/evp/p5_crpt.c
  ../../libs/openssl/crypto/evp/evp_pbe.c
  ../../libs/openssl/crypto/evp/evp_pkey.c
  ../../libs/openssl/crypto/evp/bio_ok.c
  ../../libs/openssl/crypto/evp/evp_lib.c
  ../../libs/openssl/crypto/evp/c_alld.c
  ../../libs/openssl/crypto/evp/c_allc.c
  ../../libs/openssl/crypto/evp/c_all.c
  ../../libs/openssl/crypto/evp/e_null.c
  ../../libs/openssl/crypto/evp/evp_err.c
  ../../libs/openssl/crypto/evp/bio_enc.c
  ../../libs/openssl/crypto/evp/bio_b64.c
  ../../libs/openssl/crypto/evp/bio_md.c
  ../../libs/openssl/crypto/evp/p_dec.c
  ../../libs/openssl/crypto/evp/p_enc.c
  ../../libs/openssl/crypto/evp/p_lib.c
  ../../libs/openssl/crypto/evp/p_verify.c
  ../../libs/openssl/crypto/evp/p_sign.c
  ../../libs/openssl/crypto/evp/p_seal.c
  ../../libs/openssl/crypto/evp/p_open.c
  ../../libs/openssl/crypto/evp/m_ecdsa.c
  ../../libs/openssl/crypto/evp/m_mdc2.c
  ../../libs/openssl/crypto/evp/m_dss1.c
  ../../libs/openssl/crypto/evp/m_dss.c
  ../../libs/openssl/crypto/evp/m_wp.c
  ../../libs/openssl/crypto/evp/m_sha1.c
  ../../libs/openssl/crypto/evp/m_sha.c
  ../../libs/openssl/crypto/evp/m_md5.c
  ../../libs/openssl/crypto/evp/m_md4.c
  ../../libs/openssl/crypto/evp/m_null.c
  ../../libs/openssl/crypto/evp/e_rc5.c
  ../../libs/openssl/crypto/evp/e_cast.c
  ../../libs/openssl/crypto/evp/e_rc2.c
  ../../libs/openssl/crypto/evp/e_xcbc_d.c
  ../../libs/openssl/crypto/evp/names.c
  ../../libs/openssl/crypto/evp/e_aes.c
  ../../libs/openssl/crypto/evp/e_des3.c
  ../../libs/openssl/crypto/evp/e_idea.c
  ../../libs/openssl/crypto/evp/e_bf.c
  ../../libs/openssl/crypto/evp/e_des.c
  ../../libs/openssl/crypto/evp/evp_cnf.c
  ../../libs/openssl/crypto/evp/evp_acnf.c
  ../../libs/openssl/crypto/evp/evp_key.c
  ../../libs/openssl/crypto/evp/evp_enc.c
  ../../libs/openssl/crypto/evp/digest.c
  ../../libs/openssl/crypto/evp/digest.c
  ../../libs/openssl/crypto/evp/encode.c
  ../../libs/openssl/crypto/evp/m_ripemd.c
  ../../libs/openssl/crypto/ripemd/rmd_one.c
  ../../libs/openssl/crypto/ripemd/rmd_dgst.c
  ../../libs/openssl/crypto/ripemd/rmd160.c
#  ../../libs/openssl/crypto/ripemd/rmdtest.c
  ../../libs/openssl/crypto/objects/obj_xref.c
  ../../libs/openssl/crypto/objects/obj_err.c
  ../../libs/openssl/crypto/objects/obj_lib.c
  ../../libs/openssl/crypto/objects/obj_dat.c
  ../../libs/openssl/crypto/objects/o_names.c
  ../../libs/openssl/crypto/err/err_prn.c
  ../../libs/openssl/crypto/err/err_all.c
  ../../libs/openssl/crypto/err/err.c
  ../../libs/openssl/crypto/rand/rand_nw.c
  ../../libs/openssl/crypto/rand/rand_os2.c
  ../../libs/openssl/crypto/rand/rand_unix.c
  ../../libs/openssl/crypto/rand/rand_win.c
  ../../libs/openssl/crypto/rand/rand_egd.c
  ../../libs/openssl/crypto/rand/rand_err.c
  ../../libs/openssl/crypto/rand/rand_lib.c
  ../../libs/openssl/crypto/rand/randfile.c
  ../../libs/openssl/crypto/rand/md_rand.c
  ../../libs/openssl/crypto/lhash/lh_stats.c
  ../../libs/openssl/crypto/lhash/lhash.c
  ../../libs/openssl/crypto/stack/stack.c
  ../../libs/openssl/crypto/bio/bss_dgram.c
  ../../libs/openssl/crypto/bio/bss_bio.c
  ../../libs/openssl/crypto/bio/bss_log.c
  ../../libs/openssl/crypto/bio/bf_nbio.c
  ../../libs/openssl/crypto/bio/bss_acpt.c
  ../../libs/openssl/crypto/bio/b_sock.c
  ../../libs/openssl/crypto/bio/b_dump.c
  ../../libs/openssl/crypto/bio/b_print.c
  ../../libs/openssl/crypto/bio/bf_buff.c
  ../../libs/openssl/crypto/bio/bf_null.c
  ../../libs/openssl/crypto/bio/bss_conn.c
  ../../libs/openssl/crypto/bio/bss_sock.c
  ../../libs/openssl/crypto/bio/bss_file.c
  ../../libs/openssl/crypto/bio/bss_fd.c
  ../../libs/openssl/crypto/bio/bss_null.c
  ../../libs/openssl/crypto/bio/bss_mem.c
  ../../libs/openssl/crypto/bio/bio_err.c
  ../../libs/openssl/crypto/bio/bio_cb.c
  ../../libs/openssl/crypto/bio/bio_lib.c
  ../../libs/openssl/crypto/buffer/buf_err.c
  ../../libs/openssl/crypto/buffer/buf_str.c
  ../../libs/openssl/crypto/buffer/buffer.c
  ../../libs/openssl/crypto/ecdsa/ecs_err.c
  ../../libs/openssl/crypto/ecdsa/ecs_vrf.c
  ../../libs/openssl/crypto/ecdsa/ecs_sign.c
  ../../libs/openssl/crypto/ecdsa/ecs_ossl.c
  ../../libs/openssl/crypto/ecdsa/ecs_asn1.c
  ../../libs/openssl/crypto/ecdsa/ecs_lib.c
  ../../libs/openssl/crypto/ecdh/ech_kdf.c
  ../../libs/openssl/crypto/ecdh/ech_err.c
  ../../libs/openssl/crypto/ecdh/ech_key.c
  ../../libs/openssl/crypto/ecdh/ech_ossl.c
  ../../libs/openssl/crypto/ecdh/ech_lib.c
  ../../libs/openssl/crypto/ec/ec_oct.c
  ../../libs/openssl/crypto/ec/ec2_oct.c
  ../../libs/openssl/crypto/ec/ecp_oct.c
#  ../../libs/openssl/crypto/ec/ecp_nistputil.c
#  ../../libs/openssl/crypto/ec/ecp_nistp521.c
#  ../../libs/openssl/crypto/ec/ecp_nistp256.c
#  ../../libs/openssl/crypto/ec/ecp_nistp224.c
  ../../libs/openssl/crypto/ec/eck_prn.c
  ../../libs/openssl/crypto/ec/ec_pmeth.c
  ../../libs/openssl/crypto/ec/ec_ameth.c
  ../../libs/openssl/crypto/ec/ec2_mult.c
  ../../libs/openssl/crypto/ec/ec2_smpl.c
  ../../libs/openssl/crypto/ec/ec_key.c
  ../../libs/openssl/crypto/ec/ec_asn1.c
  ../../libs/openssl/crypto/ec/ec_print.c
  ../../libs/openssl/crypto/ec/ec_check.c
  ../../libs/openssl/crypto/ec/ec_curve.c
  ../../libs/openssl/crypto/ec/ec_err.c
  ../../libs/openssl/crypto/ec/ec_mult.c
  ../../libs/openssl/crypto/ec/ec_cvt.c
  ../../libs/openssl/crypto/ec/ecp_nist.c
  ../../libs/openssl/crypto/ec/ecp_mont.c
  ../../libs/openssl/crypto/ec/ecp_smpl.c
  ../../libs/openssl/crypto/ec/ec_lib.c
  ../../libs/openssl/crypto/dh/dh_kdf.c
  ../../libs/openssl/crypto/dh/dh_rfc5114.c
  ../../libs/openssl/crypto/dh/dh_prn.c
  ../../libs/openssl/crypto/dh/dh_pmeth.c
  ../../libs/openssl/crypto/dh/dh_ameth.c
  ../../libs/openssl/crypto/dh/dh_depr.c
  ../../libs/openssl/crypto/dh/dh_err.c
  ../../libs/openssl/crypto/dh/dh_check.c
  ../../libs/openssl/crypto/dh/dh_lib.c
  ../../libs/openssl/crypto/dh/dh_key.c
  ../../libs/openssl/crypto/dh/dh_gen.c
  ../../libs/openssl/crypto/dh/dh_asn1.c
  ../../libs/openssl/crypto/dso/dso_beos.c
  ../../libs/openssl/crypto/dso/dso_vms.c
  ../../libs/openssl/crypto/dso/dso_win32.c
  ../../libs/openssl/crypto/dso/dso_openssl.c
  ../../libs/openssl/crypto/dso/dso_null.c
  ../../libs/openssl/crypto/dso/dso_lib.c
  ../../libs/openssl/crypto/dso/dso_err.c
  ../../libs/openssl/crypto/dso/dso_dlfcn.c
  ../../libs/openssl/crypto/dso/dso_dl.c
  ../../libs/openssl/crypto/dsa/dsa_prn.c
  ../../libs/openssl/crypto/dsa/dsa_pmeth.c
  ../../libs/openssl/crypto/dsa/dsa_ameth.c
  ../../libs/openssl/crypto/dsa/dsa_depr.c
  ../../libs/openssl/crypto/dsa/dsa_ossl.c
  ../../libs/openssl/crypto/dsa/dsa_err.c
  ../../libs/openssl/crypto/dsa/dsa_sign.c
  ../../libs/openssl/crypto/dsa/dsa_vrf.c
  ../../libs/openssl/crypto/dsa/dsa_asn1.c
  ../../libs/openssl/crypto/dsa/dsa_lib.c
  ../../libs/openssl/crypto/dsa/dsa_key.c
  ../../libs/openssl/crypto/dsa/dsa_gen.c
  ../../libs/openssl/crypto/rsa/rsa_crpt.c
  ../../libs/openssl/crypto/rsa/rsa_pmeth.c
  ../../libs/openssl/crypto/rsa/rsa_prn.c
  ../../libs/openssl/crypto/rsa/rsa_ameth.c
  ../../libs/openssl/crypto/rsa/rsa_depr.c
  ../../libs/openssl/crypto/rsa/rsa_asn1.c
  ../../libs/openssl/crypto/rsa/rsa_x931.c
  ../../libs/openssl/crypto/rsa/rsa_pss.c
  ../../libs/openssl/crypto/rsa/rsa_null.c
  ../../libs/openssl/crypto/rsa/rsa_chk.c
  ../../libs/openssl/crypto/rsa/rsa_oaep.c
  ../../libs/openssl/crypto/rsa/rsa_none.c
  ../../libs/openssl/crypto/rsa/rsa_ssl.c
  ../../libs/openssl/crypto/rsa/rsa_pk1.c
  ../../libs/openssl/crypto/rsa/rsa_err.c
  ../../libs/openssl/crypto/rsa/rsa_saos.c
  ../../libs/openssl/crypto/rsa/rsa_sign.c
  ../../libs/openssl/crypto/rsa/rsa_lib.c
  ../../libs/openssl/crypto/rsa/rsa_gen.c
  ../../libs/openssl/crypto/rsa/rsa_eay.c
  ../../libs/openssl/crypto/bn/bn_x931p.c
  ../../libs/openssl/crypto/bn/bn_const.c
  ../../libs/openssl/crypto/bn/bn_depr.c
  ../../libs/openssl/crypto/bn/bn_nist.c
  ../../libs/openssl/crypto/bn/bn_gf2m.c
  ../../libs/openssl/crypto/bn/bn_exp2.c
  ../../libs/openssl/crypto/bn/bn_mpi.c
  ../../libs/openssl/crypto/bn/bn_mont.c
  ../../libs/openssl/crypto/bn/bn_recp.c
  ../../libs/openssl/crypto/bn/bn_asm.c
  ../../libs/openssl/crypto/bn/bn_sqr.c
  ../../libs/openssl/crypto/bn/bn_err.c
  ../../libs/openssl/crypto/bn/bn_prime.c
  ../../libs/openssl/crypto/bn/bn_gcd.c
  ../../libs/openssl/crypto/bn/bn_sqrt.c
  ../../libs/openssl/crypto/bn/bn_kron.c
  ../../libs/openssl/crypto/bn/bn_blind.c
  ../../libs/openssl/crypto/bn/bn_word.c
  ../../libs/openssl/crypto/bn/bn_shift.c
  ../../libs/openssl/crypto/bn/bn_rand.c
  ../../libs/openssl/crypto/bn/bn_print.c
  ../../libs/openssl/crypto/bn/bn_mod.c
  ../../libs/openssl/crypto/bn/bn_mul.c
  ../../libs/openssl/crypto/bn/bn_ctx.c
  ../../libs/openssl/crypto/bn/bn_lib.c
  ../../libs/openssl/crypto/bn/bn_exp.c
  ../../libs/openssl/crypto/bn/bn_div.c
  ../../libs/openssl/crypto/bn/bn_add.c
  ../../libs/openssl/crypto/modes/wrap128.c
  ../../libs/openssl/crypto/modes/xts128.c
  ../../libs/openssl/crypto/modes/ccm128.c
  ../../libs/openssl/crypto/modes/gcm128.c
  ../../libs/openssl/crypto/modes/ofb128.c
  ../../libs/openssl/crypto/modes/cfb128.c
  ../../libs/openssl/crypto/modes/cts128.c
  ../../libs/openssl/crypto/modes/ctr128.c
  ../../libs/openssl/crypto/modes/cbc128.c
  ../../libs/openssl/crypto/aes/aes_cbc.c
  ../../libs/openssl/crypto/aes/aes_core.c
  ../../libs/openssl/crypto/aes/aes_wrap.c
  ../../libs/openssl/crypto/aes/aes_ige.c
  ../../libs/openssl/crypto/aes/aes_ctr.c
  ../../libs/openssl/crypto/aes/aes_ofb.c
  ../../libs/openssl/crypto/aes/aes_cfb.c
  ../../libs/openssl/crypto/aes/aes_ecb.c
  ../../libs/openssl/crypto/aes/aes_misc.c
  ../../libs/openssl/crypto/rc2/rc2ofb64.c
  ../../libs/openssl/crypto/rc2/rc2cfb64.c
  ../../libs/openssl/crypto/rc2/rc2_cbc.c
  ../../libs/openssl/crypto/rc2/rc2_skey.c
  ../../libs/openssl/crypto/rc2/rc2_ecb.c
  ../../libs/openssl/crypto/des/read2pwd.c
  ../../libs/openssl/crypto/des/des_old2.c
  ../../libs/openssl/crypto/des/des_old.c
  ../../libs/openssl/crypto/des/ede_cbcm_enc.c
  ../../libs/openssl/crypto/des/cbc_cksm.c
  ../../libs/openssl/crypto/des/rpc_enc.c
  ../../libs/openssl/crypto/des/xcbc_enc.c
  ../../libs/openssl/crypto/des/fcrypt.c
  ../../libs/openssl/crypto/des/fcrypt_b.c
  ../../libs/openssl/crypto/des/des_enc.c
  ../../libs/openssl/crypto/des/rand_key.c
  ../../libs/openssl/crypto/des/qud_cksm.c
  ../../libs/openssl/crypto/des/pcbc_enc.c
  ../../libs/openssl/crypto/des/str2key.c
  ../../libs/openssl/crypto/des/ofb_enc.c
  ../../libs/openssl/crypto/des/ofb64enc.c
  ../../libs/openssl/crypto/des/enc_writ.c
  ../../libs/openssl/crypto/des/enc_read.c
  ../../libs/openssl/crypto/des/ofb64ede.c
  ../../libs/openssl/crypto/des/cfb_enc.c
  ../../libs/openssl/crypto/des/cfb64ede.c
  ../../libs/openssl/crypto/des/cfb64enc.c
  ../../libs/openssl/crypto/des/ecb3_enc.c
  ../../libs/openssl/crypto/des/cbc_enc.c
  ../../libs/openssl/crypto/des/ecb_enc.c
  ../../libs/openssl/crypto/des/set_key.c
  ../../libs/openssl/crypto/cmac/cm_pmeth.c
  ../../libs/openssl/crypto/cmac/cm_ameth.c
  ../../libs/openssl/crypto/cmac/cmac.c
  ../../libs/openssl/crypto/hmac/hm_pmeth.c
  ../../libs/openssl/crypto/hmac/hm_ameth.c
  ../../libs/openssl/crypto/hmac/hmac.c
  ../../libs/openssl/crypto/mdc2/mdc2_one.c
  ../../libs/openssl/crypto/mdc2/mdc2dgst.c
  ../../libs/openssl/crypto/sha/sha512.c
  ../../libs/openssl/crypto/sha/sha256.c
  ../../libs/openssl/crypto/sha/sha1_one.c
  ../../libs/openssl/crypto/sha/sha_one.c
  ../../libs/openssl/crypto/sha/sha1dgst.c
  ../../libs/openssl/crypto/sha/sha_dgst.c
  ../../libs/openssl/crypto/md5/md5_one.c
  ../../libs/openssl/crypto/md5/md5_dgst.c
  ../../libs/openssl/crypto/md4/md4_one.c
  ../../libs/openssl/crypto/md4/md4_dgst.c
  ../../libs/openssl/crypto/mem_clr.c
  ../../libs/openssl/crypto/bf/bf_skey.c
  ../../libs/openssl/crypto/bf/bf_ofb64.c
  ../../libs/openssl/crypto/bf/bf_enc.c
  ../../libs/openssl/crypto/bf/bf_ecb.c
  ../../libs/openssl/crypto/bf/bf_cfb64.c
#  ../../libs/openssl/crypto/bf/bf_cbc.c
#  ../../libs/openssl/crypto/bf/bf_opts.c
#  ../../libs/openssl/crypto/bf/bfspeed.c
  ../../libs/openssl/crypto/o_init.c
  ../../libs/openssl/crypto/o_fips.c
  ../../libs/openssl/crypto/o_dir.c
  ../../libs/openssl/crypto/o_str.c
  ../../libs/openssl/crypto/o_time.c
  ../../libs/openssl/crypto/uid.c
  ../../libs/openssl/crypto/ebcdic.c
  ../../libs/openssl/crypto/cpt_err.c
  ../../libs/openssl/crypto/ex_data.c
  ../../libs/openssl/crypto/cversion.c
  ../../libs/openssl/crypto/mem_dbg.c
  ../../libs/openssl/crypto/mem.c
  ../../libs/openssl/crypto/cryptlib.c
  ../../libs/openssl/crypto/getenv.c
)

target_compile_options(libcrypto PRIVATE
  ${OPENSSL_COMPILE_FLAGS}
)

target_include_directories(libcrypto SYSTEM BEFORE PRIVATE
  ../../libs/openssl
  ../../libs/openssl/${OPENSSL_LIB_DIR}
  ../../libs/openssl/crypto
  ../../libs/openssl/crypto/asn1
  ../../libs/openssl/crypto/modes
  ../../libs/openssl/crypto/evp
)

add_library(libssl STATIC
  ../../libs/openssl/ssl/t1_trce.c
  ../../libs/openssl/ssl/t1_reneg.c
  ../../libs/openssl/ssl/kssl.c
  ../../libs/openssl/ssl/ssl_err.c
  ../../libs/openssl/ssl/bio_ssl.c
  ../../libs/openssl/ssl/ssl_conf.c
  ../../libs/openssl/ssl/ssl_algs.c
  ../../libs/openssl/ssl/ssl_txt.c
  ../../libs/openssl/ssl/ssl_asn1.c
  ../../libs/openssl/ssl/ssl_rsa.c
  ../../libs/openssl/ssl/ssl_stat.c
  ../../libs/openssl/ssl/ssl_ciph.c
  ../../libs/openssl/ssl/ssl_sess.c
  ../../libs/openssl/ssl/ssl_cert.c
  ../../libs/openssl/ssl/ssl_err2.c
  ../../libs/openssl/ssl/ssl_lib.c
  ../../libs/openssl/ssl/d1_srtp.c
  ../../libs/openssl/ssl/d1_both.c
  ../../libs/openssl/ssl/d1_pkt.c
  ../../libs/openssl/ssl/d1_lib.c
  ../../libs/openssl/ssl/d1_clnt.c
  ../../libs/openssl/ssl/d1_srvr.c
  ../../libs/openssl/ssl/d1_meth.c
  ../../libs/openssl/ssl/t1_ext.c
  ../../libs/openssl/ssl/t1_enc.c
  ../../libs/openssl/ssl/t1_lib.c
  ../../libs/openssl/ssl/t1_clnt.c
  ../../libs/openssl/ssl/t1_srvr.c
  ../../libs/openssl/ssl/t1_meth.c
  ../../libs/openssl/ssl/s23_pkt.c
  ../../libs/openssl/ssl/s23_lib.c
  ../../libs/openssl/ssl/s23_clnt.c
  ../../libs/openssl/ssl/s23_srvr.c
  ../../libs/openssl/ssl/s23_meth.c
  ../../libs/openssl/ssl/s3_cbc.c
  ../../libs/openssl/ssl/s3_both.c
  ../../libs/openssl/ssl/s3_pkt.c
  ../../libs/openssl/ssl/s3_enc.c
  ../../libs/openssl/ssl/s3_lib.c
  ../../libs/openssl/ssl/s3_clnt.c
  ../../libs/openssl/ssl/s3_srvr.c
  ../../libs/openssl/ssl/s3_meth.c
  ../../libs/openssl/ssl/s2_pkt.c
  ../../libs/openssl/ssl/s2_enc.c
  ../../libs/openssl/ssl/s2_lib.c
  ../../libs/openssl/ssl/s2_clnt.c
  ../../libs/openssl/ssl/s2_srvr.c
  ../../libs/openssl/ssl/s2_meth.c
)

target_compile_options(libssl PRIVATE
  ${OPENSSL_COMPILE_FLAGS}
)

target_include_directories(libssl SYSTEM BEFORE PRIVATE
  ../../libs/openssl
  ../../libs/openssl/${OPENSSL_LIB_DIR}
  ../../libs/openssl/crypto
  ../../libs/openssl/crypto/asn1
  ../../libs/openssl/crypto/modes
  ../../libs/openssl/crypto/evp
)

else(OPT_BUILD_OPENSSL)

add_library (libcrypto STATIC IMPORTED)
set_property(TARGET libcrypto
    PROPERTY IMPORTED_LOCATION ${PROJECT_ROOT}/libs/openssl/${OPENSSL_LIB_DIR}/libcrypto.lib)
add_library (libssl STATIC IMPORTED)
set_property(TARGET libssl
    PROPERTY IMPORTED_LOCATION ${PROJECT_ROOT}/libs/openssl/${OPENSSL_LIB_DIR}/libssl.lib)

endif(OPT_BUILD_OPENSSL)

#-------------------------------------------------------------------------------
#atlmfc

set(ATLMFC_COMPILE_FLAGS
  -D_ATL_NO_DEBUG_CRT -D_ATL_NO_UUIDOF -D_ATL_NO_CONNECTION_POINTS -D_ATL_NO_DATETIME_RESOURCES_
  -D_ATL_NO_DEFAULT_LIBS -D_ATL_NO_PERF_SUPPORT -D_AFX_PORTABLE -D_ATL_NO_COMMODULE
  -D_AFX_NO_SOCKET_SUPPORT
)

if(CMAKE_COMPILER_IS_MINGW)

set(ATLMFC_COMPILE_FLAGS ${ATLMFC_COMPILE_FLAGS}
  -D_AFXDLL
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
  ../../libs/atlmfc/mfc3.cpp
  ../../libs/atlmfc/mfc4.cpp
  ../../libs/atlmfc/mfc5.cpp
  ../../libs/atlmfc/src/mfc/thrdcore.cpp
)

target_compile_options(atlmfc PRIVATE
  ${ATLMFC_COMPILE_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)
if(MSVC)
target_compile_options(atlmfc PRIVATE
  -wd4995 -wd4996 -wd4101 -wd4290
)
endif()

target_include_directories(atlmfc PRIVATE
  ../../libs/atlmfc/include
  ../base
)

#-------------------------------------------------------------------------------
#putty

add_library(putty STATIC
  ../../libs/putty/cproxy.c
  ../../libs/putty/int64.c
  ../../libs/putty/logging.c
  ../../libs/putty/misc.c
  ../../libs/putty/pgssapi.c
  ../../libs/putty/portfwd.c
  ../../libs/putty/proxy.c
  ../../libs/putty/ssh.c
  ../../libs/putty/sshaes.c
  ../../libs/putty/ssharcf.c
  ../../libs/putty/sshblowf.c
  ../../libs/putty/sshbn.c
  ../../libs/putty/sshcrc.c
  ../../libs/putty/sshcrcda.c
  ../../libs/putty/sshdes.c
  ../../libs/putty/sshdh.c
  ../../libs/putty/sshdss.c
  ../../libs/putty/sshgssc.c
  ../../libs/putty/sshmd5.c
  ../../libs/putty/sshpubk.c
  ../../libs/putty/sshrand.c
  ../../libs/putty/sshrsa.c
  ../../libs/putty/sshsh256.c
  ../../libs/putty/sshsh512.c
  ../../libs/putty/sshsha.c
  ../../libs/putty/sshzlib.c
  ../../libs/putty/tree234.c
  ../../libs/putty/wildcard.c
  ../../libs/putty/miscucs.c
  ../../libs/putty/windows/wingss.c
  ../../libs/putty/windows/winhandl.c
  ../../libs/putty/windows/winmisc.c
  ../../libs/putty/windows/winnet.c
  ../../libs/putty/windows/winnoise.c
  ../../libs/putty/windows/winnojmp.c
  ../../libs/putty/windows/winpgntc.c
  ../../libs/putty/windows/winproxy.c
  ../../libs/putty/windows/winsecur.c
  ../../libs/putty/windows/winstore.c
  ../../libs/putty/windows/wintime.c
  ../../libs/putty/windows/winhsock.c
  ../../libs/putty/windows/winucs.c
  ../../libs/putty/x11fwd.c
  ../../libs/putty/conf.c
  ../../libs/putty/callback.c
  ../../libs/putty/sshshare.c
  ../../libs/putty/noshare.c
  ../../libs/putty/errsock.c
  ../../libs/putty/sshecc.c
  ../../libs/putty/sshccp.c
  ../../libs/putty/import.c
  ../../libs/putty/be_misc.c
  ../../libs/putty/sshbcrypt.c
#  ../../libs/putty/ldisc.c
)

set(PUTTY_COMPILE_FLAGS
  -DLibrary -D_MT -DMPEXT -DNET_SETUP_DIAGNOSTICS
  -wd4996
)
if(MSVC)
set(PUTTY_COMPILE_FLAGS ${PUTTY_COMPILE_FLAGS}
  -wd4996
)
endif()

target_compile_options(putty PRIVATE
  ${PUTTY_COMPILE_FLAGS}
)

target_include_directories(putty PRIVATE
  ../../libs/putty
  ../../libs/putty/windows
)

#-------------------------------------------------------------------------------
#tinyxml2

add_library(tinyxml2 STATIC
  ../../libs/tinyxml2/tinyxml2.cpp
)

target_compile_options(tinyxml2 PRIVATE
)

#-------------------------------------------------------------------------------
#neon

add_library(neon STATIC
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

set(NEON_COMPILE_FLAGS )
if(MSVC)
set(NEON_COMPILE_FLAGS ${NEON_COMPILE_FLAGS}
  -wd4996
)
endif()

target_compile_options(neon PRIVATE
  ${NEON_COMPILE_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
)

target_include_directories(neon PRIVATE
  ../../libs/zlib/src
  ../../libs/expat/lib
  ../../libs/openssl
  ../../libs/openssl/${OPENSSL_LIB_DIR}
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

target_compile_options(expat PRIVATE
  ${NETBOX_UNICODE_FLAGS}
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

target_compile_options(zlib PRIVATE
  -DUNALIGNED_OK
  ${NETBOX_UNICODE_FLAGS}
)

target_include_directories(zlib PRIVATE
  ../../libs/zlib/src
)

#-------------------------------------------------------------------------------
#dlmalloc

add_library(dlmalloc STATIC
  ../../libs/dlmalloc/dlmalloc.cc
  ../../libs/dlmalloc/dlmalloc-2.8.6.h
)

target_compile_options(dlmalloc PRIVATE
  ${NETBOX_UNICODE_FLAGS}
)

#-------------------------------------------------------------------------------
#rdestl

add_library(rdestl STATIC
  ../../libs/rdestl/allocator.cpp
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
  ../base
)

target_compile_options(rdestl PRIVATE
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
  ../base
)

target_compile_options(tinylog PRIVATE
  -DTINYLOG_EXPORT
  ${NETBOX_UNICODE_FLAGS}
)

#-------------------------------------------------------------------------------
#fmt

add_library(fmt STATIC
  ../../libs/fmt/fmt/format.cc
  ../../libs/fmt/fmt/format.h
  ../../libs/fmt/fmt/printf.cc
  ../../libs/fmt/fmt/printf.h
#  ../../libs/fmt/fmt/ostream.cc
#  ../../libs/fmt/fmt/ostream.h
  ../../libs/fmt/fmt/string.h
)

target_compile_options(fmt PRIVATE
  -DFMT_EXPORT
  ${NETBOX_UNICODE_FLAGS}
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

set(NETBOX_SOURCES
#  UnityBuildCore.cpp
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
  ../core/Configuration.cpp
  ../core/ScpFileSystem.cpp
  ../core/FtpFileSystem.cpp
  ../core/SftpFileSystem.cpp
  ../core/WebDAVFileSystem.cpp
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

#  UnityBuildFilezilla.cpp
#  ../filezilla/stdafx.cpp
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

#  UnityBuildMain.cpp
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
  FarPluginStrings.cpp
)

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
  FarUtils.h
  FarPluginStrings.h

  ../include/FastDelegateBind.h
  ../include/FastDelegate.h
  ../include/type_traits.h
  ../include/rtti.hpp
  ../include/nbglobals.h
  ../include/headers.hpp
  ../include/nbcore.h
  ../include/nbtypes.h
  ../include/nbstring.h
  ../include/nbstring.inl
  ../include/nbutils.h

  ../base/Global.h
  ../base/Common.h
  ../base/UnicodeString.hpp
  ../base/Exceptions.h
  ../base/Sysutils.hpp
  ../base/StrUtils.hpp
  ../base/MsgIDs.h
  ../base/vcl.h
  ../base/FileBuffer.h
  ../base/ObjIDs.h

  ../core/WebDAVFileSystem.h
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

  ../PluginSDK/Far2/plugin.hpp
  ../PluginSDK/Far2/farcolor.hpp
  ../PluginSDK/Far2/farkeys.hpp

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

find_program(MASM_EXECUTABLE ml)
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
)

endif(NOT x64)
endif(MSVC)

set(NETBOX_LIBRARIES
  putty
  atlmfc
  zlib
  tinyxml2
  libssl libcrypto
  neon
  expat
  dlmalloc
  rdestl
  fmt
  tinylog

  Ws2_32.lib
  kernel32.lib
  user32.lib
  advapi32.lib
  Version.lib
  winhttp.lib
  winspool.lib
  Crypt32.lib
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
  ../../libs/openssl
  ../../libs/openssl/${OPENSSL_LIB_DIR}
  ../../libs/putty
  ../../libs/putty/windows
  ../../libs/neon/src
  ../../libs/tinyxml2
  ../../libs/rdestl
  ../../libs/expat/lib
  ../../libs/atlmfc/include
  ../../libs/fmt
  ../../libs/tinylog
  ../filezilla
  ../base
  ../core
  ../windows
  ../resource
  ../include
  ${CMAKE_CURRENT_SOURCE_DIR}
  ../PluginSDK/${FAR_VERSION}
)

set_target_properties(NetBox
  PROPERTIES
  LINK_FLAGS "${NETBOX_DLL_LINK_FLAGS}"
  LINK_FLAGS_RELEASE "${NETBOX_DLL_LINK_FLAGS_RELEASE}"
  LINK_FLAGS_DEBUG "${NETBOX_DLL_LINK_FLAGS_DEBUG}"
)

#TODO: COMMAND create_ver.py

target_link_libraries(NetBox PRIVATE
  ${NETBOX_LIBRARIES}
)

target_compile_options(NetBox PRIVATE
  -DBUILD_OFFICIAL
  -wd4251 -wd4275 -wd4661
  ${ATLMFC_COMPILE_FLAGS}
  ${NETBOX_UNICODE_FLAGS}
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
  ${NETBOX_DIR}/*.lng
#  ${BUILD_DIR}/*.dll
#  ${BUILD_DIR}/*.pdb
#  ${BUILD_DIR}/*.map
)

add_custom_target(copy_to_plugin_dir)
foreach(DIST_FILE ${DIST_FILES})
  add_custom_command(TARGET copy_to_plugin_dir POST_BUILD
#    COMMAND echo DIST_FILE: ${DIST_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DIST_FILE} ${NETBOX_PLUGIN_DIR}
  )
endforeach()

add_custom_command(TARGET NetBox POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BUILD_DIR}/NetBox.dll ${NETBOX_PLUGIN_DIR}
  COMMAND if exist ${BUILD_DIR}/NetBox.pdb ( ${CMAKE_COMMAND} -E copy_if_different ${BUILD_DIR}/NetBox.pdb ${NETBOX_PLUGIN_DIR} )
  COMMAND if exist ${BUILD_DIR}/NetBox.map ( ${CMAKE_COMMAND} -E copy_if_different ${BUILD_DIR}/NetBox.map ${NETBOX_PLUGIN_DIR} )
)

add_dependencies(copy_to_plugin_dir create_plugin_dir )
add_dependencies(NetBox copy_to_plugin_dir)

#-------------------------------------------------------------------------------
