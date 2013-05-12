CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

STRING(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})

#-------------------------------------------------------------------------------

PROJECT(NetBox)

include(CMakeDetermineCXXCompiler)
message("CMAKE_COMPILER_IS_MINGW = ${CMAKE_COMPILER_IS_MINGW}")

IF(NOT DEFINED CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

IF(NOT DEFINED FAR_VERSION OR FAR_VERSION STREQUAL "")
  SET(FAR_VERSION Far3)
ENDIF()

IF(NOT DEFINED CONF)
  SET(CONF "x86")
ENDIF()

IF(CONF STREQUAL "x86")
  SET(PLATFORM "Win32")
ELSE()
  SET(PLATFORM "x64")
ENDIF()

SET(OUTPUT_DIR ${PROJECT_ROOT}/${FAR_VERSION}_${CONF}/Plugins/NetBox)

SET(LIBS_ROOT ${PROJECT_ROOT}/libs)
SET(SRC_ROOT ${PROJECT_ROOT}/src)
SET(TESTS_ROOT ${SRC_ROOT}/tests)

#-------------------------------------------------------------------------------

IF(NOT DEFINED BUILD_DIR OR BUILD_DIR STREQUAL "")
STRING(REPLACE "/" "\\" BUILD_DIR ${PROJECT_ROOT}/build/${CMAKE_BUILD_TYPE}/${CONF})
ENDIF()
MESSAGE("BUILD_DIR: ${BUILD_DIR}")

#-------------------------------------------------------------------------------

SET(LIBNEON_DEFS "-DNE_HAVE_SSL -DHAVE_OPENSSL -DHAVE_EXPAT -DHAVE_EXPAT_H -DNE_HAVE_DAV -DNE_HAVE_ZLIB -DUSE_GETADDRINFO"
)

SET(LIBEXPAT_DEFS "-DCOMPILED_FROM_DSP -DXML_STATIC"
)

SET(DLMALLOC_DEFS "-DUSE_DLMALLOC -DUSE_DL_PREFIX -DNO_MALLINFO -DUSE_LOCKS=1")

SET(CMAKE_C_FLAGS
  "-D_WINDOWS -DWIN32 -D_WINDLL -D_USRDLL -D_SCL_SECURE_NO_WARNINGS -DAPR_DECLARE_STATIC -DAPR_HAS_LARGE_FILES ${LIBEXPAT_DEFS} ${LIBNEON_DEFS} -DMPEXT -DNO_STRICT -DNOCRYPT ${DLMALLOC_DEFS}"
)

IF(PLATFORM STREQUAL "x64")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN64"
  )
ELSE()
ENDIF()

SET(NETBOX_CFLAGS "-DCOM_NO_WINDOWS_H"
)

SET(FLAGS_RELEASE "-U_DEBUG -UNETBOX_DEBUG -DNDEBUG"
)

SET(FLAGS_DEBUG "-D_DEBUG -DDEBUG -DNETBOX_DEBUG"
)

IF(MSVC)

  SET(NETBOX_WARNING_FLAGS "")

  SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /EHsc /MP2 /Zi -D_UNICODE -DUNICODE "
  )

  SET(FLAGS_RELEASE "${FLAGS_RELEASE} /GL /Gm- /MT -Ox -Ob1 -Oi -Os -Oy -GF -GS- -Gy"
  )
  SET(FLAGS_DEBUG "${FLAGS_DEBUG} /Gm- /MTd /GS"
  )

ELSEIF(CMAKE_COMPILER_IS_MINGW)

  SET(NETBOX_CFLAGS "${NETBOX_CFLAGS} -D_AFXDLL -static -s -O2"
  )

  SET(NETBOX_WARNING_FLAGS "-Wno-attributes -Wno-write-strings")

  SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}"
  )

  SET(FLAGS_RELEASE "${FLAGS_RELEASE} ${NETBOX_CFLAGS}"
  )

  SET(FLAGS_DEBUG "${FLAGS_DEBUG} ${NETBOX_CFLAGS}"
  )

ENDIF()

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${NETBOX_WARNING_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NETBOX_WARNING_FLAGS}")

SET(CMAKE_CXX_FLAGS_RELEASE "${FLAGS_RELEASE}")

SET(CMAKE_C_FLAGS_RELEASE "${FLAGS_RELEASE}")

SET(CMAKE_CXX_FLAGS_DEBUG "${FLAGS_DEBUG}")

SET(CMAKE_C_FLAGS_DEBUG "${FLAGS_DEBUG}")

IF(MSVC)
SET(CMAKE_LINK_FLAGS
  "/NODEFAULTLIB:MSVCURT.LIB
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
  /MANIFEST:NO
  /TLBID:1 /DYNAMICBASE /SUBSYSTEM:WINDOWS
  /MAP /OPT:REF /OPT:ICF /LTCG
  /INCREMENTAL:NO
"
)

ELSEIF(CMAKE_COMPILER_IS_MINGW)
SET(CMAKE_LINK_FLAGS ""
)
ENDIF()

INCLUDE(${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake)
MESSAGE("system: ${CMAKE_SYSTEM_NAME}")

#-------------------------------------------------------

INCLUDE_DIRECTORIES(
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
  ${LIBS_ROOT}/Putty/charset
  ${LIBS_ROOT}/tinyxml2
  ${LIBS_ROOT}/openssl/${CONF}/inc32
  ${LIBS_ROOT}/zlib/src
  ${LIBS_ROOT}/expat/lib
  ${LIBS_ROOT}/neon/src
  ${LIBS_ROOT}/apr/include
  ${LIBS_ROOT}/apr/include/arch/win32
  ${LIBS_ROOT}/apr/include/arch/unix
  ${LIBS_ROOT}/rdestl
)

#-------------------------------------------------------------------------------
# target NetBox

SET(NETBOX_SOURCES
  ${SRC_ROOT}/NetBox/UnityBuildCore.cpp
  ${SRC_ROOT}/NetBox/UnityBuildMain.cpp
  ${SRC_ROOT}/NetBox/UnityBuildFilezilla.cpp
  ${SRC_ROOT}/resource/TextsCore1.rc
  ${SRC_ROOT}/resource/TextsCore2.rc
  ${SRC_ROOT}/resource/rtlconsts.rc
  ${SRC_ROOT}/resource/TextsFileZilla.rc
  ${SRC_ROOT}/NetBox/NetBox.rc
  ${SRC_ROOT}/NetBox/NetBox.def
)

IF(MSVC)
IF(PLATFORM STREQUAL "Win32")
  FIND_PROGRAM (MASM_EXECUTABLE ml)
  # message("masm: ${MASM_EXECUTABLE}")
  SET(ASM_OBJECTS)
  FOREACH(src vc10)
    SET(ASM_SOURCE ${SRC_ROOT}/NetBox/${src}.asm)
    SET(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${src}.obj)
    SET(ASM_OBJECTS ${ASM_OBJECTS} ${ASM_OBJECT})
    ADD_CUSTOM_COMMAND(
      OUTPUT ${ASM_OBJECT}
      COMMAND ${MASM_EXECUTABLE}
      ARGS   /c /Fo ${ASM_OBJECT} ${ASM_SOURCE}
      DEPENDS ${ASM_SOURCE}
    )
  ENDFOREACH(src)

  SET(NETBOX_SOURCES ${NETBOX_SOURCES}
    ${SRC_ROOT}/NetBox/vc10wrapper.cpp
    ${ASM_OBJECTS}
  )
ENDIF(PLATFORM STREQUAL "Win32")
ENDIF(MSVC)

ADD_LIBRARY(libeay32 STATIC IMPORTED)
SET_PROPERTY(TARGET libeay32
  # PROPERTY IMPORTED_IMPLIB ${LIBS_ROOT}/openssl/${CONF}/libeay32.lib)
  PROPERTY IMPORTED_LOCATION ${LIBS_ROOT}/openssl/${CONF}/libeay32.lib
)

ADD_LIBRARY(ssleay32 STATIC IMPORTED)
SET_PROPERTY(TARGET ssleay32
  PROPERTY IMPORTED_LOCATION ${LIBS_ROOT}/openssl/${CONF}/ssleay32.lib
)

#-------------------------------------------------------------------------------

SET(NETBOX_LIBRARIES
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

IF(MSVC)
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  SET(NETBOX_LIBRARIES ${NETBOX_LIBRARIES}
    libcmtd.lib
    libcpmtd.lib
  )
ELSE()
  SET(NETBOX_LIBRARIES ${NETBOX_LIBRARIES}
    libcmt.lib
    libcpmt.lib
  )
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
ENDIF(MSVC)

ADD_LIBRARY(NetBox SHARED ${NETBOX_SOURCES})

IF(MSVC)
SET_TARGET_PROPERTIES(NetBox
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
ELSEIF(CMAKE_COMPILER_IS_MINGW)
SET_TARGET_PROPERTIES(NetBox
  PROPERTIES
  COMPILE_FLAGS "-fpermissive ${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
ENDIF(MSVC)

#-------------------------------------------------------------------------------

#TODO: COMMAND create_ver.py

TARGET_LINK_LIBRARIES(NetBox ${NETBOX_LIBRARIES})

ADD_DEPENDENCIES(NetBox
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

# USE_PRECOMPILED_HEADER(NETBOX_SOURCES stdafx.h stdafx.cpp netbox)

STRING(REPLACE "/" "\\" OUTPUT_DIR ${OUTPUT_DIR})

ADD_CUSTOM_COMMAND(TARGET NetBox
  POST_BUILD
  COMMAND if not exist "${OUTPUT_DIR}" ( mkdir "${OUTPUT_DIR}" )
  COMMAND cp -f NetBox.dll ${OUTPUT_DIR}
  COMMAND if exist NetBox.pdb ( cp -f NetBox.pdb ${OUTPUT_DIR} )
  WORKING_DIRECTORY ${BUILD_DIR}
  VERBATIM
)

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
  ADD_CUSTOM_COMMAND(TARGET NetBox
    POST_BUILD
    DEPENDS ${BUILD_DIR}\\NetBox.dll
    COMMAND scripts\\make_dist.cmd ${FAR_VERSION} ${CONF}
    WORKING_DIRECTORY ${SRC_ROOT}/NetBox
    VERBATIM
  )
ENDIF()

#-------------------------------------------------------------------------------

ADD_LIBRARY(atlmfc STATIC
  ${LIBS_ROOT}/atlmfc/mfc1.cpp
  ${LIBS_ROOT}/atlmfc/mfc2.cpp
  ${LIBS_ROOT}/atlmfc/mfc3.cpp
  ${LIBS_ROOT}/atlmfc/mfc4.cpp
  ${LIBS_ROOT}/atlmfc/mfc5.cpp
  ${LIBS_ROOT}/atlmfc/src/mfc/thrdcore.cpp
  # ${LIBS_ROOT}/atlmfc/include/afxres.rc
  # ${LIBS_ROOT}/atlmfc/include/mfcdll.rc
)

#-------------------------------------------------------------------------------
SET(ATLMFC_COMPILE_FLAGS "-D_WIN32_WINNT=0x0501 -D_ATL_NO_DEBUG_CRT -D_ATL_NO_UUIDOF -D_ATL_NO_CONNECTION_POINTS -D_ATL_NO_DATETIME_RESOURCES_ -D_ATL_NO_DEFAULT_LIBS -D_ATL_NO_PERF_SUPPORT -D_AFX_PORTABLE -D_ATL_NO_COMMODULE -D_AFX_NO_OLE_SUPPORT"
)

IF(MSVC)
SET_TARGET_PROPERTIES(atlmfc
  PROPERTIES
  COMPILE_FLAGS "${ATLMFC_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
ELSEIF(CMAKE_COMPILER_IS_MINGW)
SET_TARGET_PROPERTIES(atlmfc
  PROPERTIES
  COMPILE_FLAGS "-Wno-int-to-pointer-cast -Wno-pragmas -fpermissive ${ATLMFC_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
ENDIF()

#-------------------------------------------------------------------------------

ADD_LIBRARY(putty STATIC
  ${LIBS_ROOT}/Putty/CPROXY.C
  ${LIBS_ROOT}/Putty/INT64.C
  ${LIBS_ROOT}/Putty/LOGGING.C
  ${LIBS_ROOT}/Putty/MISC.C
  ${LIBS_ROOT}/Putty/PGSSAPI.C
  ${LIBS_ROOT}/Putty/PORTFWD_.C
  ${LIBS_ROOT}/Putty/PROXY.C
  ${LIBS_ROOT}/Putty/SSH_.C
  ${LIBS_ROOT}/Putty/SSHAES_.C
  ${LIBS_ROOT}/Putty/SSHARCF.C
  ${LIBS_ROOT}/Putty/SSHBLOWF.C
  ${LIBS_ROOT}/Putty/SSHBN.C
  ${LIBS_ROOT}/Putty/SSHCRC.C
  ${LIBS_ROOT}/Putty/SSHCRCDA.C
  ${LIBS_ROOT}/Putty/SSHDES.C
  ${LIBS_ROOT}/Putty/SSHDH.C
  ${LIBS_ROOT}/Putty/SSHDSS.C
  ${LIBS_ROOT}/Putty/SSHGSSC.C
  ${LIBS_ROOT}/Putty/SSHMD5.C
  ${LIBS_ROOT}/Putty/SSHPUBK.C
  ${LIBS_ROOT}/Putty/SSHRAND.C
  ${LIBS_ROOT}/Putty/SSHRSA.C
  ${LIBS_ROOT}/Putty/SSHSH256.C
  ${LIBS_ROOT}/Putty/SSHSH512.C
  ${LIBS_ROOT}/Putty/SSHSHA_.C
  ${LIBS_ROOT}/Putty/SSHZLIB.C
  ${LIBS_ROOT}/Putty/TREE234.C
  ${LIBS_ROOT}/Putty/CHARSET/UTF8.C
  ${LIBS_ROOT}/Putty/WILDCARD.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINGSS.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINHANDL.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINMISC.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINNET.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINNOISE.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINNOJMP.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINPGNTC.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINPROXY.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINSTORE_.C
  ${LIBS_ROOT}/Putty/WINDOWS/WINTIME.C
  ${LIBS_ROOT}/Putty/X11FWD.C
)

SET(PUTTY_COMPILE_FLAGS "-D_CRTIMP= -DLibrary -DSECURITY_WIN32 -D_MT -DNET_SETUP_DIAGNOSTICS -DNO_SECURITY -U_UNICODE -UUNICODE"
)

IF(MSVC)
SET_TARGET_PROPERTIES(putty
  PROPERTIES
  COMPILE_FLAGS "/TC ${PUTTY_COMPILE_FLAGS}"
)
ELSEIF(CMAKE_COMPILER_IS_MINGW)
SET_TARGET_PROPERTIES(putty
  PROPERTIES
  COMPILE_FLAGS "-x c --std=c99 ${PUTTY_COMPILE_FLAGS}"
)
ENDIF()

#-------------------------------------------------------------------------------

ADD_LIBRARY(tinyxml2 STATIC
  ${LIBS_ROOT}/tinyxml2/tinyxml2.cpp
)

#-------------------------------------------------------------------------------

ADD_LIBRARY(neon STATIC
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

SET(NEON_COMPILE_FLAGS "-D_CRTIMP= -D_LIB -DNOCRYPT ${LIBNEON_DEFS} ${LIBEXPAT_DEFS}"
)

IF(MSVC)
SET_TARGET_PROPERTIES(neon
  PROPERTIES
  COMPILE_FLAGS "/TC ${NEON_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
ELSEIF(CMAKE_COMPILER_IS_MINGW)
SET_TARGET_PROPERTIES(neon
  PROPERTIES
  COMPILE_FLAGS "-x c --std=c99 ${NEON_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
ENDIF()

#-------------------------------------------------------------------------------

ADD_LIBRARY(apr STATIC
  ${LIBS_ROOT}/apr/atomic/win32/apr_atomic.c
  ${LIBS_ROOT}/apr/dso/win32/dso.c
  ${LIBS_ROOT}/apr/file_io/win32/buffer.c
  ${LIBS_ROOT}/apr/file_io/unix/copy.c
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
  ${LIBS_ROOT}/apr/file_io/unix/tempdir.c
  ${LIBS_ROOT}/apr/locks/win32/proc_mutex.c
  ${LIBS_ROOT}/apr/locks/win32/thread_cond.c
  ${LIBS_ROOT}/apr/locks/win32/thread_mutex.c
  ${LIBS_ROOT}/apr/locks/win32/thread_rwlock.c
  ${LIBS_ROOT}/apr/memory/unix/apr_pools.c
  ${LIBS_ROOT}/apr/misc/win32/apr_app.c
  ${LIBS_ROOT}/apr/misc/win32/charset.c
  ${LIBS_ROOT}/apr/misc/win32/env.c
  ${LIBS_ROOT}/apr/misc/unix/errorcodes.c
  ${LIBS_ROOT}/apr/misc/unix/getopt.c
  ${LIBS_ROOT}/apr/misc/win32/internal.c
  ${LIBS_ROOT}/apr/misc/win32/misc.c
  ${LIBS_ROOT}/apr/misc/unix/otherchild.c
  ${LIBS_ROOT}/apr/misc/win32/rand.c
  ${LIBS_ROOT}/apr/misc/win32/start.c
  ${LIBS_ROOT}/apr/misc/win32/utf8.c
  ${LIBS_ROOT}/apr/misc/unix/version.c
  ${LIBS_ROOT}/apr/mmap/unix/common.c
  ${LIBS_ROOT}/apr/mmap/win32/mmap.c
  ${LIBS_ROOT}/apr/network_io/unix/inet_ntop.c
  ${LIBS_ROOT}/apr/network_io/unix/inet_pton.c
  ${LIBS_ROOT}/apr/network_io/unix/multicast.c
  ${LIBS_ROOT}/apr/network_io/win32/sendrecv.c
  ${LIBS_ROOT}/apr/network_io/unix/sockaddr.c
  ${LIBS_ROOT}/apr/network_io/win32/sockets.c
  ${LIBS_ROOT}/apr/network_io/unix/socket_util.c
  ${LIBS_ROOT}/apr/network_io/win32/sockopt.c
  ${LIBS_ROOT}/apr/passwd/apr_getpass.c
  ${LIBS_ROOT}/apr/poll/unix/poll.c
  ${LIBS_ROOT}/apr/poll/unix/pollcb.c
  ${LIBS_ROOT}/apr/poll/unix/pollset.c
  ${LIBS_ROOT}/apr/poll/unix/select.c
  ${LIBS_ROOT}/apr/random/unix/apr_random.c
  ${LIBS_ROOT}/apr/random/unix/sha2.c
  ${LIBS_ROOT}/apr/random/unix/sha2_glue.c
  ${LIBS_ROOT}/apr/shmem/win32/shm.c
  ${LIBS_ROOT}/apr/strings/apr_cpystrn.c
  ${LIBS_ROOT}/apr/strings/apr_fnmatch.c
  ${LIBS_ROOT}/apr/strings/apr_snprintf.c
  ${LIBS_ROOT}/apr/strings/apr_strings.c
  ${LIBS_ROOT}/apr/strings/apr_strnatcmp.c
  ${LIBS_ROOT}/apr/strings/apr_strtok.c
  ${LIBS_ROOT}/apr/tables/apr_hash.c
  ${LIBS_ROOT}/apr/tables/apr_tables.c
  ${LIBS_ROOT}/apr/threadproc/win32/proc.c
  ${LIBS_ROOT}/apr/threadproc/win32/signals.c
  ${LIBS_ROOT}/apr/threadproc/win32/thread.c
  ${LIBS_ROOT}/apr/threadproc/win32/threadpriv.c
  ${LIBS_ROOT}/apr/time/win32/time.c
  ${LIBS_ROOT}/apr/time/win32/timestr.c
  ${LIBS_ROOT}/apr/user/win32/groupinfo.c
  ${LIBS_ROOT}/apr/user/win32/userinfo.c
)

SET_TARGET_PROPERTIES(apr
  PROPERTIES
  COMPILE_FLAGS "-D_CRTIMP= -D_LIB -DAPR_DECLARE_STATIC -DAPR_HAS_LARGE_FILES -U_UNICODE -UUNICODE"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

#-------------------------------------------------------------------------------

ADD_LIBRARY(expat STATIC
  ${LIBS_ROOT}/expat/lib/xmlparse.c
  ${LIBS_ROOT}/expat/lib/xmlrole.c
  ${LIBS_ROOT}/expat/lib/xmltok.c
  ${LIBS_ROOT}/expat/lib/xmltok_impl.c
  ${LIBS_ROOT}/expat/lib/xmltok_ns.c
)

SET(EXPAT_COMPILE_FLAGS "-D_CRTIMP= -D_LIB ${LIBEXPAT_DEFS}"
)

IF(MSVC)
SET_TARGET_PROPERTIES(expat
  PROPERTIES
  COMPILE_FLAGS "/TC ${EXPAT_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
ELSEIF(CMAKE_COMPILER_IS_MINGW)
SET_TARGET_PROPERTIES(expat
  PROPERTIES
  COMPILE_FLAGS "-x c --std=c99 ${EXPAT_COMPILE_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
ENDIF()

#-------------------------------------------------------------------------------

ADD_LIBRARY(zlib STATIC
  ${LIBS_ROOT}/zlib/src/adler32.c
  ${LIBS_ROOT}/zlib/src/compress.c
  ${LIBS_ROOT}/zlib/src/crc32.c
  ${LIBS_ROOT}/zlib/src/deflate.c
  ${LIBS_ROOT}/zlib/src/gzclose.c
  ${LIBS_ROOT}/zlib/src/gzlib.c
  ${LIBS_ROOT}/zlib/src/gzread.c
  ${LIBS_ROOT}/zlib/src/gzwrite.c
  ${LIBS_ROOT}/zlib/src/infback.c
  ${LIBS_ROOT}/zlib/src/inffast.c
  ${LIBS_ROOT}/zlib/src/inflate.c
  ${LIBS_ROOT}/zlib/src/inftrees.c
  ${LIBS_ROOT}/zlib/src/trees.c
  ${LIBS_ROOT}/zlib/src/uncompr.c
  ${LIBS_ROOT}/zlib/src/zutil.c
)

SET_TARGET_PROPERTIES(zlib
  PROPERTIES
  COMPILE_FLAGS "-D_CRTIMP= -D_LIB"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

#-------------------------------------------------------------------------------

ADD_LIBRARY(dlmalloc STATIC
  ${LIBS_ROOT}/dlmalloc/malloc.c
  ${LIBS_ROOT}/dlmalloc/malloc-2.8.6.h
)

SET_TARGET_PROPERTIES(dlmalloc
  PROPERTIES
  COMPILE_FLAGS "-D_CRTIMP= -D_LIB ${DLMALLOC_DEFS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

#-------------------------------------------------------------------------------

ADD_LIBRARY(rdestl STATIC
  ${LIBS_ROOT}/rdestl/allocator.cpp
  ${LIBS_ROOT}/rdestl/intrusive_list.cpp
  ${LIBS_ROOT}/rdestl/list.cpp
  ${LIBS_ROOT}/rdestl/intrusive_slist.cpp
  ${LIBS_ROOT}/rdestl/slist.cpp
  ${LIBS_ROOT}/rdestl/sort.h
  ${LIBS_ROOT}/rdestl/string_utils.h
  ${LIBS_ROOT}/rdestl/intrusive_list.h
  ${LIBS_ROOT}/rdestl/sorted_vector.h
  ${LIBS_ROOT}/rdestl/allocator.h
  ${LIBS_ROOT}/rdestl/hash_map.h
  ${LIBS_ROOT}/rdestl/map.h
  ${LIBS_ROOT}/rdestl/basic_string.h
  ${LIBS_ROOT}/rdestl/list.h
  ${LIBS_ROOT}/rdestl/type_traits.h
  ${LIBS_ROOT}/rdestl/intrusive_slist.h
  ${LIBS_ROOT}/rdestl/radix_sorter.h
  ${LIBS_ROOT}/rdestl/stack_allocator.h
  ${LIBS_ROOT}/rdestl/utility.h
  ${LIBS_ROOT}/rdestl/vector.h
  ${LIBS_ROOT}/rdestl/slist.h
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

SET_TARGET_PROPERTIES(rdestl
  PROPERTIES
  COMPILE_FLAGS ""
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

#-------------------------------------------------------------------------------

IF(DEFINED COMPILE_TESTS)
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_DISABLE_ASSERTS"
)

INCLUDE_DIRECTORIES(
  ${LIBS_ROOT}/boost
)

IF(PLATFORM STREQUAL "x64")
  SET(BOOST_LINK_DIR ${LIBS_ROOT}/boost/stage/x64)
ELSE()
  SET(BOOST_LINK_DIR ${LIBS_ROOT}/boost/stage)
ENDIF()
LINK_DIRECTORIES(${BOOST_LINK_DIR})

#-------------------------------------------------------
# target testnetbox_01

SET(TESTNETBOX_01_DIR ${TESTS_ROOT})

SET(TESTNETBOX_01_SOURCES
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

ADD_EXECUTABLE(testnetbox_01 ${TESTNETBOX_01_SOURCES})

SET_TARGET_PROPERTIES(testnetbox_01
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

ADD_DEPENDENCIES(testnetbox_01
  atlmfc
  putty
  tinyxml2
)

TARGET_LINK_LIBRARIES(testnetbox_01 ${NETBOX_LIBRARIES})

#-------------------------------------------------------
# target testnetbox_02

SET(TESTNETBOX_02_DIR ${TESTS_ROOT})

SET(TESTNETBOX_02_SOURCES
  ${SRC_ROOT}/NetBox/UnityBuildCore.cpp
  ${SRC_ROOT}/NetBox/UnityBuildMain.cpp
  ${SRC_ROOT}/NetBox/UnityBuildFilezilla.cpp
  ${TESTNETBOX_02_DIR}/testnetbox_02.rc
  ${TESTNETBOX_02_DIR}/testnetbox_02.cpp
  ${SRC_ROOT}/resource/TextsCore1.rc
  ${SRC_ROOT}/resource/rtlconsts.rc
)

ADD_EXECUTABLE(testnetbox_02 ${TESTNETBOX_02_SOURCES})

SET_TARGET_PROPERTIES(testnetbox_02
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS} /verbose:lib"
)

ADD_DEPENDENCIES(testnetbox_02
  atlmfc
  putty
  tinyxml2
  neon
  apr
  expat
  zlib
)

TARGET_LINK_LIBRARIES(testnetbox_02 ${NETBOX_LIBRARIES})

#-------------------------------------------------------
# target testnetbox_03

SET(TESTNETBOX_03_DIR ${TESTS_ROOT})

SET(TESTNETBOX_03_SOURCES
  ${SRC_ROOT}/NetBox/UnityBuildCore.cpp
  ${SRC_ROOT}/NetBox/UnityBuildMain.cpp
  ${SRC_ROOT}/NetBox/UnityBuildFilezilla.cpp
  ${TESTNETBOX_03_DIR}/testnetbox_03.cpp
  ${SRC_ROOT}/resource/TextsCore1.rc
  ${SRC_ROOT}/resource/rtlconsts.rc
)

ADD_EXECUTABLE(testnetbox_03 ${TESTNETBOX_03_SOURCES})

SET_TARGET_PROPERTIES(testnetbox_03
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

ADD_DEPENDENCIES(testnetbox_03
  putty
  tinyxml2
  neon
  apr
  expat
  zlib
)

TARGET_LINK_LIBRARIES(testnetbox_03 ${NETBOX_LIBRARIES})

#-------------------------------------------------------
# target testnetbox_04

SET(TESTNETBOX_04_DIR ${TESTS_ROOT})

SET(TESTNETBOX_04_SOURCES
  ${SRC_ROOT}/NetBox/UnityBuildCore.cpp
  ${SRC_ROOT}/NetBox/UnityBuildMain.cpp
  ${SRC_ROOT}/NetBox/UnityBuildFilezilla.cpp
  ${TESTNETBOX_04_DIR}/testnetbox_04.cpp
  ${SRC_ROOT}/resource/TextsCore1.rc
  ${SRC_ROOT}/resource/rtlconsts.rc
)

ADD_EXECUTABLE(testnetbox_04 ${TESTNETBOX_04_SOURCES})

SET_TARGET_PROPERTIES(testnetbox_04
  PROPERTIES
  COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
  LINK_FLAGS "${CMAKE_LINK_FLAGS} /verbose:lib"
)

ADD_DEPENDENCIES(testnetbox_04 ${NETBOX_LIBRARIES}
  calculator_dll
)

TARGET_LINK_LIBRARIES(testnetbox_04 ${NETBOX_LIBRARIES}
)

ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
ENDIF(DEFINED COMPILE_TESTS)
