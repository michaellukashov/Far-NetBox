#===============================================================================
# NetBox Main Plugin CMakeLists.txt
#
# Purpose: Build the NetBox plugin DLL for Far Manager
#
# Components:
#   - Base library (src/base/, src/nbcore/)
#   - FileZilla FTP implementation (src/filezilla/)
#   - WinSCP core (src/core/, src/windows/)
#   - NetBox plugin (src/NetBox/)
#   - Resources (src/resource/)
#===============================================================================

#-------------------------------------------------------------------------------
# Source Groups
#-------------------------------------------------------------------------------

if(OPT_USE_UNITY_BUILD)

set(NETBOX_SOURCES
  src/NetBox/UnityBuildCore.cpp
  src/NetBox/UnityBuildFilezilla.cpp
  src/NetBox/UnityBuildMain.cpp
)

else(OPT_USE_UNITY_BUILD)

#===============================================================================
# Base Sources (src/base/, src/nbcore/)
#===============================================================================

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

#===============================================================================
# FileZilla Sources (FTP implementation)
#===============================================================================

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

#===============================================================================
# WinSCP Core Sources (src/core/, src/windows/)
#===============================================================================

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

#===============================================================================
# NetBox Plugin Sources (src/NetBox/)
#===============================================================================

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

# Combine all sources for non-unity build
set(NETBOX_SOURCES
  ${WinSCP_HEADERS}
  ${WinSCP_SOURCES}

  ${Filezilla_HEADERS}
  ${Filezilla_SOURCES}

  ${NetBox_HEADERS}
  ${NetBox_SOURCES}
)

# Source groups for IDE organization
source_group("Filezilla headers" FILES ${Filezilla_HEADERS})
source_group("Filezilla sources" FILES ${Filezilla_SOURCES})
source_group("WinSCP headers" FILES ${WinSCP_HEADERS})
source_group("WinSCP sources" FILES ${WinSCP_SOURCES})
source_group("Base headers" FILES ${Base_HEADERS})
source_group("Base sources" FILES ${Base_SOURCES})
source_group("NetBox headers" FILES ${NetBox_HEADERS})
source_group("NetBox sources" FILES ${NetBox_SOURCES})

#-------------------------------------------------------------------------------
# baselib (static library for base classes)
#-------------------------------------------------------------------------------

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

#===============================================================================
# Additional Headers and Resources
#===============================================================================

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

#-------------------------------------------------------------------------------
# Resources Target
#-------------------------------------------------------------------------------

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

#===============================================================================
# Library Dependencies
#===============================================================================

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

#===============================================================================
# NetBox Shared Library (DLL)
#===============================================================================

add_library(NetBox SHARED
  ${NETBOX_SOURCES}
)

target_include_directories(NetBox PRIVATE
  libs
  libs/openssl-3/include
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

#===============================================================================
# Plugin Directory Installation (Optional)
#===============================================================================

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

#===============================================================================
# Compiler and Linker Flags (using ucm.cmake)
#===============================================================================

ucm_set_runtime(STATIC)
ucm_add_flags(CXX /EHs)
ucm_remove_flags(CXX C /Ob1 /Ob2 CONFIG Release MinSizeRel RelWithDebInfo)
ucm_add_flags(CXX C /fp:fast /Gw /GL CONFIG Release RelWithDebInfo MinSizeRel)
ucm_add_flags(CXX C /RTC1 /DDEBUG CONFIG Debug)
ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_RELEASE} CONFIG Release RelWithDebInfo MinSizeRel)
ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_DEBUG} CONFIG Debug)
ucm_add_linker_flags(MODULE SHARED /DEBUG /MAP CONFIG RelWithDebInfo)
