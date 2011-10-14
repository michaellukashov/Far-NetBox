#pragma once

#include "stdafx.h"
#include <ShellAPI.h>

#include "boostdefines.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>

#include "Classes.h"
#include "Common.h"

namespace alg = boost::algorithm;

//---------------------------------------------------------------------------
const std::wstring sLineBreak = L"\n";

//---------------------------------------------------------------------------

void TStrings::SetTextStr(const std::wstring Text)
{
    TStrings *Self = this;
    Self->BeginUpdate();
    BOOST_SCOPE_EXIT( (&Self) )
    {
        Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    Clear();
    const wchar_t *P = Text.c_str();
    if (P != NULL)
    {
      while (*P != 0x00)
      {
        const wchar_t *Start = P;
        while (!((*P == 0x00) || (*P == 0x0A) || (*P == 0x0D)))
        {
            P++;
        }
        std::wstring S;
        S.resize(P - Start + 1);
        memcpy((wchar_t *)S.c_str(), Start, (P - Start) * sizeof(wchar_t));
        Add(S);
        if (*P == 0x0D) P++;
        if (*P == 0x0A) P++;
      };
    }
}

std::wstring TStrings::GetCommaText() const
{
    wchar_t LOldDelimiter = GetDelimiter();
    wchar_t LOldQuoteChar = GetQuoteChar();
    FDelimiter = L',';
    FQuoteChar = L'"';

    BOOST_SCOPE_EXIT( (&FDelimiter) (&FQuoteChar) (LOldDelimiter) (LOldQuoteChar) )
    {
        FDelimiter = LOldDelimiter;
        FQuoteChar = LOldQuoteChar;
    } BOOST_SCOPE_EXIT_END

    std::wstring Result = GetDelimitedText();
    return Result;
}
std::wstring TStrings::GetDelimitedText() const
{
    std::wstring Result;
    int Count = GetCount();
    if ((Count == 1) && GetString(0).empty())
    {
      Result = GetQuoteChar() + GetQuoteChar();
    }
    else
    {
        for (int i = 0; i < GetCount(); i++)
        {
            std::wstring line = GetString(i);
            Result += GetQuoteChar() + line + GetQuoteChar() + GetDelimiter();
        }
        Result.resize(Result.size() - 1);
    }
    return Result;
}
void TStrings::SetDelimitedText(const std::wstring Value)
{
    TStrings *Self = this;
    Self->BeginUpdate();
    BOOST_SCOPE_EXIT( (&Self) )
    {
        Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    Clear();
    std::vector<std::wstring> lines;
    std::wstring delim = std::wstring(1, GetDelimiter());
    delim.append(1, L'\n');
    alg::split(lines, Value, alg::is_any_of(delim), alg::token_compress_on);
    std::wstring line;
    // for (std::vector<std::wstring>::const_iterator it = lines.begin(); it != lines.end(); ++it)
    BOOST_FOREACH(line, lines)
    {
        Add(line);
    }
}

int TStrings::CompareStrings(const std::wstring &S1, const std::wstring &S2)
{
    return ::AnsiCompareText(S1, S2);
}

//---------------------------------------------------------------------------
int StringListCompareStrings(TStringList *List, int Index1, int Index2)
{
  int Result = List->CompareStrings(List->FList[Index1].FString,
    List->FList[Index2].FString);
  return Result;
}

void TStringList::Sort()
{
    CustomSort(StringListCompareStrings);
}
void TStringList::CustomSort(TStringListSortCompare CompareFunc)
{
  if (!GetSorted() && (GetCount() > 1))
  {
    Changing();
    QuickSort(0, GetCount() - 1, CompareFunc);
    Changed();
  }
}
void TStringList::QuickSort(int L, int R, TStringListSortCompare SCompare)
{
  int I, J, P;
  do
  {
    I = L;
    J = R;
    P = (L + R) >> 1;
    // DEBUG_PRINTF(L"L = %d, R = %d, P = %d", L, R, P);
    do
    {
      while (SCompare(this, I, P) < 0) I++;
      while (SCompare(this, J, P) > 0) J--;
      // DEBUG_PRINTF(L"I = %d, J = %d, P = %d", I, J, P);
      if (I <= J)
      {
        ExchangeItems(I, J);
        if (P == I)
          P = J;
        else if (P == J)
          P = I;
        I++;
        J--;
      }
    } while (I <= J);
    if (L < J) QuickSort(L, J, SCompare);
    L = I;
  } while (I < R);
}

void TStringList::ExchangeItems(int Index1, int Index2)
{
  TStringItem *Item1 = &FList[Index1];
  TStringItem *Item2 = &FList[Index2];
  std::wstring Temp1 = Item1->FString;
  Item1->FString = Item2->FString;
  Item2->FString = Temp1;
  TObject *Temp2 = Item1->FObject;
  Item1->FObject = Item2->FObject;
  Item2->FObject = Temp2;
}

int TStringList::CompareStrings(const std::wstring &S1, const std::wstring &S2)
{
  if (GetCaseSensitive())
    return ::AnsiCompareStr(S1, S2);
  else
    return ::AnsiCompareText(S1, S2);
}

//---------------------------------------------------------------------------
/**
 * Encoding multibyte to wide std::string
 * \param src source std::string
 * \param cp code page
 * \return wide std::string
 */
std::wstring MB2W(const char *src, const UINT cp)
{
    assert(src);

    std::wstring wide;
    const int reqLength = MultiByteToWideChar(cp, 0, src, -1, NULL, 0);
    if (reqLength)
    {
        wide.resize(static_cast<size_t>(reqLength));
        MultiByteToWideChar(cp, 0, src, -1, &wide[0], reqLength);
        wide.erase(wide.length() - 1);  //remove NULL character
    }
    return wide;
}

/**
 * Encoding wide to multibyte std::string
 * \param src source std::string
 * \param cp code page
 * \return multibyte std::string
 */
std::string W2MB(const wchar_t *src, const UINT cp)
{
    assert(src);

    std::string mb;
    const int reqLength = WideCharToMultiByte(cp, 0, src, -1, 0, 0, NULL, NULL);
    if (reqLength)
    {
        mb.resize(static_cast<size_t>(reqLength));
        WideCharToMultiByte(cp, 0, src, -1, &mb[0], reqLength, NULL, NULL);
        mb.erase(mb.length() - 1);  //remove NULL character
    }
    return mb;
}

//---------------------------------------------------------------------------

TDateTime::TDateTime(unsigned int Hour,
    unsigned int Min, unsigned int Sec, unsigned int MSec)
{
    FValue = ::EncodeTimeVerbose(Hour, Min, Sec, MSec);
}
void TDateTime::DecodeDate(unsigned int &Y,
        unsigned int &M, unsigned int &D)
{
    ::DecodeDate(*this, Y, M, D);
}
void TDateTime::DecodeTime(unsigned int &H,
        unsigned int &N, unsigned int &S, unsigned int &MS)
{
    ::DecodeTime(*this, H, N, S, MS);
}

//---------------------------------------------------------------------------
TSHFileInfo::TSHFileInfo()
{

}

TSHFileInfo::~TSHFileInfo()
{
}


HIMAGELIST  TSHFileInfo::GetSystemImageListHandle( BOOL bSmallIcon )
{
    HIMAGELIST  hSystemImageList; 
    SHFILEINFO    ssfi; 

    if (bSmallIcon)
    {
        hSystemImageList = (HIMAGELIST)SHGetFileInfo( 
            (LPCTSTR)std::wstring(L"c:\\").c_str(), 
             0, 
             &ssfi, 
             sizeof(SHFILEINFO), 
             SHGFI_SYSICONINDEX | SHGFI_SMALLICON); 
    }
    else
    {
        hSystemImageList = (HIMAGELIST)SHGetFileInfo( 
            (LPCTSTR)std::wstring(L"c:\\").c_str(), 
             0, 
             &ssfi, 
             sizeof(SHFILEINFO), 
             SHGFI_SYSICONINDEX | SHGFI_LARGEICON); 
    }
    return hSystemImageList;
}


int TSHFileInfo::GetFileIconIndex( std::wstring strFileName , BOOL bSmallIcon )
{
    SHFILEINFO    sfi;

    if (bSmallIcon)
    {
        SHGetFileInfo(
           (LPCTSTR)strFileName.c_str(), 
           FILE_ATTRIBUTE_NORMAL,
           &sfi, 
           sizeof(SHFILEINFO), 
           SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    }
    else
    {
        SHGetFileInfo(
           (LPCTSTR)strFileName.c_str(), 
           FILE_ATTRIBUTE_NORMAL,
           &sfi, 
           sizeof(SHFILEINFO), 
           SHGFI_SYSICONINDEX | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
    }

    return sfi.iIcon;

}

int TSHFileInfo::GetDirIconIndex(BOOL bSmallIcon )
{
    SHFILEINFO    sfi;
    if (bSmallIcon)
    {
         SHGetFileInfo(
             (LPCTSTR)L"Doesn't matter", 
             FILE_ATTRIBUTE_DIRECTORY,
             &sfi, 
             sizeof(SHFILEINFO), 
             SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    }
    else
    {
         SHGetFileInfo(
             (LPCTSTR)L"Doesn't matter", 
             FILE_ATTRIBUTE_DIRECTORY,
             &sfi, 
             sizeof(SHFILEINFO), 
             SHGFI_SYSICONINDEX | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
    }
    return sfi.iIcon;
}
HICON TSHFileInfo::GetFileIconHandle(std::wstring strFileName, BOOL bSmallIcon)
{
    SHFILEINFO    sfi;
    if (bSmallIcon)
    {
        SHGetFileInfo(
           (LPCTSTR)strFileName.c_str(), 
           FILE_ATTRIBUTE_NORMAL,
           &sfi, 
           sizeof(SHFILEINFO), 
           SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    }
    else
    {
        SHGetFileInfo(
           (LPCTSTR)strFileName.c_str(), 
           FILE_ATTRIBUTE_NORMAL,
           &sfi, 
           sizeof(SHFILEINFO), 
           SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
    }
    return sfi.hIcon;
}

HICON TSHFileInfo::GetFolderIconHandle(BOOL bSmallIcon )
{
    SHFILEINFO    sfi;
    if (bSmallIcon)
    {
         SHGetFileInfo(
         (LPCTSTR)"Doesn't matter", 
         FILE_ATTRIBUTE_DIRECTORY,
         &sfi, 
         sizeof(SHFILEINFO), 
         SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    }
    else
    {
         SHGetFileInfo(
         (LPCTSTR)"Does not matter", 
         FILE_ATTRIBUTE_DIRECTORY,
         &sfi, 
         sizeof(SHFILEINFO), 
         SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES);
    }
    return sfi.hIcon;
}

std::wstring TSHFileInfo::GetFileType(std::wstring strFileName)
{
	SHFILEINFO    sfi;
	
	SHGetFileInfo(
     (LPCTSTR)strFileName.c_str(), 
     FILE_ATTRIBUTE_NORMAL,
     &sfi, 
     sizeof(SHFILEINFO), 
     	SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
     
	return sfi.szTypeName;

}

//---------------------------------------------------------------------------

void TStream::ReadBuffer(void *Buffer, unsigned long int Count)
{
    ::Error(SNotImplemented, 400);
}

unsigned long TStream::Read(void *Buffer, unsigned long int Count)
{
    ::Error(SNotImplemented, 401);
    return 0;
}

void TStream::WriteBuffer(void *Buffer, unsigned long int Count)
{
    ::Error(SNotImplemented, 402);
}

unsigned long TStream::Write(void *Buffer, unsigned long int Count)
{
    ::Error(SNotImplemented, 403);
    return 0;
}

//---------------------------------------------------------------------------
TRegistry::TRegistry()
{
    SetRootKey(HKEY_CURRENT_USER);
    SetAccess(KEY_ALL_ACCESS);
    // LazyWrite = True;
}

TRegistry::~TRegistry()
{
    CloseKey();
}

void TRegistry::SetAccess(int access)
{}
void TRegistry::SetRootKey(HKEY ARootKey)
{
  if (FRootKey != ARootKey)
  {
    if (FCloseRootKey)
    {
      RegCloseKey(GetRootKey());
      FCloseRootKey = false;
    }
    FRootKey = ARootKey;
    CloseKey();
  }
}
void TRegistry::GetValueNames(TStrings * Names)
{}

void TRegistry::GetKeyNames(TStrings * Names)
{}
HKEY TRegistry::GetCurrentKey() const { return 0; }
HKEY TRegistry::GetRootKey() const { return 0; }
void TRegistry::CloseKey()
{
  if (GetCurrentKey() != 0)
  {
    // if LazyWrite then
    RegCloseKey(GetCurrentKey()); //else RegFlushKey(CurrentKey);
    FCurrentKey = 0;
    FCurrentPath = L"";
  }
}

bool TRegistry::OpenKey(const std::wstring &key, bool CanCreate)
{
    DEBUG_PRINTF(L"key = %s, CanCreate = %d", key.c_str(), CanCreate);
  S := Key;
  Relative := IsRelative(S);

  if not Relative then Delete(S, 1, 1);
  TempKey := 0;
  if not CanCreate or (S = '') then
  begin
    Result := RegOpenKeyEx(GetBaseKey(Relative), PChar(S), 0,
      FAccess, TempKey) = ERROR_SUCCESS;
  end else
    Result := RegCreateKeyEx(GetBaseKey(Relative), PChar(S), 0, nil,
      REG_OPTION_NON_VOLATILE, FAccess, nil, TempKey, @Disposition) = ERROR_SUCCESS;
  if Result then
  begin
    if (CurrentKey <> 0) and Relative then S := CurrentPath + '\' + S;
    ChangeKey(TempKey, S);
  end;
    return false;
}
bool TRegistry::DeleteKey(const std::wstring &key) { return false; }
bool TRegistry::DeleteValue(const std::wstring &value) { return false; }
bool TRegistry::KeyExists(const std::wstring SubKey) { return false; }
bool TRegistry::ValueExists(const std::wstring Value) { return false; }
int TRegistry::GetDataSize(const std::wstring Name) { return 0; }
bool TRegistry::Readbool(const std::wstring Name) { return false; }
TDateTime TRegistry::ReadDateTime(const std::wstring Name) { return TDateTime(); }
double TRegistry::ReadFloat(const std::wstring Name)
{ return 0; }
int TRegistry::Readint(const std::wstring Name)
{ return 0; }
__int64 TRegistry::ReadInt64(const std::wstring Name)
{ return 0; }
std::wstring TRegistry::ReadString(const std::wstring Name)
{ return L""; }
std::wstring TRegistry::ReadStringRaw(const std::wstring Name)
{ return L""; }
int TRegistry::ReadBinaryData(const std::wstring Name,
  void * Buffer, int Size)
{ return 0; }

void TRegistry::Writebool(const std::wstring Name, bool Value)
{}
void TRegistry::WriteDateTime(const std::wstring Name, TDateTime Value)
{}
void TRegistry::WriteFloat(const std::wstring Name, double Value)
{}
void TRegistry::WriteString(const std::wstring Name, const std::wstring Value)
{}
void TRegistry::WriteStringRaw(const std::wstring Name, const std::wstring Value)
{}
void TRegistry::Writeint(const std::wstring Name, int Value)
{}
void TRegistry::WriteInt64(const std::wstring Name, __int64 Value)
{}
void TRegistry::WriteBinaryData(const std::wstring Name,
  const void * Buffer, int Size)
{}

