#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "CoreMain.h"
#include "Interface.h"
#include "Configuration.h"
#include "PuttyIntf.h"
#include "Cryptography.h"
#include <DateUtils.hpp>
#include "FileZillaIntf.h"
#include "NeonIntf.h"
#include "TextsCore.h"
// #include "WebDAVFileSystem.h"

__removed #pragma package(smart_init)

__removed TConfiguration * Configuration = nullptr;
TStoredSessionList *StoredSessions = nullptr;
TApplicationLog * ApplicationLog = nullptr;
bool AnySession = false;

TQueryButtonAlias::TQueryButtonAlias() noexcept :
  Button(0),
  OnSubmit(nullptr),
  GroupWith(-1),
  Default(false),
  GrouppedShiftState(ssShift),
  ElevationRequired(false)
{
  MenuButton = false;
}

TQueryButtonAlias TQueryButtonAlias::CreateYesToAllGrouppedWithYes()
{
  TQueryButtonAlias Result;
  Result.Button = qaYesToAll;
  Result.GroupWith = qaYes;
  Result.GrouppedShiftState = ssShift;
  return Result;
}

TQueryButtonAlias TQueryButtonAlias::CreateNoToAllGrouppedWithNo()
{
  TQueryButtonAlias Result;
  Result.Button = qaNoToAll;
  Result.GroupWith = qaNo;
  Result.GrouppedShiftState = ssShift;
  return Result;
}

TQueryButtonAlias TQueryButtonAlias::CreateAllAsYesToNewerGrouppedWithYes()
{
  TQueryButtonAlias Result;
  Result.Button = qaAll;
  Result.Alias = LoadStr(YES_TO_NEWER_BUTTON);
  Result.GroupWith = qaYes;
  Result.GrouppedShiftState = ssCtrl;
  return Result;
}

TQueryButtonAlias TQueryButtonAlias::CreateIgnoreAsRenameGrouppedWithNo()
{
  TQueryButtonAlias Result;
  Result.Button = qaIgnore;
  Result.Alias = LoadStr(RENAME_BUTTON);
  Result.GroupWith = qaNo;
  Result.GrouppedShiftState = ssCtrl;
  return Result;
}

TQueryParams::TQueryParams(uint32_t AParams, const UnicodeString AHelpKeyword) noexcept :
  Aliases(nullptr),
  AliasesCount(0),
  Params(AParams),
  Timer(0),
  TimerEvent(nullptr),
  TimerMessage(L""),
  TimerAnswers(0),
  TimerQueryType(static_cast<TQueryType>(-1)),
  Timeout(0),
  TimeoutAnswer(0),
  NoBatchAnswers(0),
  HelpKeyword(AHelpKeyword)
{
  TimeoutResponse = 0;
}

TQueryParams::TQueryParams(const TQueryParams & Source) noexcept
{
  Assign(Source);
}

void TQueryParams::Assign(const TQueryParams & Source)
{
  *this = Source;
}

TQueryParams &TQueryParams::operator=(const TQueryParams &other)
{
  Params = other.Params;
  Aliases = other.Aliases;
  AliasesCount = other.AliasesCount;
  Timer = other.Timer;
  TimerEvent = other.TimerEvent;
  TimerMessage = other.TimerMessage;
  TimerAnswers = other.TimerAnswers;
  TimerQueryType = other.TimerQueryType;
  Timeout = other.Timeout;
  TimeoutAnswer = other.TimeoutAnswer;
  NoBatchAnswers = other.NoBatchAnswers;
  HelpKeyword = other.HelpKeyword;
  return *this;
}

bool IsAuthenticationPrompt(TPromptKind Kind)
{
  return
    (Kind == pkUserName) || (Kind == pkPassphrase) || (Kind == pkTIS) ||
    (Kind == pkCryptoCard) || (Kind == pkKeybInteractive) ||
    (Kind == pkPassword) || (Kind == pkNewPassword);
}

bool IsPasswordOrPassphrasePrompt(TPromptKind Kind, TStrings * Prompts)
{
  return
    (Prompts->GetCount() == 1) && FLAGCLEAR(nb::ToIntPtr(Prompts->GetObj(0)), pupEcho) &&
    ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
     (Kind == pkTIS) || (Kind == pkCryptoCard));
}

bool IsPasswordPrompt(TPromptKind Kind, TStrings * Prompts)
{
  return
    IsPasswordOrPassphrasePrompt(Kind, Prompts) &&
    (Kind != pkPassphrase);
}

TConfiguration * GetConfiguration()
{
  static TConfiguration * Configuration = nullptr;
  if (Configuration == nullptr)
  {
    Configuration = CreateConfiguration();
  }
  return Configuration;
}

void DeleteConfiguration()
{
  static bool ConfigurationDeleted = false;
  if (!ConfigurationDeleted)
  {
    TConfiguration *Conf = GetConfiguration();
    SAFE_DESTROY(Conf);
    ConfigurationDeleted = true;
  }
}

void CoreLoad()
{
  bool SessionList = false;
  std::unique_ptr<THierarchicalStorage> SessionsStorage(GetConfiguration()->CreateScpStorage(SessionList));
  THierarchicalStorage * ConfigStorage{nullptr};
  std::unique_ptr<THierarchicalStorage> ConfigStorageAuto;
  if (!SessionList)
  {
    // can reuse this for configuration
    ConfigStorage = SessionsStorage.get();
  }
  else
  {
    ConfigStorageAuto.reset(GetConfiguration()->CreateConfigStorage());
    ConfigStorage = ConfigStorageAuto.get();
  }

  DebugAssert(GetConfiguration() != nullptr);

  try
  {
    GetConfiguration()->Load(ConfigStorage);
  }
  catch (Exception &E)
  {
    ShowExtendedException(&E);
  }

  // should be noop, unless exception occured above
  ConfigStorage->CloseAll();

  StoredSessions = new TStoredSessionList();

  try
  {
    if (SessionsStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false))
    {
      StoredSessions->Load(SessionsStorage.get());
    }
  }
  catch (Exception &E)
  {
    ShowExtendedException(&E);
  }
}

void CoreInitialize()
{
  WinInitialize();
  Randomize();
  CryptographyInitialize();

  // we do not expect configuration re-creation
  DebugAssert(GetConfiguration() != nullptr);
  // configuration needs to be created and loaded before putty is initialized,
  // so that random seed path is known
//  Configuration = CreateConfiguration();

  PuttyInitialize();
  TFileZillaIntf::Initialize();
  NeonInitialize();

  CoreLoad();
}

void CoreFinalize()
{
  try
  {
    // GetConfiguration()->Save();
  }
  catch(Exception &E)
  {
    ShowExtendedException(&E);
  }

  NeonFinalize();
  TFileZillaIntf::Finalize();
  PuttyFinalize();

  SAFE_DESTROY(StoredSessions);
  DeleteConfiguration();

  CryptographyFinalize();
  WinFinalize();
}

void CoreSetResourceModule(void * ResourceHandle)
{
  TFileZillaIntf::SetResourceModule(ResourceHandle);
}

void CoreMaintenanceTask()
{
  DontSaveRandomSeed();
}

void CoreUpdateFinalStaticUsage()
{
  if (!AnySession)
  {
    GetConfiguration()->Usage->Inc(L"RunsWithoutSession");
  }
}


TOperationVisualizer::TOperationVisualizer(bool UseBusyCursor) noexcept :
  FUseBusyCursor(UseBusyCursor),
  FToken(nullptr)
{
  if (FUseBusyCursor)
  {
    FToken = BusyStart();
  }
}

TOperationVisualizer::~TOperationVisualizer() noexcept
{
  if (FUseBusyCursor)
  {
    BusyEnd(FToken);
  }
}


TInstantOperationVisualizer::TInstantOperationVisualizer() noexcept :
  TOperationVisualizer(true),
  FStart(Now())
{
}

TInstantOperationVisualizer::~TInstantOperationVisualizer() noexcept
{
  TDateTime Time = Now();
  int64_t Duration = MilliSecondsBetween(Time, FStart);
  const int64_t MinDuration = 250;
  if (Duration < MinDuration)
  {
    Sleep(static_cast<uint32_t>(MinDuration - Duration));
  }
}

