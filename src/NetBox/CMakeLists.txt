cmake_minimum_required (VERSION 2.8)

string(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})

#-------------------------------------------------------------------------------

set ( CMAKE_CXX_FLAGS
    "-DWIN32;_WINDOWS /EHsc /MP2 -D_SCL_SECURE_NO_WARNINGS; -D_UNICODE;UNICODE" )
set ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG;_WINDOWS;_WINDLL;_USRDLL;NETBOX_DEBUG;MPEXT;NO_STRICT;"
     " /Gm- /EHsc /MTd /GS" )
set ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG;BOOST_DISABLE_ASSERTS" )

set ( CMAKE_LINK_FLAGS
    "/INCREMENTAL:NO /NODEFAULTLIB:LIBCMT.LIB /NODEFAULTLIB:MSVCRT.LIB /NODEFAULTLIB:LIBCMTD.LIB"
    #/NODEFAULTLIB:LIBCPMTD.LIB"
    # /NODEFAULTLIB:MSVCPRTD.LIB"
)

# definitions
ADD_DEFINITIONS("-DNETBOX_DEBUG")
ADD_DEFINITIONS("-DUSE_FASTFLOW
                -D_FF_WIN_XP;CURL_STATICLIB;MPEXT;NO_SECURITY
")
# ADD_DEFINITIONS("-D_USRDLL;_AFXDLL;_AFXEXT;_DLL")

include ( ${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake )
# message ( "system: ${CMAKE_SYSTEM_NAME}" )

#-------------------------------------------------------------------------------

macro ( use_precompiled_header SRC_LIST_VAR HDR_FILE SRC_FILE PCHNAME )
    get_filename_component ( PCH_HEADER ${HDR_FILE} NAME )
    set ( PCH_BINARY ${PCHNAME} )
    set ( PCH_BINARY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${PCH_BINARY}.pch" )
    if (MSVC)
        set ( SRC_LIST ${${SRC_LIST_VAR}} )
        set_source_files_properties ( ${SRC_LIST} PROPERTIES
            COMPILE_FLAGS "/Yu${PCH_HEADER} /Fp${PCH_BINARY}"
            OBJECT_DEPENDS "${PCH_BINARY}" )
        set_source_files_properties ( ${SRC_FILE} PROPERTIES
            COMPILE_FLAGS "/Yc${PCH_HEADER} /Fp${PCH_BINARY}"
            OBJECT_OUTPUTS "${PCH_BINARY}"
            OBJECT_DEPENDS "" )
    endif (MSVC)
endmacro ( use_precompiled_header )

#-------------------------------------------------------

project ( NetBox )

set ( CMAKE_BUILD_TYPE Debug )

set ( FAR_VERSION Far2 )
set ( PLATFORM x86 )

set ( OUTPUT_DIR ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )

#-------------------------------------------------------

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )

#-------------------------------------------------------------------------------

include_directories (
    ${PROJECT_ROOT}/src/NetBox
    ${PROJECT_ROOT}/src/PluginSDK/${FAR_VERSION}
    ${PROJECT_ROOT}/src/core
    ${PROJECT_ROOT}/src/windows
    ${PROJECT_ROOT}/src/resource
    ${PROJECT_ROOT}/src/Common
    ${PROJECT_ROOT}/src/filezilla
    ${PROJECT_ROOT}/src/filezilla/misc
    ${PROJECT_ROOT}/libs
    ${PROJECT_ROOT}/libs/boost
    ${PROJECT_ROOT}/libs/Putty
    ${PROJECT_ROOT}/libs/Putty/windows
    ${PROJECT_ROOT}/libs/Putty/charset
    ${PROJECT_ROOT}/libs/libcurl/include
    ${PROJECT_ROOT}/libs/libssh2/include
    ${PROJECT_ROOT}/libs/tinyXML
    ${PROJECT_ROOT}/libs/openssl/${PLATFORM}/inc32
    ${PROJECT_ROOT}/libs/zlib/src
)

link_directories (
    ${PROJECT_ROOT}/libs/boost/stage
    ${PROJECT_ROOT}/Temp/NetBox/${CMAKE_BUILD_TYPE}/${PLATFORM}/libcurl
    ${PROJECT_ROOT}/Temp/NetBox/${CMAKE_BUILD_TYPE}/${PLATFORM}/libssh2
    ${PROJECT_ROOT}/Temp/NetBox/${CMAKE_BUILD_TYPE}/${PLATFORM}/zlib
)

#-------------------------------------------------------------------------------

# add_subdirectory ( ${PROJECT_ROOT}
#     libs/libcurl
#     EXCLUDE_FROM_ALL
# )

#-------------------------------------------------------------------------------

set ( NETBOX_ALL_HEADERS
    ${PROJECT_ROOT}/src/NetBox/resource.h
)

set ( COMMON_SOURCES
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
    ${PROJECT_ROOT}/src/core/SynchronizeController.cpp
    ${PROJECT_ROOT}/src/core/Cryptography.cpp
    ${PROJECT_ROOT}/src/core/WinSCPSecurity.cpp
    ${PROJECT_ROOT}/src/core/Classes.cpp
    ${PROJECT_ROOT}/src/core/Common.cpp
    ${PROJECT_ROOT}/src/core/ustring.cpp
    ${PROJECT_ROOT}/src/windows/GUIConfiguration.cpp
    ${PROJECT_ROOT}/src/windows/GUITools.cpp
    ${PROJECT_ROOT}/src/NetBox/stdafx.cpp
    ${PROJECT_ROOT}/src/NetBox/EasyURL.cpp
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

set ( NETBOX_HEADERS ${NETBOX_ALL_HEADERS}
)

set ( NETBOX_SOURCES ${COMMON_SOURCES}
    ${PROJECT_ROOT}/src/NetBox/NetBox.cpp
    ${PROJECT_ROOT}/src/NetBox/NetBox.def
)

add_library (libcurl STATIC IMPORTED)
set_property (TARGET libcurl
    PROPERTY IMPORTED_LOCATION ${PROJECT_ROOT}/Temp/NetBox/${CMAKE_BUILD_TYPE}/${PLATFORM}/libcurl/libcurl.lib)
add_library (libeay32 SHARED IMPORTED)
set_property (TARGET libeay32
    PROPERTY IMPORTED_IMPLIB ${PROJECT_ROOT}/libs/openssl/${PLATFORM}/libeay32.lib )
add_library (ssleay32 SHARED IMPORTED)
set_property (TARGET ssleay32
    PROPERTY IMPORTED_IMPLIB ${PROJECT_ROOT}/libs/openssl/${PLATFORM}/ssleay32.lib )

#-------------------------------------------------------------------------------

add_library (libputty STATIC
    ${PROJECT_ROOT}/libs/Putty/CPROXY.C
    ${PROJECT_ROOT}/libs/Putty/INT64.C
    ${PROJECT_ROOT}/libs/Putty/LOGGING.C
    ${PROJECT_ROOT}/libs/Putty/MISC.C
    ${PROJECT_ROOT}/libs/Putty/PGSSAPI.C
    ${PROJECT_ROOT}/libs/Putty/PORTFWD_.C
    ${PROJECT_ROOT}/libs/Putty/PROXY.C
    ${PROJECT_ROOT}/libs/Putty/SSH_.C
    ${PROJECT_ROOT}/libs/Putty/SSHAES_.C
    ${PROJECT_ROOT}/libs/Putty/SSHARCF.C
    ${PROJECT_ROOT}/libs/Putty/SSHBLOWF.C
    ${PROJECT_ROOT}/libs/Putty/SSHBN.C
    ${PROJECT_ROOT}/libs/Putty/SSHCRC.C
    ${PROJECT_ROOT}/libs/Putty/SSHCRCDA.C
    ${PROJECT_ROOT}/libs/Putty/SSHDES.C
    ${PROJECT_ROOT}/libs/Putty/SSHDH.C
    ${PROJECT_ROOT}/libs/Putty/SSHDSS.C
    ${PROJECT_ROOT}/libs/Putty/SSHGSSC.C
    ${PROJECT_ROOT}/libs/Putty/SSHMD5.C
    ${PROJECT_ROOT}/libs/Putty/SSHPUBK.C
    ${PROJECT_ROOT}/libs/Putty/SSHRAND.C
    ${PROJECT_ROOT}/libs/Putty/SSHRSA.C
    ${PROJECT_ROOT}/libs/Putty/SSHSH256.C
    ${PROJECT_ROOT}/libs/Putty/SSHSH512.C
    ${PROJECT_ROOT}/libs/Putty/SSHSHA_.C
    ${PROJECT_ROOT}/libs/Putty/SSHZLIB.C
    ${PROJECT_ROOT}/libs/Putty/TREE234.C
    ${PROJECT_ROOT}/libs/Putty/CHARSET/UTF8.C
    ${PROJECT_ROOT}/libs/Putty/WILDCARD.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINGSS.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINHANDL.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINMISC.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINNET.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINNOISE.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINNOJMP.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINPGNTC.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINPROXY.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINSTORE_.C
    ${PROJECT_ROOT}/libs/Putty/WINDOWS/WINTIME.C
    ${PROJECT_ROOT}/libs/Putty/X11FWD.C
)

set_target_properties( libputty
    PROPERTIES
    COMPILE_FLAGS "/TC -D_CRTIMP= -DLibrary;SECURITY_WIN32;_WINDOWS;NET_SETUP_DIAGNOSTICS"
)

#-------------------------------------------------------------------------------

add_library (libtinyxml STATIC
    ${PROJECT_ROOT}/libs/tinyXML/tinyxml.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinystr.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinyxmlerror.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinyxmlparser.cpp
)

set_target_properties( libtinyxml
    PROPERTIES
    COMPILE_FLAGS "-DSECURITY_WIN32;_WINDOWS"
)

#-------------------------------------------------------------------------------

set ( NETBOX_LIBRARIES
    mfc100ud.lib
    Atl.lib
    uafxcwd.lib
    comctl32.lib
    msvcrtd.lib
    msvcprtd.lib
    msvcmrtd.lib
    msvcurtd.lib
    libputty
    libtinyxml
    libcurl
    libeay32
    ssleay32
    libssh2.lib
    zlib.lib
    ws2_32.lib
    winhttp.lib
    kernel32.lib
    user32.lib
    gdi32.lib
    winspool.lib
    comdlg32.lib
    advapi32.lib
    shell32.lib
    ole32.lib
    oleaut32.lib
    uuid.lib
    odbc32.lib
    odbccp32.lib
    Version.lib
)

add_library ( NetBox SHARED ${NETBOX_HEADERS} ${NETBOX_SOURCES} )
set_target_properties( NetBox
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -D_UNICODE;UNICODE /MTd"
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
    #zlib libssh2 libcurl
)

# use_precompiled_header ( NETBOX_SOURCES stdafx.h stdafx.cpp netbox )

#-------------------------------------------------------
# target testnetbox_01

set ( TESTNETBOX_01_DIR ${PROJECT_ROOT}/src/NetBox )

set ( TESTNETBOX_01_SOURCES
    ${PROJECT_ROOT}/src/core/Classes.cpp
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
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -D_UNICODE;UNICODE /MTd"
    LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)

add_dependencies ( testnetbox_01 ${NETBOX_LIBRARIES}
    libputty
    libtinyxml
)

target_link_libraries ( testnetbox_01 )

#-------------------------------------------------------
# target testnetbox_02

set ( TESTNETBOX_02_DIR ${PROJECT_ROOT}/src/NetBox )

set ( TESTNETBOX_02_SOURCES ${COMMON_SOURCES}
    ${PROJECT_ROOT}/src/resource/testnetbox_02.rc
    ${TESTNETBOX_02_DIR}/testnetbox_02.cpp
)

add_executable( testnetbox_02 ${TESTNETBOX_02_SOURCES} )
set_target_properties( testnetbox_02
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS}"
    LINK_FLAGS "${CMAKE_LINK_FLAGS} /NODEFAULTLIB:MSVCRTD.LIB"
)
add_dependencies ( testnetbox_02
    libputty
    libtinyxml
)

target_link_libraries ( testnetbox_02 ${NETBOX_LIBRARIES} )

