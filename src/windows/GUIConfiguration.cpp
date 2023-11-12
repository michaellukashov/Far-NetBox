
#include <vcl.h>
#pragma hdrstop

#include <System.ShlObj.hpp>

#include <Common.h>
#include "GUIConfiguration.h"
#include "GUITools.h"
#include <FileInfo.h>
#include <TextsCore.h>
#include <TextsWin.h>
#include <Terminal.h>
#include <CoreMain.h>
#include <System.ShlObj.hpp>

__removed #pragma package(smart_init)

const int32_t ccLocal = ccUser;
const int32_t ccShowResults = ccUser << 1;
const int32_t ccCopyResults = ccUser << 2;
const int32_t ccRemoteFiles = ccUser << 3;
const int32_t ccShowResultsInMsgBox = ccUser << 4;
const int32_t ccSet = 0x80000000;

constexpr const uint32_t AdditionaLanguageMask = 0xFFFFFF00;
static UnicodeString AdditionaLanguagePrefix("XX");
static UnicodeString TranslationsSubFolder("Translations");

__removed TGUIConfiguration * GUIConfiguration = nullptr;

TGUICopyParamType::TGUICopyParamType() noexcept
  : TCopyParamType(OBJECT_CLASS_TGUICopyParamType)
{
  GUIDefault();
}

TGUICopyParamType::TGUICopyParamType(const TCopyParamType & Source) noexcept
  : TCopyParamType(Source)
{
  GUIDefault();
}

TGUICopyParamType::TGUICopyParamType(const TGUICopyParamType & Source) noexcept
  : TCopyParamType(Source)
{
  GUIAssign(&Source);
}

void TGUICopyParamType::Assign(const TCopyParamType * Source)
{
  TCopyParamType::Assign(Source);

  const TGUICopyParamType * GUISource = dyn_cast<TGUICopyParamType>(Source);
  if (GUISource != nullptr)
  {
    GUIAssign(GUISource);
  }
}

void TGUICopyParamType::GUIAssign(const TGUICopyParamType * Source)
{
  SetQueue(Source->GetQueue());
  SetQueueNoConfirmation(Source->GetQueueNoConfirmation());
  SetQueueParallel(Source->GetQueueParallel());
}

void TGUICopyParamType::Default()
{
  TCopyParamType::Default();

  GUIDefault();
}

void TGUICopyParamType::GUIDefault()
{
  TCopyParamType::Default();

  SetQueue(false);
  SetQueueNoConfirmation(true);
  SetQueueParallel(true);
}

void TGUICopyParamType::Load(THierarchicalStorage * Storage)
{
  TCopyParamType::Load(Storage);

  SetQueue(Storage->ReadBool("Queue", GetQueue()));
  SetQueueNoConfirmation(Storage->ReadBool("QueueNoConfirmation", GetQueueNoConfirmation()));
  SetQueueParallel(Storage->ReadBool("QueueParallel", GetQueueParallel()));
}

void TGUICopyParamType::Save(THierarchicalStorage * Storage, const TCopyParamType * Defaults) const
{
  DebugAssert(Defaults == nullptr);
  TCopyParamType::Save(Storage, Defaults);

  Storage->WriteBool("Queue", GetQueue());
  Storage->WriteBool("QueueNoConfirmation", GetQueueNoConfirmation());
  Storage->WriteBool("QueueQueueParallel", GetQueueParallel());
}

TGUICopyParamType & TGUICopyParamType::operator =(const TCopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}

TGUICopyParamType & TGUICopyParamType::operator =(const TGUICopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}


void TCopyParamRuleData::Default()
{
  HostName.Clear();
  UserName.Clear();
  RemoteDirectory.Clear();
  LocalDirectory.Clear();
}


TCopyParamRule::TCopyParamRule() noexcept :
  TObject(OBJECT_CLASS_TCopyParamRule)
{
}

TCopyParamRule::TCopyParamRule(const TCopyParamRuleData & Data) noexcept :
  TObject(OBJECT_CLASS_TCopyParamRule),
  FData(Data)
{
  __removed FData = Data;
}

TCopyParamRule::TCopyParamRule(const TCopyParamRule & Source) noexcept :
  TObject(OBJECT_CLASS_TCopyParamRule)
{
  FData.HostName = Source.FData.HostName;
  FData.UserName = Source.FData.UserName;
  FData.RemoteDirectory = Source.FData.RemoteDirectory;
  FData.LocalDirectory = Source.FData.LocalDirectory;
}

#undef C
#define C(Property) (Property == rhp.Property)
bool TCopyParamRule::operator==(const TCopyParamRule & rhp) const
{
  return
    C(FData.HostName) &&
    C(FData.UserName) &&
    C(FData.RemoteDirectory) &&
    C(FData.LocalDirectory) &&
    true;
}
#undef C

bool TCopyParamRule::Match(const UnicodeString & Mask,
  const UnicodeString & Value, bool Path, bool Local, int32_t ForceDirectoryMasks) const
{
  bool Result;
  if (Mask.IsEmpty())
  {
    Result = true;
  }
  else
  {
    TFileMasks M(ForceDirectoryMasks);
    M.Masks(Mask);
    if (Path)
    {
      Result = M.Matches(Value, Local, true);
    }
    else
    {
      Result = M.MatchesFileName(Value);
    }
  }
  return Result;
}

bool TCopyParamRule::Matches(const TCopyParamRuleData & Value) const
{
  return
    Match(FData.HostName, Value.HostName, false, true, 0) &&
    Match(FData.UserName, Value.UserName, false, true, 0) &&
    Match(FData.RemoteDirectory, Value.RemoteDirectory, true, false, 1) &&
    Match(FData.LocalDirectory, Value.LocalDirectory, true, true, 1);
}

void TCopyParamRule::Load(THierarchicalStorage * Storage)
{
  FData.HostName = Storage->ReadString("HostName", FData.HostName);
  FData.UserName = Storage->ReadString("UserName", FData.UserName);
  FData.RemoteDirectory = Storage->ReadString("RemoteDirectory", FData.RemoteDirectory);
  FData.LocalDirectory = Storage->ReadString("LocalDirectory", FData.LocalDirectory);
}

void TCopyParamRule::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteString("HostName", FData.HostName);
  Storage->WriteString("UserName", FData.UserName);
  Storage->WriteString("RemoteDirectory", FData.RemoteDirectory);
  Storage->WriteString("LocalDirectory", FData.LocalDirectory);
}

bool TCopyParamRule::GetEmpty() const
{
  return
    FData.HostName.IsEmpty() &&
    FData.UserName.IsEmpty() &&
    FData.RemoteDirectory.IsEmpty() &&
    FData.LocalDirectory.IsEmpty();
}

TCopyParamRule &TCopyParamRule::operator=(const TCopyParamRule & other)
{
  SetData(other.FData);
  return *this;
}


UnicodeString TCopyParamRule::GetInfoStr(const UnicodeString & Separator) const
{
  UnicodeString Result;
#define ADD(FMT, ELEM) \
    if (!FData.ELEM.IsEmpty()) \
      Result += (Result.IsEmpty() ? UnicodeString() : Separator) + FMTLOAD(FMT, FData.ELEM);
  ADD(COPY_RULE_HOSTNAME, HostName);
  ADD(COPY_RULE_USERNAME, UserName);
  ADD(COPY_RULE_REMOTE_DIR, RemoteDirectory);
  ADD(COPY_RULE_LOCAL_DIR, LocalDirectory);
#undef ADD
  return Result;
}


UnicodeString TCopyParamList::FInvalidChars(L"/\\[]");

TCopyParamList::TCopyParamList() noexcept
{
  Init();
}

void TCopyParamList::Init()
{
  FCopyParams = std::make_unique<TList>();
  FRules = std::make_unique<TList>();
  FNames = std::make_unique<TStringList>();
  FNameList = nullptr;
  FModified = false;
}

TCopyParamList::~TCopyParamList() noexcept
{
  Clear();
  __removed delete FCopyParams;
  __removed delete FRules;
  __removed delete FNames;
  __removed delete FNameList;
}

void TCopyParamList::Reset()
{
  FNameList.reset();
  FModified = false;
}

void TCopyParamList::Modify()
{
  FNameList.reset();
  FModified = true;
}

void TCopyParamList::ValidateName(const UnicodeString & Name)
{
  if (Name.LastDelimiter(FInvalidChars) > 0)
  {
    throw Exception(FMTLOAD(ITEM_NAME_INVALID, Name, FInvalidChars));
  }
}

TCopyParamList & TCopyParamList::operator=(const TCopyParamList & rhl)
{
  if (this == &rhl)
    return *this;
  Clear();

  for (int32_t Index = 0; Index < rhl.GetCount(); ++Index)
  {
    TCopyParamType *CopyParam = new TCopyParamType(*rhl.GetCopyParam(Index));
    TCopyParamRule *Rule = nullptr;
    if (rhl.GetRule(Index) != nullptr)
    {
      Rule = new TCopyParamRule(*rhl.GetRule(Index));
    }
    Add(rhl.GetName(Index), CopyParam, Rule);
  }
  // there should be comparison of with the assigned list, but we rely on caller
  // to do it instead (TGUIConfiguration::SetCopyParamList)
  Modify();
  return *this;
}

bool TCopyParamList::operator==(const TCopyParamList & rhl) const
{
  bool Result = (GetCount() == rhl.GetCount());
  if (Result)
  {
    int32_t Index = 0;
    while ((Index < GetCount()) && Result)
    {
      Result =
       (GetName(Index) == rhl.GetName(Index)) &&
        CompareItem(Index, rhl.GetCopyParam(Index), rhl.GetRule(Index));
      ++Index;
    }
  }
  return Result;
}

int32_t TCopyParamList::IndexOfName(const UnicodeString & Name) const
{
  return FNames->IndexOf(Name);
}

bool TCopyParamList::CompareItem(int32_t Index,
  const TCopyParamType * CopyParam, const TCopyParamRule * Rule) const
{
  return
    ((*GetCopyParam(Index)) == *CopyParam) &&
    ((GetRule(Index) == nullptr) ?
      (Rule == nullptr) :
      ((Rule != nullptr) && (*GetRule(Index)) == (*Rule)));
}

void TCopyParamList::Clear()
{
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    delete GetCopyParam(Index);
    delete GetRule(Index);
  }
  FCopyParams->Clear();
  FRules->Clear();
  FNames->Clear();
}

void TCopyParamList::Add(const UnicodeString & Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  Insert(GetCount(), Name, CopyParam, Rule);
}

void TCopyParamList::Insert(int32_t Index, const UnicodeString & Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  DebugAssert(FNames->IndexOf(Name) < 0);
  FNames->Insert(Index, Name);
  DebugAssert(CopyParam != nullptr);
  FCopyParams->Insert(Index, reinterpret_cast<TObject *>(CopyParam));
  FRules->Insert(Index, reinterpret_cast<TObject *>(Rule));
  Modify();
}

void TCopyParamList::Change(int32_t Index, const UnicodeString & Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  if ((Name != GetName(Index)) || !CompareItem(Index, CopyParam, Rule))
  {
    FNames->SetString(Index, Name);
    delete GetCopyParam(Index);
    FCopyParams->SetItem(Index, CopyParam);
    delete GetRule(Index);
    FRules->SetItem(Index, Rule);
    Modify();
  }
  else
  {
    SAFE_DESTROY(CopyParam);
    SAFE_DESTROY(Rule);
  }
}

void TCopyParamList::Move(int32_t CurIndex, int32_t NewIndex)
{
  if (CurIndex != NewIndex)
  {
    FNames->Move(CurIndex, NewIndex);
    FCopyParams->Move(CurIndex, NewIndex);
    FRules->Move(CurIndex, NewIndex);
    Modify();
  }
}

void TCopyParamList::Delete(int32_t Index)
{
  DebugAssert((Index >= 0) && (Index < GetCount()));
  FNames->Delete(Index);
  delete GetCopyParam(Index);
  FCopyParams->Delete(Index);
  delete GetRule(Index);
  FRules->Delete(Index);
  Modify();
}

int32_t TCopyParamList::Find(const TCopyParamRuleData & Value) const
{
  int32_t Result = -1;
  int32_t Index = 0;
  while ((Index < FRules->GetCount()) && (Result < 0))
  {
    if (FRules->GetItem(Index) != nullptr)
    {
      if (GetRule(Index)->Matches(Value))
      {
        Result = Index;
      }
    }
    ++Index;
  }
  return Result;
}

void TCopyParamList::Load(THierarchicalStorage * Storage, int32_t ACount)
{
  for (int32_t Index = 0; Index < ACount; ++Index)
  {
    UnicodeString Name = ::IntToStr(Index);
    std::unique_ptr<TCopyParamRule> Rule;
    std::unique_ptr<TCopyParamType> CopyParam(std::make_unique<TCopyParamType>());
    try__removed
    {
      if (Storage->OpenSubKey(Name, false))
      {
        try__finally
        {
          Name = Storage->ReadString("Name", Name);
          CopyParam->Load(Storage);

          if (Storage->ReadBool("HasRule", false))
          {
            Rule = std::make_unique<TCopyParamRule>();
            Rule->Load(Storage);
          }
        },
        __finally
        {
          Storage->CloseSubKey();
        } end_try__finally
      }
    }
    catch__removed
    ({
      delete CopyParam;
      delete Rule;
      throw;
    })

    FCopyParams->Add(CopyParam.release());
    FRules->Add(Rule.release());
    FNames->Add(Name);
  }
  Reset();
}

void TCopyParamList::Save(THierarchicalStorage * Storage) const
{
  Storage->ClearSubKeys();
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    if (Storage->OpenSubKey(::IntToStr(Index), /*CanCreate*/ true))
    {
      try__finally
      {
        const TCopyParamType *CopyParam = GetCopyParam(Index);
        const TCopyParamRule *Rule = GetRule(Index);

        Storage->WriteString("Name", GetName(Index));
        CopyParam->Save(Storage);
        Storage->WriteBool("HasRule", (Rule != nullptr));
        if (Rule != nullptr)
        {
          Rule->Save(Storage);
        }
      },
      __finally
      {
        Storage->CloseSubKey();
      } end_try__finally
    }
  }
}

int32_t TCopyParamList::GetCount() const
{
  return FCopyParams ? FCopyParams->GetCount() : 0;
}

const TCopyParamRule * TCopyParamList::GetRule(int32_t Index) const
{
  return FRules->GetAs<TCopyParamRule>(Index);
}

const TCopyParamType * TCopyParamList::GetCopyParam(int32_t Index) const
{
  return FCopyParams->GetAs<TCopyParamType>(Index);
}

UnicodeString TCopyParamList::GetName(int32_t Index) const
{
  return FNames->GetString(Index);
}

TStrings * TCopyParamList::GetNameList() const
{
  if (FNameList == nullptr)
  {
    FNameList = std::make_unique<TStringList>();

    for (int32_t Index = 0; Index < GetCount(); ++Index)
    {
      FNameList->Add(FNames->GetString(Index));
    }
  }
  return FNameList.get();
}

bool TCopyParamList::GetAnyRule() const
{
  bool Result = false;
  int32_t Index = 0;
  while ((Index < GetCount()) && !Result)
  {
    Result = (GetRule(Index) != nullptr);
    ++Index;
  }
  return Result;
}


TGUIConfiguration::TGUIConfiguration(TObjectClassId Kind) noexcept : TConfiguration(Kind)
{
  FLocale = 0;
  __removed SetAppliedLocale(InternalLocale(), UnicodeString());
  FLocales = std::make_unique<TObjectList>();
  FLastLocalesExts = L"*";
  FCopyParamList = std::make_unique<TCopyParamList>();
  __removed CoreSetResourceModule(GetResourceModule());
}

TGUIConfiguration::~TGUIConfiguration() noexcept
{
  __removed delete FLocales;
  __removed delete FCopyParamList;
}

void TGUIConfiguration::ConfigurationInit()
{
  SetAppliedLocale(InternalLocale(), UnicodeString());
  CoreSetResourceModule(GetResourceModule());
  TConfiguration::ConfigurationInit();
}

void TGUIConfiguration::Default()
{
  TConfiguration::Default();

  // reset before call to DefaultLocalized()
  FDefaultCopyParam.Default();

  FCopyParamListDefaults = true;
  DefaultLocalized();

  FIgnoreCancelBeforeFinish = TDateTime(0, 0, 3, 0);
  FContinueOnError = false;
  FConfirmCommandSession = true;
  FSynchronizeParams = TTerminal::spDefault;
  FSynchronizeModeAuto = -1;
  FSynchronizeMode = TTerminal::smRemote;
  FMaxWatchDirectories = 500;
  FSynchronizeOptions = soRecurse | soSynchronizeAsk;
  FQueueBootstrap = false;
  FQueueKeepDoneItems = true;
  FQueueKeepDoneItemsFor = 15;
  FQueueAutoPopup = true;
  FSessionRememberPassword = true;
  UnicodeString ProgramsFolder;
#if defined(_MSC_VER)
  SpecialFolderLocation(CSIDL_PROGRAM_FILES, ProgramsFolder);
#endif // if defined(_MSC_VER)
  FDefaultPuttyPathOnly = IncludeTrailingBackslash(ProgramsFolder) + L"PuTTY\\" + OriginalPuttyExecutable;
  FDefaultPuttyPath = L"%ProgramFiles%\\PuTTY\\" + OriginalPuttyExecutable;
  FPuttyPath = FormatCommand(FDefaultPuttyPath, L"");
  FUsePuttyPwFile = asAuto;
  FPuttyPassword = false;
  FTelnetForFtpInPutty = true;
  FPuttySession = "WinSCP temporary session";
  FBeepOnFinish = false;
  FBeepOnFinishAfter = TDateTime(0, 0, 30, 0);
  FBeepSound = L"SystemDefault";
  FCopyParamCurrent = L"";
  FKeepUpToDateChangeDelay = 500;
  FChecksumAlg = L"sha1";
  FSessionReopenAutoIdle = 9000;

  FNewDirectoryProperties.Default();
  FNewDirectoryProperties.Rights = TRights::rfDefault | TRights::rfExec;
}

void TGUIConfiguration::DefaultLocalized()
{
  if (FCopyParamListDefaults)
  {
    FCopyParamList->Clear();

    // guard against "empty resource string" from obsolete translations
    // (DefaultLocalized is called for the first time before detection of
    // obsolete translations)
    if (!LoadStr(COPY_PARAM_PRESET_ASCII).IsEmpty())
    {
      TCopyParamType * CopyParam{nullptr};

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmAscii);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_ASCII), CopyParam, nullptr);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmBinary);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_BINARY), CopyParam, nullptr);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetNewerOnly(true);
      FCopyParamList->Add(LoadStr(COPY_PARAM_NEWER_ONLY), CopyParam, nullptr);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->IncludeFileMask = TFileMasks(FORMAT("%s */", IncludeExcludeFileMasksDelimiter));
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_EXCLUDE_ALL_DIR), CopyParam, nullptr);
    }

    FCopyParamList->Reset();
  }
}

void TGUIConfiguration::UpdateStaticUsage()
{
  TConfiguration::UpdateStaticUsage();
  Usage->Set("CopyParamsCount", (int32_t)(FCopyParamListDefaults ? 0 : FCopyParamList->Count));
  Usage->Set("Putty", ExtractProgramName(PuttyPath));
}

// duplicated from core\configuration.cpp
#undef BLOCK
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKeyPath(KEY, CANCREATE)) \
    { SCOPE_EXIT { Storage->CloseSubKeyPath(); }; { BLOCK } }
#undef REGCONFIG

#define REGCONFIG(CANCREATE) \
  BLOCK("Interface", CANCREATE, \
    KEY(Bool,     ContinueOnError); \
    KEY(Bool,     ConfirmCommandSession); \
    KEY3(Integer,  SynchronizeParams); \
    KEY3(Integer,  SynchronizeOptions); \
    KEY3(Integer,  SynchronizeModeAuto); \
    KEY3(Integer,  SynchronizeMode); \
    KEY3(Integer,  MaxWatchDirectories); \
    KEY(Bool,     QueueBootstrap); \
    KEY(Bool,     QueueKeepDoneItems); \
    KEY3(Integer,  QueueKeepDoneItemsFor); \
    KEY(Bool,     QueueAutoPopup); \
    KEYEX2(Bool,   SessionRememberPassword, SessionRememberPassword); \
    KEY(String,   PuttySession); \
    KEY(String,   PuttyPath); \
    KEY(Bool,     PuttyPassword); \
    KEY(Bool,     TelnetForFtpInPutty); \
    KEY(DateTime, IgnoreCancelBeforeFinish); \
    KEY(Bool,     BeepOnFinish); \
    KEY(DateTime, BeepOnFinishAfter); \
    KEY3(Integer,  KeepUpToDateChangeDelay); \
    KEY(String,   ChecksumAlg); \
    KEY3(Integer,  SessionReopenAutoIdle); \
  ); \

    __removed KEY3(Integer, UsePuttyPwFile);
    __removed KEY(String,   BeepSound);

bool TGUIConfiguration::DoSaveCopyParam(THierarchicalStorage * Storage, const TCopyParamType * CopyParam, const TCopyParamType * Defaults)
{
  bool Result = Storage->OpenSubKeyPath(L"Interface\\CopyParam", true);
  if (Result)
  {
    CopyParam->Save(Storage, Defaults);
  }
  return Result;
}

void TGUIConfiguration::SaveData(THierarchicalStorage * Storage, bool All)
{
  TConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
#undef LASTELEM
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>") + 1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
#undef KEYEX
#undef KEYEX2
#define KEYEX(TYPE, NAME, VAR) Storage->Write ## TYPE(LASTELEM(UnicodeString(#NAME)), Get ## VAR())
#define KEYEX2(TYPE, NAME, VAR) Storage->Write ## TYPE(LASTELEM(UnicodeString(#NAME)), VAR)
#undef KEY
#undef KEY2
#undef KEY3
#undef KEY4
#define KEY(TYPE, NAME) Storage->Write ## TYPE(PropertyToKey(#NAME), Get ## NAME())
#define KEY2(TYPE, NAME) Storage->Write ## TYPE(PropertyToKey(#NAME), NAME)
#define KEY3(TYPE, NAME) Storage->Write ## TYPE(PropertyToKey(#NAME), nb::ToInt(Get ## NAME()))
#define KEY4(TYPE, NAME) Storage->Write ## TYPE(PropertyToKey(#NAME), nb::ToInt(NAME))
  REGCONFIG(true);
#undef KEY4
#undef KEY3
#undef KEY2
#undef KEY
#undef KEYEX2
#undef KEYEX

  if (DoSaveCopyParam(Storage, &FDefaultCopyParam, nullptr))
  try__finally
  {
    FDefaultCopyParam.Save(Storage);

    if (FCopyParamListDefaults)
    {
      DebugAssert(!FCopyParamList->GetModified());
      Storage->WriteInteger("CopyParamList", -1);
    }
    else if (All || FCopyParamList->GetModified())
    {
      Storage->WriteInteger("CopyParamList", nb::ToInt32(FCopyParamList->GetCount()));
      FCopyParamList->Save(Storage);
    }
  },
  __finally
  {
    Storage->CloseSubKeyPath();
  } end_try__finally

  if (Storage->OpenSubKeyPath("Interface\\NewDirectory2", true))
  try__finally
  {
    FNewDirectoryProperties.Save(Storage);
  },
  __finally
  {
    Storage->CloseSubKeyPath();
  } end_try__finally
}

bool TGUIConfiguration::LoadCopyParam(THierarchicalStorage * Storage, TCopyParamType * CopyParam)
{
  bool Result =
    Storage->OpenSubKeyPath(L"Interface\\CopyParam", false);
  if (Result)
  {
    try
    {
      CopyParam->Load(Storage);
    }
    catch (...)
    {
      Storage->CloseSubKeyPath();
      throw;
    }
  }
  return Result;
}

void TGUIConfiguration::LoadDefaultCopyParam(THierarchicalStorage * Storage)
{
  FDefaultCopyParam.Load(Storage);
}

void TGUIConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
#undef KEYEX
#undef KEYEX2
#define KEYEX(TYPE, NAME, VAR) Set ## VAR(Storage->Read ## TYPE(LASTELEM(UnicodeString(#NAME)), Get ## VAR()))
#define KEYEX2(TYPE, NAME, VAR) VAR = Storage->Read ## TYPE(LASTELEM(UnicodeString(#NAME)), VAR)
#undef KEY
#undef KEY2
#undef KEY3
#undef KEY4
#define KEY(TYPE, NAME) Set ## NAME(Storage->Read ## TYPE(PropertyToKey(#NAME), Get ## NAME()))
#define KEY2(TYPE, NAME) NAME = Storage->Read ## TYPE(PropertyToKey(#NAME), NAME)
#define KEY3(TYPE, NAME) Set ## NAME(Storage->Read ## TYPE(PropertyToKey(#NAME), nb::ToInt(Get ## NAME())))
#define KEY4(TYPE, NAME) NAME = Storage->Read ## TYPE(PropertyToKey(#NAME), nb::ToInt(NAME))
  REGCONFIG(false);
#undef KEY
#undef KEYEX

  // FDefaultCopyParam must be loaded before eventual setting defaults for CopyParamList
  if (LoadCopyParam(Storage, &FDefaultCopyParam))
  try__finally
  {
    int32_t CopyParamListCount = Storage->ReadInteger(L"CopyParamList", -1);
    FCopyParamListDefaults = (CopyParamListCount < 0);
    if (!FCopyParamListDefaults)
    {
      FCopyParamList->Clear();
      FCopyParamList->Load(Storage, CopyParamListCount);
    }
    else if (FCopyParamList->GetModified())
    {
      FCopyParamList->Clear();
      FCopyParamListDefaults = false;
    }
    FCopyParamList->Reset();
  },
  __finally
  {
    Storage->CloseSubKeyPath();
  } end_try__finally

  // Make it compatible with versions prior to 3.7.1 that have not saved PuttyPath
  // with quotes. First check for absence of quotes.
  // Add quotes either if the path is set to default putty path (even if it does
  // not exists) or when the path points to existing file (so there are no parameters
  // yet in the string). Note that FileExists may display error dialog, but as
  // it should be called only for custom users path, let's expect that the user
  // can take care of it.
  if ((FPuttyPath.SubString(1, 1) != L"\"") &&
      (IsPathToSameFile(ExpandEnvironmentVariables(FPuttyPath), FDefaultPuttyPathOnly) ||
       SysUtulsFileExists(ApiPath(ExpandEnvironmentVariables(FPuttyPath)))))
  {
    FPuttyPath = FormatCommand(FPuttyPath, L"");
  }

  if (Storage->OpenSubKeyPath("Interface\\NewDirectory2", false))
  try__finally
  {
    FNewDirectoryProperties.Load(Storage);
  },
  __finally
  {
    Storage->CloseSubKeyPath();
  } end_try__finally
}

void TGUIConfiguration::Saved()
{
  TConfiguration::Saved();

  FCopyParamList->Reset();
}


UnicodeString TGUIConfiguration::GetTranslationModule(const UnicodeString & Path) const
{
  UnicodeString SubPath = AddTranslationsSubFolder(Path);
  UnicodeString Result;
  // Prefer the SubPath. Default to SubPath.
  if (SysUtulsFileExists(Path) && !SysUtulsFileExists(SubPath))
  {
    Result = Path;
  }
  else
  {
    Result = SubPath;
  }
  return Result;
}

UnicodeString TGUIConfiguration::AddTranslationsSubFolder(const UnicodeString & Path) const
{
  return
    IncludeTrailingBackslash(IncludeTrailingBackslash(ExtractFilePath(Path)) + TranslationsSubFolder) +
    base::ExtractFileName(Path, false);
}

HINSTANCE TGUIConfiguration::LoadNewResourceModule(LCID ALocale,
  UnicodeString &AFileName)
{
  UnicodeString LibraryFileName;
  HINSTANCE NewInstance = nullptr;
  LCID AInternalLocale = InternalLocale();
  bool Internal = (ALocale == AInternalLocale);
  DWORD PrimaryLang = PRIMARYLANGID(ALocale);
  if (!Internal)
  {
    __removed UnicodeString Module;
    UnicodeString LocaleName;

    UnicodeString Module = ModuleFileName();
    if ((ALocale & AdditionaLanguageMask) != AdditionaLanguageMask)
    {
      wchar_t LocaleStr[4];
      GetLocaleInfo(ALocale, LOCALE_SABBREVLANGNAME, LocaleStr, LENOF(LocaleStr));
      LocaleName = LocaleStr;
      DebugAssert(!LocaleName.IsEmpty());
    }
    else
    {
      LocaleName = AdditionaLanguagePrefix +
        char(ALocale & ~AdditionaLanguageMask);
    }

    Module = ChangeFileExt(Module, UnicodeString(".") + LocaleName);
    // Look for a potential language/country translation
    UnicodeString ModulePath = GetTranslationModule(Module);
    NewInstance = LoadLibraryEx(ModulePath.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
    if (NewInstance)
    {
      LibraryFileName = ModulePath;
    }
    else
    {
      DWORD SubLang = SUBLANGID(ALocale);
      DebugAssert(SUBLANG_DEFAULT == SUBLANG_CHINESE_TRADITIONAL);
      // Finally look for a language-only translation.
      // But for Chinese, never use "traditional" (what is the "default" Chinese), if we want "Simplified"
      // (the same what Inno Setup does)
      if ((PrimaryLang != LANG_CHINESE) ||
          (SubLang == SUBLANG_CHINESE_TRADITIONAL))
      {
        Module.SetLength(Module.Length() - 1);
        ModulePath = GetTranslationModule(Module);
        NewInstance = LoadLibraryEx(ModulePath.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
        if (NewInstance)
        {
          LibraryFileName = ModulePath;
        }
      }
    }
  }

  // If the locale is non-US English and we do not have that translation (and it's unlikely we ever have),
  // treat it as if it were US English.
  if (!NewInstance && !Internal && (PrimaryLang == static_cast<DWORD>(PRIMARYLANGID(AInternalLocale))))
  {
    Internal = true;
  }

  if (!NewInstance && !Internal)
  {
    throw Exception(FMTLOAD(LOCALE_LOAD_ERROR, nb::ToInt(ALocale)));
  }
  else
  {
    if (Internal)
    {
      ThrowNotImplemented(90);
      NewInstance = nullptr; // FIXME  HInstance;
    }
  }

  AFileName = LibraryFileName;

  return NewInstance;
}

LCID TGUIConfiguration::InternalLocale()
{
  LCID Result{0};
  void * FileInfo = GetApplicationInfo();
  if (FileInfo && GetTranslationCount(FileInfo) > 0)
  {
    TTranslation Translation;
    Translation = GetTranslation(FileInfo, 0);
    Result = MAKELANGID(PRIMARYLANGID(Translation.Language), SUBLANG_DEFAULT);
    FreeFileInfo(FileInfo);
  }
  else
  {
    //DebugFail();
    Result = GetDefaultLCID();
  }
  return Result;
}

LCID TGUIConfiguration::GetLocale() const
{
  return FLocale;
}

void TGUIConfiguration::SetLocale(LCID Value)
{
  if (FLocale != Value)
  {
    SetLocaleInternal(Value, false, false);
  }
}

void TGUIConfiguration::SetLocaleSafe(LCID Value)
{
  if (GetLocale() != Value)
  {
    SetLocaleInternal(Value, true, false);
  }
}

UnicodeString TGUIConfiguration::GetAppliedLocaleHex() const
{
  return IntToHex(uint64_t(GetAppliedLocale()), 4);
}

int32_t TGUIConfiguration::GetResourceModuleCompleteness(HINSTANCE /*Module*/)
{
  return 100;
}

bool TGUIConfiguration::IsTranslationComplete(HINSTANCE /*Module*/)
{
  return true;
}

void TGUIConfiguration::SetLocaleInternal(LCID Value, bool Safe, bool CompleteOnly)
{
  LCID L = Value;
  if (L == 0)
  {
    L = GetUserDefaultUILanguage();
  }

  HINSTANCE Module = nullptr;
  UnicodeString FileName;

  try
  {
    Module = LoadNewResourceModule(L, FileName);
    DebugAssert(Module != nullptr);
    if (CompleteOnly && !IsTranslationComplete(Module))
    {
      Abort();
    }
  }
  catch (...)
  {
    if (Module != nullptr)
    {
      FreeResourceModule(Module);
      Module = nullptr;
    }

    if (Safe)
    {
      // ignore any exception while loading locale
    }
    else
    {
      throw;
    }
  }

  if (Module != nullptr)
  {
    FLocale = Value;
    if (GetCanApplyLocaleImmediately())
    {
      SetAppliedLocale(L, FileName);
      SetResourceModule(Module);
    }
  }
}

bool TGUIConfiguration::GetCanApplyLocaleImmediately() const
{
  return true;
    __removed (Screen->FormCount == 0) &&
    __removed (Screen->DataModuleCount == 0);
}

bool TGUIConfiguration::UsingInternalTranslation() const
{
  return FLocaleModuleName.IsEmpty();
}

UnicodeString TGUIConfiguration::AppliedLocaleCopyright() const
{
  UnicodeString Result;
  if (UsingInternalTranslation())
  {
    DebugFail(); // we do not expect to get called with internal locale
    Result = UnicodeString();
  }
  else
  {
    Result = GetFileFileInfoString(L"LegalCopyright", FLocaleModuleName);
  }
  return Result;
}

UnicodeString TGUIConfiguration::AppliedLocaleVersion()
{
  UnicodeString Result;
  if (UsingInternalTranslation())
  {
    // noop
  }
  else
  {
    Result = GetFileVersion(FLocaleModuleName);
  }
  return Result;
}

void TGUIConfiguration::SetAppliedLocale(LCID AppliedLocale, const UnicodeString & LocaleModuleName)
{
  FAppliedLocale = AppliedLocale;
  FLocaleModuleName = LocaleModuleName;
}

void TGUIConfiguration::FreeResourceModule(HANDLE /*Instance*/)
{
#if 0
  TLibModule * MainModule = FindModule(HInstance);
  if ((unsigned)Instance != MainModule->Instance)
  {
    FreeLibrary(static_cast<HMODULE>(Instance));
  }
#endif // #if 0
}

HANDLE TGUIConfiguration::ChangeToDefaultResourceModule()
{
  return ChangeResourceModule(nullptr);
}

HANDLE TGUIConfiguration::ChangeResourceModule(HANDLE /*Instance*/)
{
#if 0
  if (Instance == nullptr)
  {
    Instance = HInstance;
  }
  TLibModule * MainModule = FindModule(HInstance);
  HANDLE Result = (HANDLE)MainModule->ResInstance;
  MainModule->ResInstance = (unsigned)Instance;
  CoreSetResourceModule(Instance);
  return Result;
#endif // #if 0
  return nullptr;
}

HANDLE TGUIConfiguration::GetResourceModule()
{
#if 0
  return (HANDLE)FindModule(HInstance)->ResInstance;
#endif // #if 0
  return nullptr;
}

void TGUIConfiguration::SetResourceModule(HINSTANCE Instance)
{
  HANDLE PrevHandle = ChangeResourceModule(Instance);
  FreeResourceModule(PrevHandle);

  DefaultLocalized();
}

void TGUIConfiguration::FindLocales(const UnicodeString & LocalesMask, TStrings * Exts, UnicodeString & LocalesExts)
{
  int32_t FindAttrs = faReadOnly | faArchive;

  TSearchRecOwned SearchRec;
  bool Found = (FindFirstUnchecked(LocalesMask, static_cast<DWORD>(FindAttrs), SearchRec) == 0);
  while (Found)
  {
    UnicodeString Ext = ExtractFileExt(SearchRec.Name).UpperCase();
    // DLL is a remnant from times the .NET assembly was winscp.dll, not winscpnet.dll
    if ((Ext.Length() >= 3) && (Ext != L".EXE") && (Ext != L".COM") &&
        (Ext != L".DLL") && (Ext != L".INI") && (Ext != L".MAP"))
    {
      Ext = Ext.SubString(2, Ext.Length() - 1);
      LocalesExts += Ext;
      Exts->Add(Ext);
    }
    Found = (FindNextChecked(SearchRec) == 0);
  }
}

void TGUIConfiguration::AddLocale(LCID Locale, const UnicodeString & Name)
{
  std::unique_ptr<TLocaleInfo> LocaleInfo(std::make_unique<TLocaleInfo>());
  LocaleInfo->Locale = Locale;
  LocaleInfo->Name = Name;

  try
  {
    UnicodeString FileName;
    HINSTANCE Module = LoadNewResourceModule(Locale, FileName);
    try__finally
    {
      LocaleInfo->Completeness = GetResourceModuleCompleteness(Module);
    },
    __finally
    {
      FreeResourceModule(Module);
    } end_try__finally
  }
  catch (...)
  {
    LocaleInfo->Completeness = -1;
  }

  FLocales->Add(LocaleInfo.release());
}

int32_t TGUIConfiguration::LocalesCompare(void * Item1, void * Item2)
{
  TLocaleInfo * LocaleInfo1 = static_cast<TLocaleInfo *>(Item1);
  TLocaleInfo * LocaleInfo2 = static_cast<TLocaleInfo *>(Item2);
  return CompareText(LocaleInfo1->Name, LocaleInfo2->Name);
}

TObjectList * TGUIConfiguration::GetLocales()
{
  UnicodeString LocalesMask = ChangeFileExt(ModuleFileName(), L".*");
  UnicodeString SubLocalesMask = AddTranslationsSubFolder(LocalesMask);

  UnicodeString LocalesExts;
  std::unique_ptr<TStringList> Exts(CreateSortedStringList());
  FindLocales(SubLocalesMask, Exts.get(), LocalesExts);
  FindLocales(LocalesMask, Exts.get(), LocalesExts);

  if (FLastLocalesExts != LocalesExts)
  {
    FLastLocalesExts = LocalesExts;
    FLocales->Clear();

#if 0
    TLanguages * Langs = Languages();

    int Count = Langs->Count;
    int Index = -1;
    while (Index < Count)
    {
      LCID Locale;
      if (Index >= 0)
      {
        Locale = Langs->LocaleID[Index];
        DWORD SubLang = SUBLANGID(Locale);
        int Ext = Exts->IndexOf(Langs->Ext[Index]);
        if ((Ext >= 0) && (Exts->Objects[Ext] == nullptr))
        {
          // noop
        }
        else if (SubLang == SUBLANG_DEFAULT)
        {
          Ext = Exts->IndexOf(Langs->Ext[Index].SubString(1, 2));
          if ((Ext >= 0) && (Exts->Objects[Ext] == nullptr))
          {
            Locale = MAKELANGID(PRIMARYLANGID(Locale), SUBLANG_DEFAULT);
          }
        }

        if (Ext >= 0)
        {
          Exts->SetObj(Ext, ToObj(Locale));
        }
        else
        {
          Locale = 0;
        }
      }
      else
      {
        Locale = InternalLocale();
      }

      if (Locale)
      {
        wchar_t LocaleStr[255];
        GetLocaleInfo(Locale, LOCALE_SENGLANGUAGE,
          LocaleStr, LENOF(LocaleStr));
        UnicodeString Name = LocaleStr;
        Name += TitleSeparator;
        // LOCALE_SNATIVELANGNAME
        GetLocaleInfo(Locale, LOCALE_SLANGUAGE,
          LocaleStr, LENOF(LocaleStr));
        Name += LocaleStr;
        AddLocale(Locale, Name);
      }
      Index++;
    }

    for (int Index = 0; Index < Exts->Count; Index++)
    {
      if ((Exts->Objects[Index] == nullptr) &&
          (Exts->Strings[Index].Length() == 3) &&
          SameText(Exts->Strings[Index].SubString(1, 2), AdditionaLanguagePrefix))
      {
        UnicodeString ModulePath = ChangeFileExt(ModuleFileName(), UnicodeString(L".") + Exts->Strings[Index]);
        ModulePath = GetTranslationModule(ModulePath);
        UnicodeString LangName = GetFileFileInfoString(L"LangName", ModulePath);
        if (!LangName.IsEmpty())
        {
          AddLocale(AdditionaLanguageMask + Exts->Strings[Index][3], LangName);
        }
      }
    }

    FLocales->Sort(LocalesCompare);
#endif // #if 0
  }

  return FLocales.get();
}

void TGUIConfiguration::SetDefaultCopyParam(const TGUICopyParamType &Value)
{
  FDefaultCopyParam.Assign(&Value);
  Changed();
}

bool TGUIConfiguration::GetRememberPassword() const
{
  bool Result = SessionRememberPassword || PuttyPassword;

  if (!Result)
  {
    try
    {
      TRemoteCustomCommand RemoteCustomCommand;
      TInteractiveCustomCommand InteractiveCustomCommand(&RemoteCustomCommand);
      UnicodeString APuttyPath = InteractiveCustomCommand.Complete(PuttyPath, false);
      Result = RemoteCustomCommand.IsPasswordCommand(PuttyPath);
    }
    catch (...)
    {
      // noop
    }
  }

  return Result;
}

const TCopyParamList * TGUIConfiguration::GetCopyParamList() const
{
  return FCopyParamList.get();
}

void TGUIConfiguration::SetCopyParamList(const TCopyParamList * Value)
{
  if (!(*FCopyParamList == *Value))
  {
    *FCopyParamList = *Value;
    FCopyParamListDefaults = false;
    Changed();
  }
}

int32_t TGUIConfiguration::GetCopyParamIndex() const
{
  int32_t Result;
  if (FCopyParamCurrent.IsEmpty())
  {
    Result = -1;
  }
  else
  {
    Result = FCopyParamList->IndexOfName(FCopyParamCurrent);
  }
  return Result;
}

void TGUIConfiguration::SetCopyParamIndex(int32_t Value)
{
  UnicodeString Name;
  if (Value < 0)
  {
    Name = "";
  }
  else
  {
    Name = FCopyParamList->GetName(Value);
  }
  SetCopyParamCurrent(Name);
}

void TGUIConfiguration::SetCopyParamCurrent(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY(CopyParamCurrent);
}

TGUICopyParamType TGUIConfiguration::GetCurrentCopyParam() const
{
  return GetCopyParamPreset(GetCopyParamCurrent());
}

TGUICopyParamType TGUIConfiguration::GetCopyParamPreset(const UnicodeString & Name) const
{
  TGUICopyParamType Result = FDefaultCopyParam;
  if (!Name.IsEmpty())
  {
    int32_t Index = FCopyParamList->IndexOfName(Name);
    DebugAssert(Index >= 0);
    if (Index >= 0)
    {
      const TCopyParamType * Preset = FCopyParamList->GetCopyParam(Index);
      DebugAssert(Preset != nullptr);
      Result.Assign(Preset); // overwrite all but GUI options
      // reset all options known not to be configurable per-preset
      // kind of hack
      Result.SetResumeSupport(FDefaultCopyParam.GetResumeSupport());
      Result.SetResumeThreshold(FDefaultCopyParam.GetResumeThreshold());
      Result.SetLocalInvalidChars(FDefaultCopyParam.GetLocalInvalidChars());
    }
  }
  return Result;
}

bool TGUIConfiguration::GetHasCopyParamPreset(const UnicodeString & Name) const
{
  return Name.IsEmpty() || (FCopyParamList->IndexOfName(Name) >= 0);
}

void TGUIConfiguration::SetNewDirectoryProperties(
  const TRemoteProperties & Value)
{
  SET_CONFIG_PROPERTY(NewDirectoryProperties);
}

void TGUIConfiguration::SetQueueBootstrap(bool Value)
{
  SET_CONFIG_PROPERTY(QueueBootstrap);
}

void TGUIConfiguration::SetQueueKeepDoneItems(bool Value)
{
  SET_CONFIG_PROPERTY(QueueKeepDoneItems);
}

void TGUIConfiguration::SetQueueKeepDoneItemsFor(int32_t Value)
{
  SET_CONFIG_PROPERTY(QueueKeepDoneItemsFor);
}

TStoredSessionList * TGUIConfiguration::SelectPuttySessionsForImport(
  const UnicodeString & RootKey, const UnicodeString & Source, TStoredSessionList * Sessions, UnicodeString & AError)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(std::make_unique<TStoredSessionList>(true));
  ImportSessionList->SetDefaultSettings(Sessions->GetDefaultSettings());

  UnicodeString SessionsKey = GetPuttySessionsKey(RootKey);
  std::unique_ptr<TRegistryStorage> Storage(std::make_unique<TRegistryStorage>(SessionsKey));
  Storage->ConfigureForPutty();
  if (Storage->OpenRootKey(false))
  {
    ImportSessionList->Load(Storage.get(), false, true, true);
  }

  TSessionData * PuttySessionData =
    dyn_cast<TSessionData>(ImportSessionList->FindByName(GetPuttySession()));
  if (PuttySessionData != nullptr)
  {
    ImportSessionList->Remove(PuttySessionData);
  }
  if (ImportSessionList->GetCount() > 0)
  {
    ImportSessionList->SelectSessionsToImport(Sessions, true);
  }
  else
  {
    AError = FMTLOAD(PUTTY_NO_SITES2, Source, SessionsKey);
  }

  return ImportSessionList.release();
}

bool TGUIConfiguration::AnyPuttySessionForImport(TStoredSessionList * ASessions)
{
  try
  {
    UnicodeString Error;
    std::unique_ptr<TStoredSessionList> SessionsForImport(
      SelectPuttySessionsForImport(OriginalPuttyRegistryStorageKey, L"PuTTY", ASessions, Error));
    return (SessionsForImport->Count > 0);
  }
  catch (...)
  {
    return false;
  }
}

UnicodeString TGUIConfiguration::GetPuttyPath() const
{
  return FPuttyPath;
}

void TGUIConfiguration::SetPuttyPath(const UnicodeString & Value)
{
  FPuttyPath = Value;
}

UnicodeString TGUIConfiguration::GetDefaultPuttyPath() const
{
  return FDefaultPuttyPath;
}

UnicodeString TGUIConfiguration::GetPuttySession() const
{
  return FPuttySession;
}

void TGUIConfiguration::SetPuttySession(const UnicodeString & Value)
{
  FPuttySession = Value;
}

UnicodeString TGUIConfiguration::GetCopyParamCurrent() const
{
  return FCopyParamCurrent;
}

UnicodeString TGUIConfiguration::GetChecksumAlg() const
{
  return FChecksumAlg;
}

void TGUIConfiguration::SetChecksumAlg(const UnicodeString & Value)
{
  FChecksumAlg = Value;
}

TGUIConfiguration *GetGUIConfiguration()
{
  return dyn_cast<TGUIConfiguration>(GetConfiguration());
}

