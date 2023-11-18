#include <Classes.hpp>
#include <Common.h>
#include <Exceptions.h>
#include <PuttyIntf.h>
#include "Far3Storage.h"
#include "TextsCore.h"


TFar3Storage::TFar3Storage(const UnicodeString & AStorage,
  const GUID & Guid, FARAPISETTINGSCONTROL SettingsControl) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FPluginSettings(Guid, SettingsControl)
{
}

void TFar3Storage::Init()
{
  THierarchicalStorage::Init();
  FRoot = 0;
}

TFar3Storage::~TFar3Storage() = default;

bool TFar3Storage::Copy(TFar3Storage * /*Storage*/)
{
  Error(SNotImplemented, 3014);
  const bool Result = true;
  return Result;
}

UnicodeString TFar3Storage::GetSource() const
{
  return GetStorage();
}

//UnicodeString TFar3Storage::GetSource()
//{
//  return GetStorage();
//}

//void TFar3Storage::SetAccessMode(TStorageAccessMode Value)
//{
//  THierarchicalStorage::SetAccessMode(Value);
//}

int32_t TFar3Storage::OpenSubKeyInternal(int32_t Root, const UnicodeString & SubKey, bool CanCreate)
{
  int32_t NewRoot = 0;
  if (CanCreate)
  {
    NewRoot = FPluginSettings.CreateSubKey(Root, SubKey.c_str());
  }
  else
  {
    NewRoot = FPluginSettings.OpenSubKey(Root, SubKey.c_str());
  }
  return NewRoot;
}

bool TFar3Storage::DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate)
{
  int32_t OldRoot = FRoot;
  int32_t Root = FRoot;
  bool Result = true;
  {
    UnicodeString subKey = MungedSubKey;
    DebugAssert(subKey.IsEmpty() || (subKey[subKey.Length()] != '\\'));
    // CutToChar(subKey, L'\\', false);
    while (!subKey.IsEmpty())
    {
      Root = OpenSubKeyInternal(Root, CutToChar(subKey, L'\\', false), CanCreate);
      Result &= Root != 0;
    }
    if (Result)
    {
      FSubKeyIds.push_back(OldRoot);
      FRoot = Root;
    }
  }
  return Result;
}

void TFar3Storage::DoCloseSubKey()
{
  THierarchicalStorage::CloseSubKey();
  // DebugAssert(FKeyHistory->GetCount() == FSubKeyIds.size() - 1);
  if (FKeyHistory.size() && FSubKeyIds.size())
  {
    FRoot = FSubKeyIds.back();
    FSubKeyIds.pop_back();
  }
  else
  {
    FRoot = 0;
  }
}

void TFar3Storage::DoDeleteSubKey(const UnicodeString & SubKey)
{
  UnicodeString K;
  if (FKeyHistory.size() == 0)
  {
    K = GetFullCurrentSubKey();
    FRoot = FPluginSettings.OpenSubKey(FRoot, K.c_str());
  }
  const size_t Root = FPluginSettings.OpenSubKey(FRoot, MungeKeyName(SubKey).c_str());
  if (Root != 0)
  {
    FPluginSettings.DeleteSubKey(Root);
  }
}

void TFar3Storage::DoGetSubKeyNames(TStrings * Strings)
{
  FarSettingsEnum Settings = {sizeof(FarSettingsEnum), 0, 0, 0};
  Settings.Root = FRoot;
  if (FPluginSettings.GetSubKeyNames(Settings))
  {
    for (size_t Index = 0; Index < Settings.Count; ++Index)
    {
      if (Settings.Items[Index].Type == FST_SUBKEY)
      {
        Strings->Add(PuttyUnMungeStr(Settings.Items[Index].Name));
      }
    }
  }
}

void TFar3Storage::DoGetValueNames(TStrings * Strings)
{
  Strings->Clear();
  FarSettingsEnum Settings = {sizeof(FarSettingsEnum), 0, 0, 0};
  Settings.Root = FRoot;
  if (FPluginSettings.GetSubKeyNames(Settings))
  {
    for (size_t Index = 0; Index < Settings.Count; ++Index)
    {
      const struct FarSettingsName *Item = &Settings.Items[Index];
      Strings->Add(Item->Name);
    }
  }
}

bool TFar3Storage::DoDeleteValue(const UnicodeString & Name)
{
  return FPluginSettings.DeleteValue(FRoot, Name.c_str());
}

int32_t TFar3Storage::DoBinaryDataSize(const UnicodeString & Name)
{
  const int32_t Result = nb::ToInt32(FPluginSettings.BinaryDataSize(FRoot, Name.c_str()));
  return Result;
}

bool TFar3Storage::DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi)
{
  DebugUsedParam(ForceAnsi);
  // Error(SNotImplemented, 3011);
  const UnicodeString K = PuttyMungeStr(SubKey);
  const bool Result = FPluginSettings.ValueExists(FRoot, K.c_str());
  return Result;
}

bool TFar3Storage::DoValueExists(const UnicodeString & Value)
{
  const bool Result = FPluginSettings.ValueExists(FRoot, Value.c_str());
  return Result;
}

bool TFar3Storage::DoReadBool(const UnicodeString & Name, bool Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}

TDateTime TFar3Storage::DoReadDateTime(const UnicodeString & Name, TDateTime Default)
{
  TDateTime Result;
  double Val = 0.0;
  void * Value = nb::ToPtr(&Val);
  const size_t Sz = sizeof(Val);
  if (FPluginSettings.Get(FRoot, Name.c_str(), Value, Sz) == Sz)
  {
    Result = Val;
  }
  else
  {
    Result = Default;
  }
  return Result;
}

double TFar3Storage::DoReadFloat(const UnicodeString & Name, double Default)
{
  double Result = 0.0;
  double Val = 0.0;
  void * Value = nb::ToPtr(&Val);
  size_t Sz = sizeof(Val);
  if (FPluginSettings.Get(FRoot, Name.c_str(), Value, Sz) == Sz)
  {
    Result = Val;
  }
  else
  {
    Result = Default;
  }
  return Result;
}

int32_t TFar3Storage::DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}

int64_t TFar3Storage::DoReadInt64(const UnicodeString & Name, int64_t Default)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}

UnicodeString TFar3Storage::DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default)
{
  UnicodeString Result = FPluginSettings.Get(FRoot, Name.c_str(), Default.c_str());
  return Result;
}

int32_t TFar3Storage::DoReadBinaryData(const UnicodeString & Name,
  void * Buffer, int32_t Size)
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Buffer, Size);
}

void TFar3Storage::DoWriteBool(const UnicodeString & Name, bool Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}

void TFar3Storage::DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value.c_str());
}

void TFar3Storage::DoWriteInteger(const UnicodeString & Name, int32_t Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}

void TFar3Storage::DoWriteInt64(const UnicodeString & Name, int64_t Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}

void TFar3Storage::DoWriteBinaryData(const UnicodeString & Name,
  const void * Buffer, int32_t Size)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Buffer, Size);
}

