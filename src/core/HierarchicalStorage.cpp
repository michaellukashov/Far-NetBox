
#include <vcl.h>
#pragma hdrstop

#include <rdestl/vector.h>
#include <Common.h>
#include <Exceptions.h>
#include <StrUtils.hpp>

#include "PuttyIntf.h"
#include "HierarchicalStorage.h"
//---------------------------------------------------------------------------
__removed #pragma package(smart_init)
//---------------------------------------------------------------------------
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch (...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch (...) { FFailed++; }
//---------------------------------------------------------------------------
UnicodeString MungeStr(UnicodeString Str, bool ForceAnsi, bool Value)
{
  RawByteString Source;
  if (ForceAnsi)
  {
    Source = RawByteString(AnsiString(Str));
  }
  else
  {
    Source = RawByteString(UTF8String(Str));
    if (Source.Length() > Str.Length())
    {
      Source.Insert(CONST_BOM, 1);
    }
  }
  // should contain ASCII characters only
  RawByteString Dest;
  char *Buffer = Dest.SetLength(Source.Length() * 3 + 1);
  putty_mungestr(Source.c_str(), Buffer);
  PackStr(Dest);
  if (Value)
  {
    // We do not want to munge * in PasswordMask
    Dest = ReplaceStr(Dest, L"%2A", L"*");
  }
  return UnicodeString(Dest.c_str(), Dest.Length());
}
//---------------------------------------------------------------------------
UnicodeString UnMungeStr(UnicodeString Str)
{
  // Str should contain ASCII characters only
  RawByteString Source = Str;
  RawByteString Dest;
  char *Buffer = Dest.SetLength(Source.GetLength());
  putty_unmungestr(Source.c_str(), Buffer, nb::ToInt(Source.GetLength()));
  // Cut the string at null character
  PackStr(Dest);
  UnicodeString Result;
  const AnsiString Bom(CONST_BOM);
  if (Dest.Pos(Bom.c_str()) == 1)
  {
    Dest.Delete(1, Bom.GetLength());
    Result = UTF8ToString(Dest);
  }
  else
  {
    Result = AnsiToString(Dest);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString PuttyMungeStr(UnicodeString Str)
{
  return MungeStr(Str, false, false);
}

UnicodeString PuttyUnMungeStr(UnicodeString Str)
{
  return UnMungeStr(Str);
}
//---------------------------------------------------------------------------
UnicodeString MungeIniName(UnicodeString Str)
{
  intptr_t P = Str.Pos(L"=");
  // make this fast for now
  if (P > 0)
  {
    return ReplaceStr(Str, L"=", L"%3D");
  }
  return Str;
}
//---------------------------------------------------------------------------
UnicodeString UnMungeIniName(UnicodeString Str)
{
  intptr_t P = Str.Pos(L"%3D");
  // make this fast for now
  if (P > 0)
  {
    return ReplaceStr(Str, L"%3D", L"=");
  }
  return Str;
}
//===========================================================================
THierarchicalStorage::THierarchicalStorage(UnicodeString AStorage) noexcept :
  FStorage(AStorage),
  FKeyHistory(std::make_unique<TStringList>())
{
  SetAccessMode(smRead);
  SetExplicit(false);
  ForceSave = false;
  // While this was implemented in 5.0 already, for some reason
  // it was disabled (by mistake?). So although enabled for 5.6.1 only,
  // data written in Unicode/UTF8 can be read by all versions back to 5.0.
  SetForceAnsi(false);
  SetMungeStringValues(true);
}
//---------------------------------------------------------------------------
THierarchicalStorage::~THierarchicalStorage() noexcept
{
  __removed SAFE_DESTROY(FKeyHistory);
}
//---------------------------------------------------------------------------
void THierarchicalStorage::Flush()
{
}
//---------------------------------------------------------------------------
void THierarchicalStorage::SetAccessModeProtected(TStorageAccessMode Value)
{
  FAccessMode = Value;
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::GetCurrentSubKeyMunged() const
{
  if (FKeyHistory->GetCount())
  {
    return FKeyHistory->GetString(FKeyHistory->GetCount() - 1);
  }
  return L"";
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::GetCurrentSubKey() const
{
  return UnMungeStr(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::OpenRootKey(bool CanCreate)
{
  return OpenSubKey(L"", CanCreate);
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::MungeKeyName(UnicodeString Key)
{
  UnicodeString Result = MungeStr(Key, GetForceAnsi(), false);
  // if there's already ANSI-munged subkey, keep ANSI munging
  if ((Result != Key) && !GetForceAnsi() && DoKeyExists(Key, true))
  {
    Result = MungeStr(Key, true, false);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::OpenSubKey(UnicodeString ASubKey, bool CanCreate, bool Path)
{
  UnicodeString MungedKey;
  UnicodeString Key = ASubKey;
  if (Path)
  {
    DebugAssert(Key.IsEmpty() || (Key[Key.Length()] != L'\\'));
    while (!Key.IsEmpty())
    {
      if (!MungedKey.IsEmpty())
      {
        MungedKey += L'\\';
      }
      MungedKey += MungeKeyName(CutToChar(Key, L'\\', false));
    }
  }
  else
  {
    MungedKey = MungeKeyName(Key);
  }

  bool Result = DoOpenSubKey(MungedKey, CanCreate);

  if (Result)
  {
    FKeyHistory->Add(::IncludeTrailingBackslash(GetCurrentSubKey() + MungedKey));
  }

  return Result;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::CloseSubKey()
{
  if (FKeyHistory->GetCount() > 0)
  {
    FKeyHistory->Delete(FKeyHistory->GetCount() - 1);
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::CloseAll()
{
  while (!GetCurrentSubKey().IsEmpty())
  {
    CloseSubKey();
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ClearSubKeys()
{
  std::unique_ptr<TStringList> SubKeys(std::make_unique<TStringList>());
  try__finally
  {
    GetSubKeyNames(SubKeys.get());
    for (intptr_t Index = 0; Index < SubKeys->GetCount(); ++Index)
    {
      RecursiveDeleteSubKey(SubKeys->GetString(Index));
    }
  },
  __finally__removed
  ({
    delete SubKeys;
  }) end_try__finally
}
//---------------------------------------------------------------------------
void THierarchicalStorage::RecursiveDeleteSubKey(UnicodeString Key)
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
  std::unique_ptr<TStrings> SubKeys(std::make_unique<TStringList>());
  try__finally
  {
    GetSubKeyNames(SubKeys.get());
    Result = SubKeys->GetCount() > 0;
  },
  __finally__removed
  ({
    delete SubKeys;
  }) end_try__finally
  return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::HasSubKey(UnicodeString SubKey)
{
  bool Result = OpenSubKey(SubKey, false);
  if (Result)
  {
    CloseSubKey();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::KeyExists(UnicodeString SubKey)
{
  return DoKeyExists(SubKey, GetForceAnsi());
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ReadValues(TStrings *Strings,
  bool MaintainKeys)
{
  std::unique_ptr<TStrings> Names(std::make_unique<TStringList>());
  try__finally
  {
    GetValueNames(Names.get());
    for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
    {
      if (MaintainKeys)
      {
        Strings->Add(FORMAT("%s=%s", Names->GetString(Index),
              ReadString(Names->GetString(Index), L"")));
      }
      else
      {
        Strings->Add(ReadString(Names->GetString(Index), L""));
      }
    }
  },
  __finally__removed
  ({
    delete Names;
  }) end_try__finally
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ClearValues()
{
  std::unique_ptr<TStrings> Names(std::make_unique<TStringList>());
  try__finally
  {
    GetValueNames(Names.get());
    for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
    {
      DeleteValue(Names->GetString(Index));
    }
  },
  __finally__removed
  ({
    delete Names;
  }) end_try__finally
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteValues(TStrings *Strings,
  bool MaintainKeys)
{
  ClearValues();

  if (Strings)
  {
    for (intptr_t Index = 0; Index < Strings->GetCount(); ++Index)
    {
      if (MaintainKeys)
      {
        DebugAssert(Strings->GetString(Index).Pos(L"=") > 1);
        WriteString(Strings->GetName(Index), Strings->GetValue(Strings->GetName(Index)));
      }
      else
      {
        WriteString(::IntToStr(Index), Strings->GetString(Index));
      }
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::ReadString(UnicodeString Name, UnicodeString Default) const
{
  UnicodeString Result;
  if (GetMungeStringValues())
  {
    Result = UnMungeStr(ReadStringRaw(Name, MungeStr(Default, GetForceAnsi(), true)));
  }
  else
  {
    Result = ReadStringRaw(Name, Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
RawByteString THierarchicalStorage::ReadBinaryData(UnicodeString Name) const
{
  size_t Size = BinaryDataSize(Name);
  RawByteString Value;
  char *Buf = Value.SetLength(Size);
  ReadBinaryData(Name, Buf, Size);
  return Value;
}
//---------------------------------------------------------------------------
RawByteString THierarchicalStorage::ReadStringAsBinaryData(UnicodeString Name, RawByteString Default) const
{
  UnicodeString UnicodeDefault = AnsiToString(Default);
  // This should be exactly the same operation as calling ReadString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  UnicodeString String = ReadString(Name, UnicodeDefault);
  AnsiString Ansi = AnsiString(String);
  RawByteString Result = RawByteString(Ansi);
  return Result;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteString(UnicodeString Name, UnicodeString Value)
{
  if (GetMungeStringValues())
  {
    WriteStringRaw(Name, MungeStr(Value, GetForceAnsi(), true));
  }
  else
  {
    WriteStringRaw(Name, Value);
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteBinaryData(UnicodeString Name,
  const RawByteString Value)
{
  WriteBinaryData(Name, Value.c_str(), Value.Length());
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteBinaryDataAsString(UnicodeString Name, const RawByteString Value)
{
  // This should be exactly the same operation as calling WriteString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  WriteString(Name, AnsiToString(Value));
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::IncludeTrailingBackslash(UnicodeString S)
{
  // expanded from ?: as it caused memory leaks
  if (S.IsEmpty())
  {
    return S;
  }
  return ::IncludeTrailingBackslash(S);
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::ExcludeTrailingBackslash(UnicodeString S)
{
  // expanded from ?: as it caused memory leaks
  if (S.IsEmpty())
  {
    return S;
  }
  return ::ExcludeTrailingBackslash(S);
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::GetTemporaryProtected() const
{
  return false;
}
//===========================================================================
TRegistryStorage::TRegistryStorage(UnicodeString AStorage) noexcept :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(std::make_unique<TRegistry>())
{
  FWowMode = 0;
}
//---------------------------------------------------------------------------
TRegistryStorage::TRegistryStorage(UnicodeString AStorage, HKEY ARootKey, REGSAM WowMode) noexcept :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(std::make_unique<TRegistry>())
{
  FWowMode = WowMode;
  FRegistry->SetRootKey(ARootKey);
}
//---------------------------------------------------------------------------
void TRegistryStorage::Init()
{
  FFailed = 0;
  FRegistry->SetAccess(KEY_READ | FWowMode);
}
//---------------------------------------------------------------------------
TRegistryStorage::~TRegistryStorage() noexcept
{
  __removed SAFE_DESTROY(FRegistry);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::Copy(TRegistryStorage *Storage)
{
  TRegistry *Registry = Storage->FRegistry.get();
  bool Result = true;
  std::unique_ptr<TStrings> Names(std::make_unique<TStringList>());
  try__finally
  {
    rde::vector<uint8_t> Buffer(1024);
    Registry->GetValueNames(Names.get());
    intptr_t Index = 0;
    while ((Index < Names->GetCount()) && Result)
    {
      UnicodeString Name = MungeStr(Names->GetString(Index), GetForceAnsi(), false);
      DWORD Size = nb::ToDWord(Buffer.size());
      DWORD Type;
      int RegResult;
      do
      {
        RegResult = ::RegQueryValueEx(Registry->GetCurrentKey(), Name.c_str(), nullptr,
          &Type, &Buffer[0], &Size);
        if (RegResult == ERROR_MORE_DATA)
        {
          Buffer.resize(Size);
        }
      } while (RegResult == ERROR_MORE_DATA);

      Result = (RegResult == ERROR_SUCCESS);
      if (Result)
      {
        RegResult = ::RegSetValueEx(FRegistry->GetCurrentKey(), Name.c_str(), 0, Type,
          &Buffer[0], Size);
        Result = (RegResult == ERROR_SUCCESS);
      }

      ++Index;
    }
  },
  __finally__removed
  ({
    delete Names;
  }) end_try__finally
  return Result;
}

UnicodeString TRegistryStorage::GetSourceProtected() const
{
  return const_cast<TRegistryStorage *>(this)->GetSourceProtected();
}
//---------------------------------------------------------------------------
UnicodeString TRegistryStorage::GetSourceProtected()
{
  return RootKeyToStr(FRegistry->GetRootKey()) + L"\\" + GetStorage();
}
//---------------------------------------------------------------------------
void TRegistryStorage::SetAccessModeProtected(TStorageAccessMode Value)
{
  THierarchicalStorage::SetAccessModeProtected(Value);
  if (FRegistry)
  {
    switch (GetAccessMode())
    {
    case smRead:
        FRegistry->SetAccess(KEY_READ | FWowMode);
      break;

    case smReadWrite:
    default:
        FRegistry->SetAccess(KEY_READ | KEY_WRITE | FWowMode);
      break;
    }
  }
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DoOpenSubKey(UnicodeString SubKey, bool CanCreate)
{
  UnicodeString PrevPath;
  bool WasOpened = (FRegistry->GetCurrentKey() != nullptr);
  if (WasOpened)
  {
    PrevPath = FRegistry->GetCurrentPath();
    DebugAssert(SamePaths(PrevPath, Storage() + GetCurrentSubKeyMunged()));
    FRegistry->CloseKey();
  }
  UnicodeString K = ExcludeTrailingBackslash(Storage() + CurrentSubKey() + SubKey);
  bool Result = FRegistry->OpenKey(K, CanCreate);
  if (!Result && WasOpened)
  {
    FRegistry->OpenKey(PrevPath, false);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRegistryStorage::CloseSubKey()
{
  FRegistry->CloseKey();
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount())
  {
    FRegistry->OpenKey(GetStorage() + GetCurrentSubKeyMunged(), True);
  }
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteSubKey(UnicodeString SubKey)
{
  UnicodeString Key;
  if (FKeyHistory->GetCount() == 0)
  {
    Key = GetStorage() + GetCurrentSubKey();
  }
  Key += MungeKeyName(SubKey);
  return FRegistry->DeleteKey(Key);
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetSubKeyNames(TStrings *Strings)
{
  FRegistry->GetKeyNames(Strings);
  for (intptr_t Index = 0; Index < Strings->GetCount(); ++Index)
  {
    Strings->SetString(Index, UnMungeStr(Strings->GetString(Index)));
  }
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetValueNames(TStrings *Strings) const
{
  FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteValue(UnicodeString Name)
{
  return FRegistry->DeleteValue(Name);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DoKeyExists(UnicodeString SubKey, bool AForceAnsi)
{
  UnicodeString Key = MungeStr(SubKey, AForceAnsi, false);
  bool Result = FRegistry->KeyExists(Key);
  return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::ValueExists(UnicodeString Value) const
{
  bool Result = FRegistry->ValueExists(Value);
  return Result;
}
//---------------------------------------------------------------------------
size_t TRegistryStorage::BinaryDataSize(UnicodeString Name) const
{
  size_t Result = FRegistry->GetDataSize(Name);
  return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::ReadBool(UnicodeString Name, bool Default) const
{
  READ_REGISTRY(ReadBool);
}
//---------------------------------------------------------------------------
TDateTime TRegistryStorage::ReadDateTime(UnicodeString Name, const TDateTime &Default) const
{
  READ_REGISTRY(ReadDateTime);
}
//---------------------------------------------------------------------------
double TRegistryStorage::ReadFloat(UnicodeString Name, double Default) const
{
  READ_REGISTRY(ReadFloat);
}
//---------------------------------------------------------------------------
intptr_t TRegistryStorage::ReadIntPtr(UnicodeString Name, intptr_t Default) const
{
  READ_REGISTRY(ReadIntPtr);
}
//---------------------------------------------------------------------------
int TRegistryStorage::ReadInteger(UnicodeString Name, int Default) const
{
  READ_REGISTRY(ReadInteger);
}
//---------------------------------------------------------------------------
int64_t TRegistryStorage::ReadInt64(UnicodeString Name, int64_t Default) const
{
  int64_t Result = Default;
  if (FRegistry->ValueExists(Name))
  {
    try
    {
      FRegistry->ReadBinaryData(Name, &Result, sizeof(Result));
    }
    catch (...)
    {
      FFailed++;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TRegistryStorage::ReadStringRaw(UnicodeString Name, UnicodeString Default) const
{
  READ_REGISTRY(ReadString);
}
//---------------------------------------------------------------------------
size_t TRegistryStorage::ReadBinaryData(UnicodeString Name,
  void *Buffer, size_t Size) const
{
  size_t Result;
  if (FRegistry->ValueExists(Name))
  {
    try
    {
      Result = FRegistry->ReadBinaryData(Name, Buffer, Size);
    }
    catch (...)
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
void TRegistryStorage::WriteBool(UnicodeString Name, bool Value)
{
  WRITE_REGISTRY(WriteBool);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteDateTime(UnicodeString Name, const TDateTime &Value)
{
  WRITE_REGISTRY(WriteDateTime);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteFloat(UnicodeString Name, double Value)
{
  WRITE_REGISTRY(WriteFloat);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteStringRaw(UnicodeString Name, UnicodeString Value)
{
  WRITE_REGISTRY(WriteString);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteIntPtr(UnicodeString Name, intptr_t Value)
{
  WRITE_REGISTRY(WriteIntPtr);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteInteger(UnicodeString Name, int Value)
{
  WRITE_REGISTRY(WriteInteger);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteInt64(UnicodeString Name, int64_t Value)
{
  try
  {
    FRegistry->WriteBinaryData(Name, &Value, sizeof(Value));
  }
  catch (...)
  {
    FFailed++;
  }
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteBinaryData(UnicodeString AName,
  const void *ABuffer, size_t Size)
{
  try
  {
    FRegistry->WriteBinaryData(AName, const_cast<void *>(ABuffer), Size);
  }
  catch (...)
  {
    FFailed++;
  }
}
//---------------------------------------------------------------------------
intptr_t TRegistryStorage::GetFailedProtected() const
{
  intptr_t Result = FFailed;
  FFailed = 0;
  return Result;
}
//===========================================================================
#if 0
TCustomIniFileStorage::TCustomIniFileStorage(UnicodeString Storage, TCustomIniFile *IniFile) :
  THierarchicalStorage(Storage),
  FIniFile(IniFile),
  FMasterStorageOpenFailures(0),
  FOpeningSubKey(false)
{
}
//---------------------------------------------------------------------------
TCustomIniFileStorage::~TCustomIniFileStorage()
{
  delete FIniFile;
}
//---------------------------------------------------------------------------
UnicodeString TCustomIniFileStorage::GetSource()
{
  return Storage;
}
//---------------------------------------------------------------------------
UnicodeString TCustomIniFileStorage::GetCurrentSection()
{
  return ExcludeTrailingBackslash(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::CacheSections()
{
  if (FSections.get() == nullptr)
  {
    FSections.reset(std::make_unique<TStringList>());
    FIniFile->ReadSections(FSections.get());
    FSections->Sorted = true; // has to set only after reading as ReadSections reset it to false
  }
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::ResetCache()
{
  FSections.reset(nullptr);
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::SetAccessMode(TStorageAccessMode value)
{
  if (FMasterStorage.get() != nullptr)
  {
    FMasterStorage->AccessMode = value;
  }
  THierarchicalStorage::SetAccessMode(value);
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::DoOpenSubKey(UnicodeString SubKey, bool CanCreate)
{
  bool Result = CanCreate;

  if (!Result)
  {
    CacheSections();
    UnicodeString NewKey = ExcludeTrailingBackslash(CurrentSubKey + SubKey);
    if (FSections->Count > 0)
    {
      int Index = -1;
      Result = FSections->Find(NewKey, Index);
      if (!Result &&
        (Index < FSections->Count) &&
        (FSections->Strings[Index].SubString(1, NewKey.Length() + 1) == NewKey + L"\\"))
      {
        Result = true;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::OpenRootKey(bool CanCreate)
{
  // Not supported with master storage.
  // Actually currently, we use OpenRootKey with TRegistryStorage only.
  DebugAssert(FMasterStorage.get() == nullptr);

  return THierarchicalStorage::OpenRootKey(CanCreate);
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::OpenSubKey(UnicodeString Key, bool CanCreate, bool Path)
{
  bool Result;

  {
    TAutoFlag Flag(FOpeningSubKey); nb::used(Flag);
    Result = THierarchicalStorage::OpenSubKey(Key, CanCreate, Path);
  }

  if (FMasterStorage.get() != nullptr)
  {
    if (FMasterStorageOpenFailures > 0)
    {
      FMasterStorageOpenFailures++;
    }
    else
    {
      bool MasterResult = FMasterStorage->OpenSubKey(Key, CanCreate, Path);
      if (!Result && MasterResult)
      {
        Result = THierarchicalStorage::OpenSubKey(Key, true, Path);
        DebugAssert(Result);
      }
      else if (Result && !MasterResult)
      {
        FMasterStorageOpenFailures++;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::CloseSubKey()
{
  THierarchicalStorage::CloseSubKey();

  // What we are called to restore previous key from OpenSubKey,
  // when opening path component fails, the master storage was not involved yet
  if (!FOpeningSubKey && (FMasterStorage.get() != nullptr))
  {
    if (FMasterStorageOpenFailures > 0)
    {
      FMasterStorageOpenFailures--;
    }
    else
    {
      FMasterStorage->CloseSubKey();
    }
  }
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::DeleteSubKey(UnicodeString SubKey)
{
  bool Result;
  try
  {
    ResetCache();
    FIniFile->EraseSection(CurrentSubKey + MungeKeyName(SubKey));
    Result = true;
  }
  catch (...)
  {
    Result = false;
  }
  if (HandleByMasterStorage())
  {
    Result = FMasterStorage->DeleteSubKey(SubKey);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::GetSubKeyNames(Classes::TStrings *Strings)
{
  Strings->Clear();
  if (HandleByMasterStorage())
  {
    FMasterStorage->GetSubKeyNames(Strings);
  }
  CacheSections();
  for (int i = 0; i < FSections->Count; i++)
  {
    UnicodeString Section = FSections->Strings[i];
    if (AnsiCompareText(CurrentSubKey,
        Section.SubString(1, CurrentSubKey.Length())) == 0)
    {
      UnicodeString SubSection = Section.SubString(CurrentSubKey.Length() + 1,
          Section.Length() - CurrentSubKey.Length());
      int P = SubSection.Pos(L"\\");
      if (P)
      {
        SubSection.SetLength(P - 1);
      }
      if (Strings->IndexOf(SubSection) < 0)
      {
        Strings->Add(UnMungeStr(SubSection));
      }
    }
  }
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::GetValueNames(Classes::TStrings *Strings)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->GetValueNames(Strings);
  }
  FIniFile->ReadSection(CurrentSection, Strings);
  for (int Index = 0; Index < Strings->Count; Index++)
  {
    Strings->Strings[Index] = UnMungeIniName(Strings->Strings[Index]);
  }
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::DoKeyExists(UnicodeString SubKey, bool AForceAnsi)
{
  return
    (HandleByMasterStorage() && FMasterStorage->DoKeyExists(SubKey, AForceAnsi)) ||
    FIniFile->SectionExists(CurrentSubKey + MungeStr(SubKey, AForceAnsi, false));
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::DoValueExists(UnicodeString Value)
{
  return FIniFile->ValueExists(CurrentSection, MungeIniName(Value));
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::ValueExists(UnicodeString Value)
{
  return
    (HandleByMasterStorage() && FMasterStorage->ValueExists(Value)) ||
    DoValueExists(Value);
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::DeleteValue(UnicodeString Name)
{
  bool Result = true;
  if (HandleByMasterStorage())
  {
    Result = FMasterStorage->DeleteValue(Name);
  }
  ResetCache();
  FIniFile->DeleteKey(CurrentSection, MungeIniName(Name));
  return Result;
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::HandleByMasterStorage()
{
  return
    (FMasterStorage.get() != nullptr) &&
    (FMasterStorageOpenFailures == 0);
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::HandleReadByMasterStorage(UnicodeString Name)
{
  return HandleByMasterStorage() && !DoValueExists(Name);
}
//---------------------------------------------------------------------------
size_t TCustomIniFileStorage::BinaryDataSize(UnicodeString Name)
{
  if (HandleReadByMasterStorage(Name))
  {
    return FMasterStorage->BinaryDataSize(Name);
  }
  else
  {
    return ReadStringRaw(Name, L"").Length() / 2;
  }
}
//---------------------------------------------------------------------------
bool TCustomIniFileStorage::ReadBool(UnicodeString Name, bool Default)
{
  if (HandleReadByMasterStorage(Name))
  {
    return FMasterStorage->ReadBool(Name, Default);
  }
  else
  {
    return FIniFile->ReadBool(CurrentSection, MungeIniName(Name), Default);
  }
}
//---------------------------------------------------------------------------
int TCustomIniFileStorage::ReadInteger(UnicodeString Name, int Default)
{
  int Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadInteger(Name, Default);
  }
  else
  {
    Result = FIniFile->ReadInteger(CurrentSection, MungeIniName(Name), Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
__int64 TCustomIniFileStorage::ReadInt64(UnicodeString Name, __int64 Default)
{
  __int64 Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadInt64(Name, Default);
  }
  else
  {
    Result = Default;
    UnicodeString Str;
    Str = ReadStringRaw(Name, L"");
    if (!Str.IsEmpty())
    {
      Result = StrToInt64Def(Str, Default);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime TCustomIniFileStorage::ReadDateTime(UnicodeString Name, TDateTime Default)
{
  TDateTime Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadDateTime(Name, Default);
  }
  else
  {
    UnicodeString Value = FIniFile->ReadString(CurrentSection, MungeIniName(Name), L"");
    if (Value.IsEmpty())
    {
      Result = Default;
    }
    else
    {
      try
      {
        RawByteString Raw = HexToBytes(Value);
        if (ToSizeT(Raw.Length()) == sizeof(Result))
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
  }

  return Result;
}
//---------------------------------------------------------------------------
double TCustomIniFileStorage::ReadFloat(UnicodeString Name, double Default)
{
  double Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadFloat(Name, Default);
  }
  else
  {
    UnicodeString Value = FIniFile->ReadString(CurrentSection, MungeIniName(Name), L"");
    if (Value.IsEmpty())
    {
      Result = Default;
    }
    else
    {
      try
      {
        RawByteString Raw = HexToBytes(Value);
        if (ToSizeT(Raw.Length()) == sizeof(Result))
        {
          memcpy(&Result, Raw.c_str(), sizeof(Result));
        }
        else
        {
          Result = ToDouble(StrToFloat(Value));
        }
      }
      catch(...)
      {
        Result = Default;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TCustomIniFileStorage::ReadStringRaw(UnicodeString Name, UnicodeString Default)
{
  UnicodeString Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadStringRaw(Name, Default);
  }
  else
  {
    Result = FIniFile->ReadString(CurrentSection, MungeIniName(Name), Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
size_t TCustomIniFileStorage::ReadBinaryData(UnicodeString Name,
  void *Buffer, size_t Size)
{
  size_t Len;
  if (HandleReadByMasterStorage(Name))
  {
    FMasterStorage->ReadBinaryData(Name, Buffer, Size);
  }
  else
  {
    RawByteString Value = HexToBytes(ReadStringRaw(Name, L""));
    Len = Value.Length();
    if (Size > Len)
    {
      Size = Len;
    }
    DebugAssert(Buffer);
    memcpy(Buffer, Value.c_str(), Size);
  }
  return Size;
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::WriteBool(UnicodeString Name, bool Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteBool(Name, Value);
  }
  ResetCache();
  FIniFile->WriteBool(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::WriteInteger(UnicodeString Name, int Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteInteger(Name, Value);
  }
  ResetCache();
  FIniFile->WriteInteger(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::WriteInt64(UnicodeString Name, __int64 Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteInt64(Name, Value);
  }
  DoWriteStringRaw(Name, IntToStr(Value));
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::WriteDateTime(UnicodeString Name, TDateTime Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteDateTime(Name, Value);
  }
  DoWriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::WriteFloat(UnicodeString Name, double Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteFloat(Name, Value);
  }
  DoWriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::DoWriteStringRaw(UnicodeString Name, UnicodeString Value)
{
  ResetCache();
  FIniFile->WriteString(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::WriteStringRaw(UnicodeString Name, UnicodeString Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteStringRaw(Name, Value);
  }
  DoWriteStringRaw(Name, Value);
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::DoWriteBinaryData(UnicodeString Name,
  const void *Buffer, int Size)
{
  DoWriteStringRaw(Name, BytesToHex(RawByteString(static_cast<const char *>(Buffer), Size)));
}
//---------------------------------------------------------------------------
void TCustomIniFileStorage::WriteBinaryData(UnicodeString Name,
  const void *Buffer, int Size)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteBinaryData(Name, Buffer, Size);
  }
  DoWriteBinaryData(Name, Buffer, Size);
}
//===========================================================================
TIniFileStorage *TIniFileStorage::CreateFromPath(UnicodeString AStorage)
{
  // The code was originally inline in the parent contructor call in the TIniFileStorage::TIniFileStorage [public originally].
  // But if the TMemIniFile contructor throws (e.g. because the INI file is locked), the exception causes access violation.
  // Moving the code to a factory solves this.
  TMemIniFile *IniFile = new TMemIniFile(AStorage);
  return new TIniFileStorage(AStorage, IniFile);
}
//---------------------------------------------------------------------------
TIniFileStorage::TIniFileStorage(UnicodeString AStorage, TCustomIniFile *IniFile):
  TCustomIniFileStorage(AStorage, IniFile)
{
  FOriginal = new TStringList();
  dynamic_cast<TMemIniFile *>(FIniFile)->GetStrings(FOriginal);
  ApplyOverrides();
}
//---------------------------------------------------------------------------
void TIniFileStorage::Flush()
{
  if (FMasterStorage.get() != nullptr)
  {
    FMasterStorage->Flush();
  }
  if (FOriginal != nullptr)
  {
    TStrings *Strings = new TStringList;
    try
    {
      dynamic_cast<TMemIniFile *>(FIniFile)->GetStrings(Strings);
      if (!Strings->Equals(FOriginal))
      {
        int Attr;
        // preserve attributes (especially hidden)
        bool Exists = FileExists(ApiPath(Storage));
        if (Exists)
        {
          Attr = GetFileAttributes(ApiPath(Storage).c_str());
        }
        else
        {
          Attr = FILE_ATTRIBUTE_NORMAL;
        }

        if (FLAGSET(Attr, FILE_ATTRIBUTE_READONLY) && ForceSave)
        {
          SetFileAttributes(ApiPath(Storage).c_str(), Attr & ~FILE_ATTRIBUTE_READONLY);
        }

        HANDLE Handle = CreateFile(ApiPath(Storage).c_str(), GENERIC_READ | GENERIC_WRITE,
            0, nullptr, CREATE_ALWAYS, Attr, 0);

        if (Handle == INVALID_HANDLE_VALUE)
        {
          // "access denied" errors upon implicit saves to existing file are ignored
          if (Explicit || !Exists || (GetLastError() != ERROR_ACCESS_DENIED))
          {
            throw EOSExtException(FMTLOAD((Exists ? WRITE_ERROR : CREATE_FILE_ERROR), Storage));
          }
        }
        else
        {
          TStream *Stream = new THandleStream(int(Handle));
          try
          {
            Strings->SaveToStream(Stream);
          }
          __finally
          {
            SAFE_CLOSE_HANDLE(Handle);
            delete Stream;
          }
        }
      }
    }
    __finally
    {
      delete FOriginal;
      FOriginal = nullptr;
      delete Strings;
    }
  }
}
//---------------------------------------------------------------------------
TIniFileStorage::~TIniFileStorage()
{
  Flush();
}
//---------------------------------------------------------------------------
void TIniFileStorage::ApplyOverrides()
{
  UnicodeString OverridesKey = IncludeTrailingBackslash(L"Override");

  CacheSections();
  for (int i = 0; i < FSections->Count; i++)
  {
    UnicodeString Section = FSections->Strings[i];

    if (::SameText(OverridesKey,
          Section.SubString(1, OverridesKey.Length())))
    {
      UnicodeString SubKey = Section.SubString(OverridesKey.Length() + 1,
        Section.Length() - OverridesKey.Length());

      // this all uses raw names (munged)
      TStrings *Names = new TStringList;
      try
      {
        FIniFile->ReadSection(Section, Names);

        for (int ii = 0; ii < Names->Count; ii++)
        {
          UnicodeString Name = Names->Strings[ii];
          UnicodeString Value = FIniFile->ReadString(Section, Name, L"");
          FIniFile->WriteString(SubKey, Name, Value);
        }
      }
      __finally
      {
        delete Names;
      }

      FIniFile->EraseSection(Section);
      ResetCache();
    }
  }
}
//===========================================================================
enum TWriteMode { wmAllow, wmFail, wmIgnore };
//---------------------------------------------------------------------------
class TOptionsIniFile : public TCustomIniFile
{
public:
  TOptionsIniFile(TStrings *Options, TWriteMode WriteMode, UnicodeString RootKey);

  virtual UnicodeString ReadString(UnicodeString Section, UnicodeString Ident, UnicodeString Default);
  virtual void WriteString(UnicodeString Section, UnicodeString Ident, UnicodeString Value);
  virtual void ReadSection(UnicodeString Section, TStrings *Strings);
  virtual void ReadSections(TStrings *Strings);
  virtual void ReadSectionValues(UnicodeString Section, TStrings *Strings);
  virtual void EraseSection(UnicodeString Section);
  virtual void DeleteKey(UnicodeString Section, UnicodeString Ident);
  virtual void UpdateFile();
  // Hoisted overload
  void ReadSections(UnicodeString Section, TStrings *Strings);
  // Ntb, we can implement ValueExists more efficiently than the TCustomIniFile.ValueExists

private:
  TStrings *FOptions;
  TWriteMode FWriteMode;
  UnicodeString FRootKey;

  bool AllowWrite();
  void NotImplemented();
  bool AllowSection(UnicodeString & Section);
  UnicodeString FormatKey(UnicodeString & Section, UnicodeString & Ident);
};
//---------------------------------------------------------------------------
TOptionsIniFile::TOptionsIniFile(TStrings *Options, TWriteMode WriteMode, UnicodeString RootKey) :
  TCustomIniFile(UnicodeString())
{
  FOptions = Options;
  FWriteMode = WriteMode;
  FRootKey = RootKey;
  if (!FRootKey.IsEmpty())
  {
    FRootKey += PathDelim;
  }
}
//---------------------------------------------------------------------------
void TOptionsIniFile::NotImplemented()
{
  throw Exception("Not implemented");
}
//---------------------------------------------------------------------------
bool TOptionsIniFile::AllowWrite()
{
  switch (FWriteMode)
  {
  case wmAllow:
    return true;

  case wmFail:
    NotImplemented();
    return false; // never gets here

  case wmIgnore:
    return false;

  default:
    DebugFail();
    return false;
  }
}
//---------------------------------------------------------------------------
bool TOptionsIniFile::AllowSection(UnicodeString & Section)
{
  UnicodeString Name = Section;
  if (!Name.IsEmpty())
  {
    Name += PathDelim;
  }
  bool Result = SameText(Name.SubString(1, FRootKey.Length()), FRootKey);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TOptionsIniFile::FormatKey(UnicodeString & Section, UnicodeString & Ident)
{
  UnicodeString Result = Section;
  if (!Result.IsEmpty())
  {
    Result += PathDelim;
  }
  Result += Ident; // Can be empty, when called from a contructor, AllowSection or ReadSection
  if (DebugAlwaysTrue(AllowSection(Section)))
  {
    Result.Delete(1, FRootKey.Length());
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TOptionsIniFile::ReadString(UnicodeString Section, UnicodeString Ident, UnicodeString Default)
{
  UnicodeString Value;
  if (!AllowSection(Section))
  {
    Value = Default;
  }
  else
  {
    UnicodeString Name = FormatKey(Section, Ident);

    int Index = FOptions->IndexOfName(Name);
    if (Index >= 0)
    {
      Value = FOptions->ValueFromIndex[Index];
    }
    else
    {
      Value = Default;
    }
  }
  return Value;
}
//---------------------------------------------------------------------------
void TOptionsIniFile::WriteString(UnicodeString Section, UnicodeString Ident, UnicodeString Value)
{
  if (AllowWrite() &&
      DebugAlwaysTrue(AllowSection(Section)))
  {
    UnicodeString Name = FormatKey(Section, Ident);
    FOptions->Values[Name] = Value;
  }
}
//---------------------------------------------------------------------------
void TOptionsIniFile::ReadSection(UnicodeString Section, TStrings *Strings)
{

  if (AllowSection(Section))
  {
    UnicodeString SectionPrefix = FormatKey(Section, UnicodeString());

    Strings->BeginUpdate();
    try
    {
      for (int Index = 0; Index < FOptions->Count; Index++)
      {
        UnicodeString Name = FOptions->Names[Index];
        if (::SameText(Name.SubString(1, SectionPrefix.Length()), SectionPrefix) &&
          (LastDelimiter(PathDelim, Name) <= SectionPrefix.Length()))
        {
          Strings->Add(Name.SubString(SectionPrefix.Length() + 1, Name.Length() - SectionPrefix.Length()));
        }
      }
    }
    __finally
    {
      Strings->EndUpdate();
    }
  }
}
//---------------------------------------------------------------------------
void TOptionsIniFile::ReadSections(TStrings *Strings)
{
  std::unique_ptr<TStringList> Sections(CreateSortedStringList());

  for (int Index = 0; Index < FOptions->Count; Index++)
  {
    UnicodeString Name = FOptions->Names[Index];
    int P = LastDelimiter(PathDelim, Name);
    if (P > 0)
    {
      UnicodeString Section = Name.SubString(1, P - 1);
      if (Sections->IndexOf(Section) < 0)
      {
        Sections->Add(Section);
      }
    }
  }

  for (int Index = 0; Index < Sections->Count; Index++)
  {
    Strings->Add(FRootKey + Sections->Strings[Index]);
  }
}
//---------------------------------------------------------------------------
void TOptionsIniFile::ReadSectionValues(UnicodeString Section, TStrings * / *Strings * / )
{
  NotImplemented();
}
//---------------------------------------------------------------------------
void TOptionsIniFile::EraseSection(UnicodeString Section)
{
  if (AllowWrite())
  {
    NotImplemented();
  }
}
//---------------------------------------------------------------------------
void TOptionsIniFile::DeleteKey(UnicodeString Section, UnicodeString Ident)
{
  if (AllowWrite() &&
      DebugAlwaysTrue(AllowSection(Section)))
  {
    UnicodeString Name = FormatKey(Section, Ident);

    int Index = FOptions->IndexOfName(Name);
    if (Index >= 0)
    {
      FOptions->Delete(Index);
    }
  }
}
//---------------------------------------------------------------------------
void TOptionsIniFile::UpdateFile()
{
  if (AllowWrite())
  {
    // noop
  }
}
//---------------------------------------------------------------------------
void TOptionsIniFile::ReadSections(UnicodeString / *Section * /, TStrings * / * Strings * / )
{
  NotImplemented();
}
//===========================================================================
TOptionsStorage::TOptionsStorage(TStrings *Options, bool AllowWrite):
  TCustomIniFileStorage(
    UnicodeString(L"Command-line options"),
    new TOptionsIniFile(Options, (AllowWrite ? wmAllow : wmFail), UnicodeString()))
{
}
//---------------------------------------------------------------------------
TOptionsStorage::TOptionsStorage(TStrings *Options, UnicodeString RootKey, THierarchicalStorage *MasterStorage) :
  TCustomIniFileStorage(
    UnicodeString(L"Command-line options overriding " + MasterStorage->Source),
    new TOptionsIniFile(Options, wmIgnore, RootKey))
{
  FMasterStorage.reset(MasterStorage);
}
//---------------------------------------------------------------------------
bool TOptionsStorage::GetTemporary()
{
  return true;
}
#endif // #if 0
