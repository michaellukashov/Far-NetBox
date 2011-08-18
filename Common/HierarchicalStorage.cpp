//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "HierarchicalStorage.h"
#include <TextsCore.h>
#include <vector>
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch(...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch(...) { FFailed++; }
//---------------------------------------------------------------------------
wstring MungeStr(const wstring Str)
{
  wstring Result;
  Result.SetLength(Str.Length() * 3 + 1);
  putty_mungestr(Str.c_str(), Result.c_str());
  PackStr(Result);
  return Result;
}
//---------------------------------------------------------------------------
wstring UnMungeStr(const wstring Str)
{
  wstring Result;
  Result.SetLength(Str.Length() * 3 + 1);
  putty_unmungestr(Str.c_str(), Result.c_str(), Result.Length());
  PackStr(Result);
  return Result;
}
//---------------------------------------------------------------------------
wstring PuttyMungeStr(const wstring Str)
{
  return MungeStr(Str);
}
//---------------------------------------------------------------------------
wstring MungeIniName(const wstring Str)
{
  int P = Str.Pos("=");
  // make this fast for now
  if (P > 0)
  {
    return StringReplace(Str, "=", "%3D", TReplaceFlags() << rfReplaceAll);
  }
  else
  {
    return Str;
  }
}
//---------------------------------------------------------------------------
wstring UnMungeIniName(const wstring Str)
{
  int P = Str.Pos("%3D");
  // make this fast for now
  if (P > 0)
  {
    return StringReplace(Str, "%3D", "=", TReplaceFlags() << rfReplaceAll);
  }
  else
  {
    return Str;
  }
}
//===========================================================================
THierarchicalStorage::THierarchicalStorage(const wstring AStorage)
{
  FStorage = AStorage;
  FKeyHistory = new TStringList();
  AccessMode = smRead;
  Explicit = false;
  MungeStringValues = true;
}
//---------------------------------------------------------------------------
THierarchicalStorage::~THierarchicalStorage()
{
  delete FKeyHistory;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::SetAccessMode(TStorageAccessMode value)
{
  FAccessMode = value;
}
//---------------------------------------------------------------------------
wstring THierarchicalStorage::GetCurrentSubKeyMunged()
{
  if (FKeyHistory->Count) return FKeyHistory->Strings[FKeyHistory->Count-1];
    else return "";
}
//---------------------------------------------------------------------------
wstring THierarchicalStorage::GetCurrentSubKey()
{
  return UnMungeStr(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::OpenRootKey(bool CanCreate)
{
  return OpenSubKey("", CanCreate);
}
//---------------------------------------------------------------------------
wstring THierarchicalStorage::MungeSubKey(wstring Key, bool Path)
{
  wstring Result;
  if (Path)
  {
    assert(Key.IsEmpty() || (Key[Key.Length()] != '\\'));
    while (!Key.IsEmpty())
    {
      if (!Result.IsEmpty())
      {
        Result += '\\';
      }
      Result += MungeStr(CutToChar(Key, '\\', false));
    }
  }
  else
  {
    Result = MungeStr(Key);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::OpenSubKey(const wstring SubKey, bool /*CanCreate*/, bool Path)
{
  FKeyHistory->Add(IncludeTrailingBackslash(CurrentSubKey+MungeSubKey(SubKey, Path)));
  return true;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::CloseSubKey()
{
  if (FKeyHistory->Count == 0) throw exception("");
    else FKeyHistory->Delete(FKeyHistory->Count-1);
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ClearSubKeys()
{
  TStringList *SubKeys = new TStringList();
  try
  {
    GetSubKeyNames(SubKeys);
    for (int Index = 0; Index < SubKeys->Count; Index++)
    {
      RecursiveDeleteSubKey(SubKeys->Strings[Index]);
    }
  }
  __finally
  {
    delete SubKeys;
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::RecursiveDeleteSubKey(const wstring Key)
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
  TStrings * SubKeys = new TStringList();
  try
  {
    GetSubKeyNames(SubKeys);
    Result = (SubKeys->Count > 0);
  }
  __finally
  {
    delete SubKeys;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::HasSubKey(const wstring SubKey)
{
  bool Result = OpenSubKey(SubKey, false);
  if (Result)
  {
    CloseSubKey();
  }
  return Result;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ReadValues(TStrings* Strings,
  bool MaintainKeys)
{
  TStrings * Names = new TStringList();
  try
  {
    GetValueNames(Names);
    for (int Index = 0; Index < Names->Count; Index++)
    {
      if (MaintainKeys)
      {
        Strings->Add(FORMAT("%s=%s", (Names->Strings[Index],
          ReadString(Names->Strings[Index], ""))));
      }
      else
      {
        Strings->Add(ReadString(Names->Strings[Index], ""));
      }
    }
  }
  __finally
  {
    delete Names;
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ClearValues()
{
  TStrings * Names = new TStringList();
  try
  {
    GetValueNames(Names);
    for (int Index = 0; Index < Names->Count; Index++)
    {
      DeleteValue(Names->Strings[Index]);
    }
  }
  __finally
  {
    delete Names;
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteValues(TStrings * Strings,
  bool MaintainKeys)
{
  ClearValues();

  if (Strings)
  {
    for (int Index = 0; Index < Strings->Count; Index++)
    {
      if (MaintainKeys)
      {
        assert(Strings->Strings[Index].Pos("=") > 1);
        WriteString(Strings->Names[Index], Strings->Values[Strings->Names[Index]]);
      }
      else
      {
        WriteString(IntToStr(Index), Strings->Strings[Index]);
      }
    }
  }
}
//---------------------------------------------------------------------------
wstring THierarchicalStorage::ReadString(const wstring Name, const wstring Default)
{
  wstring Result;
  if (MungeStringValues)
  {
    Result = UnMungeStr(ReadStringRaw(Name, MungeStr(Default)));
  }
  else
  {
    Result = ReadStringRaw(Name, Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
wstring THierarchicalStorage::ReadBinaryData(const wstring Name)
{
  int Size = BinaryDataSize(Name);
  wstring Value;
  Value.SetLength(Size);
  ReadBinaryData(Name, Value.c_str(), Size);
  return Value;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteString(const wstring Name, const wstring Value)
{
  if (MungeStringValues)
  {
    WriteStringRaw(Name, MungeStr(Value));
  }
  else
  {
    WriteStringRaw(Name, Value);
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteBinaryData(const wstring Name,
  const wstring Value)
{
  WriteBinaryData(Name, Value.c_str(), Value.Length());
}
//---------------------------------------------------------------------------
wstring THierarchicalStorage::IncludeTrailingBackslash(const wstring & S)
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
wstring THierarchicalStorage::ExcludeTrailingBackslash(const wstring & S)
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
TRegistryStorage::TRegistryStorage(const wstring AStorage):
  THierarchicalStorage(IncludeTrailingBackslash(AStorage))
{
  Init();
};
//---------------------------------------------------------------------------
TRegistryStorage::TRegistryStorage(const wstring AStorage, HKEY ARootKey):
  THierarchicalStorage(IncludeTrailingBackslash(AStorage))
{
  Init();
  FRegistry->RootKey = ARootKey;
}
//---------------------------------------------------------------------------
void TRegistryStorage::Init()
{
  FFailed = 0;
  FRegistry = new TRegistry();
  FRegistry->Access = KEY_READ;
}
//---------------------------------------------------------------------------
TRegistryStorage::~TRegistryStorage()
{
  delete FRegistry;
};
//---------------------------------------------------------------------------
bool TRegistryStorage::Copy(TRegistryStorage * Storage)
{
  TRegistry * Registry = Storage->FRegistry;
  bool Result = true;
  TStrings * Names = new TStringList();
  try
  {
    Registry->GetValueNames(Names);
    std::vector<unsigned char> Buffer(1024, 0);
    int Index = 0;
    while ((Index < Names->Count) && Result)
    {
      wstring Name = MungeStr(Names->Strings[Index]);
      unsigned long Size = Buffer.size();
      unsigned long Type;
      int RegResult;
      do
      {
        RegResult = RegQueryValueEx(Registry->CurrentKey, Name.c_str(), NULL,
          &Type, &Buffer[0], &Size);
        if (Result == ERROR_MORE_DATA)
        {
          Buffer.resize(Size);
        }
      } while (RegResult == ERROR_MORE_DATA);

      Result = (RegResult == ERROR_SUCCESS);
      if (Result)
      {
        RegResult = RegSetValueEx(FRegistry->CurrentKey, Name.c_str(), NULL, Type,
          &Buffer[0], Size);
        Result = (RegResult == ERROR_SUCCESS);
      }

      ++Index;
    }
  }
  __finally
  {
    delete Names;
  }
  return Result;
}
//---------------------------------------------------------------------------
wstring TRegistryStorage::GetSource()
{
  return RootKeyToStr(FRegistry->RootKey) + "\\" + Storage;
}
//---------------------------------------------------------------------------
void TRegistryStorage::SetAccessMode(TStorageAccessMode value)
{
  THierarchicalStorage::SetAccessMode(value);
  if (FRegistry)
  {
    switch (AccessMode) {
      case smRead:
        FRegistry->Access = KEY_READ;
        break;

      case smReadWrite:
      default:
        FRegistry->Access = KEY_READ | KEY_WRITE;
        break;
    }
  }
}
//---------------------------------------------------------------------------
bool TRegistryStorage::OpenSubKey(const wstring SubKey, bool CanCreate, bool Path)
{
  bool Result;
  if (FKeyHistory->Count > 0) FRegistry->CloseKey();
  wstring K = ExcludeTrailingBackslash(Storage + CurrentSubKey + MungeSubKey(SubKey, Path));
  Result = FRegistry->OpenKey(K, CanCreate);
  if (Result) Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
  return Result;
}
//---------------------------------------------------------------------------
void TRegistryStorage::CloseSubKey()
{
  FRegistry->CloseKey();
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->Count)
  {
    FRegistry->OpenKey(Storage + CurrentSubKey, true);
  }
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteSubKey(const wstring SubKey)
{
  wstring K;
  if (FKeyHistory->Count == 0) K = Storage + CurrentSubKey;
  K += MungeStr(SubKey);
  return FRegistry->DeleteKey(K);
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetSubKeyNames(TStrings* Strings)
{
  FRegistry->GetKeyNames(Strings);
  for (int Index = 0; Index < Strings->Count; Index++)
  {
    Strings->Strings[Index] = UnMungeStr(Strings->Strings[Index]);
  }
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetValueNames(TStrings* Strings)
{
  FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteValue(const wstring Name)
{
  return FRegistry->DeleteValue(Name);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::KeyExists(const wstring SubKey)
{
  wstring K = MungeStr(SubKey);
  bool Result = FRegistry->KeyExists(K);
  return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::ValueExists(const wstring Value)
{
  bool Result = FRegistry->ValueExists(Value);
  return Result;
}
//---------------------------------------------------------------------------
int TRegistryStorage::BinaryDataSize(const wstring Name)
{
  int Result = FRegistry->GetDataSize(Name);
  return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::ReadBool(const wstring Name, bool Default)
{
  READ_REGISTRY(ReadBool);
}
//---------------------------------------------------------------------------
TDateTime TRegistryStorage::ReadDateTime(const wstring Name, TDateTime Default)
{
  READ_REGISTRY(ReadDateTime);
}
//---------------------------------------------------------------------------
double TRegistryStorage::ReadFloat(const wstring Name, double Default)
{
  READ_REGISTRY(ReadFloat);
}
//---------------------------------------------------------------------------
int TRegistryStorage::ReadInteger(const wstring Name, int Default)
{
  READ_REGISTRY(ReadInteger);
}
//---------------------------------------------------------------------------
__int64 TRegistryStorage::ReadInt64(const wstring Name, __int64 Default)
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
wstring TRegistryStorage::ReadStringRaw(const wstring Name, const wstring Default)
{
  READ_REGISTRY(ReadString);
}
//---------------------------------------------------------------------------
int TRegistryStorage::ReadBinaryData(const wstring Name,
  void * Buffer, int Size)
{
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
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteBool(const wstring Name, bool Value)
{
  WRITE_REGISTRY(WriteBool);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteDateTime(const wstring Name, TDateTime Value)
{
  WRITE_REGISTRY(WriteDateTime);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteFloat(const wstring Name, double Value)
{
  WRITE_REGISTRY(WriteFloat);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteStringRaw(const wstring Name, const wstring Value)
{
  WRITE_REGISTRY(WriteString);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteInteger(const wstring Name, int Value)
{
  WRITE_REGISTRY(WriteInteger);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteInt64(const wstring Name, __int64 Value)
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
void TRegistryStorage::WriteBinaryData(const wstring Name,
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
int TRegistryStorage::GetFailed()
{
  int Result = FFailed;
  FFailed = 0;
  return Result;
}
//===========================================================================
TIniFileStorage::TIniFileStorage(const wstring AStorage):
  THierarchicalStorage(AStorage)
{
  FIniFile = new TMemIniFile(Storage);
  FOriginal = new TStringList();
  FIniFile->GetStrings(FOriginal);
  ApplyOverrides();
}
//---------------------------------------------------------------------------
TIniFileStorage::~TIniFileStorage()
{
  TStrings * Strings = new TStringList;
  try
  {
    FIniFile->GetStrings(Strings);
    if (!Strings->Equals(FOriginal))
    {
      int Attr;
      // preserve attributes (especially hidden)
      if (FileExists(Storage))
      {
        Attr = GetFileAttributes(Storage.c_str());
      }
      else
      {
        Attr = FILE_ATTRIBUTE_NORMAL;
      }

      HANDLE Handle = CreateFile(Storage.c_str(), GENERIC_READ | GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, Attr, 0);

      if (Handle == INVALID_HANDLE_VALUE)
      {
        // "access denied" errors upon implicit saves are ignored
        if (Explicit || (GetLastError() != ERROR_ACCESS_DENIED))
        {
          try
          {
            RaiseLastOSError();
          }
          catch(exception & E)
          {
            throw ExtException(&E, FMTLOAD(CREATE_FILE_ERROR, (Storage)));
          }
        }
      }
      else
      {
        TStream * Stream = new THandleStream(int(Handle));
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
    delete Strings;
    delete FIniFile;
  }
}
//---------------------------------------------------------------------------
wstring TIniFileStorage::GetSource()
{
  return Storage;
}
//---------------------------------------------------------------------------
wstring TIniFileStorage::GetCurrentSection()
{
  return ExcludeTrailingBackslash(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool TIniFileStorage::OpenSubKey(const wstring SubKey, bool CanCreate, bool Path)
{
  bool Result = CanCreate;

  if (!Result)
  {
    TStringList * Sections = new TStringList();
    try
    {
      Sections->Sorted = true;
      FIniFile->ReadSections(Sections);
      wstring NewKey = ExcludeTrailingBackslash(CurrentSubKey+MungeSubKey(SubKey, Path));
      int Index = -1;
      if (Sections->Count)
      {
        Result = Sections->Find(NewKey, Index);
        if (!Result && Index < Sections->Count &&
            Sections->Strings[Index].SubString(1, NewKey.Length()+1) == NewKey + "\\")
        {
          Result = true;
        }
      }
    }
    __finally
    {
      delete Sections;
    }
  }

  if (Result)
  {
    Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TIniFileStorage::DeleteSubKey(const wstring SubKey)
{
  bool Result;
  try
  {
    FIniFile->EraseSection(CurrentSubKey + MungeStr(SubKey));
    Result = true;
  }
  catch (...)
  {
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TIniFileStorage::GetSubKeyNames(TStrings* Strings)
{
  TStrings * Sections = new TStringList();
  try
  {
    Strings->Clear();
    FIniFile->ReadSections(Sections);
    for (int i = 0; i < Sections->Count; i++)
    {
      wstring Section = Sections->Strings[i];
      if (AnsiCompareText(CurrentSubKey,
          Section.SubString(1, CurrentSubKey.Length())) == 0)
      {
        wstring SubSection = Section.SubString(CurrentSubKey.Length() + 1,
          Section.Length() - CurrentSubKey.Length());
        int P = SubSection.Pos("\\");
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
  __finally
  {
    delete Sections;
  }
}
//---------------------------------------------------------------------------
void TIniFileStorage::GetValueNames(TStrings* Strings)
{
  FIniFile->ReadSection(CurrentSection, Strings);
  for (int Index = 0; Index < Strings->Count; Index++)
  {
    Strings->Strings[Index] = UnMungeIniName(Strings->Strings[Index]);
  }
}
//---------------------------------------------------------------------------
bool TIniFileStorage::KeyExists(const wstring SubKey)
{
  return FIniFile->SectionExists(CurrentSubKey + MungeStr(SubKey));
}
//---------------------------------------------------------------------------
bool TIniFileStorage::ValueExists(const wstring Value)
{
  return FIniFile->ValueExists(CurrentSection, MungeIniName(Value));
}
//---------------------------------------------------------------------------
bool TIniFileStorage::DeleteValue(const wstring Name)
{
  FIniFile->DeleteKey(CurrentSection, MungeIniName(Name));
  return true;
}
//---------------------------------------------------------------------------
int TIniFileStorage::BinaryDataSize(const wstring Name)
{
  return ReadStringRaw(Name, "").Length() / 2;
}
//---------------------------------------------------------------------------
void TIniFileStorage::ApplyOverrides()
{
  wstring OverridesKey = IncludeTrailingBackslash("Override");

  TStrings * Sections = new TStringList();
  try
  {
    Sections->Clear();
    FIniFile->ReadSections(Sections);
    for (int i = 0; i < Sections->Count; i++)
    {
      wstring Section = Sections->Strings[i];

      if (AnsiSameText(OverridesKey,
            Section.SubString(1, OverridesKey.Length())))
      {
        wstring SubKey = Section.SubString(OverridesKey.Length() + 1,
          Section.Length() - OverridesKey.Length());

        // this all uses raw names (munged)
        TStrings * Names = new TStringList;
        try
        {
          FIniFile->ReadSection(Section, Names);

          for (int ii = 0; ii < Names->Count; ii++)
          {
            wstring Name = Names->Strings[ii];
            wstring Value = FIniFile->ReadString(Section, Name, "");
            FIniFile->WriteString(SubKey, Name, Value);
          }
        }
        __finally
        {
          delete Names;
        }

        FIniFile->EraseSection(Section);
      }
    }
  }
  __finally
  {
    delete Sections;
  }
}
//---------------------------------------------------------------------------
bool TIniFileStorage::ReadBool(const wstring Name, bool Default)
{
  return FIniFile->ReadBool(CurrentSection, MungeIniName(Name), Default);
}
//---------------------------------------------------------------------------
int TIniFileStorage::ReadInteger(const wstring Name, int Default)
{
  int Result = FIniFile->ReadInteger(CurrentSection, MungeIniName(Name), Default);
  return Result;
}
//---------------------------------------------------------------------------
__int64 TIniFileStorage::ReadInt64(const wstring Name, __int64 Default)
{
  __int64 Result = Default;
  wstring Str;
  Str = ReadStringRaw(Name, "");
  if (!Str.IsEmpty())
  {
    Result = StrToInt64Def(Str, Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime TIniFileStorage::ReadDateTime(const wstring Name, TDateTime Default)
{
  TDateTime Result;
  wstring Value = FIniFile->ReadString(CurrentSection, MungeIniName(Name), "");
  if (Value.IsEmpty())
  {
    Result = Default;
  }
  else
  {
    try
    {
      wstring Raw = HexToStr(Value);
      if (Raw.Length() == sizeof(Result))
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
double TIniFileStorage::ReadFloat(const wstring Name, double Default)
{
  double Result;
  wstring Value = FIniFile->ReadString(CurrentSection, MungeIniName(Name), "");
  if (Value.IsEmpty())
  {
    Result = Default;
  }
  else
  {
    try
    {
      wstring Raw = HexToStr(Value);
      if (Raw.Length() == sizeof(Result))
      {
        memcpy(&Result, Raw.c_str(), sizeof(Result));
      }
      else
      {
        Result = StrToFloat(Value);
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
wstring TIniFileStorage::ReadStringRaw(const wstring Name, wstring Default)
{
  wstring Result = FIniFile->ReadString(CurrentSection, MungeIniName(Name), Default);
  return Result;
}
//---------------------------------------------------------------------------
int TIniFileStorage::ReadBinaryData(const wstring Name,
  void * Buffer, int Size)
{
  wstring Value = HexToStr(ReadStringRaw(Name, ""));
  int Len = Value.Length();
  if (Size > Len)
  {
    Size = Len;
  }
  assert(Buffer);
  memcpy(Buffer, Value.c_str(), Size);
  return Size;
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteBool(const wstring Name, bool Value)
{
  FIniFile->WriteBool(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteInteger(const wstring Name, int Value)
{
  FIniFile->WriteInteger(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteInt64(const wstring Name, __int64 Value)
{
  WriteStringRaw(Name, IntToStr(Value));
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteDateTime(const wstring Name, TDateTime Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteFloat(const wstring Name, double Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteStringRaw(const wstring Name, const wstring Value)
{
  FIniFile->WriteString(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteBinaryData(const wstring Name,
  const void * Buffer, int Size)
{
  WriteStringRaw(Name, StrToHex(wstring(static_cast<const char*>(Buffer), Size)));
}
