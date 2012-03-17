#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "Far3Storage.h"
#include "TextsCore.h"

//---------------------------------------------------------------------------
TFar3Storage::TFar3Storage(const std::wstring AStorage,
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
}
//---------------------------------------------------------------------------
TFar3Storage::~TFar3Storage()
{
};
//---------------------------------------------------------------------------
bool TFar3Storage::Copy(TFar3Storage * Storage)
{
  nb::Error(SNotImplemented, 3014);
  bool Result = true;
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TFar3Storage::GetSource()
{
  return GetStorage();
}
//---------------------------------------------------------------------------
void TFar3Storage::SetAccessMode(TStorageAccessMode value)
{
  THierarchicalStorage::SetAccessMode(value);
}
//---------------------------------------------------------------------------
int TFar3Storage::OpenSubKeyInternal(int Root, const std::wstring SubKey, bool CanCreate, bool Path)
{
    int root = 0;
    if (CanCreate)
    {
      // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
      root = FPluginSettings.CreateSubKey(Root, SubKey.c_str());
    }
    else
    {
      root = FPluginSettings.OpenSubKey(Root, SubKey.c_str());
    }
    return root;
}
//---------------------------------------------------------------------------
bool TFar3Storage::OpenSubKey(const std::wstring SubKey, bool CanCreate, bool Path)
{
  // DEBUG_PRINTF(L"SubKey = %s, CanCreate = %d, Path = %d", SubKey.c_str(), CanCreate, Path);
  int OldRoot = FRoot;
  int root = FRoot;
  bool Result = false;
  {
    std::wstring subKey = SubKey;
    assert(subKey.empty() || (subKey[subKey.size() - 1] != '\\'));
    bool Result = true;
    // CutToChar(subKey, L'\\', false);
    while (!subKey.empty())
    {
      root = OpenSubKeyInternal(root, CutToChar(subKey, L'\\', false), CanCreate, Path);
      Result &= root != 0;
      // DEBUG_PRINTF(L"SubKey = %s, Result = %d", SubKey.c_str(), Result);
    }
    if (Result)
    {
      Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
      if (Result)
      {
          FSubKeyIds.push_back(OldRoot);
          FRoot = root;
      }
    }
    return Result;
  }
  // DEBUG_PRINTF(L"end, Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void TFar3Storage::CloseSubKey()
{
  // DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  assert(FKeyHistory->GetCount() == FSubKeyIds.size());
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount() && FSubKeyIds.size())
  {
    FRoot = FSubKeyIds.back();
    FSubKeyIds.pop_back();
    // OpenSubKey(GetCurrentSubKey(), true);
    // DEBUG_PRINTF(L"GetCurrentSubKey = %s", GetCurrentSubKey().c_str());
  }
  else
  {
    FRoot = 0;
  }
  // DEBUG_PRINTF(L"end, FRoot = %d", FRoot);
}
//---------------------------------------------------------------------------
bool TFar3Storage::DeleteSubKey(const std::wstring SubKey)
{
  std::wstring K;
  if (FKeyHistory->GetCount() == 0)
  {
    K = GetFullCurrentSubKey();
    FRoot = FPluginSettings.OpenSubKey(FRoot, K.c_str());
  }
  int root = FPluginSettings.OpenSubKey(FRoot, SubKey.c_str());
  if (root != 0)
  {
    return FPluginSettings.DeleteSubKey(root);
  }
  return false;
}
//---------------------------------------------------------------------------
void TFar3Storage::GetSubKeyNames(nb::TStrings *Strings)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  FarSettingsEnum settings = {0};
  settings.Root = FRoot;
  if (FPluginSettings.GetSubKeyNames(settings))
  {
      for (size_t Index = 0; Index < settings.Count; Index++)
      {
        // DEBUG_PRINTF(L"settings.Items[%d].Type = %d", Index, settings.Items[Index].Type);
        if (settings.Items[Index].Type == FST_SUBKEY)
        {
            Strings->PutString(Index, PuttyUnMungeStr(settings.Items[Index].Name));
        }
      }
  }
  // DEBUG_PRINTF(L"end, FRoot = %d", FRoot);
}
//---------------------------------------------------------------------------
void TFar3Storage::GetValueNames(nb::TStrings * Strings)
{
    DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
    Strings->Clear();
    FarSettingsEnum settings = {0};
    settings.Root = FRoot;
    if (FPluginSettings.GetSubKeyNames(settings))
    {
        for (size_t I = 0; I < settings.Count; I++)
        {
            const struct FarSettingsName *Item = &settings.Items[I];
            Strings->Add(Item->Name);
        }
    }
    DEBUG_PRINTF(L"end, Strings->GetCount() = %d", Strings->GetCount());
}
//---------------------------------------------------------------------------
bool TFar3Storage::DeleteValue(const std::wstring Name)
{
  return FPluginSettings.DeleteValue(FRoot, Name.c_str());
}
//---------------------------------------------------------------------------
bool TFar3Storage::KeyExists(const std::wstring SubKey)
{
  nb::Error(SNotImplemented, 3011);
  // DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  std::wstring K = PuttyMungeStr(SubKey);
  bool Result = true; // FPluginSettings.KeyExists(K);
  // DEBUG_PRINTF(L"end, FRoot = %d, K = %s, Result = %d", FRoot, K.c_str(), Result);
  return Result;
}
//---------------------------------------------------------------------------
bool TFar3Storage::ValueExists(const std::wstring Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Value = %s", FRoot, Value.c_str());
  bool Result = FPluginSettings.ValueExists(FRoot, Value.c_str());
  return Result;
}
//---------------------------------------------------------------------------
int TFar3Storage::BinaryDataSize(const std::wstring Name)
{
  int Result = FPluginSettings.BinaryDataSize(FRoot, Name.c_str());
  return Result;
}
//---------------------------------------------------------------------------
bool TFar3Storage::Readbool(const std::wstring Name, bool Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
nb::TDateTime TFar3Storage::ReadDateTime(const std::wstring Name, nb::TDateTime Default)
{
  nb::TDateTime Result;
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
double TFar3Storage::ReadFloat(const std::wstring Name, double Default)
{
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
int TFar3Storage::Readint(const std::wstring Name, int Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
__int64 TFar3Storage::ReadInt64(const std::wstring Name, __int64 Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
std::wstring TFar3Storage::ReadStringRaw(const std::wstring Name, const std::wstring Default)
{
  std::wstring Result = FPluginSettings.Get(FRoot, Name.c_str(), Default.c_str());
  return Result;
}
//---------------------------------------------------------------------------
size_t TFar3Storage::ReadBinaryData(const std::wstring Name,
  void * Buffer, size_t Size)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Buffer, Size);
}
//---------------------------------------------------------------------------
void TFar3Storage::Writebool(const std::wstring Name, bool Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteDateTime(const std::wstring Name, nb::TDateTime Value)
{
  double val = Value.operator double();
  void *value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (!FPluginSettings.Set(FRoot, Name.c_str(), value, sz))
  {
    // TODO: report error
  }
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteFloat(const std::wstring Name, double Value)
{
  double val = Value.operator double();
  void *value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (!FPluginSettings.Set(FRoot, Name.c_str(), value, sz))
  {
    // TODO: report error
  }
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteStringRaw(const std::wstring Name, const std::wstring Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value.c_str());
}
//---------------------------------------------------------------------------
void TFar3Storage::Writeint(const std::wstring Name, int Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteInt64(const std::wstring Name, __int64 Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteBinaryData(const std::wstring Name,
  const void * Buffer, size_t Size)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Buffer, Size);
}
//---------------------------------------------------------------------------
int TFar3Storage::GetFailed()
{
  int Result = FFailed;
  FFailed = 0;
  return Result;
}
