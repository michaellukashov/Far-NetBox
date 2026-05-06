#===============================================================================
# NetBox Source Groups Configuration Module
#
# Purpose: Define source file groups for different components
#
# Usage:
#   include(${CMAKE_SOURCE_DIR}/cmake/SourceGroups.cmake)
#   netbox_configure_source_groups()
#===============================================================================

#===============================================================================
# Source Group Configuration Function
#===============================================================================

function(netbox_configure_source_groups)
  if(OPT_USE_UNITY_BUILD)
    netbox_configure_unity_build()
    # Set directly to parent scope
    set(NETBOX_SOURCES
      NetBox/UnityBuildCore.cpp
      NetBox/UnityBuildFilezilla.cpp
      NetBox/UnityBuildMain.cpp
    PARENT_SCOPE)
  else()
    # Initialize empty sources lis
    set(NETBOX_SOURCES "")

    # Add all source groups
    netbox_define_base_sources()
    list(APPEND NETBOX_SOURCES ${Base_SOURCES})

    netbox_define_filezilla_sources()
    list(APPEND NETBOX_SOURCES ${Filezilla_SOURCES})

    netbox_define_core_sources()
    list(APPEND NETBOX_SOURCES ${Core_SOURCES})

    netbox_define_windows_sources()
    list(APPEND NETBOX_SOURCES ${Windows_SOURCES})

    netbox_define_netbox_sources()
    list(APPEND NETBOX_SOURCES ${NetBox_SOURCES})

    netbox_define_resource_sources()
    list(APPEND NETBOX_SOURCES ${Resource_SOURCES})

    # Set the combined sources to parent scope
    set(NETBOX_SOURCES "${NETBOX_SOURCES}" PARENT_SCOPE)
  endif()
endfunction()

#===============================================================================
# Unity Build Configuration
#===============================================================================

function(netbox_configure_unity_build)
  set(NETBOX_SOURCES
    NetBox/UnityBuildCore.cpp
    NetBox/UnityBuildFilezilla.cpp
    NetBox/UnityBuildMain.cpp
  PARENT_SCOPE)
endfunction()

#===============================================================================
# Standard Build Configuration
#===============================================================================

function(netbox_configure_standard_build)
  # Initialize empty sources lis
  set(NETBOX_SOURCES "")

  # Add all source groups
  netbox_define_base_sources()
  list(APPEND NETBOX_SOURCES ${Base_SOURCES})

  netbox_define_filezilla_sources()
  list(APPEND NETBOX_SOURCES ${Filezilla_SOURCES})

  netbox_define_core_sources()
  list(APPEND NETBOX_SOURCES ${Core_SOURCES})

  netbox_define_windows_sources()
  list(APPEND NETBOX_SOURCES ${Windows_SOURCES})

  netbox_define_netbox_sources()
  list(APPEND NETBOX_SOURCES ${NetBox_SOURCES})

  netbox_define_resource_sources()
  list(APPEND NETBOX_SOURCES ${Resource_SOURCES})

  # Set the combined sources to parent scope
  set(NETBOX_SOURCES "${NETBOX_SOURCES}" PARENT_SCOPE)
endfunction()

#===============================================================================
# Individual Component Source Definitions
#===============================================================================

function(netbox_define_base_sources)
  set(Base_SOURCES
    nbcore/nbstring.cpp
    nbcore/nbmemory.cpp
    nbcore/nbutils.cpp
    nbcore/SessionHistory.cpp
    base/UnicodeString.cpp
    base/Classes.cpp
    base/Masks.cpp
    base/Sysutils.cpp
    base/StrUtils.cpp
    base/WideStrUtils.cpp
    base/LibraryLoader.cpp
    base/SecureString.cpp
  PARENT_SCOPE)
endfunction()

function(netbox_define_filezilla_sources)
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
  PARENT_SCOPE)
endfunction()

function(netbox_define_core_sources)
  set(Core_SOURCES
    base/Common.cpp
    base/Exceptions.cpp
    base/FileBuffer.cpp
    base/System.IOUtils.cpp
    base/Global.cpp
    base/LogContext.cpp
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
    core/Certificates.cpp
    core/ScpFileSystem.cpp
    core/FtpFileSystem.cpp
    core/SftpFileSystem.cpp
    core/WebDAVFileSystem.cpp
    core/S3FileSystem.cpp
    core/PuttyIntf.cpp
    core/KittyKeyboard.cpp
    core/Win32Input.cpp
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
    windows/WinConfiguration.cpp
    windows/Tools.cpp
    windows/ProgParams.cpp
    windows/MasterPassword.cpp
    windows/UserInterface.cpp
    windows/WinInterface.cpp
  PARENT_SCOPE)
endfunction()

function(netbox_define_windows_sources)
  # Windows-specific code is included in core sources
  # This function is kept for consistency but can be empty
  set(Windows_SOURCES
  PARENT_SCOPE)
endfunction()

function(netbox_define_netbox_sources)
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
  PARENT_SCOPE)
endfunction()

function(netbox_define_resource_sources)
  set(Resource_SOURCES
    include/FastDelegateBind.h
    include/FastDelegate.h
    include/type_traits.h
    include/rtti.hpp
    include/nbtypes.h
    resource/rtlconsts.rc
    resource/TextsCore1.rc
    resource/TextsFileZilla.rc
    resource/TextsWin1.rc
    NetBox/resource.h
    NetBox/NetBox.rc
    NetBox/NetBox.def
  PARENT_SCOPE)
endfunction()
