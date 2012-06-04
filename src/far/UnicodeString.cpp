#include "headers.hpp"
#include "UnicodeString.hpp"
#include "Sysutils.h"
#pragma hdrstop

//------------------------------------------------------------------------------

AnsiString::operator UnicodeString() const
{
  return UnicodeString(Data.c_str(), Data.size());
}

int AnsiString::Pos(wchar_t Ch) const
{
  AnsiString s(&Ch, 1);
  return Data.find(s.c_str(), 0, 1) + 1;
}

AnsiString & AnsiString::Insert(const char * Str, int Pos)
{
  Data.insert(Pos - 1, Str);
  return *this;
}

AnsiString AnsiString::SubString(int Pos, int Len) const
{
  std::string s = Data.substr(Pos - 1, Len);
  AnsiString Result(s.c_str(), s.size());
  return Result;
}

const AnsiString & AnsiString::operator=(const UnicodeString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const AnsiString & AnsiString::operator=(const AnsiString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const AnsiString & AnsiString::operator=(const UTF8String & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const AnsiString & AnsiString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData));
  return *this;
}

const AnsiString & AnsiString::operator=(const wchar_t * lpszData)
{
  Init(lpszData, wcslen(lpszData));
  return *this;
}

AnsiString __fastcall AnsiString::operator +(const RawByteString & rhs) const
{
  std::string Result = Data + rhs.c_str();
  return AnsiString(Result.c_str(), Result.size());
}

const AnsiString & __fastcall AnsiString::operator +=(const RawByteString & rhs)
{
  Data.append(reinterpret_cast<const char *>(rhs.c_str()), rhs.size());
  return *this;
}

const AnsiString & __fastcall AnsiString::operator +=(const AnsiString & rhs)
{
  Data.append(rhs.c_str(), rhs.size());
  return *this;
}

const AnsiString & __fastcall AnsiString::operator +=(const UTF8String & rhs)
{
  Data.append(reinterpret_cast<const char *>(rhs.c_str()), rhs.size());
  return *this;
}

const AnsiString & __fastcall AnsiString::operator +=(const char Ch)
{
  Data.append(1, Ch);
  return *this;
}

//------------------------------------------------------------------------------

RawByteString::operator UnicodeString() const
{
  return UnicodeString(reinterpret_cast<const char *>(Data.c_str()), Data.size());
}

int RawByteString::Pos(wchar_t Ch) const
{
  return Data.find(Ch) + 1;
}

int RawByteString::Pos(const char Ch) const
{
  return Data.find((unsigned char)Ch) + 1;
}

int RawByteString::Pos(const char * Str) const
{
  return Data.find((const unsigned char *)Str) + 1;
}

RawByteString & RawByteString::Insert(const char * Str, int Pos)
{
  Data.insert(Pos - 1, reinterpret_cast<const unsigned char *>(Str));
  return *this;
}

RawByteString RawByteString::SubString(int Pos, int Len) const
{
  rawstring_t s = Data.substr(Pos - 1, Len);
  RawByteString Result(s.c_str(), s.size());
  return Result;
}

const RawByteString & RawByteString::operator=(const UnicodeString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const RawByteString & RawByteString::operator=(const RawByteString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const RawByteString & RawByteString::operator=(const AnsiString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const RawByteString & RawByteString::operator=(const UTF8String & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const RawByteString & RawByteString::operator=(const std::wstring & strCopy)
{
  Init(strCopy.c_str(), strCopy.size());
  return *this;
}

const RawByteString & RawByteString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData));
  return *this;
}

const RawByteString & RawByteString::operator=(const wchar_t * lpszData)
{
  Init(lpszData, wcslen(lpszData));
  return *this;
}

RawByteString __fastcall RawByteString::operator +(const RawByteString & rhs) const
{
  rawstring_t Result = Data + rhs.Data;
  return RawByteString(reinterpret_cast<const char *>(Result.c_str()), Result.size());
}

const RawByteString & __fastcall RawByteString::operator +=(const RawByteString & rhs)
{
  Data.append(reinterpret_cast<const unsigned char *>(rhs.c_str()), rhs.size());
  return *this;
}
const RawByteString & __fastcall RawByteString::operator +=(const UTF8String & rhs)
{
  Data.append(reinterpret_cast<const unsigned char *>(rhs.c_str()), rhs.size());
  return *this;
}

const RawByteString & __fastcall RawByteString::operator +=(const char Ch)
{
  unsigned char ch(static_cast<unsigned char>(Ch));
  Data.append(1, ch);
  return *this;
}

//------------------------------------------------------------------------------

UTF8String::UTF8String(const UnicodeString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

int UTF8String::Pos(wchar_t Ch) const
{
  return Data.find(Ch) + 1;
}

UTF8String & UTF8String::Insert(const wchar_t * Str, int Pos)
{
  Data.insert(Pos - 1, Str);
  return *this;
}

const UTF8String & UTF8String::operator=(const UnicodeString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UTF8String & UTF8String::operator=(const UTF8String & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UTF8String & UTF8String::operator=(const RawByteString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UTF8String & UTF8String::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData));
  return *this;
}

const UTF8String & UTF8String::operator=(const wchar_t * lpszData)
{
  Init(lpszData, wcslen(lpszData));
  return *this;
}

UTF8String __fastcall UTF8String::operator +(const UTF8String & rhs) const
{
  wstring_t Result = Data + rhs.Data;
  return UTF8String(Result.c_str(), Result.size());
}

const UTF8String & __fastcall UTF8String::operator +=(const UTF8String & rhs)
{
  Data.append(rhs.Data.c_str(), rhs.size());
  return *this;
}
const UTF8String & __fastcall UTF8String::operator +=(const RawByteString & rhs)
{
  UTF8String s(rhs.c_str(), rhs.size());
  Data.append(s.Data.c_str(), s.size());
  return *this;
}

const UTF8String & __fastcall UTF8String::operator +=(const char Ch)
{
  unsigned char ch(static_cast<unsigned char>(Ch));
  Data.append(1, ch);
  return *this;
}

bool __fastcall operator ==(const UTF8String & lhs, const UTF8String & rhs)
{
  return lhs.Data == rhs.Data;
}

bool __fastcall operator !=(const UTF8String & lhs, const UTF8String & rhs)
{
  return lhs.Data != rhs.Data;
}

//------------------------------------------------------------------------------

UnicodeString & UnicodeString::Lower(int nStartPos, int nLength)
{
  Data = Sysutils::LowerCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

UnicodeString & UnicodeString::Upper(int nStartPos, int nLength)
{
  Data = Sysutils::UpperCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

int UnicodeString::CompareIC(const UnicodeString str) const
{
  return Sysutils::AnsiCompareIC(*this, str);
}

int UnicodeString::ToInt() const
{
  return Sysutils::StrToIntDef(*this, 0);
}

UnicodeString & UnicodeString::Replace(int Pos, int Len, const wchar_t * Str, int DataLen)
{
  Data.replace(Pos - 1, Len, std::wstring(Str, DataLen));
  return *this;
}

UnicodeString & UnicodeString::Append(const char * lpszAdd, UINT CodePage)
{
  Data.append(::MB2W(lpszAdd, CodePage).c_str());
  return *this;
}

UnicodeString & UnicodeString::Insert(int Pos, const wchar_t * Str, int StrLen)
{
  Data.insert(Pos - 1, Str, StrLen);
  return *this;
}

bool UnicodeString::RPos(int & nPos, wchar_t Ch, int nStartPos) const
{
  int pos = (int)Data.find_last_of(Ch, Data.size() - nStartPos);
  nPos = pos + 1;
  return pos != std::wstring::npos;
}

UnicodeString UnicodeString::SubStr(int Pos, int Len) const
{
  std::wstring S(Data.substr(Pos - 1, Len));
  return UnicodeString(S);
}

bool UnicodeString::IsDelimiter(UnicodeString Chars, int Pos) const
{
  return Sysutils::IsDelimiter(Chars, *this, Pos);
}

int UnicodeString::LastDelimiter(const UnicodeString & delimiters) const
{
  return Sysutils::LastDelimiter(delimiters, *this);
}

UnicodeString UnicodeString::Trim() const
{
  return Sysutils::Trim(*this);
}

UnicodeString UnicodeString::TrimLeft() const
{
  return Sysutils::TrimLeft(*this);
}

UnicodeString UnicodeString::TrimRight() const
{
  return Sysutils::TrimRight(*this);
}

const UnicodeString & UnicodeString::operator=(const UnicodeString & strCopy)
{
  // Init(strCopy.c_str(), strCopy.Length());
  Data = strCopy.Data;
  return *this;
}

const UnicodeString & UnicodeString::operator=(const RawByteString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UnicodeString & UnicodeString::operator=(const AnsiString & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  // Data = strCopy.Data;
  return *this;
}

const UnicodeString & UnicodeString::operator=(const UTF8String & strCopy)
{
  Init(strCopy.c_str(), strCopy.Length());
  return *this;
}

const UnicodeString & UnicodeString::operator=(const std::wstring & strCopy)
{
  Init(strCopy.c_str(), strCopy.size());
  return *this;
}

const UnicodeString & UnicodeString::operator=(const wchar_t * Str)
{
  Init(Str, wcslen(Str));
  return *this;
}

const UnicodeString & UnicodeString::operator=(const wchar_t Ch)
{
  Init(&Ch, 1);
  return *this;
}

const UnicodeString & UnicodeString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData));
  return *this;
}

UnicodeString __fastcall UnicodeString::operator +(const UnicodeString & rhs) const
{
  std::wstring Result = Data + rhs.Data;
  return UnicodeString(Result.c_str(), Result.size());
}

const UnicodeString & __fastcall UnicodeString::operator +=(const UnicodeString & rhs)
{
  Data.append(rhs.Data.c_str(), rhs.size());
  return *this;
}

const UnicodeString & __fastcall UnicodeString::operator +=(const wchar_t * rhs)
{
  Data.append(rhs);
  return *this;
}

const UnicodeString & __fastcall UnicodeString::operator +=(const RawByteString & rhs)
{
  UnicodeString s(rhs.c_str(), rhs.size());
  Data.append(s.Data.c_str(), s.size());
  return *this;
}

const UnicodeString & __fastcall UnicodeString::operator +=(const std::wstring & rhs)
{
  Data.append(rhs.c_str(), rhs.size());
  return *this;
}

const UnicodeString & __fastcall UnicodeString::operator +=(const char Ch)
{
  Data.append(1, Ch);
  return *this;
}

const UnicodeString & __fastcall UnicodeString::operator +=(const wchar_t Ch)
{
  Data += Ch;
  return *this;
}

void  __cdecl UnicodeString::ThrowIfOutOfRange(int idx) const
{
  if (idx < 1 || idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw std::runtime_error("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

//------------------------------------------------------------------------------

UnicodeString __fastcall operator +(const wchar_t lhs, const UnicodeString & rhs)
{
  return UnicodeString(&lhs, 1) + rhs;
}

UnicodeString __fastcall operator +(const UnicodeString & lhs, const wchar_t rhs)
{
  return lhs + UnicodeString(&rhs);
}

UnicodeString __fastcall operator +(const wchar_t * lhs, const UnicodeString & rhs)
{
  return UnicodeString(lhs) + rhs;
}

UnicodeString __fastcall operator +(const UnicodeString & lhs, const wchar_t * rhs)
{
  return lhs + UnicodeString(rhs);
}

UnicodeString __fastcall operator +(const UnicodeString & lhs, const char * rhs)
{
  return lhs + UnicodeString(rhs);
}

//------------------------------------------------------------------------------
