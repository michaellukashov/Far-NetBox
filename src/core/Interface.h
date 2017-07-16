
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
//void IgnoreException(const std::type_info & ExceptionType);
//UnicodeString GetExceptionDebugInfo();

NB_CORE_EXPORT UnicodeString GetCompanyRegistryKey();
NB_CORE_EXPORT UnicodeString GetRegistryKey();
NB_CORE_EXPORT void * BusyStart();
NB_CORE_EXPORT void BusyEnd(void * Token);
NB_CORE_EXPORT const uint32_t GUIUpdateInterval = 100;
NB_CORE_EXPORT void SetNoGUI();
NB_CORE_EXPORT bool ProcessGUI(bool Force = false);
NB_CORE_EXPORT UnicodeString GetAppNameString();
NB_CORE_EXPORT UnicodeString GetSshVersionString();
NB_CORE_EXPORT void CopyToClipboard(const UnicodeString & Text);
NB_CORE_EXPORT HANDLE StartThread(void * SecurityAttributes, DWORD StackSize,
  /*TThreadFunc ThreadFunc,*/ void * Parameter, DWORD CreationFlags,
  TThreadID & ThreadId);

NB_CORE_EXPORT void WinInitialize();
NB_CORE_EXPORT void WinFinalize();

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
};

/*
typedef void (__closure *TQueryParamsTimerEvent)(unsigned int & Result);
*/
typedef nb::FastDelegate1<void, intptr_t & /*Result*/> TQueryParamsTimerEvent;

struct NB_CORE_EXPORT TQueryParams : public TObject
{
  explicit TQueryParams(uintptr_t AParams = 0, const UnicodeString & AHelpKeyword = HELP_NONE);
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
  pkNewPassword
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

/*
typedef void (__closure *TFileFoundEvent)
  (TTerminal * Terminal, const UnicodeString FileName, const TRemoteFile * File,
   bool & Cancel);
*/
typedef nb::FastDelegate4<void,
  TTerminal * /*Terminal*/, const UnicodeString & /*FileName*/,
  const TRemoteFile * /*File*/,
  bool & /*Cancel*/> TFileFoundEvent;
/*
typedef void (__closure *TFindingFileEvent)
  (TTerminal * Terminal, const UnicodeString Directory, bool & Cancel);
*/
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, const UnicodeString & /*Directory*/, bool & /*Cancel*/> TFindingFileEvent;

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

