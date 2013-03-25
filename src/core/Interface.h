//---------------------------------------------------------------------------
#ifndef InterfaceH
#define InterfaceH
//---------------------------------------------------------------------------
#include <CoreDefs.hpp>

#include "Configuration.h"
#include "SessionData.h"
#define HELP_NONE ""
//---------------------------------------------------------------------------
TConfiguration * CreateConfiguration();

void ShowExtendedException(Exception * E);

UnicodeString GetRegistryKey();
void Busy(bool Start);
UnicodeString AppNameString();
UnicodeString SshVersionString();
void CopyToClipboard(const UnicodeString & Text);
HANDLE StartThread(void * SecurityAttributes, unsigned StackSize,
  /* TThreadFunc ThreadFunc, */ void * Parameter, unsigned CreationFlags,
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

struct TQueryButtonAlias : public TObject
{
  TQueryButtonAlias();

  uintptr_t Button;
  UnicodeString Alias;
  TNotifyEvent OnClick;
};

DEFINE_CALLBACK_TYPE1(TQueryParamsTimerEvent, void,
  uintptr_t & /* Result */);

struct TQueryParams : public TObject
{
  explicit TQueryParams(unsigned int AParams = 0, const UnicodeString & AHelpKeyword = HELP_NONE);

  const TQueryButtonAlias * Aliases;
  uintptr_t AliasesCount;
  uintptr_t Params;
  uintptr_t Timer;
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uintptr_t TimerAnswers;
  uintptr_t Timeout;
  uintptr_t TimeoutAnswer;
  uintptr_t NoBatchAnswers;
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

bool IsAuthenticationPrompt(TPromptKind Kind);
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE4(TFileFoundEvent, void,
  TTerminal * /* Terminal */, const UnicodeString & /* FileName */, const TRemoteFile * /* File */,
  bool & /* Cancel */);
DEFINE_CALLBACK_TYPE3(TFindingFileEvent, void,
  TTerminal * /* Terminal */, const UnicodeString & /* Directory */, bool & /* Cancel */);
//---------------------------------------------------------------------------
#endif
