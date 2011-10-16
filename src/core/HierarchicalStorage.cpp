//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "HierarchicalStorage.h"
#include <TextsCore.h>
#include <vector>
//---------------------------------------------------------------------------
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch(...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch(...) { FFailed++; }
//---------------------------------------------------------------------------
std::wstring MungeStr(const std::wstring Str)
{
  std::string Result2;
  Result2.resize(Str.size() * 3 + 1);
  putty_mungestr(::W2MB(Str.c_str()).c_str(), (char *)Result2.c_str());
  std::wstring Result = ::MB2W(Result2.c_str());
  PackStr(Result);
  // DEBUG_PRINTF(L"Str = %s, Result = %s", Str.c_str(), Result.c_str());
  return Result;
}
//---------------------------------------------------------------------------
std::wstring UnMungeStr(const std::wstring Str)
{
  std::string Result2;
  Result2.resize(Str.size() * 3 + 1);
  putty_unmungestr((char *)::W2MB(Str.c_str()).c_str(), (char *)Result2.c_str(), Result2.size());
  std::wstring Result = ::MB2W(Result2.c_str());
  PackStr(Result);
  return Result;
}
//---------------------------------------------------------------------------
std::wstring PuttyMungeStr(const std::wstring Str)
{
  return MungeStr(Str);
}
//---------------------------------------------------------------------------
std::wstring MungeIniName(const std::wstring Str)
{
  int P = Str.find_first_of(L"=");
  // make this fast for now
  if (P > 0)
  {
    return ::StringReplace(Str, L"=", L"%3D");
  }
  else
  {
    return Str;
  }
}
//---------------------------------------------------------------------------
std::wstring UnMungeIniName(const std::wstring Str)
{
  int P = Str.find_first_of(L"%3D");
  // make this fast for now
  if (P > 0)
  {
    return ::StringReplace(Str, L"%3D", L"=");
  }
  else
  {
    return Str;
  }
}
//===========================================================================
THierarchicalStorage::THierarchicalStorage(const std::wstring AStorage)
{
  FStorage = AStorage;
  FKeyHistory = new TStringList();
  SetAccessMode(smRead);
  SetExplicit(false);
  SetMungeStringValues(true);
}
//---------------------------------------------------------------------------
THierarchicalStorage::~THierarchicalStorage()
{
  delete FKeyHistory;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::SetAccessMode(TStorageAccessMode value)
{
  FAccessMode = value;
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::GetCurrentSubKeyMunged()
{
  if (FKeyHistory->GetCount()) return FKeyHistory->GetString(FKeyHistory->GetCount()-1);
    else return L"";
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::GetCurrentSubKey()
{
  return UnMungeStr(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::OpenRootKey(bool CanCreate)
{
  return OpenSubKey(L"", CanCreate);
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::MungeSubKey(std::wstring Key, bool Path)
{
  std::wstring Result;
  // DEBUG_PRINTF(L"Key = %s, Path = %d", Key.c_str(), Path);
  if (Path)
  {
    assert(Key.empty() || (Key[Key.size() - 1] != '\\'));
    while (!Key.empty())
    {
      if (!Result.empty())
      {
        Result += '\\';
      }
      Result += MungeStr(CutToChar(Key, L'\\', false));
      // DEBUG_PRINTF(L"Key = %s, Result = %s", Key.c_str(), Result.c_str());
    }
  }
  else
  {
    Result = MungeStr(Key);
  }
  // DEBUG_PRINTF(L"Result = %s", Result.c_str());
  return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::OpenSubKey(const std::wstring SubKey, bool /*CanCreate*/, bool Path)
{
  // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
  FKeyHistory->Add(IncludeTrailingBackslash(GetCurrentSubKey() + MungeSubKey(SubKey, Path)));
  return true;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::CloseSubKey()
{
  if (FKeyHistory->GetCount() == 0)
    throw std::exception("");
  else FKeyHistory->Delete(FKeyHistory->GetCount()-1);
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ClearSubKeys()
{
  TStringList *SubKeys = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&SubKeys) )
    {
      delete SubKeys;
    } BOOST_SCOPE_EXIT_END
    GetSubKeyNames(SubKeys);
    for (int Index = 0; Index < SubKeys->GetCount(); Index++)
    {
      RecursiveDeleteSubKey(SubKeys->GetString(Index));
    }
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::RecursiveDeleteSubKey(const std::wstring Key)
{
  if (OpenSubKey(Key, false))
  {
    ClearSubKeys();
    CloseSubKey();
  }
  DeleteSubKey(Key);
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::HasSubKeys()
{
  bool Result;
  TStrings * SubKeys = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&SubKeys) )
    {
      delete SubKeys;
    } BOOST_SCOPE_EXIT_END
    GetSubKeyNames(SubKeys);
    Result = (SubKeys->GetCount() > 0);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::HasSubKey(const std::wstring SubKey)
{
  bool Result = OpenSubKey(SubKey, false);
  if (Result)
  {
    CloseSubKey();
  }
  return Result;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ReadValues(TStrings* Strings,
  bool MaintainKeys)
{
  TStrings * Names = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&Names) )
    {
      delete Names;
    } BOOST_SCOPE_EXIT_END
    GetValueNames(Names);
    for (int Index = 0; Index < Names->GetCount(); Index++)
    {
      if (MaintainKeys)
      {
        Strings->Add(FORMAT(L"%s=%s", (Names->GetString(Index),
          ReadString(Names->GetString(Index), L""))));
      }
      else
      {
        Strings->Add(ReadString(Names->GetString(Index), L""));
      }
    }
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ClearValues()
{
  TStrings * Names = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&Names) )
    {
      delete Names;
    } BOOST_SCOPE_EXIT_END
    GetValueNames(Names);
    for (int Index = 0; Index < Names->GetCount(); Index++)
    {
      DeleteValue(Names->GetString(Index));
    }
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteValues(TStrings * Strings,
  bool MaintainKeys)
{
  ClearValues();

  if (Strings)
  {
    for (int Index = 0; Index < Strings->GetCount(); Index++)
    {
      if (MaintainKeys)
      {
        assert(Strings->GetString(Index).find_first_of(L"=") > 1);
        WriteString(Strings->GetName(Index), Strings->GetValue(Strings->GetName(Index)));
      }
      else
      {
        WriteString(IntToStr(Index), Strings->GetString(Index));
      }
    }
  }
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::ReadString(const std::wstring Name, const std::wstring Default)
{
  std::wstring Result;
  if (GetMungeStringValues())
  {
    Result = UnMungeStr(ReadStringRaw(Name, MungeStr(Default)));
  }
  else
  {
    Result = ReadStringRaw(Name, Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::ReadBinaryData(const std::wstring Name)
{
  int Size = BinaryDataSize(Name);
  std::wstring Value;
  Value.resize(Size);
  ReadBinaryData(Name, (void *)Value.c_str(), Size);
  return Value;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteString(const std::wstring Name, const std::wstring Value)
{
  // DEBUG_PRINTF(L"GetMungeStringValues = %d", GetMungeStringValues());
  if (GetMungeStringValues())
  {
    // DEBUG_PRINTF(L"Name = %s, Value = %s, MungeStr(Value) = %s", Name.c_str(), Value.c_str(), MungeStr(Value).c_str());
    WriteStringRaw(Name, MungeStr(Value));
  }
  else
  {
    // DEBUG_PRINTF(L"Value = %s", Value.c_str());
    WriteStringRaw(Name, Value);
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteBinaryData(const std::wstring Name,
  const std::wstring Value)
{
  WriteBinaryData(Name, Value.c_str(), Value.size());
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::IncludeTrailingBackslash(const std::wstring & S)
{
  // expanded from ?: as it caused memory leaks
  if (S.empty())
  {
    return S;
  }
  else
  {
    return ::IncludeTrailingBackslash(S);
  }
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::ExcludeTrailingBackslash(const std::wstring & S)
{
  // expanded from ?: as it caused memory leaks
  if (S.empty())
  {
    return S;
  }
  else
  {
    return ::ExcludeTrailingBackslash(S);
  }
}
//===========================================================================
TRegistryStorage::TRegistryStorage(const std::wstring AStorage):
  THierarchicalStorage(IncludeTrailingBackslash(AStorage))
{
  Init();
};
//---------------------------------------------------------------------------
TRegistryStorage::TRegistryStorage(const std::wstring AStorage, HKEY ARootKey):
  THierarchicalStorage(IncludeTrailingBackslash(AStorage))
{
  Init();
  FRegistry->SetRootKey(ARootKey);
}
//---------------------------------------------------------------------------
void TRegistryStorage::Init()
{
  FFailed = 0;
  FRegistry = new TRegistry;
  FRegistry->SetAccess(KEY_READ);
}
//---------------------------------------------------------------------------
TRegistryStorage::~TRegistryStorage()
{
  delete FRegistry;
};
//---------------------------------------------------------------------------
bool TRegistryStorage::Copy(TRegistryStorage * Storage)
{
  TRegistry * Registry = Storage->FRegistry;
  bool Result = true;
  TStrings * Names = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&Names) )
    {
      delete Names;
    } BOOST_SCOPE_EXIT_END
    Registry->GetValueNames(Names);
    std::vector<unsigned char> Buffer(1024, 0);
    int Index = 0;
    while ((Index < Names->GetCount()) && Result)
    {
      std::wstring Name = MungeStr(Names->GetString(Index));
      unsigned long Size = Buffer.size();
      unsigned long Type;
      int RegResult;
      do
      {
        RegResult = RegQueryValueEx(Registry->GetCurrentKey(), Name.c_str(), NULL,
          &Type, &Buffer[0], &Size);
        if (RegResult == ERROR_MORE_DATA)
        {
          Buffer.resize(Size);
        }
      } while (RegResult == ERROR_MORE_DATA);

      Result = (RegResult == ERROR_SUCCESS);
      if (Result)
      {
        RegResult = RegSetValueEx(FRegistry->GetCurrentKey(), Name.c_str(), NULL, Type,
          &Buffer[0], Size);
        Result = (RegResult == ERROR_SUCCESS);
      }

      ++Index;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TRegistryStorage::GetSource()
{
  return RootKeyToStr(FRegistry->GetRootKey()) + L"\\" + GetStorage();
}
//---------------------------------------------------------------------------
void TRegistryStorage::SetAccessMode(TStorageAccessMode value)
{
  THierarchicalStorage::SetAccessMode(value);
  if (FRegistry)
  {
    switch (GetAccessMode()) {
      case smRead:
        FRegistry->SetAccess(KEY_READ);
        break;

      case smReadWrite:
      default:
        FRegistry->SetAccess(KEY_READ | KEY_WRITE);
        break;
    }
  }
}
//---------------------------------------------------------------------------
bool TRegistryStorage::OpenSubKey(const std::wstring SubKey, bool CanCreate, bool Path)
{
  // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
  bool Result = CanCreate;
  if (FKeyHistory->GetCount() > 0) FRegistry->CloseKey();
  std::wstring K = ExcludeTrailingBackslash(GetStorage() + GetCurrentSubKey() + MungeSubKey(SubKey, Path));
  Result = FRegistry->OpenKey(K, CanCreate);
  if (Result) Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
  // DEBUG_PRINTF(L"K = %s, Result = %d", K.c_str(), Result);
  return Result;
}
//---------------------------------------------------------------------------
void TRegistryStorage::CloseSubKey()
{
  FRegistry->CloseKey();
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount())
  {
    FRegistry->OpenKey(GetStorage() + GetCurrentSubKey(), true);
  }
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteSubKey(const std::wstring SubKey)
{
  std::wstring K;
  if (FKeyHistory->GetCount() == 0) K = GetStorage() + GetCurrentSubKey();
  K += MungeStr(SubKey);
  return FRegistry->DeleteKey(K);
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetSubKeyNames(TStrings* Strings)
{
  FRegistry->GetKeyNames(Strings);
  for (int Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->GetString(Index) = UnMungeStr(Strings->GetString(Index));
  }
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetValueNames(TStrings* Strings)
{
  FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteValue(const std::wstring Name)
{
  return FRegistry->DeleteValue(Name);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::KeyExists(const std::wstring SubKey)
{
  std::wstring K = MungeStr(SubKey);
  bool Result = FRegistry->KeyExists(K);
  return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::ValueExists(const std::wstring Value)
{
  bool Result = FRegistry->ValueExists(Value);
  return Result;
}
//---------------------------------------------------------------------------
int TRegistryStorage::BinaryDataSize(const std::wstring Name)
{
  int Result = FRegistry->GetDataSize(Name);
  return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::Readbool(const std::wstring Name, bool Default)
{
  READ_REGISTRY(Readbool);
}
//---------------------------------------------------------------------------
TDateTime TRegistryStorage::ReadDateTime(const std::wstring Name, TDateTime Default)
{
  READ_REGISTRY(ReadDateTime);
}
//---------------------------------------------------------------------------
double TRegistryStorage::ReadFloat(const std::wstring Name, double Default)
{
  READ_REGISTRY(ReadFloat);
}
//---------------------------------------------------------------------------
int TRegistryStorage::Readint(const std::wstring Name, int Default)
{
  READ_REGISTRY(Readint);
}
//---------------------------------------------------------------------------
__int64 TRegistryStorage::ReadInt64(const std::wstring Name, __int64 Default)
{
  __int64 Result = Default;
  if (FRegistry->ValueExists(Name))
  {
    try
    {
      FRegistry->ReadBinaryData(Name, &Result, sizeof(Result));
    }
    catch(...)
    {
      FFailed++;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TRegistryStorage::ReadStringRaw(const std::wstring Name, const std::wstring Default)
{
  READ_REGISTRY(ReadString);
}
//---------------------------------------------------------------------------
int TRegistryStorage::ReadBinaryData(const std::wstring Name,
  void * Buffer, int Size)
{
  int Result;
  if (FRegistry->ValueExists(Name))
  {
    try
    {
      Result = FRegistry->ReadBinaryData(Name, Buffer, Size);
    }
    catch(...)
    {
      Result = 0;
      FFailed++;
    }
  }
  else
  {
    Result = 0;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRegistryStorage::Writebool(const std::wstring Name, bool Value)
{
  WRITE_REGISTRY(Writebool);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteDateTime(const std::wstring Name, TDateTime Value)
{
  WRITE_REGISTRY(WriteDateTime);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteFloat(const std::wstring Name, double Value)
{
  WRITE_REGISTRY(WriteFloat);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteStringRaw(const std::wstring Name, const std::wstring Value)
{
  WRITE_REGISTRY(WriteString);
}
//---------------------------------------------------------------------------
void TRegistryStorage::Writeint(const std::wstring Name, int Value)
{
  // DEBUG_PRINTF(L"GetFailed = %d", GetFailed());
  WRITE_REGISTRY(Writeint);
  // DEBUG_PRINTF(L"GetFailed = %d", GetFailed());
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteInt64(const std::wstring Name, __int64 Value)
{
  try
  {
    FRegistry->WriteBinaryData(Name, &Value, sizeof(Value));
  }
  catch(...)
  {
    FFailed++;
  }
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteBinaryData(const std::wstring Name,
  const void * Buffer, int Size)
{
  try
  {
    FRegistry->WriteBinaryData(Name, const_cast<void *>(Buffer), Size);
  }
  catch(...)
  {
    FFailed++;
  }
}
//---------------------------------------------------------------------------
int TRegistryStorage::GetFailed()
{
  int Result = FFailed;
  FFailed = 0;
  return Result;
}
//===========================================================================
TIniFileStorage::TIniFileStorage(const std::wstring AStorage):
  THierarchicalStorage(AStorage)
{
  FIniFile = new TMemIniFile(GetStorage());
  FOriginal = new TStringList();
  Self = this;
  FIniFile->GetStrings(FOriginal);
  ApplyOverrides();
}
//---------------------------------------------------------------------------
TIniFileStorage::~TIniFileStorage()
{
  TStrings * Strings = new TStringList;
  {
    BOOST_SCOPE_EXIT ( (&Self) (&Strings) )
    {
      delete Self->FOriginal;
      delete Strings;
      delete Self->FIniFile;
    } BOOST_SCOPE_EXIT_END
    FIniFile->GetStrings(Strings);
    if (!Strings->Equals(FOriginal))
    {
      int Attr;
      // preserve attributes (especially hidden)
      if (FileExists(GetStorage()))
      {
        Attr = GetFileAttributes(GetStorage().c_str());
      }
      else
      {
        Attr = FILE_ATTRIBUTE_NORMAL;
      }

      HANDLE Handle = CreateFile(GetStorage().c_str(), GENERIC_READ | GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, Attr, 0);

      if (Handle == INVALID_HANDLE_VALUE)
      {
        // "access denied" errors upon implicit saves are ignored
        if (GetExplicit() || (GetLastError() != ERROR_ACCESS_DENIED))
        {
          try
          {
            RaiseLastOSError();
          }
          catch (const std::exception & E)
          {
            throw ExtException(&E, FMTLOAD(CREATE_FILE_ERROR, GetStorage().c_str()));
          }
        }
      }
      else
      {
        TStream * Stream = new THandleStream(Handle);
        {
          BOOST_SCOPE_EXIT ( (&Handle) (&Stream) )
          {
            ::CloseHandle(Handle);
            delete Stream;
          } BOOST_SCOPE_EXIT_END
          Strings->SaveToStream(Stream);
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
std::wstring TIniFileStorage::GetSource()
{
  return GetStorage();
}
//---------------------------------------------------------------------------
std::wstring TIniFileStorage::GetCurrentSection()
{
  return ExcludeTrailingBackslash(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool TIniFileStorage::OpenSubKey(const std::wstring SubKey, bool CanCreate, bool Path)
{
  // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
  bool Result = CanCreate;

  if (!Result)
  {
    TStringList * Sections = new TStringList();
    {
      BOOST_SCOPE_EXIT ( (&Sections) )
      {
        delete Sections;
      } BOOST_SCOPE_EXIT_END
      Sections->SetSorted(true);
      FIniFile->ReadSections(Sections);
      std::wstring NewKey = ExcludeTrailingBackslash(GetCurrentSubKey() + MungeSubKey(SubKey, Path));
      int Index = -1;
      if (Sections->GetCount())
      {
        Result = Sections->Find(NewKey, Index);
        if (!Result && Index < Sections->GetCount() &&
            Sections->GetString(Index).substr(0, NewKey.size()+1) == NewKey + L"\\")
        {
          Result = true;
        }
      }
    }
  }

  if (Result)
  {
    Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TIniFileStorage::DeleteSubKey(const std::wstring SubKey)
{
  bool Result;
  try
  {
    FIniFile->EraseSection(GetCurrentSubKey() + MungeStr(SubKey));
    Result = true;
  }
  catch (...)
  {
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TIniFileStorage::GetSubKeyNames(TStrings* Strings)
{
  TStrings * Sections = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&Sections) )
    {
      delete Sections;
    } BOOST_SCOPE_EXIT_END
    Strings->Clear();
    FIniFile->ReadSections(Sections);
    for (int i = 0; i < Sections->GetCount(); i++)
    {
      std::wstring Section = Sections->GetString(i);
      if (AnsiCompareText(GetCurrentSubKey(),
          Section.substr(0, GetCurrentSubKey().size())) == 0)
      {
        std::wstring SubSection = Section.substr(GetCurrentSubKey().size() + 1,
          Section.size() - GetCurrentSubKey().size());
        int P = SubSection.find_first_of(L"\\");
        if (P)
        {
          SubSection.resize(P - 1);
        }
        if (Strings->IndexOf(SubSection.c_str()) < 0)
        {
          Strings->Add(UnMungeStr(SubSection));
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TIniFileStorage::GetValueNames(TStrings* Strings)
{
  FIniFile->ReadSection(GetCurrentSection(), Strings);
  for (int Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->GetString(Index) = UnMungeIniName(Strings->GetString(Index));
  }
}
//---------------------------------------------------------------------------
bool TIniFileStorage::KeyExists(const std::wstring SubKey)
{
  return FIniFile->SectionExists(GetCurrentSubKey() + MungeStr(SubKey));
}
//---------------------------------------------------------------------------
bool TIniFileStorage::ValueExists(const std::wstring Value)
{
  return FIniFile->ValueExists(GetCurrentSection(), MungeIniName(Value));
}
//---------------------------------------------------------------------------
bool TIniFileStorage::DeleteValue(const std::wstring Name)
{
  FIniFile->DeleteKey(GetCurrentSection(), MungeIniName(Name));
  return true;
}
//---------------------------------------------------------------------------
int TIniFileStorage::BinaryDataSize(const std::wstring Name)
{
  return ReadStringRaw(Name, L"").size() / 2;
}
//---------------------------------------------------------------------------
void TIniFileStorage::ApplyOverrides()
{
  std::wstring OverridesKey = IncludeTrailingBackslash(L"Override");

  TStrings * Sections = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&Sections) )
    {
      delete Sections;
    } BOOST_SCOPE_EXIT_END
    Sections->Clear();
    FIniFile->ReadSections(Sections);
    for (int i = 0; i < Sections->GetCount(); i++)
    {
      std::wstring Section = Sections->GetString(i);

      if (AnsiSameText(OverridesKey,
            Section.substr(0, OverridesKey.size())))
      {
        std::wstring SubKey = Section.substr(OverridesKey.size() + 1,
          Section.size() - OverridesKey.size());

        // this all uses raw names (munged)
        TStrings * Names = new TStringList;
        {
            BOOST_SCOPE_EXIT ( (&Names) )
            {
              delete Names;
            } BOOST_SCOPE_EXIT_END
          FIniFile->ReadSection(Section, Names);

          for (int ii = 0; ii < Names->GetCount(); ii++)
          {
            std::wstring Name = Names->GetString(ii);
            std::wstring Value = FIniFile->ReadString(Section, Name, L"");
            FIniFile->WriteString(SubKey, Name, Value);
          }
        }

        FIniFile->EraseSection(Section);
      }
    }
  }
}
//---------------------------------------------------------------------------
bool TIniFileStorage::Readbool(const std::wstring Name, bool Default)
{
  return FIniFile->Readbool(GetCurrentSection(), MungeIniName(Name), Default);
}
//---------------------------------------------------------------------------
int TIniFileStorage::Readint(const std::wstring Name, int Default)
{
  int Result = FIniFile->Readint(GetCurrentSection(), MungeIniName(Name), Default);
  return Result;
}
//---------------------------------------------------------------------------
__int64 TIniFileStorage::ReadInt64(const std::wstring Name, __int64 Default)
{
  __int64 Result = Default;
  std::wstring Str;
  Str = ReadStringRaw(Name, L"");
  if (!Str.empty())
  {
    Result = StrToInt64Def(Str, Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime TIniFileStorage::ReadDateTime(const std::wstring Name, TDateTime Default)
{
  TDateTime Result;
  std::wstring Value = FIniFile->ReadString(GetCurrentSection(), MungeIniName(Name), L"");
  if (Value.empty())
  {
    Result = Default;
  }
  else
  {
    try
    {
      std::wstring Raw = HexToStr(Value);
      if (Raw.size() == sizeof(Result))
      {
        memcpy(&Result, Raw.c_str(), sizeof(Result));
      }
      else
      {
        Result = StrToDateTime(Value);
      }
    }
    catch(...)
    {
      Result = Default;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
double TIniFileStorage::ReadFloat(const std::wstring Name, double Default)
{
  double Result;
  std::wstring Value = FIniFile->ReadString(GetCurrentSection(), MungeIniName(Name), L"");
  if (Value.empty())
  {
    Result = Default;
  }
  else
  {
    try
    {
      std::wstring Raw = HexToStr(Value);
      if (Raw.size() == sizeof(Result))
      {
        memcpy(&Result, Raw.c_str(), sizeof(Result));
      }
      else
      {
        Result = StrToFloat(Value);
      }
    }
    catch(...)
    {
      Result = Default;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
std::wstring TIniFileStorage::ReadStringRaw(const std::wstring Name, std::wstring Default)
{
  std::wstring Result = FIniFile->ReadString(GetCurrentSection(), MungeIniName(Name), Default);
  return Result;
}
//---------------------------------------------------------------------------
int TIniFileStorage::ReadBinaryData(const std::wstring Name,
  void * Buffer, int Size)
{
  std::wstring Value = HexToStr(ReadStringRaw(Name, L""));
  int Len = Value.size();
  if (Size > Len)
  {
    Size = Len;
  }
  assert(Buffer);
  memcpy(Buffer, Value.c_str(), Size);
  return Size;
}
//---------------------------------------------------------------------------
void TIniFileStorage::Writebool(const std::wstring Name, bool Value)
{
  FIniFile->Writebool(GetCurrentSection(), MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TIniFileStorage::Writeint(const std::wstring Name, int Value)
{
  FIniFile->Writeint(GetCurrentSection(), MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteInt64(const std::wstring Name, __int64 Value)
{
  WriteStringRaw(Name, IntToStr(Value));
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteDateTime(const std::wstring Name, TDateTime Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteFloat(const std::wstring Name, double Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteStringRaw(const std::wstring Name, const std::wstring Value)
{
  FIniFile->WriteString(GetCurrentSection(), MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteBinaryData(const std::wstring Name,
  const void * Buffer, int Size)
{
  WriteStringRaw(Name, StrToHex(std::wstring(static_cast<const wchar_t*>(Buffer), Size)));
}
