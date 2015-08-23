project(NetBox)
cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(NOT DEFINED FAR_VERSION OR FAR_VERSION STREQUAL "")
  set(FAR_VERSION Far2)
endif()

if(NOT DEFINED CONF)
  set(CONF "x86")
endif()

if(NOT DEFINED PROJECT_ROOT OR PROJECT_ROOT STREQUAL "")
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
endif()
string(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})

if(NOT DEFINED BUILD_DIR OR BUILD_DIR STREQUAL "")
string(REPLACE "/" "\\" BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

#-------------------------------------------------------------------------------

include(${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake)
include(CMakeDetermineCXXCompiler)
message("System: ${CMAKE_SYSTEM_NAME}, BUILD_DIR: ${BUILD_DIR}, CMAKE_COMPILER_IS_MINGW = ${CMAKE_COMPILER_IS_MINGW}")

set(LIBS_ROOT ${PROJECT_ROOT}/libs)
set(SRC_ROOT ${PROJECT_ROOT}/src)

#-------------------------------------------------------------------------------

set(LIBNEON_DEFS "-DNE_LFS -DNE_HAVE_SSL -DHAVE_OPENSSL -DHAVE_EXPAT -DHAVE_EXPAT_H -DNE_HAVE_DAV -DNE_HAVE_ZLIB -DUSE_GETADDRINFO")
set(LIBEXPAT_DEFS "-DCOMPILED_FROM_DSP -DXML_STATIC")
set(DLMALLOC_DEFS "-DUSE_DLMALLOC -DUSE_DL_PREFIX -DNO_MALLINFO -DUSE_LOCKS=1")
set(CMAKE_C_FLAGS "-D_WINDOWS -DWIN32 -D_WINDLL -D_USRDLL -D_SCL_SECURE_NO_WARNINGS -DAPR_DECLARE_STATIC -DAPR_HAS_LARGE_FILES ${LIBEXPAT_DEFS} ${LIBNEON_DEFS} -DMPEXT -DWINSCP -DNO_STRICT -DNOCRYPT ${DLMALLOC_DEFS}")
set(CMAKE_C_FLAGS "-DCOM_NO_WINDOWS_H ${CMAKE_C_FLAGS}")

if(CONF STREQUAL "x64")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN64")
else()
endif()

set(NETBOX_FLAGS_RELEASE "-U_DEBUG -UNETBOX_DEBUG -DNDEBUG")
set(NETBOX_FLAGS_DEBUG "-D_DEBUG -DDEBUG -DNETBOX_DEBUG")

if(MSVC)

set(NETBOX_WARNING_FLAGS "")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /GR- /EHsc /MP2 /Zi -D_UNICODE -DUNICODE ")
set(NETBOX_FLAGS_RELEASE "${NETBOX_FLAGS_RELEASE} /GL /Gm- /MT -Ox -Ob1 -Oi -Os -GF -GS- -Gy /fp:fast")
set(NETBOX_FLAGS_DEBUG "${NETBOX_FLAGS_DEBUG} /Gm- /MTd /GS")

elseif(CMAKE_COMPILER_IS_MINGW)

set(NETBOX_CFLAGS "${NETBOX_CFLAGS} -static -s -O2 -Wall -Wextra -pedantic -Wold-style-cast -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code")
set(NETBOX_WARNING_FLAGS "-Wno-attributes -Wno-write-strings -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -D_UNICODE -DUNICODE -D_AFXDLL -std=c++11 -Weffc++ -Woverloaded-virtual -Wnon-virtual-dtor")
set(NETBOX_FLAGS_RELEASE "${NETBOX_FLAGS_RELEASE} ${NETBOX_CFLAGS}")
set(NETBOX_FLAGS_DEBUG "${NETBOX_FLAGS_DEBUG} ${NETBOX_CFLAGS}")

endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${NETBOX_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NETBOX_WARNING_FLAGS}")

set(CMAKE_CXX_FLAGS_RELEASE "${NETBOX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELEASE "${NETBOX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${NETBOX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG "${NETBOX_FLAGS_DEBUG}")

if(MSVC)
set(CMAKE_LINK_FLAGS
  "/DEBUG
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
  /NODEFAULTLIB:UAFXCW.LIB
  /NODEFAULTLIB:UAFXCWD.LIB
  /NODEFAULTLIB:LIBCPMT.LIB
  /NODEFAULTLIB:LIBCPMTD.LIB
  /NODEFAULTLIB:LIBEAY32.LIB
  /NODEFAULTLIB:SSLEAY32.LIB
  /NODEFAULTLIB:LIBPUTTY.LIB
  /NODEFAULTLIB:KERNEL32.LIB
  /NODEFAULTLIB:USER32.LIB
  /NODEFAULTLIB:VERSION.LIB
  /DELAYLOAD:version.dll
  /DELAYLOAD:ws2_32.dll
  /DELAYLOAD:oleaut32.dll
  /DELAYLOAD:winhttp.dll
  /DELAYLOAD:shell32.dll
  /DELAYLOAD:shlwapi.dll
  /DELAYLOAD:crypt32.dll
  /DELAYLOAD:gdi32.dll
  /MANIFEST:NO
  /TLBID:1 /DYNAMICBASE /SUBSYSTEM:WINDOWS /NOLOGO
  /MAP /OPT:REF /OPT:ICF=6 /LTCG
  /INCREMENTAL:NO
"
)

elseif(CMAKE_COMPILER_IS_MINGW)
set(CMAKE_LINK_FLAGS "")
endif()

#-------------------------------------------------------

include_directories(
  ${SRC_ROOT}/NetBox
  ${SRC_ROOT}/PluginSDK/${FAR_VERSION}
  ${SRC_ROOT}/include
  ${SRC_ROOT}/base
  ${SRC_ROOT}/core
  ${SRC_ROOT}/windows
  ${SRC_ROOT}/resource
  ${SRC_ROOT}/Common
  ${SRC_ROOT}/filezilla
  ${SRC_ROOT}/filezilla/misc
  ${LIBS_ROOT}
  ${LIBS_ROOT}/atlmfc/include
  ${LIBS_ROOT}/Putty
  ${LIBS_ROOT}/Putty/windows
  ${LIBS_ROOT}/tinyxml2
  ${LIBS_ROOT}/openssl/${CONF}/inc32
  ${LIBS_ROOT}/zlib/src
  ${LIBS_ROOT}/expat/lib
  ${LIBS_ROOT}/apr/include
  ${LIBS_ROOT}/apr/include/arch/win32
  ${LIBS_ROOT}/apr/include/arch/unix
  ${LIBS_ROOT}/rdestl
)

#-------------------------------------------------------------------------------
# target NetBox

set(NETBOX_SOURCES
  ${SRC_ROOT}/NetBox/UnityBuildCore.cpp
  ${SRC_ROOT}/NetBox/UnityBuildMain.cpp
  ${SRC_ROOT}/NetBox/UnityBuildFilezilla.cpp
  ${SRC_ROOT}/resource/TextsFileZilla.rc
  ${SRC_ROOT}/NetBox/NetBox.rc
  ${SRC_ROOT}/NetBox/NetBox.def
)

if(MSVC)
if(CONF STREQUAL "x86")

find_program(MASM_EXECUTABLE ml)
# message("masm: ${MASM_EXECUTABLE}")
set(ASM_OBJECTS)
foreach(src vc10)
  set(ASM_SOURCE ${SRC_ROOT}/NetBox/${src}.asm)
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
  ${SRC_ROOT}/NetBox/vc10wrapper.cpp
  ${SRC_ROOT}/NetBox/vc10u.cpp
  ${ASM_OBJECTS}
)

endif(CONF STREQUAL "x86")
endif(MSVC)

add_library(libeay32 STATIC IMPORTED)
set_property(TARGET libeay32
  PROPERTY IMPORTED_LOCATION ${LIBS_ROOT}/openssl/${CONF}/libeay32.lib
)

add_library(ssleay32 STATIC IMPORTED)
set_property(TARGET ssleay32
  PROPERTY IMPORTED_LOCATION ${LIBS_ROOT}/openssl/${CONF}/ssleay32.lib
)

#-------------------------------------------------------------------------------

set(NETBOX_LIBRARIES
  atlmfc
  putty
  zlib
  tinyxml2
  libeay32
  ssleay32
  neon
  apr
  expat
  dlmalloc
  rdestl
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
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
set(NETBOX_LIBRARIES ${NETBOX_LIBRARIES}
  libcmtd.lib
  libcpmtd.lib
)
else()
set(NETBOX_LIBRARIES ${NETBOX_LIBRARIES}
  libcmt.lib
  libcpmt.lib
)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")
endif(MSVC)

add_library(NetBox SHARED ${NETBOX_SOURCES})

if(MSVC)
set_target_properties(NetBox
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
elseif(CMAKE_COMPILER_IS_MINGW)
set_target_properties(NetBox
  PROPERTIES
  COMPILE_FLAGS "-fpermissive ${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
endif(MSVC)

#-------------------------------------------------------------------------------

#TODO: COMMAND create_ver.py

target_link_libraries(NetBox ${NETBOX_LIBRARIES})

add_dependencies(NetBox
  atlmfc
  putty
  zlib
  tinyxml2
  neon
  apr
  expat
  dlmalloc
  rdestl
)

set(NETBOX_PLUGIN_DIR ${PROJECT_ROOT}/${FAR_VERSION}_${CONF}/Plugins/NetBox)
string(REPLACE "/" "\\" NETBOX_PLUGIN_DIR ${NETBOX_PLUGIN_DIR})

add_custom_command(TARGET NetBox
  POST_BUILD
  COMMAND if not exist "${NETBOX_PLUGIN_DIR}" ( mkdir "${NETBOX_PLUGIN_DIR}" )
  COMMAND cp -f NetBox.dll ${NETBOX_PLUGIN_DIR}
  COMMAND if exist NetBox.pdb ( cp -f NetBox.pdb ${NETBOX_PLUGIN_DIR} )
  COMMAND if exist NetBox.map ( cp -f NetBox.map ${NETBOX_PLUGIN_DIR} )
  WORKING_DIRECTORY ${BUILD_DIR}
  VERBATIM
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
add_custom_command(TARGET NetBox
  POST_BUILD
  DEPENDS ${BUILD_DIR}\\NetBox.dll
  COMMAND scripts\\make_dist.cmd ${FAR_VERSION} ${CONF}
  WORKING_DIRECTORY ${SRC_ROOT}/NetBox
  VERBATIM
)
endif()

#-------------------------------------------------------------------------------
add_library(openssl STATIC
${LIBS_ROOT}/openssl/crypto/openssl.c
${LIBS_ROOT}/openssl/crypto/ts.c
${LIBS_ROOT}/openssl/crypto/prime.c
${LIBS_ROOT}/openssl/crypto/ocsp.c
${LIBS_ROOT}/openssl/crypto/engine.c
${LIBS_ROOT}/openssl/crypto/rand.c
${LIBS_ROOT}/openssl/crypto/cms.c
${LIBS_ROOT}/openssl/crypto/smime.c
${LIBS_ROOT}/openssl/crypto/spkac.c
${LIBS_ROOT}/openssl/crypto/pkeyutl.c
${LIBS_ROOT}/openssl/crypto/pkeyparam.c
${LIBS_ROOT}/openssl/crypto/pkey.c
${LIBS_ROOT}/openssl/crypto/pkcs8.c
${LIBS_ROOT}/openssl/crypto/pkcs12.c
${LIBS_ROOT}/openssl/crypto/nseq.c
${LIBS_ROOT}/openssl/crypto/ciphers.c
${LIBS_ROOT}/openssl/crypto/sess_id.c
${LIBS_ROOT}/openssl/crypto/version.c
${LIBS_ROOT}/openssl/crypto/app_rand.c
${LIBS_ROOT}/openssl/crypto/s_socket.c
${LIBS_ROOT}/openssl/crypto/s_cb.c
${LIBS_ROOT}/openssl/crypto/apps.c
${LIBS_ROOT}/openssl/crypto/s_time.c
${LIBS_ROOT}/openssl/crypto/speed.c
${LIBS_ROOT}/openssl/crypto/s_client.c
${LIBS_ROOT}/openssl/crypto/s_server.c
${LIBS_ROOT}/openssl/crypto/genpkey.c
${LIBS_ROOT}/openssl/crypto/gendsa.c
${LIBS_ROOT}/openssl/crypto/genrsa.c
${LIBS_ROOT}/openssl/crypto/x509.c
${LIBS_ROOT}/openssl/crypto/ecparam.c
${LIBS_ROOT}/openssl/crypto/ec.c
${LIBS_ROOT}/openssl/crypto/dsaparam.c
${LIBS_ROOT}/openssl/crypto/dsa.c
${LIBS_ROOT}/openssl/crypto/rsautl.c
${LIBS_ROOT}/openssl/crypto/rsa.c
${LIBS_ROOT}/openssl/crypto/crl.c
${LIBS_ROOT}/openssl/crypto/crl2p7.c
${LIBS_ROOT}/openssl/crypto/pkcs7.c
${LIBS_ROOT}/openssl/crypto/ca.c
${LIBS_ROOT}/openssl/crypto/errstr.c
${LIBS_ROOT}/openssl/crypto/gendh.c
${LIBS_ROOT}/openssl/crypto/passwd.c
${LIBS_ROOT}/openssl/crypto/enc.c
${LIBS_ROOT}/openssl/crypto/dhparam.c
${LIBS_ROOT}/openssl/crypto/dh.c
${LIBS_ROOT}/openssl/crypto/dgst.c
${LIBS_ROOT}/openssl/crypto/req.c
${LIBS_ROOT}/openssl/crypto/asn1pars.c
${LIBS_ROOT}/openssl/crypto/verify.c
${LIBS_ROOT}/openssl/crypto/igetest.c
${LIBS_ROOT}/openssl/crypto/heartbeat_test.c
${LIBS_ROOT}/openssl/crypto/ssltest.c
${LIBS_ROOT}/openssl/crypto/enginetest.c
${LIBS_ROOT}/openssl/crypto/v3nametest.c
${LIBS_ROOT}/openssl/crypto/verify_extra_test.c
${LIBS_ROOT}/openssl/crypto/evp_extra_test.c
${LIBS_ROOT}/openssl/crypto/evp_test.c
${LIBS_ROOT}/openssl/crypto/randtest.c
${LIBS_ROOT}/openssl/crypto/ecdsatest.c
${LIBS_ROOT}/openssl/crypto/ecdhtest.c
${LIBS_ROOT}/openssl/crypto/ectest.c
${LIBS_ROOT}/openssl/crypto/dhtest.c
${LIBS_ROOT}/openssl/crypto/dsatest.c
${LIBS_ROOT}/openssl/crypto/rsa_test.c
${LIBS_ROOT}/openssl/crypto/exptest.c
${LIBS_ROOT}/openssl/crypto/bntest.c
${LIBS_ROOT}/openssl/crypto/rc2test.c
${LIBS_ROOT}/openssl/crypto/destest.c
${LIBS_ROOT}/openssl/crypto/hmactest.c
${LIBS_ROOT}/openssl/crypto/mdc2test.c
${LIBS_ROOT}/openssl/crypto/sha512t.c
${LIBS_ROOT}/openssl/crypto/sha256t.c
${LIBS_ROOT}/openssl/crypto/sha1test.c
${LIBS_ROOT}/openssl/crypto/shatest.c
${LIBS_ROOT}/openssl/crypto/md5test.c
${LIBS_ROOT}/openssl/crypto/md4test.c
${LIBS_ROOT}/openssl/crypto/constant_time_test.c
${LIBS_ROOT}/openssl/crypto/ssl_utst.c
${LIBS_ROOT}/openssl/crypto/t1_trce.c
${LIBS_ROOT}/openssl/crypto/t1_reneg.c
${LIBS_ROOT}/openssl/crypto/kssl.c
${LIBS_ROOT}/openssl/crypto/ssl_err.c
${LIBS_ROOT}/openssl/crypto/bio_ssl.c
${LIBS_ROOT}/openssl/crypto/ssl_conf.c
${LIBS_ROOT}/openssl/crypto/ssl_algs.c
${LIBS_ROOT}/openssl/crypto/ssl_txt.c
${LIBS_ROOT}/openssl/crypto/ssl_asn1.c
${LIBS_ROOT}/openssl/crypto/ssl_rsa.c
${LIBS_ROOT}/openssl/crypto/ssl_stat.c
${LIBS_ROOT}/openssl/crypto/ssl_ciph.c
${LIBS_ROOT}/openssl/crypto/ssl_sess.c
${LIBS_ROOT}/openssl/crypto/ssl_cert.c
${LIBS_ROOT}/openssl/crypto/ssl_err2.c
${LIBS_ROOT}/openssl/crypto/ssl_lib.c
${LIBS_ROOT}/openssl/crypto/d1_srtp.c
${LIBS_ROOT}/openssl/crypto/d1_both.c
${LIBS_ROOT}/openssl/crypto/d1_pkt.c
${LIBS_ROOT}/openssl/crypto/d1_lib.c
${LIBS_ROOT}/openssl/crypto/d1_clnt.c
${LIBS_ROOT}/openssl/crypto/d1_srvr.c
${LIBS_ROOT}/openssl/crypto/d1_meth.c
${LIBS_ROOT}/openssl/crypto/t1_ext.c
${LIBS_ROOT}/openssl/crypto/t1_enc.c
${LIBS_ROOT}/openssl/crypto/t1_lib.c
${LIBS_ROOT}/openssl/crypto/t1_clnt.c
${LIBS_ROOT}/openssl/crypto/t1_srvr.c
${LIBS_ROOT}/openssl/crypto/t1_meth.c
${LIBS_ROOT}/openssl/crypto/s23_pkt.c
${LIBS_ROOT}/openssl/crypto/s23_lib.c
${LIBS_ROOT}/openssl/crypto/s23_clnt.c
${LIBS_ROOT}/openssl/crypto/s23_srvr.c
${LIBS_ROOT}/openssl/crypto/s23_meth.c
${LIBS_ROOT}/openssl/crypto/s3_cbc.c
${LIBS_ROOT}/openssl/crypto/s3_both.c
${LIBS_ROOT}/openssl/crypto/s3_pkt.c
${LIBS_ROOT}/openssl/crypto/s3_enc.c
${LIBS_ROOT}/openssl/crypto/s3_lib.c
${LIBS_ROOT}/openssl/crypto/s3_clnt.c
${LIBS_ROOT}/openssl/crypto/s3_srvr.c
${LIBS_ROOT}/openssl/crypto/s3_meth.c
${LIBS_ROOT}/openssl/crypto/s2_pkt.c
${LIBS_ROOT}/openssl/crypto/s2_enc.c
${LIBS_ROOT}/openssl/crypto/s2_lib.c
${LIBS_ROOT}/openssl/crypto/s2_clnt.c
${LIBS_ROOT}/openssl/crypto/s2_srvr.c
${LIBS_ROOT}/openssl/crypto/s2_meth.c
${LIBS_ROOT}/openssl/crypto/e_capi.c
${LIBS_ROOT}/openssl/crypto/e_padlock.c
${LIBS_ROOT}/openssl/crypto/e_ubsec.c
${LIBS_ROOT}/openssl/crypto/e_sureware.c
${LIBS_ROOT}/openssl/crypto/e_nuron.c
${LIBS_ROOT}/openssl/crypto/e_chil.c
${LIBS_ROOT}/openssl/crypto/e_gmp.c
${LIBS_ROOT}/openssl/crypto/e_cswift.c
${LIBS_ROOT}/openssl/crypto/e_atalla.c
${LIBS_ROOT}/openssl/crypto/e_aep.c
${LIBS_ROOT}/openssl/crypto/e_4758cca.c
${LIBS_ROOT}/openssl/crypto/ts_asn1.c
${LIBS_ROOT}/openssl/crypto/ts_conf.c
${LIBS_ROOT}/openssl/crypto/ts_lib.c
${LIBS_ROOT}/openssl/crypto/ts_verify_ctx.c
${LIBS_ROOT}/openssl/crypto/ts_rsp_verify.c
${LIBS_ROOT}/openssl/crypto/ts_rsp_sign.c
${LIBS_ROOT}/openssl/crypto/ts_rsp_print.c
${LIBS_ROOT}/openssl/crypto/ts_rsp_utils.c
${LIBS_ROOT}/openssl/crypto/ts_req_print.c
${LIBS_ROOT}/openssl/crypto/ts_req_utils.c
${LIBS_ROOT}/openssl/crypto/ts_err.c
${LIBS_ROOT}/openssl/crypto/pqueue.c
${LIBS_ROOT}/openssl/crypto/krb5_asn.c
${LIBS_ROOT}/openssl/crypto/ui_compat.c
${LIBS_ROOT}/openssl/crypto/ui_util.c
${LIBS_ROOT}/openssl/crypto/ui_openssl.c
${LIBS_ROOT}/openssl/crypto/ui_lib.c
${LIBS_ROOT}/openssl/crypto/ui_err.c
${LIBS_ROOT}/openssl/crypto/ocsp_err.c
${LIBS_ROOT}/openssl/crypto/ocsp_vfy.c
${LIBS_ROOT}/openssl/crypto/ocsp_prn.c
${LIBS_ROOT}/openssl/crypto/ocsp_srv.c
${LIBS_ROOT}/openssl/crypto/ocsp_cl.c
${LIBS_ROOT}/openssl/crypto/ocsp_lib.c
${LIBS_ROOT}/openssl/crypto/ocsp_ht.c
${LIBS_ROOT}/openssl/crypto/ocsp_ext.c
${LIBS_ROOT}/openssl/crypto/ocsp_asn.c
${LIBS_ROOT}/openssl/crypto/eng_rdrand.c
${LIBS_ROOT}/openssl/crypto/eng_cryptodev.c
${LIBS_ROOT}/openssl/crypto/eng_dyn.c
${LIBS_ROOT}/openssl/crypto/eng_cnf.c
${LIBS_ROOT}/openssl/crypto/eng_openssl.c
${LIBS_ROOT}/openssl/crypto/tb_asnmth.c
${LIBS_ROOT}/openssl/crypto/tb_pkmeth.c
${LIBS_ROOT}/openssl/crypto/tb_digest.c
${LIBS_ROOT}/openssl/crypto/tb_cipher.c
${LIBS_ROOT}/openssl/crypto/tb_store.c
${LIBS_ROOT}/openssl/crypto/tb_rand.c
${LIBS_ROOT}/openssl/crypto/tb_ecdh.c
${LIBS_ROOT}/openssl/crypto/tb_dh.c
${LIBS_ROOT}/openssl/crypto/tb_ecdsa.c
${LIBS_ROOT}/openssl/crypto/tb_dsa.c
${LIBS_ROOT}/openssl/crypto/tb_rsa.c
${LIBS_ROOT}/openssl/crypto/eng_all.c
${LIBS_ROOT}/openssl/crypto/eng_fat.c
${LIBS_ROOT}/openssl/crypto/eng_pkey.c
${LIBS_ROOT}/openssl/crypto/eng_table.c
${LIBS_ROOT}/openssl/crypto/eng_ctrl.c
${LIBS_ROOT}/openssl/crypto/eng_init.c
${LIBS_ROOT}/openssl/crypto/eng_list.c
${LIBS_ROOT}/openssl/crypto/eng_lib.c
${LIBS_ROOT}/openssl/crypto/eng_err.c
${LIBS_ROOT}/openssl/crypto/c_zlib.c
${LIBS_ROOT}/openssl/crypto/c_rle.c
${LIBS_ROOT}/openssl/crypto/comp_err.c
${LIBS_ROOT}/openssl/crypto/comp_lib.c
${LIBS_ROOT}/openssl/crypto/p12_p8e.c
${LIBS_ROOT}/openssl/crypto/p12_p8d.c
${LIBS_ROOT}/openssl/crypto/pk12err.c
${LIBS_ROOT}/openssl/crypto/p12_npas.c
${LIBS_ROOT}/openssl/crypto/p12_utl.c
${LIBS_ROOT}/openssl/crypto/p12_mutl.c
${LIBS_ROOT}/openssl/crypto/p12_kiss.c
${LIBS_ROOT}/openssl/crypto/p12_key.c
${LIBS_ROOT}/openssl/crypto/p12_init.c
${LIBS_ROOT}/openssl/crypto/p12_decr.c
${LIBS_ROOT}/openssl/crypto/p12_crt.c
${LIBS_ROOT}/openssl/crypto/p12_crpt.c
${LIBS_ROOT}/openssl/crypto/p12_attr.c
${LIBS_ROOT}/openssl/crypto/p12_asn.c
${LIBS_ROOT}/openssl/crypto/p12_add.c
${LIBS_ROOT}/openssl/crypto/bio_pk7.c
${LIBS_ROOT}/openssl/crypto/pk7_mime.c
${LIBS_ROOT}/openssl/crypto/pk7_attr.c
${LIBS_ROOT}/openssl/crypto/pk7_smime.c
${LIBS_ROOT}/openssl/crypto/pk7_doit.c
${LIBS_ROOT}/openssl/crypto/pkcs7err.c
${LIBS_ROOT}/openssl/crypto/pk7_lib.c
${LIBS_ROOT}/openssl/crypto/pk7_asn1.c
${LIBS_ROOT}/openssl/crypto/txt_db.c
${LIBS_ROOT}/openssl/crypto/conf_sap.c
${LIBS_ROOT}/openssl/crypto/conf_mall.c
${LIBS_ROOT}/openssl/crypto/conf_mod.c
${LIBS_ROOT}/openssl/crypto/conf_def.c
${LIBS_ROOT}/openssl/crypto/conf_api.c
${LIBS_ROOT}/openssl/crypto/conf_lib.c
${LIBS_ROOT}/openssl/crypto/conf_err.c
${LIBS_ROOT}/openssl/crypto/cms_kari.c
${LIBS_ROOT}/openssl/crypto/cms_pwri.c
${LIBS_ROOT}/openssl/crypto/cms_ess.c
${LIBS_ROOT}/openssl/crypto/cms_enc.c
${LIBS_ROOT}/openssl/crypto/cms_env.c
${LIBS_ROOT}/openssl/crypto/cms_cd.c
${LIBS_ROOT}/openssl/crypto/cms_dd.c
${LIBS_ROOT}/openssl/crypto/cms_sd.c
${LIBS_ROOT}/openssl/crypto/cms_err.c
${LIBS_ROOT}/openssl/crypto/cms_smime.c
${LIBS_ROOT}/openssl/crypto/cms_io.c
${LIBS_ROOT}/openssl/crypto/cms_att.c
${LIBS_ROOT}/openssl/crypto/cms_asn1.c
${LIBS_ROOT}/openssl/crypto/cms_lib.c
${LIBS_ROOT}/openssl/crypto/v3_scts.c
${LIBS_ROOT}/openssl/crypto/v3_addr.c
${LIBS_ROOT}/openssl/crypto/v3_asid.c
${LIBS_ROOT}/openssl/crypto/pcy_lib.c
${LIBS_ROOT}/openssl/crypto/pcy_tree.c
${LIBS_ROOT}/openssl/crypto/pcy_map.c
${LIBS_ROOT}/openssl/crypto/pcy_data.c
${LIBS_ROOT}/openssl/crypto/pcy_node.c
${LIBS_ROOT}/openssl/crypto/pcy_cache.c
${LIBS_ROOT}/openssl/crypto/v3_pci.c
${LIBS_ROOT}/openssl/crypto/v3_pcia.c
${LIBS_ROOT}/openssl/crypto/v3_ncons.c
${LIBS_ROOT}/openssl/crypto/v3_pcons.c
${LIBS_ROOT}/openssl/crypto/v3_pmaps.c
${LIBS_ROOT}/openssl/crypto/v3_akeya.c
${LIBS_ROOT}/openssl/crypto/v3_ocsp.c
${LIBS_ROOT}/openssl/crypto/v3_info.c
${LIBS_ROOT}/openssl/crypto/v3_purp.c
${LIBS_ROOT}/openssl/crypto/v3_crld.c
${LIBS_ROOT}/openssl/crypto/v3_cpols.c
${LIBS_ROOT}/openssl/crypto/v3_sxnet.c
${LIBS_ROOT}/openssl/crypto/v3_enum.c
${LIBS_ROOT}/openssl/crypto/v3_int.c
${LIBS_ROOT}/openssl/crypto/v3_pku.c
${LIBS_ROOT}/openssl/crypto/v3_akey.c
${LIBS_ROOT}/openssl/crypto/v3_skey.c
${LIBS_ROOT}/openssl/crypto/v3_alt.c
${LIBS_ROOT}/openssl/crypto/v3_genn.c
${LIBS_ROOT}/openssl/crypto/v3err.c
${LIBS_ROOT}/openssl/crypto/v3_utl.c
${LIBS_ROOT}/openssl/crypto/v3_prn.c
${LIBS_ROOT}/openssl/crypto/v3_lib.c
${LIBS_ROOT}/openssl/crypto/v3_ia5.c
${LIBS_ROOT}/openssl/crypto/v3_extku.c
${LIBS_ROOT}/openssl/crypto/v3_conf.c
${LIBS_ROOT}/openssl/crypto/v3_bitst.c
${LIBS_ROOT}/openssl/crypto/v3_bcons.c
${LIBS_ROOT}/openssl/crypto/x509_vpm.c
${LIBS_ROOT}/openssl/crypto/by_dir.c
${LIBS_ROOT}/openssl/crypto/by_file.c
${LIBS_ROOT}/openssl/crypto/x509_trs.c
${LIBS_ROOT}/openssl/crypto/x509_txt.c
${LIBS_ROOT}/openssl/crypto/x_all.c
${LIBS_ROOT}/openssl/crypto/x509_lu.c
${LIBS_ROOT}/openssl/crypto/x509type.c
${LIBS_ROOT}/openssl/crypto/x509_att.c
${LIBS_ROOT}/openssl/crypto/x509_ext.c
${LIBS_ROOT}/openssl/crypto/x509_v3.c
${LIBS_ROOT}/openssl/crypto/x509name.c
${LIBS_ROOT}/openssl/crypto/x509_err.c
${LIBS_ROOT}/openssl/crypto/x509rset.c
${LIBS_ROOT}/openssl/crypto/x509cset.c
${LIBS_ROOT}/openssl/crypto/x509_set.c
${LIBS_ROOT}/openssl/crypto/x509_vfy.c
${LIBS_ROOT}/openssl/crypto/x509spki.c
${LIBS_ROOT}/openssl/crypto/x509_req.c
${LIBS_ROOT}/openssl/crypto/x509_obj.c
${LIBS_ROOT}/openssl/crypto/x509_cmp.c
${LIBS_ROOT}/openssl/crypto/x509_r2x.c
${LIBS_ROOT}/openssl/crypto/x509_d2.c
${LIBS_ROOT}/openssl/crypto/x509_def.c
${LIBS_ROOT}/openssl/crypto/pvkfmt.c
${LIBS_ROOT}/openssl/crypto/pem_pkey.c
${LIBS_ROOT}/openssl/crypto/pem_pk8.c
${LIBS_ROOT}/openssl/crypto/pem_oth.c
${LIBS_ROOT}/openssl/crypto/pem_xaux.c
${LIBS_ROOT}/openssl/crypto/pem_x509.c
${LIBS_ROOT}/openssl/crypto/pem_err.c
${LIBS_ROOT}/openssl/crypto/pem_all.c
${LIBS_ROOT}/openssl/crypto/pem_lib.c
${LIBS_ROOT}/openssl/crypto/pem_info.c
${LIBS_ROOT}/openssl/crypto/pem_seal.c
${LIBS_ROOT}/openssl/crypto/pem_sign.c
${LIBS_ROOT}/openssl/crypto/asn_moid.c
${LIBS_ROOT}/openssl/crypto/p8_pkey.c
${LIBS_ROOT}/openssl/crypto/p5_pbev2.c
${LIBS_ROOT}/openssl/crypto/p5_pbe.c
${LIBS_ROOT}/openssl/crypto/asn_pack.c
${LIBS_ROOT}/openssl/crypto/evp_asn1.c
${LIBS_ROOT}/openssl/crypto/a_strnid.c
${LIBS_ROOT}/openssl/crypto/a_bytes.c
${LIBS_ROOT}/openssl/crypto/asn1_err.c
${LIBS_ROOT}/openssl/crypto/asn1_lib.c
${LIBS_ROOT}/openssl/crypto/asn1_par.c
${LIBS_ROOT}/openssl/crypto/asn1_gen.c
${LIBS_ROOT}/openssl/crypto/asn_mime.c
${LIBS_ROOT}/openssl/crypto/bio_ndef.c
${LIBS_ROOT}/openssl/crypto/bio_asn1.c
${LIBS_ROOT}/openssl/crypto/x_exten.c
${LIBS_ROOT}/openssl/crypto/a_bool.c
${LIBS_ROOT}/openssl/crypto/x_pkey.c
${LIBS_ROOT}/openssl/crypto/f_enum.c
${LIBS_ROOT}/openssl/crypto/f_string.c
${LIBS_ROOT}/openssl/crypto/f_int.c
${LIBS_ROOT}/openssl/crypto/ameth_lib.c
${LIBS_ROOT}/openssl/crypto/tasn_prn.c
${LIBS_ROOT}/openssl/crypto/tasn_typ.c
${LIBS_ROOT}/openssl/crypto/tasn_utl.c
${LIBS_ROOT}/openssl/crypto/tasn_dec.c
${LIBS_ROOT}/openssl/crypto/tasn_enc.c
${LIBS_ROOT}/openssl/crypto/tasn_fre.c
${LIBS_ROOT}/openssl/crypto/tasn_new.c
${LIBS_ROOT}/openssl/crypto/t_bitst.c
${LIBS_ROOT}/openssl/crypto/t_spki.c
${LIBS_ROOT}/openssl/crypto/t_pkey.c
${LIBS_ROOT}/openssl/crypto/t_crl.c
${LIBS_ROOT}/openssl/crypto/t_x509a.c
${LIBS_ROOT}/openssl/crypto/t_x509.c
${LIBS_ROOT}/openssl/crypto/t_req.c
${LIBS_ROOT}/openssl/crypto/i2d_pr.c
${LIBS_ROOT}/openssl/crypto/i2d_pu.c
${LIBS_ROOT}/openssl/crypto/d2i_pr.c
${LIBS_ROOT}/openssl/crypto/d2i_pu.c
${LIBS_ROOT}/openssl/crypto/x_nx509.c
${LIBS_ROOT}/openssl/crypto/nsseq.c
${LIBS_ROOT}/openssl/crypto/x_spki.c
${LIBS_ROOT}/openssl/crypto/x_info.c
${LIBS_ROOT}/openssl/crypto/x_crl.c
${LIBS_ROOT}/openssl/crypto/x_x509a.c
${LIBS_ROOT}/openssl/crypto/x_x509.c
${LIBS_ROOT}/openssl/crypto/x_name.c
${LIBS_ROOT}/openssl/crypto/x_long.c
${LIBS_ROOT}/openssl/crypto/x_bignum.c
${LIBS_ROOT}/openssl/crypto/x_attrib.c
${LIBS_ROOT}/openssl/crypto/x_req.c
${LIBS_ROOT}/openssl/crypto/x_sig.c
${LIBS_ROOT}/openssl/crypto/x_pubkey.c
${LIBS_ROOT}/openssl/crypto/x_val.c
${LIBS_ROOT}/openssl/crypto/x_algor.c
${LIBS_ROOT}/openssl/crypto/a_strex.c
${LIBS_ROOT}/openssl/crypto/a_mbstr.c
${LIBS_ROOT}/openssl/crypto/a_verify.c
${LIBS_ROOT}/openssl/crypto/a_digest.c
${LIBS_ROOT}/openssl/crypto/a_sign.c
${LIBS_ROOT}/openssl/crypto/a_utf8.c
${LIBS_ROOT}/openssl/crypto/a_enum.c
${LIBS_ROOT}/openssl/crypto/a_i2d_fp.c
${LIBS_ROOT}/openssl/crypto/a_d2i_fp.c
${LIBS_ROOT}/openssl/crypto/a_dup.c
${LIBS_ROOT}/openssl/crypto/a_set.c
${LIBS_ROOT}/openssl/crypto/a_type.c
${LIBS_ROOT}/openssl/crypto/a_print.c
${LIBS_ROOT}/openssl/crypto/a_octet.c
${LIBS_ROOT}/openssl/crypto/a_int.c
${LIBS_ROOT}/openssl/crypto/a_time.c
${LIBS_ROOT}/openssl/crypto/a_gentm.c
${LIBS_ROOT}/openssl/crypto/a_utctm.c
${LIBS_ROOT}/openssl/crypto/a_bitstr.c
${LIBS_ROOT}/openssl/crypto/a_object.c
${LIBS_ROOT}/openssl/crypto/e_rc4_hmac_md5.c
${LIBS_ROOT}/openssl/crypto/e_aes_cbc_hmac_sha256.c
${LIBS_ROOT}/openssl/crypto/e_aes_cbc_hmac_sha1.c
${LIBS_ROOT}/openssl/crypto/m_sigver.c
${LIBS_ROOT}/openssl/crypto/pmeth_gn.c
${LIBS_ROOT}/openssl/crypto/pmeth_fn.c
${LIBS_ROOT}/openssl/crypto/pmeth_lib.c
${LIBS_ROOT}/openssl/crypto/e_old.c
${LIBS_ROOT}/openssl/crypto/p5_crpt2.c
${LIBS_ROOT}/openssl/crypto/p5_crpt.c
${LIBS_ROOT}/openssl/crypto/evp_pbe.c
${LIBS_ROOT}/openssl/crypto/evp_pkey.c
${LIBS_ROOT}/openssl/crypto/bio_ok.c
${LIBS_ROOT}/openssl/crypto/evp_lib.c
${LIBS_ROOT}/openssl/crypto/c_alld.c
${LIBS_ROOT}/openssl/crypto/c_allc.c
${LIBS_ROOT}/openssl/crypto/c_all.c
${LIBS_ROOT}/openssl/crypto/e_null.c
${LIBS_ROOT}/openssl/crypto/evp_err.c
${LIBS_ROOT}/openssl/crypto/bio_enc.c
${LIBS_ROOT}/openssl/crypto/bio_b64.c
${LIBS_ROOT}/openssl/crypto/bio_md.c
${LIBS_ROOT}/openssl/crypto/p_dec.c
${LIBS_ROOT}/openssl/crypto/p_enc.c
${LIBS_ROOT}/openssl/crypto/p_lib.c
${LIBS_ROOT}/openssl/crypto/p_verify.c
${LIBS_ROOT}/openssl/crypto/p_sign.c
${LIBS_ROOT}/openssl/crypto/p_seal.c
${LIBS_ROOT}/openssl/crypto/p_open.c
${LIBS_ROOT}/openssl/crypto/m_ecdsa.c
${LIBS_ROOT}/openssl/crypto/m_mdc2.c
${LIBS_ROOT}/openssl/crypto/m_dss1.c
${LIBS_ROOT}/openssl/crypto/m_dss.c
${LIBS_ROOT}/openssl/crypto/m_wp.c
${LIBS_ROOT}/openssl/crypto/m_sha1.c
${LIBS_ROOT}/openssl/crypto/m_sha.c
${LIBS_ROOT}/openssl/crypto/m_md5.c
${LIBS_ROOT}/openssl/crypto/m_md4.c
${LIBS_ROOT}/openssl/crypto/m_null.c
${LIBS_ROOT}/openssl/crypto/e_rc5.c
${LIBS_ROOT}/openssl/crypto/e_cast.c
${LIBS_ROOT}/openssl/crypto/e_rc2.c
${LIBS_ROOT}/openssl/crypto/e_xcbc_d.c
${LIBS_ROOT}/openssl/crypto/names.c
${LIBS_ROOT}/openssl/crypto/e_aes.c
${LIBS_ROOT}/openssl/crypto/e_des3.c
${LIBS_ROOT}/openssl/crypto/e_idea.c
${LIBS_ROOT}/openssl/crypto/e_bf.c
${LIBS_ROOT}/openssl/crypto/e_des.c
${LIBS_ROOT}/openssl/crypto/evp_cnf.c
${LIBS_ROOT}/openssl/crypto/evp_acnf.c
${LIBS_ROOT}/openssl/crypto/evp_key.c
${LIBS_ROOT}/openssl/crypto/evp_enc.c
${LIBS_ROOT}/openssl/crypto/digest.c
${LIBS_ROOT}/openssl/crypto/encode.c
${LIBS_ROOT}/openssl/crypto/obj_xref.c
${LIBS_ROOT}/openssl/crypto/obj_err.c
${LIBS_ROOT}/openssl/crypto/obj_lib.c
${LIBS_ROOT}/openssl/crypto/obj_dat.c
${LIBS_ROOT}/openssl/crypto/o_names.c
${LIBS_ROOT}/openssl/crypto/err_prn.c
${LIBS_ROOT}/openssl/crypto/err_all.c
${LIBS_ROOT}/openssl/crypto/err.c
${LIBS_ROOT}/openssl/crypto/rand_nw.c
${LIBS_ROOT}/openssl/crypto/rand_os2.c
${LIBS_ROOT}/openssl/crypto/rand_unix.c
${LIBS_ROOT}/openssl/crypto/rand_win.c
${LIBS_ROOT}/openssl/crypto/rand_egd.c
${LIBS_ROOT}/openssl/crypto/rand_err.c
${LIBS_ROOT}/openssl/crypto/rand_lib.c
${LIBS_ROOT}/openssl/crypto/randfile.c
${LIBS_ROOT}/openssl/crypto/md_rand.c
${LIBS_ROOT}/openssl/crypto/lh_stats.c
${LIBS_ROOT}/openssl/crypto/lhash.c
${LIBS_ROOT}/openssl/crypto/stack.c
${LIBS_ROOT}/openssl/crypto/bss_dgram.c
${LIBS_ROOT}/openssl/crypto/bss_bio.c
${LIBS_ROOT}/openssl/crypto/bss_log.c
${LIBS_ROOT}/openssl/crypto/bf_nbio.c
${LIBS_ROOT}/openssl/crypto/bss_acpt.c
${LIBS_ROOT}/openssl/crypto/b_sock.c
${LIBS_ROOT}/openssl/crypto/b_dump.c
${LIBS_ROOT}/openssl/crypto/b_print.c
${LIBS_ROOT}/openssl/crypto/bf_buff.c
${LIBS_ROOT}/openssl/crypto/bf_null.c
${LIBS_ROOT}/openssl/crypto/bss_conn.c
${LIBS_ROOT}/openssl/crypto/bss_sock.c
${LIBS_ROOT}/openssl/crypto/bss_file.c
${LIBS_ROOT}/openssl/crypto/bss_fd.c
${LIBS_ROOT}/openssl/crypto/bss_null.c
${LIBS_ROOT}/openssl/crypto/bss_mem.c
${LIBS_ROOT}/openssl/crypto/bio_err.c
${LIBS_ROOT}/openssl/crypto/bio_cb.c
${LIBS_ROOT}/openssl/crypto/bio_lib.c
${LIBS_ROOT}/openssl/crypto/buf_err.c
${LIBS_ROOT}/openssl/crypto/buf_str.c
${LIBS_ROOT}/openssl/crypto/buffer.c
${LIBS_ROOT}/openssl/crypto/ecs_err.c
${LIBS_ROOT}/openssl/crypto/ecs_vrf.c
${LIBS_ROOT}/openssl/crypto/ecs_sign.c
${LIBS_ROOT}/openssl/crypto/ecs_ossl.c
${LIBS_ROOT}/openssl/crypto/ecs_asn1.c
${LIBS_ROOT}/openssl/crypto/ecs_lib.c
${LIBS_ROOT}/openssl/crypto/ech_kdf.c
${LIBS_ROOT}/openssl/crypto/ech_err.c
${LIBS_ROOT}/openssl/crypto/ech_key.c
${LIBS_ROOT}/openssl/crypto/ech_ossl.c
${LIBS_ROOT}/openssl/crypto/ech_lib.c
${LIBS_ROOT}/openssl/crypto/ec_oct.c
${LIBS_ROOT}/openssl/crypto/ec2_oct.c
${LIBS_ROOT}/openssl/crypto/ecp_oct.c
${LIBS_ROOT}/openssl/crypto/ecp_nistputil.c
${LIBS_ROOT}/openssl/crypto/ecp_nistp521.c
${LIBS_ROOT}/openssl/crypto/ecp_nistp256.c
${LIBS_ROOT}/openssl/crypto/ecp_nistp224.c
${LIBS_ROOT}/openssl/crypto/eck_prn.c
${LIBS_ROOT}/openssl/crypto/ec_pmeth.c
${LIBS_ROOT}/openssl/crypto/ec_ameth.c
${LIBS_ROOT}/openssl/crypto/ec2_mult.c
${LIBS_ROOT}/openssl/crypto/ec2_smpl.c
${LIBS_ROOT}/openssl/crypto/ec_key.c
${LIBS_ROOT}/openssl/crypto/ec_asn1.c
${LIBS_ROOT}/openssl/crypto/ec_print.c
${LIBS_ROOT}/openssl/crypto/ec_check.c
${LIBS_ROOT}/openssl/crypto/ec_curve.c
${LIBS_ROOT}/openssl/crypto/ec_err.c
${LIBS_ROOT}/openssl/crypto/ec_mult.c
${LIBS_ROOT}/openssl/crypto/ec_cvt.c
${LIBS_ROOT}/openssl/crypto/ecp_nist.c
${LIBS_ROOT}/openssl/crypto/ecp_mont.c
${LIBS_ROOT}/openssl/crypto/ecp_smpl.c
${LIBS_ROOT}/openssl/crypto/ec_lib.c
${LIBS_ROOT}/openssl/crypto/dh_kdf.c
${LIBS_ROOT}/openssl/crypto/dh_rfc5114.c
${LIBS_ROOT}/openssl/crypto/dh_prn.c
${LIBS_ROOT}/openssl/crypto/dh_pmeth.c
${LIBS_ROOT}/openssl/crypto/dh_ameth.c
${LIBS_ROOT}/openssl/crypto/dh_depr.c
${LIBS_ROOT}/openssl/crypto/dh_err.c
${LIBS_ROOT}/openssl/crypto/dh_check.c
${LIBS_ROOT}/openssl/crypto/dh_lib.c
${LIBS_ROOT}/openssl/crypto/dh_key.c
${LIBS_ROOT}/openssl/crypto/dh_gen.c
${LIBS_ROOT}/openssl/crypto/dh_asn1.c
${LIBS_ROOT}/openssl/crypto/dso_beos.c
${LIBS_ROOT}/openssl/crypto/dso_vms.c
${LIBS_ROOT}/openssl/crypto/dso_win32.c
${LIBS_ROOT}/openssl/crypto/dso_openssl.c
${LIBS_ROOT}/openssl/crypto/dso_null.c
${LIBS_ROOT}/openssl/crypto/dso_lib.c
${LIBS_ROOT}/openssl/crypto/dso_err.c
${LIBS_ROOT}/openssl/crypto/dso_dlfcn.c
${LIBS_ROOT}/openssl/crypto/dso_dl.c
${LIBS_ROOT}/openssl/crypto/dsa_prn.c
${LIBS_ROOT}/openssl/crypto/dsa_pmeth.c
${LIBS_ROOT}/openssl/crypto/dsa_ameth.c
${LIBS_ROOT}/openssl/crypto/dsa_depr.c
${LIBS_ROOT}/openssl/crypto/dsa_ossl.c
${LIBS_ROOT}/openssl/crypto/dsa_err.c
${LIBS_ROOT}/openssl/crypto/dsa_sign.c
${LIBS_ROOT}/openssl/crypto/dsa_vrf.c
${LIBS_ROOT}/openssl/crypto/dsa_asn1.c
${LIBS_ROOT}/openssl/crypto/dsa_lib.c
${LIBS_ROOT}/openssl/crypto/dsa_key.c
${LIBS_ROOT}/openssl/crypto/dsa_gen.c
${LIBS_ROOT}/openssl/crypto/rsa_crpt.c
${LIBS_ROOT}/openssl/crypto/rsa_pmeth.c
${LIBS_ROOT}/openssl/crypto/rsa_prn.c
${LIBS_ROOT}/openssl/crypto/rsa_ameth.c
${LIBS_ROOT}/openssl/crypto/rsa_depr.c
${LIBS_ROOT}/openssl/crypto/rsa_asn1.c
${LIBS_ROOT}/openssl/crypto/rsa_x931.c
${LIBS_ROOT}/openssl/crypto/rsa_pss.c
${LIBS_ROOT}/openssl/crypto/rsa_null.c
${LIBS_ROOT}/openssl/crypto/rsa_chk.c
${LIBS_ROOT}/openssl/crypto/rsa_oaep.c
${LIBS_ROOT}/openssl/crypto/rsa_none.c
${LIBS_ROOT}/openssl/crypto/rsa_ssl.c
${LIBS_ROOT}/openssl/crypto/rsa_pk1.c
${LIBS_ROOT}/openssl/crypto/rsa_err.c
${LIBS_ROOT}/openssl/crypto/rsa_saos.c
${LIBS_ROOT}/openssl/crypto/rsa_sign.c
${LIBS_ROOT}/openssl/crypto/rsa_lib.c
${LIBS_ROOT}/openssl/crypto/rsa_gen.c
${LIBS_ROOT}/openssl/crypto/rsa_eay.c
${LIBS_ROOT}/openssl/crypto/bn_x931p.c
${LIBS_ROOT}/openssl/crypto/bn_const.c
${LIBS_ROOT}/openssl/crypto/bn_depr.c
${LIBS_ROOT}/openssl/crypto/bn_nist.c
${LIBS_ROOT}/openssl/crypto/bn_gf2m.c
${LIBS_ROOT}/openssl/crypto/bn_exp2.c
${LIBS_ROOT}/openssl/crypto/bn_mpi.c
${LIBS_ROOT}/openssl/crypto/bn_mont.c
${LIBS_ROOT}/openssl/crypto/bn_recp.c
${LIBS_ROOT}/openssl/crypto/bn_asm.c
${LIBS_ROOT}/openssl/crypto/bn_sqr.c
${LIBS_ROOT}/openssl/crypto/bn_err.c
${LIBS_ROOT}/openssl/crypto/bn_prime.c
${LIBS_ROOT}/openssl/crypto/bn_gcd.c
${LIBS_ROOT}/openssl/crypto/bn_sqrt.c
${LIBS_ROOT}/openssl/crypto/bn_kron.c
${LIBS_ROOT}/openssl/crypto/bn_blind.c
${LIBS_ROOT}/openssl/crypto/bn_word.c
${LIBS_ROOT}/openssl/crypto/bn_shift.c
${LIBS_ROOT}/openssl/crypto/bn_rand.c
${LIBS_ROOT}/openssl/crypto/bn_print.c
${LIBS_ROOT}/openssl/crypto/bn_mod.c
${LIBS_ROOT}/openssl/crypto/bn_mul.c
${LIBS_ROOT}/openssl/crypto/bn_ctx.c
${LIBS_ROOT}/openssl/crypto/bn_lib.c
${LIBS_ROOT}/openssl/crypto/bn_exp.c
${LIBS_ROOT}/openssl/crypto/bn_div.c
${LIBS_ROOT}/openssl/crypto/bn_add.c
${LIBS_ROOT}/openssl/crypto/wrap128.c
${LIBS_ROOT}/openssl/crypto/xts128.c
${LIBS_ROOT}/openssl/crypto/ccm128.c
${LIBS_ROOT}/openssl/crypto/gcm128.c
${LIBS_ROOT}/openssl/crypto/ofb128.c
${LIBS_ROOT}/openssl/crypto/cfb128.c
${LIBS_ROOT}/openssl/crypto/cts128.c
${LIBS_ROOT}/openssl/crypto/ctr128.c
${LIBS_ROOT}/openssl/crypto/cbc128.c
${LIBS_ROOT}/openssl/crypto/aes_cbc.c
${LIBS_ROOT}/openssl/crypto/aes_core.c
${LIBS_ROOT}/openssl/crypto/aes_wrap.c
${LIBS_ROOT}/openssl/crypto/aes_ige.c
${LIBS_ROOT}/openssl/crypto/aes_ctr.c
${LIBS_ROOT}/openssl/crypto/aes_ofb.c
${LIBS_ROOT}/openssl/crypto/aes_cfb.c
${LIBS_ROOT}/openssl/crypto/aes_ecb.c
${LIBS_ROOT}/openssl/crypto/aes_misc.c
${LIBS_ROOT}/openssl/crypto/rc2ofb64.c
${LIBS_ROOT}/openssl/crypto/rc2cfb64.c
${LIBS_ROOT}/openssl/crypto/rc2_cbc.c
${LIBS_ROOT}/openssl/crypto/rc2_skey.c
${LIBS_ROOT}/openssl/crypto/rc2_ecb.c
${LIBS_ROOT}/openssl/crypto/read2pwd.c
${LIBS_ROOT}/openssl/crypto/des_old2.c
${LIBS_ROOT}/openssl/crypto/des_old.c
${LIBS_ROOT}/openssl/crypto/ede_cbcm_enc.c
${LIBS_ROOT}/openssl/crypto/cbc_cksm.c
${LIBS_ROOT}/openssl/crypto/rpc_enc.c
${LIBS_ROOT}/openssl/crypto/xcbc_enc.c
${LIBS_ROOT}/openssl/crypto/fcrypt.c
${LIBS_ROOT}/openssl/crypto/fcrypt_b.c
${LIBS_ROOT}/openssl/crypto/des_enc.c
${LIBS_ROOT}/openssl/crypto/rand_key.c
${LIBS_ROOT}/openssl/crypto/qud_cksm.c
${LIBS_ROOT}/openssl/crypto/pcbc_enc.c
${LIBS_ROOT}/openssl/crypto/str2key.c
${LIBS_ROOT}/openssl/crypto/ofb_enc.c
${LIBS_ROOT}/openssl/crypto/ofb64enc.c
${LIBS_ROOT}/openssl/crypto/enc_writ.c
${LIBS_ROOT}/openssl/crypto/enc_read.c
${LIBS_ROOT}/openssl/crypto/ofb64ede.c
${LIBS_ROOT}/openssl/crypto/cfb_enc.c
${LIBS_ROOT}/openssl/crypto/cfb64ede.c
${LIBS_ROOT}/openssl/crypto/cfb64enc.c
${LIBS_ROOT}/openssl/crypto/ecb3_enc.c
${LIBS_ROOT}/openssl/crypto/cbc_enc.c
${LIBS_ROOT}/openssl/crypto/ecb_enc.c
${LIBS_ROOT}/openssl/crypto/set_key.c
${LIBS_ROOT}/openssl/crypto/cm_pmeth.c
${LIBS_ROOT}/openssl/crypto/cm_ameth.c
${LIBS_ROOT}/openssl/crypto/cmac.c
${LIBS_ROOT}/openssl/crypto/hm_pmeth.c
${LIBS_ROOT}/openssl/crypto/hm_ameth.c
${LIBS_ROOT}/openssl/crypto/hmac.c
${LIBS_ROOT}/openssl/crypto/mdc2_one.c
${LIBS_ROOT}/openssl/crypto/mdc2dgst.c
${LIBS_ROOT}/openssl/crypto/sha512.c
${LIBS_ROOT}/openssl/crypto/sha256.c
${LIBS_ROOT}/openssl/crypto/sha1_one.c
${LIBS_ROOT}/openssl/crypto/sha_one.c
${LIBS_ROOT}/openssl/crypto/sha1dgst.c
${LIBS_ROOT}/openssl/crypto/sha_dgst.c
${LIBS_ROOT}/openssl/crypto/md5_one.c
${LIBS_ROOT}/openssl/crypto/md5_dgst.c
${LIBS_ROOT}/openssl/crypto/md4_one.c
${LIBS_ROOT}/openssl/crypto/md4_dgst.c
${LIBS_ROOT}/openssl/crypto/mem_clr.c
${LIBS_ROOT}/openssl/crypto/fips_ers.c
${LIBS_ROOT}/openssl/crypto/o_init.c
${LIBS_ROOT}/openssl/crypto/o_fips.c
${LIBS_ROOT}/openssl/crypto/o_dir.c
${LIBS_ROOT}/openssl/crypto/o_str.c
${LIBS_ROOT}/openssl/crypto/o_time.c
${LIBS_ROOT}/openssl/crypto/uid.c
${LIBS_ROOT}/openssl/crypto/ebcdic.c
${LIBS_ROOT}/openssl/crypto/cpt_err.c
${LIBS_ROOT}/openssl/crypto/ex_data.c
${LIBS_ROOT}/openssl/crypto/cversion.c
${LIBS_ROOT}/openssl/crypto/mem_dbg.c
${LIBS_ROOT}/openssl/crypto/mem.c
${LIBS_ROOT}/openssl/crypto/cryptlib.c
)

#-------------------------------------------------------------------------------

add_library(atlmfc STATIC
  ${LIBS_ROOT}/atlmfc/mfc1.cpp
  ${LIBS_ROOT}/atlmfc/mfc2.cpp
  ${LIBS_ROOT}/atlmfc/mfc3.cpp
  ${LIBS_ROOT}/atlmfc/mfc4.cpp
  ${LIBS_ROOT}/atlmfc/mfc5.cpp
  ${LIBS_ROOT}/atlmfc/src/mfc/thrdcore.cpp
)

#-------------------------------------------------------------------------------
set(ATLMFC_COMPILE_FLAGS "-D_WIN32_WINNT=0x0501 -D_ATL_NO_DEBUG_CRT -D_ATL_NO_UUIDOF -D_ATL_NO_CONNECTION_POINTS -D_ATL_NO_DATETIME_RESOURCES_ -D_ATL_NO_DEFAULT_LIBS -D_ATL_NO_PERF_SUPPORT -D_AFX_PORTABLE -D_ATL_NO_COMMODULE -D_AFX_NO_OLE_SUPPORT")

if(MSVC)
set_target_properties(atlmfc
  PROPERTIES
  COMPILE_FLAGS "${ATLMFC_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
elseif(CMAKE_COMPILER_IS_MINGW)
set_target_properties(atlmfc
  PROPERTIES
  COMPILE_FLAGS "-Wno-int-to-pointer-cast -Wno-pragmas -fpermissive ${ATLMFC_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
endif()

#-------------------------------------------------------------------------------

add_library(putty STATIC
  ${LIBS_ROOT}/Putty/cproxy.c
  ${LIBS_ROOT}/Putty/int64.c
  ${LIBS_ROOT}/Putty/logging.c
  ${LIBS_ROOT}/Putty/misc.c
  ${LIBS_ROOT}/Putty/pgssapi.c
  ${LIBS_ROOT}/Putty/portfwd.c
  ${LIBS_ROOT}/Putty/proxy.c
  ${LIBS_ROOT}/Putty/ssh.c
  ${LIBS_ROOT}/Putty/sshaes.c
  ${LIBS_ROOT}/Putty/ssharcf.c
  ${LIBS_ROOT}/Putty/sshblowf.c
  ${LIBS_ROOT}/Putty/sshbn.c
  ${LIBS_ROOT}/Putty/sshcrc.c
  ${LIBS_ROOT}/Putty/sshcrcda.c
  ${LIBS_ROOT}/Putty/sshdes.c
  ${LIBS_ROOT}/Putty/sshdh.c
  ${LIBS_ROOT}/Putty/sshdss.c
  ${LIBS_ROOT}/Putty/sshgssc.c
  ${LIBS_ROOT}/Putty/sshmd5.c
  ${LIBS_ROOT}/Putty/sshpubk.c
  ${LIBS_ROOT}/Putty/sshrand.c
  ${LIBS_ROOT}/Putty/sshrsa.c
  ${LIBS_ROOT}/Putty/sshsh256.c
  ${LIBS_ROOT}/Putty/sshsh512.c
  ${LIBS_ROOT}/Putty/sshsha.c
  ${LIBS_ROOT}/Putty/sshzlib.c
  ${LIBS_ROOT}/Putty/tree234.c
  ${LIBS_ROOT}/Putty/wildcard.c
  ${LIBS_ROOT}/Putty/windows/wingss.c
  ${LIBS_ROOT}/Putty/windows/winhandl.c
  ${LIBS_ROOT}/Putty/windows/winmisc.c
  ${LIBS_ROOT}/Putty/windows/winnet.c
  ${LIBS_ROOT}/Putty/windows/winnoise.c
  ${LIBS_ROOT}/Putty/windows/winnojmp.c
  ${LIBS_ROOT}/Putty/windows/winpgntc.c
  ${LIBS_ROOT}/Putty/windows/winproxy.c
  ${LIBS_ROOT}/Putty/windows/winstore.c
  ${LIBS_ROOT}/Putty/windows/wintime.c
  ${LIBS_ROOT}/Putty/windows/winhsock.c
  ${LIBS_ROOT}/Putty/x11fwd.c
  ${LIBS_ROOT}/Putty/conf.c
  ${LIBS_ROOT}/Putty/callback.c
  ${LIBS_ROOT}/Putty/sshshare.c
  ${LIBS_ROOT}/Putty/noshare.c
  ${LIBS_ROOT}/Putty/errsock.c
  ${LIBS_ROOT}/Putty/sshecc.c
  ${LIBS_ROOT}/Putty/sshccp.c
#  ${LIBS_ROOT}/Putty/ldisc.c
)

set(PUTTY_COMPILE_FLAGS "-D_CRTIMP= -DLibrary -D_MT -DMPEXT -DNET_SETUP_DIAGNOSTICS -DNO_SECURITY")

if(MSVC)
set_target_properties(putty
  PROPERTIES
  COMPILE_FLAGS "${PUTTY_COMPILE_FLAGS}"
)
elseif(CMAKE_COMPILER_IS_MINGW)
set_target_properties(putty
  PROPERTIES
  COMPILE_FLAGS "-x c --std=c99 ${PUTTY_COMPILE_FLAGS}"
)
endif()

#-------------------------------------------------------------------------------

add_library(tinyxml2 STATIC
  ${LIBS_ROOT}/tinyxml2/tinyxml2.cpp
)

#-------------------------------------------------------------------------------

add_library(neon STATIC
  ${LIBS_ROOT}/neon/src/ne_alloc.c
  ${LIBS_ROOT}/neon/src/ne_auth.c
  ${LIBS_ROOT}/neon/src/ne_basic.c
  ${LIBS_ROOT}/neon/src/ne_compress.c
  ${LIBS_ROOT}/neon/src/ne_dates.c
  ${LIBS_ROOT}/neon/src/ne_i18n.c
  ${LIBS_ROOT}/neon/src/ne_md5.c
  ${LIBS_ROOT}/neon/src/ne_pkcs11.c
  ${LIBS_ROOT}/neon/src/ne_redirect.c
  ${LIBS_ROOT}/neon/src/ne_request.c
  ${LIBS_ROOT}/neon/src/ne_session.c
  ${LIBS_ROOT}/neon/src/ne_socket.c
  ${LIBS_ROOT}/neon/src/ne_socks.c
  ${LIBS_ROOT}/neon/src/ne_sspi.c
  ${LIBS_ROOT}/neon/src/ne_string.c
  ${LIBS_ROOT}/neon/src/ne_uri.c
  ${LIBS_ROOT}/neon/src/ne_utils.c
  ${LIBS_ROOT}/neon/src/ne_207.c
  ${LIBS_ROOT}/neon/src/ne_xml.c
  ${LIBS_ROOT}/neon/src/ne_xmlreq.c
  ${LIBS_ROOT}/neon/src/ne_oldacl.c
  ${LIBS_ROOT}/neon/src/ne_acl3744.c
  ${LIBS_ROOT}/neon/src/ne_props.c
  ${LIBS_ROOT}/neon/src/ne_locks.c
  ${LIBS_ROOT}/neon/src/ne_openssl.c
)

set(NEON_COMPILE_FLAGS "-D_CRTIMP= -D_LIB -DNOCRYPT ${LIBNEON_DEFS} ${LIBEXPAT_DEFS}")

if(MSVC)
set_target_properties(neon
  PROPERTIES
  COMPILE_FLAGS "/TC ${NEON_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
elseif(CMAKE_COMPILER_IS_MINGW)
set_target_properties(neon
  PROPERTIES
  COMPILE_FLAGS "-x c --std=c99 ${NEON_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
endif()

#-------------------------------------------------------------------------------

add_library(apr STATIC
  ${LIBS_ROOT}/apr/atomic/win32/apr_atomic.c
#  ${LIBS_ROOT}/apr/dso/win32/dso.c
  ${LIBS_ROOT}/apr/file_io/win32/buffer.c
#  ${LIBS_ROOT}/apr/file_io/unix/copy.c
  ${LIBS_ROOT}/apr/file_io/win32/dir.c
  ${LIBS_ROOT}/apr/file_io/unix/fileacc.c
  ${LIBS_ROOT}/apr/file_io/win32/filedup.c
  ${LIBS_ROOT}/apr/file_io/win32/filepath.c
  ${LIBS_ROOT}/apr/file_io/unix/filepath_util.c
  ${LIBS_ROOT}/apr/file_io/win32/filestat.c
  ${LIBS_ROOT}/apr/file_io/win32/filesys.c
  ${LIBS_ROOT}/apr/file_io/win32/flock.c
  ${LIBS_ROOT}/apr/file_io/unix/fullrw.c
  ${LIBS_ROOT}/apr/file_io/unix/mktemp.c
  ${LIBS_ROOT}/apr/file_io/win32/open.c
  ${LIBS_ROOT}/apr/file_io/win32/pipe.c
  ${LIBS_ROOT}/apr/file_io/win32/readwrite.c
  ${LIBS_ROOT}/apr/file_io/win32/seek.c
#  ${LIBS_ROOT}/apr/file_io/unix/tempdir.c
  ${LIBS_ROOT}/apr/locks/win32/proc_mutex.c
  ${LIBS_ROOT}/apr/locks/win32/thread_cond.c
  ${LIBS_ROOT}/apr/locks/win32/thread_mutex.c
  ${LIBS_ROOT}/apr/locks/win32/thread_rwlock.c
  ${LIBS_ROOT}/apr/memory/unix/apr_pools.c
  ${LIBS_ROOT}/apr/misc/win32/apr_app.c
  ${LIBS_ROOT}/apr/misc/win32/charset.c
#  ${LIBS_ROOT}/apr/misc/win32/env.c
  ${LIBS_ROOT}/apr/misc/unix/errorcodes.c
#  ${LIBS_ROOT}/apr/misc/unix/getopt.c
  ${LIBS_ROOT}/apr/misc/win32/internal.c
  ${LIBS_ROOT}/apr/misc/win32/misc.c
#  ${LIBS_ROOT}/apr/misc/unix/otherchild.c
  ${LIBS_ROOT}/apr/misc/win32/rand.c
  ${LIBS_ROOT}/apr/misc/win32/start.c
  ${LIBS_ROOT}/apr/misc/win32/utf8.c
#  ${LIBS_ROOT}/apr/misc/unix/version.c
  ${LIBS_ROOT}/apr/mmap/unix/common.c
#  ${LIBS_ROOT}/apr/mmap/win32/mmap.c
  ${LIBS_ROOT}/apr/network_io/unix/inet_ntop.c
  ${LIBS_ROOT}/apr/network_io/unix/inet_pton.c
#  ${LIBS_ROOT}/apr/network_io/unix/multicast.c
  ${LIBS_ROOT}/apr/network_io/win32/sendrecv.c
  ${LIBS_ROOT}/apr/network_io/unix/sockaddr.c
  ${LIBS_ROOT}/apr/network_io/win32/sockets.c
  ${LIBS_ROOT}/apr/network_io/unix/socket_util.c
  ${LIBS_ROOT}/apr/network_io/win32/sockopt.c
#  ${LIBS_ROOT}/apr/passwd/apr_getpass.c
#  ${LIBS_ROOT}/apr/poll/unix/poll.c
#  ${LIBS_ROOT}/apr/poll/unix/pollcb.c
#  ${LIBS_ROOT}/apr/poll/unix/pollset.c
#  ${LIBS_ROOT}/apr/poll/unix/select.c
#  ${LIBS_ROOT}/apr/poll/unix/z_asio.c
  ${LIBS_ROOT}/apr/random/unix/apr_random.c
#  ${LIBS_ROOT}/apr/random/unix/sha2.c
#  ${LIBS_ROOT}/apr/random/unix/sha2_glue.c
#  ${LIBS_ROOT}/apr/shmem/win32/shm.c
  ${LIBS_ROOT}/apr/strings/apr_cpystrn.c
  ${LIBS_ROOT}/apr/strings/apr_fnmatch.c
  ${LIBS_ROOT}/apr/strings/apr_snprintf.c
  ${LIBS_ROOT}/apr/strings/apr_strings.c
  ${LIBS_ROOT}/apr/strings/apr_strnatcmp.c
  ${LIBS_ROOT}/apr/strings/apr_strtok.c
  ${LIBS_ROOT}/apr/tables/apr_hash.c
  ${LIBS_ROOT}/apr/tables/apr_tables.c
#  ${LIBS_ROOT}/apr/tables/apr_skiplist.c
  ${LIBS_ROOT}/apr/threadproc/win32/proc.c
  ${LIBS_ROOT}/apr/threadproc/win32/signals.c
  ${LIBS_ROOT}/apr/threadproc/win32/thread.c
  ${LIBS_ROOT}/apr/threadproc/win32/threadpriv.c
  ${LIBS_ROOT}/apr/time/win32/time.c
  ${LIBS_ROOT}/apr/time/win32/timestr.c
#  ${LIBS_ROOT}/apr/user/win32/groupinfo.c
  ${LIBS_ROOT}/apr/user/win32/userinfo.c
)

set_target_properties(apr
  PROPERTIES
  COMPILE_FLAGS "-D_CRTIMP= -D_LIB -DAPR_DECLARE_STATIC -DAPR_HAS_LARGE_FILES -U_UNICODE -UUNICODE"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

#-------------------------------------------------------------------------------

add_library(expat STATIC
  ${LIBS_ROOT}/expat/lib/xmlparse.c
  ${LIBS_ROOT}/expat/lib/xmlrole.c
  ${LIBS_ROOT}/expat/lib/xmltok.c
  ${LIBS_ROOT}/expat/lib/xmltok_impl.c
#  ${LIBS_ROOT}/expat/lib/xmltok_ns.c
)

set(EXPAT_COMPILE_FLAGS "-D_CRTIMP= -D_LIB ${LIBEXPAT_DEFS}")

if(MSVC)
set_target_properties(expat
  PROPERTIES
  COMPILE_FLAGS "/TC ${EXPAT_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
elseif(CMAKE_COMPILER_IS_MINGW)
set_target_properties(expat
  PROPERTIES
  COMPILE_FLAGS "-x c --std=c99 ${EXPAT_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
endif()

#-------------------------------------------------------------------------------

add_library(zlib STATIC
  ${LIBS_ROOT}/zlib/src/adler32.c
#  ${LIBS_ROOT}/zlib/src/compress.c
  ${LIBS_ROOT}/zlib/src/crc32.c
  ${LIBS_ROOT}/zlib/src/deflate.c
#  ${LIBS_ROOT}/zlib/src/gzclose.c
#  ${LIBS_ROOT}/zlib/src/gzlib.c
#  ${LIBS_ROOT}/zlib/src/gzread.c
#  ${LIBS_ROOT}/zlib/src/gzwrite.c
  ${LIBS_ROOT}/zlib/src/infback.c
  ${LIBS_ROOT}/zlib/src/inffast.c
  ${LIBS_ROOT}/zlib/src/inflate.c
  ${LIBS_ROOT}/zlib/src/inftrees.c
  ${LIBS_ROOT}/zlib/src/trees.c
#  ${LIBS_ROOT}/zlib/src/uncompr.c
  ${LIBS_ROOT}/zlib/src/zutil.c
  ${LIBS_ROOT}/zlib/src/match.c
)

set_target_properties(zlib
  PROPERTIES
  COMPILE_FLAGS "-D_CRTIMP= -D_LIB"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

#-------------------------------------------------------------------------------

add_library(dlmalloc STATIC
  ${LIBS_ROOT}/dlmalloc/malloc.c
  ${LIBS_ROOT}/dlmalloc/malloc-2.8.6.h
)

set_target_properties(dlmalloc
  PROPERTIES
  COMPILE_FLAGS "-D_CRTIMP= -D_LIB ${DLMALLOC_DEFS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

#-------------------------------------------------------------------------------

add_library(rdestl STATIC
  ${LIBS_ROOT}/rdestl/allocator.cpp
  ${LIBS_ROOT}/rdestl/intrusive_list.cpp
  ${LIBS_ROOT}/rdestl/list.cpp
#  ${LIBS_ROOT}/rdestl/intrusive_slist.cpp
#  ${LIBS_ROOT}/rdestl/slist.cpp
  ${LIBS_ROOT}/rdestl/sort.h
  ${LIBS_ROOT}/rdestl/string_utils.h
#  ${LIBS_ROOT}/rdestl/intrusive_list.h
  ${LIBS_ROOT}/rdestl/sorted_vector.h
  ${LIBS_ROOT}/rdestl/allocator.h
  ${LIBS_ROOT}/rdestl/hash_map.h
  ${LIBS_ROOT}/rdestl/map.h
  ${LIBS_ROOT}/rdestl/basic_string.h
  ${LIBS_ROOT}/rdestl/list.h
  ${LIBS_ROOT}/rdestl/type_traits.h
#  ${LIBS_ROOT}/rdestl/intrusive_slist.h
  ${LIBS_ROOT}/rdestl/radix_sorter.h
  ${LIBS_ROOT}/rdestl/stack_allocator.h
  ${LIBS_ROOT}/rdestl/utility.h
  ${LIBS_ROOT}/rdestl/vector.h
#  ${LIBS_ROOT}/rdestl/slist.h
  ${LIBS_ROOT}/rdestl/fixed_array.h
  ${LIBS_ROOT}/rdestl/pair.h
  ${LIBS_ROOT}/rdestl/algorithm.h
  ${LIBS_ROOT}/rdestl/fixed_list.h
  ${LIBS_ROOT}/rdestl/hash.h
  ${LIBS_ROOT}/rdestl/alignment.h
  ${LIBS_ROOT}/rdestl/fixed_vector.h
  ${LIBS_ROOT}/rdestl/rb_tree.h
  ${LIBS_ROOT}/rdestl/buffer_allocator.h
  ${LIBS_ROOT}/rdestl/sstream.h
  ${LIBS_ROOT}/rdestl/rdestl_common.h
  ${LIBS_ROOT}/rdestl/stack.h
  ${LIBS_ROOT}/rdestl/cow_string_storage.h
  ${LIBS_ROOT}/rdestl/fixed_sorted_vector.h
  ${LIBS_ROOT}/rdestl/functional.h
  ${LIBS_ROOT}/rdestl/simple_string_storage.h
  ${LIBS_ROOT}/rdestl/fixed_substring.h
  ${LIBS_ROOT}/rdestl/int_to_type.h
  ${LIBS_ROOT}/rdestl/rde_string.h
  ${LIBS_ROOT}/rdestl/rdestl.h
  ${LIBS_ROOT}/rdestl/set.h
  ${LIBS_ROOT}/rdestl/iterator.h
)

set_target_properties(rdestl
  PROPERTIES
  COMPILE_FLAGS ""
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

#-------------------------------------------------------------------------------

if(DEFINED OPT_COMPILE_TESTS)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")

set(TESTS_ROOT ${SRC_ROOT}/tests)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_DISABLE_ASSERTS")

include_directories(
  ${LIBS_ROOT}/boost
)

if(CONF STREQUAL "x64")
set(BOOST_LINK_DIR ${LIBS_ROOT}/boost/stage/x64)
else()
set(BOOST_LINK_DIR ${LIBS_ROOT}/boost/stage)
endif()
link_directories(${BOOST_LINK_DIR})

#-------------------------------------------------------
# target testnetbox_01

set(TESTNETBOX_01_DIR ${TESTS_ROOT})

set(TESTNETBOX_01_SOURCES
  ${SRC_ROOT}/base/Classes.cpp
  ${SRC_ROOT}/base/Sysutils.cpp
  ${SRC_ROOT}/base/UnicodeString.cpp
  ${SRC_ROOT}/base/local.cpp
  ${SRC_ROOT}/core/Common.cpp
  ${SRC_ROOT}/core/Exceptions.cpp
  ${SRC_ROOT}/core/FileBuffer.cpp
  ${TESTNETBOX_01_DIR}/testnetbox_01.cpp
  ${SRC_ROOT}/resource/TextsCore1.rc
  ${SRC_ROOT}/resource/rtlconsts.rc
)

add_executable(testnetbox_01 ${TESTNETBOX_01_SOURCES})

set_target_properties(testnetbox_01
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

add_dependencies(testnetbox_01
  atlmfc
  putty
  tinyxml2
)

target_link_libraries(testnetbox_01 ${NETBOX_LIBRARIES})

#-------------------------------------------------------
# target testnetbox_02

set(TESTNETBOX_02_DIR ${TESTS_ROOT})

set(TESTNETBOX_02_SOURCES
  ${SRC_ROOT}/NetBox/UnityBuildCore.cpp
  ${SRC_ROOT}/NetBox/UnityBuildMain.cpp
  ${SRC_ROOT}/NetBox/UnityBuildFilezilla.cpp
  ${TESTNETBOX_02_DIR}/testnetbox_02.rc
  ${TESTNETBOX_02_DIR}/testnetbox_02.cpp
  ${SRC_ROOT}/resource/TextsCore1.rc
  ${SRC_ROOT}/resource/rtlconsts.rc
)

add_executable(testnetbox_02 ${TESTNETBOX_02_SOURCES})

set_target_properties(testnetbox_02
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS} /verbose:lib"
)

add_dependencies(testnetbox_02
  atlmfc
  putty
  tinyxml2
  neon
  apr
  expat
  zlib
)

target_link_libraries(testnetbox_02 ${NETBOX_LIBRARIES})

#-------------------------------------------------------
# target testnetbox_03

set(TESTNETBOX_03_DIR ${TESTS_ROOT})

set(TESTNETBOX_03_SOURCES
  ${SRC_ROOT}/NetBox/UnityBuildCore.cpp
  ${SRC_ROOT}/NetBox/UnityBuildMain.cpp
  ${SRC_ROOT}/NetBox/UnityBuildFilezilla.cpp
  ${TESTNETBOX_03_DIR}/testnetbox_03.cpp
  ${SRC_ROOT}/resource/TextsCore1.rc
  ${SRC_ROOT}/resource/rtlconsts.rc
)

add_executable(testnetbox_03 ${TESTNETBOX_03_SOURCES})

set_target_properties(testnetbox_03
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

add_dependencies(testnetbox_03
  putty
  tinyxml2
  neon
  apr
  expat
  zlib
)

target_link_libraries(testnetbox_03 ${NETBOX_LIBRARIES})

#-------------------------------------------------------
# target testnetbox_04

set(TESTNETBOX_04_DIR ${TESTS_ROOT})

set(TESTNETBOX_04_SOURCES
  ${SRC_ROOT}/NetBox/UnityBuildCore.cpp
  ${SRC_ROOT}/NetBox/UnityBuildMain.cpp
  ${SRC_ROOT}/NetBox/UnityBuildFilezilla.cpp
  ${TESTNETBOX_04_DIR}/testnetbox_04.cpp
  ${SRC_ROOT}/resource/TextsCore1.rc
  ${SRC_ROOT}/resource/rtlconsts.rc
)

add_executable(testnetbox_04 ${TESTNETBOX_04_SOURCES})

set_target_properties(testnetbox_04
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS} /verbose:lib"
)

add_dependencies(testnetbox_04 ${NETBOX_LIBRARIES}
  calculator_dll
)

target_link_libraries(testnetbox_04 ${NETBOX_LIBRARIES}
)

endif(CMAKE_BUILD_TYPE STREQUAL "Debug")
endif(DEFINED OPT_COMPILE_TESTS)
