#pragma once

#include <nbsystem.h>

using TObjectClassId = uint16_t;

extern "C" {

extern const TObjectClassId OBJECT_CLASS_TSimpleThread;
extern const TObjectClassId OBJECT_CLASS_TSignalThread;

extern const TObjectClassId OBJECT_CLASS_TObject;
extern const TObjectClassId OBJECT_CLASS_TPersistent;
extern const TObjectClassId OBJECT_CLASS_TListBase;
extern const TObjectClassId OBJECT_CLASS_TList;
extern const TObjectClassId OBJECT_CLASS_TObjectList;
extern const TObjectClassId OBJECT_CLASS_TStrings;
extern const TObjectClassId OBJECT_CLASS_TStringList;

extern const TObjectClassId OBJECT_CLASS_Exception;
extern const TObjectClassId OBJECT_CLASS_EAbort;
extern const TObjectClassId OBJECT_CLASS_EAccessViolation;
extern const TObjectClassId OBJECT_CLASS_EFileNotFoundError;
extern const TObjectClassId OBJECT_CLASS_EOSError;
extern const TObjectClassId OBJECT_CLASS_EInvalidOperation;
extern const TObjectClassId OBJECT_CLASS_EConvertError;
extern const TObjectClassId OBJECT_CLASS_EDirectoryNotFoundException;
extern const TObjectClassId OBJECT_CLASS_EFatal;
extern const TObjectClassId OBJECT_CLASS_ECRTExtException;
extern const TObjectClassId OBJECT_CLASS_EOSExtException;
extern const TObjectClassId OBJECT_CLASS_ESkipFile;
extern const TObjectClassId OBJECT_CLASS_ExtException;
extern const TObjectClassId OBJECT_CLASS_ETerminal;
extern const TObjectClassId OBJECT_CLASS_ECommand;
extern const TObjectClassId OBJECT_CLASS_EConnectionFatal;
extern const TObjectClassId OBJECT_CLASS_EScp;
extern const TObjectClassId OBJECT_CLASS_EScpFileSkipped;

extern const TObjectClassId OBJECT_CLASS_ETerminate;
extern const TObjectClassId OBJECT_CLASS_ECallbackGuardAbort;
extern const TObjectClassId OBJECT_CLASS_EStreamError;
extern const TObjectClassId OBJECT_CLASS_EFCreateError;
extern const TObjectClassId OBJECT_CLASS_EFOpenError;

extern const TObjectClassId OBJECT_CLASS_TBookmarks;
extern const TObjectClassId OBJECT_CLASS_TBookmarkList;
extern const TObjectClassId OBJECT_CLASS_TBookmark;
extern const TObjectClassId OBJECT_CLASS_TConfiguration;
extern const TObjectClassId OBJECT_CLASS_TCopyParamType;

extern const TObjectClassId OBJECT_CLASS_TSinkFileParams;
extern const TObjectClassId OBJECT_CLASS_TFileTransferData;
extern const TObjectClassId OBJECT_CLASS_TOverwriteFileParams;
extern const TObjectClassId OBJECT_CLASS_TOpenRemoteFileParams;
extern const TObjectClassId OBJECT_CLASS_TCustomFileSystem;
extern const TObjectClassId OBJECT_CLASS_TFTPFileSystem;
extern const TObjectClassId OBJECT_CLASS_TNamedObject;
extern const TObjectClassId OBJECT_CLASS_TNamedObjectList;

extern const TObjectClassId OBJECT_CLASS_TTunnelThread;
extern const TObjectClassId OBJECT_CLASS_TSynchronizeFileData;
extern const TObjectClassId OBJECT_CLASS_TSFTPPacket;
extern const TObjectClassId OBJECT_CLASS_TTunnelUI;
extern const TObjectClassId OBJECT_CLASS_TSynchronizeData;
extern const TObjectClassId OBJECT_CLASS_TSFTPQueuePacket;
extern const TObjectClassId OBJECT_CLASS_TSFTPQueue;
extern const TObjectClassId OBJECT_CLASS_TPuttyCleanupThread;
extern const TObjectClassId OBJECT_CLASS_TPuttyPasswordThread;

extern const TObjectClassId OBJECT_CLASS_TTerminalQueue;
extern const TObjectClassId OBJECT_CLASS_TQueueItem;
extern const TObjectClassId OBJECT_CLASS_TQueueItemProxy;
extern const TObjectClassId OBJECT_CLASS_TBootstrapQueueItem;
extern const TObjectClassId OBJECT_CLASS_TLocatedQueueItem;
extern const TObjectClassId OBJECT_CLASS_TTransferQueueItem;
extern const TObjectClassId OBJECT_CLASS_TUploadQueueItem;
extern const TObjectClassId OBJECT_CLASS_TDownloadQueueItem;
extern const TObjectClassId OBJECT_CLASS_TDeleteQueueItem;
extern const TObjectClassId OBJECT_CLASS_TTerminalThread;
extern const TObjectClassId OBJECT_CLASS_TRemoteFile;
extern const TObjectClassId OBJECT_CLASS_TRemoteDirectoryFile;
extern const TObjectClassId OBJECT_CLASS_TRemoteParentDirectory;
extern const TObjectClassId OBJECT_CLASS_TRemoteFileList;
extern const TObjectClassId OBJECT_CLASS_TRemoteDirectory;
extern const TObjectClassId OBJECT_CLASS_TRemoteProperties;
extern const TObjectClassId OBJECT_CLASS_TChecklistItem;
extern const TObjectClassId OBJECT_CLASS_TParallelTransferQueueItem;
extern const TObjectClassId OBJECT_CLASS_TS3FileSystem;
extern const TObjectClassId OBJECT_CLASS_TSessionActionRecord;
extern const TObjectClassId OBJECT_CLASS_TMoveFileParams;
extern const TObjectClassId OBJECT_CLASS_TFilesFindParams;
extern const TObjectClassId OBJECT_CLASS_TSCPFileSystem;
extern const TObjectClassId OBJECT_CLASS_TSecureShell;
extern const TObjectClassId OBJECT_CLASS_TSessionData;
extern const TObjectClassId OBJECT_CLASS_TStoredSessionList;
extern const TObjectClassId OBJECT_CLASS_TSessionUI;
extern const TObjectClassId OBJECT_CLASS_TTerminal;
extern const TObjectClassId OBJECT_CLASS_TSecondaryTerminal;
extern const TObjectClassId OBJECT_CLASS_TTerminalList;
extern const TObjectClassId OBJECT_CLASS_TCustomCommandParams;
extern const TObjectClassId OBJECT_CLASS_TCalculateSizeParams;
extern const TObjectClassId OBJECT_CLASS_TMakeLocalFileListParams;
extern const TObjectClassId OBJECT_CLASS_TCollectedFileList;
extern const TObjectClassId OBJECT_CLASS_TLocalFile;
extern const TObjectClassId OBJECT_CLASS_TSFTPFileSystem;
extern const TObjectClassId OBJECT_CLASS_TWebDAVFileSystem;
extern const TObjectClassId OBJECT_CLASS_TGUICopyParamType;
extern const TObjectClassId OBJECT_CLASS_TCopyParamRule;
extern const TObjectClassId OBJECT_CLASS_TGUIConfiguration;

extern const TObjectClassId OBJECT_CLASS_TFarConfiguration;
extern const TObjectClassId OBJECT_CLASS_TFarDialog;
extern const TObjectClassId OBJECT_CLASS_TFarDialogContainer;
extern const TObjectClassId OBJECT_CLASS_TFarDialogItem;
extern const TObjectClassId OBJECT_CLASS_TFarBox;
extern const TObjectClassId OBJECT_CLASS_TFarButton;

extern const TObjectClassId OBJECT_CLASS_TWinSCPPlugin;

extern const TObjectClassId OBJECT_CLASS_TFarLister;
extern const TObjectClassId OBJECT_CLASS_TCustomFarPlugin;
extern const TObjectClassId OBJECT_CLASS_TCustomFarFileSystem;
extern const TObjectClassId OBJECT_CLASS_TCustomFarPanelItem;
extern const TObjectClassId OBJECT_CLASS_TFarPanelItem;
extern const TObjectClassId OBJECT_CLASS_TFarPanelItemData;
extern const TObjectClassId OBJECT_CLASS_THintPanelItem;
extern const TObjectClassId OBJECT_CLASS_TFarMenuItems;
extern const TObjectClassId OBJECT_CLASS_TWinSCPFileSystem;
extern const TObjectClassId OBJECT_CLASS_TSessionPanelItem;
extern const TObjectClassId OBJECT_CLASS_TSessionFolderPanelItem;
extern const TObjectClassId OBJECT_CLASS_TRemoteFilePanelItem;

extern const TObjectClassId OBJECT_CLASS_TFarCheckBox;
extern const TObjectClassId OBJECT_CLASS_TFarRadioButton;
extern const TObjectClassId OBJECT_CLASS_TFarEdit;
extern const TObjectClassId OBJECT_CLASS_TFarSeparator;
extern const TObjectClassId OBJECT_CLASS_TFarText;
extern const TObjectClassId OBJECT_CLASS_TFarList;
extern const TObjectClassId OBJECT_CLASS_TFarListBox;
extern const TObjectClassId OBJECT_CLASS_TFarComboBox;

extern const TObjectClassId OBJECT_CLASS_TPluginIdleThread;
extern const TObjectClassId OBJECT_CLASS_TFarMessageData;
extern const TObjectClassId OBJECT_CLASS_TTabButton;
extern const TObjectClassId OBJECT_CLASS_TRightsContainer;
extern const TObjectClassId OBJECT_CLASS_TCopyParamsContainer;
extern const TObjectClassId OBJECT_CLASS_TLabelList;

} // extern "C"

