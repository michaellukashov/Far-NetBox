#include "stdafx.h"

#include <vector>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "Common.h"
#include "Exceptions.h"
#include "PuttyIntf.h"
#include "Far3Storage.h"
#include "TextsCore.h"

//---------------------------------------------------------------------------
#define READ_REGISTRY(Method) \
  if (FRegistry->ValueExists(Name)) \
  try { return FRegistry->Method(Name); } catch(...) { FFailed++; return Default; } \
  else return Default;
#define WRITE_REGISTRY(Method) \
  try { FRegistry->Method(Name, Value); } catch(...) { FFailed++; }

//---------------------------------------------------------------------------
/*
TFar3Storage::TFar3Storage(const std::wstring &AStorage, HKEY ARootKey) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage))
{
  Init();
  FRegistry->SetRootKey(ARootKey);
}
*/
//---------------------------------------------------------------------------
TFar3Storage::TFar3Storage(const std::wstring &AStorage,
  const GUID &guid, FARAPISETTINGSCONTROL SettingsControl) :
  THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
  FPluginSettings(guid, SettingsControl)
{
  Init();
};
//---------------------------------------------------------------------------
void TFar3Storage::Init()
{
  FFailed = 0;
  FRegistry = new TRegistry;
  FRegistry->SetAccess(KEY_READ);
}
//---------------------------------------------------------------------------
TFar3Storage::~TFar3Storage()
{
  delete FRegistry;
};
//---------------------------------------------------------------------------
bool TFar3Storage::Copy(TFar3Storage * Storage)
{
  TRegistry * Registry = Storage->FRegistry;
  bool Result = true;
  TStrings * Names = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&Names) )
    {
      delete Names;
    } BOOST_SCOPE_EXIT_END
    Registry->GetValueNames(Names);
    std::vector<unsigned char> Buffer(1024, 0);
    size_t Index = 0;
    while ((Index < Names->GetCount()) && Result)
    {
      std::wstring Name = PuttyMungeStr(Names->GetString(Index));
      unsigned long Size = Buffer.size();
      unsigned long Type;
      int RegResult;
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
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TFar3Storage::GetSource()
{
  return RootKeyToStr(FRegistry->GetRootKey()) + L"\\" + GetStorage();
}
//---------------------------------------------------------------------------
void TFar3Storage::SetAccessMode(TStorageAccessMode value)
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
bool TFar3Storage::OpenSubKey(const std::wstring &SubKey, bool CanCreate, bool Path)
{
  // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
  if (FKeyHistory->GetCount() > 0) FRegistry->CloseKey();
  std::wstring K = ExcludeTrailingBackslash(GetStorage() + GetCurrentSubKey() + MungeSubKey(SubKey, Path));
  bool Result = FRegistry->OpenKey(K, CanCreate);
  if (Result) Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path);
  // DEBUG_PRINTF(L"K = %s, Result = %d", K.c_str(), Result);
  return Result;
}
//---------------------------------------------------------------------------
void TFar3Storage::CloseSubKey()
{
  FRegistry->CloseKey();
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->GetCount())
  {
    FRegistry->OpenKey(GetStorage() + GetCurrentSubKey(), true);
  }
}
//---------------------------------------------------------------------------
bool TFar3Storage::DeleteSubKey(const std::wstring &SubKey)
{
  std::wstring K;
  if (FKeyHistory->GetCount() == 0) K = GetStorage() + GetCurrentSubKey();
  K += PuttyMungeStr(SubKey);
  return FRegistry->DeleteKey(K);
}
//---------------------------------------------------------------------------
void TFar3Storage::GetSubKeyNames(TStrings* Strings)
{
  FRegistry->GetKeyNames(Strings);
  for (size_t Index = 0; Index < Strings->GetCount(); Index++)
  {
    Strings->PutString(Index, PuttyUnMungeStr(Strings->GetString(Index)));
  }
}
//---------------------------------------------------------------------------
void TFar3Storage::GetValueNames(TStrings* Strings)
{
  FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool TFar3Storage::DeleteValue(const std::wstring &Name)
{
  return FRegistry->DeleteValue(Name);
}
//---------------------------------------------------------------------------
bool TFar3Storage::KeyExists(const std::wstring &SubKey)
{
  std::wstring K = PuttyMungeStr(SubKey);
  bool Result = FRegistry->KeyExists(K);
  return Result;
}
//---------------------------------------------------------------------------
bool TFar3Storage::ValueExists(const std::wstring &Value)
{
  bool Result = FRegistry->ValueExists(Value);
  return Result;
}
//---------------------------------------------------------------------------
int TFar3Storage::BinaryDataSize(const std::wstring &Name)
{
  int Result = FRegistry->GetDataSize(Name);
  return Result;
}
//---------------------------------------------------------------------------
bool TFar3Storage::Readbool(const std::wstring &Name, bool Default)
{
  READ_REGISTRY(Readbool);
}
//---------------------------------------------------------------------------
TDateTime TFar3Storage::ReadDateTime(const std::wstring &Name, TDateTime Default)
{
  READ_REGISTRY(ReadDateTime);
}
//---------------------------------------------------------------------------
double TFar3Storage::ReadFloat(const std::wstring &Name, double Default)
{
  READ_REGISTRY(ReadFloat);
}
//---------------------------------------------------------------------------
int TFar3Storage::Readint(const std::wstring &Name, int Default)
{
  READ_REGISTRY(Readint);
}
//---------------------------------------------------------------------------
__int64 TFar3Storage::ReadInt64(const std::wstring &Name, __int64 Default)
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
std::wstring TFar3Storage::ReadStringRaw(const std::wstring &Name, const std::wstring &Default)
{
  READ_REGISTRY(ReadString);
}
//---------------------------------------------------------------------------
int TFar3Storage::ReadBinaryData(const std::wstring &Name,
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
void TFar3Storage::Writebool(const std::wstring &Name, bool Value)
{
  WRITE_REGISTRY(Writebool);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteDateTime(const std::wstring &Name, TDateTime Value)
{
  WRITE_REGISTRY(WriteDateTime);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteFloat(const std::wstring &Name, double Value)
{
  WRITE_REGISTRY(WriteFloat);
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteStringRaw(const std::wstring &Name, const std::wstring &Value)
{
  WRITE_REGISTRY(WriteString);
}
//---------------------------------------------------------------------------
void TFar3Storage::Writeint(const std::wstring &Name, int Value)
{
  // DEBUG_PRINTF(L"GetFailed = %d", GetFailed());
  WRITE_REGISTRY(Writeint);
  // DEBUG_PRINTF(L"GetFailed = %d", GetFailed());
}
//---------------------------------------------------------------------------
void TFar3Storage::WriteInt64(const std::wstring &Name, __int64 Value)
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
void TFar3Storage::WriteBinaryData(const std::wstring &Name,
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
int TFar3Storage::GetFailed()
{
  int Result = FFailed;
  FFailed = 0;
  return Result;
}
