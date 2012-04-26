//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

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
UnicodeString MungeStr(const UnicodeString Str)
{
    std::string Result2;
    Result2.SetLength(Str.Length() * sizeof(wchar_t) * 3 + 1);
    putty_mungestr(System::W2MB(Str.c_str()).c_str(), const_cast<char *>(Result2.c_str()));
    UnicodeString Result = System::MB2W(Result2.c_str());
    PackStr(Result);
    // DEBUG_PRINTF(L"Str = %s, Result = %s", Str.c_str(), Result.c_str());
    return Result;
}
//---------------------------------------------------------------------------
UnicodeString UnMungeStr(const UnicodeString Str)
{
    std::string Result2;
    Result2.SetLength(Str.Length() * sizeof(wchar_t) * 3 + 1);
    putty_unmungestr(const_cast<char *>(System::W2MB(Str.c_str()).c_str()), const_cast<char *>(Result2.c_str()), static_cast<int>(Result2.Length()));
    UnicodeString Result = System::MB2W(Result2.c_str());
    PackStr(Result);
    return Result;
}
//---------------------------------------------------------------------------
UnicodeString PuttyMungeStr(const UnicodeString Str)
{
    return MungeStr(Str);
}
//---------------------------------------------------------------------------
UnicodeString PuttyUnMungeStr(const UnicodeString Str)
{
    return UnMungeStr(Str);
}
//---------------------------------------------------------------------------
UnicodeString MungeIniName(const UnicodeString Str)
{
    size_t P = Str.find_first_of(L"=");
    // make this fast for now
    if (P > 0)
    {
        return ::StringReplace(Str, L"=", L"%3D");
    }
    else
    {
        return Str;
    }
}
//---------------------------------------------------------------------------
UnicodeString UnMungeIniName(const UnicodeString Str)
{
    size_t P = Str.Pos(L"%3D");
    // make this fast for now
    if (P != UnicodeString::npos)
    {
        return ::StringReplace(Str, L"%3D", L"=");
    }
    else
    {
        return Str;
    }
}
//===========================================================================
THierarchicalStorage::THierarchicalStorage(const UnicodeString AStorage)
{
    FStorage = AStorage;
    FKeyHistory = new System::TStringList();
    SetAccessMode(smRead);
    SetExplicit(false);
    SetMungeStringValues(true);
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
UnicodeString THierarchicalStorage::GetCurrentSubKeyMunged()
{
    if (FKeyHistory->GetCount()) { return FKeyHistory->GetString(FKeyHistory->GetCount()-1); }
    else { return L""; }
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::GetCurrentSubKey()
{
    return UnMungeStr(GetCurrentSubKeyMunged());
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::OpenRootKey(bool CanCreate)
{
    return OpenSubKey(L"", CanCreate);
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::MungeSubKey(const UnicodeString Key, bool Path)
{
    UnicodeString Result;
    // DEBUG_PRINTF(L"Key = %s, Path = %d", Key.c_str(), Path);
    UnicodeString key = Key;
    if (Path)
    {
        assert(key.IsEmpty() || (key[key.Length() - 1] != '\\'));
        while (!key.IsEmpty())
        {
            if (!Result.IsEmpty())
            {
                Result += '\\';
            }
            Result += MungeStr(CutToChar(key, L'\\', false));
            // DEBUG_PRINTF(L"key = %s, Result = %s", key.c_str(), Result.c_str());
        }
    }
    else
    {
        Result = MungeStr(key);
    }
    // DEBUG_PRINTF(L"Result = %s", Result.c_str());
    return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::OpenSubKey(const UnicodeString SubKey, bool /*CanCreate*/, bool Path)
{
    // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
    FKeyHistory->Add(IncludeTrailingBackslash(GetCurrentSubKey() + MungeSubKey(SubKey, Path)));
    return true;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::CloseSubKey()
{
    if (FKeyHistory->GetCount() == 0)
    {
        throw std::exception("");
    }
    else { FKeyHistory->Delete(FKeyHistory->GetCount()-1); }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ClearSubKeys()
{
    System::TStringList *SubKeys = new System::TStringList();
    {
        BOOST_SCOPE_EXIT ( (&SubKeys) )
        {
            delete SubKeys;
        } BOOST_SCOPE_EXIT_END
        GetSubKeyNames(SubKeys);
        for (size_t Index = 0; Index < SubKeys->GetCount(); Index++)
        {
            RecursiveDeleteSubKey(SubKeys->GetString(Index));
        }
    }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::RecursiveDeleteSubKey(const UnicodeString Key)
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
    System::TStrings *SubKeys = new System::TStringList();
    {
        BOOST_SCOPE_EXIT ( (&SubKeys) )
        {
            delete SubKeys;
        } BOOST_SCOPE_EXIT_END
        GetSubKeyNames(SubKeys);
        Result = (SubKeys->GetCount() > 0);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool THierarchicalStorage::HasSubKey(const UnicodeString SubKey)
{
    bool Result = OpenSubKey(SubKey, false);
    if (Result)
    {
        CloseSubKey();
    }
    return Result;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ReadValues(System::TStrings *Strings,
                                      bool MaintainKeys)
{
    System::TStrings *Names = new System::TStringList();
    {
        BOOST_SCOPE_EXIT ( (&Names) )
        {
            delete Names;
        } BOOST_SCOPE_EXIT_END
        GetValueNames(Names);
        for (size_t Index = 0; Index < Names->GetCount(); Index++)
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
}
//---------------------------------------------------------------------------
void THierarchicalStorage::ClearValues()
{
    System::TStrings *Names = new System::TStringList();
    {
        BOOST_SCOPE_EXIT ( (&Names) )
        {
            delete Names;
        } BOOST_SCOPE_EXIT_END
        GetValueNames(Names);
        for (size_t Index = 0; Index < Names->GetCount(); Index++)
        {
            DeleteValue(Names->GetString(Index));
        }
    }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteValues(System::TStrings *Strings,
                                       bool MaintainKeys)
{
    ClearValues();

    if (Strings)
    {
        for (size_t Index = 0; Index < Strings->GetCount(); Index++)
        {
            if (MaintainKeys)
            {
                assert(Strings->GetString(Index).find_first_of(L"=") > 1);
                WriteString(Strings->GetName(Index), Strings->GetValue(Strings->GetName(Index)));
            }
            else
            {
                WriteString(IntToStr(static_cast<int>(Index)), Strings->GetString(Index));
            }
        }
    }
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::ReadString(const UnicodeString Name, const UnicodeString Default)
{
    UnicodeString Result;
    if (GetMungeStringValues())
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
UnicodeString THierarchicalStorage::ReadBinaryData(const UnicodeString Name)
{
    size_t Size = static_cast<size_t>(BinaryDataSize(Name));
    UnicodeString Value(Size, 0);
    ReadBinaryData(Name, const_cast<wchar_t *>(Value.c_str()), Size);
    return Value;
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteString(const UnicodeString Name, const UnicodeString Value)
{
    // DEBUG_PRINTF(L"GetMungeStringValues = %d", GetMungeStringValues());
    if (GetMungeStringValues())
    {
        // DEBUG_PRINTF(L"Name = %s, Value = %s, MungeStr(Value) = %s", Name.c_str(), Value.c_str(), MungeStr(Value).c_str());
        WriteStringRaw(Name, MungeStr(Value));
    }
    else
    {
        // DEBUG_PRINTF(L"Value = %s", Value.c_str());
        WriteStringRaw(Name, Value);
    }
}
//---------------------------------------------------------------------------
void THierarchicalStorage::WriteBinaryData(const UnicodeString Name,
        const UnicodeString Value)
{
    WriteBinaryData(Name, Value.c_str(), Value.Length());
}
//---------------------------------------------------------------------------
UnicodeString THierarchicalStorage::IncludeTrailingBackslash(const UnicodeString S)
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
UnicodeString THierarchicalStorage::ExcludeTrailingBackslash(const UnicodeString S)
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
TRegistryStorage::TRegistryStorage(const UnicodeString AStorage) :
    THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
    FRegistry(NULL)
{
    Init();
};
//---------------------------------------------------------------------------
TRegistryStorage::TRegistryStorage(const UnicodeString AStorage, HKEY ARootKey) :
    THierarchicalStorage(IncludeTrailingBackslash(AStorage)),
    FRegistry(NULL)
{
    Init();
    FRegistry->SetRootKey(ARootKey);
}
//---------------------------------------------------------------------------
void TRegistryStorage::Init()
{
    FFailed = 0;
    FRegistry = new System::TRegistry;
    FRegistry->SetAccess(KEY_READ);
}
//---------------------------------------------------------------------------
TRegistryStorage::~TRegistryStorage()
{
    delete FRegistry;
};
//---------------------------------------------------------------------------
bool TRegistryStorage::Copy(TRegistryStorage *Storage)
{
    System::TRegistry *Registry = Storage->FRegistry;
    bool Result = true;
    System::TStrings *Names = new System::TStringList();
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
            UnicodeString Name = MungeStr(Names->GetString(Index));
            DWORD Size = static_cast<DWORD>(Buffer.Length());
            unsigned long Type;
            int RegResult;
            do
            {
                RegResult = RegQueryValueEx(Registry->GetCurrentKey(), Name.c_str(), NULL,
                                            &Type, &Buffer[0], &Size);
                if (RegResult == ERROR_MORE_DATA)
                {
                    Buffer.SetLength(static_cast<size_t>(Size));
                }
            }
            while (RegResult == ERROR_MORE_DATA);

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
UnicodeString TRegistryStorage::GetSource()
{
    return RootKeyToStr(FRegistry->GetRootKey()) + L"\\" + GetStorage();
}
//---------------------------------------------------------------------------
void TRegistryStorage::SetAccessMode(TStorageAccessMode value)
{
    THierarchicalStorage::SetAccessMode(value);
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
//---------------------------------------------------------------------------
bool TRegistryStorage::OpenSubKey(const UnicodeString SubKey, bool CanCreate, bool Path)
{
    // DEBUG_PRINTF(L"SubKey = %s", SubKey.c_str());
    if (FKeyHistory->GetCount() > 0) { FRegistry->CloseKey(); }
    UnicodeString K = ExcludeTrailingBackslash(GetStorage() + GetCurrentSubKey() + MungeSubKey(SubKey, Path));
    bool Result = FRegistry->OpenKey(K, CanCreate);
    if (Result) { Result = THierarchicalStorage::OpenSubKey(SubKey, CanCreate, Path); }
    // DEBUG_PRINTF(L"K = %s, Result = %d", K.c_str(), Result);
    return Result;
}
//---------------------------------------------------------------------------
void TRegistryStorage::CloseSubKey()
{
    FRegistry->CloseKey();
    THierarchicalStorage::CloseSubKey();
    if (FKeyHistory->GetCount())
    {
        FRegistry->OpenKey(GetStorage() + GetCurrentSubKey(), true);
    }
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteSubKey(const UnicodeString SubKey)
{
    UnicodeString K;
    if (FKeyHistory->GetCount() == 0) { K = GetStorage() + GetCurrentSubKey(); }
    K += MungeStr(SubKey);
    return FRegistry->DeleteKey(K);
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetSubKeyNames(System::TStrings *Strings)
{
    FRegistry->GetKeyNames(Strings);
    for (size_t Index = 0; Index < Strings->GetCount(); Index++)
    {
        Strings->PutString(Index, UnMungeStr(Strings->GetString(Index)));
    }
}
//---------------------------------------------------------------------------
void TRegistryStorage::GetValueNames(System::TStrings *Strings)
{
    FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::DeleteValue(const UnicodeString Name)
{
    return FRegistry->DeleteValue(Name);
}
//---------------------------------------------------------------------------
bool TRegistryStorage::KeyExists(const UnicodeString SubKey)
{
    UnicodeString K = MungeStr(SubKey);
    bool Result = FRegistry->KeyExists(K);
    return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::ValueExists(const UnicodeString Value)
{
    bool Result = FRegistry->ValueExists(Value);
    return Result;
}
//---------------------------------------------------------------------------
int TRegistryStorage::BinaryDataSize(const UnicodeString Name)
{
    int Result = FRegistry->GetDataSize(Name);
    return Result;
}
//---------------------------------------------------------------------------
bool TRegistryStorage::Readbool(const UnicodeString Name, bool Default)
{
    READ_REGISTRY(Readbool);
}
//---------------------------------------------------------------------------
System::TDateTime TRegistryStorage::ReadDateTime(const UnicodeString Name, System::TDateTime Default)
{
    READ_REGISTRY(ReadDateTime);
}
//---------------------------------------------------------------------------
double TRegistryStorage::ReadFloat(const UnicodeString Name, double Default)
{
    READ_REGISTRY(ReadFloat);
}
//---------------------------------------------------------------------------
int TRegistryStorage::Readint(const UnicodeString Name, int Default)
{
    READ_REGISTRY(Readint);
}
//---------------------------------------------------------------------------
__int64 TRegistryStorage::ReadInt64(const UnicodeString Name, __int64 Default)
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
UnicodeString TRegistryStorage::ReadStringRaw(const UnicodeString Name, const UnicodeString Default)
{
    READ_REGISTRY(ReadString);
}
//---------------------------------------------------------------------------
size_t TRegistryStorage::ReadBinaryData(const UnicodeString Name,
                                     void *Buffer, size_t Size)
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
void TRegistryStorage::Writebool(const UnicodeString Name, bool Value)
{
    WRITE_REGISTRY(Writebool);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteDateTime(const UnicodeString Name, System::TDateTime Value)
{
    WRITE_REGISTRY(WriteDateTime);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteFloat(const UnicodeString Name, double Value)
{
    WRITE_REGISTRY(WriteFloat);
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteStringRaw(const UnicodeString Name, const UnicodeString Value)
{
    WRITE_REGISTRY(WriteString);
}
//---------------------------------------------------------------------------
void TRegistryStorage::Writeint(const UnicodeString Name, int Value)
{
    // DEBUG_PRINTF(L"GetFailed = %d", GetFailed());
    WRITE_REGISTRY(Writeint);
    // DEBUG_PRINTF(L"GetFailed = %d", GetFailed());
}
//---------------------------------------------------------------------------
void TRegistryStorage::WriteInt64(const UnicodeString Name, __int64 Value)
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
void TRegistryStorage::WriteBinaryData(const UnicodeString Name,
                                       const void *Buffer, size_t Size)
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
TOptionsStorage::TOptionsStorage(System::TStrings * Options) :
  TRegistryStorage(UnicodeString(L"Command-line options")) // , new TOptionsIniFile(Options))
{
}
