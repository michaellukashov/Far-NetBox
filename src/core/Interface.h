
#pragma once

#include <Classes.hpp>
#if defined(FARPLUGIN)
#include "Configuration.h"
#include "SessionData.h"
#endif // FARPLUGIN
__removed #include <typeinfo>
#define HELP_NONE ""
#define SCRIPT_SWITCH "script"
#define COMMAND_SWITCH "Command"
#define SESSIONNAME_SWITCH "SessionName"
#define NEWPASSWORD_SWITCH "newpassword"
#define INI_NUL "nul"
#define PRESERVETIME_SWITCH "preservetime"
#define PRESERVETIMEDIRS_SWITCH_VALUE "all"
#define NOPRESERVETIME_SWITCH "nopreservetime"
#define PERMISSIONS_SWITCH "permissions"
#define NOPERMISSIONS_SWITCH "nopermissions"
#define SPEED_SWITCH "speed"
#define TRANSFER_SWITCH "transfer"
#define FILEMASK_SWITCH "filemask"
#define RESUMESUPPORT_SWITCH "resumesupport"
#define NEWERONLY_SWITCH "neweronly"
#define NONEWERONLY_SWITCH "noneweronly"
#define DELETE_SWITCH "delete"
#define REFRESH_SWITCH "refresh"
#define RAWTRANSFERSETTINGS_SWITCH "rawtransfersettings"
extern const wchar_t *TransferModeNames[];
extern const int TransferModeNamesCount;
extern const wchar_t *ToggleNames[];
enum TToggle { ToggleOff, ToggleOn };

#if defined(FARPLUGIN)
NB_CORE_EXPORT TConfiguration *CreateConfiguration();
class TOptions;
NB_CORE_EXPORT TOptions *GetGlobalOptions();
#endif // FARPLUGIN

NB_CORE_EXPORT void ShowExtendedException(Exception *E);
NB_CORE_EXPORT bool AppendExceptionStackTraceAndForget(TStrings *&MoreMessages);
__removed void IgnoreException(const std::type_info &ExceptionType);
__removed UnicodeString GetExceptionDebugInfo();

NB_CORE_EXPORT UnicodeString GetCompanyRegistryKey();
NB_CORE_EXPORT UnicodeString GetRegistryKey();
NB_CORE_EXPORT void *BusyStart();
NB_CORE_EXPORT void BusyEnd(void *Token);
NB_CORE_EXPORT static const uint32_t GUIUpdateInterval = 100;
NB_CORE_EXPORT void SetNoGUI();
NB_CORE_EXPORT bool ProcessGUI(bool Force = false);
NB_CORE_EXPORT UnicodeString GetAppNameString();
NB_CORE_EXPORT UnicodeString GetSshVersionString();
NB_CORE_EXPORT void CopyToClipboard(UnicodeString Text);
NB_CORE_EXPORT HANDLE StartThread(void *SecurityAttributes, DWORD StackSize,
  /*TThreadFunc ThreadFunc,*/ void *Parameter, DWORD CreationFlags,
  TThreadID &ThreadId);
NB_CORE_EXPORT bool TextFromClipboard(UnicodeString &Text, bool Trim);

NB_CORE_EXPORT void WinInitialize();
NB_CORE_EXPORT void WinFinalize();

#if 0
// Order of the values also define order of the buttons/answers on the prompts
// MessageDlg relies on these to be <= 0x0000FFFF
const unsigned int qaYes =      0x00000001;
// MessageDlg relies that answer do not conflict with mrCancel (=0x2)
const unsigned int qaNo =       0x00000004;
const unsigned int qaOK =       0x00000008;
const unsigned int qaCancel =   0x00000010;
const unsigned int qaYesToAll = 0x00000020;
const unsigned int qaNoToAll =  0x00000040;
const unsigned int qaAbort =    0x00000080;
const unsigned int qaRetry =    0x00000100;
const unsigned int qaIgnore =   0x00000200;
const unsigned int qaSkip =     0x00000400;
const unsigned int qaAll =      0x00000800;
const unsigned int qaHelp =     0x00001000;
const unsigned int qaReport =   0x00002000;

const unsigned int qaFirst = qaYes;
const unsigned int qaLast = qaReport;

const unsigned int qaNeverAskAgain = 0x00010000;

const int qpFatalAbort =           0x01;
const int qpNeverAskAgainCheck =   0x02;
const int qpAllowContinueOnError = 0x04;
const int qpIgnoreAbort =          0x08;
const int qpWaitInBatch =          0x10;
#endif // #if 0

#if 0
typedef void (__closure *TButtonSubmitEvent)(TObject * Sender, unsigned int & Answer);
#endif // #if 0
using TButtonSubmitEvent = nb::FastDelegate2<void,
  TObject * /*Sender*/, uint32_t & /*Answer*/>;

struct NB_CORE_EXPORT TQueryButtonAlias : public TObject
{
  TQueryButtonAlias() noexcept;

  uintptr_t Button{0};
  UnicodeString Alias;
  TButtonSubmitEvent OnSubmit;
  int GroupWith{0};
  bool Default{false};
  TShiftStateFlag GrouppedShiftState{ssShift};
  bool ElevationRequired{false};
  bool MenuButton{false};
  UnicodeString ActionAlias;

  static TQueryButtonAlias CreateYesToAllGrouppedWithYes();
  static TQueryButtonAlias CreateNoToAllGrouppedWithNo();
  static TQueryButtonAlias CreateAllAsYesToNewerGrouppedWithYes();
  static TQueryButtonAlias CreateIgnoreAsRenameGrouppedWithNo();
};

#if 0
typedef void (__closure *TQueryParamsTimerEvent)(unsigned int & Result);
#endif // #if 0
using TQueryParamsTimerEvent = nb::FastDelegate1<void, uint32_t & /*Result*/>;
__removed enum TQueryType { qtConfirmation, qtWarning, qtError, qtInformation };

struct NB_CORE_EXPORT TQueryParams : public TObject
{
//  TQueryParams() noexcept = delete;
  explicit TQueryParams(uintptr_t AParams = 0, const UnicodeString AHelpKeyword = HELP_NONE) noexcept;
  explicit TQueryParams(const TQueryParams &Source) noexcept;

  void Assign(const TQueryParams &Source);

  const TQueryButtonAlias *Aliases{nullptr};
  uintptr_t AliasesCount{0};
  uintptr_t Params{0};
  uintptr_t Timer{0};
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uint32_t TimerAnswers{0};
  TQueryType TimerQueryType;
  uintptr_t Timeout{0};
  uintptr_t TimeoutAnswer{0};
  uintptr_t NoBatchAnswers{0};
  UnicodeString HelpKeyword;

public:
  TQueryParams &operator=(const TQueryParams &other);
};

enum TPromptKind
{
  pkPrompt,
  pkFileName,
  pkUserName,
  pkPassphrase,
  pkTIS,
  pkCryptoCard,
  pkKeybInteractive,
  pkPassword,
  pkNewPassword,
};

enum TPromptUserParam { pupEcho = 0x01, pupRemember = 0x02, };

NB_CORE_EXPORT bool IsAuthenticationPrompt(TPromptKind Kind);
NB_CORE_EXPORT bool IsPasswordOrPassphrasePrompt(TPromptKind Kind, TStrings *Prompts);
NB_CORE_EXPORT bool IsPasswordPrompt(TPromptKind Kind, TStrings *Prompts);
class TTerminal;
class TRemoteFile;

#if 0
typedef void (__closure *TFileFoundEvent)
  (TTerminal *Terminal, const UnicodeString FileName, const TRemoteFile * File,
  bool & Cancel);
#endif // #if 0
using TFileFoundEvent = nb::FastDelegate4<void,
  TTerminal * /*Terminal*/, UnicodeString /*FileName*/,
  const TRemoteFile * /*File*/,
  bool & /*Cancel*/>;
#if 0
typedef void (__closure *TFindingFileEvent)
(TTerminal *Terminal, const UnicodeString Directory, bool &Cancel);
#endif // #if 0
using TFindingFileEvent = nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, UnicodeString /*ADirectory*/, bool & /*Cancel*/>;

class NB_CORE_EXPORT TOperationVisualizer
{
  NB_DISABLE_COPY(TOperationVisualizer)
public:
  TOperationVisualizer() = delete;
  explicit TOperationVisualizer(bool UseBusyCursor = true) noexcept;
  ~TOperationVisualizer() noexcept;

private:
  bool FUseBusyCursor{false};
  void *FToken{nullptr};
};

class NB_CORE_EXPORT TInstantOperationVisualizer : public TOperationVisualizer
{
public:
  TInstantOperationVisualizer() noexcept;
  ~TInstantOperationVisualizer() noexcept;

private:
  TDateTime FStart;
};

struct TClipboardHandler
{
  NB_DISABLE_COPY(TClipboardHandler)
public:
  TClipboardHandler() = default;

  UnicodeString Text;

  void Copy(TObject * /*Sender*/, uint32_t & /*Answer*/)
  {
    TInstantOperationVisualizer Visualizer; nb::used(Visualizer);
    CopyToClipboard(Text);
  }
};

