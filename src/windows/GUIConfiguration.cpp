#include <vcl.h>
#pragma hdrstop
#include <LanguagesDEPfix.hpp>
#include "GUIConfiguration.h"
#include "GUITools.h"
#include <Common.h>
#include <FileInfo.h>
#include <TextsCore.h>
#include <Terminal.h>
#include <CoreMain.h>
#include <shlobj.h>

const intptr_t ccLocal = ccUser;
const intptr_t ccShowResults = ccUser << 1;
const intptr_t ccCopyResults = ccUser << 2;
const intptr_t ccRemoteFiles = ccUser << 3;
const intptr_t ccSet = 0x80000000;

static const uintptr_t AdditionaLanguageMask = 0xFFFFFF00;
static const UnicodeString AdditionaLanguagePrefix(L"XX");
static const UnicodeString TranslationsSubFolder(L"Translations");

TGUICopyParamType::TGUICopyParamType() :
  TCopyParamType(OBJECT_CLASS_TGUICopyParamType)
{
  GUIDefault();
}

TGUICopyParamType::TGUICopyParamType(const TCopyParamType & Source) :
  TCopyParamType(Source)
{
  GUIDefault();
}

TGUICopyParamType::TGUICopyParamType(const TGUICopyParamType & Source) :
  TCopyParamType(Source)
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
  GUIDefault();
}

void TGUICopyParamType::GUIDefault()
{
  TCopyParamType::Default();

  SetQueue(false);
  SetQueueNoConfirmation(true);
  SetQueueParallel(false);
}

void TGUICopyParamType::Load(THierarchicalStorage * Storage)
{
  TCopyParamType::Load(Storage);

  SetQueue(Storage->ReadBool("Queue", GetQueue()));
  SetQueueNoConfirmation(Storage->ReadBool("QueueNoConfirmation", GetQueueNoConfirmation()));
  SetQueueParallel(Storage->ReadBool("QueueParallel", GetQueueParallel()));
}

void TGUICopyParamType::Save(THierarchicalStorage * Storage)
{
  TCopyParamType::Save(Storage);

  Storage->WriteBool("Queue", GetQueue());
  Storage->WriteBool("QueueNoConfirmation", GetQueueNoConfirmation());
  Storage->WriteBool("QueueQueueParallel", GetQueueParallel());
}

TGUICopyParamType & TGUICopyParamType::operator=(const TCopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}

TGUICopyParamType & TGUICopyParamType::operator=(const TGUICopyParamType & rhp)
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

TCopyParamRule::TCopyParamRule() :
  TObject(OBJECT_CLASS_TCopyParamRule)
{
}

TCopyParamRule::TCopyParamRule(const TCopyParamRuleData & Data) :
  TObject(OBJECT_CLASS_TCopyParamRule),
  FData(Data)
{
}

TCopyParamRule::TCopyParamRule(const TCopyParamRule & Source) :
  TObject(OBJECT_CLASS_TCopyParamRule)
{
  FData.HostName = Source.FData.HostName;
  FData.UserName = Source.FData.UserName;
  FData.RemoteDirectory = Source.FData.RemoteDirectory;
  FData.LocalDirectory = Source.FData.LocalDirectory;
}

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

bool TCopyParamRule::Match(UnicodeString Mask,
  UnicodeString Value, bool Path, bool Local, int ForceDirectoryMasks) const
{
  bool Result;
  if (Mask.IsEmpty())
  {
    Result = true;
  }
  else
  {
    TFileMasks M(ForceDirectoryMasks);
    M.SetMasks(Mask);
    if (Path)
    {
      Result = M.Matches(Value, Local, true);
    }
    else
    {
      Result = M.Matches(Value, false);
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

TCopyParamRule & TCopyParamRule::operator=(const TCopyParamRule & other)
{
  SetData(other.FData);
  return *this;
}

UnicodeString TCopyParamRule::GetInfoStr(UnicodeString Separator) const
{
  UnicodeString Result;
#define ADD(FMT, ELEM) \
    if (!FData.ELEM.IsEmpty()) \
      Result += (Result.IsEmpty() ? UnicodeString() : Separator) + FMTLOAD(FMT, FData.ELEM.c_str());
  ADD(COPY_RULE_HOSTNAME, HostName);
  ADD(COPY_RULE_USERNAME, UserName);
  ADD(COPY_RULE_REMOTE_DIR, RemoteDirectory);
  ADD(COPY_RULE_LOCAL_DIR, LocalDirectory);
#undef ADD
  return Result;
}

TCopyParamList::TCopyParamList() :
  FRules(new TList()),
  FCopyParams(new TList()),
  FNames(new TStringList()),
  FNameList(nullptr),
  FModified(false)
{
}

TCopyParamList::TCopyParamList(const TCopyParamList & other) :
  FRules(new TList()),
  FCopyParams(new TList()),
  FNames(new TStringList()),
  FNameList(nullptr),
  FModified(false)
{
  this->operator=(other);
}

TCopyParamList::~TCopyParamList()
{
  Clear();
  SAFE_DESTROY(FCopyParams);
  SAFE_DESTROY(FRules);
  SAFE_DESTROY(FNames);
  SAFE_DESTROY(FNameList);
}

void TCopyParamList::Reset()
{
  SAFE_DESTROY(FNameList);
  FModified = false;
}

void TCopyParamList::Modify()
{
  SAFE_DESTROY(FNameList);
  FModified = true;
}

void TCopyParamList::ValidateName(UnicodeString Name)
{
  if (Name.LastDelimiter(CONST_INVALID_CHARS) > 0)
  {
    throw Exception(FMTLOAD(ITEM_NAME_INVALID, Name.c_str(), CONST_INVALID_CHARS));
  }
}

TCopyParamList & TCopyParamList::operator=(const TCopyParamList & rhl)
{
  Clear();

  for (intptr_t Index = 0; Index < rhl.GetCount(); ++Index)
  {
    TCopyParamType * CopyParam = new TCopyParamType(*rhl.GetCopyParam(Index));
    TCopyParamRule * Rule = nullptr;
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
    intptr_t Index = 0;
    while ((Index < GetCount()) && Result)
    {
      Result = (GetName(Index) == rhl.GetName(Index)) &&
        CompareItem(Index, rhl.GetCopyParam(Index), rhl.GetRule(Index));
      ++Index;
    }
  }
  return Result;
}

intptr_t TCopyParamList::IndexOfName(UnicodeString Name) const
{
  return FNames->IndexOf(Name);
}

bool TCopyParamList::CompareItem(intptr_t Index,
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
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    delete GetCopyParam(Index);
    delete GetRule(Index);
  }
  FCopyParams->Clear();
  FRules->Clear();
  FNames->Clear();
}

void TCopyParamList::Add(UnicodeString Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  Insert(GetCount(), Name, CopyParam, Rule);
}

void TCopyParamList::Insert(intptr_t Index, UnicodeString Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  DebugAssert(FNames->IndexOf(Name) < 0);
  FNames->Insert(Index, Name);
  DebugAssert(CopyParam != nullptr);
  FCopyParams->Insert(Index, CopyParam);
  FRules->Insert(Index, Rule);
  Modify();
}

void TCopyParamList::Change(intptr_t Index, UnicodeString Name,
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

void TCopyParamList::Move(intptr_t CurIndex, intptr_t NewIndex)
{
  if (CurIndex != NewIndex)
  {
    FNames->Move(CurIndex, NewIndex);
    FCopyParams->Move(CurIndex, NewIndex);
    FRules->Move(CurIndex, NewIndex);
    Modify();
  }
}

void TCopyParamList::Delete(intptr_t Index)
{
  DebugAssert((Index >= 0) && (Index < GetCount()));
  FNames->Delete(Index);
  delete GetCopyParam(Index);
  FCopyParams->Delete(Index);
  delete GetRule(Index);
  FRules->Delete(Index);
  Modify();
}

intptr_t TCopyParamList::Find(const TCopyParamRuleData & Value) const
{
  intptr_t Result = -1;
  intptr_t Index = 0;
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

void TCopyParamList::Load(THierarchicalStorage * Storage, intptr_t ACount)
{
  for (intptr_t Index = 0; Index < ACount; ++Index)
  {
    UnicodeString Name = ::IntToStr(Index);
    std::unique_ptr<TCopyParamRule> Rule;
    std::unique_ptr<TCopyParamType> CopyParam(new TCopyParamType());
    try__catch
    {
      if (Storage->OpenSubKey(Name, false))
      {
        try__finally
        {
          SCOPE_EXIT
          {
            Storage->CloseSubKey();
          };
          Name = Storage->ReadString("Name", Name);
          CopyParam->Load(Storage);

          if (Storage->ReadBool("HasRule", false))
          {
            Rule.reset(new TCopyParamRule());
            Rule->Load(Storage);
          }
        }
        __finally
        {
#if 0
          Storage->CloseSubKey();
#endif // #if 0
        };
      }
    }
#if 0
    catch(...)
    {
      delete CopyParam;
      delete Rule;
      throw;
    }
#endif // #if 0

    FCopyParams->Add(CopyParam.release());
    FRules->Add(Rule.release());
    FNames->Add(Name);
  }
  Reset();
}

void TCopyParamList::Save(THierarchicalStorage * Storage) const
{
  Storage->ClearSubKeys();
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (Storage->OpenSubKey(::IntToStr(Index), /*CanCreate*/ true))
    {
      try__finally
      {
        SCOPE_EXIT
        {
          Storage->CloseSubKey();
        };
        const TCopyParamType * CopyParam = GetCopyParam(Index);
        const TCopyParamRule * Rule = GetRule(Index);

        Storage->WriteString("Name", GetName(Index));
        CopyParam->Save(Storage);
        Storage->WriteBool("HasRule", (Rule != nullptr));
        if (Rule != nullptr)
        {
          Rule->Save(Storage);
        }
      }
      __finally
      {
#if 0
        Storage->CloseSubKey();
#endif // #if 0
      };
    }
  }
}

intptr_t TCopyParamList::GetCount() const
{
  return FCopyParams ? FCopyParams->GetCount() : 0;
}

const TCopyParamRule * TCopyParamList::GetRule(intptr_t Index) const
{
  return FRules->GetAs<TCopyParamRule>(Index);
}

const TCopyParamType * TCopyParamList::GetCopyParam(intptr_t Index) const
{
  return FCopyParams->GetAs<TCopyParamType>(Index);
}

UnicodeString TCopyParamList::GetName(intptr_t Index) const
{
  return FNames->GetString(Index);
}

TStrings * TCopyParamList::GetNameList() const
{
  if (FNameList == nullptr)
  {
    FNameList = new TStringList();

    for (intptr_t Index = 0; Index < GetCount(); ++Index)
    {
      FNameList->Add(FNames->GetString(Index));
    }
  }
  return FNameList;
}

bool TCopyParamList::GetAnyRule() const
{
  bool Result = false;
  intptr_t Index = 0;
  while ((Index < GetCount()) && !Result)
  {
    Result = (GetRule(Index) != nullptr);
    ++Index;
  }
  return Result;
}

TGUIConfiguration::TGUIConfiguration(TObjectClassId Kind) :
  TConfiguration(Kind),
  FLocales(CreateSortedStringList()),
  FLastLocalesExts(L"*"),
  FContinueOnError(false),
  FConfirmCommandSession(false),
  FPuttyPassword(false),
  FTelnetForFtpInPutty(false),
  FSynchronizeParams(0),
  FSynchronizeOptions(0),
  FSynchronizeModeAuto(0),
  FSynchronizeMode(0),
  FMaxWatchDirectories(0),
  FQueueAutoPopup(false),
  FSessionRememberPassword(false),
  FQueueTransfersLimit(0),
  FQueueKeepDoneItems(false),
  FQueueKeepDoneItemsFor(0),
  FBeepOnFinish(false),
  FCopyParamList(new TCopyParamList()),
  FCopyParamListDefaults(false),
  FKeepUpToDateChangeDelay(0),
  FSessionReopenAutoIdle(0),
  FAppliedLocale(0),
  FLocale(0)
{
  CoreSetResourceModule(nullptr);
  FLocales = new TObjectList();
}

TGUIConfiguration::~TGUIConfiguration()
{
  SAFE_DESTROY(FLocales);
  SAFE_DESTROY(FCopyParamList);
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
  FSynchronizeParams = TTerminal::spNoConfirmation | TTerminal::spPreviewChanges;
  FSynchronizeModeAuto = -1;
  FSynchronizeMode = TTerminal::smRemote;
  FMaxWatchDirectories = 500;
  FSynchronizeOptions = soRecurse | soSynchronizeAsk;
  FQueueTransfersLimit = 2;
  FQueueKeepDoneItems = true;
  FQueueKeepDoneItemsFor = 15;
  FQueueAutoPopup = true;
  FSessionRememberPassword = true;
  UnicodeString ProgramsFolder;
  SpecialFolderLocation(CSIDL_PROGRAM_FILES, ProgramsFolder);
  FDefaultPuttyPathOnly = ::IncludeTrailingBackslash(ProgramsFolder) + OriginalPuttyExecutable;
  FDefaultPuttyPath = L"%PROGRAMFILES%\\PuTTY\\" + OriginalPuttyExecutable;
  FPuttyPath = FormatCommand(FDefaultPuttyPath, L"");
  //SetPSftpPath(FormatCommand("%PROGRAMFILES%\\PuTTY\\psftp.exe", L""));
  FPuttyPassword = false;
  FTelnetForFtpInPutty = true;
  FPuttySession = "WinSCP temporary session";
  FBeepOnFinish = false;
  FBeepOnFinishAfter = TDateTime(0, 0, 30, 0);
  FCopyParamCurrent.Clear();
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
      TCopyParamType * CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmAscii);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_ASCII), CopyParam, nullptr);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmBinary);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_BINARY), CopyParam, nullptr);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->GetIncludeFileMask().SetMasks("|*.bak; *.tmp; ~$*; *.wbk; *~; #*; .#*");
      CopyParam->SetNewerOnly(true);
      FCopyParamList->Add(LoadStr(COPY_PARAM_NEWER_ONLY), CopyParam, nullptr);
    }

    FCopyParamList->Reset();
  }
}

void TGUIConfiguration::UpdateStaticUsage()
{
  // TConfiguration::UpdateStaticUsage();
  // Usage->Set(L"CopyParamsCount", (FCopyParamListDefaults ? 0 : FCopyParamList->Count));
  // Usage->Set(L"CopyParamsCount", (FCopyParamListDefaults ? 0 : FCopyParamList->GetCount()));
}

static UnicodeString PropertyToKey(UnicodeString Property)
{
  // no longer useful
  intptr_t P = Property.LastDelimiter(L".>");
  return Property.SubString(P + 1, Property.Length() - P);
}

// duplicated from core\configuration.cpp
#undef BLOCK
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
    { SCOPE_EXIT { Storage->CloseSubKey(); }; { BLOCK } }
#undef REGCONFIG

#define REGCONFIG(CANCREATE) \
  BLOCK(L"Interface", CANCREATE, \
    KEY(Bool,     ContinueOnError); \
    KEY(Bool,     ConfirmCommandSession); \
    KEY(Integer,  SynchronizeParams); \
    KEY(Integer,  SynchronizeOptions); \
    KEY(Integer,  SynchronizeModeAuto); \
    KEY(Integer,  SynchronizeMode); \
    KEY(Integer,  MaxWatchDirectories); \
    KEY(Integer,  QueueTransfersLimit); \
    KEY(Bool,  QueueKeepDoneItems); \
    KEY(Integer,  QueueKeepDoneItemsFor); \
    KEY(Bool,     QueueAutoPopup); \
    KEYEX(Bool,   QueueRememberPassword, SessionRememberPassword); \
    KEY(String,   PuttySession); \
    KEY(String,   PuttyPath); \
    KEY(Bool,     PuttyPassword); \
    KEY(Bool,     TelnetForFtpInPutty); \
    KEY(DateTime, IgnoreCancelBeforeFinish); \
    KEY(Bool,     BeepOnFinish); \
    KEY(DateTime, BeepOnFinishAfter); \
    KEY(Integer,  KeepUpToDateChangeDelay); \
    KEY(String,   ChecksumAlg); \
    KEY(Integer,  SessionReopenAutoIdle); \
  )

void TGUIConfiguration::SaveData(THierarchicalStorage * Storage, bool All)
{
  TConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
#ifndef LASTELEM
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>")+1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
#endif
#undef KEYEX
#define KEYEX(TYPE, NAME, VAR) Storage->Write ## TYPE(LASTELEM(UnicodeString(#NAME)), Get ## VAR())
#undef KEY
#define KEY(TYPE, NAME) Storage->Write ## TYPE(PropertyToKey(#NAME), Get ## NAME())
  REGCONFIG(true);
#undef KEY
#undef KEYEX

  if (Storage->OpenSubKey(L"Interface\\CopyParam", /*CanCreate*/ true, /*Path*/ true))
  {
    try__finally
    {
      SCOPE_EXIT
      {
        Storage->CloseSubKey();
      };
      FDefaultCopyParam.Save(Storage);

      if (FCopyParamListDefaults)
      {
        DebugAssert(!FCopyParamList->GetModified());
        Storage->WriteInteger("CopyParamList", -1);
      }
      else if (All || FCopyParamList->GetModified())
      {
        Storage->WriteInteger("CopyParamList", FCopyParamList->GetCount());
        FCopyParamList->Save(Storage);
      }
    }
    __finally
    {
#if 0
      Storage->CloseSubKey();
#endif // #if 0
    };
  }

  if (Storage->OpenSubKey(L"Interface\\NewDirectory", /*CanCreate*/ true, /*Path*/ true))
  {
    try__finally
    {
      SCOPE_EXIT
      {
        Storage->CloseSubKey();
      };
      FNewDirectoryProperties.Save(Storage);
    }
    __finally
    {
#if 0
      Storage->CloseSubKey();
#endif // #if 0
    };
  }
}

void TGUIConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
#undef KEYEX
#define KEYEX(TYPE, NAME, VAR) Set ## VAR(Storage->Read ## TYPE(LASTELEM(UnicodeString(#NAME)), Get ## VAR()))
#undef KEY
#define KEY(TYPE, NAME) Set ## NAME(Storage->Read ## TYPE(PropertyToKey(#NAME), Get ## NAME()))
  REGCONFIG(false);
#undef KEY
#undef KEYEX

  if (Storage->OpenSubKey(L"Interface\\CopyParam", /*CanCreate*/ false, /*Path*/ true))
  {
    try__finally
    {
      SCOPE_EXIT
      {
        Storage->CloseSubKey();
      };
      // must be loaded before eventual setting defaults for CopyParamList
      FDefaultCopyParam.Load(Storage);

      intptr_t CopyParamListCount = Storage->ReadInteger("CopyParamList", -1);
      FCopyParamListDefaults = ((int)CopyParamListCount <= 0);
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
    }
    __finally
    {
#if 0
      Storage->CloseSubKey();
#endif // #if 0
    };
  }

  // Make it compatible with versions prior to 3.7.1 that have not saved PuttyPath
  // with quotes. First check for absence of quotes.
  // Add quotes either if the path is set to default putty path (even if it does
  // not exists) or when the path points to existing file (so there are no parameters
  // yet in the string). Note that FileExists may display error dialog, but as
  // it should be called only for custom users path, let's expect that the user
  // can take care of it.
  if ((FPuttyPath.SubString(1, 1) != L"\"") &&
    (CompareFileName(::ExpandEnvironmentVariables(FPuttyPath), FDefaultPuttyPathOnly) ||
      ::FileExists(::ExpandEnvironmentVariables(FPuttyPath))))
  {
    FPuttyPath = FormatCommand(FPuttyPath, L"");
  }

  if (Storage->OpenSubKey(L"Interface\\NewDirectory", false, true))
  {
    try__finally
    {
      SCOPE_EXIT
      {
        Storage->CloseSubKey();
      };
      FNewDirectoryProperties.Load(Storage);
    }
    __finally
    {
#if 0
      Storage->CloseSubKey();
#endif // #if 0
    };
  }
}

void TGUIConfiguration::Saved()
{
  TConfiguration::Saved();

  FCopyParamList->Reset();
}


UnicodeString TGUIConfiguration::GetTranslationModule(UnicodeString Path) const
{
  UnicodeString SubPath = AddTranslationsSubFolder(Path);
  UnicodeString Result;
  // Prefer the SubPath. Default to SubPath.
  if (FileExists(Path) && !FileExists(SubPath))
  {
    Result = Path;
  }
  else
  {
    Result = SubPath;
  }
  return Result;
}

UnicodeString TGUIConfiguration::AddTranslationsSubFolder(UnicodeString Path) const
{
  return
    ::IncludeTrailingBackslash(::IncludeTrailingBackslash(::ExtractFilePath(Path)) + TranslationsSubFolder) +
    base::ExtractFileName(Path, false);
}

HINSTANCE TGUIConfiguration::LoadNewResourceModule(LCID ALocale,
  UnicodeString & AFileName)
{
  UnicodeString LibraryFileName;
  HINSTANCE NewInstance = nullptr;
  bool Internal = (ALocale == InternalLocale());
  if (!Internal)
  {
    UnicodeString LocaleName;

    UnicodeString Module = ModuleFileName();
    if ((ALocale & AdditionaLanguageMask) != AdditionaLanguageMask)
    {
      wchar_t LocaleStr[4];
      GetLocaleInfo(ALocale, LOCALE_SABBREVLANGNAME, LocaleStr, _countof(LocaleStr));
      LocaleName = LocaleStr;
      DebugAssert(!LocaleName.IsEmpty());
    }
    else
    {
      LocaleName = AdditionaLanguagePrefix +
        char(ALocale & ~AdditionaLanguageMask);
    }

    Module = ::ChangeFileExt(Module, UnicodeString(L".") + LocaleName);
    // Look for a potential language/country translation
    UnicodeString ModulePath = GetTranslationModule(Module);
    NewInstance = ::LoadLibraryEx(ModulePath.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
    if (NewInstance)
    {
      LibraryFileName = ModulePath;
    }
    else
    {
      DWORD PrimaryLang = PRIMARYLANGID(ALocale);
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
        NewInstance = LoadLibraryEx(ModulePath.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
        if (NewInstance)
        {
          LibraryFileName = ModulePath;
        }
      }
    }
  }

  if (!NewInstance && !Internal)
  {
    throw Exception(FMTLOAD(LOCALE_LOAD_ERROR, static_cast<int>(ALocale)));
  }
  if (Internal)
  {
    ThrowNotImplemented(90);
    NewInstance = nullptr; // FIXME  HInstance;
  }

  AFileName = LibraryFileName;

  return NewInstance;
}

LCID TGUIConfiguration::InternalLocale() const
{
  LCID Result;
  if (GetTranslationCount(GetApplicationInfo()) > 0)
  {
    TTranslation Translation;
    Translation = GetTranslation(GetApplicationInfo(), 0);
    Result = MAKELANGID(PRIMARYLANGID(Translation.Language), SUBLANG_DEFAULT);
  }
  else
  {
    DebugFail();
    Result = 0;
  }
  return Result;
}

LCID TGUIConfiguration::GetLocale()
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
  if (FLocale != Value)
  {
    SetLocaleInternal(Value, true, false);
  }
}

UnicodeString TGUIConfiguration::GetAppliedLocaleHex() const
{
  return IntToHex(int64_t(GetAppliedLocale()), 4);
}

int TGUIConfiguration::GetResourceModuleCompleteness(HINSTANCE /*Module*/)
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
#if 0
    (Screen->FormCount == 0) &&
    (Screen->DataModuleCount == 0);
#endif // #if 0
}

UnicodeString TGUIConfiguration::AppliedLocaleCopyright() const
{
  UnicodeString Result;
  if ((FAppliedLocale == 0) || (FAppliedLocale == InternalLocale()))
  {
    DebugFail(); // we do not expect to get called with internal locale
    Result = UnicodeString();
  }
  else
  {
    DebugAssert(!FLocaleModuleName.IsEmpty());
    Result = GetFileFileInfoString(L"LegalCopyright", FLocaleModuleName);
  }
  return Result;
}

UnicodeString TGUIConfiguration::AppliedLocaleVersion()
{
  UnicodeString Result;
  if ((FAppliedLocale == 0) || (FAppliedLocale == InternalLocale()))
  {
    // noop
  }
  else
  {
    Result = GetFileVersion(FLocaleModuleName);
  }
  return Result;
}

void TGUIConfiguration::SetAppliedLocale(LCID AppliedLocale, UnicodeString LocaleModuleName)
{
  FAppliedLocale = AppliedLocale;
  FLocaleModuleName = LocaleModuleName;
}

void TGUIConfiguration::FreeResourceModule(HANDLE Instance)
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

void TGUIConfiguration::FindLocales(UnicodeString LocalesMask, TStrings * Exts, UnicodeString & LocalesExts)
{
  int FindAttrs = faReadOnly | faArchive;
  TSearchRecChecked SearchRec;
  bool Found =
    (FindFirstUnchecked(LocalesMask, FindAttrs, SearchRec) == 0);
  try__finally
  {
    SCOPE_EXIT
    {
      base::FindClose(SearchRec);
    };
    while (Found)
    {
      UnicodeString Ext = ::ExtractFileExt(SearchRec.Name).UpperCase();
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
  __finally
  {
#if 0
    FindClose(SearchRec);
#endif // #if 0
  };
}

void TGUIConfiguration::AddLocale(LCID Locale, UnicodeString Name)
{
  std::unique_ptr<TLocaleInfo> LocaleInfo(new TLocaleInfo());
  LocaleInfo->Locale = Locale;
  LocaleInfo->Name = Name;

  try
  {
    UnicodeString FileName;
    HINSTANCE Module = LoadNewResourceModule(Locale, FileName);
    try__finally
    {
      SCOPE_EXIT
      {
        FreeResourceModule(Module);
      };
      LocaleInfo->Completeness = GetResourceModuleCompleteness(Module);
    }
    __finally
    {
#if 0
      FreeResourceModule(Module);
#endif // #if 0
    };
  }
  catch (...)
  {
    LocaleInfo->Completeness = -1;
  }

  FLocales->Add(LocaleInfo.release());
}

intptr_t TGUIConfiguration::LocalesCompare(void * Item1, void * Item2)
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
        Name += L" - ";
        // LOCALE_SNATIVELANGNAME
        GetLocaleInfo(Locale, LOCALE_SLANGUAGE,
          LocaleStr, LENOF(LocaleStr));
        Name += LocaleStr;
        // AddLocale(Locale, Name);
        FLocales->AddObject(Name, ToObj(Locale));
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
          //AddLocale(AdditionaLanguageMask + Exts->Strings[Index][3], LangName);
          FLocales->AddObject(LangName, ToObj(
            AdditionaLanguageMask + Exts->GetString(Index)[3]));
        }
      }
    }

    FLocales->Sort(LocalesCompare);
#endif // #if 0
  }

  return FLocales;
}

void TGUIConfiguration::SetDefaultCopyParam(const TGUICopyParamType & Value)
{
  FDefaultCopyParam.Assign(&Value);
  Changed();
}

bool TGUIConfiguration::GetRememberPassword() const
{
  bool Result = GetSessionRememberPassword() || GetPuttyPassword();

  if (!Result)
  {
    try
    {
      TRemoteCustomCommand RemoteCustomCommand;
      TInteractiveCustomCommand InteractiveCustomCommand(&RemoteCustomCommand);
      UnicodeString APuttyPath = InteractiveCustomCommand.Complete(FPuttyPath, false);
      Result = RemoteCustomCommand.IsPasswordCommand(FPuttyPath);
    }
    catch (...)
    {
      // noop
    }
  }

  return Result;
}

TCopyParamList * TGUIConfiguration::GetCopyParamList() const
{
  return FCopyParamList;
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

intptr_t TGUIConfiguration::GetCopyParamIndex() const
{
  intptr_t Result;
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

void TGUIConfiguration::SetCopyParamIndex(intptr_t Value)
{
  UnicodeString Name;
  if (Value < 0)
  {
    Name.Clear();
  }
  else
  {
    Name = FCopyParamList->GetName(Value);
  }
  SetCopyParamCurrent(Name);
}

void TGUIConfiguration::SetCopyParamCurrent(UnicodeString Value)
{
  SET_CONFIG_PROPERTY(CopyParamCurrent);
}

TGUICopyParamType TGUIConfiguration::GetCurrentCopyParam() const
{
  return GetCopyParamPreset(GetCopyParamCurrent());
}

TGUICopyParamType TGUIConfiguration::GetCopyParamPreset(UnicodeString Name) const
{
  TGUICopyParamType Result = FDefaultCopyParam;
  if (!Name.IsEmpty())
  {
    intptr_t Index = FCopyParamList->IndexOfName(Name);
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

bool TGUIConfiguration::GetHasCopyParamPreset(UnicodeString Name) const
{
  return Name.IsEmpty() || (FCopyParamList->IndexOfName(Name) >= 0);
}

void TGUIConfiguration::SetNewDirectoryProperties(
  const TRemoteProperties & Value)
{
  SET_CONFIG_PROPERTY(NewDirectoryProperties);
}

void TGUIConfiguration::SetQueueTransfersLimit(intptr_t Value)
{
  SET_CONFIG_PROPERTY(QueueTransfersLimit);
}

void TGUIConfiguration::SetQueueKeepDoneItems(bool Value)
{
  SET_CONFIG_PROPERTY(QueueKeepDoneItems);
}

void TGUIConfiguration::SetQueueKeepDoneItemsFor(intptr_t Value)
{
  SET_CONFIG_PROPERTY(QueueKeepDoneItemsFor);
}

TStoredSessionList * TGUIConfiguration::SelectPuttySessionsForImport(
  TStoredSessionList * Sessions, UnicodeString & /*Error*/)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(new TStoredSessionList(true));
  ImportSessionList->SetDefaultSettings(Sessions->GetDefaultSettings());

  std::unique_ptr<TRegistryStorage> Storage(new TRegistryStorage(GetPuttySessionsKey()));
  Storage->SetForceAnsi(true);
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
    // Error = FMTLOAD(PUTTY_NO_SITES, PuttySessionsKey.c_str());
  }

  return ImportSessionList.release();
}

bool TGUIConfiguration::AnyPuttySessionForImport(TStoredSessionList * Sessions)
{
  try
  {
    UnicodeString Error;
    std::unique_ptr<TStoredSessionList> SessionsList(SelectPuttySessionsForImport(Sessions, Error));
    return (SessionsList->GetCount() > 0);
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

void TGUIConfiguration::SetPuttyPath(UnicodeString Value)
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

void TGUIConfiguration::SetPuttySession(UnicodeString Value)
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

void TGUIConfiguration::SetChecksumAlg(UnicodeString Value)
{
  FChecksumAlg = Value;
}

TGUIConfiguration * GetGUIConfiguration()
{
  return dyn_cast<TGUIConfiguration>(GetConfiguration());
}

