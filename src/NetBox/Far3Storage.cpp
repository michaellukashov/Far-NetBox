#include "stdafx.h"

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "Far3Storage.h"
#include "TextsCore.h"

//---------------------------------------------------------------------------
TFar3Storage::TFar3Storage(const UnicodeString & AStorage,
  const GUID & guid, FARAPISETTINGSCONTROL SettingsControl) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FPluginSettings(guid, SettingsControl)
{
  // DEBUG_PRINTF(L"AStorage = %s", AStorage.c_str());
  Init();
}
//---------------------------------------------------------------------------
void TFar3Storage::Init()
{
  FRoot = 0;
}
//---------------------------------------------------------------------------
TFar3Storage::~TFar3Storage()
{
}
//---------------------------------------------------------------------------
bool TFar3Storage::Copy(TFar3Storage * Storage)
{
  Error(SNotImplemented, 3014);
  bool Result = true;
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TFar3Storage::GetSource() const
{
  return GetStorage();
}
//---------------------------------------------------------------------------
void TFar3Storage::SetAccessMode(TStorageAccessMode Value)
{
  THierarchicalStorage::SetAccessMode(Value);
}
//---------------------------------------------------------------------------
intptr_t TFar3Storage::OpenSubKeyInternal(intptr_t Root, const UnicodeString & SubKey, bool CanCreate)
{
  intptr_t NewRoot = 0;
  UnicodeString UnmungedSubKey = PuttyUnMungeStr(SubKey);
  if (CanCreate)
  {
    // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
    NewRoot = FPluginSettings.CreateSubKey(Root, UnmungedSubKey.c_str());
  }
  else
  {
    NewRoot = FPluginSettings.OpenSubKey(Root, UnmungedSubKey.c_str());
  }
  return NewRoot;
}
//---------------------------------------------------------------------------
bool TFar3Storage::DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate)
{
  // DEBUG_PRINTF(L"SubKey = %s, CanCreate = %d, Path = %d", SubKey.c_str(), CanCreate, Path);
  intptr_t OldRoot = FRoot;
  intptr_t Root = FRoot;
  bool Result = true;
  {
    UnicodeString subKey = MungedSubKey;
    assert(subKey.IsEmpty() || (subKey[subKey.Length()] != '\\'));
    // CutToChar(subKey, L'\\', false);
    while (!subKey.IsEmpty())
    {
      Root = OpenSubKeyInternal(Root, CutToChar(subKey, L'\\', false), CanCreate);
      Result &= Root != 0;
      // DEBUG_PRINTF(L"SubKey = %s, Result = %d", SubKey.c_str(), Result);
    }
    if (Result)
    {
      FSubKeyIds.push_back(OldRoot);
      FRoot = Root;
    }
  }
  // DEBUG_PRINTF(L"end, Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void TFar3Storage::CloseSubKey()
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
bool TFar3Storage::DeleteSubKey(const UnicodeString & SubKey)
{
  UnicodeString K;
  if (FKeyHistory->GetCount() == 0)
  {
    K = GetFullCurrentSubKey();
    FRoot = FPluginSettings.OpenSubKey(FRoot, K.c_str());
  }
  intptr_t Root = FPluginSettings.OpenSubKey(FRoot, SubKey.c_str());
  if (Root != 0)
  {
    return FPluginSettings.DeleteSubKey(Root);
  }
  return false;
}
//---------------------------------------------------------------------------
void TFar3Storage::GetSubKeyNames(TStrings * Strings)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  FarSettingsEnum settings = {sizeof(FarSettingsEnum),0,0,0};
  settings.Root = FRoot;
  if (FPluginSettings.GetSubKeyNames(settings))
  {
    for (size_t Index = 0; Index < settings.Count; ++Index)
    {
      // DEBUG_PRINTF(L"settings.Items[%d].Type = %d", Index, settings.Items[Index].Type);
      if (settings.Items[Index].Type == FST_SUBKEY)
      {
        Strings->Add(PuttyUnMungeStr(settings.Items[Index].Name));
      }
    }
  }
  // DEBUG_PRINTF(L"end, FRoot = %d", FRoot);
}
//---------------------------------------------------------------------------
void TFar3Storage::GetValueNames(TStrings * Strings) const
{
  // DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  Strings->Clear();
  FarSettingsEnum Settings = {sizeof(FarSettingsEnum),0,0,0};
  Settings.Root = FRoot;
  if (FPluginSettings.GetSubKeyNames(Settings))
  {
    for (size_t Index = 0; Index < Settings.Count; ++Index)
    {
      const struct FarSettingsName * Item = &Settings.Items[Index];
      Strings->Add(Item->Name);
    }
  }
  // DEBUG_PRINTF(L"end, Strings->GetCount() = %d", Strings->Count);
}
//---------------------------------------------------------------------------
bool TFar3Storage::DeleteValue(const UnicodeString & Name)
{
  return FPluginSettings.DeleteValue(FRoot, Name.c_str());
}
//---------------------------------------------------------------------------
bool TFar3Storage::DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi)
{
  Error(SNotImplemented, 3011);
  // DEBUG_PRINTF(L"begin, FRoot = %d", FRoot);
  UnicodeString K = PuttyMungeStr(SubKey);
  bool Result = true; // FPluginSettings.KeyExists(K);
  // DEBUG_PRINTF(L"end, FRoot = %d, K = %s, Result = %d", FRoot, K.c_str(), Result);
  return Result;
}
//---------------------------------------------------------------------------
bool TFar3Storage::ValueExists(const UnicodeString & Value) const
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Value = %s", FRoot, Value.c_str());
  bool Result = FPluginSettings.ValueExists(FRoot, Value.c_str());
  return Result;
}
//---------------------------------------------------------------------------
size_t TFar3Storage::BinaryDataSize(const UnicodeString & Name)
{
  size_t Result = FPluginSettings.BinaryDataSize(FRoot, Name.c_str());
  return Result;
}
//---------------------------------------------------------------------------
bool TFar3Storage::ReadBool(const UnicodeString & Name, bool Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
TDateTime TFar3Storage::ReadDateTime(const UnicodeString & Name, TDateTime Default)
{
  TDateTime Result;
  double Val = 0.0;
  void * Value = reinterpret_cast<void *>(&Val);
  size_t Sz = sizeof(Val);
  if (FPluginSettings.Get(FRoot, Name.c_str(), Value, Sz) == Sz)
  {
    Result = Val;
  }
  return Result;
}
//---------------------------------------------------------------------------
double TFar3Storage::ReadFloat(const UnicodeString & Name, double Default)
{
  double Result = 0.0;
  double Val = 0.0;
  void * Value = reinterpret_cast<void *>(&Val);
  size_t Sz = sizeof(Val);
  if (FPluginSettings.Get(FRoot, Name.c_str(), Value, Sz) == Sz)
  {
    Result = Val;
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TFar3Storage::ReadInteger(const UnicodeString & Name, intptr_t Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
__int64 TFar3Storage::ReadInt64(const UnicodeString & Name, __int64 Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}
//---------------------------------------------------------------------------
UnicodeString TFar3Storage::ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default)
{
  UnicodeString Result = FPluginSettings.Get(FRoot, Name.c_str(), Default.c_str());
  return Result;
}
//---------------------------------------------------------------------------
size_t TFar3Storage::ReadBinaryData(const UnicodeString & Name,
  void * Buffer, size_t Size)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Buffer, Size);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteBool(const UnicodeString & Name, bool Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteDateTime(const UnicodeString & Name, TDateTime AValue)
{
  double Val = AValue.operator double();
  void * Value = reinterpret_cast<void *>(&Val);
  size_t Sz = sizeof(Val);
  if (!FPluginSettings.Set(FRoot, Name.c_str(), Value, Sz))
  {
    // TODO: report error
  }
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteFloat(const UnicodeString & Name, double AValue)
{
  double Val = AValue;
  void * Value = reinterpret_cast<void *>(&Val);
  size_t sz = sizeof(Val);
  if (!FPluginSettings.Set(FRoot, Name.c_str(), Value, sz))
  {
    // TODO: report error
  }
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value.c_str());
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteInteger(const UnicodeString & Name, intptr_t Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteInt64(const UnicodeString & Name, __int64 Value)
{
  // DEBUG_PRINTF(L"begin, FRoot = %d, Name = %s", FRoot, Name.c_str());
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteBinaryData(const UnicodeString & Name,
  const void * Buffer, size_t Size)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Buffer, Size);
}
//---------------------------------------------------------------------------
