cmake_minimum_required (VERSION 2.8)

#-------------------------------------------------------------------------------
# Устанавливаем флаги компиляции ( -DCMAKE_BUILD_TYPE=[Debug,Release] )

set ( CMAKE_CXX_FLAGS
    "-DWIN32 /EHsc" )
set ( CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG;_MT" )
set ( CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG;BOOST_DISABLE_ASSERTS" )

set ( CMAKE_EXE_LINK_FLAGS
    "/INCREMENTAL:NO /NODEFAULTLIB:LIBCMT.LIB /NODEFAULTLIB:MSVCRT.LIB"
    #/NODEFAULTLIB:LIBCMTD.LIB 
)

# Дополнительные флаги компиляции
add_definitions ( "-D_UNICODE;UNICODE;NETBOX_DEBUG;USE_FASTFLOW;CURL_STATICLIB;_USRDLL;NETBOX_EXPORTS" )
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
    # ${PROJECT_ROOT}/NetBox/WinSCPFileSystem.cpp
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

# список библиотек к линковке
set ( NETBOX_LIBRARIES
    libssh2.lib
    zlib.lib
    ws2_32.lib
    winhttp.lib
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
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} /MTd"
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
    ${PROJECT_ROOT}/NetBox/stdafx.cpp
    ${PROJECT_ROOT}/NetBox/SCP.cpp
    ${PROJECT_ROOT}/NetBox/WinSCPPlugin.cpp
)

set ( TESTNETBOX_01_LIBRARIES
    ${NETBOX_LIBRARIES}
    ${PROJECT_ROOT}/libs/boost/stage/libboost_exception-vc100-mt-gd-1_48.lib
)

add_executable( testnetbox_01 ${TESTNETBOX_01_HEADERS} ${TESTNETBOX_01_SOURCES} )
set_target_properties( testnetbox_01
    PROPERTIES
    COMPILE_FLAGS "${CMAKE_CXX_FLAGS} /MDd"
    LINK_FLAGS "${CMAKE_EXE_LINK_FLAGS} /NODEFAULTLIB:LIBCMTD.LIB"
)
# add_dependencies ( testnetbox_01 zlib libssh2 libcurl )

# Добавить к линковке цели ${PROJECT} библиотеки ${LIBRARIES}
target_link_libraries ( testnetbox_01 ${TESTNETBOX_01_LIBRARIES} )

