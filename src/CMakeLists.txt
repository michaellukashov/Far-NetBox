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
# CMake Includes
#-------------------------------------------------------------------------------

include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/NetBox.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Install.cmake)

#-------------------------------------------------------------------------------
# Source Groups
#-------------------------------------------------------------------------------

if(OPT_USE_UNITY_BUILD)

set(NETBOX_SOURCES
  NetBox/UnityBuildCore.cpp
  NetBox/UnityBuildFilezilla.cpp
  NetBox/UnityBuildMain.cpp
)

else(OPT_USE_UNITY_BUILD)

#===============================================================================
# Base Sources (src/base/, src/nbcore/)
#===============================================================================

set(Base_SOURCES
  nbcore/nbstring.cpp
  nbcore/nbmemory.cpp
  nbcore/nbutils.cpp

  base/UnicodeString.cpp
  base/Classes.cpp
  base/Masks.cpp
  base/Sysutils.cpp
  base/StrUtils.cpp
  base/WideStrUtils.cpp
  base/LibraryLoader.cpp
)

set(Base_HEADERS
  nbcore/stdafx.h

  base/Classes.hpp
  base/Sysutils.hpp
  base/Property.hpp
  base/function2.hpp
  base/UnicodeString.hpp
  base/Masks.hpp
  base/System.StrUtils.hpp
  base/StrUtils.hpp
  base/DirectoryMonitor.hpp
  base/System.SyncObjs.hpp
  base/Windows.hpp
  base/Xml.XMLIntf.hpp
  base/LibraryLoader.hpp
  base/DynamicQueue.hpp
  base/System.IOUtils.hpp
  base/System.ShlObj.hpp
  base/WideStrUtils.hpp
  base/Math.hpp
  base/PasTools.hpp
  base/System.DateUtils.hpp
  base/ComCtrls.hpp
  base/registry.hpp
  base/contnrs.hpp
  base/SysInit.hpp
  base/System.hpp
  base/LanguagesDEPfix.hpp
  base/FileCtrl.hpp
  base/CompThread.hpp
  base/DateUtils.hpp
  base/DiscMon.hpp
)

#===============================================================================
# FileZilla Sources (FTP implementation)
#===============================================================================

set(Filezilla_HEADERS
  filezilla/AsyncSslSocketLayer.h
  filezilla/structures.h
  filezilla/stdafx.h
  filezilla/TransferSocket.h
  filezilla/ServerPath.h
  filezilla/MainThread.h
  filezilla/MFC64bitFix.h
  filezilla/FzApiStructures.h
  filezilla/FtpListResult.h
  filezilla/FtpControlSocket.h
  filezilla/FilezillaTools.h
  filezilla/FileZillaOpt.h
  filezilla/FileZillaIntf.h
  filezilla/FileZillaIntern.h
  filezilla/FileZillaApi.h
  filezilla/AsyncSocketExLayer.h
  filezilla/AsyncSocketEx.h
  filezilla/AsyncProxySocketLayer.h
  filezilla/ApiLog.h
  filezilla/afxdll.h
)

set(Filezilla_SOURCES
  filezilla/afxdll.cpp
  filezilla/FileZillaIntf.cpp
  filezilla/FileZillaIntern.cpp
  filezilla/ApiLog.cpp
  filezilla/ServerPath.cpp
  filezilla/AsyncSslSocketLayer.cpp
  filezilla/AsyncSocketExLayer.cpp
  filezilla/AsyncSocketEx.cpp
  filezilla/FileZillaApi.cpp
  filezilla/FzApiStructures.cpp
  filezilla/FtpControlSocket.cpp
  filezilla/MainThread.cpp
  filezilla/TransferSocket.cpp
  filezilla/FtpListResult.cpp
  filezilla/AsyncProxySocketLayer.cpp
  filezilla/structures.cpp
  filezilla/MFC64bitFix.cpp
  filezilla/misc/CBase64Coding.cpp
)

#===============================================================================
# WinSCP Core Sources (src/core/, src/windows/)
#===============================================================================

set(WinSCP_SOURCES
  base/Common.cpp
  base/Exceptions.cpp
  base/FileBuffer.cpp
  base/System.IOUtils.cpp
  base/Global.cpp
  base/System.SyncObjs.cpp
  base/FormatUtils.cpp
  base/ObjIDs.cpp

  core/RemoteFiles.cpp
  core/Terminal.cpp
  core/FileOperationProgress.cpp
  core/Queue.cpp
  core/SecureShell.cpp
  core/SessionInfo.cpp
  core/Script.cpp
  core/CoreMain.cpp
  core/FileMasks.cpp
  core/CopyParam.cpp
  core/SessionData.cpp
  core/Usage.cpp
  core/Configuration.cpp
  core/ScpFileSystem.cpp
  core/FtpFileSystem.cpp
  core/SftpFileSystem.cpp
  core/WebDAVFileSystem.cpp
  core/S3FileSystem.cpp
  core/PuttyIntf.cpp
  core/Cryptography.cpp
  core/NamedObjs.cpp
  core/HierarchicalStorage.cpp
  core/Option.cpp
  core/FileInfo.cpp
  core/FileSystems.cpp
  core/Bookmarks.cpp
  core/WinSCPSecurity.cpp
  core/Http.cpp
  core/NeonIntf.cpp

  windows/SynchronizeController.cpp
  windows/GUITools.cpp
  windows/GUIConfiguration.cpp
  windows/Tools.cpp
  windows/ProgParams.cpp
  windows/UserInterface.cpp
  windows/WinInterface.cpp
)

set(WinSCP_HEADERS
  core/FileMasks.h
  core/Bookmarks.h
  core/HierarchicalStorage.h
  core/Configuration.h
  core/Usage.h
  core/Terminal.h
  core/SessionData.h
  core/NamedObjs.h
  core/SessionInfo.h
  core/CopyParam.h
  core/SecureShell.h
  core/FileSystems.h
  core/ScpFileSystem.h
  core/WebDAVFileSystem.h
  core/SftpFileSystem.h
  core/FtpFileSystem.h
  core/S3FileSystem.h
  core/Queue.h
  core/RemoteFiles.h
  core/Cryptography.h
  core/Http.h
  core/FileOperationProgress.h
  core/Option.h
  core/Interface.h
  core/NeonIntf.h
  core/Script.h
  core/PuttyTools.h
  core/WinSCPSecurity.h
  core/PuttyIntf.h
  core/FileInfo.h
  core/CoreMain.h

  windows/WinInterface.h
  windows/GUIConfiguration.h
  windows/TerminalManager.h
  windows/Tools.h
  windows/GUITools.h
  windows/VCLCommon.h
  windows/Setup.h
  windows/ProgParams.h
  windows/CustomWinConfiguration.h
  windows/WinConfiguration.h
  windows/SynchronizeController.h
)

#===============================================================================
# NetBox Plugin Sources (src/NetBox/)
#===============================================================================

set(NetBox_SOURCES
  NetBox/NetBox.cpp
  NetBox/FarDialog.cpp
  NetBox/FarPlugin.cpp
  NetBox/FarUtils.cpp
  NetBox/WinSCPFileSystem.cpp
  NetBox/WinSCPDialogs.cpp
  NetBox/WinSCPPlugin.cpp
  NetBox/FarConfiguration.cpp
  NetBox/FarInterface.cpp
  NetBox/XmlStorage.cpp
  NetBox/Far3Storage.cpp
  NetBox/FarPluginStrings.cpp
  NetBox/MessageDlg.cpp
)

set(NetBox_HEADERS
  NetBox/FarUtils.h
  NetBox/FarPlugin.h
  NetBox/FarConfiguration.h
  NetBox/WinSCPPlugin.h
  NetBox/WinSCPFileSystem.h
  NetBox/FarDialog.h
  NetBox/XmlStorage.h
  NetBox/FarInterface.h
  NetBox/resource.h
  NetBox/FarPluginStrings.h
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
  ${CMAKE_SOURCE_DIR}/libs
  ${CMAKE_SOURCE_DIR}/libs/fmt
  ${CMAKE_SOURCE_DIR}/libs/tinylog
  ${CMAKE_SOURCE_DIR}/libs/GSL/include
  include
  base
  resource
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

  include/FastDelegateBind.h
  include/FastDelegate.h
  include/type_traits.h
  include/rtti.hpp
  include/nbtypes.h
  include/nbglobals.h
  include/nbsystem.h
  include/nbcore.h
  include/nbstring.h
  include/nbstring.inl
  include/nbutils.h

  base/Property.hpp
  base/Global.h
  base/Common.h
  base/UnicodeString.hpp
  base/Exceptions.h
  base/Sysutils.hpp
  base/StrUtils.hpp
  base/MsgIDs.h
  base/vcl.h
  base/FileBuffer.h
  base/System.IOUtils.hpp
  base/FormatUtils.h
  base/ObjIDs.h

  core/WebDAVFileSystem.h
  core/S3FileSystem.h
  core/Queue.h
  core/Terminal.h
  core/FileSystems.h
  core/SessionInfo.h
  core/SessionData.h
  core/RemoteFiles.h
  core/Http.h
  core/FtpFileSystem.h
  core/FileMasks.h
  core/Configuration.h
  core/WinSCPSecurity.h
  core/SecureShell.h
  core/ScpFileSystem.h
  core/NeonIntf.h
  core/Interface.h
  core/FileInfo.h
  core/CopyParam.h
  core/SftpFileSystem.h
  core/PuttyIntf.h
  core/Option.h
  core/HierarchicalStorage.h
  core/Cryptography.h
  core/PuttyTools.h
  core/NamedObjs.h
  core/FileOperationProgress.h
  core/Bookmarks.h
  core/CoreMain.h

  windows/WinInterface.h
  windows/GUITools.h
  windows/GUIConfiguration.h
  windows/Tools.h
  windows/ProgParams.h
  windows/SynchronizeController.h

  resource/TextsCore.h
  resource/rtlconsts.h
  resource/HelpCore.h
  resource/TextsFileZilla.h
  PluginSDK/Far3/plugin.hpp
  PluginSDK/Far3/farcolor.hpp
  PluginSDK/Far3/PluginSettings.hpp

  resource/TextsFileZilla.rc
  resource/TextsCore1.rc
  resource/rtlconsts.rc
  resource/TextsWin1.rc
  NetBox/NetBox.rc
  NetBox/NetBox.def
)

#-------------------------------------------------------------------------------
# Resources Target
#-------------------------------------------------------------------------------

add_custom_target(NetBoxResources SOURCES
  NetBox/NetBoxEng.lng
  NetBox/NetBoxRus.lng
  NetBox/NetBoxPol.lng
  NetBox/NetBoxSpa.lng
  NetBox/resource.h.template
  NetBox/NetBox.rc
  NetBox/NetBox.rc.template
  ${CMAKE_SOURCE_DIR}/.github/workflows/release.yml
  ${CMAKE_SOURCE_DIR}/appveyor.yml
  ${CMAKE_SOURCE_DIR}/appveyor.bat
  ${CMAKE_SOURCE_DIR}/ChangeLog
  ${CMAKE_SOURCE_DIR}/README.md
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
  ${CMAKE_SOURCE_DIR}/libs
  ${CMAKE_SOURCE_DIR}/libs/openssl-3/include
  ${CMAKE_SOURCE_DIR}/libs/putty
  ${CMAKE_SOURCE_DIR}/libs/putty/windows
  ${CMAKE_SOURCE_DIR}/libs/neon/src
  ${CMAKE_SOURCE_DIR}/libs/tinyxml2
  ${CMAKE_SOURCE_DIR}/libs/expat/lib
  ${CMAKE_SOURCE_DIR}/libs/atlmfc/include
  ${CMAKE_SOURCE_DIR}/libs/fmt
  ${CMAKE_SOURCE_DIR}/libs/tinylog
  ${CMAKE_SOURCE_DIR}/libs/libs3/inc
  ${CMAKE_SOURCE_DIR}/libs/GSL/include
  include
  filezilla
  base
  core
  windows
  resource
  PluginSDK/Far3
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

netbox_setup_plugin_installation(NetBox)
