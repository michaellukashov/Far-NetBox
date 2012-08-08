cmake_minimum_required (VERSION 2.8)

string(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})

#-------------------------------------------------------------------------------

set ( CMAKE_CXX_FLAGS
    "-DWIN32;_WINDOWS /EHsc /MP2 -D_SCL_SECURE_NO_WARNINGS; -D_UNICODE;UNICODE /MTd
    /DAPR_DECLARE_STATIC;
    /DCOMPILED_FROM_DSP;XML_STATIC;_RTLDLL;
    /DNE_HAVE_SSL;HAVE_OPENSSL;HAVE_EXPAT;HAVE_EXPAT_H;NE_HAVE_DAV;NE_HAVE_ZLIB;USE_GETADDRINFO;NOUSER;NOGDI;NOCRYPT;
    /D_ATL_MIN_CRT;_ATL_NO_DEFAULT_LIBS;
"
)
    # /D_CONVERSION_DONT_USE_THREAD_LOCALE

set ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG;_WINDOWS;NETBOX_DEBUG;MPEXT;NO_STRICT;_ATL_MIN_CRT;_ATL_NO_DEFAULT_LIBS;_WINDLL;_USRDLL;"
     " /Gm- /EHsc /MTd /GS" )
set ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG;BOOST_DISABLE_ASSERTS" )

set ( CMAKE_LINK_FLAGS
    "/INCREMENTAL:NO /NODEFAULTLIB:LIBCMT.LIB
    /NODEFAULTLIB:LIBCRTD.LIB
    /NODEFAULTLIB:MSVCRT.LIB
    /NODEFAULTLIB:MSVCRTD.LIB
    /NODEFAULTLIB:MFCS100UD.LIB
    /NODEFAULTLIB:ATLSD.LIB
    /NODEFAULTLIB:UAFXCWD.LIB
    /NODEFAULTLIB:LIBCMTD.LIB
    /NODEFAULTLIB:LIBCPMTD.LIB
    /NODEFAULTLIB:WINDOWSCODECS.LIB
    /NODEFAULTLIB:DELAYIMP.LIB
    /NODEFAULTLIB:LIBEAY32.LIB
    /NODEFAULTLIB:SSLEAY32.LIB
    /NODEFAULTLIB:LIBPUTTY.LIB
    /NODEFAULTLIB:WS2_32.LIB
    /NODEFAULTLIB:KERNEL32.LIB
    /NODEFAULTLIB:USER32.LIB
    /NODEFAULTLIB:GDI32.LIB
    /NODEFAULTLIB:COMDLG32.LIB
    /NODEFAULTLIB:ADVAPI32.LIB
    /NODEFAULTLIB:VERSION.LIB
    /NODEFAULTLIB:WINHTTP.LIB
    /NODEFAULTLIB:WINSPOOL.LIB
    /NODEFAULTLIB:COMCTL32.LIB
"
)

# definitions
ADD_DEFINITIONS("-DNETBOX_DEBUG")
ADD_DEFINITIONS("-DUSE_FASTFLOW
    -D_FF_WIN_XP;MPEXT;NO_SECURITY
")

include ( ${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake )
# message ( "system: ${CMAKE_SYSTEM_NAME}" )

#-------------------------------------------------------------------------------

project ( NetBox )

set ( CMAKE_BUILD_TYPE UnityDebug )

set ( FAR_VERSION Far2 )
set ( PLATFORM x86 )

set ( OUTPUT_DIR ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )

set ( LIBS_ROOT ${PROJECT_ROOT}/libs )

#-------------------------------------------------------

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )

#-------------------------------------------------------------------------------

include_directories (
    ${PROJECT_ROOT}/src/NetBox
    ${PROJECT_ROOT}/src/PluginSDK/${FAR_VERSION}
    ${PROJECT_ROOT}/src/base
    ${PROJECT_ROOT}/src/core
    ${PROJECT_ROOT}/src/windows
    ${PROJECT_ROOT}/src/resource
    ${PROJECT_ROOT}/src/Common
    ${PROJECT_ROOT}/src/filezilla
    ${PROJECT_ROOT}/src/filezilla/misc
    ${LIBS_ROOT}
    ${LIBS_ROOT}/boost
    ${LIBS_ROOT}/Putty
    ${LIBS_ROOT}/Putty/windows
    ${LIBS_ROOT}/Putty/charset
    ${LIBS_ROOT}/tinyXML
    ${LIBS_ROOT}/openssl/${PLATFORM}/inc32
    ${LIBS_ROOT}/zlib/src
    ${LIBS_ROOT}/expat/lib
    ${LIBS_ROOT}/neon/src
    ${LIBS_ROOT}/apr/include
    ${LIBS_ROOT}/apr/include/arch/win32
    ${LIBS_ROOT}/apr/include/arch/unix
)

link_directories (
    ${LIBS_ROOT}/boost/stage
    # ${PROJECT_ROOT}/Temp/NetBox/${CMAKE_BUILD_TYPE}/${PLATFORM}/zlib
)

#-------------------------------------------------------------------------------

set ( NETBOX_ALL_HEADERS
    ${PROJECT_ROOT}/src/NetBox/resource.h
)

set ( COMMON_SOURCES
    ${PROJECT_ROOT}/src/base/Classes.cpp
    ${PROJECT_ROOT}/src/base/Sysutils.cpp
    ${PROJECT_ROOT}/src/base/UnicodeString.cpp
    ${PROJECT_ROOT}/src/base/farrtl.cpp
    ${PROJECT_ROOT}/src/base/local.cpp
    ${PROJECT_ROOT}/src/core/RemoteFiles.cpp
    ${PROJECT_ROOT}/src/core/Terminal.cpp
    ${PROJECT_ROOT}/src/core/Exceptions.cpp
    ${PROJECT_ROOT}/src/core/FileOperationProgress.cpp
    ${PROJECT_ROOT}/src/core/Queue.cpp
    ${PROJECT_ROOT}/src/core/SecureShell.cpp
    ${PROJECT_ROOT}/src/core/SessionInfo.cpp
    ${PROJECT_ROOT}/src/core/CoreMain.cpp
    ${PROJECT_ROOT}/src/core/FileMasks.cpp
    ${PROJECT_ROOT}/src/core/CopyParam.cpp
    ${PROJECT_ROOT}/src/core/SessionData.cpp
    ${PROJECT_ROOT}/src/core/Configuration.cpp
    ${PROJECT_ROOT}/src/core/ScpFileSystem.cpp
    ${PROJECT_ROOT}/src/core/FtpFileSystem.cpp
    ${PROJECT_ROOT}/src/core/SftpFileSystem.cpp
    ${PROJECT_ROOT}/src/core/WebDAVFileSystem.cpp
    ${PROJECT_ROOT}/src/core/PuttyIntf.cpp
    ${PROJECT_ROOT}/src/core/FileBuffer.cpp
    ${PROJECT_ROOT}/src/core/NamedObjs.cpp
    ${PROJECT_ROOT}/src/core/HierarchicalStorage.cpp
    ${PROJECT_ROOT}/src/core/Option.cpp
    ${PROJECT_ROOT}/src/core/FileInfo.cpp
    ${PROJECT_ROOT}/src/core/FileSystems.cpp
    ${PROJECT_ROOT}/src/core/Bookmarks.cpp
    ${PROJECT_ROOT}/src/core/Cryptography.cpp
    ${PROJECT_ROOT}/src/core/WinSCPSecurity.cpp
    ${PROJECT_ROOT}/src/core/Common.cpp
    ${PROJECT_ROOT}/src/windows/SynchronizeController.cpp
    ${PROJECT_ROOT}/src/windows/GUIConfiguration.cpp
    ${PROJECT_ROOT}/src/windows/GUITools.cpp
    ${PROJECT_ROOT}/src/NetBox/stdafx.cpp
    ${PROJECT_ROOT}/src/NetBox/FarUtil.cpp
    ${PROJECT_ROOT}/src/NetBox/FarPlugin.cpp
    ${PROJECT_ROOT}/src/NetBox/FarDialog.cpp
    ${PROJECT_ROOT}/src/NetBox/WinSCPDialogs.cpp
    ${PROJECT_ROOT}/src/NetBox/WinSCPFileSystem.cpp
    ${PROJECT_ROOT}/src/NetBox/WinSCPPlugin.cpp
    ${PROJECT_ROOT}/src/NetBox/FarInterface.cpp
    ${PROJECT_ROOT}/src/NetBox/FarConfiguration.cpp
    ${PROJECT_ROOT}/src/NetBox/XmlStorage.cpp
    ${PROJECT_ROOT}/src/resource/TextsCore1.rc
    ${PROJECT_ROOT}/src/resource/TextsCore2.rc
    ${PROJECT_ROOT}/src/resource/rtlconsts.rc
    ${PROJECT_ROOT}/src/resource/TextsFileZilla.rc
    ${PROJECT_ROOT}/src/filezilla/FileZillaIntf.cpp
    ${PROJECT_ROOT}/src/filezilla/FileZillaIntern.cpp
    ${PROJECT_ROOT}/src/filezilla/ApiLog.cpp
    ${PROJECT_ROOT}/src/filezilla/ServerPath.cpp
    ${PROJECT_ROOT}/src/filezilla/AsyncSslSocketLayer.cpp
    ${PROJECT_ROOT}/src/filezilla/AsyncSocketExLayer.cpp
    ${PROJECT_ROOT}/src/filezilla/AsyncSocketEx.cpp
    ${PROJECT_ROOT}/src/filezilla/FileZillaApi.cpp
    ${PROJECT_ROOT}/src/filezilla/FzApiStructures.cpp
    ${PROJECT_ROOT}/src/filezilla/Crypt.cpp
    ${PROJECT_ROOT}/src/filezilla/ControlSocket.cpp
    ${PROJECT_ROOT}/src/filezilla/FtpControlSocket.cpp
    ${PROJECT_ROOT}/src/filezilla/MainThread.cpp
    ${PROJECT_ROOT}/src/filezilla/TransferSocket.cpp
    ${PROJECT_ROOT}/src/filezilla/PathFunctions.cpp
    ${PROJECT_ROOT}/src/filezilla/FtpListResult.cpp
    ${PROJECT_ROOT}/src/filezilla/AsyncProxySocketLayer.cpp
    ${PROJECT_ROOT}/src/filezilla/structures.cpp
    ${PROJECT_ROOT}/src/filezilla/MFC64bitFix.cpp
    ${PROJECT_ROOT}/src/filezilla/Options.cpp
    ${PROJECT_ROOT}/src/filezilla/afxdll.cpp
    ${PROJECT_ROOT}/src/filezilla/misc/Fzutf8.cpp
    ${PROJECT_ROOT}/src/filezilla/misc/CBase64Coding.cpp
)

#-------------------------------------------------------------------------------
# target NetBox

set ( NETBOX_SOURCES # ${COMMON_SOURCES}
    ${PROJECT_ROOT}/src/NetBox/UnityBuildCore.cpp
    ${PROJECT_ROOT}/src/NetBox/UnityBuildMain.cpp
    ${PROJECT_ROOT}/src/NetBox/UnityBuildFilezilla.cpp
    # ${PROJECT_ROOT}/src/NetBox/NetBox.cpp
    ${PROJECT_ROOT}/src/NetBox/NetBox.def
)

add_library (libeay32 STATIC IMPORTED)
set_property (TARGET libeay32
    # PROPERTY IMPORTED_IMPLIB ${LIBS_ROOT}/openssl/${PLATFORM}/libeay32.lib )
    PROPERTY IMPORTED_LOCATION ${LIBS_ROOT}/openssl/${PLATFORM}/libeay32.lib )
add_library (ssleay32 STATIC IMPORTED)
set_property (TARGET ssleay32
    PROPERTY IMPORTED_LOCATION ${LIBS_ROOT}/openssl/${PLATFORM}/ssleay32.lib )

#-------------------------------------------------------------------------------

add_library (libputty STATIC
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

set_target_properties( libputty
    PROPERTIES
    COMPILE_FLAGS "/TC -D_CRTIMP= -DLibrary;SECURITY_WIN32;_WINDOWS;NET_SETUP_DIAGNOSTICS;NO_SECURITY"
)

#-------------------------------------------------------------------------------

add_library (libtinyxml STATIC
    ${LIBS_ROOT}/tinyXML/tinyxml.cpp
    ${LIBS_ROOT}/tinyXML/tinystr.cpp
    ${LIBS_ROOT}/tinyXML/tinyxmlerror.cpp
    ${LIBS_ROOT}/tinyXML/tinyxmlparser.cpp
)

set_target_properties( libtinyxml
    PROPERTIES
    COMPILE_FLAGS "-DSECURITY_WIN32;_WINDOWS"
)

add_library (libneon STATIC
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

set_target_properties( libneon
    PROPERTIES
    COMPILE_FLAGS "-D_CRTIMP= -D_LIB;NE_HAVE_SSL;HAVE_OPENSSL;HAVE_EXPAT;HAVE_EXPAT_H;NE_HAVE_DAV;NE_HAVE_ZLIB;USE_GETADDRINFO;NOUSER;NOGDI;NONLS;NOCRYPT;COMPILED_FROM_DSP;XML_STATIC;_RTLDLL;"
    LINK_FLAGS ${CMAKE_LINK_FLAGS}
)

#-------------------------------------------------------------------------------

add_library (libapr STATIC
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

set_target_properties( libapr
    PROPERTIES
    COMPILE_FLAGS "-D_CRTIMP= -D_LIB;APR_DECLARE_STATIC;"
    LINK_FLAGS ${CMAKE_LINK_FLAGS}
)

#-------------------------------------------------------------------------------

add_library (libexpat STATIC
    ${LIBS_ROOT}/expat/lib/xmlparse.c
    ${LIBS_ROOT}/expat/lib/xmlrole.c
    ${LIBS_ROOT}/expat/lib/xmltok.c
    ${LIBS_ROOT}/expat/lib/xmltok_impl.c
    ${LIBS_ROOT}/expat/lib/xmltok_ns.c
)

set_target_properties( libexpat
    PROPERTIES
    COMPILE_FLAGS "/TC -D_CRTIMP= -D_LIB;COMPILED_FROM_DSP;XML_STATIC;_RTLDLL;"
    LINK_FLAGS ${CMAKE_LINK_FLAGS}
)

#-------------------------------------------------------------------------------

add_library (zlib STATIC
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

set_target_properties( zlib
    PROPERTIES
    COMPILE_FLAGS "-D_CRTIMP= -D_LIB"
    LINK_FLAGS ${CMAKE_LINK_FLAGS}
)

#-------------------------------------------------------------------------------

set ( NETBOX_LIBRARIES
    mfcs100ud.lib
    Atlsd.lib
    uafxcwd.lib
    libcmtd.lib
    libcpmtd.lib
    windowscodecs.lib
    delayimp.lib
    libputty
    zlib
    libtinyxml
    libeay32
    ssleay32
    libneon
    libapr
    libexpat
    Ws2_32.lib
    kernel32.lib
    user32.lib
    gdi32.lib
    comdlg32.lib
    advapi32.lib
    Version.lib
    winhttp.lib
    winspool.lib
    comctl32.lib
    Crypt32.lib
)

add_library ( NetBox SHARED ${NETBOX_SOURCES} )
set_target_properties( NetBox
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
    LINK_FLAGS ${CMAKE_LINK_FLAGS}
)

# move from NetBox/Debug to NetBox/
# string(REPLACE "/" "\\" outdir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
# add_custom_command ( TARGET NetBox
#     POST_BUILD
#     COMMAND copy /Y ${outdir}\\${CMAKE_BUILD_TYPE}\\ ${outdir}
#     COMMAND del /F /Q ${outdir}\\${CMAKE_BUILD_TYPE}\\NetBox.dll_
#     COMMAND ren ${outdir}\\${CMAKE_BUILD_TYPE}\\NetBox.dll NetBox.dll_
#     WORKING_DIRECTORY ${outdir}
#     VERBATIM
# )
#TODO: COMMAND create_ver.py

target_link_libraries ( NetBox ${NETBOX_LIBRARIES} )

add_dependencies ( NetBox
    libputty
    libtinyxml
    libneon
    libapr
    libexpat
    zlib
)

# use_precompiled_header ( NETBOX_SOURCES stdafx.h stdafx.cpp netbox )

#-------------------------------------------------------
# target testnetbox_01

set ( TESTNETBOX_01_DIR ${PROJECT_ROOT}/src/NetBox )

set ( TESTNETBOX_01_SOURCES
    ${PROJECT_ROOT}/src/base/Classes.cpp
    ${PROJECT_ROOT}/src/base/Sysutils.cpp
    ${PROJECT_ROOT}/src/base/UnicodeString.cpp
    ${PROJECT_ROOT}/src/base/farrtl.cpp
    ${PROJECT_ROOT}/src/base/local.cpp
    ${PROJECT_ROOT}/src/core/Common.cpp
    ${PROJECT_ROOT}/src/core/Exceptions.cpp
    ${PROJECT_ROOT}/src/core/FileBuffer.cpp
    ${TESTNETBOX_01_DIR}/testnetbox_01.cpp
    ${PROJECT_ROOT}/src/resource/TextsCore1.rc
    ${PROJECT_ROOT}/src/resource/rtlconsts.rc
)

add_executable( testnetbox_01 ${TESTNETBOX_01_SOURCES} )
set_target_properties( testnetbox_01
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
    LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

add_dependencies ( testnetbox_01
    libputty
    libtinyxml
)

target_link_libraries ( testnetbox_01 ${NETBOX_LIBRARIES} )

#-------------------------------------------------------
# target testnetbox_02

set ( TESTNETBOX_02_DIR ${PROJECT_ROOT}/src/NetBox )

set ( TESTNETBOX_02_SOURCES
    ${TESTNETBOX_02_DIR}/UnityBuildCore.cpp
    ${TESTNETBOX_02_DIR}/UnityBuildMain.cpp
    ${TESTNETBOX_02_DIR}/UnityBuildFilezilla.cpp
    ${PROJECT_ROOT}/src/resource/testnetbox_02.rc
    ${TESTNETBOX_02_DIR}/testnetbox_02.cpp
)

add_executable( testnetbox_02 ${TESTNETBOX_02_SOURCES} )
set_target_properties( testnetbox_02
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
    LINK_FLAGS "${CMAKE_LINK_FLAGS} /verbose:lib"
)
add_dependencies ( testnetbox_02
    libputty
    libtinyxml
    libneon
    libapr
    libexpat
    zlib
)

target_link_libraries ( testnetbox_02 ${NETBOX_LIBRARIES} )

#-------------------------------------------------------
# target testnetbox_03

set ( TESTNETBOX_03_DIR ${PROJECT_ROOT}/src/NetBox )

set ( TESTNETBOX_03_SOURCES
    ${TESTNETBOX_03_DIR}/UnityBuildCore.cpp
    ${TESTNETBOX_03_DIR}/UnityBuildMain.cpp
    ${TESTNETBOX_03_DIR}/UnityBuildFilezilla.cpp
    ${TESTNETBOX_03_DIR}/testnetbox_03.cpp
)

add_executable( testnetbox_03 ${TESTNETBOX_03_SOURCES} )
set_target_properties( testnetbox_03
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
    LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
add_dependencies ( testnetbox_03
    libputty
    libtinyxml
    libneon
    libapr
    libexpat
    zlib
)

target_link_libraries ( testnetbox_03 ${NETBOX_LIBRARIES} )

#-------------------------------------------------------
# target testnetbox_04

set ( TESTNETBOX_04_DIR ${PROJECT_ROOT}/src/NetBox )
set ( TESTNETBOX_04_SOURCES
    ${TESTNETBOX_04_DIR}/UnityBuildCore.cpp
    ${TESTNETBOX_04_DIR}/UnityBuildMain.cpp
    ${TESTNETBOX_04_DIR}/UnityBuildFilezilla.cpp
    ${TESTNETBOX_04_DIR}/testnetbox_04.cpp
)
add_executable( testnetbox_04 ${TESTNETBOX_04_SOURCES} )
set_target_properties( testnetbox_04
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
    LINK_FLAGS "${CMAKE_LINK_FLAGS} /verbose:lib"
)
add_dependencies ( testnetbox_04 ${NETBOX_LIBRARIES}
)
target_link_libraries ( testnetbox_04 ${NETBOX_LIBRARIES}
)

