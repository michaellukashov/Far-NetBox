//---------------------------------------------------------------------------
#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "HierarchicalStorage.h"
#include <TextsCore.h>
#include <vector>
//---------------------------------------------------------------------------
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch(...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch(...) { FFailed++; }
//---------------------------------------------------------------------------
std::wstring MungeStr(const std::wstring Str)
{
  std::wstring Result;
  Result.resize(Str.size() * 3 + 1);
  putty_mungestr(Str.c_str(), Result.c_str());
  PackStr(Result);
  return Result;
}
//---------------------------------------------------------------------------
std::wstring UnMungeStr(const std::wstring Str)
{
  std::wstring Result;
  Result.resize(Str.size() * 3 + 1);
  putty_unmungestr(Str.c_str(), Result.c_str(), Result.size());
  PackStr(Result);
  return Result;
}
//---------------------------------------------------------------------------
std::wstring PuttyMungeStr(const std::wstring Str)
{
  return MungeStr(Str);
}
//---------------------------------------------------------------------------
std::wstring MungeIniName(const std::wstring Str)
{
  int P = Str.find_first_of(L"=");
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
std::wstring UnMungeIniName(const std::wstring Str)
{
  int P = Str.find_first_of(L"%3D");
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
THierarchicalStorage::THierarchicalStorage(const std::wstring AStorage)
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
std::wstring THierarchicalStorage::GetCurrentSubKeyMunged()
{
  if (FKeyHistory->GetCount()) return FKeyHistory->GetString(FKeyHistory->GetCount()-1];
    else return "";
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::GetCurrentSubKey()
{
  return UnMungeStr(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::OpenRootKey(bool CanCreate)
{
  return OpenSubKey("", CanCreate);
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::MungeSubKey(std::wstring Key, bool Path)
{
  std::wstring Result;
  if (Path)
  {
    assert(Key.empty() || (Key[Key.size()] != '\\'));
    while (!Key.empty())
    {
      if (!Result.empty())
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
bool THierarchicalStorage::OpenSubKey(const std::wstring SubKey, bool /*CanCreate*/, bool Path)
{
  FKeyHistory->Add(IncludeTrailingBackslash(CurrentSubKey+MungeSubKey(SubKey, Path)));
  return true;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::CloseSubKey()
{
  if (FKeyHistory->GetCount() == 0) throw exception("");
    else FKeyHistory->Delete(FKeyHistory->GetCount()-1);
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ClearSubKeys()
{
  TStringList *SubKeys = new TStringList();
  try
  {
    GetSubKeyNames(SubKeys);
    for (int Index = 0; Index < SubKeys->GetCount(); Index++)
    {
      RecursiveDeleteSubKey(SubKeys->GetString(Index));
    }
  }
  catch(...)
  {
    delete SubKeys;
  }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::RecursiveDeleteSubKey(const std::wstring Key)
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
    Result = (SubKeys->GetCount() > 0);
  }
  catch(...)
  {
    delete SubKeys;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::HasSubKey(const std::wstring SubKey)
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
    for (int Index = 0; Index < Names->GetCount(); Index++)
    {
      if (MaintainKeys)
      {
        Strings->Add(::FORMAT(L"%s=%s", (Names->GetString(Index),
          ReadString(Names->GetString(Index), ""))));
      }
      else
      {
        Strings->Add(ReadString(Names->GetString(Index), ""));
      }
    }
  }
  catch(...)
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
    for (int Index = 0; Index < Names->GetCount(); Index++)
    {
      DeleteValue(Names->GetString(Index));
    }
  }
  catch(...)
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
    for (int Index = 0; Index < Strings->GetCount(); Index++)
    {
      if (MaintainKeys)
      {
        assert(Strings->GetString(Index).find_first_of(L"=") > 1);
        WriteString(Strings->Names[Index], Strings->Values[Strings->Names[Index]]);
      }
      else
      {
        WriteString(IntToStr(Index), Strings->GetString(Index));
      }
    }
  }
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::ReadString(const std::wstring Name, const std::wstring Default)
{
  std::wstring Result;
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
std::wstring THierarchicalStorage::ReadBinaryData(const std::wstring Name)
{
  int Size = BinaryDataSize(Name);
  std::wstring Value;
  Value.resize(Size);
  ReadBinaryData(Name, Value.c_str(), Size);
  return Value;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteString(const std::wstring Name, const std::wstring Value)
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
void THierarchicalStorage::WriteBinaryData(const std::wstring Name,
  const std::wstring Value)
{
  WriteBinaryData(Name, Value.c_str(), Value.size());
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::IncludeTrailingBackslash(const std::wstring & S)
{
  // expanded from ?: as it caused memory leaks
  if (S.empty())
  {
    return S;
  }
  else
  {
    return ::IncludeTrailingBackslash(S);
  }
}
//---------------------------------------------------------------------------
std::wstring THierarchicalStorage::ExcludeTrailingBackslash(const std::wstring & S)
{
  // expanded from ?: as it caused memory leaks
  if (S.empty())
  {
    return S;
  }
  else
  {
    return ::ExcludeTrailingBackslash(S);
  }
}
//===========================================================================
TRegistryStorage::TRegistryStorage(const std::wstring AStorage):
  THierarchicalStorage(IncludeTrailingBackslash(AStorage))
{
  Init();
};
//---------------------------------------------------------------------------
TRegistryStorage::TRegistryStorage(const std::wstring AStorage, HKEY ARootKey):
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
    while ((Index < Names->GetCount()) && Result)
    {
      std::wstring Name = MungeStr(Names->GetString(Index));
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
  catch(...)
  {
    delete Names;
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TRegistryStorage::GetSource()
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
bool TRegistryStorage::OpenSubKey(const std::wstring SubKey, bool CanCreate, bool Path)
{
  bool Result;
  if (FKeyHistory->GetCount() > 0) FRegistry->CloseKey();
  std::wstring K = ExcludeTrailingBackslash(Storage + CurrentSubKey + MungeSubKey(SubKey, Path));
  Result = FRegistry->OpenKey(K, CanCreate);
  if (Result) Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
  return Result;
}
//---------------------------------------------------------------------------
void TRegistryStorage::CloseSubKey()
{
  FRegistry->CloseKey();
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount())
  {
    FRegistry->OpenKey(Storage + CurrentSubKey, true);
  }
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteSubKey(const std::wstring SubKey)
{
  std::wstring K;
  if (FKeyHistory->GetCount() == 0) K = Storage + CurrentSubKey;
  K += MungeStr(SubKey);
  return FRegistry->DeleteKey(K);
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetSubKeyNames(TStrings* Strings)
{
  FRegistry->GetKeyNames(Strings);
  for (int Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->GetString(Index) = UnMungeStr(Strings->GetString(Index));
  }
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetValueNames(TStrings* Strings)
{
  FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteValue(const std::wstring Name)
{
  return FRegistry->DeleteValue(Name);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::KeyExists(const std::wstring SubKey)
{
  std::wstring K = MungeStr(SubKey);
  bool Result = FRegistry->KeyExists(K);
  return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::ValueExists(const std::wstring Value)
{
  bool Result = FRegistry->ValueExists(Value);
  return Result;
}
//---------------------------------------------------------------------------
int TRegistryStorage::BinaryDataSize(const std::wstring Name)
{
  int Result = FRegistry->GetDataSize(Name);
  return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::ReadBool(const std::wstring Name, bool Default)
{
  READ_REGISTRY(ReadBool);
}
//---------------------------------------------------------------------------
TDateTime TRegistryStorage::ReadDateTime(const std::wstring Name, TDateTime Default)
{
  READ_REGISTRY(ReadDateTime);
}
//---------------------------------------------------------------------------
double TRegistryStorage::ReadFloat(const std::wstring Name, double Default)
{
  READ_REGISTRY(ReadFloat);
}
//---------------------------------------------------------------------------
int TRegistryStorage::ReadInteger(const std::wstring Name, int Default)
{
  READ_REGISTRY(ReadInteger);
}
//---------------------------------------------------------------------------
__int64 TRegistryStorage::ReadInt64(const std::wstring Name, __int64 Default)
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
std::wstring TRegistryStorage::ReadStringRaw(const std::wstring Name, const std::wstring Default)
{
  READ_REGISTRY(ReadString);
}
//---------------------------------------------------------------------------
int TRegistryStorage::ReadBinaryData(const std::wstring Name,
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
void TRegistryStorage::Writebool(const std::wstring Name, bool Value)
{
  WRITE_REGISTRY(Writebool);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteDateTime(const std::wstring Name, TDateTime Value)
{
  WRITE_REGISTRY(WriteDateTime);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteFloat(const std::wstring Name, double Value)
{
  WRITE_REGISTRY(WriteFloat);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteStringRaw(const std::wstring Name, const std::wstring Value)
{
  WRITE_REGISTRY(WriteString);
}
//---------------------------------------------------------------------------
void TRegistryStorage::Writeint(const std::wstring Name, int Value)
{
  WRITE_REGISTRY(Writeint);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteInt64(const std::wstring Name, __int64 Value)
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
void TRegistryStorage::WriteBinaryData(const std::wstring Name,
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
TIniFileStorage::TIniFileStorage(const std::wstring AStorage):
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
        catch(...)
        {
          CloseHandle(Handle);
          delete Stream;
        }
      }
    }
  }
  catch(...)
  {
    delete FOriginal;
    delete Strings;
    delete FIniFile;
  }
}
//---------------------------------------------------------------------------
std::wstring TIniFileStorage::GetSource()
{
  return Storage;
}
//---------------------------------------------------------------------------
std::wstring TIniFileStorage::GetCurrentSection()
{
  return ExcludeTrailingBackslash(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool TIniFileStorage::OpenSubKey(const std::wstring SubKey, bool CanCreate, bool Path)
{
  bool Result = CanCreate;

  if (!Result)
  {
    TStringList * Sections = new TStringList();
    try
    {
      Sections->Sorted = true;
      FIniFile->ReadSections(Sections);
      std::wstring NewKey = ExcludeTrailingBackslash(CurrentSubKey+MungeSubKey(SubKey, Path));
      int Index = -1;
      if (Sections->GetCount())
      {
        Result = Sections->Find(NewKey, Index);
        if (!Result && Index < Sections->GetCount() &&
            Sections->GetString(Index).substr(1, NewKey.size()+1) == NewKey + "\\")
        {
          Result = true;
        }
      }
    }
    catch(...)
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
bool TIniFileStorage::DeleteSubKey(const std::wstring SubKey)
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
    for (int i = 0; i < Sections->GetCount(); i++)
    {
      std::wstring Section = Sections->GetString(i);
      if (AnsiCompareText(CurrentSubKey,
          Section.substr(1, CurrentSubKey.size())) == 0)
      {
        std::wstring SubSection = Section.substr(CurrentSubKey.size() + 1,
          Section.size() - CurrentSubKey.size());
        int P = SubSection.find_first_of(L"\\");
        if (P)
        {
          SubSection.resize(P - 1);
        }
        if (Strings->IndexOf(SubSection) < 0)
        {
          Strings->Add(UnMungeStr(SubSection));
        }
      }
    }
  }
  catch(...)
  {
    delete Sections;
  }
}
//---------------------------------------------------------------------------
void TIniFileStorage::GetValueNames(TStrings* Strings)
{
  FIniFile->ReadSection(CurrentSection, Strings);
  for (int Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->GetString(Index) = UnMungeIniName(Strings->GetString(Index));
  }
}
//---------------------------------------------------------------------------
bool TIniFileStorage::KeyExists(const std::wstring SubKey)
{
  return FIniFile->SectionExists(CurrentSubKey + MungeStr(SubKey));
}
//---------------------------------------------------------------------------
bool TIniFileStorage::ValueExists(const std::wstring Value)
{
  return FIniFile->ValueExists(CurrentSection, MungeIniName(Value));
}
//---------------------------------------------------------------------------
bool TIniFileStorage::DeleteValue(const std::wstring Name)
{
  FIniFile->DeleteKey(CurrentSection, MungeIniName(Name));
  return true;
}
//---------------------------------------------------------------------------
int TIniFileStorage::BinaryDataSize(const std::wstring Name)
{
  return ReadStringRaw(Name, "").size() / 2;
}
//---------------------------------------------------------------------------
void TIniFileStorage::ApplyOverrides()
{
  std::wstring OverridesKey = IncludeTrailingBackslash("Override");

  TStrings * Sections = new TStringList();
  try
  {
    Sections->Clear();
    FIniFile->ReadSections(Sections);
    for (int i = 0; i < Sections->GetCount(); i++)
    {
      std::wstring Section = Sections->GetString(i);

      if (AnsiSameText(OverridesKey,
            Section.substr(1, OverridesKey.size())))
      {
        std::wstring SubKey = Section.substr(OverridesKey.size() + 1,
          Section.size() - OverridesKey.size());

        // this all uses raw names (munged)
        TStrings * Names = new TStringList;
        try
        {
          FIniFile->ReadSection(Section, Names);

          for (int ii = 0; ii < Names->GetCount(); ii++)
          {
            std::wstring Name = Names->GetString(ii];
            std::wstring Value = FIniFile->ReadString(Section, Name, "");
            FIniFile->WriteString(SubKey, Name, Value);
          }
        }
        catch(...)
        {
          delete Names;
        }

        FIniFile->EraseSection(Section);
      }
    }
  }
  catch(...)
  {
    delete Sections;
  }
}
//---------------------------------------------------------------------------
bool TIniFileStorage::ReadBool(const std::wstring Name, bool Default)
{
  return FIniFile->ReadBool(CurrentSection, MungeIniName(Name), Default);
}
//---------------------------------------------------------------------------
int TIniFileStorage::ReadInteger(const std::wstring Name, int Default)
{
  int Result = FIniFile->ReadInteger(CurrentSection, MungeIniName(Name), Default);
  return Result;
}
//---------------------------------------------------------------------------
__int64 TIniFileStorage::ReadInt64(const std::wstring Name, __int64 Default)
{
  __int64 Result = Default;
  std::wstring Str;
  Str = ReadStringRaw(Name, "");
  if (!Str.empty())
  {
    Result = StrToInt64Def(Str, Default);
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime TIniFileStorage::ReadDateTime(const std::wstring Name, TDateTime Default)
{
  TDateTime Result;
  std::wstring Value = FIniFile->ReadString(CurrentSection, MungeIniName(Name), "");
  if (Value.empty())
  {
    Result = Default;
  }
  else
  {
    try
    {
      std::wstring Raw = HexToStr(Value);
      if (Raw.size() == sizeof(Result))
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
double TIniFileStorage::ReadFloat(const std::wstring Name, double Default)
{
  double Result;
  std::wstring Value = FIniFile->ReadString(CurrentSection, MungeIniName(Name), "");
  if (Value.empty())
  {
    Result = Default;
  }
  else
  {
    try
    {
      std::wstring Raw = HexToStr(Value);
      if (Raw.size() == sizeof(Result))
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
std::wstring TIniFileStorage::ReadStringRaw(const std::wstring Name, std::wstring Default)
{
  std::wstring Result = FIniFile->ReadString(CurrentSection, MungeIniName(Name), Default);
  return Result;
}
//---------------------------------------------------------------------------
int TIniFileStorage::ReadBinaryData(const std::wstring Name,
  void * Buffer, int Size)
{
  std::wstring Value = HexToStr(ReadStringRaw(Name, ""));
  int Len = Value.size();
  if (Size > Len)
  {
    Size = Len;
  }
  assert(Buffer);
  memcpy(Buffer, Value.c_str(), Size);
  return Size;
}
//---------------------------------------------------------------------------
void TIniFileStorage::Writebool(const std::wstring Name, bool Value)
{
  FIniFile->Writebool(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TIniFileStorage::Writeint(const std::wstring Name, int Value)
{
  FIniFile->Writeint(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteInt64(const std::wstring Name, __int64 Value)
{
  WriteStringRaw(Name, IntToStr(Value));
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteDateTime(const std::wstring Name, TDateTime Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteFloat(const std::wstring Name, double Value)
{
  WriteBinaryData(Name, &Value, sizeof(Value));
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteStringRaw(const std::wstring Name, const std::wstring Value)
{
  FIniFile->WriteString(CurrentSection, MungeIniName(Name), Value);
}
//---------------------------------------------------------------------------
void TIniFileStorage::WriteBinaryData(const std::wstring Name,
  const void * Buffer, int Size)
{
  WriteStringRaw(Name, StrToHex(std::wstring(static_cast<const char*>(Buffer), Size)));
}
