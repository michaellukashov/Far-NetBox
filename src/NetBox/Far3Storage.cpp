#include "stdafx.h"

#include <vector>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "Far3Storage.h"
#include "TextsCore.h"

//---------------------------------------------------------------------------
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch(...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch(...) { FFailed++; }

//---------------------------------------------------------------------------
TFar3Storage::TFar3Storage(const std::wstring &AStorage,
  const GUID &guid, FARAPISETTINGSCONTROL SettingsControl) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FPluginSettings(guid, SettingsControl)
{
  // DEBUG_PRINTF(L"AStorage = %s", AStorage.c_str());
  Init();
};
//---------------------------------------------------------------------------
void TFar3Storage::Init()
{
  FRoot = 0;
  FFailed = 0;
  // FRegistry = new TRegistry;
  // FRegistry->SetAccess(KEY_READ);
}
//---------------------------------------------------------------------------
TFar3Storage::~TFar3Storage()
{
  // delete FRegistry;
};
//---------------------------------------------------------------------------
bool TFar3Storage::Copy(TFar3Storage * Storage)
{
  bool Result = true;
  /*
  TRegistry * Registry = Storage->FRegistry;
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
      std::wstring Name = PuttyMungeStr(Names->GetString(Index));
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
  */
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TFar3Storage::GetSource()
{
  return /* RootKeyToStr(/* FRegistry->GetRootKey()) + L"\\" + */ GetStorage();
}
//---------------------------------------------------------------------------
void TFar3Storage::SetAccessMode(TStorageAccessMode value)
{
  THierarchicalStorage::SetAccessMode(value);
}
//---------------------------------------------------------------------------
bool TFar3Storage::OpenSubKey(const std::wstring &SubKey, bool CanCreate, bool Path)
{
  DEBUG_PRINTF(L"SubKey = %s, CanCreate = %d, Path = %d", SubKey.c_str(), CanCreate, Path);
  // DEBUG_PRINTF(L"MungeSubKey(SubKey, Path) = %s", MungeSubKey(SubKey, Path).c_str());
  // if (FKeyHistory->GetCount() > 0) FRegistry->CloseKey();
  // std::wstring K = ExcludeTrailingBackslash(GetFullCurrentSubKey() + MungeSubKey(SubKey, Path));
  // bool Result = FRegistry->OpenKey(K, CanCreate);
  // std::wstring LastElem = SubKey.substr(::LastDelimiter(ELEM, L".>") + 1, ELEM.size() - ::LastDelimiter(ELEM, L".>"))
  // std::wstring LastElem = SubKey;
  int OldRoot = FRoot;
  if (Path)
  {
    std::wstring subKey = SubKey;
    assert(subKey.empty() || (subKey[subKey.size() - 1] != '\\'));
    bool Result = true;
    while (!subKey.empty())
    {
      Result &= OpenSubKey(CutToChar(subKey, L'\\', false), CanCreate, false);
      // DEBUG_PRINTF(L"SubKey = %s, Result = %d", SubKey.c_str(), Result);
    }
    return Result;
  }
  else
  {
      if (CanCreate)
      {
        FRoot = FPluginSettings.CreateSubKey(FRoot, SubKey.c_str());
      }
      else
      {
        FRoot = FPluginSettings.OpenSubKey(FRoot, SubKey.c_str());
      }
  }
  DEBUG_PRINTF(L"FRoot = %d", FRoot);
  bool Result = FRoot != 0;
  if (Result)
  {
    Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
    if (Result)
    {
        FSubKeyIds.push_back(OldRoot);
    }
    // FSubKey = SubKey;
  }
  DEBUG_PRINTF(L"end, Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void TFar3Storage::CloseSubKey()
{
  DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  // FRegistry->CloseKey();
  DEBUG_PRINTF(L"GetFullCurrentSubKey1 = %s", GetFullCurrentSubKey().c_str());
  assert(FKeyHistory->GetCount() == FSubKeyIds.size());
  THierarchicalStorage::CloseSubKey();
  DEBUG_PRINTF(L"GetFullCurrentSubKey2 = %s", GetFullCurrentSubKey().c_str());
  if (FKeyHistory->GetCount() && FSubKeyIds.size())
  {
    // FRegistry->OpenKey(GetFullCurrentSubKey(), true);
    // FRoot = FPluginSettings.OpenSubKey(FRoot, GetFullCurrentSubKey().c_str());
    FRoot = FSubKeyIds.back();
    FSubKeyIds.pop_back();
  }
  else
  {
    FRoot = 0;
  }
  DEBUG_PRINTF(L"end, FRoot = %d", FRoot);
}
//---------------------------------------------------------------------------
bool TFar3Storage::DeleteSubKey(const std::wstring &SubKey)
{
  std::wstring K;
  if (FKeyHistory->GetCount() == 0)
  {
    K = GetFullCurrentSubKey();
    FRoot = FPluginSettings.OpenSubKey(FRoot, K.c_str());
  }
  // K += PuttyMungeStr(SubKey);
  // return FRegistry->DeleteKey(K);
  return FPluginSettings.DeleteValue(FRoot, SubKey.c_str());
}
//---------------------------------------------------------------------------
void TFar3Storage::GetSubKeyNames(TStrings* Strings)
{
  DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  /*
  FRegistry->GetKeyNames(Strings);
  for (size_t Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->PutString(Index, PuttyUnMungeStr(Strings->GetString(Index)));
  }
  */
  FarSettingsEnum settings = {0};
  settings.Root = FRoot;
  if (FPluginSettings.GetSubKeyNames(settings))
  {
      for (size_t Index = 0; Index < settings.Count; Index++)
      {
        Strings->PutString(Index, PuttyUnMungeStr(settings.Items[Index].Name));
        DEBUG_PRINTF(L"settings.Items[%d].Type = %d", Index, settings.Items[Index].Type);
      }
  }
  DEBUG_PRINTF(L"end, FRoot = %d", FRoot);
}
//---------------------------------------------------------------------------
void TFar3Storage::GetValueNames(TStrings* Strings)
{
  DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  // TODO: use SettingsControl
  // FRegistry->GetValueNames(Strings);
  DEBUG_PRINTF(L"end, FRoot = %d", FRoot);
}
//---------------------------------------------------------------------------
bool TFar3Storage::DeleteValue(const std::wstring &Name)
{
  // return FRegistry->DeleteValue(Name);
  return FPluginSettings.DeleteValue(FRoot, Name.c_str());
}
//---------------------------------------------------------------------------
bool TFar3Storage::KeyExists(const std::wstring &SubKey)
{
  DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  // TODO: use SettingsControl
  std::wstring K = PuttyMungeStr(SubKey);
  // bool Result = FRegistry->KeyExists(K);
  bool Result = true; // FPluginSettings.KeyExists(K);
  DEBUG_PRINTF(L"end, FRoot = %d, K = %s, Result = %d", FRoot, K.c_str(), Result);
  return Result;
}
//---------------------------------------------------------------------------
bool TFar3Storage::ValueExists(const std::wstring &Value)
{
  DEBUG_PRINTF(L"begin, FRoot = %d, Value = %s", FRoot, Value.c_str());
  // TODO: use SettingsControl
  // bool Result = FRegistry->ValueExists(Value);
  bool Result = false;
  return Result;
}
//---------------------------------------------------------------------------
int TFar3Storage::BinaryDataSize(const std::wstring &Name)
{
  // TODO: use SettingsControl
  // int Result = FRegistry->GetDataSize(Name);
  int Result = 0;
  return Result;
}
//---------------------------------------------------------------------------
bool TFar3Storage::Readbool(const std::wstring &Name, bool Default)
{
  // READ_REGISTRY(Readbool);
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
TDateTime TFar3Storage::ReadDateTime(const std::wstring &Name, TDateTime Default)
{
  // READ_REGISTRY(ReadDateTime);
  TDateTime Result;
  double val = 0.0;
  void *value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (FPluginSettings.Get(FRoot, Name.c_str(), value, sz) == sz)
  {
    Result = val;
  }
  return Result;
}
//---------------------------------------------------------------------------
double TFar3Storage::ReadFloat(const std::wstring &Name, double Default)
{
  // READ_REGISTRY(ReadFloat);
  double Result = 0.0;
  double val = 0.0;
  void *value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (FPluginSettings.Get(FRoot, Name.c_str(), value, sz) == sz)
  {
    Result = val;
  }
  return Result;
}
//---------------------------------------------------------------------------
int TFar3Storage::Readint(const std::wstring &Name, int Default)
{
  // READ_REGISTRY(Readint);
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
__int64 TFar3Storage::ReadInt64(const std::wstring &Name, __int64 Default)
{
  /*
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
  */
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
std::wstring TFar3Storage::ReadStringRaw(const std::wstring &Name, const std::wstring &Default)
{
  // READ_REGISTRY(ReadString);
  std::wstring Result = FPluginSettings.Get(FRoot, Name.c_str(), Default.c_str());
  return Result;
}
//---------------------------------------------------------------------------
int TFar3Storage::ReadBinaryData(const std::wstring &Name,
  void * Buffer, int Size)
{
  /*
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
  */
  return FPluginSettings.Get(FRoot, Name.c_str(), Buffer, Size);
}
//---------------------------------------------------------------------------
void TFar3Storage::Writebool(const std::wstring &Name, bool Value)
{
  DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  // WRITE_REGISTRY(Writebool);
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteDateTime(const std::wstring &Name, TDateTime Value)
{
  // WRITE_REGISTRY(WriteDateTime);
  double val = Value.operator double();
  void *value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (!FPluginSettings.Set(FRoot, Name.c_str(), value, sz))
  {
    // TODO: report error
  }
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteFloat(const std::wstring &Name, double Value)
{
  // WRITE_REGISTRY(WriteFloat);
  double val = Value.operator double();
  void *value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (!FPluginSettings.Set(FRoot, Name.c_str(), value, sz))
  {
    // TODO: report error
  }
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteStringRaw(const std::wstring &Name, const std::wstring &Value)
{
  // WRITE_REGISTRY(WriteString);
  FPluginSettings.Set(FRoot, Name.c_str(), Value.c_str());
}
//---------------------------------------------------------------------------
void TFar3Storage::Writeint(const std::wstring &Name, int Value)
{
  DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  // DEBUG_PRINTF(L"GetFailed = %d", GetFailed());
  // WRITE_REGISTRY(Writeint);
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
  // DEBUG_PRINTF(L"GetFailed = %d", GetFailed());
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteInt64(const std::wstring &Name, __int64 Value)
{
  DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  /*
  try
  {
    FRegistry->WriteBinaryData(Name, &Value, sizeof(Value));
  }
  catch(...)
  {
    FFailed++;
  }
  */
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteBinaryData(const std::wstring &Name,
  const void * Buffer, int Size)
{
  /*
  try
  {
    FRegistry->WriteBinaryData(Name, const_cast<void *>(Buffer), Size);
  }
  catch(...)
  {
    FFailed++;
  }
  */
  FPluginSettings.Set(FRoot, Name.c_str(), Buffer, Size);
}
//---------------------------------------------------------------------------
int TFar3Storage::GetFailed()
{
  int Result = FFailed;
  FFailed = 0;
  return Result;
}
