cmake_minimum_required (VERSION 2.8)

string(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})

#-------------------------------------------------------------------------------

set ( CMAKE_CXX_FLAGS
    "-DWIN32;_WINDOWS /EHsc /MP2 -D_SCL_SECURE_NO_WARNINGS" )
set ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG;_MT;WINSCP" )
set ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG;BOOST_DISABLE_ASSERTS" )

set ( CMAKE_LINK_FLAGS
    "/INCREMENTAL:NO /NODEFAULTLIB:LIBCMT.LIB /NODEFAULTLIB:MSVCRT.LIB"
    #/NODEFAULTLIB:LIBCMTD.LIB
    #/NODEFAULTLIB:LIBCPMTD.LIB"
    # /NODEFAULTLIB:MSVCPRTD.LIB"
)

# definitions
add_definitions ( "-DNETBOX_DEBUG;NETBOX_EXPORTS;_USRDLL" )
add_definitions ( "-DUSE_FASTFLOW;_FF_WIN_XP;CURL_STATICLIB;NO_FILEZILLA;MPEXT" )

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

set ( PLATFORM x86 )
set ( FAR_VERSION Far2 )

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
    ${PROJECT_ROOT}/src/WinSCP
    ${PROJECT_ROOT}/src/Putty
    ${PROJECT_ROOT}/src/Putty/windows
    ${PROJECT_ROOT}/src/Putty/charset
    ${PROJECT_ROOT}/libs
    ${PROJECT_ROOT}/libs/boost
    ${PROJECT_ROOT}/libs/libcurl/include
    ${PROJECT_ROOT}/libs/libssh2/include
    ${PROJECT_ROOT}/libs/tinyXML
    ${PROJECT_ROOT}/libs/openssl/${PLATFORM}/include
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
    ${PROJECT_ROOT}/libs/tinyXML/tinyxml.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinystr.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinyxmlerror.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinyxmlparser.cpp
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
    ${PROJECT_ROOT}/src/Common/Classes.cpp
    ${PROJECT_ROOT}/src/Common/Common.cpp
    ${PROJECT_ROOT}/src/Common/FarUtil.cpp
    ${PROJECT_ROOT}/src/Common/FarPlugin.cpp
    ${PROJECT_ROOT}/src/Common/FarDialog.cpp
    ${PROJECT_ROOT}/src/windows/GUIConfiguration.cpp
    ${PROJECT_ROOT}/src/windows/GUITools.cpp
    ${PROJECT_ROOT}/src/WinSCP/WinSCPDialogs.cpp
    ${PROJECT_ROOT}/src/WinSCP/WinSCPFileSystem.cpp
    ${PROJECT_ROOT}/src/WinSCP/WinSCPPlugin.cpp
    ${PROJECT_ROOT}/src/WinSCP/FarInterface.cpp
    ${PROJECT_ROOT}/src/WinSCP/FarConfiguration.cpp
    ${PROJECT_ROOT}/src/NetBox/stdafx.cpp
    ${PROJECT_ROOT}/src/NetBox/Logging.cpp
    ${PROJECT_ROOT}/src/NetBox/Settings.cpp
    ${PROJECT_ROOT}/src/NetBox/EasyURL.cpp
    ${PROJECT_ROOT}/src/NetBox/Session.cpp
    ${PROJECT_ROOT}/src/NetBox/SessionManager.cpp
    ${PROJECT_ROOT}/src/NetBox/SessionEditor.cpp
    ${PROJECT_ROOT}/src/NetBox/FTP.cpp
    ${PROJECT_ROOT}/src/NetBox/SFTP.cpp
    ${PROJECT_ROOT}/src/NetBox/FTPS.cpp
    ${PROJECT_ROOT}/src/NetBox/SCP.cpp
    ${PROJECT_ROOT}/src/NetBox/Panel.cpp
    ${PROJECT_ROOT}/src/NetBox/WebDAV.cpp
    ${PROJECT_ROOT}/src/NetBox/Session.cpp
    ${PROJECT_ROOT}/src/NetBox/NetBoxPlugin.cpp
    ${PROJECT_ROOT}/src/NetBox/ProgressWindow.cpp
    ${PROJECT_ROOT}/src/resource/TextsCore1.rc
    ${PROJECT_ROOT}/src/resource/rtlconsts.rc
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
    PROPERTY IMPORTED_IMPLIB ${PROJECT_ROOT}/libs/openssl/${PLATFORM}/out32dll/libeay32.lib )
add_library (ssleay32 SHARED IMPORTED)
set_property (TARGET ssleay32
    PROPERTY IMPORTED_IMPLIB ${PROJECT_ROOT}/libs/openssl/${PLATFORM}/out32dll/ssleay32.lib )

#-------------------------------------------------------------------------------

add_library (libputty STATIC
    ${PROJECT_ROOT}/src/Putty/CPROXY.C
    ${PROJECT_ROOT}/src/Putty/INT64.C
    ${PROJECT_ROOT}/src/Putty/LOGGING.C
    ${PROJECT_ROOT}/src/Putty/MISC.C
    ${PROJECT_ROOT}/src/Putty/PGSSAPI.C
    ${PROJECT_ROOT}/src/Putty/PORTFWD_.C
    ${PROJECT_ROOT}/src/Putty/PROXY.C
    ${PROJECT_ROOT}/src/Putty/SSH_.C
    ${PROJECT_ROOT}/src/Putty/SSHAES_.C
    ${PROJECT_ROOT}/src/Putty/SSHARCF.C
    ${PROJECT_ROOT}/src/Putty/SSHBLOWF.C
    ${PROJECT_ROOT}/src/Putty/SSHBN.C
    ${PROJECT_ROOT}/src/Putty/SSHCRC.C
    ${PROJECT_ROOT}/src/Putty/SSHCRCDA.C
    ${PROJECT_ROOT}/src/Putty/SSHDES.C
    ${PROJECT_ROOT}/src/Putty/SSHDH.C
    ${PROJECT_ROOT}/src/Putty/SSHDSS.C
    ${PROJECT_ROOT}/src/Putty/SSHGSSC.C
    ${PROJECT_ROOT}/src/Putty/SSHMD5.C
    ${PROJECT_ROOT}/src/Putty/SSHPUBK.C
    ${PROJECT_ROOT}/src/Putty/SSHRAND.C
    ${PROJECT_ROOT}/src/Putty/SSHRSA.C
    ${PROJECT_ROOT}/src/Putty/SSHSH256.C
    ${PROJECT_ROOT}/src/Putty/SSHSH512.C
    ${PROJECT_ROOT}/src/Putty/SSHSHA_.C
    ${PROJECT_ROOT}/src/Putty/SSHZLIB.C
    ${PROJECT_ROOT}/src/Putty/TREE234.C
    ${PROJECT_ROOT}/src/Putty/CHARSET/UTF8.C
    ${PROJECT_ROOT}/src/Putty/WILDCARD.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINGSS.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINHANDL.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINMISC.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINNET.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINNOISE.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINNOJMP.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINPGNTC.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINPROXY.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINSTORE_.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/WINTIME.C
    ${PROJECT_ROOT}/src/Putty/WINDOWS/strftime.c
    ${PROJECT_ROOT}/src/Putty/X11FWD.C
)

set_target_properties( libputty
    PROPERTIES
    COMPILE_FLAGS "/TC -D_CRTIMP= -DLibrary;SECURITY_WIN32;_WINDOWS;NET_SETUP_DIAGNOSTICS"
)

#-------------------------------------------------------------------------------

set ( NETBOX_LIBRARIES
    libssh2.lib
    zlib.lib
    ws2_32.lib
    winhttp.lib
    Version.lib
    libputty
    libcurl
    libeay32
    ssleay32
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
    #zlib libssh2 libcurl
)

# use_precompiled_header ( NETBOX_SOURCES stdafx.h stdafx.cpp netbox )

#-------------------------------------------------------
# target testnetbox_01

set ( TESTNETBOX_01_DIR ${PROJECT_ROOT}/src/NetBox )

set ( TESTNETBOX_01_SOURCES
    ${PROJECT_ROOT}/src/Common/Classes.cpp
    ${PROJECT_ROOT}/src/Common/Common.cpp
    ${PROJECT_ROOT}/src/core/Exceptions.cpp
    ${TESTNETBOX_01_DIR}/testnetbox_01.cpp
    ${PROJECT_ROOT}/src/resource/TextsCore1.rc
    ${PROJECT_ROOT}/src/resource/rtlconsts.rc
)

add_executable( testnetbox_01 ${TESTNETBOX_01_SOURCES} )
set_target_properties( testnetbox_01
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -D_UNICODE;UNICODE /MTd"
    LINK_FLAGS "${CMAKE_LINK_FLAGS} /NODEFAULTLIB:MSVCRTD.LIB"
)
add_dependencies ( testnetbox_01
    libputty
)

target_link_libraries ( testnetbox_01 )

#-------------------------------------------------------
# target testnetbox_02

set ( TESTNETBOX_02_DIR ${PROJECT_ROOT}/src/NetBox )

set ( TESTNETBOX_02_SOURCES
    ${PROJECT_ROOT}/src/Common/Classes.cpp
    ${PROJECT_ROOT}/src/Common/Common.cpp
    ${PROJECT_ROOT}/src/Common/FarUtil.cpp
    ${PROJECT_ROOT}/src/Common/FarDialog.cpp
    ${PROJECT_ROOT}/src/Common/FarPlugin.cpp
    ${PROJECT_ROOT}/src/core/Exceptions.cpp
    ${PROJECT_ROOT}/src/core/FileOperationProgress.cpp
    ${PROJECT_ROOT}/src/core/Cryptography.cpp
    ${PROJECT_ROOT}/src/core/WinSCPSecurity.cpp
    ${PROJECT_ROOT}/src/core/PuttyIntf.cpp
    ${PROJECT_ROOT}/src/resource/TextsCore1.rc
    ${PROJECT_ROOT}/src/resource/rtlconsts.rc
    ${TESTNETBOX_02_DIR}/testnetbox_02.cpp
)

add_executable( testnetbox_02 ${TESTNETBOX_02_SOURCES} )
set_target_properties( testnetbox_02
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -D_UNICODE;UNICODE /MTd"
    LINK_FLAGS "${CMAKE_LINK_FLAGS} /NODEFAULTLIB:MSVCRTD.LIB"
)

add_dependencies ( testnetbox_02
    libputty
)

target_link_libraries ( testnetbox_02 ${NETBOX_LIBRARIES} )

#-------------------------------------------------------
# target testnetbox_10

set ( TESTNETBOX_10_DIR ${PROJECT_ROOT}/src/NetBox )

set ( TESTNETBOX_10_SOURCES ${COMMON_SOURCES}
    ${PROJECT_ROOT}/src/resource/testnetbox_10.rc
    ${TESTNETBOX_10_DIR}/testnetbox_10.cpp
)

add_executable( testnetbox_10 ${TESTNETBOX_10_SOURCES} )
set_target_properties( testnetbox_10
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -D_UNICODE;UNICODE /MTd"
    LINK_FLAGS "${CMAKE_LINK_FLAGS} /NODEFAULTLIB:MSVCRTD.LIB"
)
add_dependencies ( testnetbox_10
    libputty
)

target_link_libraries ( testnetbox_10 ${NETBOX_LIBRARIES} )

