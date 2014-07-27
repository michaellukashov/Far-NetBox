//------------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "HierarchicalStorage.h"
#include <TextsCore.h>
#include <StrUtils.hpp>

using namespace Sysutils;
//------------------------------------------------------------------------------
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch (...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch (...) { FFailed++; }
//------------------------------------------------------------------------------
UnicodeString MungeStr(const UnicodeString & Str, bool ForceAnsi)
{
  RawByteString Source;
  if (ForceAnsi)
  {
    Source = AnsiString(Str);
  }
  else
  {
    Source = UTF8String(Str);
    if (Source.Length() > Str.Length())
    {
      Source.Insert(Bom.c_str(), 1);
    }
  }
  // should contain ASCII characters only
  RawByteString Dest;
  Dest.SetLength(Source.Length() * 3 + 1);
  putty_mungestr(Source.c_str(), const_cast<char *>(Dest.c_str()));
  PackStr(Dest);
  return UnicodeString(Dest.c_str(), Dest.Length());
}
//------------------------------------------------------------------------------
UnicodeString UnMungeStr(const UnicodeString & Str)
{
  // Str should contain ASCII characters only
  RawByteString Source = Str;
  RawByteString Dest;
  Dest.SetLength(Source.Length() + 1);
  putty_unmungestr(Source.c_str(), const_cast<char *>(Dest.c_str()), static_cast<int>(Dest.Length()));
  UnicodeString Result;
  if (Dest.Pos(Bom.c_str()) == 1)
  {
    Dest.Delete(1, Bom.size());
    Result = UTF8String(Dest.c_str());
  }
  else
  {
    Result = AnsiString(Dest.c_str());
  }
  return Result;
}
//------------------------------------------------------------------------------
UnicodeString PuttyMungeStr(const UnicodeString & Str)
{
  return MungeStr(Str, false);
}
//------------------------------------------------------------------------------
UnicodeString PuttyUnMungeStr(const UnicodeString & Str)
{
  return UnMungeStr(Str);
}
//------------------------------------------------------------------------------
UnicodeString MungeIniName(const UnicodeString & Str)
{
  intptr_t P = Str.Pos(L"=");
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
//------------------------------------------------------------------------------
UnicodeString UnMungeIniName(const UnicodeString & Str)
{
  intptr_t P = Str.Pos(L"%3D");
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
//===========================================================================
THierarchicalStorage::THierarchicalStorage(const UnicodeString & AStorage) :
  FStorage(AStorage),
  FKeyHistory(new TStringList())
{
  SetAccessMode(smRead);
  SetExplicit(false);
  SetForceAnsi(true);
  SetMungeStringValues(true);
}
//------------------------------------------------------------------------------
THierarchicalStorage::~THierarchicalStorage()
{
  SAFE_DESTROY(FKeyHistory);
}
//------------------------------------------------------------------------------
void THierarchicalStorage::Flush()
{
}
//------------------------------------------------------------------------------
void THierarchicalStorage::SetAccessMode(TStorageAccessMode Value)
{
  FAccessMode = Value;
}
//------------------------------------------------------------------------------
UnicodeString THierarchicalStorage::GetCurrentSubKeyMunged() const
{
  if (FKeyHistory->GetCount())
  {
    return FKeyHistory->GetString(FKeyHistory->GetCount() - 1);
  }
  else
  {
    return L"";
  }
}
//------------------------------------------------------------------------------
UnicodeString THierarchicalStorage::GetCurrentSubKey() const
{
  return UnMungeStr(GetCurrentSubKeyMunged());
}
//------------------------------------------------------------------------------
bool THierarchicalStorage::OpenRootKey(bool CanCreate)
{
  return OpenSubKey(L"", CanCreate);
}
//------------------------------------------------------------------------------
UnicodeString THierarchicalStorage::MungeKeyName(const UnicodeString & Key)
{
  UnicodeString Result = MungeStr(Key, GetForceAnsi());
  // if there's already ANSI-munged subkey, keep ANSI munging
  if ((Result != Key) && !GetForceAnsi() && DoKeyExists(Key, true))
  {
    Result = MungeStr(Key, true);
  }
  return Result;
}
//------------------------------------------------------------------------------
bool THierarchicalStorage::OpenSubKey(const UnicodeString & SubKey, bool CanCreate, bool Path)
{
  bool Result;
  UnicodeString MungedKey;
  UnicodeString SubKey2 = SubKey;
  if (Path)
  {
    assert(SubKey2.IsEmpty() || (SubKey2[SubKey2.Length()] != L'\\'));
    Result = true;
    while (!SubKey2.IsEmpty() && Result)
    {
      if (!MungedKey.IsEmpty())
      {
        MungedKey += L'\\';
      }
      MungedKey += MungeKeyName(CutToChar(SubKey2, L'\\', false));
      Result = DoOpenSubKey(MungedKey, CanCreate);
    }

    // hack to restore last opened key for registry storage
    if (!Result)
    {
      FKeyHistory->Add(Sysutils::IncludeTrailingBackslash(GetCurrentSubKey() + MungedKey));
      CloseSubKey();
    }
  }
  else
  {
    MungedKey = MungeKeyName(SubKey2);
    Result = DoOpenSubKey(MungedKey, CanCreate);
  }

  if (Result)
  {
    FKeyHistory->Add(Sysutils::IncludeTrailingBackslash(GetCurrentSubKey() + MungedKey));
  }

  return Result;
}
//------------------------------------------------------------------------------
void THierarchicalStorage::CloseSubKey()
{
  if (FKeyHistory->GetCount() == 0)
    throw Exception(L"");
  else
    FKeyHistory->Delete(FKeyHistory->GetCount() - 1);
}
//------------------------------------------------------------------------------
void THierarchicalStorage::ClearSubKeys()
{
  std::unique_ptr<TStringList> SubKeys(new TStringList());
  GetSubKeyNames(SubKeys.get());
  for (intptr_t Index = 0; Index < SubKeys->GetCount(); ++Index)
  {
    RecursiveDeleteSubKey(SubKeys->GetString(Index));
  }
}
//------------------------------------------------------------------------------
void THierarchicalStorage::RecursiveDeleteSubKey(const UnicodeString & Key)
{
  if (OpenSubKey(Key, false))
  {
    ClearSubKeys();
    CloseSubKey();
  }
  DeleteSubKey(Key);
}
//------------------------------------------------------------------------------
bool THierarchicalStorage::HasSubKeys()
{
  std::unique_ptr<TStrings> SubKeys(new TStringList());
  GetSubKeyNames(SubKeys.get());
  bool Result = SubKeys->GetCount() > 0;
  return Result;
}
//------------------------------------------------------------------------------
bool THierarchicalStorage::HasSubKey(const UnicodeString & SubKey)
{
  bool Result = OpenSubKey(SubKey, false);
  if (Result)
  {
    CloseSubKey();
  }
  return Result;
}
//------------------------------------------------------------------------------
bool THierarchicalStorage::KeyExists(const UnicodeString & SubKey)
{
  return DoKeyExists(SubKey, GetForceAnsi());
}
//------------------------------------------------------------------------------
void THierarchicalStorage::ReadValues(Classes::TStrings* Strings,
  bool MaintainKeys)
{
  std::unique_ptr<TStrings> Names(new TStringList());
  GetValueNames(Names.get());
  for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
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
//------------------------------------------------------------------------------
void THierarchicalStorage::ClearValues()
{
  std::unique_ptr<TStrings> Names(new TStringList());
  GetValueNames(Names.get());
  for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
  {
    DeleteValue(Names->GetString(Index));
  }
}
//------------------------------------------------------------------------------
void THierarchicalStorage::WriteValues(Classes::TStrings * Strings,
  bool MaintainKeys)
{
  ClearValues();

  if (Strings)
  {
    for (intptr_t Index = 0; Index < Strings->GetCount(); ++Index)
    {
      if (MaintainKeys)
      {
        assert(Strings->GetString(Index).Pos(L"=") > 1);
        WriteString(Strings->GetName(Index), Strings->GetValue(Strings->GetName(Index)));
      }
      else
      {
        WriteString(Sysutils::IntToStr(Index), Strings->GetString(Index));
      }
    }
  }
}
//------------------------------------------------------------------------------
UnicodeString THierarchicalStorage::ReadString(const UnicodeString & Name, const UnicodeString & Default)
{
  UnicodeString Result;
  if (GetMungeStringValues())
  {
    Result = UnMungeStr(ReadStringRaw(Name, MungeStr(Default, GetForceAnsi())));
  }
  else
  {
    Result = ReadStringRaw(Name, Default);
  }
  return Result;
}
//------------------------------------------------------------------------------
RawByteString THierarchicalStorage::ReadBinaryData(const UnicodeString & Name)
{
  size_t Size = BinaryDataSize(Name);
  RawByteString Value;
  Value.SetLength(Size);
  ReadBinaryData(Name, static_cast<void *>(const_cast<char *>(Value.c_str())), Size);
  return Value;
}
//------------------------------------------------------------------------------
RawByteString THierarchicalStorage::ReadStringAsBinaryData(const UnicodeString & Name, const RawByteString & Default)
{
  UnicodeString UnicodeDefault = UnicodeString(AnsiString(Default.c_str(), Default.Length()).c_str());
  // This should be exactly the same operation as calling ReadString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  UnicodeString String = ReadString(Name, UnicodeDefault);
  AnsiString Ansi = AnsiString(String);
  RawByteString Result = RawByteString(Ansi.c_str(), Ansi.Length());
  return Result;
}
//------------------------------------------------------------------------------
void THierarchicalStorage::WriteString(const UnicodeString & Name, const UnicodeString & Value)
{
  if (GetMungeStringValues())
  {
    WriteStringRaw(Name, MungeStr(Value, GetForceAnsi()));
  }
  else
  {
    WriteStringRaw(Name, Value);
  }
}
//------------------------------------------------------------------------------
void THierarchicalStorage::WriteBinaryData(const UnicodeString & Name,
  const RawByteString & Value)
{
  WriteBinaryData(Name, Value.c_str(), Value.Length());
}
//------------------------------------------------------------------------------
void THierarchicalStorage::WriteBinaryDataAsString(const UnicodeString & Name, const RawByteString & Value)
{
  // This should be exactly the same operation as calling WriteString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  AnsiString Ansi = AnsiString(Value.c_str(), Value.Length());
  WriteString(Name, UnicodeString(Ansi.c_str()));
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
UnicodeString THierarchicalStorage::ExcludeTrailingBackslash(const UnicodeString & S)
{
  // expanded from ?: as it caused memory leaks
  if (S.IsEmpty())
  {
    return S;
  }
  else
  {
    return Sysutils::ExcludeTrailingBackslash(S);
  }
}
//===========================================================================
TRegistryStorage::TRegistryStorage(const UnicodeString & AStorage) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(nullptr)
{
  Init();
}
//------------------------------------------------------------------------------
TRegistryStorage::TRegistryStorage(const UnicodeString & AStorage, HKEY ARootKey):
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(nullptr),
  FFailed(0)
{
  Init();
  FRegistry->SetRootKey(ARootKey);
}
//------------------------------------------------------------------------------
void TRegistryStorage::Init()
{
  FFailed = 0;
  FRegistry = new TRegistry();
  FRegistry->SetAccess(KEY_READ);
}
//------------------------------------------------------------------------------
TRegistryStorage::~TRegistryStorage()
{
  SAFE_DESTROY(FRegistry);
}
//------------------------------------------------------------------------------
bool TRegistryStorage::Copy(TRegistryStorage * Storage)
{
  TRegistry * Registry = Storage->FRegistry;
  bool Result = true;
  std::unique_ptr<TStrings> Names(new TStringList());
  rde::vector<uint8_t> Buffer(1024);
  Registry->GetValueNames(Names.get());
  intptr_t Index = 0;
  while ((Index < Names->GetCount()) && Result)
  {
    UnicodeString Name = MungeStr(Names->GetString(Index), GetForceAnsi());
    DWORD Size = static_cast<DWORD>(Buffer.size());
    DWORD Type;
    int RegResult = 0;
    do
    {
      RegResult = RegQueryValueEx(Registry->GetCurrentKey(), Name.c_str(), nullptr,
        &Type, &Buffer[0], &Size);
      if (RegResult == ERROR_MORE_DATA)
      {
        Buffer.resize(Size);
      }
    } while (RegResult == ERROR_MORE_DATA);

    Result = (RegResult == ERROR_SUCCESS);
    if (Result)
    {
      RegResult = RegSetValueEx(FRegistry->GetCurrentKey(), Name.c_str(), 0, Type,
        &Buffer[0], Size);
      Result = (RegResult == ERROR_SUCCESS);
    }

    ++Index;
  }
  return Result;
}
//------------------------------------------------------------------------------
UnicodeString TRegistryStorage::GetSource() const
{
  return RootKeyToStr(FRegistry->GetRootKey()) + L"\\" + GetStorage();
}
//------------------------------------------------------------------------------
void TRegistryStorage::SetAccessMode(TStorageAccessMode Value)
{
  THierarchicalStorage::SetAccessMode(Value);
  if (FRegistry)
  {
    switch (GetAccessMode())
    {
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
//------------------------------------------------------------------------------
bool TRegistryStorage::DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate)
{
  if (FKeyHistory->GetCount() > 0)
  {
    FRegistry->CloseKey();
  }
  UnicodeString K = ExcludeTrailingBackslash(GetStorage() + GetCurrentSubKey() + SubKey);
  return FRegistry->OpenKey(K, CanCreate);
}
//------------------------------------------------------------------------------
void TRegistryStorage::CloseSubKey()
{
  FRegistry->CloseKey();
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount())
  {
    FRegistry->OpenKey(GetStorage() + GetCurrentSubKeyMunged(), True);
  }
}
//------------------------------------------------------------------------------
bool TRegistryStorage::DeleteSubKey(const UnicodeString & SubKey)
{
  UnicodeString K;
  if (FKeyHistory->GetCount() == 0)
  {
    K = GetStorage() + GetCurrentSubKey();
  }
  K += MungeKeyName(SubKey);
  return FRegistry->DeleteKey(K);
}
//------------------------------------------------------------------------------
void TRegistryStorage::GetSubKeyNames(Classes::TStrings * Strings)
{
  FRegistry->GetKeyNames(Strings);
  for (intptr_t Index = 0; Index < Strings->GetCount(); ++Index)
  {
    Strings->SetString(Index, UnMungeStr(Strings->GetString(Index)));
  }
}
//------------------------------------------------------------------------------
void TRegistryStorage::GetValueNames(Classes::TStrings * Strings) const
{
  FRegistry->GetValueNames(Strings);
}
//------------------------------------------------------------------------------
bool TRegistryStorage::DeleteValue(const UnicodeString & Name)
{
  return FRegistry->DeleteValue(Name);
}
//------------------------------------------------------------------------------
bool TRegistryStorage::DoKeyExists(const UnicodeString & SubKey, bool AForceAnsi)
{
  UnicodeString K = MungeStr(SubKey, AForceAnsi);
  bool Result = FRegistry->KeyExists(K);
  return Result;
}
//------------------------------------------------------------------------------
bool TRegistryStorage::ValueExists(const UnicodeString & Value) const
{
  bool Result = FRegistry->ValueExists(Value);
  return Result;
}
//------------------------------------------------------------------------------
size_t TRegistryStorage::BinaryDataSize(const UnicodeString & Name)
{
  size_t Result = FRegistry->GetDataSize(Name);
  return Result;
}
//------------------------------------------------------------------------------
bool TRegistryStorage::ReadBool(const UnicodeString & Name, bool Default)
{
  READ_REGISTRY(ReadBool);
}
//------------------------------------------------------------------------------
TDateTime TRegistryStorage::ReadDateTime(const UnicodeString & Name, const TDateTime & Default)
{
  READ_REGISTRY(ReadDateTime);
}
//------------------------------------------------------------------------------
double TRegistryStorage::ReadFloat(const UnicodeString & Name, double Default)
{
  READ_REGISTRY(ReadFloat);
}
//------------------------------------------------------------------------------
intptr_t TRegistryStorage::ReadInteger(const UnicodeString & Name, intptr_t Default)
{
  READ_REGISTRY(ReadInteger);
}
//------------------------------------------------------------------------------
int64_t TRegistryStorage::ReadInt64(const UnicodeString & Name, int64_t Default)
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
//------------------------------------------------------------------------------
UnicodeString TRegistryStorage::ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default)
{
  READ_REGISTRY(ReadString);
}
//------------------------------------------------------------------------------
size_t TRegistryStorage::ReadBinaryData(const UnicodeString & Name,
  void * Buffer, size_t Size)
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
//------------------------------------------------------------------------------
void TRegistryStorage::WriteBool(const UnicodeString & Name, bool Value)
{
  WRITE_REGISTRY(WriteBool);
}
//------------------------------------------------------------------------------
void TRegistryStorage::WriteDateTime(const UnicodeString & Name, const TDateTime & Value)
{
  WRITE_REGISTRY(WriteDateTime);
}
//------------------------------------------------------------------------------
void TRegistryStorage::WriteFloat(const UnicodeString & Name, double Value)
{
  WRITE_REGISTRY(WriteFloat);
}
//------------------------------------------------------------------------------
void TRegistryStorage::WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value)
{
  WRITE_REGISTRY(WriteString);
}
//------------------------------------------------------------------------------
void TRegistryStorage::WriteInteger(const UnicodeString & Name, intptr_t Value)
{
  WRITE_REGISTRY(WriteInteger);
}
//------------------------------------------------------------------------------
void TRegistryStorage::WriteInt64(const UnicodeString & Name, int64_t Value)
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
//------------------------------------------------------------------------------
void TRegistryStorage::WriteBinaryData(const UnicodeString & Name,
  const void * Buffer, size_t Size)
{
  try
  {
    FRegistry->WriteBinaryData(Name, const_cast<void *>(Buffer), Size);
  }
  catch (...)
  {
    FFailed++;
  }
}
//------------------------------------------------------------------------------
intptr_t TRegistryStorage::GetFailed()
{
  intptr_t Result = FFailed;
  FFailed = 0;
  return Result;
}
