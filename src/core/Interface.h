
#pragma once

#include <Classes.hpp>
#if defined(FARPLUGIN)
#include "Configuration.h"
#include "SessionData.h"
#endif // FARPLUGIN
__removed #include <typeinfo>
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
extern const wchar_t *TransferModeNames[];
extern const int TransferModeNamesCount;
extern const wchar_t *ToggleNames[];
enum TToggle { ToggleOff, ToggleOn };
//---------------------------------------------------------------------------
#if defined(FARPLUGIN)
NB_CORE_EXPORT TConfiguration *CreateConfiguration();
class TOptions;
NB_CORE_EXPORT TOptions *GetGlobalOptions();

#endif // FARPLUGIN

NB_CORE_EXPORT void ShowExtendedException(Exception *E);
NB_CORE_EXPORT bool AppendExceptionStackTraceAndForget(TStrings *&MoreMessages);
#if 0
void IgnoreException(const std::type_info &ExceptionType);
UnicodeString GetExceptionDebugInfo();
#endif // #if 0

NB_CORE_EXPORT UnicodeString GetCompanyRegistryKey();
NB_CORE_EXPORT UnicodeString GetRegistryKey();
NB_CORE_EXPORT void *BusyStart();
NB_CORE_EXPORT void BusyEnd(void *Token);
NB_CORE_EXPORT extern const uint32_t GUIUpdateInterval;
NB_CORE_EXPORT void SetNoGUI();
NB_CORE_EXPORT bool ProcessGUI(bool Force = false);
NB_CORE_EXPORT UnicodeString GetAppNameString();
NB_CORE_EXPORT UnicodeString GetSshVersionString();
NB_CORE_EXPORT void CopyToClipboard(const UnicodeString Text);
NB_CORE_EXPORT HANDLE StartThread(void *SecurityAttributes, DWORD StackSize,
  /*TThreadFunc ThreadFunc,*/ void *Parameter, DWORD CreationFlags,
  TThreadID &ThreadId);
bool TextFromClipboard(UnicodeString &Text, bool Trim);

NB_CORE_EXPORT void WinInitialize();
NB_CORE_EXPORT void WinFinalize();

#if 0
typedef void (__closure *TButtonSubmitEvent)(TObject * Sender, unsigned int & Answer);
#endif // #if 0
typedef nb::FastDelegate2<void,
  TObject * /*Sender*/, uint32_t & /*Answer*/> TButtonSubmitEvent;

struct NB_CORE_EXPORT TQueryButtonAlias : public TObject
{
  TQueryButtonAlias();

  uintptr_t Button;
  UnicodeString Alias;
  TButtonSubmitEvent OnClick;
  int GroupWith;
  bool Default;
  TShiftStateFlag GrouppedShiftState;
  bool ElevationRequired;
  bool MenuButton;
  UnicodeString ActionAlias;

  static TQueryButtonAlias CreateYesToAllGrouppedWithYes();
  static TQueryButtonAlias CreateNoToAllGrouppedWithNo();
  static TQueryButtonAlias CreateAllAsYesToNewerGrouppedWithYes();
  static TQueryButtonAlias CreateIgnoreAsRenameGrouppedWithNo();
};

#if 0
typedef void (__closure *TQueryParamsTimerEvent)(uintptr_t &Result);
#endif // #if 0
typedef nb::FastDelegate1<void, uint32_t & /*Result*/> TQueryParamsTimerEvent;

struct NB_CORE_EXPORT TQueryParams : public TObject
{
  explicit TQueryParams(uintptr_t AParams = 0, const UnicodeString AHelpKeyword = HELP_NONE);
  explicit TQueryParams(const TQueryParams &Source);

  void Assign(const TQueryParams &Source);

  const TQueryButtonAlias *Aliases;
  uintptr_t AliasesCount;
  uintptr_t Params;
  uintptr_t Timer;
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uint32_t TimerAnswers;
  TQueryType TimerQueryType;
  uintptr_t Timeout;
  uintptr_t TimeoutAnswer;
  uintptr_t NoBatchAnswers;
  UnicodeString HelpKeyword;

public:
  TQueryParams &operator=(const TQueryParams &other);

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
NB_CORE_EXPORT bool IsPasswordOrPassphrasePrompt(TPromptKind Kind, TStrings *Prompts);
NB_CORE_EXPORT bool IsPasswordPrompt(TPromptKind Kind, TStrings *Prompts);
//---------------------------------------------------------------------------
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
  TTerminal * /*Terminal*/, const UnicodeString & /*ADirectory*/, bool & /*Cancel*/> TFindingFileEvent;
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TOperationVisualizer
{
  NB_DISABLE_COPY(TOperationVisualizer)
public:
  explicit TOperationVisualizer(bool UseBusyCursor = true);
  ~TOperationVisualizer();

private:
  bool FUseBusyCursor;
  void *FToken;
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TInstantOperationVisualizer : public TOperationVisualizer
{
public:
  TInstantOperationVisualizer();
  ~TInstantOperationVisualizer();

private:
  TDateTime FStart;
};
//---------------------------------------------------------------------------
struct TClipboardHandler
{
  NB_DISABLE_COPY(TClipboardHandler)
public:
  TClipboardHandler() {}

  UnicodeString Text;

  void Copy(TObject * /*Sender*/, uint32_t & /*Answer*/)
  {
    volatile TInstantOperationVisualizer Visualizer;
    CopyToClipboard(Text);
  }
};
//---------------------------------------------------------------------------
