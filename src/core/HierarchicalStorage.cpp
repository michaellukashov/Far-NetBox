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
  Result2.resize(Str.size() * sizeof(wchar_t) * 3 + 1);
  putty_mungestr(::W2MB(Str.c_str()).c_str(), const_cast<char *>(Result2.c_str()));
  std::wstring Result = ::MB2W(Result2.c_str());
  PackStr(Result);
  // DEBUG_PRINTF(L"Str = %s, Result = %s", Str.c_str(), Result.c_str());
  return Result;
}
//---------------------------------------------------------------------------
std::wstring UnMungeStr(const std::wstring Str)
{
  std::string Result2;
  Result2.resize(Str.size() * sizeof(wchar_t) * 3 + 1);
  putty_unmungestr(const_cast<char *>(::W2MB(Str.c_str()).c_str()), const_cast<char *>(Result2.c_str()), Result2.size());
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
std::wstring PuttyUnMungeStr(const std::wstring Str)
{
  return UnMungeStr(Str);
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
  size_t P = Str.find(L"%3D");
  // make this fast for now
  if (P != std::wstring::npos)
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
std::wstring THierarchicalStorage::MungeSubKey(const std::wstring Key, bool Path)
{
  std::wstring Result;
  // DEBUG_PRINTF(L"Key = %s, Path = %d", Key.c_str(), Path);
  std::wstring key = Key;
  if (Path)
  {
    assert(key.empty() || (key[key.size() - 1] != '\\'));
    while (!key.empty())
    {
      if (!Result.empty())
      {
        Result += '\\';
      }
      Result += MungeStr(CutToChar(key, L'\\', false));
      // DEBUG_PRINTF(L"key = %s, Result = %s", key.c_str(), Result.c_str());
    }
  }
  else
  {
    Result = MungeStr(key);
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
    for (size_t Index = 0; Index < SubKeys->GetCount(); Index++)
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
    for (size_t Index = 0; Index < Names->GetCount(); Index++)
    {
      if (MaintainKeys)
      {
        Strings->Add(FORMAT(L"%s=%s", Names->GetString(Index).c_str(),
          ReadString(Names->GetString(Index), L"").c_str()));
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
    for (size_t Index = 0; Index < Names->GetCount(); Index++)
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
    for (size_t Index = 0; Index < Strings->GetCount(); Index++)
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
  std::wstring Value(Size, 0);
  ReadBinaryData(Name, const_cast<wchar_t *>(Value.c_str()), Size);
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
std::wstring THierarchicalStorage::IncludeTrailingBackslash(const std::wstring S)
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
std::wstring THierarchicalStorage::ExcludeTrailingBackslash(const std::wstring S)
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
TRegistryStorage::TRegistryStorage(const std::wstring AStorage) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(NULL)
{
  Init();
};
//---------------------------------------------------------------------------
TRegistryStorage::TRegistryStorage(const std::wstring AStorage, HKEY ARootKey) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(NULL)
{
  // DEBUG_PRINTF(L"AStorage = %s", AStorage.c_str());
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
    size_t Index = 0;
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
  // DEBUG_PRINTF(L"Storage = %s, CurrentSubKey = %s, SubKey = %s", GetStorage().c_str(), GetCurrentSubKey().c_str(), SubKey.c_str());
  if (FKeyHistory->GetCount() > 0) FRegistry->CloseKey();
  std::wstring K = ExcludeTrailingBackslash(GetStorage() + GetCurrentSubKey() + MungeSubKey(SubKey, Path));
  bool Result = FRegistry->OpenKey(K, CanCreate);
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
  for (size_t Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->PutString(Index, UnMungeStr(Strings->GetString(Index)));
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
  // DEBUG_PRINTF(L"Name = %s, Value = %d", Name.c_str(), Value);
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
