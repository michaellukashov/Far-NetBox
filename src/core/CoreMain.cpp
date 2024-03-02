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

// #pragma package(smart_init)

#if defined(__BORLANDC__)
TConfiguration * Configuration = nullptr;
TStoredSessionList * StoredSessions = nullptr;
#endif // defined(__BORLANDC__)
TApplicationLog * ApplicationLog = nullptr;
bool AnySession = false;

TQueryButtonAlias::TQueryButtonAlias() noexcept :
  OnSubmit(nullptr),
  GroupWith(-1)
{
#if defined(__BORLANDC__)
  OnSubmit = nullptr;
  GroupWith = -1;
  ElevationRequired = false;
  MenuButton = false;
#endif // defined(__BORLANDC__)
}

TQueryButtonAlias TQueryButtonAlias::CreateYesToAllGroupedWithYes()
{
  TQueryButtonAlias Result;
  Result.Button = qaYesToAll;
  Result.GroupWith = qaYes;
  Result.GroupedShiftState = ssShift;
  return Result;
}

TQueryButtonAlias TQueryButtonAlias::CreateNoToAllGroupedWithNo()
{
  TQueryButtonAlias Result;
  Result.Button = qaNoToAll;
  Result.GroupWith = qaNo;
  Result.GroupedShiftState = ssShift;
  return Result;
}

TQueryButtonAlias TQueryButtonAlias::CreateAllAsYesToNewerGroupedWithYes()
{
  TQueryButtonAlias Result;
  Result.Button = qaAll;
  Result.Alias = LoadStr(YES_TO_NEWER_BUTTON);
  Result.GroupWith = qaYes;
  Result.GroupedShiftState = ssCtrl;
  return Result;
}

TQueryButtonAlias TQueryButtonAlias::CreateIgnoreAsRenameGroupedWithNo()
{
  TQueryButtonAlias Result;
  Result.Button = qaIgnore;
  Result.Alias = LoadStr(RENAME_BUTTON);
  Result.GroupWith = qaNo;
  Result.GroupedShiftState = ssCtrl;
  return Result;
}

TQueryParams::TQueryParams(uint32_t AParams, const UnicodeString & AHelpKeyword) noexcept :
  Params(AParams),
  TimerEvent(nullptr),
  TimerMessage(L""),
  TimerQueryType(static_cast<TQueryType>(-1)),
  HelpKeyword(AHelpKeyword)
{
#if defined(__BORLANDC__)
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
  TimeoutResponse = 0;
  NoBatchAnswers = 0;
  HelpKeyword = AHelpKeyword;
#endif // defined(__BORLANDC__)
}

TQueryParams::TQueryParams(const TQueryParams & ASource) noexcept : TQueryParams(0, "")
{
  Assign(ASource);
}

void TQueryParams::Assign(const TQueryParams & ASource)
{
  *this = ASource;
}

TQueryParams & TQueryParams::operator =(const TQueryParams & Other)
{
  Params = Other.Params;
  Aliases = Other.Aliases;
  AliasesCount = Other.AliasesCount;
  Timer = Other.Timer;
  TimerEvent = Other.TimerEvent;
  TimerMessage = Other.TimerMessage;
  TimerAnswers = Other.TimerAnswers;
  TimerQueryType = Other.TimerQueryType;
  Timeout = Other.Timeout;
  TimeoutAnswer = Other.TimeoutAnswer;
  NoBatchAnswers = Other.NoBatchAnswers;
  HelpKeyword = Other.HelpKeyword;
  return *this;
}

bool IsAuthenticationPrompt(TPromptKind Kind)
{
  return
    (Kind == pkUserName) || (Kind == pkPassphrase) || (Kind == pkTIS) ||
    (Kind == pkCryptoCard) || (Kind == pkKeybInteractive) ||
    (Kind == pkPassword) || (Kind == pkNewPassword);
}

bool IsPasswordOrPassphrasePrompt(TPromptKind Kind, const TStrings * Prompts)
{
  return
    (Prompts->GetCount() == 1) && FLAGCLEAR(nb::ToIntPtr(Prompts->Objects[0]), pupEcho) &&
    ((Kind == pkPassword) || (Kind == pkPassphrase) || (Kind == pkKeybInteractive) ||
     (Kind == pkTIS) || (Kind == pkCryptoCard));
}

bool IsPasswordPrompt(TPromptKind Kind, const TStrings * Prompts)
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
    TConfiguration * Conf = GetConfiguration();
    SAFE_DESTROY(Conf);
    ConfigurationDeleted = true;
  }
}

static bool StoredSessionsInitialized = false;

TStoredSessionList * GetStoredSessions(bool * JustLoaded)
{
#define SET_LOADED(Value) do { if (JustLoaded != nullptr) *JustLoaded = Value; } while(0)
  static TStoredSessionList * StoredSessions = nullptr;
  if (StoredSessionsInitialized)
  {
    SET_LOADED(false);
    return StoredSessions;
  }

  StoredSessions = new TStoredSessionList();

  bool SessionList = false;
  std::unique_ptr<THierarchicalStorage> SessionsStorage(GetConfiguration()->CreateScpStorage(SessionList));
  try
  {
    if (SessionsStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false))
    {
      StoredSessions->Load(SessionsStorage.get());
    }
  }
  catch(Exception & E)
  {
    ShowExtendedException(&E);
  }
  StoredSessionsInitialized = true;
  SET_LOADED(true);
#undef SET_LOADED
  return StoredSessions;
}

void DeleteStoredSessions()
{
  if (StoredSessionsInitialized)
  {
    TStoredSessionList * StoredSessions = GetStoredSessions();
    SAFE_DESTROY(StoredSessions);
    StoredSessionsInitialized = false;
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
  catch(Exception & E)
  {
    ShowExtendedException(&E);
  }

  // should be noop, unless exception occurred above
  ConfigStorage->CloseAll();

  // moved to GetStoredSessions()
#if defined(__BORLANDC__)
  StoredSessions = new TStoredSessionList();

  try
  {
    if (SessionsStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false))
    {
      StoredSessions->Load(SessionsStorage.get());
    }
  }
  catch(Exception & E)
  {
    ShowExtendedException(&E);
  }
#endif // defined(__BORLANDC__)
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
  catch(Exception & E)
  {
    ShowExtendedException(&E);
  }

  NeonFinalize();
  TFileZillaIntf::Finalize();
  PuttyFinalize();

  DeleteStoredSessions();
  DeleteConfiguration();
#if defined(__BORLANDC__)
  delete StoredSessions;
  StoredSessions = NULL;
  delete Configuration;
  Configuration = NULL;
#endif // defined(__BORLANDC__)

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
  FUseBusyCursor(UseBusyCursor)
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


TInstantOperationVisualizer::TInstantOperationVisualizer() noexcept : TOperationVisualizer(true),
  FStart(Now())
{
}

TInstantOperationVisualizer::~TInstantOperationVisualizer() noexcept
{
  const TDateTime Time = Now();
  const int64_t Duration = MilliSecondsBetween(Time, FStart);
  constexpr int64_t MinDuration = 250;
  if (Duration < MinDuration)
  {
    Sleep(nb::ToUInt32(MinDuration - Duration));
  }
}

