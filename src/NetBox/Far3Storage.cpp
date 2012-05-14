#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "Far3Storage.h"
#include "TextsCore.h"

//---------------------------------------------------------------------------
TFar3Storage::TFar3Storage(const UnicodeString AStorage,
                           const GUID & guid, FARAPISETTINGSCONTROL SettingsControl) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FPluginSettings(guid, SettingsControl)
{
  // DEBUG_PRINTF(L"AStorage = %s", AStorage.c_str());
  Init();
};
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::Init()
{
  FRoot = 0;
}
//---------------------------------------------------------------------------
TFar3Storage::~TFar3Storage()
{
};
//---------------------------------------------------------------------------
bool __fastcall TFar3Storage::Copy(TFar3Storage * Storage)
{
  Error(SNotImplemented, 3014);
  bool Result = true;
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFar3Storage::GetSource()
{
  return GetStorage();
}
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::SetAccessMode(TStorageAccessMode value)
{
  THierarchicalStorage::SetAccessMode(value);
}
//---------------------------------------------------------------------------
int __fastcall TFar3Storage::OpenSubKeyInternal(int Root, const UnicodeString SubKey, bool CanCreate)
{
  int root = 0;
  UnicodeString UnmungedSubKey = PuttyUnMungeStr(SubKey);
  if (CanCreate)
  {
    // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
    root = FPluginSettings.CreateSubKey(Root, UnmungedSubKey.c_str());
  }
  else
  {
    root = FPluginSettings.OpenSubKey(Root, UnmungedSubKey.c_str());
  }
  return root;
}
//---------------------------------------------------------------------------
bool __fastcall TFar3Storage::DoOpenSubKey(const UnicodeString MungedSubKey, bool CanCreate)
{
  // DEBUG_PRINTF(L"SubKey = %s, CanCreate = %d, Path = %d", SubKey.c_str(), CanCreate, Path);
  int OldRoot = FRoot;
  int root = FRoot;
  bool Result = false;
  {
    UnicodeString subKey = MungedSubKey;
    assert(subKey.IsEmpty() || (subKey[subKey.size()] != '\\'));
    bool Result = true;
    // CutToChar(subKey, L'\\', false);
    while (!subKey.IsEmpty())
    {
      root = OpenSubKeyInternal(root, CutToChar(subKey, L'\\', false), CanCreate);
      Result &= root != 0;
      // DEBUG_PRINTF(L"SubKey = %s, Result = %d", SubKey.c_str(), Result);
    }
    if (Result)
    {
      FSubKeyIds.push_back(OldRoot);
      FRoot = root;
    }
    return Result;
  }
  // DEBUG_PRINTF(L"end, Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::CloseSubKey()
{
  // DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  THierarchicalStorage::CloseSubKey();
  // assert(FKeyHistory->GetCount() == FSubKeyIds.size() - 1);
  if (FKeyHistory->GetCount() && FSubKeyIds.size())
  {
    FRoot = FSubKeyIds.back();
    FSubKeyIds.pop_back();
  }
  else
  {
    FRoot = 0;
  }
  // DEBUG_PRINTF(L"end, FRoot = %d", FRoot);
}
//---------------------------------------------------------------------------
bool __fastcall TFar3Storage::DeleteSubKey(const UnicodeString SubKey)
{
  UnicodeString K;
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
void __fastcall TFar3Storage::GetSubKeyNames(TStrings * Strings)
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
void __fastcall TFar3Storage::GetValueNames(TStrings * Strings)
{
  DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  Strings->Clear();
  FarSettingsEnum settings = {0};
  settings.Root = FRoot;
  if (FPluginSettings.GetSubKeyNames(settings))
  {
    for (size_t I = 0; I < settings.Count; I++)
    {
      const struct FarSettingsName * Item = &settings.Items[I];
      Strings->Add(Item->Name);
    }
  }
  DEBUG_PRINTF(L"end, Strings->GetCount() = %d", Strings->GetCount());
}
//---------------------------------------------------------------------------
bool __fastcall TFar3Storage::DeleteValue(const UnicodeString Name)
{
  return FPluginSettings.DeleteValue(FRoot, Name.c_str());
}
//---------------------------------------------------------------------------
bool __fastcall TFar3Storage::DoKeyExists(const UnicodeString SubKey, bool ForceAnsi)
{
  Error(SNotImplemented, 3011);
  // DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  UnicodeString K = PuttyMungeStr(SubKey);
  bool Result = true; // FPluginSettings.KeyExists(K);
  // DEBUG_PRINTF(L"end, FRoot = %d, K = %s, Result = %d", FRoot, K.c_str(), Result);
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TFar3Storage::ValueExists(const UnicodeString Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Value = %s", FRoot, Value.c_str());
  bool Result = FPluginSettings.ValueExists(FRoot, Value.c_str());
  return Result;
}
//---------------------------------------------------------------------------
size_t __fastcall TFar3Storage::BinaryDataSize(const UnicodeString Name)
{
  int Result = FPluginSettings.BinaryDataSize(FRoot, Name.c_str());
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TFar3Storage::ReadBool(const UnicodeString Name, bool Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
TDateTime __fastcall TFar3Storage::ReadDateTime(const UnicodeString Name, TDateTime Default)
{
  TDateTime Result;
  double val = 0.0;
  void * value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (FPluginSettings.Get(FRoot, Name.c_str(), value, sz) == sz)
  {
    Result = val;
  }
  return Result;
}
//---------------------------------------------------------------------------
double __fastcall TFar3Storage::ReadFloat(const UnicodeString Name, double Default)
{
  double Result = 0.0;
  double val = 0.0;
  void * value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (FPluginSettings.Get(FRoot, Name.c_str(), value, sz) == sz)
  {
    Result = val;
  }
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TFar3Storage::ReadInteger(const UnicodeString Name, int Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
__int64 __fastcall TFar3Storage::ReadInt64(const UnicodeString Name, __int64 Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFar3Storage::ReadStringRaw(const UnicodeString Name, const UnicodeString Default)
{
  UnicodeString Result = FPluginSettings.Get(FRoot, Name.c_str(), Default.c_str());
  return Result;
}
//---------------------------------------------------------------------------
size_t __fastcall TFar3Storage::ReadBinaryData(const UnicodeString Name,
    void * Buffer, size_t Size)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Buffer, Size);
}
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::WriteBool(const UnicodeString Name, bool Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::WriteDateTime(const UnicodeString Name, TDateTime Value)
{
  double val = Value.operator double();
  void * value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (!FPluginSettings.Set(FRoot, Name.c_str(), value, sz))
  {
    // TODO: report error
  }
}
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::WriteFloat(const UnicodeString Name, double Value)
{
  double val = Value.operator double();
  void * value = reinterpret_cast<void *>(&val);
  size_t sz = sizeof(val);
  if (!FPluginSettings.Set(FRoot, Name.c_str(), value, sz))
  {
    // TODO: report error
  }
}
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::WriteStringRaw(const UnicodeString Name, const UnicodeString Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value.c_str());
}
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::WriteInteger(const UnicodeString Name, int Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::WriteInt64(const UnicodeString Name, __int64 Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void __fastcall TFar3Storage::WriteBinaryData(const UnicodeString Name,
    const void * Buffer, int Size)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Buffer, Size);
}
//---------------------------------------------------------------------------
