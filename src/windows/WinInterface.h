//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include <Classes.hpp>
__removed #include <Buttons.hpp>
#include <Interface.h>
#include <WinConfiguration.h>
#include <Terminal.h>
#include <SynchronizeController.h>
#include <Script.h>
#if defined(FARPLUGIN)
#include <GUIConfiguration.h>
#include <SynchronizeController.h>
#endif // FARPLUGIN

#ifdef LOCALINTERFACE
#include <LocalInterface.h>
#endif

#define SITE_ICON 1
#define SITE_FOLDER_ICON 2
#define WORKSPACE_ICON 3

class TStoredSessionList;
class TConfiguration;
class TTerminal;

constexpr const uintptr_t mpNeverAskAgainCheck =   0x01;
constexpr const uintptr_t mpAllowContinueOnError = 0x02;

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
#define LOG_SWITCH L"Log"
#define LOGSIZE_SWITCH L"LogSize"
#define LOGSIZE_SEPARATOR L"*"
#define INI_SWITCH L"Ini"
#define RAW_CONFIG_SWITCH L"RawConfig"
#define FINGERPRINTSCAN_SWITCH L"FingerprintScan"
#define DUMPCALLSTACK_SWITCH L"DumpCallstack"
#define INFO_SWITCH L"Info"
#define COMREGISTRATION_SWITCH L"ComRegistration"

#define DUMPCALLSTACK_EVENT L"WinSCPCallstack%d"

#define DUMPCALLSTACK_EVENT L"WinSCPCallstack%d"

struct NB_CORE_EXPORT TMessageParams : public TObject
{
  NB_DISABLE_COPY(TMessageParams)
public:
  explicit TMessageParams(uintptr_t AParams) noexcept;
  void Assign(const TMessageParams *AParams);

  const TQueryButtonAlias *Aliases{nullptr};
  uintptr_t AliasesCount{0};
  uintptr_t Flags{0};
  uintptr_t Params{0};
  uintptr_t Timer{0};
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uint32_t TimerAnswers{0};
  TQueryType TimerQueryType{static_cast<TQueryType>(-1)};
  uintptr_t Timeout{0};
  uintptr_t TimeoutAnswer{0};
  UnicodeString NeverAskAgainTitle;
  uintptr_t NeverAskAgainAnswer{0};
  bool NeverAskAgainCheckedInitially{false};
  bool AllowHelp{false};
  UnicodeString ImageName;
  UnicodeString MoreMessagesUrl;
  intptr_t MoreMessagesSize{0};
  UnicodeString CustomCaption;

private:
  void Reset();
};

class TCustomScpExplorerForm;
TCustomScpExplorerForm *CreateScpExplorer();

void ConfigureInterface();

void DoProductLicense();

extern const UnicodeString AppName;

void SetOnForeground(bool OnForeground);
void FlashOnBackground();

void TerminateApplication();
void ShowExtendedExceptionEx(TTerminal * Terminal, Exception * E);
__removed void FormHelp(TCustomForm * Form);
void SearchHelp(const UnicodeString & Message);
void MessageWithNoHelp(UnicodeString Message);

class TProgramParams;
bool CheckSafe(TProgramParams * Params);
void CheckLogParam(TProgramParams * Params);
bool CheckXmlLogParam(TProgramParams * Params);

#if 0
UnicodeString GetToolbarKey(const UnicodeString & ToolbarName);
UnicodeString GetToolbarsLayoutStr(TControl * OwnerControl);
void LoadToolbarsLayoutStr(TControl * OwnerControl, UnicodeString LayoutStr);

namespace Tb2item { class TTBCustomItem; }
namespace Tbx { class TTBXSeparatorItem; }
Tbx::TTBXSeparatorItem * AddMenuSeparator(Tb2item::TTBCustomItem * Menu);
void AddMenuLabel(Tb2item::TTBCustomItem * Menu, const UnicodeString & Label);
void ClickToolbarItem(Tb2item::TTBCustomItem * Item, bool PositionCursor);
#endif // #if 0

// windows\WinHelp.cpp
void InitializeWinHelp();
void FinalizeWinHelp();

// windows\WinInterface.cpp
uint32_t MessageDialog(UnicodeString Msg, TQueryType Type,
  uint32_t Answers, UnicodeString HelpKeyword = HELP_NONE, const TMessageParams *Params = nullptr);
uint32_t MessageDialog(intptr_t Ident, TQueryType Type,
  uint32_t Answers, UnicodeString HelpKeyword = HELP_NONE, const TMessageParams *Params = nullptr);
uint32_t SimpleErrorDialog(UnicodeString Msg, UnicodeString MoreMessages = "");

uint32_t MoreMessageDialog(UnicodeString Message,
  TStrings *MoreMessages, TQueryType Type, uint32_t Answers,
  UnicodeString HelpKeyword, const TMessageParams *Params = nullptr);

uint32_t ExceptionMessageDialog(Exception *E, TQueryType Type,
  UnicodeString MessageFormat = "", uint32_t Answers = qaOK,
  UnicodeString HelpKeyword = HELP_NONE, const TMessageParams *Params = nullptr);
uint32_t FatalExceptionMessageDialog(Exception *E, TQueryType Type,
  intptr_t SessionReopenTimeout, UnicodeString MessageFormat = "", uint32_t Answers = qaOK,
  UnicodeString HelpKeyword = HELP_NONE, const TMessageParams *Params = nullptr);

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
  const TShortCuts & ShortCuts, UnicodeString HelpKeyword);
#if 0
bool DoCustomCommandOptionsDialog(
  const TCustomCommandType * Command, TStrings * CustomCommandOptions, TShortCut * ShortCut, unsigned int Flags,
  TCustomCommand * CustomCommandForOptions, const UnicodeString & Site, const TShortCuts * ShortCuts);
#endif // #if 0
void DoUsageStatisticsDialog();

// windows\UserInterface.cpp
bool DoMasterPasswordDialog();
bool DoChangeMasterPasswordDialog(UnicodeString & NewPassword);

// windows\WinMain.cpp
int Execute();
void GetLoginData(UnicodeString SessionName, TOptions * Options,
  TObjectList * DataList, UnicodeString & DownloadFile, bool NeedSession, // TForm * LinkedForm,
  int Flags = 0);

#if 0
// forms\InputDlg.cpp
struct TInputDialogData
{
  TCustomEdit * Edit{nullptr};
};
typedef void (__closure *TInputDialogInitialize)
  (TObject * Sender, TInputDialogData * Data);
#endif // #if 0
bool InputDialog(UnicodeString ACaption,
  UnicodeString APrompt, UnicodeString & Value, UnicodeString HelpKeyword = HELP_NONE,
  TStrings * History = nullptr, bool PathInput = false,
  TInputDialogInitializeEvent OnInitialize = nullptr, bool Echo = true);

// forms\About.cpp
struct TRegistration
{
  bool Registered{false};
  UnicodeString Subject;
  int Licenses{0};
  UnicodeString ProductId;
  bool NeverExpires{false};
  TDateTime Expiration;
  bool EduLicense{false};
  TNotifyEvent OnRegistrationLink;
};
void DoAboutDialog(TConfiguration * Configuration,
  bool AllowLicense, TRegistration * Registration);
void DoAboutDialog(TConfiguration *Configuration);

// forms\Cleanup.cpp
bool DoCleanupDialog(TStoredSessionList *SessionList,
    TConfiguration *Configuration);

// forms\Console.cpp
void DoConsoleDialog(TTerminal * Terminal,
    const UnicodeString Command = "", const TStrings * Log = nullptr);

// forms\Copy.cpp
constexpr intptr_t coTemp                = 0x001;
constexpr intptr_t coDisableQueue        = 0x002;
constexpr intptr_t coDisableDirectory    = 0x008; // not used anymore
constexpr intptr_t coDoNotShowAgain      = 0x020;
constexpr intptr_t coDisableSaveSettings = 0x040; // not used anymore
constexpr intptr_t coDoNotUsePresets     = 0x080; // not used anymore
constexpr intptr_t coAllowRemoteTransfer = 0x100;
constexpr intptr_t coNoQueue             = 0x200;
constexpr intptr_t coShortCutHint        = 0x800;
constexpr intptr_t coAllFiles            = 0x1000;
constexpr intptr_t cooDoNotShowAgain     = 0x01;
constexpr intptr_t cooRemoteTransfer     = 0x02;
constexpr intptr_t cooSaveSettings       = 0x04;

constexpr const int coTempTransfer        = 0x08;
constexpr const int coDisableNewerOnly    = 0x10;

bool DoCopyDialog(bool ToRemote,
  bool Move, TStrings * FileList, UnicodeString & TargetDirectory,
  TGUICopyParamType * Params, int Options, int CopyParamAttrs,
  TSessionData * SessionData, int * OutputOptions);

// forms\CreateDirectory.cpp
bool DoCreateDirectoryDialog(UnicodeString & Directory,
  TRemoteProperties * Properties, int AllowedChanges, bool & SaveSettings);

// forms\ImportSessions.cpp
bool DoImportSessionsDialog(TList * Imported);

// forms\License.cpp
enum TLicense { lcNoLicense = -1, lcWinScp, lcExpat };
void DoLicenseDialog(TLicense License);

bool DoLoginDialog(TStoredSessionList * SessionList, TList * DataList); // , TForm * LinkedForm);

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
constexpr const int ccoDisableRemote = 0x01;
constexpr const int ccoDisableRemoteFiles = 0x02;
#if 0
typedef void (__closure *TCustomCommandValidate)
  (const TCustomCommandType & Command);
#endif // #if 0
using TCustomCommandValidateEvent = nb::FastDelegate1<void,
  const TCustomCommandType & /*Command*/>;

bool DoCustomCommandDialog(TCustomCommandType & Command,
  const TCustomCommandList * CustomCommandList,
  TCustomCommandsMode Mode, int Options, TCustomCommandValidateEvent OnValidate,
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
constexpr const int cpMode =  0x01;
constexpr const int cpOwner = 0x02;
constexpr const int cpGroup = 0x04;
#if 0
typedef void (__closure *TCalculateSizeEvent)
  (TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats,
   bool & Close);
typedef void (__closure *TCalculatedChecksumCallbackEvent)(
  const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash);
typedef void (__closure *TCalculateChecksumEvent)
  (const UnicodeString & Alg, TStrings * FileList,
   TCalculatedChecksumCallbackEvent OnCalculatedChecksum, bool & Close);
#endif // #if 0
using TCalculateSizeEvent = nb::FastDelegate4<void,
  TStrings * /*FileList*/, int64_t & /*Size*/, TCalculateSizeStats & /*Stats*/,
  bool & /*Close*/>;
using TCalculatedChecksumCallbackEvent = nb::FastDelegate3<void,
  UnicodeString /*FileName*/, UnicodeString /*Alg*/,
  UnicodeString /*Hash*/>;
using TCalculateChecksumEvent = nb::FastDelegate4<void,
  UnicodeString /*Alg*/, TStrings * /*FileList*/,
  TCalculatedChecksumCallbackEvent /*OnCalculatedChecksum*/,
  bool & /*Close*/>;

bool DoPropertiesDialog(TStrings * FileList,
    UnicodeString Directory, const TRemoteTokenList * GroupList,
    const TRemoteTokenList * UserList, TStrings * ChecksumAlgs,
    TRemoteProperties * Properties,
    int AllowedChanges, bool UserGroupByID, TCalculateSizeEvent OnCalculateSize,
    TCalculateChecksumEvent OnCalculateChecksum);

bool DoRemoteMoveDialog(bool Multi, UnicodeString & Target, UnicodeString & FileMask);
enum TDirectRemoteCopy { drcDisallow, drcAllow, drcConfirmCommandSession };
bool DoRemoteCopyDialog(TStrings * Sessions, TStrings * Directories,
  TDirectRemoteCopy AllowDirectCopy, bool Multi, void *& Session,
  UnicodeString & Target, UnicodeString & FileMask, bool & DirectCopy, void * CurrentSession);

#if 0
// forms\SelectMask.cpp
bool DoSelectMaskDialog(TControl * Parent, bool Select, TFileFilter & Filter);
bool DoFilterMaskDialog(TControl * Parent, UnicodeString & Mask);
bool DoFileColorDialog(TFileColorData & FileColorData);

// forms\EditMask.cpp
bool DoEditMaskDialog(TFileMasks & Mask);
#endif // #if 0

// forms\Synchronize.cpp
constexpr const int soDoNotUsePresets =  0x01;
constexpr const int soNoMinimize =       0x02;
constexpr const int soAllowSelectedOnly = 0x04;
#if 0
typedef void (__closure *TGetSynchronizeOptionsEvent)
  (int Params, TSynchronizeOptions & Options);
#endif // #if 0
using TGetSynchronizeOptionsEvent = nb::FastDelegate2<void,
  intptr_t /*Params*/,
  TSynchronizeOptions & /*Options*/>;
#if 0
typedef void (__closure *TSynchronizeSessionLog)
  (const UnicodeString & Message);
typedef void (__closure *TFeedSynchronizeError)
  (const UnicodeString & Message, TStrings * MoreMessages, TQueryType Type,
   const UnicodeString & HelpKeyword);
typedef void (__closure *TSynchronizeInNewWindow)
  (const TSynchronizeParamType & Params, const TCopyParamType * CopyParams);
#endif // #if 0
using TSynchronizeSessionLogEvent = nb::FastDelegate1<void,
  UnicodeString /*Message*/>;
using TFeedSynchronizeErrorEvent = nb::FastDelegate4<void,
  UnicodeString /*Message*/, TStrings * /*MoreMessages*/, TQueryType /*Type*/,
  UnicodeString /*HelpKeyword*/>;
using TSynchronizeInNewWindowEvent = nb::FastDelegate4<void,
  const TSynchronizeParamType & /*Params*/, const TCopyParamType * /*CopyParams*/>;

bool DoSynchronizeDialog(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
  bool & SaveSettings, intptr_t Options, intptr_t CopyParamAttrs,
  TGetSynchronizeOptionsEvent OnGetOptions,
  TSynchronizeSessionLogEvent OnSynchronizeSessionLog,
  TFeedSynchronizeErrorEvent & OnFeedSynchronizeError,
  TSynchronizeInNewWindowEvent OnSynchronizeInNewWindow,
  bool Start);

// forms\FullSynchronize.cpp
struct TUsableCopyParamAttrs;
__removed enum TSynchronizeMode { smRemote, smLocal, smBoth };
constexpr const intptr_t fsoDisableTimestamp = 0x01;
constexpr const intptr_t fsoDoNotUsePresets =  0x02;
constexpr const intptr_t fsoAllowSelectedOnly = 0x04;
#if 0
typedef void (__closure *TFullSynchronizeInNewWindow)
  (TSynchronizeMode Mode, int Params, const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
   const TCopyParamType * CopyParams);
#endif // #if 0
using TFullSynchronizeInNewWindowEvent = nb::FastDelegate5<void,
  TTerminal::TSynchronizeMode /*Mode*/, intptr_t /*Params*/, UnicodeString /*LocalDirectory*/, UnicodeString /*RemoteDirectory*/,
   const TCopyParamType * /*CopyParams*/>;

bool DoFullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode, intptr_t & Params,
  UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode,
  intptr_t Options, const TUsableCopyParamAttrs & CopyParamAttrs,
  TFullSynchronizeInNewWindowEvent OnFullSynchronizeInNewWindow);

// forms\SynchronizeChecklist.cpp
class TSynchronizeChecklist;
#if 0
typedef void (__closure *TCustomCommandMenuEvent)
  (TAction * Action, TStrings * LocalFileList, TStrings * RemoteFileList);
typedef void (__closure *TFullSynchronizeEvent)(
  void * Token, TProcessedSynchronizationChecklistItem OnProcessedItem,
  TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems);
typedef void (__closure *TSynchronizeChecklistCalculateSize)
  (TSynchronizeChecklist * Checklist, const TSynchronizeChecklist::TItemList & Items, void * Token);
typedef void (__closure *TSynchronizeMoveEvent)(
  TOperationSide Side, const UnicodeString & FileName, const UnicodeString & NewFileName, TRemoteFile * RemoteFile);
#endif // #if 0
using TCustomCommandMenuEvent = nb::FastDelegate3<void,
  void * /*Action*/, TStrings * /*LocalFileList*/,
  TStrings * /*RemoteFileList*/>;
using TFullSynchronizeEvent = nb::FastDelegate3<void,
  void * /*Token*/, TProcessedSynchronizationChecklistItem /*OnProcessedItem*/,
  TUpdatedSynchronizationChecklistItems /*OnUpdatedSynchronizationChecklistItems*/>;
using TSynchronizeChecklistCalculateSizeEvent = nb::FastDelegate3<void,
  TSynchronizeChecklist * /*Checklist*/, const TSynchronizeChecklist::TItemList & /*Items*/, void * /*Token*/>;
using TSynchronizeMoveEvent = nb::FastDelegate4<void,
  TOperationSide /*Side*/, UnicodeString /*FileName*/, UnicodeString /*NewFileName*/, TRemoteFile * /*RemoteFile*/>;

bool DoSynchronizeChecklistDialog(TSynchronizeChecklist * Checklist,
  TTerminal::TSynchronizeMode Mode, int Params,
  const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
  TCustomCommandMenuEvent OnCustomCommandMenu, TFullSynchronizeEvent OnSynchronize,
  TSynchronizeChecklistCalculateSizeEvent OnSynchronizeChecklistCalculateSize, TSynchronizeMoveEvent OnSynchronizeMove,
  void * Token);

// forms\Editor.cpp
#if 0
typedef void (__closure *TFileClosedEvent)
  (TObject * Sender, bool Forced);
typedef void (__closure *TAnyModifiedEvent)
  (TObject * Sender, bool & Modified);
typedef nb::FastDelegate2<void,
  TObject * /*Sender* /, bool /*Forced*/> TFileClosedEvent;
typedef nb::FastDelegate2<void,
  TObject * /*Sender* /, bool & /*Modified*/> TAnyModifiedEvent;
TForm * ShowEditorForm(const UnicodeString FileName, TForm * ParentForm,
  TNotifyEvent OnFileChanged, TNotifyEvent OnFileReload, TFileClosedEvent OnClose,
  TNotifyEvent OnSaveAll, TAnyModifiedEvent OnAnyModified,
  const UnicodeString Caption, bool StandaloneEditor, TColor Color, int InternalEditorEncodingOverride);
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
#if 0
typedef void (__closure *TGetSpaceAvailable)
  (const UnicodeString Path, TSpaceAvailable & ASpaceAvailable, bool & Close);
#endif // #if 0
using TGetSpaceAvailableEvent = nb::FastDelegate3<void,
  UnicodeString /*Path*/, TSpaceAvailable & /*ASpaceAvailable*/,
  bool & /*Close*/>;

void DoFileSystemInfoDialog(
  const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  UnicodeString SpaceAvailablePath, TGetSpaceAvailableEvent OnGetSpaceAvailable);

//moved to FarInterface.h
#if 0
// forms\MessageDlg.cpp
void AnswerNameAndCaption(
  unsigned int Answer, UnicodeString & Name, UnicodeString & Caption);
TForm * CreateMoreMessageDialog(const UnicodeString & Msg,
  TStrings * MoreMessages, TMsgDlgType DlgType, unsigned int Answers,
  const TQueryButtonAlias * Aliases, unsigned int AliasesCount,
  unsigned int TimeoutAnswer, TButton ** TimeoutButton,
  const UnicodeString & ImageName, const UnicodeString & NeverAskAgainCaption,
  const UnicodeString & MoreMessagesUrl, TSize MoreMessagesSize,
  const UnicodeString & CustomCaption);
TForm * CreateMoreMessageDialogEx(const UnicodeString Message, TStrings * MoreMessages,
  TQueryType Type, unsigned int Answers, UnicodeString HelpKeyword, const TMessageParams * Params);
unsigned int ExecuteMessageDialog(TForm * Dialog, unsigned int Answers, const TMessageParams * Params);
void InsertPanelToMessageDialog(TCustomForm * Form, TPanel * Panel);
void NavigateMessageDialogToUrl(TCustomForm * Form, const UnicodeString & Url);
extern const UnicodeString MessagePanelName;
extern const UnicodeString MainMessageLabelName;
extern const UnicodeString MessageLabelName;
extern const UnicodeString YesButtonName;
extern const UnicodeString OKButtonName;
#endif // #if 0

// windows\Console.cpp
enum TConsoleMode { cmNone, cmScripting, cmHelp, cmBatchSettings, cmKeyGen, cmFingerprintScan };
int Console(TConsoleMode Mode);

// forms\EditorPreferences.cpp
enum TEditorPreferencesMode { epmAdd, epmEdit, epmAdHoc };
class TEditorData;
bool DoEditorPreferencesDialog(TEditorData * Editor,
  bool & Remember, TEditorPreferencesMode Mode, bool MayRemote);

// forms\Find.cpp
#if 0
typedef void (__closure *TFindEvent)
  (TTerminal * Terminal, UnicodeString Directory, const TFileMasks & FileMask,
   TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
#endif // #if 0
using TFindEvent = nb::FastDelegate4<void,
  UnicodeString /*Directory*/, const TFileMasks & /*FileMask*/,
  TFileFoundEvent /*OnFileFound*/,
  TFindingFileEvent /*OnFindingFile*/>;
#if 0
typedef void (__closure *TFocusFileEvent)
  (TTerminal * Terminal, const UnicodeString & Path);
#endif // #if 0
using TFocusFileEvent = nb::FastDelegate2<void,
  TTerminal * /*Terminal*/, UnicodeString /*Path*/>;
#if 0
typedef void (__closure *TFileOperationFinishedEvent)
  (const UnicodeString & FileName, bool Success);
#endif // #if 0
using TFileOperationFinished2Event = nb::FastDelegate2<void,
  UnicodeString /*FileName*/, bool /*Success*/>;
#if 0
typedef void (__closure *TFileListOperationEvent)
  (TTerminal * Terminal, TStrings * FileList, TFileOperationFinishedEvent OnFileOperationFinished);
#endif // #if 0
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
  TStrings * FileList, UnicodeString Path, const TCopyParamType & CopyParam);

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
  bool * SaveSettings = NULL);

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

void ShowNotification(TTerminal *Terminal, UnicodeString Str,
  TQueryType Type);
#if 0
void InitializeShortCutCombo(TComboBox * ComboBox,
  const TShortCuts & ShortCuts);
void SetShortCutCombo(TComboBox * ComboBox, TShortCut Value);
TShortCut GetShortCutCombo(TComboBox * ComboBox);
bool IsCustomShortCut(TShortCut ShortCut);
TShortCut NormalizeCustomShortCut(TShortCut ShortCut);
#endif // #if 0

UnicodeString DumpCallstackEventName(int ProcessId);
UnicodeString DumpCallstackFileName(int ProcessId);

void CheckConfigurationForceSave();
//---------------------------------------------------------------------------
#define HIDDEN_WINDOW_NAME L"WinSCPHiddenWindow3"
//---------------------------------------------------------------------------
struct TCopyDataMessage
{
  enum { CommandCanCommandLine, CommandCommandLine, MainWindowCheck, RefreshPanel };
  static const uintptr_t Version1 = 1;

  uintptr_t Version{TCopyDataMessage::Version1};
  uintptr_t Command{static_cast<uintptr_t>(-1)};

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
    Command = static_cast<unsigned int>(-1);
  }
};
//---------------------------------------------------------------------------
class TWinInteractiveCustomCommand : public TInteractiveCustomCommand
{
public:
  TWinInteractiveCustomCommand(
    TCustomCommand * ChildCustomCommand, const UnicodeString CustomCommandName, const UnicodeString HelpKeyword) noexcept;

protected:
  void Prompt(intptr_t Index, const UnicodeString Prompt,
    UnicodeString &Value) const override;
  void Execute(const UnicodeString Command,
    UnicodeString &Value) const override;
  void PatternHint(intptr_t Index, UnicodeString Pattern) override;

private:
  UnicodeString FCustomCommandName;
  rde::map<int, size_t> FIndexes;
  TUnicodeStringVector FPrompts;
  TUnicodeStringVector FDefaults;
  TUnicodeStringVector FValues;
  UnicodeString FHelpKeyword;
};
#if 0
//---------------------------------------------------------------------------
class TTrayIcon
{
public:
  TTrayIcon(unsigned int Id);
  ~TTrayIcon();

  void PopupBalloon(UnicodeString Title, const UnicodeString & Str,
    TQueryType QueryType, unsigned int Timeout, TNotifyEvent OnBalloonClick,
    TObject * BalloonUserData);
  void CancelBalloon();

  __property bool Visible = { read = FVisible, write = SetVisible };
  __property TNotifyEvent OnClick = { read = FOnClick, write = FOnClick };
  __property UnicodeString Hint = { read = GetHint, write = SetHint };

protected:
  void Update();
  bool Notify(unsigned int Message);

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
  void SetHint(UnicodeString value);
  void BalloonCancelled();
};
#endif // #if 0
//---------------------------------------------------------------------------
class TConsole
{
public:
  virtual ~TConsole() noexcept = default;
  virtual void Print(UnicodeString Str, bool FromBeginning = false, bool Error = false) = 0;
  void PrintLine(const UnicodeString & Str = UnicodeString(), bool Error = false);
  virtual bool Input(UnicodeString & Str, bool Echo, unsigned int Timer) = 0;
  virtual int Choice(
    UnicodeString Options, int Cancel, int Break, int Continue, int Timeouted, bool Timeouting, unsigned int Timer,
    UnicodeString Message) = 0;
  virtual bool PendingAbort() = 0;
  virtual void SetTitle(UnicodeString Title) = 0;
  virtual bool LimitedOutput() = 0;
  virtual bool LiveOutput() = 0;
  virtual bool NoInteractiveInput() = 0;
  virtual void WaitBeforeExit() = 0;
  virtual bool CommandLineOnly() = 0;
  virtual bool WantsProgress() = 0;
//  virtual void Progress(TScriptProgress & Progress) = 0;
  virtual UnicodeString FinalLogMessage() = 0;
};
//---------------------------------------------------------------------------
int HandleException(TConsole * Console, Exception & E);
//---------------------------------------------------------------------------
enum { RESULT_SUCCESS = 0, RESULT_ANY_ERROR = 1 };
//---------------------------------------------------------------------------
