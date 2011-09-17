//---------------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------------
#include "Configuration.h"
#include "SessionData.h"
#define HELP_NONE L""
//---------------------------------------------------------------------------
typedef int (TObject::*TThreadFunc)(void *Parameter);
//---------------------------------------------------------------------------
TConfiguration * CreateConfiguration();

void ShowExtendedException(std::exception * E);

std::wstring GetRegistryKey();
void Busy(bool Start);
std::wstring AppNameString();
std::wstring SshVersionString();
void CopyToClipboard(std::wstring Text);
int StartThread(void * SecurityAttributes, unsigned StackSize,
  TThreadFunc ThreadFunc, void * Parameter, unsigned CreationFlags,
  unsigned & ThreadId);

const unsigned int qaYes =      0x00000001;
const unsigned int qaNo =       0x00000002;
const unsigned int qaOK =       0x00000004;
const unsigned int qaCancel =   0x00000008;
const unsigned int qaAbort =    0x00000010;
const unsigned int qaRetry =    0x00000020;
const unsigned int qaIgnore =   0x00000040;
const unsigned int qaAll =      0x00000080;
const unsigned int qaNoToAll =  0x00000100;
const unsigned int qaYesToAll = 0x00000200;
const unsigned int qaHelp =     0x00000400;
const unsigned int qaSkip =     0x00000800;

const unsigned int qaNeverAskAgain = 0x00010000;

const int qpFatalAbort =           0x01;
const int qpNeverAskAgainCheck =   0x02;
const int qpAllowContinueOnError = 0x04;

struct TQueryButtonAlias
{
  TQueryButtonAlias();

  unsigned int Button;
  std::wstring Alias;
  TNotifyEvent OnClick;
};

typedef void ( *TQueryParamsTimerEvent)(unsigned int & Result);

struct TQueryParams
{
  TQueryParams(unsigned int AParams = 0, std::wstring AHelpKeyword = HELP_NONE);

  const TQueryButtonAlias * Aliases;
  unsigned int AliasesCount;
  unsigned int Params;
  unsigned int Timer;
  TQueryParamsTimerEvent TimerEvent;
  std::wstring TimerMessage;
  unsigned int TimerAnswers;
  unsigned int Timeout;
  unsigned int TimeoutAnswer;
  unsigned int NoBatchAnswers;
  std::wstring HelpKeyword;
};

enum TQueryType { qtConfirmation, qtWarning, qtError, qtInformation };

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

bool IsAuthenticationPrompt(TPromptKind Kind);
//---------------------------------------------------------------------------
typedef void ( *TFileFoundEvent)
  (TTerminal * Terminal, const std::wstring FileName, const TRemoteFile * File,
   bool & Cancel);
typedef void ( *TFindingFileEvent)
  (TTerminal * Terminal, const std::wstring Directory, bool & Cancel);
//---------------------------------------------------------------------------
