
#pragma once

#include <Classes.hpp>
// #include <Buttons.hpp>
#include <Interface.h>
#include <WinConfiguration.h>
#include <Terminal.h>
#include <SynchronizeController.h>
#include <Script.h>
#if defined(FARPLUGIN)
#include <GUIConfiguration.h>
#include <SynchronizeController.h>
#endif // FARPLUGIN
// #include <Script.h>
// #include "HistoryComboBox.hpp"

#ifdef LOCALINTERFACE
#include <LocalInterface.h>
#endif

// #define SITE_ICON 1
// #define SITE_FOLDER_ICON 2
// #define WORKSPACE_ICON 3

class TStoredSessionList;
class TConfiguration;
class TTerminal;

constexpr const uint32_t mpNeverAskAgainCheck =   0x01;
constexpr const uint32_t mpAllowContinueOnError = 0x02;

extern HINSTANCE HInstance;

#define UPLOAD_IF_ANY_SWITCH L"UploadIfAny"
#define UPLOAD_SWITCH L"Upload"
#define SYNCHRONIZE_SWITCH L"Synchronize"
#define KEEP_UP_TO_DATE_SWITCH L"KeepUpToDate"
#define JUMPLIST_SWITCH L"JumpList"
#define DESKTOP_SWITCH L"Desktop"
#define SEND_TO_HOOK_SWITCH L"SendToHook"
#define UNSAFE_SWITCH L"Unsafe"
#define DEFAULTS_SWITCH L"Defaults"
#define NEWINSTANCE_SWICH L"NewInstance"
#define KEYGEN_SWITCH L"KeyGen"
#define KEYGEN_OUTPUT_SWITCH L"Output"
#define KEYGEN_COMMENT_SWITCH L"Comment"
#define KEYGEN_CHANGE_PASSPHRASE_SWITCH L"ChangePassphrase"
#define KEYGEN_CERTIFICATE_SWITCH L"Certificate"
#define LOG_SWITCH L"Log"
#define LOGSIZE_SWITCH L"LogSize"
#define LOGSIZE_SEPARATOR L"*"
#define INI_SWITCH L"Ini"
#define RAW_CONFIG_SWITCH L"RawConfig"
#define FINGERPRINTSCAN_SWITCH L"FingerprintScan"
#define DUMPCALLSTACK_SWITCH L"DumpCallstack"
#define INFO_SWITCH L"Info"
#define COMREGISTRATION_SWITCH L"ComRegistration"
#define BROWSE_SWITCH L"Browse"
#define NOINTERACTIVEINPUT_SWITCH L"NoInteractiveInput"
#define STDOUT_SWITCH L"StdOut"
#define STDIN_SWITCH L"StdIn"
#define STDINOUT_BINARY_VALUE L"binary"
#define STDINOUT_CHUNKED_VALUE L"chunked"

#define DUMPCALLSTACK_EVENT L"WinSCPCallstack%d"

struct NB_CORE_EXPORT TMessageParams final : public TObject
{
  NB_DISABLE_COPY(TMessageParams)
public:
  explicit TMessageParams(uint32_t AParams) noexcept;
  explicit TMessageParams(const TQueryParams * AParams) noexcept;
  void Assign(const TMessageParams * AParams);
  void Assign(const TQueryParams * AParams);

  const TQueryButtonAlias * Aliases{nullptr};
  uint32_t AliasesCount{0};
  uint32_t Flags{0};
  uint32_t Params{0};
  uint32_t Timer{0};
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uint32_t TimerAnswers{0};
  TQueryType TimerQueryType{static_cast<TQueryType>(-1)};
  uint32_t Timeout{0};
  uint32_t TimeoutAnswer{0};
  uint32_t TimeoutResponse{0};
  UnicodeString NeverAskAgainTitle;
  uint32_t NeverAskAgainAnswer{0};
  bool NeverAskAgainCheckedInitially{false};
  bool AllowHelp{false};
  UnicodeString ImageName;
  UnicodeString MoreMessagesUrl;
  TSize MoreMessagesSize{0};
  UnicodeString CustomCaption;

private:
  void Reset();
};

class TCustomScpExplorerForm;
TCustomScpExplorerForm * CreateScpExplorer();

UnicodeString GetThemeName(bool Dark);
void ConfigureInterface();

void DoProductLicense();

constexpr const wchar_t * AppName = L"WinSCP";

void SetOnForeground(bool OnForeground);
void FlashOnBackground();

void TerminateApplication();
void ShowExtendedExceptionEx(TTerminal * Terminal, Exception * E);
// void FormHelp(TCustomForm * Form);
void SearchHelp(const UnicodeString & Message);
void MessageWithNoHelp(const UnicodeString & Message);

class TProgramParams;
bool CheckSafe(TProgramParams * Params);
void CheckLogParam(TProgramParams * Params);
bool CheckXmlLogParam(TProgramParams * Params);

#if 0
UnicodeString GetToolbarKey(const UnicodeString & ToolbarName);
UnicodeString GetToolbarsLayoutStr(TControl * OwnerControl);
void LoadToolbarsLayoutStr(TControl * OwnerControl, const UnicodeString & LayoutStr);

namespace Tb2item { class TTBCustomItem; }
namespace Tbx { class TTBXSeparatorItem; }
Tbx::TTBXSeparatorItem * AddMenuSeparator(Tb2item::TTBCustomItem * Menu);
void AddMenuLabel(Tb2item::TTBCustomItem * Menu, const UnicodeString & Label);
void ClickToolbarItem(Tb2item::TTBCustomItem * Item, bool PositionCursor);

void InitiateDialogTimeout(TForm * Dialog, uint32_t Timeout, TButton * Button, uint32_t Answer = 0);
#endif // #if 0

// windows\WinHelp.cpp
void InitializeWinHelp();
void FinalizeWinHelp();

// windows\WinInterface.cpp
uint32_t MessageDialog(const UnicodeString & Msg, TQueryType Type,
  uint32_t Answers, const UnicodeString & HelpKeyword = HELP_NONE, const TMessageParams * Params = nullptr);
uint32_t MessageDialog(int32_t Ident, TQueryType Type,
  uint32_t Answers, const UnicodeString & HelpKeyword = HELP_NONE, const TMessageParams * Params = nullptr);
uint32_t SimpleErrorDialog(const UnicodeString & Msg, const UnicodeString & MoreMessages = L"");

uint32_t MoreMessageDialog(const UnicodeString & Message,
  TStrings * MoreMessages, TQueryType Type, uint32_t Answers,
  const UnicodeString & HelpKeyword, const TMessageParams * Params = nullptr);

uint32_t ExceptionMessageDialog(Exception * E, TQueryType Type,
  const UnicodeString & MessageFormat = L"", uint32_t Answers = qaOK,
  const UnicodeString & HelpKeyword = HELP_NONE, const TMessageParams * Params = nullptr);
uint32_t FatalExceptionMessageDialog(
  Exception * E, TQueryType Type, const UnicodeString & MessageFormat = L"", uint32_t Answers = qaOK,
  const UnicodeString & HelpKeyword = HELP_NONE, const TMessageParams * Params = nullptr);

// forms\Custom.cpp
TSessionData * DoSaveSession(TSessionData * SessionData,
  TSessionData * OriginalSession, bool ForceDialog,
  TStrings * AdditionalFolders);
void SessionNameValidate(const UnicodeString & Text,
  const UnicodeString & OriginalName);
bool DoSaveWorkspaceDialog(UnicodeString & WorkspaceName,
  bool * SavePasswords, bool NotRecommendedSavingPasswords,
  bool & CreateShortcut, bool & EnableAutoSave);
class TShortCuts;
bool DoShortCutDialog(TShortCut & ShortCut,
  const TShortCuts & ShortCuts, const UnicodeString & HelpKeyword);
#if 0
bool DoCustomCommandOptionsDialog(
  const TCustomCommandType * Command, TStrings * CustomCommandOptions, TShortCut * ShortCut, uint32_t Flags,
  TCustomCommand * CustomCommandForOptions, const UnicodeString & Site, const TShortCuts * ShortCuts);
#endif // #if 0
void DoUsageStatisticsDialog();
void DoSiteRawDialog(TSessionData * Data);
bool DoSshHostCADialog(bool Add, TSshHostCA & SshHostCA);

// windows\UserInterface.cpp
bool DoMasterPasswordDialog();
bool DoChangeMasterPasswordDialog(UnicodeString & NewPassword);

// windows\WinMain.cpp
int32_t Execute();
void GetLoginData(const UnicodeString & SessionName, TOptions * Options,
  TObjectList * DataList, UnicodeString & DownloadFile, bool NeedSession, /*TForm * LinkedForm,*/ int32_t Flags = 0);
int32_t GetCommandLineParseUrlFlags(TProgramParams * Params);

#if 0
// forms\InputDlg.cpp
struct TInputDialogData
{
  TCustomEdit * Edit{nullptr};
};
typedef void (__closure *TInputDialogInitialize)
  (TObject * Sender, TInputDialogData * Data);
#endif // #if 0
bool InputDialog(const UnicodeString & ACaption,
  const UnicodeString & APrompt, UnicodeString & Value, const UnicodeString & HelpKeyword = HELP_NONE,
  TStrings * History = nullptr, bool PathInput = false,
  TInputDialogInitializeEvent && OnInitialize = nullptr, bool Echo = true, int32_t Width = 275);

// forms\About.cpp
struct TRegistration
{
  bool Registered{false};
  UnicodeString Subject;
  int32_t Licenses{0};
  UnicodeString ProductId;
  bool NeverExpires{false};
  TDateTime Expiration;
  bool EduLicense{false};
  TNotifyEvent OnRegistrationLink;
};
void DoAboutDialog(TConfiguration * Configuration,
  bool AllowLicense, TRegistration * Registration);
// void DoAboutDialog(TConfiguration * Configuration);

// forms\Cleanup.cpp
bool DoCleanupDialog();
void DoCleanupDialogIfAnyDataAndWanted();

// forms\Console.cpp
void DoConsoleDialog(TTerminal * Terminal,
    const UnicodeString & Command = "", const TStrings * Log = nullptr);

// forms\Copy.cpp
constexpr int32_t coTemp                = 0x001;
constexpr int32_t coDisableQueue        = 0x002;
constexpr int32_t coDisableDirectory    = 0x008; // not used anymore
constexpr int32_t coDoNotShowAgain      = 0x020;
constexpr int32_t coDisableSaveSettings = 0x040; // not used anymore
constexpr int32_t coDoNotUsePresets     = 0x080; // not used anymore
constexpr int32_t coAllowRemoteTransfer = 0x100;
constexpr int32_t coNoQueue             = 0x200;
constexpr int32_t coShortCutHint        = 0x800;
constexpr int32_t coAllFiles            = 0x1000;
constexpr int32_t coBrowse              = 0x2000;
constexpr int32_t cooDoNotShowAgain     = 0x01;
constexpr int32_t cooRemoteTransfer     = 0x02;
constexpr int32_t cooSaveSettings       = 0x04;
constexpr int32_t cooBrowse             = 0x08;

constexpr int32_t coTempTransfer        = 0x08;
constexpr int32_t coDisableNewerOnly    = 0x10;

bool DoCopyDialog(
  bool ToRemote, bool Move, TStrings * FileList, UnicodeString & TargetDirectory,
  TGUICopyParamType * Params, int Options, int CopyParamAttrs,
  TSessionData * SessionData, int * OutputOptions, int AutoSubmit);
bool CopyDialogValidateLocalDirectory(const UnicodeString & Directory/*, THistoryComboBox * DirectoryEdit*/);
bool CopyDialogValidateFileMask(
  const UnicodeString & FileMask, /*THistoryComboBox * DirectoryEdit, */bool MultipleFiles, bool RemotePaths);

// forms\CopyLocal.cpp
constexpr int32_t cloShortCutHint = 0x01;
constexpr int32_t cloMultipleFiles = 0x02;
constexpr int32_t clooDoNotShowAgain = 0x01;
bool DoCopyLocalDialog(bool Move, int Options, UnicodeString & TargetDirectory, UnicodeString & FileMask, int & OutputOptions);

// forms\CreateDirectory.cpp
bool DoCreateDirectoryDialog(UnicodeString & Directory,
  TRemoteProperties * Properties, int32_t AllowedChanges, bool & SaveSettings);

// forms\ImportSessions.cpp
bool DoImportSessionsDialog(TList * Imported);

// forms\License.cpp
enum TLicense { lcNoLicense = -1, lcWinScp, lcExpat };
void DoLicenseDialog(TLicense License);

bool DoLoginDialog(TList * DataList); //, TForm * LinkedForm);

  // forms\SiteAdvanced.cpp
bool DoSiteAdvancedDialog(TSessionData * SessionData);

// forms\OpenDirectory.cpp
enum TOpenDirectoryMode { odBrowse, odAddBookmark };
bool DoOpenDirectoryDialog(TOpenDirectoryMode Mode, TOperationSide Side,
  UnicodeString & Directory, TStrings * Directories, TTerminal * Terminal,
  bool AllowSwitch);

// forms\LocatinoProfiles.cpp
bool LocationProfilesDialog(TOpenDirectoryMode Mode,
  TOperationSide Side, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TStrings * LocalDirectories, TStrings * RemoteDirectories, TTerminal * Terminal);

// forms\Preferences.cpp
enum TPreferencesMode { pmDefault, pmEditor, pmCustomCommands,
    pmQueue, pmLogging, pmUpdates, pmPresets, pmEditors, pmCommander,
    pmEditorInternal, pmFileColors };
struct TCopyParamRuleData;
struct TPreferencesDialogData
{
  TCopyParamRuleData * CopyParamRuleData{nullptr};
};
bool DoPreferencesDialog(TPreferencesMode APreferencesMode,
  TPreferencesDialogData * DialogData = nullptr);

// forms\CustomCommand.cpp
class TCustomCommandList;
class TCustomCommandType;
class TShortCuts;
enum TCustomCommandsMode { ccmAdd, ccmEdit, ccmAdHoc };
constexpr int32_t ccoDisableRemote = 0x01;
constexpr int32_t ccoDisableRemoteFiles = 0x02;
#if 0
typedef void (__closure *TCustomCommandValidate)
  (const TCustomCommandType & Command);
#endif // #if 0
using TCustomCommandValidateEvent = nb::FastDelegate1<void,
  const TCustomCommandType & /*Command*/>;

bool DoCustomCommandDialog(TCustomCommandType & Command,
  const TCustomCommandList * CustomCommandList,
  TCustomCommandsMode Mode, int32_t Options, TCustomCommandValidateEvent && OnValidate,
  const TShortCuts * ShortCuts);

// forms\CopyParamPreset.cpp
class TCopyParamList;
enum TCopyParamPresetMode { cpmAdd, cpmAddCurrent, cpmEdit, cpmDuplicate };
bool DoCopyParamPresetDialog(TCopyParamList * CopyParamList,
  int & Index, TCopyParamPresetMode Mode, TCopyParamRuleData * CurrentRuleData,
  const TCopyParamType & DefaultCopyParams);

// forms\CopyParamCustom.cpp
bool DoCopyParamCustomDialog(TCopyParamType & CopyParam,
  int CopyParamAttrs);

// forms\Properties.cpp
class TRemoteProperties;
class TRemoteTokenList;
struct TCalculateSizeStats;
constexpr int32_t cpMode =  0x01;
constexpr int32_t cpOwner = 0x02;
constexpr int32_t cpGroup = 0x04;
constexpr int32_t cpAcl =   0x08;
using TCalculateSizeEvent = nb::FastDelegate4<void,
  TStrings * /*FileList*/, int64_t & /*Size*/, TCalculateSizeStats & /*Stats*/,
  bool & /*Close*/>;
using TCalculatedChecksumCallbackEvent = nb::FastDelegate3<void,
  const UnicodeString & /*FileName*/, const UnicodeString & /*Alg*/,
  const UnicodeString & /*Hash*/>;
using TCalculateChecksumEvent = nb::FastDelegate4<void,
  const UnicodeString & /*Alg*/, TStrings * /*FileList*/,
  TCalculatedChecksumCallbackEvent && /*OnCalculatedChecksum*/,
  bool & /*Close*/>;

bool DoPropertiesDialog(TStrings * FileList,
    const UnicodeString & Directory, const TRemoteTokenList * GroupList,
    const TRemoteTokenList * UserList, TStrings * ChecksumAlgs,
    TRemoteProperties * Properties,
    int32_t AllowedChanges, bool UserGroupByID, TCalculateSizeEvent && OnCalculateSize,
    TCalculateChecksumEvent && OnCalculateChecksum);

using TDirectoryExistsEvent = nb::FastDelegate4<bool,
  void * /*Session*/, const UnicodeString & /*Directory*/>;
bool DoRemoteMoveDialog(
  bool Multi, UnicodeString & Target, UnicodeString & FileMask, TDirectoryExistsEvent && OnDirectoryExists);
enum TDirectRemoteCopy { drcDisallow, drcAllow, drcConfirmCommandSession, drcConfirmCommandSessionDirs };
bool DoRemoteCopyDialog(
  TStrings * Sessions, TStrings * Directories,
  TDirectRemoteCopy AllowDirectCopy, bool Multi, void *& Session, UnicodeString & Target, UnicodeString & FileMask,
  bool & DirectCopy, void * CurrentSession, TDirectoryExistsEvent && OnDirectoryExists,
  bool TargetConfirmed);

#if 0
// forms\SelectMask.cpp
bool DoSelectMaskDialog(TControl * Parent, bool Select, TFileFilter & Filter);
bool DoFilterMaskDialog(TControl * Parent, UnicodeString & Mask);
bool DoFileColorDialog(TFileColorData & FileColorData);

// forms\EditMask.cpp
bool DoEditMaskDialog(TFileMasks & Mask);
#endif // #if 0

// forms\Synchronize.cpp
constexpr int soDoNotUsePresets =  0x01;
constexpr int soNoMinimize =       0x02;
constexpr int soAllowSelectedOnly = 0x04;
using TGetSynchronizeOptionsEvent = nb::FastDelegate2<void,
  int32_t /*Params*/,
  TSynchronizeOptions & /*Options*/>;
using TSynchronizeSessionLogEvent = nb::FastDelegate1<void,
  const UnicodeString & /*Message*/>;
using TFeedSynchronizeErrorEvent = nb::FastDelegate4<void,
  const UnicodeString & /*Message*/, TStrings * /*MoreMessages*/, TQueryType /*Type*/,
  const UnicodeString & /*HelpKeyword*/>;
using TSynchronizeInNewWindowEvent = nb::FastDelegate4<void,
  const TSynchronizeParamType & /*Params*/, const TCopyParamType * /*CopyParams*/>;

bool DoSynchronizeDialog(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, TSynchronizeStartStopEvent && OnStartStop,
  bool & SaveSettings, int32_t Options, int32_t CopyParamAttrs,
  TGetSynchronizeOptionsEvent && OnGetOptions,
  TSynchronizeSessionLogEvent && OnSynchronizeSessionLog,
  TFeedSynchronizeErrorEvent && OnFeedSynchronizeError,
  TNotifyEvent && OnSynchronizeAbort,
  TSynchronizeInNewWindowEvent && OnSynchronizeInNewWindow,
  int32_t AutoSubmit);

// forms\FullSynchronize.cpp
struct TUsableCopyParamAttrs;
// enum TSynchronizeMode { smRemote, smLocal, smBoth };
constexpr int32_t fsoDisableTimestamp = 0x01;
constexpr int32_t fsoDoNotUsePresets =  0x02;
constexpr int32_t fsoAllowSelectedOnly = 0x04;
constexpr int32_t fsoDisableByChecksum = 0x08;
using TFullSynchronizeInNewWindowEvent = nb::FastDelegate5<void,
  TTerminal::TSynchronizeMode /*Mode*/, int32_t /*Params*/, const UnicodeString & /*LocalDirectory*/, const UnicodeString & /*RemoteDirectory*/,
   const TCopyParamType * /*CopyParams*/>;
bool DoFullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode, int32_t & Params,
  UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode,
  int32_t Options, const TUsableCopyParamAttrs & CopyParamAttrs,
  TFullSynchronizeInNewWindowEvent && OnFullSynchronizeInNewWindow, int32_t AutoSubmit);

// forms\SynchronizeChecklist.cpp
class TSynchronizeChecklist;
using TCustomCommandMenuEvent = nb::FastDelegate3<void,
  void * /*Action*/, TStrings * /*LocalFileList*/,
  TStrings * /*RemoteFileList*/>;
using TFullSynchronizeEvent = nb::FastDelegate3<void,
  void * /*Token*/, TProcessedSynchronizationChecklistItem /*OnProcessedItem*/,
  TUpdatedSynchronizationChecklistItems /*OnUpdatedSynchronizationChecklistItems*/>;
using TSynchronizeChecklistCalculateSizeEvent = nb::FastDelegate3<void,
  TSynchronizeChecklist * /*Checklist*/, const TSynchronizeChecklist::TItemList & /*Items*/, void * /*Token*/>;
using TSynchronizeMoveEvent = nb::FastDelegate4<void,
  TOperationSide /*Side*/, const UnicodeString & /*FileName*/, const UnicodeString & /*NewFileName*/, TRemoteFile * /*RemoteFile*/>;
using TSynchronizeBrowseEvent = nb::FastDelegate3<void,
  TOperationSide /*Side*/, TChecklistAction /*Action*/, const TChecklistItem * /*Item*/>;
bool DoSynchronizeChecklistDialog(TSynchronizeChecklist * Checklist,
  TTerminal::TSynchronizeMode Mode, int32_t Params,
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
  TCustomCommandMenuEvent && OnCustomCommandMenu, TFullSynchronizeEvent && OnSynchronize,
  TSynchronizeChecklistCalculateSizeEvent && OnSynchronizeChecklistCalculateSize, TSynchronizeMoveEvent && OnSynchronizeMove,
  TSynchronizeBrowseEvent && OnSynchronizeBrowse, void * Token);

// forms\Editor.cpp
#if 0

typedef void (__closure *TFileClosedEvent)
  (TObject * Sender, bool Forced);
typedef void (__closure *TAnyModifiedEvent)
  (TObject * Sender, bool & Modified);
TForm * ShowEditorForm(const UnicodeString & FileName, TForm * ParentForm,
  TNotifyEvent OnFileChanged, TNotifyEvent OnFileReload, TFileClosedEvent OnClose,
  TNotifyEvent OnSaveAll, TAnyModifiedEvent OnAnyModified,
  const UnicodeString Caption, bool StandaloneEditor, TColor Color, int InternalEditorEncodingOverride,
  bool NewFile);
void ReconfigureEditorForm(TForm * Form);
void EditorFormFileUploadComplete(TForm * Form);
void EditorFormFileSave(TForm * Form);
bool IsEditorFormModified(TForm * Form);

#endif // #if 0

bool DoSymlinkDialog(UnicodeString & FileName, UnicodeString & PointTo,
  TOperationSide Side, bool & SymbolicLink, bool Edit, bool AllowSymbolic);

// forms\FileSystemInfo.cpp
struct TSpaceAvailable;
struct TFileSystemInfo;
struct TSessionInfo;
using TGetSpaceAvailableEvent = nb::FastDelegate3<void,
  const UnicodeString & /*Path*/, TSpaceAvailable & /*ASpaceAvailable*/, bool & /*Close*/>;

void DoFileSystemInfoDialog(
  const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  const UnicodeString & SpaceAvailablePath, TGetSpaceAvailableEvent && OnGetSpaceAvailable);

//moved to FarInterface.h

#if 0

// forms\MessageDlg.cpp
TForm * CreateMoreMessageDialog(const UnicodeString & Msg,
  TStrings * MoreMessages, TMsgDlgType DlgType, uint32_t Answers,
  const TQueryButtonAlias * Aliases, uint32_t AliasesCount,
  uint32_t TimeoutAnswer, TButton ** TimeoutButton,
  const UnicodeString & ImageName, const UnicodeString & NeverAskAgainCaption,
  const UnicodeString & MoreMessagesUrl, TSize MoreMessagesSize,
  const UnicodeString & CustomCaption);
TForm * CreateMoreMessageDialogEx(const UnicodeString & Message, TStrings * MoreMessages,
  TQueryType Type, uint32_t Answers, UnicodeString HelpKeyword, const TMessageParams * Params);
uint32_t ExecuteMessageDialog(TForm * Dialog, uint32_t Answers, const TMessageParams * Params);
void InsertPanelToMessageDialog(TCustomForm * Form, TPanel * Panel);
int GetMessageDialogContentWidth(TCustomForm * Form);
void NavigateMessageDialogToUrl(TCustomForm * Form, const UnicodeString & Url);
extern const UnicodeString MessagePanelName;
extern const UnicodeString MainMessageLabelName;
extern const UnicodeString MessageLabelName;
extern const UnicodeString YesButtonName;
extern const UnicodeString OKButtonName;

#endif // #if 0

// windows\Console.cpp
enum TConsoleMode
{
  cmNone, cmScripting, cmHelp, cmBatchSettings, cmKeyGen, cmFingerprintScan, cmDumpCallstack, cmInfo, cmComRegistration,
};
int Console(TConsoleMode Mode);

// forms\EditorPreferences.cpp
enum TEditorPreferencesMode { epmAdd, epmEdit, epmAdHoc };
class TEditorData;
bool DoEditorPreferencesDialog(TEditorData * Editor,
  bool & Remember, TEditorPreferencesMode Mode, bool MayRemote);

// forms\Find.cpp
using TFindEvent = nb::FastDelegate5<void,
  TTerminal * /*Terminal*/, const UnicodeString & /*Directory*/, const TFileMasks & /*FileMask*/,
  TFileFoundEvent /*OnFileFound*/, TFindingFileEvent /*OnFindingFile*/>;
using TFocusFileEvent = nb::FastDelegate2<void,
  TTerminal * /*Terminal*/, const UnicodeString & /*Path*/>;
using TFileOperationFinished2Event = nb::FastDelegate3<void,
  TOperationSide /*Side*/, const UnicodeString & /*FileName*/, bool /*Success*/>;
using TFileListOperationEvent = nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, TStrings * /*FileList*/, TFileOperationFinished2Event /*OnFileOperationFinished*/>;

#if 0
void ShowFileFindDialog(
  TTerminal * Terminal, UnicodeString Directory, TFindEvent OnFind, TFocusFileEvent OnFocusFile,
  TFileListOperationEvent OnDeleteFiles, TFileListOperationEvent OnDownloadFiles,
  TFileListOperationEvent OnEditFiles);
void HideFileFindDialog();

// forms\GenerateUrl.cpp
void DoGenerateUrlDialog(TSessionData * Data, TStrings * Paths);
enum TFilesSelected { fsList, fsAll };
void DoGenerateTransferCodeDialog(
  bool ToRemote, bool Move, int CopyParamAttrs, TSessionData * Data, TFilesSelected FilesSelected,
  TStrings * FileList, const UnicodeString & Path, const TCopyParamType & CopyParam);

void CopyParamListButton(TButton * Button);
const int cplNone =             0x00;
const int cplCustomizeDefault = 0x02;
const int cplSaveSettings =     0x04;
const int cplGenerateCode =     0x08;
void CopyParamListPopup(TRect R, TPopupMenu * Menu,
  const TCopyParamType & Param, UnicodeString Preset, TNotifyEvent OnClick,
  int Options, int CopyParamAttrs, bool SaveSettings = false);
int CopyParamListPopupClick(TObject * Sender,
  TCopyParamType & Param, UnicodeString & Preset, int CopyParamAttrs,
  bool * SaveSettings = nullptr);

void MenuPopup(TPopupMenu * Menu, TRect Rect, TComponent * PopupComponent);
void MenuPopup(TPopupMenu * Menu, TButton * Button);
void MenuPopup(TObject * Sender, const TPoint & MousePos, bool & Handled);
void MenuButton(TButton * Button);
TComponent * GetPopupComponent(TObject * Sender);
TRect CalculatePopupRect(TButton * Button);
TRect CalculatePopupRect(TControl * Control, TPoint MousePos);

typedef void (__closure *TColorChangeEvent)
  (TColor Color);
TPopupMenu * CreateSessionColorPopupMenu(TColor Color,
  TColorChangeEvent OnColorChange);
void CreateSessionColorMenu(TComponent * AOwner, TColor Color,
  TColorChangeEvent OnColorChange);
void CreateEditorBackgroundColorMenu(TComponent * AOwner, TColor Color,
  TColorChangeEvent OnColorChange);
TPopupMenu * CreateColorPopupMenu(TColor Color,
  TColorChangeEvent OnColorChange);
TColor RestoreColor(const UnicodeString & CStr);
UnicodeString StoreColor(TColor Color);

void FixButtonImage(TButton * Button);
void CenterButtonImage(TButton * Button);

void UpgradeSpeedButton(TSpeedButton * Button);

int AdjustLocaleFlag(const UnicodeString & S, TLocaleFlagOverride LocaleFlagOverride, bool Recommended, int On, int Off);

void SetGlobalMinimizeHandler(TCustomForm * Form, TNotifyEvent OnMinimize);
void ClearGlobalMinimizeHandler(TNotifyEvent OnMinimize);
void CallGlobalMinimizeHandler(TObject * Sender);
bool IsApplicationMinimized();
void ApplicationMinimize();
void ApplicationRestore();
bool HandleMinimizeSysCommand(TMessage & Message);

void WinInitialize();
void WinFinalize();
#endif // #if 0

void ShowNotification(TTerminal * Terminal, const UnicodeString & Str,
  TQueryType Type);
#if 0
void InitializeShortCutCombo(TComboBox * ComboBox,
  const TShortCuts & ShortCuts);
void SetShortCutCombo(TComboBox * ComboBox, TShortCut Value);
TShortCut GetShortCutCombo(TComboBox * ComboBox);
bool IsCustomShortCut(TShortCut ShortCut);
TShortCut NormalizeCustomShortCut(TShortCut ShortCut);
#endif // #if 0

UnicodeString DumpCallstackEventName(int32_t ProcessId);
UnicodeString DumpCallstackFileName(int32_t ProcessId);

void CheckConfigurationForceSave();
void InterfaceStarted();
void InterfaceStartDontMeasure();
void AddStartupSequence(const UnicodeString & Tag);

// #define HIDDEN_WINDOW_NAME L"WinSCPHiddenWindow3"

struct TCopyDataMessage
{
  enum { CommandCanCommandLine, CommandCommandLine, MainWindowCheck, RefreshPanel };
  static constexpr const uint32_t Version1 = 1;

  uint32_t Version{TCopyDataMessage::Version1};
  uint32_t Command{static_cast<uint32_t>(-1)};

  union
  {
    wchar_t CommandLine[10240]{};

    struct
    {
      wchar_t Session[1024]{};
      wchar_t Path[1024]{};
    } Refresh;
  };

  TCopyDataMessage() noexcept
  {
    Version = TCopyDataMessage::Version1;
    // Command = static_cast<uint32_t>(-1);
  }
};

class TWinInteractiveCustomCommand final : public TInteractiveCustomCommand
{
  TWinInteractiveCustomCommand() = delete;
public:
  explicit TWinInteractiveCustomCommand(
    TCustomCommand * ChildCustomCommand, const UnicodeString & CustomCommandName, const UnicodeString & AHelpKeyword) noexcept;

protected:
  virtual void Prompt(int32_t Index, const UnicodeString & Prompt,
    UnicodeString & Value) const override;
  virtual void Execute(const UnicodeString & Command,
    UnicodeString & Value) const override;
  virtual void PatternHint(int32_t Index, const UnicodeString & Pattern) override;

private:
  UnicodeString FCustomCommandName;
  nb::map_t<int32_t, size_t> FIndexes;
  TUnicodeStringVector FPrompts;
  TUnicodeStringVector FDefaults;
  TUnicodeStringVector FValues;
  UnicodeString FHelpKeyword;
};

#if 0

class TTrayIcon
{
public:
  TTrayIcon(uint32_t Id);
  ~TTrayIcon();

  void PopupBalloon(const UnicodeString & Title, const UnicodeString & Str,
    TQueryType QueryType, uint32_t Timeout, TNotifyEvent OnBalloonClick,
    TObject * BalloonUserData);
  void CancelBalloon();

  __property bool Visible = { read = FVisible, write = SetVisible };
  __property TNotifyEvent OnClick = { read = FOnClick, write = FOnClick };
  __property UnicodeString Hint = { read = GetHint, write = SetHint };

protected:
  void Update();
  bool Notify(uint32_t Message);

private:
  bool FVisible;
  NOTIFYICONDATA * FTrayIcon;
  TNotifyEvent FOnClick;
  TNotifyEvent FOnBalloonClick;
  TObject * FBalloonUserData;
  UINT FTaskbarCreatedMsg;

  void WndProc(TMessage & Message);
  void SetVisible(bool value);
  UnicodeString GetHint();
  void SetHint(const UnicodeString & value);
  void BalloonCancelled();
};
#endif // #if 0

enum TConsoleFlag
{
  cfLimitedOutput,
  cfLiveOutput,
  cfNoInteractiveInput,
  cfInteractive,
  cfCommandLineOnly,
  cfWantsProgress,
  cfStdOut,
  cfStdIn
};

class TConsole
{
public:
  virtual ~TConsole() = default;
  virtual void Print(const UnicodeString & Str, bool FromBeginning = false, bool Error = false) = 0;
  void PrintLine(const UnicodeString & Str = UnicodeString(), bool Error = false);
  virtual bool Input(UnicodeString & Str, bool Echo, uint32_t Timer) = 0;
  virtual int Choice(
    const UnicodeString & Options, int32_t Cancel, int32_t Break, int32_t Continue, int32_t Timeouted, bool Timeouting, uint32_t Timer,
    const UnicodeString & Message) = 0;
  virtual bool HasFlag(TConsoleFlag Flag) const = 0;
  virtual bool PendingAbort() = 0;
  virtual void SetTitle(const UnicodeString & Title) = 0;
  virtual void WaitBeforeExit() = 0;
  // virtual void Progress(TScriptProgress & Progress) = 0;
  virtual void TransferOut(const uint8_t * Data, size_t Len) = 0;
  virtual size_t TransferIn(uint8_t * Data, size_t Len) = 0;
  virtual UnicodeString FinalLogMessage() = 0;
};

int32_t HandleException(TConsole * Console, Exception & E);

enum { RESULT_SUCCESS = 0, RESULT_ANY_ERROR = 1 };

