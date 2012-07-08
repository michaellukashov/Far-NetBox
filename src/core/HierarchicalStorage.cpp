//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#endif

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "HierarchicalStorage.h"
#include <TextsCore.h>
#include <vector>
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch(...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch(...) { FFailed++; }
//---------------------------------------------------------------------------
UnicodeString __fastcall MungeStr(const UnicodeString Str, bool ForceAnsi)
{
  RawByteString Source;
  if (ForceAnsi)
  {
    Source = AnsiString(W2MB(Str.c_str()).c_str());
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
  putty_mungestr(Source.c_str(), (char *)Dest.c_str());
  PackStr(Dest);
  return UnicodeString(Dest.c_str(), Dest.Length());
}
//---------------------------------------------------------------------------
UnicodeString __fastcall UnMungeStr(const UnicodeString Str)
{
  // Str should contain ASCII characters only
  RawByteString Source = Str;
  RawByteString Dest;
  Dest.SetLength(Source.Length() + 1);
  putty_unmungestr(Source.c_str(), (char *)Dest.c_str(), Dest.Length());
  UnicodeString Result;
  if (Dest.Pos(Bom.c_str()) == 1)
  {
    Dest.Delete(1, Bom.size());
    Result = UTF8String(Dest.c_str());
  }
  else
  {
    Result = AnsiString(MB2W(Dest.c_str()).c_str());
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall PuttyMungeStr(const UnicodeString Str)
{
  return MungeStr(Str, false);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall PuttyUnMungeStr(const UnicodeString Str)
{
  return UnMungeStr(Str);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall MungeIniName(const UnicodeString Str)
{
  int P = Str.Pos(L"=");
  // make this fast for now
  if (P > 0)
  {
    return StringReplace(Str, L"=", L"%3D", TReplaceFlags::Init(rfReplaceAll));
  }
  else
  {
    return Str;
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall UnMungeIniName(const UnicodeString Str)
{
  int P = Str.Pos(L"%3D");
  // make this fast for now
  if (P > 0)
  {
    return StringReplace(Str, L"%3D", L"=", TReplaceFlags::Init(rfReplaceAll));
  }
  else
  {
    return Str;
  }
}
//===========================================================================
/* __fastcall */ THierarchicalStorage::THierarchicalStorage(const UnicodeString AStorage)
{
  FStorage = AStorage;
  FKeyHistory = new TStringList();
  SetAccessMode(smRead);
  SetExplicit(false);
  SetForceAnsi(true);
  SetMungeStringValues(true);
}
//---------------------------------------------------------------------------
/* __fastcall */ THierarchicalStorage::~THierarchicalStorage()
{
  delete FKeyHistory;
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::Flush()
{
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::SetAccessMode(TStorageAccessMode value)
{
  FAccessMode = value;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall THierarchicalStorage::GetCurrentSubKeyMunged()
{
  if (FKeyHistory->GetCount()) { return FKeyHistory->GetStrings(FKeyHistory->GetCount()-1); }
    else { return L""; }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall THierarchicalStorage::GetCurrentSubKey()
{
  return UnMungeStr(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool __fastcall THierarchicalStorage::OpenRootKey(bool CanCreate)
{
  return OpenSubKey(L"", CanCreate);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall THierarchicalStorage::MungeKeyName(UnicodeString Key)
{
  UnicodeString Result = MungeStr(Key, GetForceAnsi());
  // if there's already ANSI-munged subkey, keep ANSI munging
  if ((Result != Key) && !GetForceAnsi() && DoKeyExists(Key, true))
  {
    Result = MungeStr(Key, true);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall THierarchicalStorage::OpenSubKey(UnicodeString Key, bool CanCreate, bool Path)
{
  bool Result;
  UnicodeString MungedKey;
  if (Path)
  {
    assert(Key.IsEmpty() || (Key[Key.Length()] != L'\\'));
    Result = true;
    while (!Key.IsEmpty() && Result)
    {
      if (!MungedKey.IsEmpty())
      {
        MungedKey += L'\\';
      }
      MungedKey += MungeKeyName(CutToChar(Key, L'\\', false));
      Result = DoOpenSubKey(MungedKey, CanCreate);
    }

    // hack to restore last opened key for registry storage
    if (!Result)
    {
      FKeyHistory->Add(IncludeTrailingBackslash(GetCurrentSubKey()+MungedKey));
      CloseSubKey();
    }
  }
  else
  {
    MungedKey = MungeKeyName(Key);
    Result = DoOpenSubKey(MungedKey, CanCreate);
  }

  if (Result)
  {
    FKeyHistory->Add(IncludeTrailingBackslash(GetCurrentSubKey()+MungedKey));
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::CloseSubKey()
{
  if (FKeyHistory->GetCount() == 0) throw Exception(L"");
    else FKeyHistory->Delete(FKeyHistory->GetCount()-1);
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::ClearSubKeys()
{
  TStringList *SubKeys = new TStringList();
  // try
  {
    BOOST_SCOPE_EXIT ( (&SubKeys) )
    {
      delete SubKeys;
    } BOOST_SCOPE_EXIT_END
    GetSubKeyNames(SubKeys);
    for (int Index = 0; Index < SubKeys->GetCount(); Index++)
    {
      RecursiveDeleteSubKey(SubKeys->GetStrings(Index));
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete SubKeys;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::RecursiveDeleteSubKey(const UnicodeString Key)
{
  if (OpenSubKey(Key, false))
  {
    ClearSubKeys();
    CloseSubKey();
  }
  DeleteSubKey(Key);
}
//---------------------------------------------------------------------------
bool __fastcall THierarchicalStorage::HasSubKeys()
{
  bool Result;
  TStrings * SubKeys = new TStringList();
  // try
  {
    BOOST_SCOPE_EXIT ( (&SubKeys) )
    {
      delete SubKeys;
    } BOOST_SCOPE_EXIT_END
    GetSubKeyNames(SubKeys);
    Result = (SubKeys->GetCount() > 0);
  }
#ifndef _MSC_VER
  __finally
  {
    delete SubKeys;
  }
#endif
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall THierarchicalStorage::HasSubKey(const UnicodeString SubKey)
{
  bool Result = OpenSubKey(SubKey, false);
  if (Result)
  {
    CloseSubKey();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall THierarchicalStorage::KeyExists(const UnicodeString SubKey)
{
  return DoKeyExists(SubKey, GetForceAnsi());
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::ReadValues(Classes::TStrings* Strings,
  bool MaintainKeys)
{
  TStrings * Names = new TStringList();
  // try
  {
    BOOST_SCOPE_EXIT ( (&Names) )
    {
      delete Names;
    } BOOST_SCOPE_EXIT_END
    GetValueNames(Names);
    for (int Index = 0; Index < Names->GetCount(); Index++)
    {
      if (MaintainKeys)
      {
        Strings->Add(FORMAT(L"%s=%s", Names->GetStrings(Index).c_str(),
          ReadString(Names->GetStrings(Index), L"").c_str()));
      }
      else
      {
        Strings->Add(ReadString(Names->GetStrings(Index), L""));
      }
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete Names;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::ClearValues()
{
  TStrings * Names = new TStringList();
  // try
  {
    BOOST_SCOPE_EXIT ( (&Names) )
    {
      delete Names;
    } BOOST_SCOPE_EXIT_END
    GetValueNames(Names);
    for (int Index = 0; Index < Names->GetCount(); Index++)
    {
      DeleteValue(Names->GetStrings(Index));
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete Names;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::WriteValues(Classes::TStrings * Strings,
  bool MaintainKeys)
{
  ClearValues();

  if (Strings)
  {
    for (int Index = 0; Index < Strings->GetCount(); Index++)
    {
      if (MaintainKeys)
      {
        assert(Strings->GetStrings(Index).Pos(L"=") > 1);
        WriteString(Strings->GetName(Index), Strings->GetValue(Strings->GetName(Index)));
      }
      else
      {
        WriteString(IntToStr(Index), Strings->GetStrings(Index));
      }
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall THierarchicalStorage::ReadString(const UnicodeString Name, const UnicodeString Default)
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
//---------------------------------------------------------------------------
RawByteString __fastcall THierarchicalStorage::ReadBinaryData(const UnicodeString Name)
{
  size_t Size = BinaryDataSize(Name);
  RawByteString Value;
  Value.SetLength(Size);
  ReadBinaryData(Name, (void *)Value.c_str(), Size);
  return Value;
}
//---------------------------------------------------------------------------
RawByteString __fastcall THierarchicalStorage::ReadStringAsBinaryData(const UnicodeString Name, const RawByteString Default)
{
  UnicodeString UnicodeDefault = UnicodeString(AnsiString(Default.c_str(), Default.Length()));
  // This should be exactly the same operation as calling ReadString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  UnicodeString String = ReadString(Name, UnicodeDefault);
  AnsiString Ansi = AnsiString(String);
  RawByteString Result = RawByteString(Ansi.c_str(), Ansi.Length());
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::WriteString(const UnicodeString Name, const UnicodeString Value)
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
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::WriteBinaryData(const UnicodeString Name,
  const RawByteString Value)
{
  WriteBinaryData(Name, Value.c_str(), Value.Length());
}
//---------------------------------------------------------------------------
void __fastcall THierarchicalStorage::WriteBinaryDataAsString(const UnicodeString Name, const RawByteString Value)
{
  // This should be exactly the same operation as calling WriteString in
  // C++Builder 6 (non-Unicode) on Unicode-based OS
  // (conversion is done by Ansi layer of the OS)
  AnsiString Ansi = AnsiString(Value.c_str(), Value.Length());
  WriteString(Name, UnicodeString(Ansi));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall THierarchicalStorage::IncludeTrailingBackslash(const UnicodeString & S)
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
//---------------------------------------------------------------------------
UnicodeString __fastcall THierarchicalStorage::ExcludeTrailingBackslash(const UnicodeString & S)
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
//===========================================================================
/* __fastcall */ TRegistryStorage::TRegistryStorage(const UnicodeString AStorage):
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(NULL)
{
  Init();
}
//---------------------------------------------------------------------------
/* __fastcall */ TRegistryStorage::TRegistryStorage(const UnicodeString AStorage, HKEY ARootKey):
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FRegistry(NULL)
{
  Init();
  FRegistry->SetRootKey(ARootKey);
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::Init()
{
  FFailed = 0;
  FRegistry = new TRegistry();
  FRegistry->SetAccess(KEY_READ);
}
//---------------------------------------------------------------------------
/* __fastcall */ TRegistryStorage::~TRegistryStorage()
{
  delete FRegistry;
};
//---------------------------------------------------------------------------
bool __fastcall TRegistryStorage::Copy(TRegistryStorage * Storage)
{
  TRegistry * Registry = Storage->FRegistry;
  bool Result = true;
  TStrings * Names = new TStringList();
  // try
  {
    BOOST_SCOPE_EXIT ( (&Names) )
    {
      delete Names;
    } BOOST_SCOPE_EXIT_END
    Registry->GetValueNames(Names);
    std::vector<unsigned char> Buffer(1024, 0);
    int Index = 0;
    while ((Index < Names->GetCount()) && Result)
    {
      UnicodeString Name = MungeStr(Names->GetStrings(Index), GetForceAnsi());
      unsigned long Size = Buffer.size();
      unsigned long Type;
      int RegResult = 0;
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
#ifndef _MSC_VER
  __finally
  {
    delete Names;
  }
#endif
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRegistryStorage::GetSource()
{
  return RootKeyToStr(FRegistry->GetRootKey()) + L"\\" + GetStorage();
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::SetAccessMode(TStorageAccessMode value)
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
bool __fastcall TRegistryStorage::DoOpenSubKey(const UnicodeString SubKey, bool CanCreate)
{
  if (FKeyHistory->GetCount() > 0) { FRegistry->CloseKey(); }
  UnicodeString K = ExcludeTrailingBackslash(GetStorage() + GetCurrentSubKey() + SubKey);
  return FRegistry->OpenKey(K, CanCreate);
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::CloseSubKey()
{
  FRegistry->CloseKey();
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount())
  {
    FRegistry->OpenKey(GetStorage() + GetCurrentSubKeyMunged(), True);
  }
}
//---------------------------------------------------------------------------
bool __fastcall TRegistryStorage::DeleteSubKey(const UnicodeString SubKey)
{
  UnicodeString K;
  if (FKeyHistory->GetCount() == 0) { K = GetStorage() + GetCurrentSubKey(); }
  K += MungeKeyName(SubKey);
  return FRegistry->DeleteKey(K);
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::GetSubKeyNames(Classes::TStrings* Strings)
{
  FRegistry->GetKeyNames(Strings);
  for (int Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->PutString(Index, UnMungeStr(Strings->GetStrings(Index)));
  }
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::GetValueNames(Classes::TStrings* Strings)
{
  FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool __fastcall TRegistryStorage::DeleteValue(const UnicodeString Name)
{
  return FRegistry->DeleteValue(Name);
}
//---------------------------------------------------------------------------
bool __fastcall TRegistryStorage::DoKeyExists(const UnicodeString SubKey, bool AForceAnsi)
{
  UnicodeString K = MungeStr(SubKey, AForceAnsi);
  bool Result = FRegistry->KeyExists(K);
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TRegistryStorage::ValueExists(const UnicodeString Value)
{
  bool Result = FRegistry->ValueExists(Value);
  return Result;
}
//---------------------------------------------------------------------------
size_t __fastcall TRegistryStorage::BinaryDataSize(const UnicodeString Name)
{
  size_t Result = FRegistry->GetDataSize(Name);
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TRegistryStorage::ReadBool(const UnicodeString Name, bool Default)
{
  READ_REGISTRY(ReadBool);
}
//---------------------------------------------------------------------------
TDateTime __fastcall TRegistryStorage::ReadDateTime(const UnicodeString Name, TDateTime Default)
{
  READ_REGISTRY(ReadDateTime);
}
//---------------------------------------------------------------------------
double __fastcall TRegistryStorage::ReadFloat(const UnicodeString Name, double Default)
{
  READ_REGISTRY(ReadFloat);
}
//---------------------------------------------------------------------------
int __fastcall TRegistryStorage::ReadInteger(const UnicodeString Name, int Default)
{
  READ_REGISTRY(ReadInteger);
}
//---------------------------------------------------------------------------
__int64 __fastcall TRegistryStorage::ReadInt64(const UnicodeString Name, __int64 Default)
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
UnicodeString __fastcall TRegistryStorage::ReadStringRaw(const UnicodeString Name, const UnicodeString Default)
{
  READ_REGISTRY(ReadString);
}
//---------------------------------------------------------------------------
size_t __fastcall TRegistryStorage::ReadBinaryData(const UnicodeString Name,
  void * Buffer, size_t Size)
{
  size_t Result;
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
void __fastcall TRegistryStorage::WriteBool(const UnicodeString Name, bool Value)
{
  WRITE_REGISTRY(WriteBool);
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::WriteDateTime(const UnicodeString Name, TDateTime Value)
{
  WRITE_REGISTRY(WriteDateTime);
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::WriteFloat(const UnicodeString Name, double Value)
{
  WRITE_REGISTRY(WriteFloat);
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::WriteStringRaw(const UnicodeString Name, const UnicodeString Value)
{
  WRITE_REGISTRY(WriteString);
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::WriteInteger(const UnicodeString Name, int Value)
{
  WRITE_REGISTRY(WriteInteger);
}
//---------------------------------------------------------------------------
void __fastcall TRegistryStorage::WriteInt64(const UnicodeString Name, __int64 Value)
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
void __fastcall TRegistryStorage::WriteBinaryData(const UnicodeString Name,
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
int __fastcall TRegistryStorage::GetFailed()
{
  int Result = FFailed;
  FFailed = 0;
  return Result;
}
#ifndef _MSC_VER
//===========================================================================
__fastcall TCustomIniFileStorage::TCustomIniFileStorage(const UnicodeString Storage, TCustomIniFile * IniFile) :
  THierarchicalStorage(Storage),
  FIniFile(IniFile)
{
}
//---------------------------------------------------------------------------
__fastcall TCustomIniFileStorage::~TCustomIniFileStorage()
{
  delete FIniFile;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomIniFileStorage::GetSource()
{
  return GetStorage();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomIniFileStorage::GetCurrentSection()
{
  return ExcludeTrailingBackslash(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::DoOpenSubKey(const UnicodeString SubKey, bool CanCreate)
{
  bool Result = CanCreate;

  if (!Result)
  {
    TStringList * Sections = new TStringList();
    // try
    {
      BOOST_SCOPE_EXIT ( (&Sections) )
      {
        delete Sections;
      } BOOST_SCOPE_EXIT_END
      Sections->SetSorted(true);
      FIniFile->ReadSections(Sections);
      UnicodeString NewKey = ExcludeTrailingBackslash(GetCurrentSubKey()+SubKey);
      if (Sections->GetCount())
      {
        int Index = -1;
        Result = Sections->Find(NewKey, Index);
        if (!Result && Index < Sections->GetCount() &&
            Sections->GetStrings(Index).SubString(1, NewKey.Length()+1) == NewKey + L"\\")
        {
          Result = true;
        }
      }
    }
#ifndef _MSC_VER
    __finally
    {
      delete Sections;
    }
#endif
  }

  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::DeleteSubKey(const UnicodeString SubKey)
{
  bool Result;
  try
  {
    FIniFile->EraseSection(GetCurrentSubKey() + MungeKeyName(SubKey));
    Result = true;
  }
  catch (...)
  {
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::GetSubKeyNames(Classes::TStrings* Strings)
{
  TStrings * Sections = new TStringList();
  // try
  {
    BOOST_SCOPE_EXIT ( (&Sections) )
    {
      delete Sections;
    } BOOST_SCOPE_EXIT_END
    Strings->Clear();
    FIniFile->ReadSections(Sections);
    for (int i = 0; i < Sections->GetCount(); i++)
    {
      UnicodeString Section = Sections->GetStrings(i);
      if (AnsiCompareText(GetCurrentSubKey(),
          Section.SubString(1, GetCurrentSubKey().Length())) == 0)
      {
        UnicodeString SubSection = Section.SubString(GetCurrentSubKey().Length() + 1,
          Section.Length() - GetCurrentSubKey().Length());
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
#ifndef _MSC_VER
  __finally
  {
    delete Sections;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::GetValueNames(Classes::TStrings* Strings)
{
  FIniFile->ReadSection(GetCurrentSection(), Strings);
  for (int Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->PutString(Index, UnMungeIniName(Strings->GetStrings(Index)));
  }
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::DoKeyExists(const UnicodeString SubKey, bool AForceAnsi)
{
  return FIniFile->SectionExists(GetCurrentSubKey() + MungeStr(SubKey, AForceAnsi));
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::ValueExists(const UnicodeString Value)
{
  return FIniFile->ValueExists(GetCurrentSection(), MungeIniName(Value));
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::DeleteValue(const UnicodeString Name)
{
  FIniFile->DeleteKey(GetCurrentSection(), MungeIniName(Name));
  return true;
}
//---------------------------------------------------------------------------
size_t __fastcall TCustomIniFileStorage::BinaryDataSize(const UnicodeString Name)
{
  return ReadStringRaw(Name, L"").Length() / 2;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomIniFileStorage::ReadBool(const UnicodeString Name, bool Default)
{
  return FIniFile->ReadBool(GetCurrentSection(), MungeIniName(Name), Default);
}
//---------------------------------------------------------------------------
int __fastcall TCustomIniFileStorage::ReadInteger(const UnicodeString Name, int Default)
{
  int Result = FIniFile->ReadInteger(GetCurrentSection(), MungeIniName(Name), Default);
  return Result;
}
//---------------------------------------------------------------------------
__int64 __fastcall TCustomIniFileStorage::ReadInt64(const UnicodeString Name, __int64 Default)
{
  __int64 Result = Default;
  UnicodeString Str;
  Str = ReadStringRaw(Name, L"");
  if (!Str.IsEmpty())
  {
    Result = StrToInt64Def(Str, Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime __fastcall TCustomIniFileStorage::ReadDateTime(const UnicodeString Name, TDateTime Default)
{
  TDateTime Result;
  UnicodeString Value = FIniFile->ReadString(GetCurrentSection(), MungeIniName(Name), L"");
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

  return Result;
}
//---------------------------------------------------------------------------
double __fastcall TCustomIniFileStorage::ReadFloat(const UnicodeString Name, double Default)
{
  double Result;
  UnicodeString Value = FIniFile->ReadString(GetCurrentSection(), MungeIniName(Name), L"");
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

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomIniFileStorage::ReadStringRaw(const UnicodeString Name, UnicodeString Default)
{
  AnsiString Result = FIniFile->ReadString(GetCurrentSection(), MungeIniName(Name), Default);
  return Result;
}
//---------------------------------------------------------------------------
size_t __fastcall TCustomIniFileStorage::ReadBinaryData(const UnicodeString Name,
  void * Buffer, size_t Size)
{
  RawByteString Value = HexToBytes(ReadStringRaw(Name, L""));
  size_t Len = Value.Length();
  if (Size > Len)
  {
    Size = Len;
  }
  assert(Buffer);
  memcpy(Buffer, Value.c_str(), Size);
  return Size;
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteBool(const UnicodeString Name, bool Value)
{
  FIniFile->WriteBool(GetCurrentSection(), MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteInteger(const UnicodeString Name, int Value)
{
  FIniFile->WriteInteger(GetCurrentSection(), MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteInt64(const UnicodeString Name, __int64 Value)
{
  WriteStringRaw(Name, IntToStr(Value));
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteDateTime(const UnicodeString Name, TDateTime Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteFloat(const UnicodeString Name, double Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteStringRaw(const UnicodeString Name, const UnicodeString Value)
{
  FIniFile->WriteString(GetCurrentSection(), MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void __fastcall TCustomIniFileStorage::WriteBinaryData(const UnicodeString Name,
  const void * Buffer, int Size)
{
  WriteStringRaw(Name, BytesToHex(RawByteString(static_cast<const char*>(Buffer), Size)));
}
//===========================================================================
/* __fastcall */ TIniFileStorage::TIniFileStorage(const UnicodeString AStorage):
  TCustomIniFileStorage(AStorage, new TMemIniFile(AStorage))
{
  FOriginal = new TStringList();
  dynamic_cast<TMemIniFile *>(FIniFile)->GetStrings(FOriginal);
  ApplyOverrides();
}
//---------------------------------------------------------------------------
void __fastcall TIniFileStorage::Flush()
{
  if (FOriginal != NULL)
  {
    TStrings * Strings = new TStringList;
    try
    {
      dynamic_cast<TMemIniFile *>(FIniFile)->GetStrings(Strings);
      if (!Strings->Equals(FOriginal))
      {
        int Attr;
        // preserve attributes (especially hidden)
        bool Exists = FileExists(Storage);
        if (Exists)
        {
          Attr = GetFileAttributes(UnicodeString(Storage).c_str());
        }
        else
        {
          Attr = FILE_ATTRIBUTE_NORMAL;
        }

        HANDLE Handle = CreateFile(UnicodeString(Storage).c_str(), GENERIC_READ | GENERIC_WRITE,
          0, NULL, CREATE_ALWAYS, Attr, 0);

        if (Handle == INVALID_HANDLE_VALUE)
        {
          // "access denied" errors upon implicit saves to existing file are ignored
          if (Explicit || !Exists || (GetLastError() != ERROR_ACCESS_DENIED))
          {
            try
            {
              RaiseLastOSError();
            }
            catch(Exception & E)
            {
              throw ExtException(&E, FMTLOAD(CREATE_FILE_ERROR, (Storage)));
            }
          }
        }
        else
        {
          TStream * Stream = new THandleStream(Handle);
          try
          {
            Strings->SaveToStream(Stream);
          }
          __finally
          {
            CloseHandle(Handle);
            delete Stream;
          }
        }
      }
    }
    __finally
    {
      delete FOriginal;
      FOriginal = NULL;
      delete Strings;
    }
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ TIniFileStorage::~TIniFileStorage()
{
  Flush();
}
//---------------------------------------------------------------------------
void __fastcall TIniFileStorage::ApplyOverrides()
{
  UnicodeString OverridesKey = IncludeTrailingBackslash(L"Override");

  TStrings * Sections = new TStringList();
  // try
  {
    BOOST_SCOPE_EXIT ( (&Sections) )
    {
      delete Sections;
    } BOOST_SCOPE_EXIT_END
    Sections->Clear();
    FIniFile->ReadSections(Sections);
    for (int i = 0; i < Sections->Count; i++)
    {
      UnicodeString Section = Sections->Strings[i];

      if (AnsiSameText(OverridesKey,
            Section.SubString(1, OverridesKey.Length())))
      {
        UnicodeString SubKey = Section.SubString(OverridesKey.Length() + 1,
          Section.Length() - OverridesKey.Length());

        // this all uses raw names (munged)
        TStrings * Names = new TStringList;
        // try
        {
          BOOST_SCOPE_EXIT ( (&Names) )
          {
            delete Names;
          } BOOST_SCOPE_EXIT_END
          FIniFile->ReadSection(Section, Names);

          for (int ii = 0; ii < Names->Count; ii++)
          {
            UnicodeString Name = Names->Strings[ii];
            UnicodeString Value = FIniFile->ReadString(Section, Name, L"");
            FIniFile->WriteString(SubKey, Name, Value);
          }
        }
#ifndef _MSC_VER
        __finally
        {
          delete Names;
        }
#endif

        FIniFile->EraseSection(Section);
      }
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete Sections;
  }
#endif
}
//===========================================================================
#define NOT_IMPLEMENTED throw Exception(L"Not implemented")
//===========================================================================
class TOptionsIniFile : public TCustomIniFile
{
public:
  /* __fastcall */ TOptionsIniFile(TStrings * Options);

  virtual UnicodeString __fastcall ReadString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Default);
  virtual void __fastcall WriteString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Value);
  virtual void __fastcall ReadSection(const UnicodeString Section, TStrings * Strings);
  virtual void __fastcall ReadSections(TStrings* Strings);
  virtual void __fastcall ReadSectionValues(const UnicodeString Section, TStrings* Strings);
  virtual void __fastcall EraseSection(const UnicodeString Section);
  virtual void __fastcall DeleteKey(const UnicodeString Section, const UnicodeString Ident);
  virtual void __fastcall UpdateFile();
  // Hoisted overload
  void __fastcall ReadSections(const UnicodeString Section, TStrings* Strings);

private:
  TStrings * FOptions;
};
//---------------------------------------------------------------------------
/* __fastcall */ TOptionsIniFile::TOptionsIniFile(TStrings * Options) :
  TCustomIniFile(UnicodeString())
{
  FOptions = Options;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TOptionsIniFile::ReadString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Default)
{
  assert(Section.IsEmpty());
  int Index = FOptions->IndexOfName(Ident);
  UnicodeString Value;
  if (Index >= 0)
  {
    Value = FOptions->ValueFromIndex[Index];
  }
  else
  {
    Value = Default;
  }
  return Value;
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::WriteString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Value)
{
  NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::ReadSection(const UnicodeString Section, TStrings * Strings)
{
  assert(Section.IsEmpty());
  Strings->BeginUpdate();

  // try
  {
    BOOST_SCOPE_EXIT ( (&Strings) )
    {
      Strings->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    for (int Index = 0; Index < FOptions->Count; Index++)
    {
      Strings->Add(FOptions->Names[Index]);
    }
  }
#ifndef _MSC_VER
  __finally
  {
    Strings->EndUpdate();
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::ReadSections(TStrings * /*Strings*/)
{
  NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::ReadSectionValues(const UnicodeString Section, TStrings * /*Strings*/)
{
  NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::EraseSection(const UnicodeString Section)
{
  NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::DeleteKey(const UnicodeString Section, const UnicodeString Ident)
{
  NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::UpdateFile()
{
  NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------------
void __fastcall TOptionsIniFile::ReadSections(const UnicodeString Section, TStrings* Strings)
{
  TCustomIniFile::ReadSections(Section, Strings);
}
//===========================================================================
/* __fastcall */ TOptionsStorage::TOptionsStorage(TStrings * Options) :
  TCustomIniFileStorage(UnicodeString(L"Command-line options")) // , new TOptionsIniFile(Options))
{
}
#endif
