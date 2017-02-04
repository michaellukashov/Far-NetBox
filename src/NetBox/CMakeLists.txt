message(STATUS "CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 2.8.12)

project(NetBox)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type to Debug" FORCE)
endif()
message(STATUS "Build type -- ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CL_64 -- ${CMAKE_CL_64}")
message(STATUS "MSVC10 -- ${MSVC10}")

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
    set(OPENSSL_INC_DIR "vs2015-${PROJECT_PLATFORM}")
  else()
    set(OPENSSL_INC_DIR "${PROJECT_PLATFORM}")
  endif()
else()
  set(OPENSSL_INC_DIR "${PROJECT_PLATFORM}")
endif(MSVC)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(BUILD_DEBUG 1)
else()
  set(BUILD_DEBUG 0)
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
message(STATUS "System: ${CMAKE_SYSTEM_NAME}, PROJECT_PLATFORM: ${PROJECT_PLATFORM}, BUILD_DIR: ${BUILD_DIR}, CMAKE_COMPILER_IS_MINGW = ${CMAKE_COMPILER_IS_MINGW}")
message(STATUS "MSVC_VERSION: ${MSVC_VERSION}")

set(LIBS_ROOT ${PROJECT_ROOT}/libs)
set(SRC_ROOT ${PROJECT_ROOT}/src)

#-------------------------------------------------------------------------------

set(LIBNEON_DEFS "-DNE_LFS -DNE_HAVE_SSL -DHAVE_OPENSSL -DHAVE_EXPAT -DHAVE_EXPAT_H -DNE_HAVE_DAV -DNE_HAVE_ZLIB") #" -DUSE_GETADDRINFO")
set(LIBEXPAT_DEFS "-DCOMPILED_FROM_DSP -DXML_STATIC")
set(DLMALLOC_DEFS "-DUSE_DLMALLOC -DUSE_DL_PREFIX -DNO_MALLINFO -DUSE_LOCKS=1")
set(NETBOX_DEFS "-DMPEXT -DWINSCP -DSTRICT -DNOCRYPT -DOPENSSL_NO_LOCKING")
set(NETBOX_DEFS "${NETBOX_DEFS} -D_CRTIMP= -D_LIB -D_WINDOWS -DWIN32 -D_WIN32_WINNT=0x0501")
set(NETBOX_DEFS "${NETBOX_DEFS} -D_SCL_SECURE_NO_WARNINGS -DAPR_DECLARE_STATIC -DAPR_HAS_LARGE_FILES")
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
set(NETBOX_UNICODE_FLAGS_STRING "-D_UNICODE -DUNICODE")

if(MSVC)

set(NETBOX_C_WARNING_FLAGS "")
set(NETBOX_CXX_WARNING_FLAGS "")
set(NETBOX_CXX_FLAGS "/MP2 /Zi")
set(NETBOX_FLAGS_RELEASE "${NETBOX_FLAGS_RELEASE} /Gm- /MT -Ox -Ob1 -Oi -Os -GF -GS- -Gy /fp:fast")
set(NETBOX_FLAGS_DEBUG "${NETBOX_FLAGS_DEBUG} /Gm- /Od /MTd /GS /RTC1")

elseif(CMAKE_COMPILER_IS_MINGW)

set(NETBOX_C_WARNING_FLAGS "")
set(NETBOX_CXX_WARNING_FLAGS "-Wno-attributes -Wno-write-strings -Wno-unknown-pragmas -Wno-old-style-cast -Wno-effc++ -Wno-unknown-pragmas -Wno-unused-value")
set(NETBOX_CXX_FLAGS "${NETBOX_CXX_FLAGS} -std=c++11 -Woverloaded-virtual -Wnon-virtual-dtor")
set(NETBOX_C_FLAGS "${NETBOX_C_FLAGS} -static -s -O2 -Wall -Wextra -pedantic -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code")
#-Weffc++
set(NETBOX_FLAGS_RELEASE "${NETBOX_FLAGS_RELEASE}")
set(NETBOX_FLAGS_DEBUG "${NETBOX_FLAGS_DEBUG}")

endif()

set(NETBOX_C_FLAGS "${NETBOX_DEFS} ${NETBOX_C_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_C_WARNING_FLAGS}")
set(NETBOX_CXX_FLAGS "${NETBOX_DEFS} ${NETBOX_CXX_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_CXX_WARNING_FLAGS} ${NETBOX_UNICODE_FLAGS_STRING}")

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
  /MAP
  /INCREMENTAL:NO
  /ignore:4099
"
)

set(NETBOX_DLL_LINK_FLAGS_RELEASE "/OPT:REF ${NETBOX_DLL_LINK_FLAGS}")
set(NETBOX_DLL_LINK_FLAGS_DEBUG "/DEBUG ${NETBOX_DLL_LINK_FLAGS}")

elseif(CMAKE_COMPILER_IS_MINGW)
set(NETBOX_DLL_LINK_FLAGS "")
set(NETBOX_DLL_LINK_FLAGS_RELEASE "${NETBOX_DLL_LINK_FLAGS}")
set(NETBOX_DLL_LINK_FLAGS_DEBUG "${NETBOX_DLL_LINK_FLAGS}")
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
  ${LIBS_ROOT}/zlib/src
  ${LIBS_ROOT}/expat/lib
  ${LIBS_ROOT}/apr/include
  ${LIBS_ROOT}/apr/include/arch/win32
  ${LIBS_ROOT}/apr/include/arch/unix
  ${LIBS_ROOT}/rdestl

  ${LIBS_ROOT}/openssl/${OPENSSL_INC_DIR}/include
)

#-------------------------------------------------------------------------------
add_library (libcrypto STATIC IMPORTED)
set_property(TARGET libcrypto
    PROPERTY IMPORTED_LOCATION ${PROJECT_ROOT}/libs/openssl/${OPENSSL_INC_DIR}/libcrypto.lib)
add_library (libssl STATIC IMPORTED)
set_property(TARGET libssl
    PROPERTY IMPORTED_LOCATION ${PROJECT_ROOT}/libs/openssl/${OPENSSL_INC_DIR}/libssl.lib)

#-------------------------------------------------------------------------------
# target NetBox

set(NETBOX_SOURCES
  UnityBuildCore.cpp
  UnityBuildMain.cpp
  UnityBuildFilezilla.cpp

  FarPlugin.h
  WinSCPPlugin.h
  FarDialog.h
  resource.h
  WinSCPFileSystem.h
  FarInterface.h
  FarConfiguration.h
  XmlStorage.h
  FarUtil.h
  FarPluginStrings.h

  ../base/nbglobals.h
  ../base/Exceptions.h
  ../base/MsgIDs.h
  ../base/Global.h
  ../base/Common.h
  ../base/vcl.h
  ../base/FileBuffer.h
  ../base/FastDelegateBind.h
  ../base/FastDelegate.h

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
  ${SRC_ROOT}/NetBox/vc_crt_fix_impl.cpp
  ${ASM_OBJECTS}
)

endif(NOT x64)
endif(MSVC)

#-------------------------------------------------------------------------------

set(NETBOX_LIBRARIES
  atlmfc
  putty
  zlib
  tinyxml2
  libssl libcrypto
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
if(BUILD_DEBUG)
set(NETBOX_LIBRARIES ${NETBOX_LIBRARIES}
  libcmtd.lib
  libcpmtd.lib
)
else()
set(NETBOX_LIBRARIES ${NETBOX_LIBRARIES}
  libcmt.lib
  libcpmt.lib
)
endif(BUILD_DEBUG)
endif(MSVC)

add_library(NetBox SHARED
  ${NETBOX_SOURCES}
)

set(NETBOX_DLL_CXX_FLAGS )
if(CMAKE_COMPILER_IS_MINGW)
  set(NETBOX_DLL_CXX_FLAGS -fpermissive)
endif()

target_compile_options(NetBox
  PRIVATE ${NETBOX_DLL_CXX_FLAGS} -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS
)

set_target_properties(NetBox
  PROPERTIES
  LINK_FLAGS "${NETBOX_DLL_LINK_FLAGS}"
  LINK_FLAGS_RELEASE "${NETBOX_DLL_LINK_FLAGS_RELEASE}"
  LINK_FLAGS_DEBUG "${NETBOX_DLL_LINK_FLAGS_DEBUG}"
)

#-------------------------------------------------------------------------------

#TODO: COMMAND create_ver.py

target_link_libraries(NetBox
  ${NETBOX_LIBRARIES}
)

add_dependencies(NetBox
  atlmfc
  putty
  zlib
  tinyxml2
  libssl libcrypto
  neon
  apr
  expat
  dlmalloc
  rdestl
)

set(NETBOX_PLUGIN_DIR ${PROJECT_ROOT}/${FAR_VERSION}_${PROJECT_PLATFORM}/Plugins/NetBox)
string(REPLACE "/" "\\" NETBOX_PLUGIN_DIR ${NETBOX_PLUGIN_DIR})

add_custom_command(TARGET NetBox
  POST_BUILD
  COMMAND if not exist "${NETBOX_PLUGIN_DIR}" ( ${CMAKE_COMMAND} -E make_directory "${NETBOX_PLUGIN_DIR}" )
  COMMAND if exist NetBox.dll ${CMAKE_COMMAND} -E copy NetBox.dll ${NETBOX_PLUGIN_DIR}
  COMMAND if exist NetBox.pdb ( ${CMAKE_COMMAND} -E copy NetBox.pdb ${NETBOX_PLUGIN_DIR} )
  COMMAND if exist NetBox.map ( ${CMAKE_COMMAND} -E copy NetBox.map ${NETBOX_PLUGIN_DIR} )
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/NetBoxRus.lng
    ${CMAKE_CURRENT_SOURCE_DIR}/NetBoxEng.lng
    ${NETBOX_PLUGIN_DIR}
  WORKING_DIRECTORY ${BUILD_DIR}
  VERBATIM
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
add_custom_command(TARGET NetBox
  POST_BUILD
  DEPENDS ${BUILD_DIR}\\NetBox.dll
  COMMAND scripts\\make_dist.cmd ${FAR_VERSION} ${PROJECT_PLATFORM}
  WORKING_DIRECTORY ${SRC_ROOT}/NetBox
  VERBATIM
)
endif()

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
set(ATLMFC_COMPILE_FLAGS
  -D_ATL_NO_DEBUG_CRT -D_ATL_NO_UUIDOF -D_ATL_NO_CONNECTION_POINTS -D_ATL_NO_DATETIME_RESOURCES_
  -D_ATL_NO_DEFAULT_LIBS -D_ATL_NO_PERF_SUPPORT -D_AFX_PORTABLE -D_ATL_NO_COMMODULE
  -D_AFX_NO_OLE_SUPPORT -D_AFX_NO_SOCKET_SUPPORT -DCOM_NO_WINDOWS_H
)

if(CMAKE_COMPILER_IS_MINGW)
  set(ATLMFC_COMPILE_FLAGS ${ATLMFC_COMPILE_FLAGS}
    -Wno-int-to-pointer-cast -Wno-pragmas -fpermissive
  )
endif()

target_compile_options(atlmfc
  PRIVATE ${ATLMFC_COMPILE_FLAGS} -wd4995 -wd4996 -wd4101 -wd4290
)

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
  ${LIBS_ROOT}/Putty/windows/winsecur.c
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
  ${LIBS_ROOT}/Putty/import.c
  ${LIBS_ROOT}/Putty/be_misc.c
  ${LIBS_ROOT}/Putty/sshbcrypt.c
#  ${LIBS_ROOT}/Putty/ldisc.c
)

set(PUTTY_COMPILE_FLAGS -DLibrary -D_MT -DMPEXT -DNET_SETUP_DIAGNOSTICS -D_CRT_SECURE_NO_WARNINGS -wd4996)

target_compile_options(putty
  PRIVATE ${PUTTY_COMPILE_FLAGS}
)

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

set(NEON_COMPILE_FLAGS -D_CRT_SECURE_NO_WARNINGS -wd4996)

target_compile_options(neon
  PRIVATE ${NEON_COMPILE_FLAGS} ${NETBOX_UNICODE_FLAGS}
)

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

target_compile_options(apr
  PRIVATE -DAPR_DECLARE_STATIC -DAPR_HAS_LARGE_FILES -U_UNICODE -UUNICODE
)

#-------------------------------------------------------------------------------

add_library(expat STATIC
  ${LIBS_ROOT}/expat/lib/xmlparse.c
  ${LIBS_ROOT}/expat/lib/xmlrole.c
  ${LIBS_ROOT}/expat/lib/xmltok.c
  ${LIBS_ROOT}/expat/lib/xmltok_impl.c
#  ${LIBS_ROOT}/expat/lib/xmltok_ns.c
)

target_compile_options(expat
  PRIVATE
)

#-------------------------------------------------------------------------------

set(ZLIB_SOURCES
  ${LIBS_ROOT}/zlib/src/adler32.c
  ${LIBS_ROOT}/zlib/src/crc32.c
  ${LIBS_ROOT}/zlib/src/infback.c
  ${LIBS_ROOT}/zlib/src/inffast.c
  ${LIBS_ROOT}/zlib/src/inflate.c
  ${LIBS_ROOT}/zlib/src/inftrees.c
  ${LIBS_ROOT}/zlib/src/trees.c
  ${LIBS_ROOT}/zlib/src/zutil.c
  ${LIBS_ROOT}/zlib/src/match.c
  ${LIBS_ROOT}/zlib/src/deflate.c
  ${LIBS_ROOT}/zlib/src/deflate_slow.c
  ${LIBS_ROOT}/zlib/src/deflate_medium.c
  ${LIBS_ROOT}/zlib/src/deflate_fast.c
)

if(NOT x64)

set(ZLIB_SOURCES ${ZLIB_SOURCES}
  ${LIBS_ROOT}/zlib/src/arch/x86/crc_folding.c
  ${LIBS_ROOT}/zlib/src/arch/x86/fill_window_sse.c
  ${LIBS_ROOT}/zlib/src/arch/x86/insert_string_sse.c
  ${LIBS_ROOT}/zlib/src/arch/x86/x86.c
)

if(MSVC)
set(ZLIB_SOURCES ${ZLIB_SOURCES}
  ${LIBS_ROOT}/zlib/src/arch/x86/deflate_quick.c
)
endif(MSVC)

endif(NOT x64)

add_library(zlib STATIC
  ${ZLIB_SOURCES}
)

target_compile_options(zlib
  PRIVATE ${NETBOX_UNICODE_FLAGS}
)

#-------------------------------------------------------------------------------

add_library(dlmalloc STATIC
  ${LIBS_ROOT}/dlmalloc/malloc.c
  ${LIBS_ROOT}/dlmalloc/malloc-2.8.6.h
)

target_compile_options(dlmalloc
  PRIVATE ${NETBOX_UNICODE_FLAGS}
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
