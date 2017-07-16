
#pragma once

#include <Classes.hpp>
#if defined(FARPLUGIN)
#include "Configuration.h"
#include "SessionData.h"
#endif // FARPLUGIN
#define HELP_NONE L""
#define SCRIPT_SWITCH "script"
#define COMMAND_SWITCH L"Command"
#define SESSIONNAME_SWITCH L"SessionName"
#define NEWPASSWORD_SWITCH L"newpassword"
#define INI_NUL L"nul"
#define PRESERVETIME_SWITCH L"preservetime"
#define PRESERVETIMEDIRS_SWITCH_VALUE L"all"
#define NOPRESERVETIME_SWITCH L"nopreservetime"
#define PERMISSIONS_SWITCH L"permissions"
#define NOPERMISSIONS_SWITCH L"nopermissions"
#define SPEED_SWITCH L"speed"
#define TRANSFER_SWITCH L"transfer"
#define FILEMASK_SWITCH L"filemask"
#define RESUMESUPPORT_SWITCH L"resumesupport"
#define NEWERONLY_SWICH L"neweronly"
#define NONEWERONLY_SWICH L"noneweronly"
#define DELETE_SWITCH L"delete"
#define REFRESH_SWITCH L"refresh"
extern const wchar_t * TransferModeNames[];
extern const int TransferModeNamesCount;
extern const wchar_t * ToggleNames[];
enum TToggle { ToggleOff, ToggleOn };

#if defined(FARPLUGIN)

NB_CORE_EXPORT TConfiguration * CreateConfiguration();
class TOptions;
NB_CORE_EXPORT TOptions * GetGlobalOptions();

#endif // FARPLUGIN

NB_CORE_EXPORT void ShowExtendedException(Exception * E);
NB_CORE_EXPORT bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages);
#if 0
void IgnoreException(const std::type_info & ExceptionType);
UnicodeString GetExceptionDebugInfo();
#endif // #if 0

NB_CORE_EXPORT UnicodeString GetCompanyRegistryKey();
NB_CORE_EXPORT UnicodeString GetRegistryKey();
NB_CORE_EXPORT void * BusyStart();
NB_CORE_EXPORT void BusyEnd(void * Token);
NB_CORE_EXPORT const uint32_t GUIUpdateInterval = 100;
NB_CORE_EXPORT void SetNoGUI();
NB_CORE_EXPORT bool ProcessGUI(bool Force = false);
NB_CORE_EXPORT UnicodeString GetAppNameString();
NB_CORE_EXPORT UnicodeString GetSshVersionString();
NB_CORE_EXPORT void CopyToClipboard(UnicodeString Text);
NB_CORE_EXPORT HANDLE StartThread(void * SecurityAttributes, DWORD StackSize,
  /*TThreadFunc ThreadFunc,*/ void * Parameter, DWORD CreationFlags,
  TThreadID & ThreadId);

NB_CORE_EXPORT void WinInitialize();
NB_CORE_EXPORT void WinFinalize();

#if 0
// moved to Common.h
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

struct NB_CORE_EXPORT TQueryButtonAlias : public TObject
{
  TQueryButtonAlias();

  uintptr_t Button;
  UnicodeString Alias;
  TNotifyEvent OnClick;
  int GroupWith;
  bool Default;
  TShiftStateFlag GrouppedShiftState;
  bool ElevationRequired;
  bool MenuButton;
};

#if 0
typedef void (__closure *TQueryParamsTimerEvent)(uintptr_t & Result);
#endif // #if 0
typedef nb::FastDelegate1<void, intptr_t & /*Result*/> TQueryParamsTimerEvent;

#if 0
// moved to Classes.h
enum TQueryType
{
  qtConfirmation,
  qtWarning,
  qtError,
  qtInformation,
};
#endif // #if 0

struct NB_CORE_EXPORT TQueryParams : public TObject
{
  explicit TQueryParams(uintptr_t AParams = 0, UnicodeString AHelpKeyword = HELP_NONE);
  explicit TQueryParams(const TQueryParams & Source);

  void Assign(const TQueryParams & Source);

  const TQueryButtonAlias * Aliases;
  uintptr_t AliasesCount;
  uintptr_t Params;
  uintptr_t Timer;
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uintptr_t TimerAnswers;
  TQueryType TimerQueryType;
  uintptr_t Timeout;
  uintptr_t TimeoutAnswer;
  uintptr_t NoBatchAnswers;
  UnicodeString HelpKeyword;

public:
  TQueryParams & operator=(const TQueryParams & other);

private:
  // NB_DISABLE_COPY(TQueryParams)
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

enum TPromptUserParam
{
  pupEcho = 0x01,
  pupRemember = 0x02,
};

NB_CORE_EXPORT bool IsAuthenticationPrompt(TPromptKind Kind);
NB_CORE_EXPORT bool IsPasswordOrPassphrasePrompt(TPromptKind Kind, TStrings * Prompts);
NB_CORE_EXPORT bool IsPasswordPrompt(TPromptKind Kind, TStrings * Prompts);

class TTerminal;
class TRemoteFile;

#if 0
typedef void (__closure *TFileFoundEvent)
  (TTerminal * Terminal, const UnicodeString FileName, const TRemoteFile * File,
   bool & Cancel);
#endif // #if 0
typedef nb::FastDelegate4<void,
  TTerminal * /*Terminal*/, UnicodeString /*FileName*/,
  const TRemoteFile * /*File*/,
  bool & /*Cancel*/> TFileFoundEvent;
#if 0
typedef void (__closure *TFindingFileEvent)
  (TTerminal * Terminal, const UnicodeString Directory, bool & Cancel);
#endif // #if 0
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, UnicodeString /*Directory*/, bool & /*Cancel*/> TFindingFileEvent;

class NB_CORE_EXPORT TOperationVisualizer
{
NB_DISABLE_COPY(TOperationVisualizer)
public:
  explicit TOperationVisualizer(bool UseBusyCursor = true);
  ~TOperationVisualizer();

private:
  bool FUseBusyCursor;
  void * FToken;
};

class NB_CORE_EXPORT TInstantOperationVisualizer : public TOperationVisualizer
{
public:
  TInstantOperationVisualizer();
  ~TInstantOperationVisualizer();

private:
  TDateTime FStart;
};

struct TClipboardHandler
{
NB_DISABLE_COPY(TClipboardHandler)
public:
  TClipboardHandler() {}

  UnicodeString Text;

  void Copy(TObject * /*Sender*/)
  {
    TInstantOperationVisualizer Visualizer;
    CopyToClipboard(Text);
  }
};

