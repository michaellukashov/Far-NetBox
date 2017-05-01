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
  Init();
}

void TFar3Storage::Init()
{
  FRoot = 0;
}

TFar3Storage::~TFar3Storage()
{
}

bool TFar3Storage::Copy(TFar3Storage * /*Storage*/)
{
  Error(SNotImplemented, 3014);
  bool Result = true;
  return Result;
}

UnicodeString TFar3Storage::GetSource() const
{
  return GetStorage();
}

UnicodeString TFar3Storage::GetSource()
{
  return GetStorage();
}

void TFar3Storage::SetAccessMode(TStorageAccessMode Value)
{
  THierarchicalStorage::SetAccessMode(Value);
}

int TFar3Storage::OpenSubKeyInternal(int Root, const UnicodeString & SubKey, bool CanCreate)
{
  int NewRoot = 0;
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
  int OldRoot = FRoot;
  int Root = FRoot;
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

void TFar3Storage::CloseSubKey()
{
  THierarchicalStorage::CloseSubKey();
  // DebugAssert(FKeyHistory->GetCount() == FSubKeyIds.size() - 1);
  if (FKeyHistory->GetCount() && FSubKeyIds.size())
  {
    FRoot = FSubKeyIds.back();
    FSubKeyIds.pop_back();
  }
  else
  {
    FRoot = 0;
  }
}

bool TFar3Storage::DeleteSubKey(const UnicodeString & SubKey)
{
  UnicodeString K;
  if (FKeyHistory->GetCount() == 0)
  {
    K = GetFullCurrentSubKey();
    FRoot = FPluginSettings.OpenSubKey(FRoot, K.c_str());
  }
  intptr_t Root = FPluginSettings.OpenSubKey(FRoot, MungeKeyName(SubKey).c_str());
  if (Root != 0)
  {
    return FPluginSettings.DeleteSubKey(Root);
  }
  return false;
}

void TFar3Storage::GetSubKeyNames(TStrings * Strings)
{
  FarSettingsEnum Settings = {sizeof(FarSettingsEnum),0,0,0};
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

void TFar3Storage::GetValueNames(TStrings * Strings) const
{
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
}

bool TFar3Storage::DeleteValue(const UnicodeString & Name)
{
  return FPluginSettings.DeleteValue(FRoot, Name.c_str());
}

bool TFar3Storage::DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi)
{
  DebugUsedParam(ForceAnsi);
  // Error(SNotImplemented, 3011);
  UnicodeString K = PuttyMungeStr(SubKey);
  bool Result = FPluginSettings.ValueExists(FRoot, K.c_str());
  return Result;
}

bool TFar3Storage::ValueExists(const UnicodeString & Value) const
{
  bool Result = FPluginSettings.ValueExists(FRoot, Value.c_str());
  return Result;
}

size_t TFar3Storage::BinaryDataSize(const UnicodeString & Name) const
{
  size_t Result = FPluginSettings.BinaryDataSize(FRoot, Name.c_str());
  return Result;
}

bool TFar3Storage::ReadBool(const UnicodeString & Name, bool Default) const
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}

TDateTime TFar3Storage::ReadDateTime(const UnicodeString & Name, const TDateTime & Default) const
{
  TDateTime Result;
  double Val = 0.0;
  void * Value = reinterpret_cast<void *>(&Val);
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

double TFar3Storage::ReadFloat(const UnicodeString & Name, double Default) const
{
  double Result = 0.0;
  double Val = 0.0;
  void * Value = reinterpret_cast<void *>(&Val);
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

intptr_t TFar3Storage::ReadInteger(const UnicodeString & Name, intptr_t Default) const
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}

__int64 TFar3Storage::ReadInt64(const UnicodeString & Name, __int64 Default) const
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Default);
}

UnicodeString TFar3Storage::ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) const
{
  UnicodeString Result = FPluginSettings.Get(FRoot, Name.c_str(), Default.c_str());
  return Result;
}

size_t TFar3Storage::ReadBinaryData(const UnicodeString & Name,
  void * Buffer, size_t Size) const
{
  return FPluginSettings.Get(FRoot, Name.c_str(), Buffer, Size);
}

void TFar3Storage::WriteBool(const UnicodeString & Name, bool Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}

void TFar3Storage::WriteDateTime(const UnicodeString & Name, const TDateTime & AValue)
{
  double Val = AValue.operator double();
  void * Value = reinterpret_cast<void *>(&Val);
  size_t Sz = sizeof(Val);
  if (!FPluginSettings.Set(FRoot, Name.c_str(), Value, Sz))
  {
    // TODO: report error
  }
}

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

void TFar3Storage::WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value.c_str());
}

void TFar3Storage::WriteInteger(const UnicodeString & Name, intptr_t Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}

void TFar3Storage::WriteInt64(const UnicodeString & Name, __int64 Value)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Value);
}

void TFar3Storage::WriteBinaryData(const UnicodeString & Name,
  const void * Buffer, size_t Size)
{
  FPluginSettings.Set(FRoot, Name.c_str(), Buffer, Size);
}

