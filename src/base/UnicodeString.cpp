#include <headers.hpp>
#include <Sysutils.hpp>
#include "UnicodeString.hpp"
#pragma hdrstop

//------------------------------------------------------------------------------

void AnsiString::Init(const wchar_t * Str, intptr_t Length)
{
  int Size = WideCharToMultiByte(CP_UTF8, 0, Str, (int)(Length > 0 ? Length : -1), nullptr, 0, nullptr, nullptr);
  if (Length > 0)
  {
    Data.resize(Size + 1);
    WideCharToMultiByte(CP_UTF8, 0, Str, (int)(Length > 0 ? Length : -1),
      reinterpret_cast<LPSTR>(const_cast<char *>(Data.c_str())), Size, nullptr, nullptr);
    Data[Size] = 0;
    Data = Data.c_str();
  }
  else
  {
    Data.clear();
  }
}

void AnsiString::Init(const char * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<char *>(Data.c_str()), Str, Length);
  }
  Data = Data.c_str();
}

void AnsiString::Init(const unsigned char * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<char *>(Data.c_str()), Str, Length);
  }
  Data = Data.c_str();
}

AnsiString::operator UnicodeString() const
{
  return UnicodeString(Data.c_str(), Data.size());
}

intptr_t AnsiString::Pos(wchar_t Ch) const
{
  AnsiString s(&Ch, 1);
  return static_cast<intptr_t>(Data.find(s.c_str(), 0, 1)) + 1;
}

AnsiString & AnsiString::Insert(const char * Str, intptr_t Pos)
{
  Data.insert(Pos - 1, Str);
  return *this;
}

AnsiString AnsiString::SubString(intptr_t Pos, intptr_t Len) const
{
  std::string S = Data.substr(Pos - 1, Len);
  AnsiString Result(S.c_str(), S.size());
  return Result;
}

AnsiString & AnsiString::operator=(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString & AnsiString::operator=(const AnsiString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString & AnsiString::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString & AnsiString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData ? lpszData : ""));
  return *this;
}

AnsiString & AnsiString::operator=(const wchar_t * lpwszData)
{
  Init(lpwszData, wcslen(NullToEmpty(lpwszData)));
  return *this;
}

AnsiString AnsiString::operator +(const RawByteString & rhs) const
{
  std::string Result = Data + rhs.c_str();
  return AnsiString(Result.c_str(), Result.size());
}

AnsiString & AnsiString::operator +=(const RawByteString & rhs)
{
  Data.append(reinterpret_cast<const char *>(rhs.c_str()), rhs.size());
  return *this;
}

AnsiString & AnsiString::operator +=(const AnsiString & rhs)
{
  Data.append(rhs.c_str(), rhs.size());
  return *this;
}

AnsiString & AnsiString::operator +=(const UTF8String & rhs)
{
  Data.append(reinterpret_cast<const char *>(rhs.c_str()), rhs.size());
  return *this;
}

AnsiString & AnsiString::operator +=(const char Ch)
{
  Data.append(1, Ch);
  return *this;
}

void AnsiString::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw std::runtime_error("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

//------------------------------------------------------------------------------

void RawByteString::Init(const wchar_t * Str, intptr_t Length)
{
  int Size = WideCharToMultiByte(CP_ACP, 0, Str, (int)(Length > 0 ? Length : -1), nullptr, 0, nullptr, nullptr);
  if (Length > 0)
  {
    Data.resize(Size + 1);
    WideCharToMultiByte(CP_ACP, 0, Str, (int)(Length > 0 ? Length : -1),
      reinterpret_cast<LPSTR>(const_cast<unsigned char *>(Data.c_str())), Size, nullptr, nullptr);
    Data[Size] = 0;
  }
  else
  {
    Data.clear();
  }
}

void RawByteString::Init(const char * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<unsigned char *>(Data.c_str()), Str, Length);
    // Data[Length-1] = 0;
  }
}

void RawByteString::Init(const unsigned char * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<unsigned char *>(Data.c_str()), Str, Length);
    // Data[Length-1] = 0;
  }
}

RawByteString::operator UnicodeString() const
{
  return UnicodeString(reinterpret_cast<const char *>(Data.c_str()), Data.size());
}

intptr_t RawByteString::Pos(wchar_t Ch) const
{
  return Data.find((unsigned char)Ch) + 1;
}

intptr_t RawByteString::Pos(const char Ch) const
{
  return Data.find((unsigned char)Ch) + 1;
}

intptr_t RawByteString::Pos(const char * Str) const
{
  return Data.find((const unsigned char *)Str) + 1;
}

RawByteString & RawByteString::Insert(const char * Str, intptr_t Pos)
{
  Data.insert(Pos - 1, reinterpret_cast<const unsigned char *>(Str));
  return *this;
}

RawByteString RawByteString::SubString(intptr_t Pos, intptr_t Len) const
{
  rawstring_t s = Data.substr(Pos - 1, Len);
  RawByteString Result(s.c_str(), s.size());
  return Result;
}

RawByteString & RawByteString::operator=(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator=(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator=(const AnsiString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator=(const std::wstring & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.size());
  return *this;
}

RawByteString & RawByteString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData ? lpszData : ""));
  return *this;
}

RawByteString & RawByteString::operator=(const wchar_t * lpwszData)
{
  Init(lpwszData, wcslen(NullToEmpty(lpwszData)));
  return *this;
}

RawByteString RawByteString::operator +(const RawByteString & rhs) const
{
  rawstring_t Result = Data + rhs.Data;
  return RawByteString(reinterpret_cast<const char *>(Result.c_str()), Result.size());
}

RawByteString & RawByteString::operator +=(const RawByteString & rhs)
{
  Data.append(reinterpret_cast<const unsigned char *>(rhs.c_str()), rhs.size());
  return *this;
}

RawByteString & RawByteString::operator +=(const UTF8String & rhs)
{
  Data.append(reinterpret_cast<const unsigned char *>(rhs.c_str()), rhs.size());
  return *this;
}

RawByteString & RawByteString::operator +=(const char Ch)
{
  unsigned char ch(static_cast<unsigned char>(Ch));
  Data.append(1, ch);
  return *this;
}

//------------------------------------------------------------------------------

void UTF8String::Init(const wchar_t * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
      memmove(const_cast<wchar_t *>(Data.c_str()), Str, Length * sizeof(wchar_t));
  }
  Data = Data.c_str();
}

void UTF8String::Init(const char * Str, intptr_t Length)
{
  int Size = MultiByteToWideChar(CP_UTF8, 0, Str, (int)(Length > 0 ? Length : -1), NULL, 0);
  Data.resize(Size + 1);
  if (Size > 0)
  {
    MultiByteToWideChar(CP_UTF8, 0, Str, -1, const_cast<wchar_t *>(Data.c_str()), Size);
    Data[Size] = 0;
  }
  Data = Data.c_str();
}

UTF8String::UTF8String(const UnicodeString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

intptr_t UTF8String::Pos(wchar_t Ch) const
{
  return Data.find(Ch) + 1;
}

UTF8String & UTF8String::Insert(const wchar_t * Str, intptr_t Pos)
{
  Data.insert(Pos - 1, Str);
  return *this;
}

UTF8String & UTF8String::operator=(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String & UTF8String::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String & UTF8String::operator=(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String & UTF8String::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData ? lpszData : ""));
  return *this;
}

UTF8String & UTF8String::operator=(const wchar_t * lpwszData)
{
  Init(lpwszData, wcslen(NullToEmpty(lpwszData)));
  return *this;
}

UTF8String UTF8String::operator +(const UTF8String & rhs) const
{
  wstring_t Result = Data + rhs.Data;
  return UTF8String(Result.c_str(), Result.size());
}

UTF8String & UTF8String::operator +=(const UTF8String & rhs)
{
  Data.append(rhs.Data.c_str(), rhs.size());
  return *this;
}

UTF8String & UTF8String::operator +=(const RawByteString & rhs)
{
  UTF8String s(rhs.c_str(), rhs.size());
  Data.append(s.Data.c_str(), s.size());
  return *this;
}

UTF8String & UTF8String::operator +=(const char Ch)
{
  unsigned char ch(static_cast<unsigned char>(Ch));
  Data.append(1, ch);
  return *this;
}

bool operator ==(const UTF8String & lhs, const UTF8String & rhs)
{
  return lhs.Data == rhs.Data;
}

bool operator !=(const UTF8String & lhs, const UTF8String & rhs)
{
  return lhs.Data != rhs.Data;
}

//------------------------------------------------------------------------------

void UnicodeString::Init(const wchar_t * Str, intptr_t Length)
{
  Data.resize(Length);
  if (Length > 0)
  {
    memmove(const_cast<wchar_t *>(Data.c_str()), Str, Length * sizeof(wchar_t));
  }
  Data = Data.c_str();
}

void UnicodeString::Init(const char * Str, intptr_t Length)
{
  int Size = MultiByteToWideChar(CP_UTF8, 0, Str, (int)(Length > 0 ? Length : -1), NULL, 0);
  Data.resize(Size + 1);
  if (Size > 0)
  {
    MultiByteToWideChar(CP_UTF8, 0, Str, -1, const_cast<wchar_t *>(Data.c_str()), Size);
    Data[Size] = 0;
  }
  Data = Data.c_str();
}

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

int UnicodeString::Compare(const UnicodeString & Str) const
{
  return Sysutils::AnsiCompare(*this, Str);
}

int UnicodeString::CompareIC(const UnicodeString & Str) const
{
  return Sysutils::AnsiCompareIC(*this, Str);
}

int UnicodeString::ToInt() const
{
  return Sysutils::StrToIntDef(*this, 0);
}

UnicodeString & UnicodeString::Replace(intptr_t Pos, intptr_t Len, const wchar_t * Str, intptr_t DataLen)
{
  Data.replace(Pos - 1, Len, std::wstring(Str, DataLen));
  return *this;
}

UnicodeString & UnicodeString::Append(const char * lpszAdd, UINT CodePage)
{
  Data.append(::MB2W(lpszAdd, CodePage).c_str());
  return *this;
}

UnicodeString & UnicodeString::Insert(intptr_t Pos, const wchar_t * Str, intptr_t StrLen)
{
  Data.insert(Pos - 1, Str, StrLen);
  return *this;
}

bool UnicodeString::RPos(intptr_t & nPos, wchar_t Ch, intptr_t nStartPos) const
{
  size_t pos = Data.find_last_of(Ch, Data.size() - nStartPos);
  nPos = pos + 1;
  return pos != std::wstring::npos;
}

UnicodeString UnicodeString::SubStr(intptr_t Pos, intptr_t Len) const
{
  std::wstring S(Data.substr(Pos - 1, Len));
  return UnicodeString(S);
}

bool UnicodeString::IsDelimiter(const UnicodeString & Chars, intptr_t Pos) const
{
  return Sysutils::IsDelimiter(Chars, *this, Pos);
}

intptr_t UnicodeString::LastDelimiter(const UnicodeString & Delimiters) const
{
  return Sysutils::LastDelimiter(Delimiters, *this);
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

void UnicodeString::sprintf(const wchar_t * fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  Data = ::Format(fmt, args).c_str();
  va_end(args);
}

UnicodeString & UnicodeString::operator=(const UnicodeString & StrCopy)
{
  // Init(StrCopy.c_str(), StrCopy.Length());
  Data = StrCopy.Data;
  return *this;
}

UnicodeString & UnicodeString::operator=(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UnicodeString & UnicodeString::operator=(const AnsiString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  // Data = StrCopy.Data;
  return *this;
}

UnicodeString & UnicodeString::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UnicodeString & UnicodeString::operator=(const std::wstring & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.size());
  return *this;
}

UnicodeString & UnicodeString::operator=(const wchar_t * Str)
{
  Init(Str, wcslen(NullToEmpty(Str)));
  return *this;
}

UnicodeString & UnicodeString::operator=(const wchar_t Ch)
{
  Init(&Ch, 1);
  return *this;
}

UnicodeString & UnicodeString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(lpszData ? lpszData : ""));
  return *this;
}

UnicodeString UnicodeString::operator +(const UnicodeString & rhs) const
{
  std::wstring Result = Data + rhs.Data;
  return UnicodeString(Result.c_str(), Result.size());
}

UnicodeString & UnicodeString::operator +=(const UnicodeString & rhs)
{
  Data.append(rhs.Data.c_str(), rhs.size());
  return *this;
}

UnicodeString & UnicodeString::operator +=(const wchar_t * rhs)
{
  Data.append(rhs);
  return *this;
}

UnicodeString & UnicodeString::operator +=(const RawByteString & rhs)
{
  UnicodeString s(rhs.c_str(), rhs.size());
  Data.append(s.Data.c_str(), s.size());
  return *this;
}

UnicodeString & UnicodeString::operator +=(const std::wstring & rhs)
{
  Data.append(rhs.c_str(), rhs.size());
  return *this;
}

UnicodeString & UnicodeString::operator +=(const char Ch)
{
  Data.append(1, Ch);
  return *this;
}

UnicodeString & UnicodeString::operator +=(const wchar_t Ch)
{
  Data += Ch;
  return *this;
}

void UnicodeString::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw std::runtime_error("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

//------------------------------------------------------------------------------

UnicodeString operator +(const wchar_t lhs, const UnicodeString & rhs)
{
  return UnicodeString(&lhs, 1) + rhs;
}

UnicodeString operator +(const UnicodeString & lhs, const wchar_t rhs)
{
  return lhs + UnicodeString(rhs);
}

UnicodeString operator +(const wchar_t * lhs, const UnicodeString & rhs)
{
  return UnicodeString(lhs) + rhs;
}

UnicodeString operator +(const UnicodeString & lhs, const wchar_t * rhs)
{
  return lhs + UnicodeString(rhs);
}

UnicodeString operator +(const UnicodeString & lhs, const char * rhs)
{
  return lhs + UnicodeString(rhs);
}

bool operator ==(const UnicodeString & lhs, const wchar_t * rhs)
{
  return wcscmp(lhs.Data.c_str(), rhs) == 0;
}

bool operator ==(const wchar_t * lhs, const UnicodeString & rhs)
{
  return wcscmp(lhs, rhs.Data.c_str()) == 0;
}

bool operator !=(const UnicodeString & lhs, const wchar_t * rhs)
{
  return wcscmp(lhs.Data.c_str(), rhs) != 0;
}

bool operator !=(const wchar_t * lhs, const UnicodeString & rhs)
{
  return wcscmp(lhs, rhs.Data.c_str()) != 0;
}

//------------------------------------------------------------------------------
