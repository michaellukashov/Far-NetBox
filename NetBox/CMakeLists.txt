cmake_minimum_required (VERSION 2.8)

#-------------------------------------------------------------------------------

macro(use_precompiled_header SRC_LIST_VAR HDR_FILE SRC_FILE PCHNAME)
    get_filename_component( PCH_HEADER ${HDR_FILE} NAME )
    set(PCH_BINARY ${PCHNAME})
    set( PCH_BINARY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${PCH_BINARY}.pch" )
    if (MSVC)
        set( SRC_LIST ${${SRC_LIST_VAR}} )
        set_source_files_properties( ${SRC_LIST} PROPERTIES
            COMPILE_FLAGS "/Yu${PCH_HEADER} /Fp${PCH_BINARY}"
            OBJECT_DEPENDS "${PCH_BINARY}" )
        set_source_files_properties( ${SRC_FILE} PROPERTIES
            COMPILE_FLAGS "/Yc${PCH_HEADER} /Fp${PCH_BINARY}"
            OBJECT_OUTPUTS "${PCH_BINARY}"
            OBJECT_DEPENDS "" )
    endif(MSVC)
endmacro( use_precompiled_header )

#-------------------------------------------------------------------------------
# Устанавливаем флаги компиляции ( -DCMAKE_BUILD_TYPE=[Debug,Release] )

set ( CMAKE_CXX_FLAGS
    "-DWIN32;_WINDOWS;NO_FILEZILLA /EHsc /MP2" )
set ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG;_MT" )
set ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG;BOOST_DISABLE_ASSERTS" )

set ( CMAKE_EXE_LINK_FLAGS
    "/INCREMENTAL:NO /NODEFAULTLIB:LIBCMT.LIB /NODEFAULTLIB:MSVCRT.LIB"
    #/NODEFAULTLIB:LIBCMTD.LIB
)

# Дополнительные флаги компиляции
add_definitions ( "-DNETBOX_DEBUG;USE_FASTFLOW;CURL_STATICLIB;_USRDLL;NETBOX_EXPORTS" )
add_definitions ( "-D_FF_WIN_XP" )

#include (${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake)
#message (" system: ${CMAKE_SYSTEM_NAME}")

#-------------------------------------------------------

set (PROJECT NetBox )

# задать имя проекта
project (${PROJECT})

# Настройки Release/Debug
set ( CMAKE_BUILD_TYPE Debug )

set ( PLATFORM x86 )
set ( FAR_VERSION Far2 )

set ( OUTPUT_DIR ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )

#-------------------------------------------------------

# задать папку для выходных бинарников
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )

# задать папку для выходных динамических библиотек
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )

# задать папку для выходных статических библиотек
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_ROOT}/${FAR_VERSION}_${PLATFORM}/Plugins/NetBox )

#-------------------------------------------------------------------------------

# добавить пути для поиска файлов проекта
include_directories (
    ${PROJECT_ROOT}/NetBox
    ${PROJECT_ROOT}/PluginSDK/${FAR_VERSION}
    ${PROJECT_ROOT}/Common
    ${PROJECT_ROOT}/Putty
    ${PROJECT_ROOT}/Putty/windows
    ${PROJECT_ROOT}/Putty/charset
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

# выполнить подпроект [PROJECT] и положить результат в [PATH]
# EXCLUDE_FROM_ALL - собрать только необходимые цели
#add_subdirectory ( ${DICTIONARY_PATH}
#                   bin/sub_projects
#                   EXCLUDE_FROM_ALL
#                 )

#-------------------------------------------------------------------------------

set ( NETBOX_ALL_HEADERS
    ${PROJECT_ROOT}/NetBox/resource.h
)

#-------------------------------------------------------------------------------
# target NetBox

set ( NETBOX_HEADERS ${NETBOX_ALL_HEADERS}
)

set ( NETBOX_SOURCES
    ${PROJECT_ROOT}/libs/tinyXML/tinyxml.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinystr.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinyxmlerror.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinyxmlparser.cpp
    ${PROJECT_ROOT}/NetBox/Panel.cpp
    ${PROJECT_ROOT}/NetBox/FTP.cpp
    ${PROJECT_ROOT}/NetBox/EasyURL.cpp
    ${PROJECT_ROOT}/NetBox/stdafx.cpp
    ${PROJECT_ROOT}/NetBox/WebDAV.cpp
    ${PROJECT_ROOT}/NetBox/Settings.cpp
    ${PROJECT_ROOT}/NetBox/SessionManager.cpp
    ${PROJECT_ROOT}/NetBox/SessionEditor.cpp
    ${PROJECT_ROOT}/NetBox/NetBox.cpp
    ${PROJECT_ROOT}/NetBox/Session.cpp
    ${PROJECT_ROOT}/NetBox/SFTP.cpp
    ${PROJECT_ROOT}/NetBox/ProgressWindow.cpp
    ${PROJECT_ROOT}/NetBox/Logging.cpp
    ${PROJECT_ROOT}/NetBox/FTPS.cpp
    ${PROJECT_ROOT}/NetBox/SCP.cpp
    ${PROJECT_ROOT}/NetBox/WinSCPPlugin.cpp
    ${PROJECT_ROOT}/NetBox/WinSCPFileSystem.cpp
    ${PROJECT_ROOT}/Common/CoreMain.cpp
    ${PROJECT_ROOT}/Common/Configuration.cpp
    ${PROJECT_ROOT}/Common/FarPlugin.cpp
    ${PROJECT_ROOT}/Common/FarUtil.cpp
    ${PROJECT_ROOT}/NetBox/NetBox.def
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
    ${PROJECT_ROOT}/Putty/CPROXY.C
    ${PROJECT_ROOT}/Putty/INT64.C
    ${PROJECT_ROOT}/Putty/LOGGING.C
    ${PROJECT_ROOT}/Putty/MISC.C
    ${PROJECT_ROOT}/Putty/PGSSAPI.C
    ${PROJECT_ROOT}/Putty/PORTFWD_.C
    ${PROJECT_ROOT}/Putty/PROXY.C
    ${PROJECT_ROOT}/Putty/SSH_.C
    ${PROJECT_ROOT}/Putty/SSHAES_.C
    ${PROJECT_ROOT}/Putty/SSHARCF.C
    ${PROJECT_ROOT}/Putty/SSHBLOWF.C
    # ${PROJECT_ROOT}/Putty/SSHBN.C
    ${PROJECT_ROOT}/Putty/SSHCRC.C
    ${PROJECT_ROOT}/Putty/SSHCRCDA.C
    ${PROJECT_ROOT}/Putty/SSHDES.C
    ${PROJECT_ROOT}/Putty/SSHDH.C
    ${PROJECT_ROOT}/Putty/SSHDSS.C
    ${PROJECT_ROOT}/Putty/SSHGSSC.C
    ${PROJECT_ROOT}/Putty/SSHMD5.C
    ${PROJECT_ROOT}/Putty/SSHPUBK.C
    ${PROJECT_ROOT}/Putty/SSHRAND.C
    ${PROJECT_ROOT}/Putty/SSHRSA.C
    ${PROJECT_ROOT}/Putty/SSHSH256.C
    ${PROJECT_ROOT}/Putty/SSHSH512.C
    ${PROJECT_ROOT}/Putty/SSHSHA_.C
    ${PROJECT_ROOT}/Putty/SSHZLIB.C
    ${PROJECT_ROOT}/Putty/TREE234.C
    ${PROJECT_ROOT}/Putty/CHARSET/UTF8.C
    ${PROJECT_ROOT}/Putty/WILDCARD.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINGSS.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINHANDL.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINMISC.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINNET.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINNOISE.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINNOJMP.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINPGNTC.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINPROXY.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINSTORE_.C
    ${PROJECT_ROOT}/Putty/WINDOWS/WINTIME.C
    ${PROJECT_ROOT}/Putty/X11FWD.C
)

set_target_properties( libputty
    PROPERTIES
    COMPILE_FLAGS "/TC -DLibrary;_RTLDLL;NO_STRICT;SECURITY_WIN32;NET_SETUP_DIAGNOSTICS;MPEXT;_WINDOWS"
)

#-------------------------------------------------------------------------------

# список библиотек к линковке
set ( NETBOX_LIBRARIES
    libssh2.lib
    zlib.lib
    ws2_32.lib
    winhttp.lib
    Version.lib
    # kernel32.lib
    # user32.lib
    # gdi32.lib
    # winspool.lib
    # comdlg32.lib
    # advapi32.lib
    # shell32.lib
    # ole32.lib
    # oleaut32.lib
    # uuid.lib
    # odbc32.lib
    # odbccp32.lib
    libcurl
    libeay32
    ssleay32
)

add_library ( NetBox SHARED ${NETBOX_HEADERS} ${NETBOX_SOURCES} )
set_target_properties( NetBox
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -D_UNICODE;UNICODE /MTd"
    LINK_FLAGS ${CMAKE_EXE_LINK_FLAGS}
)

#move from NetBox/Debug to NetBox/
string(REPLACE "/" "\\" outdir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
add_custom_command ( TARGET NetBox
    POST_BUILD
    COMMAND copy /Y ${outdir}\\${CMAKE_BUILD_TYPE}\\ ${outdir}
    COMMAND del /F /Q ${outdir}\\${CMAKE_BUILD_TYPE}\\NetBox.dll_
    COMMAND ren ${outdir}\\${CMAKE_BUILD_TYPE}\\NetBox.dll NetBox.dll_
    WORKING_DIRECTORY ${outdir}
    VERBATIM
)
#TODO: COMMAND create_ver.py

target_link_libraries ( NetBox ${NETBOX_LIBRARIES} )

# add_dependencies ( NetBox zlib libssh2 libcurl )

#-------------------------------------------------------
# target testnetbox_01

set ( TESTNETBOX_01_DIR ${PROJECT_ROOT}/NetBox )

set ( TESTNETBOX_01_HEADERS
)

set ( TESTNETBOX_01_SOURCES
    ${TESTNETBOX_01_DIR}/testnetbox_01.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinyxml.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinystr.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinyxmlerror.cpp
    ${PROJECT_ROOT}/libs/tinyXML/tinyxmlparser.cpp
    ${PROJECT_ROOT}/NetBox/stdafx.cpp
    ${PROJECT_ROOT}/NetBox/Logging.cpp
    ${PROJECT_ROOT}/NetBox/Settings.cpp
    ${PROJECT_ROOT}/NetBox/EasyURL.cpp
    ${PROJECT_ROOT}/NetBox/Session.cpp
    ${PROJECT_ROOT}/NetBox/SessionManager.cpp
    ${PROJECT_ROOT}/NetBox/SessionEditor.cpp
    ${PROJECT_ROOT}/NetBox/SFTP.cpp
    ${PROJECT_ROOT}/NetBox/FTP.cpp
    ${PROJECT_ROOT}/NetBox/SCP.cpp
    ${PROJECT_ROOT}/Common/Classes.cpp
    ${PROJECT_ROOT}/Common/Common.cpp
    ${PROJECT_ROOT}/Common/FarUtil.cpp
    ${PROJECT_ROOT}/Common/RemoteFiles.cpp
    ${PROJECT_ROOT}/Common/Terminal.cpp
    ${PROJECT_ROOT}/Common/Exceptions.cpp
    ${PROJECT_ROOT}/Common/FileOperationProgress.cpp
    ${PROJECT_ROOT}/Common/Queue.cpp
    ${PROJECT_ROOT}/Common/SecureShell.cpp
    ${PROJECT_ROOT}/Common/SessionInfo.cpp
    ${PROJECT_ROOT}/Common/CoreMain.cpp
    ${PROJECT_ROOT}/Common/FileMasks.cpp
    ${PROJECT_ROOT}/Common/CopyParam.cpp
    ${PROJECT_ROOT}/Common/SessionData.cpp
    ${PROJECT_ROOT}/Common/Configuration.cpp
    ${PROJECT_ROOT}/Common/ScpFileSystem.cpp
    ${PROJECT_ROOT}/Common/SftpFileSystem.cpp
    ${PROJECT_ROOT}/Common/PuttyIntf.cpp
    ${PROJECT_ROOT}/Common/Cryptography.cpp
    ${PROJECT_ROOT}/Common/FileBuffer.cpp
    ${PROJECT_ROOT}/Common/NamedObjs.cpp
    ${PROJECT_ROOT}/Common/HierarchicalStorage.cpp
    ${PROJECT_ROOT}/Common/Option.cpp
    ${PROJECT_ROOT}/Common/FileInfo.cpp
    ${PROJECT_ROOT}/Common/FileSystems.cpp
    ${PROJECT_ROOT}/Common/FarPlugin.cpp
    ${PROJECT_ROOT}/Common/FarDialog.cpp
    ${PROJECT_ROOT}/Common/GUIConfiguration.cpp
    ${PROJECT_ROOT}/NetBox/WinSCPFileSystem.cpp
    ${PROJECT_ROOT}/NetBox/WinSCPPlugin.cpp
    ${PROJECT_ROOT}/NetBox/FarInterface.cpp
    ${PROJECT_ROOT}/NetBox/FarConfiguration.cpp
)

set ( TESTNETBOX_01_LIBRARIES
    ${NETBOX_LIBRARIES}
    libputty
    ${PROJECT_ROOT}/libs/boost/stage/libboost_exception-vc100-mt-gd-1_48.lib
    Version.lib
)

add_executable( testnetbox_01 ${TESTNETBOX_01_HEADERS} ${TESTNETBOX_01_SOURCES} )
set_target_properties( testnetbox_01
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -D_UNICODE;UNICODE /MDd"
    LINK_FLAGS "${CMAKE_EXE_LINK_FLAGS} /NODEFAULTLIB:LIBCMTD.LIB"
)
# add_dependencies ( testnetbox_01 zlib libssh2 libcurl )
use_precompiled_header ( TESTNETBOX_01_SOURCES stdafx.h stdafx.cpp testnetbox_01 )

# Добавить к линковке цели ${PROJECT} библиотеки ${LIBRARIES}
target_link_libraries ( testnetbox_01 ${TESTNETBOX_01_LIBRARIES} )

