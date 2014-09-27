//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "CoreMain.h"

#include "Common.h"
#include "Interface.h"
#include "Configuration.h"
#include "PuttyIntf.h"
#include "Cryptography.h"
#include <DateUtils.hpp>
#ifndef NO_FILEZILLA
#include "FileZillaIntf.h"
#endif
#include "WebDAVFileSystem.h"

using namespace Sysutils;
//---------------------------------------------------------------------------
TStoredSessionList * StoredSessions = nullptr;
//---------------------------------------------------------------------------
TConfiguration * GetConfiguration()
{
  static TConfiguration * Configuration = nullptr;
  if (Configuration == nullptr)
  {
    // configuration needs to be created and loaded before putty is initialized,
    // so that random seed path is known
    Configuration = CreateConfiguration();
    try
    {
      Configuration->Load();
    }
    catch (Exception & E)
    {
      ShowExtendedException(&E);
    }

    PuttyInitialize();
  }
  return Configuration;
}
//---------------------------------------------------------------------------
void DeleteConfiguration()
{
  static bool ConfigurationDeleted = false;
  if (!ConfigurationDeleted)
  {
    TConfiguration * Conf = GetConfiguration();
    SAFE_DESTROY(Conf);
    ConfigurationDeleted = true;
  }
}

//---------------------------------------------------------------------------
TQueryButtonAlias::TQueryButtonAlias() :
  Button(0),
  OnClick(nullptr),
  GroupWith(-1),
  Default(false)
{
}
//---------------------------------------------------------------------------
TQueryParams::TQueryParams(uintptr_t AParams, const UnicodeString & AHelpKeyword)
{
  Params = AParams;
  Aliases = nullptr;
  AliasesCount = 0;
  Timer = 0;
  TimerEvent = nullptr;
  TimerMessage = L"";
  TimerAnswers = 0;
  TimerQueryType = static_cast<TQueryType>(-1);
  Timeout = 0;
  TimeoutAnswer = 0;
  NoBatchAnswers = 0;
  HelpKeyword = AHelpKeyword;
}
//---------------------------------------------------------------------------
TQueryParams::TQueryParams(const TQueryParams & Source)
{
  Assign(Source);
}
//---------------------------------------------------------------------------
void TQueryParams::Assign(const TQueryParams & Source)
{
  Params = Source.Params;
  Aliases = Source.Aliases;
  AliasesCount = Source.AliasesCount;
  Timer = Source.Timer;
  TimerEvent = Source.TimerEvent;
  TimerMessage = Source.TimerMessage;
  TimerAnswers = Source.TimerAnswers;
  TimerQueryType = Source.TimerQueryType;
  Timeout = Source.Timeout;
  TimeoutAnswer = Source.TimeoutAnswer;
  NoBatchAnswers = Source.NoBatchAnswers;
  HelpKeyword = Source.HelpKeyword;
}

TQueryParams &TQueryParams::operator=(const TQueryParams & other)
{
  Assign(other);
  return *this;
}
//---------------------------------------------------------------------------
bool IsAuthenticationPrompt(TPromptKind Kind)
{
  return
    (Kind == pkUserName) || (Kind == pkPassphrase) || (Kind == pkTIS) ||
    (Kind == pkCryptoCard) || (Kind == pkKeybInteractive) ||
    (Kind == pkPassword) || (Kind == pkNewPassword);
}
//---------------------------------------------------------------------------
bool IsPasswordOrPassphrasePrompt(TPromptKind Kind, TStrings * Prompts)
{
  return
    (Prompts->GetCount() == 1) && FLAGCLEAR(intptr_t(Prompts->GetObject(0)), pupEcho) &&
    ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
     (Kind == pkTIS) || (Kind == pkCryptoCard));
}
//---------------------------------------------------------------------------
bool IsPasswordPrompt(TPromptKind Kind, TStrings * Prompts)
{
  return
    IsPasswordOrPassphrasePrompt(Kind, Prompts) &&
    (Kind != pkPassphrase);
}
//---------------------------------------------------------------------------
void CoreInitialize()
{
  Randomize();
  CryptographyInitialize();

  assert(GetConfiguration() != nullptr);

  PuttyInitialize();
  #ifndef NO_FILEZILLA
  TFileZillaIntf::Initialize();
  #endif
  NeonInitialize();

  StoredSessions = new TStoredSessionList();

  try
  {
    StoredSessions->Load();
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
  }
}
//---------------------------------------------------------------------------
void CoreFinalize()
{
  try
  {
    GetConfiguration()->Save();
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
  }

  NeonFinalize();
  #ifndef NO_FILEZILLA
  TFileZillaIntf::Finalize();
  #endif
  PuttyFinalize();

  SAFE_DESTROY(StoredSessions);
  DeleteConfiguration();

  CryptographyFinalize();
}
//---------------------------------------------------------------------------
void CoreSetResourceModule(void * ResourceHandle)
{
  #ifndef NO_FILEZILLA
  TFileZillaIntf::SetResourceModule(ResourceHandle);
  #else
  USEDPARAM(ResourceHandle);
  #endif
}
//---------------------------------------------------------------------------
void CoreMaintenanceTask()
{
  DontSaveRandomSeed();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TOperationVisualizer::TOperationVisualizer(bool UseBusyCursor) :
  FUseBusyCursor(UseBusyCursor),
  FToken(nullptr)
{
  if (FUseBusyCursor)
  {
    FToken = BusyStart();
  }
}
//---------------------------------------------------------------------------
TOperationVisualizer::~TOperationVisualizer()
{
  if (FUseBusyCursor)
  {
    BusyEnd(FToken);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TInstantOperationVisualizer::TInstantOperationVisualizer() :
  TOperationVisualizer(true),
  FStart(Now())
{
}
//---------------------------------------------------------------------------
TInstantOperationVisualizer::~TInstantOperationVisualizer()
{
  TDateTime Time = Now();
  int64_t Duration = MilliSecondsBetween(Time, FStart);
  const int64_t MinDuration = 250;
  if (Duration < MinDuration)
  {
    ::Sleep(static_cast<uint32_t>(MinDuration - Duration));
  }
}
//---------------------------------------------------------------------------
