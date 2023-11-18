
#include <vcl.h>
#pragma hdrstop

#include <rdestl/vector.h>
#include <Common.h>
#include <Exceptions.h>
#include <StrUtils.hpp>

#include "PuttyIntf.h"
#include "HierarchicalStorage.h"
#include <Interface.h>
#include <TextsCore.h>
#include <StrUtils.hpp>
#include <vector>

__removed #pragma package(smart_init)

// ValueExists test was probably added to avoid registry exceptions when debugging
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch(...) { return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch(...) { }

UnicodeString MungeStr(const UnicodeString & Str, bool ForceAnsi, bool Value)
{
  RawByteString Source;
  if (ForceAnsi)
  {
    Source = RawByteString(PuttyStr(Str));
  }
  else
  {
    Source = RawByteString(UTF8String(Str));
    if (Source.Length() > Str.Length())
    {
      Source.Insert(CONST_BOM, 1);
    }
  }
  strbuf * sb = strbuf_new();
  escape_registry_key(Source.c_str(), sb);
  RawByteString Dest(sb->s);
  strbuf_free(sb);
  if (Value)
  {
    // We do not want to munge * in PasswordMask
    Dest = ReplaceStr(Dest, L"%2A", L"*");
  }
  return UnicodeString(Dest.c_str(), Dest.Length());
}

UnicodeString UnMungeStr(const UnicodeString & Str)
{
  // Str should contain ASCII characters only
  RawByteString Source = AnsiString(Str);
  strbuf * sb = strbuf_new();
  unescape_registry_key(Source.c_str(), sb);
  RawByteString Dest(sb->s);
  strbuf_free(sb);
  UnicodeString Result;
  const AnsiString Bom(CONST_BOM);
  if (Dest.SubString(1, LENOF(CONST_BOM)) == Bom)
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

AnsiString PuttyStr(const UnicodeString & Str)
{
  return AnsiString(Str);
}

UnicodeString PuttyMungeStr(const UnicodeString & Str)
{
  return MungeStr(Str, true, false);
}

UnicodeString PuttyUnMungeStr(const UnicodeString & Str)
{
  return UnMungeStr(Str);
}

UnicodeString MungeIniName(const UnicodeString & Str)
{
  int32_t P = Str.Pos(L"=");
  // make this fast for now
  if (P > 0)
  {
    return ReplaceStr(Str, L"=", L"%3D");
  }
  else
  {
    return Str;
  }
}

UnicodeString UnMungeIniName(const UnicodeString & Str)
{
  int32_t P = Str.Pos(L"%3D");
  // make this fast for now
  if (P > 0)
  {
    return ReplaceStr(Str, L"%3D", L"=");
  }
  else
  {
    return Str;
  }
}

template<typename T>
void AddIntMapping(TIntMapping & Mapping, const wchar_t * Name, const T & Value)
{
  if (Name != nullptr)
  {
    Mapping[UnicodeString(Name)] = (int32_t)Value;
  }
}

template<typename T>
TIntMapping CreateIntMapping(
  const wchar_t * Name1, const T & Value1,
  const wchar_t * Name2 = nullptr, const T & Value2 = T(0),
  const wchar_t * Name3 = nullptr, const T & Value3 = T(0))
{
  TIntMapping Result;
  AddIntMapping(Result, Name1, Value1);
  AddIntMapping(Result, Name2, Value2);
  AddIntMapping(Result, Name3, Value3);
  return TIntMapping(Result.begin(), Result.end());
}

TIntMapping CreateIntMappingFromEnumNames(const UnicodeString & ANames)
{
  UnicodeString Names(ANames);
  TIntMapping Result;
  int32_t Index = 0;
  while (!Names.IsEmpty())
  {
    UnicodeString Name = CutToChar(Names, L';', true);
    Result[Name] = Index;
    Index++;
  }
  return TIntMapping(Result.begin(), Result.end());
}

TIntMapping AutoSwitchMapping = CreateIntMapping(L"on", asOn, L"off", asOff, L"auto", asAuto);
TIntMapping AutoSwitchReversedMapping = CreateIntMapping(L"on", asOff, L"off", asOn, L"auto", asAuto);
TIntMapping BoolMapping = CreateIntMapping(L"on", true, L"off", false);
//===========================================================================
UnicodeString AccessValueName(L"Access");
UnicodeString DefaultAccessString(L"inherit");

THierarchicalStorage::THierarchicalStorage(const UnicodeString & AStorage) noexcept :
  FStorage(AStorage)
{
  // FStorage = AStorage;
  SetAccessMode(smRead);
  SetExplicit(false);
  ForceSave = false;
  // While this was implemented in 5.0 already, for some reason
  // it was disabled (by mistake?). So although enabled for 5.6.1 only,
  // data written in Unicode/UTF8 can be read by all versions back to 5.0.
  SetForceAnsi(false);
  SetMungeStringValues(true);
  FFakeReadOnlyOpens = 0;
  FRootAccess = -1;
}

THierarchicalStorage::~THierarchicalStorage() noexcept
{
}

void THierarchicalStorage::Flush()
{
}

void THierarchicalStorage::SetAccessModeProtected(TStorageAccessMode Value)
{
  FAccessMode = Value;
}

UnicodeString THierarchicalStorage::GetCurrentSubKeyMunged() const
{
  if (!FKeyHistory.empty())
  {
    return FKeyHistory.back().Key;
  }
  else
  {
    return UnicodeString();
  }
}

UnicodeString THierarchicalStorage::GetCurrentSubKey() const
{
  return UnMungeStr(GetCurrentSubKeyMunged());
}

void THierarchicalStorage::ConfigureForPutty()
{
  MungeStringValues = false;
  ForceAnsi = true;
}

bool THierarchicalStorage::OpenRootKey(bool CanCreate)
{
  // This do not seem to be doing what it advertises.
  // It probably "works" only when used as a first "Open". So let's verify that by this assertion.
  DebugAssert(CurrentSubKey().IsEmpty());

  return OpenSubKey(UnicodeString(), CanCreate);
}

UnicodeString THierarchicalStorage::MungeKeyName(const UnicodeString & Key)
{
  UnicodeString Result = MungeStr(Key, GetForceAnsi(), false);
  // if there's already ANSI-munged subkey, keep ANSI munging
  if ((Result != Key) && !GetForceAnsi() && CanRead() && DoKeyExists(Key, true))
  {
    Result = MungeStr(Key, true, false);
  }
  return Result;
}

UnicodeString THierarchicalStorage::DoReadRootAccessString()
{
  UnicodeString Result;
  if (OpenRootKey(false))
  {
    Result = ReadAccessString();
    CloseSubKey();
  }
  else
  {
    Result = DefaultAccessString;
  }
  return Result;
}

UnicodeString THierarchicalStorage::ReadAccessString()
{
  UnicodeString Result;
  if (!FKeyHistory.empty())
  {
    Result = ReadString(AccessValueName, DefaultAccessString);
  }
  else
  {
    Result = DoReadRootAccessString();
  }
  return Result;
}

uint32_t THierarchicalStorage::ReadAccess(uint32_t CurrentAccess)
{
  UnicodeString Access = ReadAccessString();
  uint32_t Result = 0;
  while (!Access.IsEmpty())
  {
    UnicodeString Token = CutToChar(Access, L',', true);
    if (SameText(Token, L"inherit"))
    {
      Result |= CurrentAccess;
    }
    else if (SameText(Token, "read"))
    {
      Result |= hsaRead;
    }
    else if (SameText(Token, "full"))
    {
      Result |= hsaRead | hsaWrite;
    }
  }
  return Result;
}

uint32_t THierarchicalStorage::GetCurrentAccess()
{
  uint32_t Result;
  if (!FKeyHistory.empty())
  {
    // We must have resolved root access when opening the sub key the latest.
    DebugAssert(FRootAccess >= 0);
    Result = FKeyHistory.back().Access;
  }
  else
  {
    if (FRootAccess < 0)
    {
      FRootAccess = hsaRead; // Prevent recursion to allow reading the access
      FRootAccess = ReadAccess(hsaRead | hsaWrite);
    }
    Result = FRootAccess;
  }
  return Result;
}

bool THierarchicalStorage::OpenSubKeyPath(const UnicodeString & KeyPath, bool CanCreate)
{
  DebugAssert(!KeyPath.IsEmpty() && (KeyPath[KeyPath.Length()] != L'\\'));
  bool Result;
  UnicodeString Buf(KeyPath);
  int32_t Opens = 0;
  while (!Buf.IsEmpty())
  {
    UnicodeString SubKey = CutToChar(Buf, L'\\', false);
    Result = OpenSubKey(SubKey, CanCreate);
    if (Result)
    {
      Opens++;
    }
  }

  if (Result)
  {
    FKeyHistory.back().Levels = Opens;
  }
  else
  {
    while (Opens > 0)
    {
      CloseSubKey();
      Opens--;
    }
  }

  return Result;
}

bool THierarchicalStorage::OpenSubKey(const UnicodeString & ASubKey, bool CanCreate)
{
  UnicodeString MungedKey = MungeKeyName(ASubKey);

  bool Result;
  uint32_t InheritAccess;
  uint32_t Access;
  // For the first open, CanWrite > GetCurrentAccess > ReadAccess has a (needed) side effect of caching root access.
  if (!CanWrite() && CanCreate && !KeyExists(MungedKey))
  {
    InheritAccess = Access = 0; // do not even try to read the access, as the key is actually not opened
    FFakeReadOnlyOpens++;
    Result = true;
  }
  else
  {
    Access = hsaRead; // allow reading the access
    InheritAccess = GetCurrentAccess();
    Result = DoOpenSubKey(MungedKey, CanCreate);
  }

  if (Result)
  {
    TKeyEntry Entry;
    Entry.Key = IncludeTrailingBackslash(CurrentSubKey() + MungedKey);
    Entry.Levels = 1;
    Entry.Access = Access;
    FKeyHistory.push_back(Entry);
    // Read the real access only now, that the key is finally opened (it's in FKeyHistory)
    FKeyHistory.back().Access = ReadAccess(InheritAccess);
  }

  return Result;
}

void THierarchicalStorage::CloseSubKeyPath()
{
  if (FKeyHistory.empty())
  {
    throw Exception(UnicodeString());
  }

  int32_t Levels = FKeyHistory.back().Levels;
  FKeyHistory.back().Levels = 1; // to satify the assertion in CloseSubKey()
  while (Levels > 0)
  {
    CloseSubKey();
    Levels--;
    DebugAssert((Levels == 0) || (FKeyHistory.back().Levels == 1));
  }
}

void THierarchicalStorage::CloseSubKey()
{
  if (FKeyHistory.empty())
  {
    throw Exception(UnicodeString());
  }

  DebugAssert(FKeyHistory.back().Levels == 1);
  FKeyHistory.pop_back();
  if (FFakeReadOnlyOpens > 0)
  {
    FFakeReadOnlyOpens--;
  }
  else
  {
    DoCloseSubKey();
  }
}

void THierarchicalStorage::CloseAll()
{
  while (!GetCurrentSubKey().IsEmpty())
  {
    CloseSubKeyPath();
  }
}

void THierarchicalStorage::ClearSubKeys()
{
  std::unique_ptr<TStringList> SubKeys(std::make_unique<TStringList>());
  GetSubKeyNames(SubKeys.get());
  for (int32_t Index = 0; Index < SubKeys->Count(); Index++)
  {
    RecursiveDeleteSubKey(SubKeys->GetString(Index));
  }
}

void THierarchicalStorage::RecursiveDeleteSubKey(const UnicodeString & Key)
{
  bool CanWriteParent = CanWrite();
  if (OpenSubKey(Key, false))
  {
    ClearSubKeys();

    // Cannot delete the key itself, but can delete its contents, so at least delete the values
    // (which would otherwise be deleted implicitly by DoDeleteSubKey)
    if (!CanWriteParent && CanWrite())
    {
      ClearValues();
    }

    // Only if all subkeys were successfully deleted in ClearSubKeys
    bool Delete = CanWriteParent && !HasSubKeys();

    CloseSubKey();

    if (Delete)
    {
      DoDeleteSubKey(Key);
    }
  }
}

bool THierarchicalStorage::HasSubKeys()
{
  std::unique_ptr<TStrings> SubKeys(std::make_unique<TStringList>());
  GetSubKeyNames(SubKeys.get());
  bool Result = (SubKeys->Count() > 0);
  return Result;
}

bool THierarchicalStorage::DeleteValue(const UnicodeString & Name)
{
  if (CanWrite())
  {
    return DoDeleteValue(Name);
  }
  else
  {
    return false;
  }
}

bool THierarchicalStorage::KeyExists(const UnicodeString & SubKey)
{
  if (CanRead())
  {
    return DoKeyExists(SubKey, ForceAnsi);
  }
  else
  {
    return false;
  }
}

bool THierarchicalStorage::ValueExists(const UnicodeString & Value)
{
  if (CanRead())
  {
    return DoValueExists(Value);
  }
  else
  {
    return false;
  }
}

void THierarchicalStorage::ReadValues(TStrings * Strings, bool MaintainKeys)
{
  std::unique_ptr<TStrings> Names(std::make_unique<TStringList>());
  GetValueNames(Names.get());
  for (int32_t Index = 0; Index < Names->Count(); Index++)
  {
    if (MaintainKeys)
    {
      Strings->Add(FORMAT(L"%s=%s", Names->GetString(Index), ReadString(Names->GetString(Index), UnicodeString())));
    }
    else
    {
      Strings->Add(ReadString(Names->GetString(Index), UnicodeString()));
    }
  }
}

void THierarchicalStorage::ClearValues()
{
  std::unique_ptr<TStrings> Names(std::make_unique<TStringList>());
  GetValueNames(Names.get());
  for (int32_t Index = 0; Index < Names->Count(); Index++)
  {
    DeleteValue(Names->GetString(Index));
  }
}

void THierarchicalStorage::WriteValues(TStrings * Strings, bool MaintainKeys)
{
  ClearValues();

  if (Strings != nullptr)
  {
    for (int32_t Index = 0; Index < Strings->Count; Index++)
    {
      if (MaintainKeys)
      {
        DebugAssert(Strings->GetString(Index).Pos(L"=") > 1);
        WriteString(Strings->GetName(Index), Strings->GetValue(Strings->GetName(Index)));
      }
      else
      {
        WriteString(IntToStr(Index), Strings->GetString(Index));
      }
    }
  }
}

bool THierarchicalStorage::HasAccess(uint32_t Access)
{
  return
    FLAGSET(GetCurrentAccess(), Access) &&
    // There should never be any kind of access to a non-existent key
    DebugAlwaysTrue(FFakeReadOnlyOpens == 0);
}

bool THierarchicalStorage::CanRead()
{
  return HasAccess(hsaRead);
}

void THierarchicalStorage::GetSubKeyNames(TStrings * Strings)
{
  if (CanRead())
  {
    DoGetSubKeyNames(Strings);
  }
  else
  {
    Strings->Clear();
  }
}

void THierarchicalStorage::GetValueNames(TStrings * Strings)
{
  if (CanRead())
  {
    DoGetValueNames(Strings);

    int32_t Index = 0;
    while (Index < Strings->Count())
    {
      if (SameText(Strings->GetString(Index), AccessValueName))
      {
        Strings->Delete(Index);
      }
      else
      {
        Index++;
      }
    }
  }
  else
  {
    Strings->Clear();
  }
}

bool THierarchicalStorage::ReadBool(const UnicodeString & Name, bool Default)
{
  if (CanRead())
  {
    return DoReadBool(Name, Default);
  }
  else
  {
    return Default;
  }
}

int32_t THierarchicalStorage::ReadInteger(const UnicodeString & Name, int32_t Default)
{
  return ReadIntegerWithMapping(Name, Default, nullptr);
}

int32_t THierarchicalStorage::ReadIntegerWithMapping(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping)
{
  if (CanRead())
  {
    return DoReadInteger(Name, Default, Mapping);
  }
  else
  {
    return Default;
  }
}

int64_t THierarchicalStorage::ReadInt64(const UnicodeString & Name, int64_t Default)
{
  if (CanRead())
  {
    return DoReadInt64(Name, Default);
  }
  else
  {
    return Default;
  }
}

TDateTime THierarchicalStorage::ReadDateTime(const UnicodeString & Name, TDateTime Default)
{
  if (CanRead())
  {
    return DoReadDateTime(Name, Default);
  }
  else
  {
    return Default;
  }
}

double THierarchicalStorage::ReadFloat(const UnicodeString & Name, double Default)
{
  if (CanRead())
  {
    return DoReadFloat(Name, Default);
  }
  else
  {
    return Default;
  }
}

UnicodeString THierarchicalStorage::ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default)
{
  if (CanRead())
  {
    return DoReadStringRaw(Name, Default);
  }
  else
  {
    return Default;
  }
}

int32_t THierarchicalStorage::ReadBinaryData(const UnicodeString & Name, void * Buffer, int32_t Size)
{
  if (CanRead())
  {
    return DoReadBinaryData(Name, Buffer, Size);
  }
  else
  {
    return 0;
  }
}

UnicodeString THierarchicalStorage::ReadString(const UnicodeString & Name, const UnicodeString & Default)
{
  UnicodeString Result;
  if (CanRead())
  {
    if (MungeStringValues)
    {
      Result = UnMungeStr(ReadStringRaw(Name, MungeStr(Default, ForceAnsi, true)));
    }
    else
    {
      Result = ReadStringRaw(Name, Default);
    }
  }
  else
  {
    Result = Default;
  }
  return Result;
}

size_t THierarchicalStorage::BinaryDataSize(const UnicodeString & Name)
{
  if (CanRead())
  {
    return DoBinaryDataSize(Name);
  }
  else
  {
    return 0;
  }
}

RawByteString THierarchicalStorage::ReadBinaryData(const UnicodeString & Name)
{
  size_t Size = BinaryDataSize(Name);
  RawByteString Value;
  Value.SetLength(Size);
  ReadBinaryData(Name, nb::ToPtr(Value.c_str()), Size);
  return Value;
}

RawByteString THierarchicalStorage::ReadStringAsBinaryData(const UnicodeString & Name, const RawByteString & Default)
{
  UnicodeString UnicodeDefault = AnsiToString(Default);
  // This should be exactly the same operation as calling ReadString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  UnicodeString String = ReadString(Name, UnicodeDefault);
  AnsiString Ansi = AnsiString(String);
  RawByteString Result = RawByteString(Ansi.c_str(), Ansi.Length());
  return Result;
}

bool THierarchicalStorage::CanWrite()
{
  return HasAccess(hsaWrite);
}

void THierarchicalStorage::WriteBool(const UnicodeString & Name, bool Value)
{
  if (CanWrite())
  {
    DoWriteBool(Name, Value);
  }
}

void THierarchicalStorage::WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  if (CanWrite())
  {
    DoWriteStringRaw(Name, Value);
  }
}

void THierarchicalStorage::WriteInteger(const UnicodeString & Name, int32_t Value)
{
  if (CanWrite())
  {
    DoWriteInteger(Name, Value);
  }
}

void THierarchicalStorage::WriteInt64(const UnicodeString & Name, int64_t Value)
{
  if (CanWrite())
  {
    DoWriteInt64(Name, Value);
  }
}

void THierarchicalStorage::WriteDateTime(const UnicodeString & Name, TDateTime Value)
{
  if (CanWrite())
  {
    // TRegistry.WriteDateTime does this internally
    DoWriteBinaryData(Name, &Value, sizeof(Value));
  }
}

void THierarchicalStorage::WriteFloat(const UnicodeString & Name, double Value)
{
  if (CanWrite())
  {
    // TRegistry.WriteFloat does this internally
    DoWriteBinaryData(Name, &Value, sizeof(Value));
  }
}

void THierarchicalStorage::WriteString(const UnicodeString & Name, const UnicodeString & Value)
{
  if (MungeStringValues)
  {
    WriteStringRaw(Name, MungeStr(Value, ForceAnsi, true));
  }
  else
  {
    WriteStringRaw(Name, Value);
  }
}

void THierarchicalStorage::WriteBinaryData(const UnicodeString & Name, const void * Buffer, int32_t Size)
{
  if (CanWrite())
  {
    DoWriteBinaryData(Name, Buffer, Size);
  }
}

void THierarchicalStorage::WriteBinaryData(const UnicodeString & Name, const RawByteString & Value)
{
  WriteBinaryData(Name, Value.c_str(), Value.Length());
}

void THierarchicalStorage::WriteBinaryDataAsString(const UnicodeString & Name, const RawByteString & Value)
{
  // This should be exactly the same operation as calling WriteString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  WriteString(Name, AnsiToString(Value));
}

UnicodeString THierarchicalStorage::IncludeTrailingBackslash(const UnicodeString & S)
{
  // expanded from ?: as it caused memory leaks
  if (S.IsEmpty())
  {
    return S;
  }
  else
  {
    return ::IncludeTrailingBackslash(S);
  }
}

UnicodeString THierarchicalStorage::ExcludeTrailingBackslash(const UnicodeString & S)
{
  // expanded from ?: as it caused memory leaks
  if (S.IsEmpty())
  {
    return S;
  }
  else
  {
    return ::ExcludeTrailingBackslash(S);
  }
}

bool THierarchicalStorage::GetTemporary() const
{
  return false;
}
//===========================================================================
TRegistryStorage::TRegistryStorage(const UnicodeString & AStorage) noexcept:
  THierarchicalStorage(IncludeTrailingBackslash(AStorage))
{
  FWowMode = 0;
  Init();
}

TRegistryStorage::TRegistryStorage(const UnicodeString & AStorage, HKEY ARootKey, REGSAM WowMode) noexcept :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage))
{
  FWowMode = WowMode;
  Init();
  FRegistry->RootKey = ARootKey;
}

void TRegistryStorage::Init()
{
  FRegistry = std::make_unique<TRegistry>();
  FRegistry->Access = KEY_READ | FWowMode;
}

TRegistryStorage::~TRegistryStorage() noexcept
{
  __removed SAFE_DESTROY(FRegistry);
}

// Used only in OpenSessionInPutty
bool TRegistryStorage::Copy(TRegistryStorage * Storage)
{
  TRegistry * Registry = Storage->FRegistry.get();
  bool Result = true;
  std::unique_ptr<TStrings> Names(std::make_unique<TStringList>());
  Registry->GetValueNames(Names.get());
  rde::vector<uint8_t> Buffer(1024);
  int32_t Index = 0;
  while ((Index < Names->Count()) && Result)
  {
    UnicodeString Name = MungeStr(Names->GetString(Index), ForceAnsi, false);
    DWORD Size = nb::ToUInt32(Buffer.size());
    DWORD Type;
    int32_t RegResult;
    do
    {
      RegResult = RegQueryValueEx(Registry->GetCurrentKey(), Name.c_str(), nullptr, &Type, &Buffer[0], &Size);
      if (RegResult == ERROR_MORE_DATA)
      {
        Buffer.resize(Size);
      }
    } while (RegResult == ERROR_MORE_DATA);

    Result = (RegResult == ERROR_SUCCESS);
    if (Result)
    {
      RegResult = RegSetValueEx(FRegistry->GetCurrentKey(), Name.c_str(), 0, Type, &Buffer[0], Size);
      Result = (RegResult == ERROR_SUCCESS);
    }

    ++Index;
  }
  return Result;
}

UnicodeString TRegistryStorage::GetSource() const
{
  return RootKeyToStr(FRegistry->RootKey) + L"\\" + Storage;
}

void TRegistryStorage::SetAccessModeProtected(TStorageAccessMode value)
{
  THierarchicalStorage::SetAccessMode(value);
  if (FRegistry)
  {
    switch (AccessMode) {
      case smRead:
        FRegistry->Access = KEY_READ | FWowMode;
        break;

      case smReadWrite:
      default:
        FRegistry->Access = KEY_READ | KEY_WRITE | FWowMode;
        break;
    }
  }
}

bool TRegistryStorage::DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate)
{
  UnicodeString PrevPath;
  bool WasOpened = (FRegistry->GetCurrentKey() != nullptr);
  if (WasOpened)
  {
    PrevPath = FRegistry->GetCurrentPath();
    DebugAssert(SamePaths(PrevPath, Storage() + GetCurrentSubKeyMunged()));
    FRegistry->CloseKey();
  }
  const UnicodeString K = ExcludeTrailingBackslash(Storage() + CurrentSubKey() + SubKey);
  bool Result = FRegistry->OpenKey(K, CanCreate);
  if (!Result && WasOpened)
  {
    FRegistry->OpenKey(PrevPath, false);
  }
  return Result;
}

void TRegistryStorage::DoCloseSubKey()
{
  FRegistry->CloseKey();
  if (!FKeyHistory.empty())
  {
    FRegistry->OpenKey(GetStorage() + GetCurrentSubKeyMunged(), True);
  }
}

void TRegistryStorage::DoDeleteSubKey(const UnicodeString & SubKey)
{
  UnicodeString K;
  if (FKeyHistory.empty())
  {
    K = Storage() + CurrentSubKey;
  }
  K += MungeKeyName(SubKey);
  FRegistry->DeleteKey(K);
}

void TRegistryStorage::DoGetSubKeyNames(TStrings * Strings)
{
  FRegistry->GetKeyNames(Strings);
  for (int32_t Index = 0; Index < Strings->GetCount(); ++Index)
  {
    Strings->SetString(Index, UnMungeStr(Strings->GetString(Index)));
  }
}

void TRegistryStorage::DoGetValueNames(TStrings * Strings)
{
  FRegistry->GetValueNames(Strings);
}

bool TRegistryStorage::DoDeleteValue(const UnicodeString & Name)
{
  return FRegistry->DeleteValue(Name);
}

bool TRegistryStorage::DoKeyExists(const UnicodeString & SubKey, bool AForceAnsi)
{
  const UnicodeString Key = MungeStr(SubKey, AForceAnsi, false);
  bool Result = FRegistry->KeyExists(Key);
  return Result;
}

bool TRegistryStorage::DoValueExists(const UnicodeString & Value)
{
  const bool Result = FRegistry->ValueExists(Value);
  return Result;
}

size_t TRegistryStorage::DoBinaryDataSize(const UnicodeString & Name)
{
  size_t Result = FRegistry->GetDataSize(Name);
  return Result;
}

bool TRegistryStorage::DoReadBool(const UnicodeString & Name, bool Default)
{
  READ_REGISTRY(ReadBool);
}

TDateTime TRegistryStorage::DoReadDateTime(const UnicodeString & Name, TDateTime Default)
{
  // Internally does what would DoReadBinaryData do (like in DoReadInt64)
  READ_REGISTRY(ReadDateTime);
}

double TRegistryStorage::DoReadFloat(const UnicodeString & Name, double Default)
{
  // Internally does what would DoReadBinaryData do (like in DoReadInt64)
  READ_REGISTRY(ReadFloat);
}

int32_t TRegistryStorage::DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping *)
{
  READ_REGISTRY(ReadInteger);
}

int64_t TRegistryStorage::DoReadInt64(const UnicodeString & Name, int64_t Default)
{
  int64_t Result;
  if (DoReadBinaryData(Name, &Result, sizeof(Result)) == 0)
  {
    Result = Default;
  }
  return Result;
}

UnicodeString TRegistryStorage::DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default)
{
  READ_REGISTRY(ReadString);
}

int32_t TRegistryStorage::DoReadBinaryData(const UnicodeString & Name, void * Buffer, int32_t Size)
{
  int32_t Result;
  if (FRegistry->ValueExists(Name))
  {
    try
    {
      Result = FRegistry->ReadBinaryData(Name, Buffer, Size);
    }
    catch(...)
    {
      Result = 0;
    }
  }
  else
  {
    Result = 0;
  }
  return Result;
}

void TRegistryStorage::DoWriteBool(const UnicodeString & Name, bool Value)
{
  WRITE_REGISTRY(WriteBool);
}

void TRegistryStorage::DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  WRITE_REGISTRY(WriteString);
}

void TRegistryStorage::DoWriteInteger(const UnicodeString & Name, int32_t Value)
{
  WRITE_REGISTRY(WriteInteger);
}

void TRegistryStorage::DoWriteInt64(const UnicodeString & Name, int64_t Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}

void TRegistryStorage::DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, int32_t Size)
{
  try
  {
    FRegistry->WriteBinaryData(Name, const_cast<void *>(Buffer), Size);
  }
  catch(...)
  {
  }
}
#if 0
//===========================================================================
TCustomIniFileStorage::TCustomIniFileStorage(const UnicodeString & Storage, TCustomIniFile * IniFile) :
  THierarchicalStorage(Storage),
  FIniFile(IniFile),
  FMasterStorageOpenFailures(0),
  FOpeningSubKey(false)
{
}

TCustomIniFileStorage::~TCustomIniFileStorage()
{
  delete FIniFile;
}

UnicodeString TCustomIniFileStorage::GetSource()
{
  return Storage;
}

UnicodeString TCustomIniFileStorage::GetCurrentSection()
{
  return ExcludeTrailingBackslash(GetCurrentSubKeyMunged());
}

void TCustomIniFileStorage::CacheSections()
{
  if (FSections.get() == nullptr)
  {
    FSections.reset(std::make_unique<TStringList>());
    FIniFile->ReadSections(FSections.get());
    FSections->Sorted = true; // has to set only after reading as ReadSections reset it to false
  }
}

void TCustomIniFileStorage::ResetCache()
{
  FSections.reset(nullptr);
}

void TCustomIniFileStorage::SetAccessMode(TStorageAccessMode value)
{
  if (FMasterStorage.get() != nullptr)
  {
    FMasterStorage->AccessMode = value;
  }
  THierarchicalStorage::SetAccessMode(value);
}

bool TCustomIniFileStorage::DoKeyExistsInternal(const UnicodeString & SubKey)
{
  CacheSections();
  bool Result = false;
  UnicodeString NewKey = ExcludeTrailingBackslash(CurrentSubKey + SubKey);
  if (FSections->Count > 0)
  {
    int32_t Index = -1;
    Result = FSections->Find(NewKey, Index);
    if (!Result &&
        (Index < FSections->Count) &&
        (FSections->Strings[Index].SubString(1, NewKey.Length()+1) == NewKey + L"\\"))
    {
      Result = true;
    }
  }
  return Result;
}

bool TCustomIniFileStorage::DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate)
{
  bool Result =
    CanCreate ||
    DoKeyExistsInternal(SubKey);
  return Result;
}

bool TCustomIniFileStorage::OpenSubKey(const UnicodeString & Key, bool CanCreate)
{
  bool Result;

  // To cache root access in advance, otherwise we end up calling outselves, what TAutoFlag does not like
  GetCurrentAccess();

  {
    TAutoFlag Flag(FOpeningSubKey);
    Result = THierarchicalStorage::OpenSubKey(Key, CanCreate);
  }

  if (FMasterStorage.get() != nullptr)
  {
    if (FMasterStorageOpenFailures > 0)
    {
      if (Result)
      {
        FMasterStorageOpenFailures++;
      }
    }
    else
    {
      bool MasterResult = FMasterStorage->OpenSubKey(Key, CanCreate);
      if (!Result && MasterResult)
      {
        Result = THierarchicalStorage::OpenSubKey(Key, true);
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

void TCustomIniFileStorage::DoCloseSubKey()
{
  // noop
}

void TCustomIniFileStorage::CloseSubKey()
{
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

  THierarchicalStorage::CloseSubKey();
}

void TCustomIniFileStorage::DoDeleteSubKey(const UnicodeString & SubKey)
{
  try
  {
    ResetCache();
    FIniFile->EraseSection(CurrentSubKey + MungeKeyName(SubKey));
  }
  catch (...)
  {
  }
  if (HandleByMasterStorage())
  {
    if (FMasterStorage->CanWrite())
    {
      FMasterStorage->DoDeleteSubKey(SubKey);
    }
  }
}

void TCustomIniFileStorage::DoGetSubKeyNames(TStrings * Strings)
{
  Strings->Clear();
  if (HandleByMasterStorage())
  {
    FMasterStorage->GetSubKeyNames(Strings);
  }
  CacheSections();
  for (int32_t i = 0; i < FSections->Count; i++)
  {
    UnicodeString Section = FSections->Strings[i];
    if (AnsiCompareText(CurrentSubKey,
        Section.SubString(1, CurrentSubKey.Length())) == 0)
    {
      UnicodeString SubSection = Section.SubString(CurrentSubKey.Length() + 1,
        Section.Length() - CurrentSubKey.Length());
      int32_t P = SubSection.Pos(L"\\");
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

void TCustomIniFileStorage::DoGetValueNames(TStrings * Strings)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->GetValueNames(Strings);
  }
  FIniFile->ReadSection(CurrentSection, Strings);
  for (int32_t Index = 0; Index < Strings->Count; Index++)
  {
    Strings->Strings[Index] = UnMungeIniName(Strings->Strings[Index]);
  }
}

bool TCustomIniFileStorage::DoKeyExists(const UnicodeString & SubKey, bool AForceAnsi)
{
  return
    (HandleByMasterStorage() && FMasterStorage->DoKeyExists(SubKey, AForceAnsi)) ||
    DoKeyExistsInternal(MungeStr(SubKey, AForceAnsi, false));
}

bool TCustomIniFileStorage::DoValueExistsInternal(const UnicodeString & Value)
{
  return FIniFile->ValueExists(CurrentSection, MungeIniName(Value));
}

bool TCustomIniFileStorage::DoValueExists(const UnicodeString & Value)
{
  return
    (HandleByMasterStorage() && FMasterStorage->ValueExists(Value)) ||
    DoValueExistsInternal(Value);
}

bool TCustomIniFileStorage::DoDeleteValue(const UnicodeString & Name)
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

bool TCustomIniFileStorage::HandleByMasterStorage()
{
  return
    (FMasterStorage.get() != nullptr) &&
    (FMasterStorageOpenFailures == 0);
}

bool TCustomIniFileStorage::HandleReadByMasterStorage(const UnicodeString & Name)
{
  return HandleByMasterStorage() && !DoValueExistsInternal(Name);
}

size_t TCustomIniFileStorage::DoBinaryDataSize(const UnicodeString & Name)
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

bool TCustomIniFileStorage::DoReadBool(const UnicodeString & Name, bool Default)
{
  if (HandleReadByMasterStorage(Name))
  {
    return FMasterStorage->ReadBool(Name, Default);
  }
  else
  {
    int32_t IntDefault = nb::ToInt32(Default);
    int32_t Int = DoReadIntegerWithMapping(Name, IntDefault, &BoolMapping);
    return (Int != 0);
  }
}

int32_t TCustomIniFileStorage::DoReadIntegerWithMapping(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping)
{
  int32_t Result;
  bool ReadAsInteger = true;
  UnicodeString MungedName = MungeIniName(Name);
  if (Mapping != nullptr) // optimization
  {
    UnicodeString S = FIniFile->ReadString(CurrentSection, MungedName, EmptyStr);
    if (!S.IsEmpty())
    {
      S = S.LowerCase();
      TIntMapping::const_iterator I = Mapping->find(S);
      if (I != Mapping->end())
      {
        Result = I->second;
        ReadAsInteger = false;
      }
    }
  }

  if (ReadAsInteger)
  {
    Result = FIniFile->ReadInteger(CurrentSection, MungedName, Default);
  }
  return Result;
}

int32_t TCustomIniFileStorage::DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping)
{
  int32_t Result;
  if (HandleReadByMasterStorage(Name))
  {
    Result = FMasterStorage->ReadIntegerWithMapping(Name, Default, Mapping);
  }
  else
  {
    Result = DoReadIntegerWithMapping(Name, Default, Mapping);
  }
  return Result;
}

int64_t TCustomIniFileStorage::DoReadInt64(const UnicodeString & Name, int64_t Default)
{
  int64_t Result;
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

TDateTime TCustomIniFileStorage::DoReadDateTime(const UnicodeString & Name, TDateTime Default)
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
        if (static_cast<size_t>(Raw.Length()) == sizeof(Result))
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

double TCustomIniFileStorage::DoReadFloat(const UnicodeString & Name, double Default)
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
        if (static_cast<size_t>(Raw.Length()) == sizeof(Result))
        {
          memcpy(&Result, Raw.c_str(), sizeof(Result));
        }
        else
        {
          Result = static_cast<double>(StrToFloat(Value));
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

UnicodeString TCustomIniFileStorage::DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default)
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

size_t TCustomIniFileStorage::DoReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size)
{
  size_t Len;
  if (HandleReadByMasterStorage(Name))
  {
    Size = FMasterStorage->ReadBinaryData(Name, Buffer, Size);
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

void TCustomIniFileStorage::DoWriteBool(const UnicodeString & Name, bool Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteBool(Name, Value);
  }
  ResetCache();
  FIniFile->WriteBool(CurrentSection, MungeIniName(Name), Value);
}

void TCustomIniFileStorage::DoWriteInteger(const UnicodeString & Name, int32_t Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteInteger(Name, Value);
  }
  ResetCache();
  FIniFile->WriteInteger(CurrentSection, MungeIniName(Name), Value);
}

void TCustomIniFileStorage::DoWriteInt64(const UnicodeString & Name, int64_t Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteInt64(Name, Value);
  }
  DoWriteStringRawInternal(Name, IntToStr(Value));
}

void TCustomIniFileStorage::DoWriteStringRawInternal(const UnicodeString & Name, const UnicodeString & Value)
{
  ResetCache();
  FIniFile->WriteString(CurrentSection, MungeIniName(Name), Value);
}

void TCustomIniFileStorage::DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteStringRaw(Name, Value);
  }
  DoWriteStringRawInternal(Name, Value);
}

void TCustomIniFileStorage::DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, int32_t Size)
{
  if (HandleByMasterStorage())
  {
    FMasterStorage->WriteBinaryData(Name, Buffer, Size);
  }
  DoWriteStringRawInternal(Name, BytesToHex(RawByteString(static_cast<const char*>(Buffer), Size)));
}

UnicodeString TCustomIniFileStorage::DoReadRootAccessString()
{
  UnicodeString Result;
  if (OpenSubKey(L"_", false))
  {
    Result = ReadAccessString();
    CloseSubKey();
  }
  else
  {
    Result = DefaultAccessString;
  }
  return Result;
}

uint32_t TCustomIniFileStorage::GetCurrentAccess()
{
  uint32_t Result;
  // The way THierarchicalStorage::OpenSubKey is implemented, the access will be zero for non-existing keys in
  // configuration overrides => delegating access handling to the master storage (which still should read overriden access keys)
  if (FMasterStorage.get() != nullptr)
  {
    Result = FMasterStorage->GetCurrentAccess();
  }
  else
  {
    Result = THierarchicalStorage::GetCurrentAccess();
  }
  return Result;
}

bool TCustomIniFileStorage::HasAccess(uint32_t Access)
{
  bool Result;
  // Avoid the FFakeReadOnlyOpens assertion in THierarchicalStorage::HasAccess, which is not valid for overriden configuration
  if (HandleByMasterStorage())
  {
    Result = FMasterStorage->HasAccess(Access);
  }
  else
  {
    Result = THierarchicalStorage::HasAccess(Access);
  }
  return Result;
}
//===========================================================================
TIniFileStorage * TIniFileStorage::CreateFromPath(const UnicodeString & AStorage)
{
  // The code was originally inline in the parent contructor call in the TIniFileStorage::TIniFileStorage [public originally].
  // But if the TMemIniFile constructor throws (e.g. because the INI file is locked), the exception causes access violation.
  // Moving the code to a factory solves this.
  TMemIniFile * IniFile = new TMemIniFile(AStorage);
  return new TIniFileStorage(AStorage, IniFile);
}

TIniFileStorage * TIniFileStorage::CreateNul()
{
  // Before we passed "nul", but we have report of a system, where opening "nul" file fails.
  // Passing an empty string is even more efficient, as it does not even try to read the file.
  TMemIniFile * IniFile = new TMemIniFile(EmptyStr);
  return new TIniFileStorage(INI_NUL, IniFile);
}

TIniFileStorage::TIniFileStorage(const UnicodeString & AStorage, TCustomIniFile * IniFile):
  TCustomIniFileStorage(AStorage, IniFile)
{
  if (!FIniFile->FileName.IsEmpty())
  {
    FOriginal = new TStringList();
    dynamic_cast<TMemIniFile *>(FIniFile)->GetStrings(FOriginal);
  }
  else
  {
    FOriginal = nullptr;
  }
  ApplyOverrides();
}

void TIniFileStorage::Flush()
{
  if (FMasterStorage.get() != nullptr)
  {
    FMasterStorage->Flush();
  }
  // This does not seem correct, if storage is written after it is flushed (though we probably never do that currently)
  if ((FOriginal != nullptr) && !FIniFile->FileName.IsEmpty())
  {
    std::unique_ptr<TStrings> Strings(std::make_unique<TStringList>());
    std::unique_ptr<TStrings> Original(FOriginal);
    FOriginal = nullptr;

    dynamic_cast<TMemIniFile *>(FIniFile)->GetStrings(Strings.get());
    if (!Strings->Equals(Original.get()))
    {
      int32_t Attr;
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

      HANDLE Handle;
      int32_t Error;
      bool Retry;
      int32_t Trying = 0;
      do
      {
        Error = 0;
        Handle =
          CreateFile(ApiPath(Storage).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, Attr, 0);
        if (Handle == INVALID_HANDLE_VALUE)
        {
          Error = GetLastError();
        }
        Retry = (Error == ERROR_SHARING_VIOLATION) && (Trying < 2000);
        if (Retry)
        {
          const int32_t Step = 100;
          Sleep(Step);
          Trying += Step;
        }
      }
      while (Retry);

      if (Handle == INVALID_HANDLE_VALUE)
      {
        // "access denied" errors upon implicit saves to existing file are ignored
        if (Explicit || !Exists || (Error != ERROR_ACCESS_DENIED))
        {
          throw EOSExtException(FMTLOAD((Exists ? WRITE_ERROR : CREATE_FILE_ERROR), Storage));
        }
      }
      else
      {
        try
        {
          std::unique_ptr<TStream> Stream(new TSafeHandleStream(nb::ToInt32(Handle)));
          try
          {
            Strings->SaveToStream(Stream.get());
          }
          __finally
          {
            CloseHandle(Handle);
          }
        }
        catch (Exception & E)
        {
          throw ExtException(&E, FMTLOAD(WRITE_ERROR, Storage));
        }
      }
    }
  }
}

TIniFileStorage::~TIniFileStorage()
{
  Flush();
}

void TIniFileStorage::ApplyOverrides()
{
  UnicodeString OverridesKey = IncludeTrailingBackslash(L"Override");

  CacheSections();
  for (int32_t i = 0; i < FSections->Count; i++)
  {
    UnicodeString Section = FSections->Strings[i];

    if (SameText(OverridesKey,
          Section.SubString(1, OverridesKey.Length())))
    {
      UnicodeString SubKey = Section.SubString(OverridesKey.Length() + 1,
        Section.Length() - OverridesKey.Length());

      // this all uses raw names (munged)
      TStrings * Names = new TStringList;
      try
      {
        FIniFile->ReadSection(Section, Names);

        for (int32_t ii = 0; ii < Names->Count; ii++)
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

class TOptionsIniFile : public TCustomIniFile
{
public:
  TOptionsIniFile(TStrings * Options, TWriteMode WriteMode, const UnicodeString & RootKey);

  virtual UnicodeString ReadString(const UnicodeString & Section, const UnicodeString & Ident, const UnicodeString & Default);
  virtual void WriteString(const UnicodeString & Section, const UnicodeString & Ident, const UnicodeString & Value);
  virtual void ReadSection(const UnicodeString & Section, TStrings * Strings);
  virtual void ReadSections(TStrings * Strings);
  virtual void ReadSectionValues(const UnicodeString & Section, TStrings * Strings);
  virtual void EraseSection(const UnicodeString & Section);
  virtual void DeleteKey(const UnicodeString & Section, const UnicodeString & Ident);
  virtual void UpdateFile();
  // Hoisted overload
  void ReadSections(const UnicodeString & Section, TStrings * Strings);
  // Ntb, we can implement ValueExists more efficiently than the TCustomIniFile.ValueExists

private:
  TStrings * FOptions;
  TWriteMode FWriteMode;
  UnicodeString FRootKey;

  bool AllowWrite();
  bool AllowSection(const UnicodeString & Section);
  UnicodeString FormatKey(const UnicodeString & Section, const UnicodeString & Ident);
};

TOptionsIniFile::TOptionsIniFile(TStrings * Options, TWriteMode WriteMode, const UnicodeString & RootKey) :
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

bool TOptionsIniFile::AllowSection(const UnicodeString & Section)
{
  UnicodeString Name = Section;
  if (!Name.IsEmpty())
  {
    Name += PathDelim;
  }
  bool Result = SameText(Name.SubString(1, FRootKey.Length()), FRootKey);
  return Result;
}

UnicodeString TOptionsIniFile::FormatKey(const UnicodeString & Section, const UnicodeString & Ident)
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

UnicodeString TOptionsIniFile::ReadString(const UnicodeString & Section, const UnicodeString & Ident, const UnicodeString & Default)
{
  UnicodeString Value;
  if (!AllowSection(Section))
  {
    Value = Default;
  }
  else
  {
    UnicodeString Name = FormatKey(Section, Ident);

    int32_t Index = FOptions->IndexOfName(Name);
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

void TOptionsIniFile::WriteString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Value)
{
  if (AllowWrite() &&
      DebugAlwaysTrue(AllowSection(Section)))
  {
    UnicodeString Name = FormatKey(Section, Ident);
    SetStringValueEvenIfEmpty(FOptions, Name, Value);
  }
}

void TOptionsIniFile::ReadSection(const UnicodeString Section, TStrings * Strings)
{

  if (AllowSection(Section))
  {
    UnicodeString SectionPrefix = FormatKey(Section, UnicodeString());

    Strings->BeginUpdate();
    try
    {
      for (int32_t Index = 0; Index < FOptions->Count; Index++)
      {
        UnicodeString Name = FOptions->Names[Index];
        if (SameText(Name.SubString(1, SectionPrefix.Length()), SectionPrefix) &&
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

void TOptionsIniFile::ReadSections(TStrings * Strings)
{
  std::unique_ptr<TStringList> Sections(CreateSortedStringList());

  for (int32_t Index = 0; Index < FOptions->Count; Index++)
  {
    UnicodeString Name = FOptions->Names[Index];
    int32_t P = LastDelimiter(PathDelim, Name);
    if (P > 0)
    {
      UnicodeString Section = Name.SubString(1, P - 1);
      if (Sections->IndexOf(Section) < 0)
      {
        Sections->Add(Section);
      }
    }
  }

  for (int32_t Index = 0; Index < Sections->Count; Index++)
  {
    Strings->Add(FRootKey + Sections->Strings[Index]);
  }
}

void TOptionsIniFile::ReadSectionValues(const UnicodeString Section, TStrings * /*Strings*/)
{
  NotImplemented();
}

void TOptionsIniFile::EraseSection(const UnicodeString Section)
{
  if (AllowWrite())
  {
    NotImplemented();
  }
}

void TOptionsIniFile::DeleteKey(const UnicodeString Section, const UnicodeString Ident)
{
  if (AllowWrite() &&
      DebugAlwaysTrue(AllowSection(Section)))
  {
    UnicodeString Name = FormatKey(Section, Ident);

    int32_t Index = FOptions->IndexOfName(Name);
    if (Index >= 0)
    {
      FOptions->Delete(Index);
    }
  }
}

void TOptionsIniFile::UpdateFile()
{
  if (AllowWrite())
  {
    // noop
  }
}

void TOptionsIniFile::ReadSections(const UnicodeString & /*Section*/, TStrings * /*Strings*/)
{
  NotImplemented();
}
//===========================================================================
TOptionsStorage::TOptionsStorage(TStrings * Options, bool AllowWrite):
  TCustomIniFileStorage(
    UnicodeString(L"Command-line options"),
    new TOptionsIniFile(Options, (AllowWrite ? wmAllow : wmFail), UnicodeString()))
{
}

TOptionsStorage::TOptionsStorage(TStrings * Options, const UnicodeString & RootKey, THierarchicalStorage * MasterStorage) :
  TCustomIniFileStorage(
    UnicodeString(L"Command-line options overriding " + MasterStorage->Source),
    new TOptionsIniFile(Options, wmIgnore, RootKey))
{
  FMasterStorage.reset(MasterStorage);
}

bool TOptionsStorage::GetTemporary()
{
  return true;
}

#endif // #if 0
