#include <ObjIDs.h>
#include <rtti.hpp>

extern "C" {

const TObjectClassId OBJECT_CLASS_TObject = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TPersistent = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TListBase = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TList = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TObjectList = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TStrings = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TStringList = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_Exception = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EAbort = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EAccessViolation = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EFileNotFoundError = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EOSError = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EInvalidOperation = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EConvertError = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EDirectoryNotFoundException = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_ExtException = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_ESkipFile = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_EOSExtException = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_ECRTExtException = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EFatal = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_ETerminate = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_ECallbackGuardAbort = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EStreamError = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EFCreateError = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EFOpenError = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_ETerminal = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_ECommand = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EScp = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_ESsh = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_EConnectionFatal = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TPuttyCleanupThread = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TGUICopyParamType = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCopyParamRule = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TPuttyPasswordThread = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TGUIConfiguration = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TSimpleThread = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSignalThread = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TTerminalQueue = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TQueueItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TQueueItemProxy = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TBootstrapQueueItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TLocatedQueueItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TTransferQueueItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TUploadQueueItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TDownloadQueueItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TDeleteQueueItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TTerminalThread = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TWebDAVFileSystem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSCPFileSystem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TS3FileSystem = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TRemoteFile = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TRemoteDirectoryFile = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TRemoteParentDirectory = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TRemoteFileList = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TRemoteDirectory = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TRemoteProperties = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TChecklistItem = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TParallelTransferQueueItem = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TSynchronizeData = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSynchronizeFileData = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TTunnelUI = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TTunnelThread = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFilesFindParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TMoveFileParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TTerminal = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSecondaryTerminal = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TTerminalList = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCustomCommandParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCalculateSizeParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TMakeLocalFileListParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCollectedFileList = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TLocalFile = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TSFTPQueue = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSFTPQueuePacket = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSFTPPacket = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSFTPFileSystem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSessionActionRecord = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSessionUI = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TSessionData = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TStoredSessionList = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TNamedObject = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TNamedObjectList = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSecureShell = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFTPFileSystem = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TSinkFileParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFileTransferData = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TOverwriteFileParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TOpenRemoteFileParams = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCustomFileSystem = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TBookmarks = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TBookmarkList = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TBookmark = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCopyParamType = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TConfiguration = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_EScpFileSkipped = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TFarConfiguration = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TFarDialog = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarDialogContainer = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarDialogItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarBox = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarButton = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarCheckBox = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarRadioButton = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarEdit = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarSeparator = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarText = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarList = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarListBox = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarComboBox = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarLister = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TCustomFarPlugin = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TPluginIdleThread = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCustomFarFileSystem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCustomFarPanelItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarPanelItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarPanelItemData = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_THintPanelItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarMenuItems = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TTabButton = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TRightsContainer = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TCopyParamsContainer = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TLabelList = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TWinSCPFileSystem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSessionPanelItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TSessionFolderPanelItem = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TRemoteFilePanelItem = static_cast<TObjectClassId>(nb::counter_id());

const TObjectClassId OBJECT_CLASS_TWinSCPPlugin = static_cast<TObjectClassId>(nb::counter_id());
const TObjectClassId OBJECT_CLASS_TFarMessageData = static_cast<TObjectClassId>(nb::counter_id());

} // extern "C"
