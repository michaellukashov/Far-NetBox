//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
const int ccLocal = ccUser;
const int ccShowResults = ccUser << 1;
const int ccCopyResults = ccUser << 2;
const int ccSet = 0x80000000;
//---------------------------------------------------------------------------
static const unsigned int AdditionaLanguageMask = 0xFFFFFF00;
static const UnicodeString AdditionaLanguagePrefix(L"XX");
//---------------------------------------------------------------------------
TGUICopyParamType::TGUICopyParamType()
  : TCopyParamType()
{
  GUIDefault();
}
//---------------------------------------------------------------------------
TGUICopyParamType::TGUICopyParamType(const TCopyParamType & Source)
  : TCopyParamType(Source)
{
  GUIDefault();
}
//---------------------------------------------------------------------------
TGUICopyParamType::TGUICopyParamType(const TGUICopyParamType & Source)
  : TCopyParamType(Source)
{
  GUIAssign(&Source);
}
//---------------------------------------------------------------------------
void TGUICopyParamType::Assign(const TCopyParamType * Source)
{
  TCopyParamType::Assign(Source);

  const TGUICopyParamType * GUISource;
  GUISource = dynamic_cast<const TGUICopyParamType *>(Source);
  if (GUISource != NULL)
  {
    GUIAssign(GUISource);
  }
}
//---------------------------------------------------------------------------
void TGUICopyParamType::GUIAssign(const TGUICopyParamType * Source)
{
  SetQueue(Source->GetQueue());
  SetQueueNoConfirmation(Source->GetQueueNoConfirmation());
  SetQueueIndividually(Source->GetQueueIndividually());
  SetNewerOnly(Source->GetNewerOnly());
}
//---------------------------------------------------------------------------
void TGUICopyParamType::Default()
{
  GUIDefault();
}
//---------------------------------------------------------------------------
void TGUICopyParamType::GUIDefault()
{
  TCopyParamType::Default();

  SetQueue(false);
  SetQueueNoConfirmation(true);
  SetQueueIndividually(false);
  SetNewerOnly(false);
}
//---------------------------------------------------------------------------
void TGUICopyParamType::Load(THierarchicalStorage * Storage)
{
  CALLSTACK;
  TCopyParamType::Load(Storage);

  SetQueue(Storage->ReadBool(L"Queue", GetQueue()));
  SetQueueNoConfirmation(Storage->ReadBool(L"QueueNoConfirmation", GetQueueNoConfirmation()));
  SetQueueIndividually(Storage->ReadBool(L"QueueIndividually", GetQueueIndividually()));
  SetNewerOnly(Storage->ReadBool(L"NewerOnly", GetNewerOnly()));
}
//---------------------------------------------------------------------------
void TGUICopyParamType::Save(THierarchicalStorage * Storage)
{
  TCopyParamType::Save(Storage);

  Storage->WriteBool(L"Queue", GetQueue());
  Storage->WriteBool(L"QueueNoConfirmation", GetQueueNoConfirmation());
  Storage->WriteBool(L"QueueIndividually", GetQueueIndividually());
  Storage->WriteBool(L"NewerOnly", GetNewerOnly());
}
//---------------------------------------------------------------------------
TGUICopyParamType & TGUICopyParamType::operator =(const TCopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}
//---------------------------------------------------------------------------
TGUICopyParamType & TGUICopyParamType::operator =(const TGUICopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TCopyParamRuleData::Default()
{
  HostName = L"";
  UserName = L"";
  RemoteDirectory = L"";
  LocalDirectory = L"";
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TCopyParamRule::TCopyParamRule()
{
}
//---------------------------------------------------------------------------
TCopyParamRule::TCopyParamRule(const TCopyParamRuleData & Data) :
  FData(Data)
{
}
//---------------------------------------------------------------------------
TCopyParamRule::TCopyParamRule(const TCopyParamRule & Source)
{
  FData.HostName = Source.FData.HostName;
  FData.UserName = Source.FData.UserName;
  FData.RemoteDirectory = Source.FData.RemoteDirectory;
  FData.LocalDirectory = Source.FData.LocalDirectory;
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
bool TCopyParamRule::Match(const UnicodeString & Mask,
  const UnicodeString & Value, bool Path, bool Local) const
{
  bool Result;
  if (Mask.IsEmpty())
  {
    Result = true;
  }
  else
  {
    TFileMasks M(Mask);
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
//---------------------------------------------------------------------------
bool TCopyParamRule::Matches(const TCopyParamRuleData & Value) const
{
  return
    Match(FData.HostName, Value.HostName, false) &&
    Match(FData.UserName, Value.UserName, false) &&
    Match(FData.RemoteDirectory, Value.RemoteDirectory, true, false) &&
    Match(FData.LocalDirectory, Value.LocalDirectory, true, true);
}
//---------------------------------------------------------------------------
void TCopyParamRule::Load(THierarchicalStorage * Storage)
{
  CALLSTACK;
  FData.HostName = Storage->ReadString(L"HostName", FData.HostName);
  FData.UserName = Storage->ReadString(L"UserName", FData.UserName);
  FData.RemoteDirectory = Storage->ReadString(L"RemoteDirectory", FData.RemoteDirectory);
  FData.LocalDirectory = Storage->ReadString(L"LocalDirectory", FData.LocalDirectory);
}
//---------------------------------------------------------------------------
void TCopyParamRule::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteString(L"HostName", FData.HostName);
  Storage->WriteString(L"UserName", FData.UserName);
  Storage->WriteString(L"RemoteDirectory", FData.RemoteDirectory);
  Storage->WriteString(L"LocalDirectory", FData.LocalDirectory);
}
//---------------------------------------------------------------------------
bool TCopyParamRule::GetEmpty() const
{
  return
    FData.HostName.IsEmpty() &&
    FData.UserName.IsEmpty() &&
    FData.RemoteDirectory.IsEmpty() &&
    FData.LocalDirectory.IsEmpty();
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void TCopyParamRule::SetData(TCopyParamRuleData value)
{
  FData = value;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
UnicodeString TCopyParamList::FInvalidChars(L"/\\[]");
//---------------------------------------------------------------------------
TCopyParamList::TCopyParamList()
{
  Init();
}
//---------------------------------------------------------------------------
void TCopyParamList::Init()
{
  FCopyParams = new TList();
  FRules = new TList();
  FNames = new TStringList();
  FNameList = NULL;
  FModified = false;
}
//---------------------------------------------------------------------------
TCopyParamList::~TCopyParamList()
{
  Clear();
  delete FCopyParams;
  delete FRules;
  delete FNames;
  delete FNameList;
}
//---------------------------------------------------------------------------
void TCopyParamList::Reset()
{
  SAFE_DESTROY(FNameList);
  FModified = false;
}
//---------------------------------------------------------------------
void TCopyParamList::Modify()
{
  SAFE_DESTROY(FNameList);
  FModified = true;
}
//---------------------------------------------------------------------
void TCopyParamList::ValidateName(const UnicodeString & Name)
{
  if (Name.LastDelimiter(FInvalidChars) > 0)
  {
    throw Exception(FMTLOAD(ITEM_NAME_INVALID, Name.c_str(), FInvalidChars.c_str()));
  }
}
//---------------------------------------------------------------------------
TCopyParamList & TCopyParamList::operator=(const TCopyParamList & rhl)
{
  Clear();

  for (intptr_t Index = 0; Index < rhl.GetCount(); ++Index)
  {
    TCopyParamType * CopyParam = new TCopyParamType(*rhl.GetCopyParam(Index));
    TCopyParamRule * Rule = NULL;
    if (rhl.GetRule(Index) != NULL)
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
//---------------------------------------------------------------------------
bool TCopyParamList::operator==(const TCopyParamList & rhl) const
{
  bool Result = (GetCount() == rhl.GetCount());
  if (Result)
  {
    int i = 0;
    while ((i < GetCount()) && Result)
    {
      Result =
        (GetName(i) == rhl.GetName(i)) &&
        CompareItem(i, rhl.GetCopyParam(i), rhl.GetRule(i));
      i++;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCopyParamList::IndexOfName(const UnicodeString & Name) const
{
  return FNames->IndexOf(Name.c_str());
}
//---------------------------------------------------------------------------
bool TCopyParamList::CompareItem(intptr_t Index,
  const TCopyParamType * CopyParam, const TCopyParamRule * Rule) const
{
  return
    ((*GetCopyParam(Index)) == *CopyParam) &&
    ((GetRule(Index) == NULL) ?
      (Rule == NULL) :
      ((Rule != NULL) && (*GetRule(Index)) == (*Rule)));
}
//---------------------------------------------------------------------------
void TCopyParamList::Clear()
{
  for (intptr_t i = 0; i < GetCount(); i++)
  {
    delete GetCopyParam(i);
    delete GetRule(i);
  }
  FCopyParams->Clear();
  FRules->Clear();
  FNames->Clear();
}
//---------------------------------------------------------------------------
void TCopyParamList::Add(const UnicodeString & Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  Insert(GetCount(), Name, CopyParam, Rule);
}
//---------------------------------------------------------------------------
void TCopyParamList::Insert(intptr_t Index, const UnicodeString & Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  assert(FNames->IndexOf(Name) < 0);
  FNames->Insert(Index, Name);
  assert(CopyParam != NULL);
  FCopyParams->Insert(Index, reinterpret_cast<TObject *>(CopyParam));
  FRules->Insert(Index, reinterpret_cast<TObject *>(Rule));
  Modify();
}
//---------------------------------------------------------------------------
void TCopyParamList::Change(intptr_t Index, const UnicodeString & Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  if ((Name != GetName(Index)) || !CompareItem(Index, CopyParam, Rule))
  {
    FNames->Strings[Index] = Name;
    delete GetCopyParam(Index);
    FCopyParams->Items(Index, reinterpret_cast<TObject *>(CopyParam));
    delete GetRule(Index);
    FRules->Items(Index, reinterpret_cast<TObject *>(Rule));
    Modify();
  }
  else
  {
    delete CopyParam;
    delete Rule;
  }
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void TCopyParamList::Delete(intptr_t Index)
{
  assert((Index >= 0) && (Index < GetCount()));
  FNames->Delete(Index);
  delete GetCopyParam(Index);
  FCopyParams->Delete(Index);
  delete GetRule(Index);
  FRules->Delete(Index);
  Modify();
}
//---------------------------------------------------------------------------
int TCopyParamList::Find(const TCopyParamRuleData & Value) const
{
  int Result = -1;
  int i = 0;
  while ((i < FRules->GetCount()) && (Result < 0))
  {
    if (FRules->Items[i] != NULL)
    {
      if (GetRule(i)->Matches(Value))
      {
        Result = i;
      }
    }
    i++;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCopyParamList::Load(THierarchicalStorage * Storage, intptr_t ACount)
{
  CALLSTACK;
  for (intptr_t Index = 0; Index < ACount; ++Index)
  {
    UnicodeString Name = IntToStr(Index);
    TCopyParamRule * Rule = NULL;
    TCopyParamType * CopyParam = new TCopyParamType();
    try
    {
      if (Storage->OpenSubKey(Name, false))
      {
        TRY_FINALLY (
        {
          Name = Storage->ReadString(L"Name", Name);
          CopyParam->Load(Storage);

          if (Storage->ReadBool(L"HasRule", false))
          {
            Rule = new TCopyParamRule();
            Rule->Load(Storage);
          }
        }
        ,
        {
          Storage->CloseSubKey();
        }
        );
      }
    }
    catch(...)
    {
      delete CopyParam;
      delete Rule;
      throw;
    }

    FCopyParams->Add(reinterpret_cast<TObject *>(CopyParam));
    FRules->Add(reinterpret_cast<TObject *>(Rule));
    FNames->Add(Name);
  }
  Reset();
}
//---------------------------------------------------------------------------
void TCopyParamList::Save(THierarchicalStorage * Storage) const
{
  Storage->ClearSubKeys();
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (Storage->OpenSubKey(IntToStr(static_cast<int>(Index)), true))
    {
      TRY_FINALLY (
      {
        const TCopyParamType * CopyParam = GetCopyParam(Index);
        const TCopyParamRule * Rule = GetRule(Index);

        Storage->WriteString(L"Name", GetName(Index));
        CopyParam->Save(Storage);
        Storage->WriteBool(L"HasRule", (Rule != NULL));
        if (Rule != NULL)
        {
          Rule->Save(Storage);
        }
      }
      ,
      {
        Storage->CloseSubKey();
      }
      );
    }
  }
}
//---------------------------------------------------------------------------
intptr_t TCopyParamList::GetCount() const
{
  return FCopyParams->GetCount();
}
//---------------------------------------------------------------------------
const TCopyParamRule * TCopyParamList::GetRule(intptr_t Index) const
{
  return reinterpret_cast<TCopyParamRule *>(FRules->Items[Index]);
}
//---------------------------------------------------------------------------
const TCopyParamType * TCopyParamList::GetCopyParam(intptr_t Index) const
{
  return reinterpret_cast<TCopyParamType *>(FCopyParams->Items[Index]);
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamList::GetName(intptr_t Index) const
{
  return FNames->Strings[Index];
}
//---------------------------------------------------------------------------
TStrings * TCopyParamList::GetNameList() const
{
  if (FNameList == NULL)
  {
    FNameList = new TStringList();

    for (intptr_t I = 0; I < GetCount(); ++I)
    {
      FNameList->Add(FNames->Strings[I]);
    }
  }
  return FNameList;
}
//---------------------------------------------------------------------------
bool TCopyParamList::GetAnyRule() const
{
  bool Result = false;
  intptr_t I = 0;
  while ((I < GetCount()) && !Result)
  {
    Result = (GetRule(I) != NULL);
    ++I;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TGUIConfiguration::TGUIConfiguration(): TConfiguration(),
  FLocales(NULL),
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
  FQueueRememberPassword(false),
  FQueueTransfersLimit(0),
  FBeepOnFinish(false),
  FCopyParamList(NULL),
  FCopyParamListDefaults(false),
  FKeepUpToDateChangeDelay(0),
  FSessionReopenAutoIdle(0),
  FLocale(0)
{
  FLocale = 0;
  FLocales = new TStringList();
  FLastLocalesExts = L"*";
  dynamic_cast<TStringList *>(FLocales)->Sorted = true;
  dynamic_cast<TStringList *>(FLocales)->CaseSensitive = false;
  FCopyParamList = new TCopyParamList();
  CoreSetResourceModule(0);
}
//---------------------------------------------------------------------------
TGUIConfiguration::~TGUIConfiguration()
{
  delete FLocales;
  delete FCopyParamList;
}
//---------------------------------------------------------------------------
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
  FQueueAutoPopup = true;
  FQueueRememberPassword = false;
  UnicodeString ProgramsFolder;
  SpecialFolderLocation(CSIDL_PROGRAM_FILES, ProgramsFolder);
  FDefaultPuttyPathOnly = IncludeTrailingBackslash(ProgramsFolder) + L"PuTTY\\putty.exe";
  FDefaultPuttyPath = FormatCommand(L"%PROGRAMFILES%\\PuTTY\\putty.exe", L"");
  FPuttyPath = FDefaultPuttyPath;
  SetPSftpPath(FormatCommand(L"%PROGRAMFILES%\\PuTTY\\psftp.exe", L""));
  FPuttyPassword = false;
  FTelnetForFtpInPutty = true;
  FPuttySession = L"WinSCP temporary session";
  FBeepOnFinish = false;
  FBeepOnFinishAfter = TDateTime(0, 0, 30, 0);
  FCopyParamCurrent = L"";
  FKeepUpToDateChangeDelay = 500;
  FChecksumAlg = L"md5";
  FSessionReopenAutoIdle = 5000;

  FNewDirectoryProperties.Default();
  FNewDirectoryProperties.Rights = TRights::rfDefault | TRights::rfExec;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::DefaultLocalized()
{
  if (FCopyParamListDefaults)
  {
    FCopyParamList->Clear();

    // guard against "empty resource string" from obsolete traslations
    // (DefaultLocalized is called for the first time before detection of
    // obsolete translations)
    if (!LoadStr(COPY_PARAM_PRESET_ASCII).IsEmpty())
    {
      TCopyParamType * CopyParam;

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmAscii);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_ASCII), CopyParam, NULL);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmBinary);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_BINARY), CopyParam, NULL);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->GetIncludeFileMask().SetMasks(L"|*.bak; *.tmp; ~$*; *.wbk; *~; #*; .#*");
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_EXCLUDE), CopyParam, NULL);
    }

    FCopyParamList->Reset();
  }
}
//---------------------------------------------------------------------------
void TGUIConfiguration::UpdateStaticUsage()
{
  CALLSTACK;
  // Usage->Set(L"CopyParamsCount", (FCopyParamListDefaults ? 0 : FCopyParamList->GetCount()));
}
//---------------------------------------------------------------------------
UnicodeString TGUIConfiguration::PropertyToKey(const UnicodeString & Property)
{
  // no longer useful
  intptr_t P = Property.LastDelimiter(L".>");
  return Property.SubString(P + 1, Property.Length() - P);
}
//---------------------------------------------------------------------------
// duplicated from core\configuration.cpp
#undef BLOCK
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) TRY_FINALLY ( { BLOCK  } , { Storage->CloseSubKey(); } );
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
    KEY(Bool,     QueueAutoPopup); \
    KEY(Bool,     QueueRememberPassword); \
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
  ); \
//---------------------------------------------------------------------------
void TGUIConfiguration::SaveData(THierarchicalStorage * Storage, bool All)
{
  TConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
  #undef KEY
  #define KEY(TYPE, NAME) Storage->Write ## TYPE(PropertyToKey(TEXT(#NAME)), Get ## NAME())
  REGCONFIG(true);
  #undef KEY

  if (Storage->OpenSubKey(L"Interface\\CopyParam", true, true))
  {
    TRY_FINALLY (
    {
      FDefaultCopyParam.Save(Storage);

      if (FCopyParamListDefaults)
      {
        assert(!FCopyParamList->GetModified());
        Storage->WriteInteger(L"CopyParamList", -1);
      }
      else if (All || FCopyParamList->GetModified())
      {
        Storage->WriteInteger(L"CopyParamList", static_cast<int>(FCopyParamList->GetCount()));
        FCopyParamList->Save(Storage);
      }
    }
    ,
    {
      Storage->CloseSubKey();
    }
    );
  }

  if (Storage->OpenSubKey(L"Interface\\NewDirectory", true, true))
  {
    TRY_FINALLY (
    {
      FNewDirectoryProperties.Save(Storage);
    }
    ,
    {
      Storage->CloseSubKey();
    }
    );
  }
}
//---------------------------------------------------------------------------
void TGUIConfiguration::LoadData(THierarchicalStorage * Storage)
{
  CALLSTACK;
  TConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
  #undef KEY
  #define KEY(TYPE, NAME) Set ## NAME(Storage->Read ## TYPE(PropertyToKey(TEXT(#NAME)), Get ## NAME()))
  #pragma warn -eas
  REGCONFIG(false);
  #pragma warn +eas
  #undef KEY

  if (Storage->OpenSubKey(L"Interface\\CopyParam", false, true))
  TRY_FINALLY (
  {
    // must be loaded before eventual setting defaults for CopyParamList
    FDefaultCopyParam.Load(Storage);

    int CopyParamListCount = Storage->ReadInteger(L"CopyParamList", -1);
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
  }
  ,
  {
    Storage->CloseSubKey();
  }
  );

  // Make it compatible with versions prior to 3.7.1 that have not saved PuttyPath
  // with quotes. First check for absence of quotes.
  // Add quotes either if the path is set to default putty path (even if it does
  // not exists) or when the path points to existing file (so there are no parameters
  // yet in the string). Note that FileExists may display error dialog, but as
  // it should be called only for custom users path, let's expect that the user
  // can take care of it.
  if ((FPuttyPath.SubString(1, 1) != L"\"") &&
      (CompareFileName(ExpandEnvironmentVariables(FPuttyPath), FDefaultPuttyPathOnly) ||
       FileExists(ExpandEnvironmentVariables(FPuttyPath))))
  {
    FPuttyPath = FormatCommand(FPuttyPath, L"");
  }

  if (Storage->OpenSubKey(L"Interface\\NewDirectory", false, true))
  {
    TRY_FINALLY (
    {
      FNewDirectoryProperties.Load(Storage);
    }
    ,
    {
      Storage->CloseSubKey();
    }
    );
  }
}
//---------------------------------------------------------------------------
void TGUIConfiguration::Saved()
{
  TConfiguration::Saved();

  FCopyParamList->Reset();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
HINSTANCE TGUIConfiguration::LoadNewResourceModule(LCID ALocale,
  UnicodeString * FileName)
{
  CALLSTACK;
  UnicodeString LibraryFileName;
  HINSTANCE NewInstance = 0;
  bool Internal = (ALocale == InternalLocale());
  if (!Internal)
  {
    TRACE("1");
    UnicodeString Module;
    UnicodeString LocaleName;

    Module = ModuleFileName();
    TRACEFMT("2 [%s]", Module.c_str());
    if ((ALocale & AdditionaLanguageMask) != AdditionaLanguageMask)
    {
      TRACE("3");
      LOCALESIGNATURE LocSig;
      GetLocaleInfo(ALocale, LOCALE_SABBREVLANGNAME, reinterpret_cast<LPWSTR>(&LocSig), sizeof(LocSig) / sizeof(TCHAR));
      LocaleName = *reinterpret_cast<LPWSTR>(&LocSig);
      assert(!LocaleName.IsEmpty());
    }
    else
    {
      TRACE("4");
      LocaleName = AdditionaLanguagePrefix +
        static_cast<wchar_t>(ALocale & ~AdditionaLanguageMask);
    }

    Module = ChangeFileExt(Module, UnicodeString(L".") + LocaleName);
    TRACEFMT("5 [%s]", Module.c_str());
    // Look for a potential language/country translation
    NewInstance = LoadLibraryEx(Module.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
    if (!NewInstance)
    {
      TRACE("6");
      // Finally look for a language only translation
      Module.SetLength(Module.Length() - 1);
      NewInstance = LoadLibraryEx(Module.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
      if (NewInstance)
      {
        LibraryFileName = Module;
      }
    }
    else
    {
      TRACE("7");
      LibraryFileName = Module;
    }
  }

  if (!NewInstance && !Internal)
  {
    TRACE("8");
    throw Exception(FMTLOAD(LOCALE_LOAD_ERROR, static_cast<int>(ALocale)));
  }
  else
  {
    TRACE("9");
    if (Internal)
    {
      TRACE("10");
      Classes::Error(SNotImplemented, 90);
      NewInstance = 0; // FIXME  HInstance;
    }
  }

  if (FileName != NULL)
  {
    TRACE("11");
    *FileName = LibraryFileName;
  }

  return NewInstance;
}
//---------------------------------------------------------------------------
LCID TGUIConfiguration::InternalLocale()
{
  CALLSTACK;
  LCID Result;
  if (GetTranslationCount(GetApplicationInfo()) > 0)
  {
    TRACE("TGUIConfiguration::InternalLocale 1");
    TTranslation Translation;
    Translation = GetTranslation(GetApplicationInfo(), 0);
    TRACE("TGUIConfiguration::InternalLocale 2");
    Result = MAKELANGID(PRIMARYLANGID(Translation.Language), SUBLANG_DEFAULT);
    TRACE("TGUIConfiguration::InternalLocale 3");
  }
  else
  {
    TRACE("TGUIConfiguration::InternalLocale 4");
    assert(false);
    Result = 0;
  }
  return Result;
}
//---------------------------------------------------------------------------
LCID TGUIConfiguration::GetLocale()
{
  CALLSTACK;
  if (!FLocale)
  {
    TRACE("TGUIConfiguration::GetLocale 1");
    FLocale = InternalLocale();
  }
  return FLocale;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetLocale(LCID value)
{
  if (GetLocale() != value)
  {
    HINSTANCE Module = LoadNewResourceModule(value);
    if (Module != NULL)
    {
      FLocale = value;
      // SetResourceModule(Module);
    }
    else
    {
      assert(false);
    }
  }
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetLocaleSafe(LCID value)
{
  CALLSTACK;
  if (GetLocale() != value)
  {
    TRACE("1");
    HINSTANCE Module;

    try
    {
      TRACE("2");
      Module = LoadNewResourceModule(value);
      TRACE("3");
    }
    catch(...)
    {
      TRACE("4");
      // ignore any exception while loading locale
      Module = NULL;
    }

    TRACE("5");
    if (Module != NULL)
    {
      TRACE("6");
      FLocale = value;
      // SetResourceModule(Module);
      TRACE("7");
    }
  }
}
#ifndef _MSC_VER
//---------------------------------------------------------------------------
void TGUIConfiguration::FreeResourceModule(HANDLE Instance)
{
  CALLSTACK;
  TLibModule * MainModule = FindModule(HInstance);
  if ((unsigned)Instance != MainModule->Instance)
  {
    FreeLibrary(static_cast<HMODULE>(Instance));
  }
}
//---------------------------------------------------------------------------
HANDLE TGUIConfiguration::ChangeResourceModule(HANDLE Instance)
{
  CALLSTACK;
  if (Instance == NULL)
  {
    Instance = HInstance;
  }
  TLibModule * MainModule = FindModule(HInstance);
  HANDLE Result = (HANDLE)MainModule->ResInstance;
  MainModule->ResInstance = (unsigned)Instance;
  CoreSetResourceModule(Instance);
  return Result;
}
//---------------------------------------------------------------------------
HANDLE TGUIConfiguration::GetResourceModule()
{
  return (HANDLE)FindModule(HInstance)->ResInstance;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetResourceModule(HINSTANCE Instance)
{
  HANDLE PrevHandle = ChangeResourceModule(Instance);
  FreeResourceModule(PrevHandle);

  DefaultLocalized();
}
#endif
//---------------------------------------------------------------------------
TStrings * TGUIConfiguration::GetLocales()
{
  CALLSTACK;
  Classes::Error(SNotImplemented, 93);
  UnicodeString LocalesExts;
  TStringList * Exts = new TStringList();
  TRY_FINALLY (
  {
    TRACE("TGUIConfiguration::GetLocales 1");
    Exts->Sorted = true;
    Exts->CaseSensitive = false;

    int FindAttrs = faReadOnly | faArchive;
    TSearchRec SearchRec;
    bool Found;

    Found = (bool)(FindFirst(ChangeFileExt(ModuleFileName(), L".*"),
      FindAttrs, SearchRec) == 0);
    TRY_FINALLY (
    {
      UnicodeString Ext;
      while (Found)
      {
        Ext = ExtractFileExt(SearchRec.Name).UpperCase();
        if ((Ext.Length() >= 3) && (Ext != L".EXE") && (Ext != L".COM") &&
            (Ext != L".DLL") && (Ext != L".INI"))
        {
          Ext = Ext.SubString(2, Ext.Length() - 1);
          LocalesExts += Ext;
          Exts->Add(Ext);
        }
        Found = (FindNext(SearchRec) == 0);
      }
    }
    ,
    {
      FindClose(SearchRec);
    }
    );

    TRACE("TGUIConfiguration::GetLocales 2");
    if (FLastLocalesExts != LocalesExts)
    {
      FLastLocalesExts = LocalesExts;
      FLocales->Clear();

      /* // FIXME
      TRACE("TGUIConfiguration::GetLocales 3");
      TLanguages * Langs = NULL; // FIXME LanguagesDEPF();
      int Ext, Index, Count;
      wchar_t LocaleStr[255];
      LCID Locale;

      TRACE("TGUIConfiguration::GetLocales 4");
      Count = Langs->GetCount();
      Index = -1;
      while (Index < Count)
      {
        TRACE("TGUIConfiguration::GetLocales 5");
        if (Index >= 0)
        {
          Locale = Langs->LocaleID[Index];
          Ext = Exts->IndexOf(Langs->Ext[Index]);
          if (Ext < 0)
          {
            TRACE("TGUIConfiguration::GetLocales 6");
            Ext = Exts->IndexOf(Langs->Ext[Index].SubString(1, 2));
            if (Ext >= 0)
            {
              Locale = MAKELANGID(PRIMARYLANGID(Locale), SUBLANG_DEFAULT);
            }
          }

          if (Ext >= 0)
          {
            TRACE("TGUIConfiguration::GetLocales 7");
            Exts->Objects[Ext] = reinterpret_cast<TObject*>(Locale);
          }
          else
          {
            Locale = 0;
          }
        }
        else
        {
          TRACE("TGUIConfiguration::GetLocales 8");
          Locale = InternalLocale();
        }

        if (Locale)
        {
          TRACE("TGUIConfiguration::GetLocales 9");
          UnicodeString Name;
          GetLocaleInfo(Locale, LOCALE_SENGLANGUAGE,
            LocaleStr, sizeof(LocaleStr));
          Name = LocaleStr;
          Name += " - ";
          // LOCALE_SNATIVELANGNAME
          GetLocaleInfo(Locale, LOCALE_SLANGUAGE,
            LocaleStr, sizeof(LocaleStr));
          Name += LocaleStr;
          FLocales->AddObject(Name, reinterpret_cast<TObject*>(Locale));
        }
        ++Index;
      }
      */
      TRACE("TGUIConfiguration::GetLocales 10");
      for (intptr_t Index = 0; Index < Exts->GetCount(); ++Index)
      {
        TRACE("TGUIConfiguration::GetLocales 11");
        if ((Exts->Objects[Index] == NULL) &&
            (Exts->Strings[Index].Length() == 3) &&
            SameText(Exts->Strings[Index].SubString(1, 2), AdditionaLanguagePrefix))
        {
          TRACE("TGUIConfiguration::GetLocales 12");
          UnicodeString LangName = GetFileFileInfoString(L"LangName",
            ChangeFileExt(ModuleFileName(), UnicodeString(L".") + Exts->Strings[Index]));
          if (!LangName.IsEmpty())
          {
            TRACE("TGUIConfiguration::GetLocales 13");
            FLocales->AddObject(LangName, reinterpret_cast<TObject *>(static_cast<size_t>(
              AdditionaLanguageMask + Exts->Strings[Index][3])));
          }
        }
      }
    }
  }
  ,
  {
    TRACE("TGUIConfiguration::GetLocales 14");
    delete Exts;
  }
  );

  TRACE("TGUIConfiguration::GetLocales 15");
  return FLocales;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetDefaultCopyParam(const TGUICopyParamType & value)
{
  FDefaultCopyParam.Assign(&value);
  Changed();
}
//---------------------------------------------------------------------------
bool TGUIConfiguration::GetRememberPassword()
{
  return GetQueueRememberPassword() || GetPuttyPassword();
}
//---------------------------------------------------------------------------
const TCopyParamList * TGUIConfiguration::GetCopyParamList()
{
  return FCopyParamList;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetCopyParamList(const TCopyParamList * value)
{
  if (!(*FCopyParamList == *value))
  {
    *FCopyParamList = *value;
    FCopyParamListDefaults = false;
    Changed();
  }
}
//---------------------------------------------------------------------------
intptr_t TGUIConfiguration::GetCopyParamIndex()
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
//---------------------------------------------------------------------------
void TGUIConfiguration::SetCopyParamIndex(int Value)
{
  UnicodeString Name;
  if (Value < 0)
  {
    Name = L"";
  }
  else
  {
    Name = FCopyParamList->GetName(Value);
  }
  SetCopyParamCurrent(Name);
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetCopyParamCurrent(UnicodeString Value)
{
  SET_CONFIG_PROPERTY(CopyParamCurrent);
}
//---------------------------------------------------------------------------
TGUICopyParamType TGUIConfiguration::GetCurrentCopyParam()
{
  return GetCopyParamPreset(GetCopyParamCurrent());
}
//---------------------------------------------------------------------------
TGUICopyParamType TGUIConfiguration::GetCopyParamPreset(UnicodeString Name)
{
  TGUICopyParamType Result = FDefaultCopyParam;
  if (!Name.IsEmpty())
  {
    intptr_t Index = FCopyParamList->IndexOfName(Name);
    assert(Index >= 0);
    if (Index >= 0)
    {
      const TCopyParamType * Preset = FCopyParamList->GetCopyParam(Index);
      assert(Preset != NULL);
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
//---------------------------------------------------------------------------
bool TGUIConfiguration::GetHasCopyParamPreset(UnicodeString Name)
{
  return Name.IsEmpty() || (FCopyParamList->IndexOfName(Name) >= 0);
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetNewDirectoryProperties(
  const TRemoteProperties & Value)
{
  SET_CONFIG_PROPERTY(NewDirectoryProperties);
}
//---------------------------------------------------------------------------
UnicodeString TGUIConfiguration::GetPuttyPath()
{
  return FPuttyPath;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetPuttyPath(const UnicodeString & Value)
{
  FPuttyPath = Value;
}
//---------------------------------------------------------------------------
UnicodeString TGUIConfiguration::GetDefaultPuttyPath()
{
  return FDefaultPuttyPath;
}
//---------------------------------------------------------------------------
UnicodeString TGUIConfiguration::GetPSftpPath()
{
  return FPSftpPath;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetPSftpPath(const UnicodeString & Value)
{
  FPSftpPath = Value;
}
//---------------------------------------------------------------------------
UnicodeString TGUIConfiguration::GetPuttySession()
{
  return FPuttySession;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetPuttySession(UnicodeString Value)
{
  FPuttySession = Value;
}
//---------------------------------------------------------------------------
UnicodeString TGUIConfiguration::GetCopyParamCurrent()
{
  return FCopyParamCurrent;
}
//---------------------------------------------------------------------------
UnicodeString TGUIConfiguration::GetChecksumAlg()
{
  return FChecksumAlg;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetChecksumAlg(const UnicodeString & Value)
{
  FChecksumAlg = Value;
}
//---------------------------------------------------------------------------
