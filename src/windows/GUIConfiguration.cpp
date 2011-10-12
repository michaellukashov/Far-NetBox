//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

// #include <LanguagesDEPfix.hpp>
#include "GUIConfiguration.h"
#include "GUITools.h"
#include <Common.h>
#include <FileInfo.h>
#include <TextsCore.h>
#include <Terminal.h>
#include <CoreMain.h>
//---------------------------------------------------------------------------
const int ccLocal = ccUser;
const int ccShowResults = ccUser << 1;
const int ccCopyResults = ccUser << 2;
const int ccSet = 0x80000000;
//---------------------------------------------------------------------------
static const unsigned int AdditionaLanguageMask = 0xFFFFFF00;
static const std::wstring AdditionaLanguagePrefix(L"XX");
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
TGUICopyParamType::~TGUICopyParamType()
{
    // DEBUG_PRINTF(L"begin");
    // DEBUG_PRINTF(L"end");
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
  TCopyParamType::Default();

  GUIDefault();
}
//---------------------------------------------------------------------------
void TGUICopyParamType::GUIDefault()
{
  SetQueue(false);
  SetQueueNoConfirmation(true);
  SetQueueIndividually(false);
  SetNewerOnly(false);
}
//---------------------------------------------------------------------------
void TGUICopyParamType::Load(THierarchicalStorage * Storage)
{
  TCopyParamType::Load(Storage);

  SetQueue(Storage->Readbool(L"Queue", GetQueue()));
  SetQueueNoConfirmation(Storage->Readbool(L"QueueNoConfirmation", GetQueueNoConfirmation()));
  SetQueueIndividually(Storage->Readbool(L"QueueIndividually", GetQueueIndividually()));
  SetNewerOnly(Storage->Readbool(L"NewerOnly", GetNewerOnly()));
}
//---------------------------------------------------------------------------
void TGUICopyParamType::Save(THierarchicalStorage * Storage)
{
  TCopyParamType::Save(Storage);

  Storage->Writebool(L"Queue", GetQueue());
  Storage->Writebool(L"QueueNoConfirmation", GetQueueNoConfirmation());
  Storage->Writebool(L"QueueIndividually", GetQueueIndividually());
  Storage->Writebool(L"NewerOnly", GetNewerOnly());
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
TCopyParamRule::TCopyParamRule(const TCopyParamRuleData & Data)
{
  FData = Data;
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
bool TCopyParamRule::Match(const std::wstring & Mask,
  const std::wstring & Value, bool Path, bool Local) const
{
  bool Result;
  if (Mask.empty())
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
    FData.HostName.empty() &&
    FData.UserName.empty() &&
    FData.RemoteDirectory.empty() &&
    FData.LocalDirectory.empty();
}
//---------------------------------------------------------------------------
std::wstring TCopyParamRule::GetInfoStr(std::wstring Separator) const
{
  std::wstring Result;
  #define ADD(FMT, ELEM) \
    if (!FData.ELEM.empty()) \
      Result += (Result.empty() ? std::wstring() : Separator) + FMTLOAD(FMT, FData.ELEM.c_str());
  ADD(COPY_RULE_HOSTNAME, HostName);
  ADD(COPY_RULE_USERNAME, UserName);
  ADD(COPY_RULE_REMOTE_DIR, RemoteDirectory);
  ADD(COPY_RULE_LOCAL_DIR, LocalDirectory);
  #undef ADD
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
std::wstring TCopyParamList::FInvalidChars(L"/\\[]");
//---------------------------------------------------------------------------
TCopyParamList::TCopyParamList()
{
  Init();
}
//---------------------------------------------------------------------------
void TCopyParamList::Init()
{
  FCopyParams = new TObjectList();
  FRules = new TObjectList();
  FNames = new TStringList();
  FNameList = NULL;
  FModified = false;
}
//---------------------------------------------------------------------------
TCopyParamList::~TCopyParamList()
{
  // DEBUG_PRINTF(L"begin");
  Clear();
  delete FCopyParams;
  delete FRules;
  delete FNames;
  delete FNameList;
  // DEBUG_PRINTF(L"end");
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
void TCopyParamList::ValidateName(const std::wstring Name)
{
  if (::LastDelimiter(Name, FInvalidChars) > 0)
  {
    throw ExtException(FMTLOAD(ITEM_NAME_INVALID, Name.c_str(), FInvalidChars.c_str()));
  }
}
//---------------------------------------------------------------------------
void TCopyParamList::operator=(const TCopyParamList & rhl)
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
  // there should be comparison of with the assigned list, be we rely on caller
  // to do it instead (TGUIConfiguration::SetCopyParamList)
  Modify();
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
int TCopyParamList::IndexOfName(const std::wstring Name) const
{
  return FNames->IndexOf(Name.c_str());
}
//---------------------------------------------------------------------------
bool TCopyParamList::CompareItem(int Index,
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
  for (int i = 0; i < GetCount(); i++)
  {
    delete GetCopyParam(i);
    delete GetRule(i);
  }
  FCopyParams->Clear();
  FRules->Clear();
  FNames->Clear();
}
//---------------------------------------------------------------------------
void TCopyParamList::Add(const std::wstring Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  Insert(GetCount(), Name, CopyParam, Rule);
}
//---------------------------------------------------------------------------
void TCopyParamList::Insert(int Index, const std::wstring Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  // DEBUG_PRINTF(L"begin");
  assert(FNames->IndexOf(Name) < 0);
  FNames->Insert(Index, Name);
  assert(CopyParam != NULL);
  FCopyParams->Insert(Index, reinterpret_cast<TObject *>(CopyParam));
  FRules->Insert(Index, reinterpret_cast<TObject *>(Rule));
  Modify();
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TCopyParamList::Change(int Index, const std::wstring Name,
  TCopyParamType * CopyParam, TCopyParamRule * Rule)
{
  if ((Name != GetName(Index)) || !CompareItem(Index, CopyParam, Rule))
  {
    FNames->GetString(Index) = Name;
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
void TCopyParamList::Move(int CurIndex, int NewIndex)
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
void TCopyParamList::Delete(int Index)
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
void TCopyParamList::Load(THierarchicalStorage * Storage, int ACount)
{
  for (int Index = 0; Index < ACount; Index++)
  {
    std::wstring Name = IntToStr(Index);
    TCopyParamRule * Rule = NULL;
    TCopyParamType * CopyParam = new TCopyParamType();
    try
    {
      if (Storage->OpenSubKey(Name, false))
      {
        {
            BOOST_SCOPE_EXIT ( (&Storage) )
            {
              Storage->CloseSubKey();
            } BOOST_SCOPE_EXIT_END
          Name = Storage->ReadString(L"Name", Name);
          CopyParam->Load(Storage);

          if (Storage->Readbool(L"HasRule", false))
          {
            Rule = new TCopyParamRule();
            Rule->Load(Storage);
          }
        }
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
  for (int Index = 0; Index < GetCount(); Index++)
  {
    if (Storage->OpenSubKey(IntToStr(Index), true))
    {
      {
        BOOST_SCOPE_EXIT ( (&Storage) )
        {
          Storage->CloseSubKey();
        } BOOST_SCOPE_EXIT_END
        const TCopyParamType * CopyParam = GetCopyParam(Index);
        const TCopyParamRule * Rule = GetRule(Index);

        Storage->WriteString(L"Name", GetName(Index));
        CopyParam->Save(Storage);
        Storage->Writebool(L"HasRule", (Rule != NULL));
        if (Rule != NULL)
        {
          Rule->Save(Storage);
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
int TCopyParamList::GetCount() const
{
  return FCopyParams->GetCount();
}
//---------------------------------------------------------------------------
const TCopyParamRule * TCopyParamList::GetRule(int Index) const
{
  return reinterpret_cast<TCopyParamRule *>(FRules->GetItem(Index));
}
//---------------------------------------------------------------------------
const TCopyParamType * TCopyParamList::GetCopyParam(int Index) const
{
  return reinterpret_cast<TCopyParamType *>(FCopyParams->GetItem(Index));
}
//---------------------------------------------------------------------------
std::wstring TCopyParamList::GetName(int Index) const
{
  return FNames->GetString(Index);
}
//---------------------------------------------------------------------------
TStrings * TCopyParamList::GetNameList() const
{
  if (FNameList == NULL)
  {
    FNameList = new TStringList();

    for (int i = 0; i < GetCount(); i++)
    {
      FNameList->Add(FNames->GetString(i));
    }
  }
  return FNameList;
}
//---------------------------------------------------------------------------
bool TCopyParamList::GetAnyRule() const
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
TGUIConfiguration::TGUIConfiguration(): TConfiguration()
{
  FLocale = 0;
  FLocales = new TStringList();
  FLastLocalesExts = L"*";
  dynamic_cast<TStringList*>(FLocales)->SetSorted(true);
  dynamic_cast<TStringList*>(FLocales)->SetCaseSensitive(false);
  FCopyParamList = new TCopyParamList();
  CoreSetResourceModule(GetResourceModule());
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
  // DEBUG_PRINTF(L"begin");
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
  std::wstring ProgramsFolder;
  SpecialFolderLocation(CSIDL_PROGRAM_FILES, ProgramsFolder);
  // DEBUG_PRINTF(L"ProgramsFolder = %s", ProgramsFolder.c_str());
  FDefaultPuttyPathOnly = IncludeTrailingBackslash(ProgramsFolder) + L"PuTTY\\putty.exe";
  // DEBUG_PRINTF(L"FDefaultPuttyPathOnly = %s", FDefaultPuttyPathOnly.c_str());
  FDefaultPuttyPath = FormatCommand(L"%PROGRAMFILES%\\PuTTY\\putty.exe", L"");
  FPuttyPath = FDefaultPuttyPath;
  SetPSftpPath(FormatCommand(L"%PROGRAMFILES%\\PuTTY\\psftp.exe", L""));
  FPuttyPassword = false;
  FTelnetForFtpInPutty = true;
  FPuttySession = L"WinSCP temporary session";
  FBeepOnFinish = false;
  FBeepOnFinishAfter = TDateTime(0, 0, 30, 0);
  FSynchronizeBrowsing = false;
  FCopyParamCurrent = L"";
  FKeepUpToDateChangeDelay = 500;
  FChecksumAlg = L"md5";
  FSessionReopenAutoIdle = 5000;

  FNewDirectoryProperties.Default();
  FNewDirectoryProperties.Rights = TRights::rfDefault;
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TGUIConfiguration::DefaultLocalized()
{
  // DEBUG_PRINTF(L"begin: FCopyParamListDefaults = %d", FCopyParamListDefaults);
  if (FCopyParamListDefaults)
  {
    FCopyParamList->Clear();

    // guard against "empty resourse string" from obsolete traslations
    // (DefaultLocalized is called for the first time before detection of
    // obsolete translations)
    if (!LoadStr(COPY_PARAM_PRESET_ASCII).empty())
    {
      TCopyParamType * CopyParam;

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmAscii);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_ASCII), CopyParam, NULL);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->SetTransferMode(tmBinary);
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_BINARY), CopyParam, NULL);

      CopyParam = new TCopyParamType(FDefaultCopyParam);
      CopyParam->GetExcludeFileMask().SetMasks(L"*.bak; *.tmp; ~$*; *.wbk; *~; #*; .#*");
      CopyParam->SetNegativeExclude(false); // just for sure
      FCopyParamList->Add(LoadStr(COPY_PARAM_PRESET_EXCLUDE), CopyParam, NULL);
    }

    FCopyParamList->Reset();
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
std::wstring TGUIConfiguration::PropertyToKey(const std::wstring Property)
{
  // no longer useful
  int P = ::LastDelimiter(Property, L".>");
  return Property.substr(P + 1, Property.size() - P);
}
//---------------------------------------------------------------------------
// duplicated from core\configuration.cpp
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
  { \
      BOOST_SCOPE_EXIT ( (&Storage) ) \
      { \
        Storage->CloseSubKey(); \
      } BOOST_SCOPE_EXIT_END \
      BLOCK \
  }
#define REGCONFIG(CANCREATE) \
  BLOCK(L"Interface", CANCREATE, \
    KEY(bool,     ContinueOnError); \
    KEY(bool,     ConfirmCommandSession); \
    KEY(int,  SynchronizeParams); \
    KEY(int,  SynchronizeOptions); \
    KEY(int,  SynchronizeModeAuto); \
    KEY(int,  SynchronizeMode); \
    KEY(int,  MaxWatchDirectories); \
    KEY(int,  QueueTransfersLimit); \
    KEY(bool,     QueueAutoPopup); \
    KEY(bool,     QueueRememberPassword); \
    KEY(String,   PuttySession); \
    KEY(String,   PuttyPath); \
    KEY(bool,     PuttyPassword); \
    KEY(bool,     TelnetForFtpInPutty); \
    KEY(DateTime, IgnoreCancelBeforeFinish); \
    KEY(bool,     BeepOnFinish); \
    KEY(DateTime, BeepOnFinishAfter); \
    KEY(bool,     SynchronizeBrowsing); \
    KEY(int,  KeepUpToDateChangeDelay); \
    KEY(String,   ChecksumAlg); \
    KEY(int,  SessionReopenAutoIdle); \
  ); \
//---------------------------------------------------------------------------
void TGUIConfiguration::SaveData(THierarchicalStorage * Storage, bool All)
{
  TConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) Storage->Write ## TYPE(PropertyToKey(::MB2W("##VAR")), Get##VAR())
  REGCONFIG(true);
  #undef KEY

  if (Storage->OpenSubKey(L"Interface\\CopyParam", true, true))
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      Storage->CloseSubKey();
    } BOOST_SCOPE_EXIT_END
    FDefaultCopyParam.Save(Storage);

    if (FCopyParamListDefaults)
    {
      assert(!FCopyParamList->GetModified());
      Storage->Writeint(L"CopyParamList", -1);
    }
    else if (All || FCopyParamList->GetModified())
    {
      Storage->Writeint(L"CopyParamList", FCopyParamList->GetCount());
      FCopyParamList->Save(Storage);
    }
  }

  if (Storage->OpenSubKey(L"Interface\\NewDirectory", true, true))
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      Storage->CloseSubKey();
    } BOOST_SCOPE_EXIT_END
    FNewDirectoryProperties.Save(Storage);
  }
}
//---------------------------------------------------------------------------
void TGUIConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TConfiguration::LoadData(Storage);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) Set##VAR(Storage->Read ## TYPE(PropertyToKey(::MB2W("##VAR")), Get##VAR()))
  // #pragma warn -eas
  REGCONFIG(false);
  // #pragma warn +eas
  #undef KEY

  if (Storage->OpenSubKey(L"Interface\\CopyParam", false, true))
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      Storage->CloseSubKey();
    } BOOST_SCOPE_EXIT_END
    // must be loaded before eventual setting defaults for CopyParamList
    FDefaultCopyParam.Load(Storage);

    int CopyParamListCount = Storage->Readint(L"CopyParamList", -1);
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

  // Make it compatible with versions prior to 3.7.1 that have not saved PuttyPath
  // with quotes. First check for absence of quotes.
  // Add quotes either if the path is set to default putty path (even if it does
  // not exists) or when the path points to existing file (so there are no parameters
  // yet in the string). Note that FileExists may display error dialog, but as
  // it should be called only for custom users path, let's expect that the user
  // can take care of it.
  if ((FPuttyPath.substr(0, 1) != L"\"") &&
      (CompareFileName(ExpandEnvironmentVariables(FPuttyPath), FDefaultPuttyPathOnly) ||
       FileExists(ExpandEnvironmentVariables(FPuttyPath))))
  {
    FPuttyPath = FormatCommand(FPuttyPath, L"");
  }

  if (Storage->OpenSubKey(L"Interface\\NewDirectory", false, true))
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      Storage->CloseSubKey();
    } BOOST_SCOPE_EXIT_END
    FNewDirectoryProperties.Load(Storage);
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
HANDLE TGUIConfiguration::LoadNewResourceModule(LCID ALocale,
  std::wstring * FileName)
{
  std::wstring LibraryFileName;
  HANDLE NewInstance = 0;
  bool Internal = (ALocale == InternalLocale());
  if (!Internal)
  {
    std::wstring Module;
    std::wstring LocaleName;

    Module = ModuleFileName();
    if ((ALocale & AdditionaLanguageMask) != AdditionaLanguageMask)
    {
      wchar_t LocaleStr[4];
      GetLocaleInfo(ALocale, LOCALE_SABBREVLANGNAME, LocaleStr, sizeof(LocaleStr));
      LocaleName = LocaleStr;
      assert(!LocaleName.empty());
    }
    else
    {
      LocaleName = AdditionaLanguagePrefix +
        wchar_t(ALocale & ~AdditionaLanguageMask);
    }

    Module = ChangeFileExt(Module, std::wstring(L".") + LocaleName);
    // Look for a potential language/country translation
    NewInstance = LoadLibraryEx(Module.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
    if (!NewInstance)
    {
      // Finally look for a language only translation
      Module.resize(Module.size() - 1);
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
    throw ExtException(FMTLOAD(LOCALE_LOAD_ERROR, int(ALocale)));
  }
  else
  {
    if (Internal)
    {
      ::Error(SNotImplemented, 90);
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
LCID TGUIConfiguration::InternalLocale()
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
LCID TGUIConfiguration::GetLocale()
{
  if (!FLocale)
  {
    FLocale = InternalLocale();
  }
  return FLocale;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetLocale(LCID value)
{
  if (GetLocale() != value)
  {
    HANDLE Module = LoadNewResourceModule(value);
    if (Module != NULL)
    {
      FLocale = value;
      SetResourceModule(Module);
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
      SetResourceModule(Module);
    }
  }
}
//---------------------------------------------------------------------------
void TGUIConfiguration::FreeResourceModule(HANDLE Instance)
{
  ::Error(SNotImplemented, 91);
  TPasLibModule * MainModule = NULL; // FindModule(0); // FIXME HInstance);
  if (Instance != MainModule->Instance)
  {
    FreeLibrary(static_cast<HMODULE>(Instance));
  }
}
//---------------------------------------------------------------------------
HANDLE TGUIConfiguration::ChangeResourceModule(HANDLE Instance)
{
  ::Error(SNotImplemented, 92);
  if (Instance == NULL)
  {
    Instance = 0; // FIXME HInstance;
  }
  TPasLibModule * MainModule = NULL; // FindModule(0); // FIXME HInstance);
  HANDLE Result = MainModule->ResInstance;
  MainModule->ResInstance = Instance;
  CoreSetResourceModule(Instance);
  return Result;
}
//---------------------------------------------------------------------------
HANDLE TGUIConfiguration::GetResourceModule()
{
  return 0; // FindModule(0/*HInstance*/)->ResInstance;
}
//---------------------------------------------------------------------------
void TGUIConfiguration::SetResourceModule(HANDLE Instance)
{
  HANDLE PrevHandle = ChangeResourceModule(Instance);
  FreeResourceModule(PrevHandle);

  DefaultLocalized();
}
//---------------------------------------------------------------------------
TStrings * TGUIConfiguration::GetLocales()
{
  ::Error(SNotImplemented, 93);
  std::wstring LocalesExts;
  TStringList * Exts = new TStringList();
  {
      BOOST_SCOPE_EXIT ( (&Exts) )
      {
        delete Exts;
      } BOOST_SCOPE_EXIT_END
    Exts->SetSorted(true);
    Exts->SetCaseSensitive(false);

    int FindAttrs = faReadOnly | faArchive;
    // TSearchRec SearchRec;
    WIN32_FIND_DATA SearchRec;
    bool Found;

    Found = false; // FIXME  (bool)(FindFirst(ChangeFileExt(ModuleFileName(), L".*"),
      // FindAttrs, SearchRec) == 0);
    {
        BOOST_SCOPE_EXIT ( (&SearchRec) )
        {
          // FIXME FindClose(SearchRec);
        } BOOST_SCOPE_EXIT_END
      std::wstring Ext;
      while (Found)
      {
        Ext = ::UpperCase(ExtractFileExt(SearchRec.cFileName));
        if ((Ext.size() >= 3) && (Ext != L".EXE") && (Ext != L".COM") &&
            (Ext != L".DLL") && (Ext != L".INI"))
        {
          Ext = Ext.substr(2, Ext.size() - 1);
          LocalesExts += Ext;
          Exts->Add(Ext);
        }
        Found = false; // FIXME (FindNext(SearchRec) == 0);
      }
    }

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
            Exts->GetObject(Ext) = reinterpret_cast<TObject*>(Locale);
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
          std::wstring Name;
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
        if ((Exts->GetObject(Index) == NULL) &&
            (Exts->GetString(Index).size() == 3) &&
            SameText(Exts->GetString(Index).substr(0, 2), AdditionaLanguagePrefix))
        {
          std::wstring LangName = GetFileFileInfoString(L"LangName",
            ChangeFileExt(ModuleFileName(), std::wstring(L".") + Exts->GetString(Index)));
          if (!LangName.empty())
          {
            FLocales->AddObject(LangName, reinterpret_cast<TObject*>(
              AdditionaLanguageMask + Exts->GetString(Index)[3]));
          }
        }
      }
    }
  }

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
int TGUIConfiguration::GetCopyParamIndex()
{
  int Result;
  if (FCopyParamCurrent.empty())
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
void TGUIConfiguration::SetCopyParamIndex(int value)
{
  std::wstring Name;
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
void TGUIConfiguration::SetCopyParamCurrent(std::wstring value)
{
  SET_CONFIG_PROPERTY(CopyParamCurrent);
}
//---------------------------------------------------------------------------
TGUICopyParamType TGUIConfiguration::GetCurrentCopyParam()
{
  return GetCopyParamPreset(GetCopyParamCurrent());
}
//---------------------------------------------------------------------------
TGUICopyParamType TGUIConfiguration::GetCopyParamPreset(std::wstring Name)
{
  TGUICopyParamType Result = FDefaultCopyParam;
  if (!Name.empty())
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
void TGUIConfiguration::SetNewDirectoryProperties(
  const TRemoteProperties & value)
{
  SET_CONFIG_PROPERTY(NewDirectoryProperties);
}
//---------------------------------------------------------------------------
