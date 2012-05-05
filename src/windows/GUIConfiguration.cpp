//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#include <LanguagesDEPfix.hpp>
#else
#include "nbafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#endif
#include "GUIConfiguration.h"
#include "GUITools.h"
#include <Common.h>
#include <FileInfo.h>
#include <TextsCore.h>
#include <Terminal.h>
#include <CoreMain.h>
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
const int ccLocal = ccUser;
const int ccShowResults = ccUser << 1;
const int ccCopyResults = ccUser << 2;
const int ccSet = 0x80000000;
//---------------------------------------------------------------------------
static const unsigned int AdditionaLanguageMask = 0xFFFFFF00;
static const UnicodeString AdditionaLanguagePrefix(L"XX");
//---------------------------------------------------------------------------
/* __fastcall */ TGUICopyParamType::TGUICopyParamType()
  : TCopyParamType()
{
  GUIDefault();
}
//---------------------------------------------------------------------------
/* __fastcall */ TGUICopyParamType::TGUICopyParamType(const TCopyParamType & Source)
  : TCopyParamType(Source)
{
  GUIDefault();
}
//---------------------------------------------------------------------------
/* __fastcall */ TGUICopyParamType::TGUICopyParamType(const TGUICopyParamType & Source)
  : TCopyParamType(Source)
{
  GUIAssign(&Source);
}
//---------------------------------------------------------------------------
void __fastcall TGUICopyParamType::Assign(const TCopyParamType * Source)
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
void __fastcall TGUICopyParamType::GUIAssign(const TGUICopyParamType * Source)
{
  SetQueue(Source->GetQueue());
  SetQueueNoConfirmation(Source->GetQueueNoConfirmation());
  SetQueueIndividually(Source->GetQueueIndividually());
  SetNewerOnly(Source->GetNewerOnly());
}
//---------------------------------------------------------------------------
void __fastcall TGUICopyParamType::Default()
{
  TCopyParamType::Default();

  GUIDefault();
}
//---------------------------------------------------------------------------
void __fastcall TGUICopyParamType::GUIDefault()
{
  SetQueue(false);
  SetQueueNoConfirmation(true);
  SetQueueIndividually(false);
  SetNewerOnly(false);
}
//---------------------------------------------------------------------------
void __fastcall TGUICopyParamType::Load(THierarchicalStorage * Storage)
{
  TCopyParamType::Load(Storage);

  SetQueue(Storage->ReadBool(L"Queue", GetQueue()));
  SetQueueNoConfirmation(Storage->ReadBool(L"QueueNoConfirmation", GetQueueNoConfirmation()));
  SetQueueIndividually(Storage->ReadBool(L"QueueIndividually", GetQueueIndividually()));
  SetNewerOnly(Storage->ReadBool(L"NewerOnly", GetNewerOnly()));
}
//---------------------------------------------------------------------------
void __fastcall TGUICopyParamType::Save(THierarchicalStorage * Storage)
{
  TCopyParamType::Save(Storage);

  Storage->WriteBool(L"Queue", GetQueue());
  Storage->WriteBool(L"QueueNoConfirmation", GetQueueNoConfirmation());
  Storage->WriteBool(L"QueueIndividually", GetQueueIndividually());
  Storage->WriteBool(L"NewerOnly", GetNewerOnly());
}
//---------------------------------------------------------------------------
TGUICopyParamType & __fastcall TGUICopyParamType::operator =(const TCopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}
//---------------------------------------------------------------------------
TGUICopyParamType & __fastcall TGUICopyParamType::operator =(const TGUICopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TCopyParamRuleData::Default()
{
  HostName = L"";
  UserName = L"";
  RemoteDirectory = L"";
  LocalDirectory = L"";
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TCopyParamRule::TCopyParamRule()
{
}
//---------------------------------------------------------------------------
/* __fastcall */ TCopyParamRule::TCopyParamRule(const TCopyParamRuleData & Data)
{
  FData = Data;
}
//---------------------------------------------------------------------------
/* __fastcall */ TCopyParamRule::TCopyParamRule(const TCopyParamRule & Source)
{
  FData.HostName = Source.FData.HostName;
  FData.UserName = Source.FData.UserName;
  FData.RemoteDirectory = Source.FData.RemoteDirectory;
  FData.LocalDirectory = Source.FData.LocalDirectory;
}
//---------------------------------------------------------------------------
#define C(Property) (Property == rhp.Property)
bool __fastcall TCopyParamRule::operator==(const TCopyParamRule & rhp) const
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
bool __fastcall TCopyParamRule::Match(const UnicodeString & Mask,
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
bool __fastcall TCopyParamRule::Matches(const TCopyParamRuleData & Value) const
{
  return
    Match(FData.HostName, Value.HostName, false) &&
    Match(FData.UserName, Value.UserName, false) &&
    Match(FData.RemoteDirectory, Value.RemoteDirectory, true, false) &&
    Match(FData.LocalDirectory, Value.LocalDirectory, true, true);
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamRule::Load(THierarchicalStorage * Storage)
{
  FData.HostName = Storage->ReadString(L"HostName", FData.HostName);
  FData.UserName = Storage->ReadString(L"UserName", FData.UserName);
  FData.RemoteDirectory = Storage->ReadString(L"RemoteDirectory", FData.RemoteDirectory);
  FData.LocalDirectory = Storage->ReadString(L"LocalDirectory", FData.LocalDirectory);
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamRule::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteString(L"HostName", FData.HostName);
  Storage->WriteString(L"UserName", FData.UserName);
  Storage->WriteString(L"RemoteDirectory", FData.RemoteDirectory);
  Storage->WriteString(L"LocalDirectory", FData.LocalDirectory);
}
//---------------------------------------------------------------------------
bool __fastcall TCopyParamRule::GetEmpty() const
{
  return
    FData.HostName.IsEmpty() &&
    FData.UserName.IsEmpty() &&
    FData.RemoteDirectory.IsEmpty() &&
    FData.LocalDirectory.IsEmpty();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCopyParamRule::GetInfoStr(UnicodeString Separator) const
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
//---------------------------------------------------------------------------
UnicodeString TCopyParamList::FInvalidChars(L"/\\[]");
//---------------------------------------------------------------------------
/* __fastcall */ TCopyParamList::TCopyParamList()
{
  Init();
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamList::Init()
{
  FCopyParams = new TObjectList();
  FRules = new TObjectList();
  FNames = new TStringList();
  FNameList = NULL;
  FModified = false;
}
//---------------------------------------------------------------------------
/* __fastcall */ TCopyParamList::~TCopyParamList()
{
  Clear();
  delete FCopyParams;
  delete FRules;
  delete FNames;
  delete FNameList;
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamList::Reset()
{
  SAFE_DESTROY(FNameList);
  FModified = false;
}
//---------------------------------------------------------------------
void __fastcall TCopyParamList::Modify()
{
  SAFE_DESTROY(FNameList);
  FModified = true;
}
//---------------------------------------------------------------------
void __fastcall TCopyParamList::ValidateName(const UnicodeString Name)
{
  if (Name.LastDelimiter(FInvalidChars) > 0)
  {
    throw Exception(FMTLOAD(ITEM_NAME_INVALID, Name.c_str(), FInvalidChars.c_str()));
  }
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamList::operator=(const TCopyParamList & rhl)
{
  Clear();

  for (int Index = 0; Index < rhl.GetCount(); Index++)
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
}
//---------------------------------------------------------------------------
bool __fastcall TCopyParamList::operator==(const TCopyParamList & rhl) const
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
int __fastcall TCopyParamList::IndexOfName(const UnicodeString Name) const
{
  return FNames->IndexOf(Name.c_str());
}
//---------------------------------------------------------------------------
bool __fastcall TCopyParamList::CompareItem(int Index,
  const TCopyParamType * CopyParam, const TCopyParamRule * Rule) const
{
  return
    ((*GetCopyParam(Index)) == *CopyParam) &&
    ((GetRule(Index) == NULL) ?
      (Rule == NULL) :
      ((Rule != NULL) && (*GetRule(Index)) == (*Rule)));
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamList::Clear()
{
  for (int i = GetCount() - 1; i != -1; i--)
  {
    FCopyParams->Delete(i);
    FRules->Delete(i);
  }
  FCopyParams->Clear();
  FRules->Clear();
  FNames->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamList::Add(const UnicodeString Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  Insert(GetCount(), Name, CopyParam, Rule);
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamList::Insert(int Index, const UnicodeString Name,
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
void __fastcall TCopyParamList::Change(int Index, const UnicodeString Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  if ((Name != GetName(Index)) || !CompareItem(Index, CopyParam, Rule))
  {
    FNames->PutString(Index, Name);
    delete GetCopyParam(Index);
    FCopyParams->SetItem(Index, (reinterpret_cast<TObject *>(CopyParam)));
    delete GetRule(Index);
    FRules->SetItem(Index, (reinterpret_cast<TObject *>(Rule)));
    Modify();
  }
  else
  {
    delete CopyParam;
    delete Rule;
  }
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamList::Move(int CurIndex, int NewIndex)
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
void __fastcall TCopyParamList::Delete(int Index)
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
int __fastcall TCopyParamList::Find(const TCopyParamRuleData & Value) const
{
  int Result = -1;
  int i = 0;
  while ((i < FRules->GetCount()) && (Result < 0))
  {
    if (FRules->GetItem(i) != NULL)
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
void __fastcall TCopyParamList::Load(THierarchicalStorage * Storage, int ACount)
{
  for (int Index = 0; Index < ACount; Index++)
  {
    UnicodeString Name = IntToStr(Index);
    TCopyParamRule * Rule = NULL;
    TCopyParamType * CopyParam = new TCopyParamType();
    try
    {
      if (Storage->OpenSubKey(Name, false))
      {
        // try
        {
          BOOST_SCOPE_EXIT ( (Storage) )
          {
            Storage->CloseSubKey();
          } BOOST_SCOPE_EXIT_END
          Name = Storage->ReadString(L"Name", Name);
          CopyParam->Load(Storage);

          if (Storage->ReadBool(L"HasRule", false))
          {
            Rule = new TCopyParamRule();
            Rule->Load(Storage);
          }
        }
#ifndef _MSC_VER
        __finally
        {
          Storage->CloseSubKey();
        }
#endif
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
void __fastcall TCopyParamList::Save(THierarchicalStorage * Storage) const
{
  Storage->ClearSubKeys();
  for (int Index = 0; Index < GetCount(); Index++)
  {
    if (Storage->OpenSubKey(IntToStr(Index), true))
    {
      // try
      {
        BOOST_SCOPE_EXIT ( (&Storage) )
        {
          Storage->CloseSubKey();
        } BOOST_SCOPE_EXIT_END
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
#ifndef _MSC_VER
      __finally
      {
        Storage->CloseSubKey();
      }
#endif
    }
  }
}
//---------------------------------------------------------------------------
int __fastcall TCopyParamList::GetCount() const
{
  return FCopyParams->GetCount();
}
//---------------------------------------------------------------------------
const TCopyParamRule * __fastcall TCopyParamList::GetRule(int Index) const
{
  return reinterpret_cast<TCopyParamRule *>(FRules->GetItem(Index));
}
//---------------------------------------------------------------------------
const TCopyParamType * __fastcall TCopyParamList::GetCopyParam(int Index) const
{
  return reinterpret_cast<TCopyParamType *>(FCopyParams->GetItem(Index));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCopyParamList::GetName(int Index) const
{
  return FNames->GetStrings(Index);
}
//---------------------------------------------------------------------------
TStrings * __fastcall TCopyParamList::GetNameList() const
{
  if (FNameList == NULL)
  {
    FNameList = new TStringList();

    for (int i = 0; i < GetCount(); i++)
    {
      FNameList->Add(FNames->GetStrings(i));
    }
  }
  return FNameList;
}
//---------------------------------------------------------------------------
bool __fastcall TCopyParamList::GetAnyRule() const
{
  bool Result = false;
  int i = 0;
  while ((i < GetCount()) && !Result)
  {
    Result = (GetRule(i) != NULL);
    i++;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TGUIConfiguration::TGUIConfiguration(): TConfiguration(),
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
  FSynchronizeBrowsing(false),
  FCopyParamList(NULL),
  FCopyParamListDefaults(false),
  FKeepUpToDateChangeDelay(0),
  FSessionReopenAutoIdle(0),
  FLocale(0)
{
  FLocale = 0;
  FLocales = new TStringList();
  FLastLocalesExts = L"*";
  dynamic_cast<TStringList *>(FLocales)->SetSorted(true);
  dynamic_cast<TStringList *>(FLocales)->SetCaseSensitive(false);
  FCopyParamList = new TCopyParamList();
  CoreSetResourceModule(0);
}
//---------------------------------------------------------------------------
/* __fastcall */ TGUIConfiguration::~TGUIConfiguration()
{
  delete FLocales;
  delete FCopyParamList;
}
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::Default()
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
  FSynchronizeBrowsing = false;

  FNewDirectoryProperties.Default();
  FNewDirectoryProperties.Rights = TRights::rfDefault | TRights::rfExec;
}
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::DefaultLocalized()
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
UnicodeString __fastcall TGUIConfiguration::PropertyToKey(const UnicodeString Property)
{
  // no longer useful
  int P = Property.LastDelimiter(L".>");
  return Property.SubString(P + 1, Property.Length() - P);
}
//---------------------------------------------------------------------------
// duplicated from core\configuration.cpp
#undef BLOCK
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
  { \
      BOOST_SCOPE_EXIT ( (Storage) ) \
      { \
        Storage->CloseSubKey(); \
      } BOOST_SCOPE_EXIT_END \
      BLOCK \
  }
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
    KEY(Bool,     SynchronizeBrowsing); \
  ); \
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::SaveData(THierarchicalStorage * Storage, bool All)
{
  TConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
  #undef KEY
  #define KEY(TYPE, VAR) Storage->Write ## TYPE(PropertyToKey(MB2W(#VAR)), Get##VAR())
  REGCONFIG(true);
  #undef KEY

  if (Storage->OpenSubKey(L"Interface\\CopyParam", true, true))
  // try
  {
    BOOST_SCOPE_EXIT ( (Storage) )
    {
      Storage->CloseSubKey();
    } BOOST_SCOPE_EXIT_END
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
#ifndef _MSC_VER
  __finally
  {
    Storage->CloseSubKey();
  }
#endif

  if (Storage->OpenSubKey(L"Interface\\NewDirectory", true, true))
  // try
  {
    BOOST_SCOPE_EXIT ( (Storage) )
    {
      Storage->CloseSubKey();
    } BOOST_SCOPE_EXIT_END
    FNewDirectoryProperties.Save(Storage);
  }
#ifndef _MSC_VER
  __finally
  {
    Storage->CloseSubKey();
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
  #undef KEY
  #define KEY(TYPE, VAR) Set##VAR(Storage->Read ## TYPE(PropertyToKey(MB2W(#VAR)), Get##VAR()))
  // #pragma warn -eas
  REGCONFIG(false);
  // #pragma warn +eas
  #undef KEY

  if (Storage->OpenSubKey(L"Interface\\CopyParam", false, true))
  // try
  {
    BOOST_SCOPE_EXIT ( (Storage) )
    {
      Storage->CloseSubKey();
    } BOOST_SCOPE_EXIT_END
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
#ifndef _MSC_VER
  __finally
  {
    Storage->CloseSubKey();
  }
#endif

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
  // try
  {
    BOOST_SCOPE_EXIT ( (Storage) )
    {
      Storage->CloseSubKey();
    } BOOST_SCOPE_EXIT_END
    FNewDirectoryProperties.Load(Storage);
  }
#ifndef _MSC_VER
  __finally
  {
    Storage->CloseSubKey();
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::Saved()
{
  TConfiguration::Saved();

  FCopyParamList->Reset();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
HANDLE __fastcall TGUIConfiguration::LoadNewResourceModule(LCID ALocale,
  UnicodeString * FileName)
{
  UnicodeString LibraryFileName;
  HANDLE NewInstance = 0;
  bool Internal = (ALocale == InternalLocale());
  if (!Internal)
  {
    UnicodeString Module;
    UnicodeString LocaleName;

    Module = ModuleFileName();
    if ((ALocale & AdditionaLanguageMask) != AdditionaLanguageMask)
    {
      LOCALESIGNATURE LocSig;
      GetLocaleInfo(ALocale, LOCALE_SABBREVLANGNAME, (LPWSTR)&LocSig, LENOF(&LocSig));
      LocaleName = *(LPWSTR)&LocSig;
      assert(!LocaleName.IsEmpty());
    }
    else
    {
      LocaleName = AdditionaLanguagePrefix +
        static_cast<wchar_t>(ALocale & ~AdditionaLanguageMask);
    }

    Module = ChangeFileExt(Module, UnicodeString(L".") + LocaleName);
    // Look for a potential language/country translation
    NewInstance = LoadLibraryEx(Module.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
    if (!NewInstance)
    {
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
      LibraryFileName = Module;
    }
  }

  if (!NewInstance && !Internal)
  {
    throw Exception(FMTLOAD(LOCALE_LOAD_ERROR, static_cast<int>(ALocale)));
  }
  else
  {
    if (Internal)
    {
      Error(SNotImplemented, 90);
      NewInstance = 0; // FIXME  HInstance;
    }
  }

  if (FileName != NULL)
  {
    *FileName = LibraryFileName;
  }

  return NewInstance;
}
//---------------------------------------------------------------------------
LCID __fastcall TGUIConfiguration::InternalLocale()
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
    assert(false);
    Result = 0;
  }
  return Result;
}
//---------------------------------------------------------------------------
LCID __fastcall TGUIConfiguration::GetLocale()
{
  if (!FLocale)
  {
    FLocale = InternalLocale();
  }
  return FLocale;
}
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::SetLocale(LCID value)
{
  if (GetLocale() != value)
  {
    HANDLE Module = LoadNewResourceModule(value);
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
void __fastcall TGUIConfiguration::SetLocaleSafe(LCID value)
{
  if (GetLocale() != value)
  {
    HANDLE Module;

    try
    {
      Module = LoadNewResourceModule(value);
    }
    catch(...)
    {
      // ignore any exception while loading locale
      Module = NULL;
    }

    if (Module != NULL)
    {
      FLocale = value;
      // SetResourceModule(Module);
    }
  }
}
#ifndef _MSC_VER
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::FreeResourceModule(HANDLE Instance)
{
  TLibModule * MainModule = FindModule(HInstance);
  if ((long)Instance != MainModule->instance)
  {
    FreeLibrary(static_cast<HMODULE>(Instance));
  }
}
//---------------------------------------------------------------------------
HANDLE __fastcall TGUIConfiguration::ChangeResourceModule(HANDLE Instance)
{
  if (Instance == NULL)
  {
    Instance = HInstance;
  }
  TLibModule * MainModule = FindModule(HInstance);
  HANDLE Result = (HANDLE)MainModule->resinstance;
  MainModule->resinstance = (long)Instance;
  CoreSetResourceModule(Instance);
  return Result;
}
//---------------------------------------------------------------------------
HANDLE __fastcall TGUIConfiguration::GetResourceModule()
{
  return (HANDLE)FindModule(HInstance)->resinstance;
}
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::SetResourceModule(HINSTANCE Instance)
{
  HANDLE PrevHandle = ChangeResourceModule(Instance);
  FreeResourceModule(PrevHandle);

  DefaultLocalized();
}
#endif
//---------------------------------------------------------------------------
TStrings * __fastcall TGUIConfiguration::GetLocales()
{
  Error(SNotImplemented, 93);
  UnicodeString LocalesExts;
  TStringList * Exts = new TStringList();
  // try
  {
    BOOST_SCOPE_EXIT ( (Exts) )
    {
      delete Exts;
    } BOOST_SCOPE_EXIT_END
    Exts->SetSorted(true);
    Exts->SetCaseSensitive(false);

    int FindAttrs = faReadOnly | faArchive;
    TSearchRec SearchRec;
    bool Found;

    Found = (bool)(FindFirst(ChangeFileExt(ModuleFileName(), L".*"),
      FindAttrs, SearchRec) == 0);
    // try
    {
      BOOST_SCOPE_EXIT ( (&SearchRec) )
      {
        FindClose(SearchRec);
      } BOOST_SCOPE_EXIT_END
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
#ifndef _MSC_VER
    __finally
    {
      FindClose(SearchRec);
    }
#endif

    if (FLastLocalesExts != LocalesExts)
    {
      FLastLocalesExts = LocalesExts;
      FLocales->Clear();
      /* // FIXME
            TLanguages * Langs = NULL; // FIXME LanguagesDEPF();
            int Ext, Index, Count;
            char LocaleStr[255];
            LCID Locale;

            Count = Langs->GetCount();
            Index = -1;
            while (Index < Count)
            {
              if (Index >= 0)
              {
                Locale = Langs->LocaleID[Index];
                Ext = Exts->IndexOf(Langs->Ext[Index]);
                if (Ext < 0)
                {
                  Ext = Exts->IndexOf(Langs->Ext[Index].substr(0, 2));
                  if (Ext >= 0)
                  {
                    Locale = MAKELANGID(PRIMARYLANGID(Locale), SUBLANG_DEFAULT);
                  }
                }

                if (Ext >= 0)
                {
                  Exts->SetObject(Ext, reinterpret_cast<TObject*>(Locale));
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
              Index++;
            }
      */
      for (int Index = 0; Index < Exts->GetCount(); Index++)
      {
        if ((Exts->GetObjects(Index) == NULL) &&
            (Exts->GetStrings(Index).Length() == 3) &&
            SameText(Exts->GetStrings(Index).SubString(1, 2), AdditionaLanguagePrefix))
        {
          UnicodeString LangName = GetFileFileInfoString(L"LangName",
            ChangeFileExt(ModuleFileName(), UnicodeString(L".") + Exts->GetStrings(Index)));
          if (!LangName.IsEmpty())
          {
            FLocales->AddObject(LangName, reinterpret_cast<TObject *>(static_cast<size_t>(
              AdditionaLanguageMask + Exts->GetStrings(Index)[2])));
          }
        }
      }
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete Exts;
  }
#endif

  return FLocales;
}
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::SetDefaultCopyParam(const TGUICopyParamType & value)
{
  FDefaultCopyParam.Assign(&value);
  Changed();
}
//---------------------------------------------------------------------------
bool __fastcall TGUIConfiguration::GetRememberPassword()
{
  return GetQueueRememberPassword() || GetPuttyPassword();
}
//---------------------------------------------------------------------------
const TCopyParamList * __fastcall TGUIConfiguration::GetCopyParamList()
{
  return FCopyParamList;
}
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::SetCopyParamList(const TCopyParamList * value)
{
  if (!(*FCopyParamList == *value))
  {
    *FCopyParamList = *value;
    FCopyParamListDefaults = false;
    Changed();
  }
}
//---------------------------------------------------------------------------
int __fastcall TGUIConfiguration::GetCopyParamIndex()
{
  int Result;
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
void __fastcall TGUIConfiguration::SetCopyParamIndex(int value)
{
  UnicodeString Name;
  if (value < 0)
  {
    Name = L"";
  }
  else
  {
    Name = FCopyParamList->GetName(value);
  }
  SetCopyParamCurrent(Name);
}
//---------------------------------------------------------------------------
void __fastcall TGUIConfiguration::SetCopyParamCurrent(UnicodeString value)
{
  SET_CONFIG_PROPERTY(CopyParamCurrent);
}
//---------------------------------------------------------------------------
TGUICopyParamType __fastcall TGUIConfiguration::GetCurrentCopyParam()
{
  return GetCopyParamPreset(GetCopyParamCurrent());
}
//---------------------------------------------------------------------------
TGUICopyParamType __fastcall TGUIConfiguration::GetCopyParamPreset(UnicodeString Name)
{
  TGUICopyParamType Result = FDefaultCopyParam;
  if (!Name.IsEmpty())
  {
    int Index = FCopyParamList->IndexOfName(Name);
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
void __fastcall TGUIConfiguration::SetNewDirectoryProperties(
  const TRemoteProperties & value)
{
  SET_CONFIG_PROPERTY(NewDirectoryProperties);
}
//---------------------------------------------------------------------------
