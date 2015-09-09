#pragma once

#include "Configuration.h"
#include "SessionData.h"
#define HELP_NONE ""

TConfiguration * CreateConfiguration();
class TOptions;
TOptions * GetGlobalOptions();

void ShowExtendedException(Exception * E);
bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages);

UnicodeString GetRegistryKey();
void * BusyStart();
void BusyEnd(void * Token);
const uint32_t GUIUpdateInterval = 200;

void WinInitialize();
void WinFinalize();
void SetNoGUI();
bool ProcessGUI(bool Force = false);

UnicodeString GetAppNameString();
UnicodeString GetSshVersionString();
void CopyToClipboard(const UnicodeString & Text);
HANDLE StartThread(void * SecurityAttributes, DWORD StackSize,
  /*TThreadFunc ThreadFunc,*/ void * Parameter, DWORD CreationFlags,
  TThreadID & ThreadId);

struct TQueryButtonAlias : public TObject
{
  TQueryButtonAlias();

  uintptr_t Button;
  UnicodeString Alias;
  TNotifyEvent OnClick;
  int GroupWith;
  bool Default;
  TShiftStateFlag GrouppedShiftState;
};

DEFINE_CALLBACK_TYPE1(TQueryParamsTimerEvent, void,
  intptr_t & /*Result*/);

enum TQueryType
{
  qtConfirmation,
  qtWarning,
  qtError,
  qtInformation
};

struct TQueryParams : public TObject
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
  pupRemember = 0x02
};

bool IsAuthenticationPrompt(TPromptKind Kind);
bool IsPasswordOrPassphrasePrompt(TPromptKind Kind, TStrings * Prompts);
bool IsPasswordPrompt(TPromptKind Kind, TStrings * Prompts);

DEFINE_CALLBACK_TYPE4(TFileFoundEvent, void,
  TTerminal * /*Terminal*/, const UnicodeString & /*FileName*/,
  const TRemoteFile * /*File*/,
  bool & /*Cancel*/);
DEFINE_CALLBACK_TYPE3(TFindingFileEvent, void,
  TTerminal * /*Terminal*/, const UnicodeString & /*Directory*/, bool & /*Cancel*/);

class TOperationVisualizer
{
NB_DISABLE_COPY(TOperationVisualizer)
public:
  explicit TOperationVisualizer(bool UseBusyCursor);
  ~TOperationVisualizer();

private:
  bool FUseBusyCursor;
  void * FToken;
};

class TInstantOperationVisualizer : public TOperationVisualizer
{
public:
  TInstantOperationVisualizer();
  ~TInstantOperationVisualizer();

private:
  TDateTime FStart;
};

struct TClipboardHandler
{
NB_DECLARE_CLASS(TClipboardHandler)
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

