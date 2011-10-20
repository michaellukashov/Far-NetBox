//---------------------------------------------------------------------------
#pragma once

#include "boostdefines.hpp"
#include <boost/signals/signal1.hpp>
#include <boost/signals/signal3.hpp>
#include <boost/signals/signal4.hpp>

#include "Configuration.h"
#include "SessionData.h"
#define HELP_NONE L""
//---------------------------------------------------------------------------
// typedef int (TObject::*TThreadFunc)(void *Parameter);
typedef boost::signal1<int, void *> threadfunc_signal_type;
typedef threadfunc_signal_type::slot_type threadfunc_slot_type;
//---------------------------------------------------------------------------
TConfiguration * CreateConfiguration();

void ShowExtendedException(const std::exception * E);

std::wstring GetRegistryKey();
void Busy(bool Start);
std::wstring AppNameString();
std::wstring SshVersionString();
void CopyToClipboard(std::wstring Text);
int StartThread(void * SecurityAttributes, unsigned StackSize,
  const threadfunc_slot_type &ThreadFunc, void *Parameter, unsigned CreationFlags,
  DWORD &ThreadId);

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
  notify_signal_type OnClick;
};

// typedef void ( *TQueryParamsTimerEvent)(unsigned int & Result);
typedef boost::signal1<void, unsigned int &> queryparamstimer_signal_type;
typedef queryparamstimer_signal_type::slot_type queryparamstimer_slot_type;

struct TQueryParams
{
  TQueryParams(unsigned int AParams = 0, std::wstring AHelpKeyword = HELP_NONE);

  const TQueryButtonAlias * Aliases;
  unsigned int AliasesCount;
  unsigned int Params;
  unsigned int Timer;
  queryparamstimer_slot_type *TimerEvent;
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
// typedef void ( *TFileFoundEvent)
  // (TTerminal * Terminal, const std::wstring FileName, const TRemoteFile * File,
   // bool & Cancel);
typedef boost::signal4<void, TTerminal *, const std::wstring, const TRemoteFile *, bool &> filefound_signal_type;
typedef filefound_signal_type::slot_type filefound_slot_type;
// typedef void ( *TFindingFileEvent)
  // (TTerminal * Terminal, const std::wstring Directory, bool & Cancel);
typedef boost::signal3<void, TTerminal *, const std::wstring, bool &> findingfile_signal_type;
typedef findingfile_signal_type::slot_type findingfile_slot_type;
//---------------------------------------------------------------------------
