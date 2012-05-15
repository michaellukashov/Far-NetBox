//---------------------------------------------------------------------------
#ifndef InterfaceH
#define InterfaceH
//---------------------------------------------------------------------------
#include "Configuration.h"
#include "SessionData.h"
#define HELP_NONE L""

#ifdef _MSC_VER
#include "boostdefines.hpp"
#include <boost/signals/signal1.hpp>
#include <boost/signals/signal3.hpp>
#include <boost/signals/signal4.hpp>

#include "Exceptions.h"
#endif
//---------------------------------------------------------------------------
TConfiguration * __fastcall CreateConfiguration();

void __fastcall ShowExtendedException(Exception * E);

UnicodeString __fastcall GetRegistryKey();
void __fastcall Busy(bool Start);
UnicodeString __fastcall AppNameString();
UnicodeString __fastcall SshVersionString();
void __fastcall CopyToClipboard(UnicodeString Text);
int __fastcall StartThread(void * SecurityAttributes, unsigned int StackSize,
  /* TThreadFunc ThreadFunc, */ void * Parameter, unsigned int CreationFlags,
  TThreadID & ThreadId);

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
const int qpIgnoreAbort =          0x08;

struct TQueryButtonAlias
{
  TQueryButtonAlias();

  unsigned int Button;
  UnicodeString Alias;
  TNotifySignal OnClick;
};

#ifndef _MSC_VER
typedef void __fastcall (__closure *TQueryParamsTimerEvent)(unsigned int & Result);
#else
typedef boost::signal1<void, unsigned int & /* Result */> TQueryParamsTimerSignal;
typedef TQueryParamsTimerSignal::slot_type TQueryParamsTimerEvent;
#endif

struct TQueryParams
{
  explicit TQueryParams(unsigned int AParams = 0, UnicodeString AHelpKeyword = HELP_NONE);

  const TQueryButtonAlias * Aliases;
  unsigned int AliasesCount;
  unsigned int Params;
  unsigned int Timer;
  TQueryParamsTimerEvent * TimerEvent;
  UnicodeString TimerMessage;
  unsigned int TimerAnswers;
  unsigned int Timeout;
  unsigned int TimeoutAnswer;
  unsigned int NoBatchAnswers;
  UnicodeString HelpKeyword;
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

bool __fastcall IsAuthenticationPrompt(TPromptKind Kind);
//---------------------------------------------------------------------------
#ifndef _MSC_VER
typedef void __fastcall (__closure *TFileFoundEvent)
  (TTerminal * Terminal, const UnicodeString FileName, const TRemoteFile * File,
   bool & Cancel);
typedef void __fastcall (__closure *TFindingFileEvent)
  (TTerminal * Terminal, const UnicodeString Directory, bool & Cancel);
#else
typedef boost::signal4<void, TTerminal * /* Terminal */, const UnicodeString /* FileName */, const TRemoteFile * /* File */,
   bool & /* Cancel */> TFileFoundSignal;
typedef TFileFoundSignal::slot_type TFileFoundEvent;
typedef boost::signal3<void, TTerminal * /* Terminal */, const UnicodeString /* Directory */, bool & /* Cancel */> TFindingFileSignal;
typedef TFindingFileSignal::slot_type TFindingFileEvent;
#endif
//---------------------------------------------------------------------------
#endif
