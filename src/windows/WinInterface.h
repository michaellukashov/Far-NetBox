
#pragma once

#include <Classes.hpp>
#include <Common.h>
#include <Interface.h>
#include <MsgIDs.h>
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

const int mpNeverAskAgainCheck   = 0x01;
const int mpAllowContinueOnError = 0x02;

#define UPLOAD_IF_ANY_SWITCH L"UploadIfAny"
#define UPLOAD_SWITCH L"Upload"
#define JUMPLIST_SWITCH L"JumpList"
#define DESKTOP_SWITCH L"Desktop"
#define SEND_TO_HOOK_SWITCH L"SendToHook"
#define UNSAFE_SWITCH L"Unsafe"
#define NEWINSTANCE_SWICH L"NewInstance"
#define KEYGEN_SWITCH L"KeyGen"
#define KEYGEN_OUTPUT_SWITCH L"Output"
#define KEYGEN_COMMENT_SWITCH L"Comment"
#define KEYGEN_CHANGE_PASSPHRASE_SWITCH L"ChangePassphrase"
#define LOG_SWITCH L"Log"
#define LOGSIZE_SWITCH L"LogSize"
#define LOGSIZE_SEPARATOR L"*"
#define INI_SWITCH L"Ini"
#define FINGERPRINTSCAN_SWITCH L"FingerprintScan"

struct NB_CORE_EXPORT TMessageParams : public TObject
{
  NB_DISABLE_COPY(TMessageParams)
public:
  explicit TMessageParams(uintptr_t AParams);
  void Assign(const TMessageParams *AParams);

  const TQueryButtonAlias *Aliases;
  uintptr_t AliasesCount;
  uintptr_t Flags;
  uintptr_t Params;
  uintptr_t Timer;
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uintptr_t TimerAnswers;
  TQueryType TimerQueryType;
  uintptr_t Timeout;
  uintptr_t TimeoutAnswer;
  UnicodeString NeverAskAgainTitle;
  uintptr_t NeverAskAgainAnswer;
  bool NeverAskAgainCheckedInitially;
  bool AllowHelp;
  UnicodeString ImageName;
  UnicodeString MoreMessagesUrl;
  int MoreMessagesSize;
  UnicodeString CustomCaption;

private:
  inline void Reset();
};

class TCustomScpExplorerForm;
TCustomScpExplorerForm *CreateScpExplorer();

void ConfigureInterface();

void DoProductLicense();

extern const UnicodeString AppName;

void SetOnForeground(bool OnForeground);
void FlashOnBackground();

void TerminateApplication();
void ShowExtendedExceptionEx(TTerminal *Terminal, Exception *E);
//void FormHelp(TCustomForm * Form);
void SearchHelp(UnicodeString Message);
void MessageWithNoHelp(UnicodeString Message);

class TProgramParams;
bool CheckSafe(TProgramParams *Params);
void CheckLogParam(TProgramParams *Params);
bool CheckXmlLogParam(TProgramParams *Params);

#if 0
UnicodeString GetToolbarsLayoutStr(TComponent *OwnerComponent);
void LoadToolbarsLayoutStr(TComponent *OwnerComponent, UnicodeString LayoutStr);

namespace Tb2item { class TTBCustomItem; }
void AddMenuSeparator(Tb2item::TTBCustomItem *Menu);
void AddMenuLabel(Tb2item::TTBCustomItem *Menu, UnicodeString Label);
#endif // #if 0

// windows\WinHelp.cpp
void InitializeWinHelp();
void FinalizeWinHelp();

// windows\WinInterface.cpp
uintptr_t MessageDialog(const UnicodeString Msg, TQueryType Type,
  uint32_t Answers, const UnicodeString HelpKeyword = HELP_NONE, const TMessageParams *Params = nullptr);
uintptr_t MessageDialog(intptr_t Ident, TQueryType Type,
  uint32_t Answers, const UnicodeString HelpKeyword = HELP_NONE, const TMessageParams *Params = nullptr);
uintptr_t SimpleErrorDialog(UnicodeString Msg, UnicodeString MoreMessages = L"");

uintptr_t MoreMessageDialog(UnicodeString Message,
  TStrings *MoreMessages, TQueryType Type, uint32_t Answers,
  const UnicodeString HelpKeyword, const TMessageParams *Params = nullptr);

uintptr_t ExceptionMessageDialog(Exception *E, TQueryType Type,
  const UnicodeString MessageFormat = L"", uint32_t Answers = qaOK,
  const UnicodeString HelpKeyword = HELP_NONE, const TMessageParams *Params = nullptr);
uintptr_t FatalExceptionMessageDialog(Exception *E, TQueryType Type,
  intptr_t SessionReopenTimeout, UnicodeString MessageFormat = L"", uint32_t Answers = qaOK,
  const UnicodeString HelpKeyword = HELP_NONE, const TMessageParams *Params = nullptr);

#if defined(FARPLUGIN)

// forms\Custom.cpp
TSessionData *DoSaveSession(TSessionData *SessionData,
  TSessionData *OriginalSession, bool ForceDialog,
  TStrings *AdditionalFolders);
void SessionNameValidate(UnicodeString Text,
  UnicodeString OriginalName);
bool DoSaveWorkspaceDialog(UnicodeString &WorkspaceName,
  bool *SavePasswords, bool NotRecommendedSavingPasswords,
  bool &CreateShortcut, bool &EnableAutoSave);
class TShortCuts;
bool DoShortCutDialog(TShortCut &ShortCut,
  const TShortCuts &ShortCuts, UnicodeString HelpKeyword);
#if 0
bool DoCustomCommandOptionsDialog(
  const TCustomCommandType *Command, TStrings *CustomCommandOptions, uintptr_t AFlags,
  TCustomCommand *CustomCommandForOptions, UnicodeString Site);
#endif // #if 0

#endif // FARPLUGIN

void DoUsageStatisticsDialog();

// windows\UserInterface.cpp
bool DoMasterPasswordDialog();
bool DoChangeMasterPasswordDialog(UnicodeString &NewPassword);

// windows\WinMain.cpp
int Execute();
#if defined(FARPLUGIN)
void GetLoginData(UnicodeString SessionName, TOptions *Options,
  TObjectList *DataList, UnicodeString &DownloadFile, bool NeedSession);
#endif // FARPLUGIN

#if 0
typedef void (__closure *TInputDialogInitialize)
  (TObject * Sender, TInputDialogData * Data);
#endif // #if 0
bool InputDialog(UnicodeString ACaption,
  UnicodeString APrompt, UnicodeString &Value, UnicodeString HelpKeyword = HELP_NONE,
  TStrings *History = nullptr, bool PathInput = false,
  TInputDialogInitializeEvent OnInitialize = nullptr, bool Echo = true);

// forms\About.cpp
struct TRegistration
{
  bool Registered;
  UnicodeString Subject;
  int Licenses;
  UnicodeString ProductId;
  bool NeverExpires;
  TDateTime Expiration;
  bool EduLicense;
  TNotifyEvent OnRegistrationLink;
};
void DoAboutDialog(TConfiguration *Configuration,
  bool AllowLicense, TRegistration *Registration);
void DoAboutDialog(TConfiguration *Configuration);

// forms\Cleanup.cpp
bool DoCleanupDialog(TStoredSessionList *SessionList,
  TConfiguration *Configuration);

// forms\Console.cpp
void DoConsoleDialog(TTerminal *Terminal,
  UnicodeString Command = L"", const TStrings *Log = nullptr);

#if defined(FARPLUGIN)

// forms\Copy.cpp
const int coTemp                = 0x001;
const int coDisableQueue        = 0x002;
const int coDisableDirectory    = 0x008; // not used anymore
const int coDoNotShowAgain      = 0x020;
const int coDisableSaveSettings = 0x040; // not used anymore
const int coDoNotUsePresets     = 0x080;
const int coAllowRemoteTransfer = 0x100;
const int coNoQueue             = 0x200;
const int coShortCutHint        = 0x800;
const int coAllFiles            = 0x1000;
const int cooDoNotShowAgain     = 0x01;
const int cooRemoteTransfer     = 0x02;
const int cooSaveSettings       = 0x04;

const int coTempTransfer        = 0x08;
const int coDisableNewerOnly    = 0x10;

bool DoCopyDialog(bool ToRemote,
  bool Move, TStrings *FileList, UnicodeString &TargetDirectory,
  TGUICopyParamType *Params, int Options, int CopyParamAttrs,
  TSessionData *SessionData, int *OutputOptions);

// forms\CreateDirectory.cpp
bool DoCreateDirectoryDialog(UnicodeString &Directory,
  TRemoteProperties *Properties, int AllowedChanges, bool &SaveSettings);

// forms\ImportSessions.cpp
bool DoImportSessionsDialog(TList *Imported);

// forms\License.cpp
enum TLicense { lcNoLicense = -1, lcWinScp, lcExpat };
void DoLicenseDialog(TLicense License);

bool DoLoginDialog(TStoredSessionList *SessionList, TList *DataList);

// forms\SiteAdvanced.cpp
bool DoSiteAdvancedDialog(TSessionData *SessionData);

// forms\OpenDirectory.cpp
enum TOpenDirectoryMode
{
  odBrowse,
  odAddBookmark
};

bool DoOpenDirectoryDialog(TOpenDirectoryMode Mode, TOperationSide Side,
  UnicodeString &Directory, TStrings *Directories, TTerminal *Terminal,
  bool AllowSwitch);

// forms\LocationProfiles.cpp
bool LocationProfilesDialog(TOpenDirectoryMode Mode,
  TOperationSide Side, UnicodeString &LocalDirectory, UnicodeString &RemoteDirectory,
  TStrings *LocalDirectories, TStrings *RemoteDirectories, TTerminal *Terminal);

// forms\Preferences.cpp
enum TPreferencesMode
{
  pmDefault,
  pmEditor,
  pmCustomCommands,
  pmQueue, pmLogging, pmUpdates, pmPresets, pmEditors, pmCommander,
  pmEditorInternal,
};

struct TCopyParamRuleData;
struct TPreferencesDialogData
{
  TCopyParamRuleData *CopyParamRuleData;
};

bool DoPreferencesDialog(TPreferencesMode APreferencesMode,
  TPreferencesDialogData *DialogData = nullptr);

// forms\CustomCommand.cpp
class TCustomCommandList;
class TCustomCommandType;
class TShortCuts;

enum TCustomCommandsMode
{
  ccmAdd,
  ccmEdit,
  ccmAdHoc,
};

const intptr_t ccoDisableRemote = 0x01;
const intptr_t ccoDisableRemoteFiles = 0x02;

#if 0
typedef void (__closure *TCustomCommandValidate)
  (const TCustomCommandType & Command);
#endif // #if 0
typedef nb::FastDelegate1<void,
  const TCustomCommandType & /*Command*/> TCustomCommandValidateEvent;

bool DoCustomCommandDialog(TCustomCommandType &Command,
  const TCustomCommandList *CustomCommandList,
  TCustomCommandsMode Mode, int Options, TCustomCommandValidateEvent OnValidate,
  const TShortCuts *ShortCuts);

// forms\CopyParamPreset.cpp
class TCopyParamList;
enum TCopyParamPresetMode { cpmAdd, cpmAddCurrent, cpmEdit, cpmDuplicate };
bool DoCopyParamPresetDialog(TCopyParamList *CopyParamList,
  int &Index, TCopyParamPresetMode Mode, TCopyParamRuleData *CurrentRuleData,
  const TCopyParamType &DefaultCopyParams);

// forms\CopyParamCustom.cpp
bool DoCopyParamCustomDialog(TCopyParamType &CopyParam,
  int CopyParamAttrs);

// forms\Properties.cpp
class TRemoteProperties;
class TRemoteTokenList;
struct TCalculateSizeStats;
const int cpMode =  0x01;
const int cpOwner = 0x02;
const int cpGroup = 0x04;
#if 0
typedef void (__closure *TCalculateSizeEvent)
  (TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats,
   bool & Close);
#endif // #if 0
typedef nb::FastDelegate4<void,
  TStrings * /*FileList*/, int64_t & /*Size*/, TCalculateSizeStats & /*Stats*/,
  bool & /*Close*/> TCalculateSizeEvent;
#if 0
typedef void (__closure *TCalculatedChecksumCallbackEvent)(
  UnicodeString FileName, UnicodeString Alg, UnicodeString Hash);
#endif // #if 0
typedef nb::FastDelegate3<void,
  UnicodeString /*FileName*/, UnicodeString /*Alg*/,
  UnicodeString /*Hash*/> TCalculatedChecksumCallbackEvent;
#if 0
typedef void (__closure *TCalculateChecksumEvent)
  (UnicodeString Alg, TStrings * FileList,
   TCalculatedChecksumCallbackEvent OnCalculatedChecksum, bool & Close);
#endif // #if 0
typedef nb::FastDelegate4<void,
  UnicodeString /*Alg*/, TStrings * /*FileList*/,
  TCalculatedChecksumCallbackEvent /*OnCalculatedChecksum*/,
  bool & /*Close*/> TCalculateChecksumEvent;

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
  UnicodeString & Target, UnicodeString & FileMask, bool & DirectCopy);

// forms\SelectMask.cpp
#ifdef CustomdirviewHPP
bool DoSelectMaskDialog(TCustomDirView *Parent, bool Select,
  TFileFilter *Filter, TConfiguration *Configuration);
bool DoFilterMaskDialog(TCustomDirView *Parent,
  TFileFilter *Filter);
#endif

// forms\EditMask.cpp
bool DoEditMaskDialog(TFileMasks &Mask);

const int spDelete = 0x01;
const int spNoConfirmation = 0x02;
const int spExistingOnly = 0x04;
const int spPreviewChanges = 0x40; // not used by core
const int spTimestamp = 0x100;
const int spNotByTime = 0x200;
const int spBySize = 0x400;
const int spSelectedOnly = 0x800;
const int spMirror = 0x1000;
// forms\Synchronize.cpp
const int soDoNotUsePresets =  0x01;
const int soNoMinimize =       0x02;
const int soAllowSelectedOnly = 0x04;

#if 0
typedef void (__closure *TGetSynchronizeOptionsEvent)
  (int Params, TSynchronizeOptions & Options);
#endif // #if 0
typedef nb::FastDelegate2<void,
  intptr_t /*Params*/,
  TSynchronizeOptions & /*Options*/> TGetSynchronizeOptionsEvent;
#if 0
typedef void (__closure *TSynchronizeSessionLog)
  (UnicodeString Message);
#endif // #if 0
typedef nb::FastDelegate1<void,
  UnicodeString /*Message*/> TSynchronizeSessionLogEvent;
#if 0
typedef void (__closure *TFeedSynchronizeError)
  (UnicodeString Message, TStrings * MoreMessages, TQueryType Type,
   UnicodeString HelpKeyword);
#endif // #if 0
typedef nb::FastDelegate4<void,
  UnicodeString /*Message*/, TStrings * /*MoreMessages*/, TQueryType /*Type*/,
  UnicodeString /*HelpKeyword*/> TFeedSynchronizeErrorEvent;

bool DoSynchronizeDialog(TSynchronizeParamType &Params,
  const TCopyParamType *CopyParams, TSynchronizeStartStopEvent OnStartStop,
  bool &SaveSettings, int Options, int CopyParamAttrs,
  TGetSynchronizeOptionsEvent OnGetOptions,
  TSynchronizeSessionLogEvent OnSynchronizeSessionLog,
  TFeedSynchronizeErrorEvent OnFeedSynchronizeError,
  bool Start);

// forms\FullSynchronize.cpp
struct TUsableCopyParamAttrs;
enum TSynchronizeMode { smRemote, smLocal, smBoth };
const int fsoDisableTimestamp = 0x01;
const int fsoDoNotUsePresets =  0x02;
const int fsoAllowSelectedOnly = 0x04;

bool DoFullSynchronizeDialog(TSynchronizeMode &Mode, intptr_t &Params,
  UnicodeString &LocalDirectory, UnicodeString &RemoteDirectory,
  TCopyParamType *CopyParams, bool &SaveSettings, bool &SaveMode,
  intptr_t Options, const TUsableCopyParamAttrs &CopyParamAttrs);

// forms\SynchronizeChecklist.cpp
class TSynchronizeChecklist;
#if 0
typedef void (__closure *TCustomCommandMenuEvent)
  (TAction * Action, TStrings * LocalFileList, TStrings * RemoteFileList);
#endif // #if 0
typedef nb::FastDelegate3<void,
  void * /*Action*/, TStrings * /*LocalFileList*/,
  TStrings * /*RemoteFileList*/> TCustomCommandMenuEvent;

bool DoSynchronizeChecklistDialog(TSynchronizeChecklist *Checklist,
  TSynchronizeMode Mode, intptr_t Params,
  UnicodeString LocalDirectory, UnicodeString RemoteDirectory,
  TCustomCommandMenuEvent OnCustomCommandMenu);

#endif // FARPLUGIN

// forms\Editor.cpp
#if 0
typedef void (__closure *TFileClosedEvent)
  (TObject * Sender, bool Forced);
typedef void (__closure *TAnyModifiedEvent)
  (TObject * Sender, bool & Modified);
#endif // #if 0
#if 0
typedef nb::FastDelegate2<void,
  TObject * /*Sender* /, bool /*Forced*/> TFileClosedEvent;
typedef nb::FastDelegate2<void,
  TObject * /*Sender* /, bool & /*Modified*/> TAnyModifiedEvent;
TForm * ShowEditorForm(const UnicodeString FileName, TForm * ParentForm,
  TNotifyEvent OnFileChanged, TNotifyEvent OnFileReload, TFileClosedEvent OnClose,
  TNotifyEvent OnSaveAll, TAnyModifiedEvent OnAnyModified,
  const UnicodeString Caption, bool StandaloneEditor, TColor Color, int InternalEditorEncodingOverride);
void ReconfigureEditorForm(TForm *Form);
void EditorFormFileUploadComplete(TForm *Form);
void EditorFormFileSave(TForm *Form);
bool IsEditorFormModified(TForm *Form);
#endif // #if 0

#if defined(FARPLUGIN)

bool DoSymlinkDialog(UnicodeString &FileName, UnicodeString &PointTo,
  TOperationSide Side, bool &SymbolicLink, bool Edit, bool AllowSymbolic);

// forms\FileSystemInfo.cpp
struct TSpaceAvailable;
struct TFileSystemInfo;
struct TSessionInfo;
#if 0
typedef void (__closure *TGetSpaceAvailable)
  (const UnicodeString Path, TSpaceAvailable & ASpaceAvailable, bool & Close);
#endif // #if 0
typedef nb::FastDelegate3<void,
  UnicodeString /*Path*/, TSpaceAvailable & /*ASpaceAvailable*/,
  bool & /*Close*/> TGetSpaceAvailableEvent;

void DoFileSystemInfoDialog(
  const TSessionInfo &SessionInfo, const TFileSystemInfo &FileSystemInfo,
  UnicodeString SpaceAvailablePath, TGetSpaceAvailableEvent OnGetSpaceAvailable);

//moved to FarInterface.h
#if 0
// forms\MessageDlg.cpp
void AnswerNameAndCaption(
  uint32_t Answer, UnicodeString &Name, UnicodeString &Caption);
TFarDialog *CreateMoreMessageDialog(UnicodeString Msg,
  TStrings *MoreMessages, TMsgDlgType DlgType, uint32_t Answers,
  const TQueryButtonAlias *Aliases, uintptr_t AliasesCount,
  uintptr_t TimeoutAnswer, TFarButton **TimeoutButton,
  UnicodeString ImageName, UnicodeString NeverAskAgainCaption,
  UnicodeString MoreMessagesUrl, TSize MoreMessagesSize,
  UnicodeString CustomCaption);
TFarDialog *CreateMoreMessageDialogEx(UnicodeString Message, TStrings *MoreMessages,
  TQueryType Type, uint32_t Answers, UnicodeString HelpKeyword, const TMessageParams *Params);
uintptr_t ExecuteMessageDialog(TFarDialog *Dialog, uint32_t Answers, const TMessageParams *Params);
void InsertPanelToMessageDialog(TFarDialog *Form, TPanel *Panel);
void NavigateMessageDialogToUrl(TFarDialog *Form, UnicodeString Url);

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
bool DoEditorPreferencesDialog(TEditorData *Editor,
  bool &Remember, TEditorPreferencesMode Mode, bool MayRemote);

// forms\Find.cpp
#if 0
typedef void (__closure *TFindEvent)
  (TTerminal * Terminal, UnicodeString Directory, const TFileMasks & FileMask,
   TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
#endif // #if 0
typedef nb::FastDelegate4<void,
  UnicodeString /*Directory*/, const TFileMasks & /*FileMask*/,
  TFileFoundEvent /*OnFileFound*/,
  TFindingFileEvent /*OnFindingFile*/> TFindEvent;
#if 0
typedef void (__closure *TFocusFileEvent)
  (TTerminal * Terminal, UnicodeString Path);
#endif // #if 0
typedef nb::FastDelegate2<void,
  TTerminal * /*Terminal*/, UnicodeString /*Path*/> TFocusFileEvent;
#if 0
typedef void (__closure *TFileOperationFinished2Event)
  (UnicodeString FileName, bool Success);
#endif // #if 0
typedef nb::FastDelegate2<void,
  UnicodeString /*FileName*/, bool /*Success*/> TFileOperationFinished2Event;
#if 0
typedef void (__closure *TFileListOperationEvent)
  (TTerminal * Terminal, TStrings * FileList, TFileOperationFinished2Event OnFileOperationFinished);
#endif // #if 0
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, TStrings * /*FileList*/, TFileOperationFinished2Event /*OnFileOperationFinished*/> TFileListOperationEvent;

void ShowFileFindDialog(
  TTerminal *Terminal, UnicodeString Directory, TFindEvent OnFind, TFocusFileEvent OnFocusFile,
  TFileListOperationEvent OnDeleteFiles, TFileListOperationEvent OnDownloadFiles);
void HideFileFindDialog();

// forms\GenerateUrl.cpp
void DoGenerateUrlDialog(TSessionData *Data, TStrings *Paths);
enum TFilesSelected { fsList, fsAll };
void DoGenerateTransferCodeDialog(
  bool ToRemote, bool Move, int CopyParamAttrs, TSessionData *Data, TFilesSelected FilesSelected,
  TStrings *FileList, UnicodeString Path, const TCopyParamType &CopyParam);

#if 0
void CopyParamListButton(TButton *Button);
const int cplNone =             0x00;
const int cplCustomize =        0x01;
const int cplCustomizeDefault = 0x02;
const int cplSaveSettings =     0x04;
const int cplGenerateCode =     0x08;
void CopyParamListPopup(TRect R, TPopupMenu *Menu,
  const TCopyParamType &Param, UnicodeString Preset, TNotifyEvent OnClick,
  int Options, int CopyParamAttrs, bool SaveSettings = false);
int CopyParamListPopupClick(TObject *Sender,
  TCopyParamType &Param, UnicodeString &Preset, int CopyParamAttrs,
  bool *SaveSettings = nullptr);

void MenuPopup(TPopupMenu *Menu, TRect Rect, TComponent *PopupComponent);
void MenuPopup(TPopupMenu *Menu, TButton *Button);
void MenuPopup(TObject *Sender, const TPoint &MousePos, bool &Handled);
void MenuButton(TButton *Button);
TComponent *GetPopupComponent(TObject *Sender);
TRect CalculatePopupRect(TButton *Button);
TRect CalculatePopupRect(TControl *Control, TPoint MousePos);

typedef void (__closure *TColorChangeEvent)
(TColor Color);
TPopupMenu *CreateSessionColorPopupMenu(TColor Color,
  TColorChangeEvent OnColorChange);
void CreateSessionColorMenu(TComponent *AOwner, TColor Color,
  TColorChangeEvent OnColorChange);
void CreateEditorBackgroundColorMenu(TComponent *AOwner, TColor Color,
  TColorChangeEvent OnColorChange);
TPopupMenu *CreateColorPopupMenu(TColor Color,
  TColorChangeEvent OnColorChange);

void FixButtonImage(TButton *Button);
void CenterButtonImage(TButton *Button);

void UpgradeSpeedButton(TSpeedButton *Button);

int AdjustLocaleFlag(UnicodeString S, TLocaleFlagOverride LocaleFlagOverride, bool Recommended, int On, int Off);

void SetGlobalMinimizeHandler(TCustomForm *Form, TNotifyEvent OnMinimize);
void ClearGlobalMinimizeHandler(TNotifyEvent OnMinimize);
void CallGlobalMinimizeHandler(TObject *Sender);
bool IsApplicationMinimized();
void ApplicationMinimize();
void ApplicationRestore();
bool HandleMinimizeSysCommand(TMessage &Message);

void WinInitialize();
void WinFinalize();
#endif // #if 0

void ShowNotification(TTerminal *Terminal, UnicodeString Str,
  TQueryType Type);
#if 0
void InitializeShortCutCombo(TComboBox *ComboBox,
  const TShortCuts &ShortCuts);
void SetShortCutCombo(TComboBox *ComboBox, TShortCut Value);
TShortCut GetShortCutCombo(TComboBox *ComboBox);
bool IsCustomShortCut(TShortCut ShortCut);
TShortCut __fastcall NormalizeCustomShortCut(TShortCut ShortCut);
#endif // #if 0

#ifdef _DEBUG
void ForceTracing();
#endif

#define HIDDEN_WINDOW_NAME L"WinSCPHiddenWindow3"

struct TCopyDataMessage
{
  enum { CommandCanCommandLine, CommandCommandLine, MainWindowCheck, RefreshPanel };
  static const unsigned int Version1 = 1;

  uintptr_t Version;
  uintptr_t Command;

  union
  {
    wchar_t CommandLine[10240];

    struct
    {
      wchar_t Session[1024];
      wchar_t Path[1024];
    } Refresh;
  };

  TCopyDataMessage()
  {
    Version = TCopyDataMessage::Version1;
    Command = static_cast<unsigned int>(-1);
  }
};

class TWinInteractiveCustomCommand : public TInteractiveCustomCommand
{
public:
  TWinInteractiveCustomCommand(
    TCustomCommand *ChildCustomCommand, const UnicodeString CustomCommandName, const UnicodeString HelpKeyword);

protected:
  virtual void Prompt(intptr_t Index, UnicodeString Prompt,
    UnicodeString &Value) const override;
  virtual void Execute(UnicodeString Command,
    UnicodeString &Value) const override;
  virtual void PatternHint(intptr_t Index, UnicodeString Pattern) override;

private:
  UnicodeString FCustomCommandName;
  rde::map<int, size_t> FIndexes;
  TUnicodeStringVector FPrompts;
  TUnicodeStringVector FDefaults;
  TUnicodeStringVector FValues;
  UnicodeString FHelpKeyword;
};

#if 0

class TTrayIcon
{
public:
  TTrayIcon(unsigned int Id);
  ~TTrayIcon();

  void PopupBalloon(UnicodeString Title, UnicodeString Str,
    TQueryType QueryType, unsigned int Timeout, TNotifyEvent OnBalloonClick,
    TObject *BalloonUserData);
  void CancelBalloon();

  __property bool Visible = { read = FVisible, write = SetVisible };
  __property TNotifyEvent OnClick = { read = FOnClick, write = FOnClick };
  __property UnicodeString Hint = { read = GetHint, write = SetHint };

protected:
  void Update();
  bool Notify(unsigned int Message);

private:
  bool FVisible;
  NOTIFYICONDATA *FTrayIcon;
  TNotifyEvent FOnClick;
  TNotifyEvent FOnBalloonClick;
  TObject *FBalloonUserData;
  UINT FTaskbarCreatedMsg;

  void WndProc(TMessage &Message);
  void SetVisible(bool value);
  UnicodeString GetHint();
  void SetHint(UnicodeString value);
  void BalloonCancelled();
};

#endif // #if 0

#endif // FARPLUGIN
