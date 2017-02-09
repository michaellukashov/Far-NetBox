#include <WinUser.h>

#include <Sysutils.hpp>

#include "UnicodeString.hpp"
#pragma hdrstop

namespace nb {

intptr_t __cdecl StrLength(const wchar_t * str) { return wcslen(NullToEmpty(str)); }

wchar_t __cdecl Upper(wchar_t Ch) { ::CharUpperBuff(&Ch, 1); return Ch; }

wchar_t __cdecl Lower(wchar_t Ch) { ::CharLowerBuff(&Ch, 1); return Ch; }

int __cdecl StrCmpNNI(const wchar_t * s1, int n1, const wchar_t * s2, int n2) { return ::CompareString(0, NORM_IGNORECASE|NORM_STOP_ON_NULL|SORT_STRINGSORT, s1, n1, s2, n2) - 2; }
int __cdecl StrLIComp(const wchar_t * s1, const wchar_t * s2, int n) { return StrCmpNNI(s1, n, s2, n); }

int __cdecl FarStrCmpI(const wchar_t * s1, const wchar_t * s2) { return ::CompareString(0, NORM_IGNORECASE|SORT_STRINGSORT, s1,-1, s2, -1) - 2; }

int __cdecl StrCmpNN(const wchar_t * s1, int n1, const wchar_t * s2, int n2) { return ::CompareString(0, NORM_STOP_ON_NULL|SORT_STRINGSORT, s1, n1, s2, n2) - 2; }

} // namespace nb


AnsiString::AnsiString(const AnsiString & rht)
{
  Init(rht.c_str(), rht.Length());
}

AnsiString::AnsiString(const wchar_t * Str)
{
  Init(Str, string_t::StringLength(Str));
}

AnsiString::AnsiString(const wchar_t * Str, intptr_t Size)
{
  Init(Str, Size);
}

AnsiString::AnsiString(const char * Str)
{
  Init(Str, strlen(NullToEmptyA(Str)));
}

AnsiString::AnsiString(const char * Str, intptr_t Size)
{
  Init(Str, Size);
}

AnsiString::AnsiString(const uint8_t * Str)
{
  Init(Str, Str ? strlen(reinterpret_cast<const char *>(Str)) : 0);
}

AnsiString::AnsiString(const uint8_t * Str, intptr_t Size)
{
  Init(Str, Size);
}

AnsiString::AnsiString(const UnicodeString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

AnsiString::AnsiString(const UTF8String & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

AnsiString::AnsiString(const RawByteString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

void AnsiString::SetLength(intptr_t nLength)
{
  Data.GetBufferSetLength(nLength);
}

AnsiString & AnsiString::Delete(intptr_t Index, intptr_t Count)
{
  Data.Delete(Index - 1, Count);
  return *this;
}

AnsiString & AnsiString::Clear()
{
  Data.Empty();
  return *this;
}

void AnsiString::Init(const wchar_t * Str, intptr_t Length)
{
  if (Str == nullptr || Length == 0)
  {
    Data.Empty();
    return;
  }
  int Size = ::WideCharToMultiByte(CP_UTF8, 0, Str, -1, nullptr, 0, nullptr, nullptr);
  char * Buffer = Data.GetBufferSetLength(Size + 1);
  if (Buffer != nullptr)
  {
    ::WideCharToMultiByte(CP_UTF8, 0, Str, -1, Buffer, Size, nullptr, nullptr);
    Buffer[Size] = 0;
  }
  Data.Truncate(Length);
}

void AnsiString::Init(const char * Str, intptr_t Length)
{
  char * Buffer = Data.GetBufferSetLength(Length);
  if (Buffer != nullptr)
  {
    memmove(Buffer, Str, Length);
  }
  Data.Truncate(Length);
}

void AnsiString::Init(const uint8_t * Str, intptr_t Length)
{
  char * Buffer = Data.GetBufferSetLength(Length);
  if (Buffer != nullptr)
  {
    memmove(const_cast<char *>(Data.c_str()), Str, Length);
  }
  Data.Truncate(Length);
}

intptr_t AnsiString::Pos(const AnsiString & Str) const
{
  return Data.Find(Str.c_str()) + 1;
}

intptr_t AnsiString::Pos(wchar_t Ch) const
{
  return Data.Find(Ch) + 1;
}

char AnsiString::operator [](intptr_t Idx) const
{
  ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
  return Data.operator [](Idx-1);
}

char & AnsiString::operator [](intptr_t Idx)
{
  ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
  return Data.GetBuffer()[Idx-1];
}

AnsiString & AnsiString::Append(const char * Str, intptr_t StrLen)
{
  Data.Append(Str, StrLen);
  return *this;
}

AnsiString & AnsiString::Append(const AnsiString & Str)
{
  return Append(Str.c_str(), Str.GetLength());
}

AnsiString & AnsiString::Append(const char * Str)
{
  return Append(Str, string_t::StringLength(Str));
}

AnsiString & AnsiString::Append(const char Ch)
{
  Data.AppendChar(Ch);
  return *this;
}

AnsiString & AnsiString::Insert(const char * Str, intptr_t Pos)
{
  Data.Insert(Pos - 1, Str);
  return *this;
}

AnsiString AnsiString::SubString(intptr_t Pos) const
{
  string_t Str(Data.Mid(Pos - 1));
  return AnsiString(Str.c_str(), Str.GetLength());
}

AnsiString AnsiString::SubString(intptr_t Pos, intptr_t Len) const
{
  string_t Str(Data.Mid(Pos - 1), Len);
  return AnsiString(Str.c_str(), Str.GetLength());
}

AnsiString & AnsiString::operator=(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString & AnsiString::operator=(const AnsiString & StrCopy)
{
  if (*this != StrCopy)
  {
    Init(StrCopy.c_str(), StrCopy.Length());
  }
  return *this;
}

AnsiString & AnsiString::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString & AnsiString::operator=(const char * lpszData)
{
  Init(lpszData, strlen(NullToEmptyA(lpszData)));
  return *this;
}

AnsiString & AnsiString::operator=(const wchar_t * lpwszData)
{
  Init(lpwszData, wcslen(NullToEmpty(lpwszData)));
  return *this;
}

AnsiString & AnsiString::operator +=(const AnsiString & rhs)
{
  Data.Append(rhs.c_str(), rhs.Length());
  return *this;
}

AnsiString & AnsiString::operator +=(const char Ch)
{
  Data.AppendChar(Ch);
  return *this;
}

AnsiString & AnsiString::operator +=(const char * rhs)
{
  Data.Append(rhs);
  return *this;
}

void AnsiString::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

void RawByteString::Init(const wchar_t * Str, intptr_t Length)
{
  if (Str == nullptr || Length == 0)
  {
    Data.Empty();
    return;
  }
  int Size = ::WideCharToMultiByte(CP_ACP, 0, Str, -1, nullptr, 0, nullptr, nullptr);
  char * Buffer = Data.GetBufferSetLength(Length);
  if (Buffer != nullptr)
  {
    ::WideCharToMultiByte(CP_ACP, 0, Str, -1, Buffer, Size, nullptr, nullptr);
    Buffer[Size] = 0;
  }
  Data.Truncate(Length);
}

void RawByteString::Init(const char * Str, intptr_t Length)
{
  char * Buffer = Data.GetBufferSetLength(Length);
  if (Length > 0)
  {
    memmove(Buffer, Str, Length);
  }
}

void RawByteString::Init(const uint8_t * Str, intptr_t Length)
{
  char * Buffer = Data.GetBufferSetLength(Length);
  if (Length > 0)
  {
    memmove(Buffer, Str, Length);
  }
}

RawByteString::operator UnicodeString() const
{
  return UnicodeString(reinterpret_cast<const char *>(Data.c_str()), Data.GetLength());
}

intptr_t RawByteString::Pos(wchar_t Ch) const
{
  return Data.Find(static_cast<uint8_t>(Ch)) + 1;
}

intptr_t RawByteString::Pos(const char Ch) const
{
  return Data.Find(static_cast<uint8_t>(Ch)) + 1;
}

intptr_t RawByteString::Pos(const char * Str) const
{
  return Data.Find(reinterpret_cast<const char *>(Str)) + 1;
}

RawByteString::RawByteString(const wchar_t * Str)
{
  Init(Str, rawstring_t::StringLength(Str));
}

RawByteString::RawByteString(const wchar_t * Str, intptr_t Size)
{
  Init(Str, Size);
}

RawByteString::RawByteString(const char * Str)
{
  Init(Str, rawstring_t::StringLength(Str));
}

RawByteString::RawByteString(const char * Str, intptr_t Size)
{
  Init(Str, Size);
}

RawByteString::RawByteString(const uint8_t * Str)
{
  Init(Str, rawstring_t::StringLength((char *)Str));
}

RawByteString::RawByteString(const UnicodeString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

RawByteString::RawByteString(const RawByteString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

RawByteString::RawByteString(const AnsiString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

RawByteString::RawByteString(const UTF8String & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

void RawByteString::SetLength(intptr_t nLength)
{
  Data.GetBufferSetLength(nLength);
}

RawByteString & RawByteString::Delete(intptr_t Index, intptr_t Count)
{
  Data.Delete(Index - 1, Count);
  return *this;
}

RawByteString & RawByteString::Insert(const char * Str, intptr_t Pos)
{
  Data.Insert(Pos - 1, static_cast<const char *>(Str));
  return *this;
}

RawByteString RawByteString::SubString(intptr_t Pos) const
{
  rawstring_t Str(Data.Mid(Pos - 1));
  return UTF8String(Str.c_str(), Str.GetLength());
}

RawByteString RawByteString::SubString(intptr_t Pos, intptr_t Len) const
{
  rawstring_t s = Data.Mid(Pos - 1, Len);
  RawByteString Result(s.c_str(), s.GetLength());
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

RawByteString & RawByteString::operator=(const char * lpszData)
{
  Init(lpszData, rawstring_t::StringLength(lpszData));
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
  return RawByteString(reinterpret_cast<const char *>(Result.c_str()), Result.GetLength());
}

RawByteString & RawByteString::operator +=(const RawByteString & rhs)
{
  Data.Append(rhs.c_str(), rhs.Length());
  return *this;
}

RawByteString & RawByteString::operator +=(const char Ch)
{
  Data.AppendChar(Ch);
  return *this;
}

UTF8String::UTF8String(const UTF8String & rht)
{
  Init(rht.c_str(), rht.Length());
}

void UTF8String::Init(const wchar_t * Str, intptr_t Length)
{
  if (Str == nullptr || Length == 0)
  {
    Data.Empty();
    return;
  }
  int Size = ::WideCharToMultiByte(CP_UTF8, 0, Str, -1, nullptr, 0, nullptr, nullptr);
  char * Buffer = Data.GetBufferSetLength(Size + 1);
  if (Buffer != nullptr)
  {
    ::WideCharToMultiByte(CP_UTF8, 0, Str, -1, Buffer, Size, nullptr, nullptr);
    Buffer[Size] = 0;
  }
  Data.Truncate(Length);
}

void UTF8String::Init(const char * Str, intptr_t Length)
{
  char * Buffer = Data.GetBufferSetLength(Length);
  if (Length > 0)
  {
    memmove(Buffer, Str, Length);
  }
}

UTF8String::UTF8String(const UnicodeString & Str)
{
  Init(Str.c_str(), Str.GetLength());
}

UTF8String::UTF8String(const wchar_t * Str)
{
  Init(Str, string_t::StringLength(Str));
}

UTF8String::UTF8String(const wchar_t * Str, intptr_t Size)
{
  Init(Str, Size);
}

UTF8String::UTF8String(const char * Str, intptr_t Size)
{
  Init(Str, Size);
}

UTF8String::UTF8String(const char * Str)
{
  Init(Str, string_t::StringLength(Str));
}

void UTF8String::SetLength(intptr_t nLength)
{
  Data.GetBufferSetLength(nLength);
}

UTF8String & UTF8String::Delete(intptr_t Index, intptr_t Count)
{
  Data.Delete(Index - 1, Count);
  return *this;
}

intptr_t UTF8String::Pos(char Ch) const
{
  return Data.Find(Ch) + 1;
}

int UTF8String::vprintf(const char * Format, va_list ArgList)
{
  SetLength(32 * 1024);
  int Size = vsnprintf_s((char *)Data.c_str(), Data.GetLength(), _TRUNCATE, Format, ArgList);
  Data.Truncate(Size);
  return Size;
}

UTF8String & UTF8String::Insert(const wchar_t * Str, intptr_t Pos)
{
  UTF8String UTF8(Str);
  Data.Insert(Pos - 1, UTF8.c_str());
  return *this;
}

UTF8String UTF8String::SubString(intptr_t Pos) const
{
  string_t Str(Data.Mid(Pos - 1));
  return UTF8String(Str.c_str(), Str.GetLength());
}

UTF8String UTF8String::SubString(intptr_t Pos, intptr_t Len) const
{
  string_t Str(Data.Mid(Pos - 1), Len);
  return UTF8String(Str.c_str(), Str.GetLength());
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
  Init(lpszData, string_t::StringLength(lpszData));
  return *this;
}

UTF8String & UTF8String::operator=(const wchar_t * lpwszData)
{
  Init(lpwszData, string_t::StringLength(lpwszData));
  return *this;
}

UTF8String UTF8String::operator +(const UTF8String & rhs) const
{
  string_t Result(Data);
  Result += rhs.Data;
  return UTF8String(Result);
}

UTF8String & UTF8String::operator +=(const UTF8String & rhs)
{
  Data.Append(rhs.Data.c_str(), rhs.Length());
  return *this;
}

UTF8String & UTF8String::operator +=(const RawByteString & rhs)
{
  UTF8String s(rhs.c_str(), rhs.Length());
  Data.Append(s.Data.c_str(), s.Length());
  return *this;
}

UTF8String & UTF8String::operator +=(const char Ch)
{
  Data.AppendChar(Ch);
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

void UnicodeString::Init(const wchar_t * Str, intptr_t Length)
{
  Data = wstring_t(Str, Length);
}

void UnicodeString::Init(const char * Str, intptr_t Length, int Codepage)
{
  if (Str == nullptr || Length == 0)
  {
    Data.Empty();
    return;
  }
  // int Len = static_cast<int>(Length > 0 ? Length : -1);
  int Size = ::MultiByteToWideChar(Codepage, 0, Str, -1, nullptr, 0);
  wchar_t * Buffer = Data.GetBufferSetLength(Size + 1);
  if (Buffer != nullptr)
  {
    ::MultiByteToWideChar(Codepage, 0, Str, -1, Buffer, Size);
    Buffer[Size] = 0;
  }
  Data.Truncate(Length);
}

UnicodeString::UnicodeString(const UTF8String & Str)
{
  Init(Str.c_str(), Str.GetLength(), CP_UTF8);
}

UnicodeString::UnicodeString(const wchar_t * Str)
{
  Init(Str, wstring_t::StringLength(Str));
}

UnicodeString::UnicodeString(const char * Str, intptr_t Size)
{
  Init(Str, Size, CP_THREAD_ACP);
}

UnicodeString::UnicodeString(const char * Str)
{
  Init(Str, wstring_t::StringLength(Str), CP_THREAD_ACP);
}

UnicodeString::UnicodeString(const AnsiString & Str)
{
  Init(Str.c_str(), Str.GetLength(), CP_THREAD_ACP);
}

void UnicodeString::SetLength(intptr_t nLength)
{
  Data.GetBufferSetLength(nLength);
}

UnicodeString & UnicodeString::Delete(intptr_t Index, intptr_t Count)
{
  Data.Delete(Index - 1, Count);
  return *this;
}

UnicodeString & UnicodeString::Lower(intptr_t nStartPos, intptr_t nLength)
{
  Data = ::LowerCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

UnicodeString & UnicodeString::Upper(intptr_t nStartPos, intptr_t nLength)
{
  Data = ::UpperCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

intptr_t UnicodeString::Compare(const UnicodeString & Str) const
{
  return ::AnsiCompare(*this, Str);
}

intptr_t UnicodeString::CompareIC(const UnicodeString & Str) const
{
  return ::AnsiCompareIC(*this, Str);
}

intptr_t UnicodeString::ToInt() const
{
  return ::StrToIntDef(*this, 0);
}

UnicodeString & UnicodeString::Replace(intptr_t Pos, intptr_t Len, const wchar_t * Str, intptr_t DataLen)
{
  wstring_t NewData = Data;
  NewData.Delete(Pos - 1, Len);
  NewData.Insert(Pos - 1, wstring_t(Str, DataLen).c_str());
  Data = NewData;
  return *this;
}

UnicodeString & UnicodeString::Replace(intptr_t Pos, intptr_t Len, const wchar_t * Str)
{
  return Replace(Pos, Len, Str, wstring_t::StringLength(Str));
}

UnicodeString & UnicodeString::Append(const wchar_t * Str)
{
  return Append(Str, wstring_t::StringLength(Str));
}

UnicodeString & UnicodeString::Append(const char * lpszAdd, UINT CodePage)
{
  //UTF8String Str(lpszAdd);
  Data.Append(wstring_t(lpszAdd)); // , CodePage));
  return *this;
}

UnicodeString & UnicodeString::Insert(intptr_t Pos, const wchar_t * Str, intptr_t StrLen)
{
  Data.Insert(Pos - 1, wstring_t(Str, StrLen).c_str());
  return *this;
}

intptr_t UnicodeString::Pos(wchar_t Ch) const
{
  return Data.Find(Ch) + 1;
}

intptr_t UnicodeString::Pos(const UnicodeString & Str) const
{
  return Data.Find(Str.Data.c_str()) + 1;
}

bool UnicodeString::RPos(intptr_t & nPos, wchar_t Ch, intptr_t nStartPos) const
{
  size_t Pos = Data.ReverseFind(Ch); //, Data.size() - nStartPos);
  nPos = Pos + 1;
  return Pos != std::wstring::npos;
}

UnicodeString UnicodeString::SubStr(intptr_t Pos, intptr_t Len) const
{
  wstring_t Str(Data.Mid(Pos - 1, Len));
  return UnicodeString(Str.c_str(), Str.GetLength());
}

UnicodeString UnicodeString::SubStr(intptr_t Pos) const
{
  wstring_t Str(Data.Mid(Pos - 1));
  return UnicodeString(Str.c_str(), Str.GetLength());
}

UnicodeString UnicodeString::SubString(intptr_t Pos, intptr_t Len) const
{
  return SubStr(Pos, Len);
}

UnicodeString UnicodeString::SubString(intptr_t Pos) const
{
  return SubStr(Pos);
}

bool UnicodeString::IsDelimiter(const UnicodeString & Chars, intptr_t Pos) const
{
  return ::IsDelimiter(Chars, *this, Pos);
}

intptr_t UnicodeString::LastDelimiter(const UnicodeString & Delimiters) const
{
  return ::LastDelimiter(Delimiters, *this);
}

UnicodeString UnicodeString::Trim() const
{
  return ::Trim(*this);
}

UnicodeString UnicodeString::TrimLeft() const
{
  return ::TrimLeft(*this);
}

UnicodeString UnicodeString::TrimRight() const
{
  return ::TrimRight(*this);
}

void UnicodeString::sprintf(const wchar_t * fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  Data = ::FormatV(fmt, args).c_str();
  va_end(args);
}

UnicodeString & UnicodeString::operator=(const UnicodeString & StrCopy)
{
  if (*this != StrCopy)
  {
    Data = StrCopy.Data;
  }
  return *this;
}

UnicodeString & UnicodeString::operator=(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length(), CP_UTF8);
  return *this;
}

UnicodeString & UnicodeString::operator=(const AnsiString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length(), CP_THREAD_ACP);
  // Data = StrCopy.Data;
  return *this;
}

UnicodeString & UnicodeString::operator=(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length(), CP_UTF8);
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
  Init(lpszData, wstring_t::StringLength(lpszData), CP_UTF8);
  return *this;
}

UnicodeString UnicodeString::operator +(const UnicodeString & rhs) const
{
  wstring_t Result(Data);
  Result += rhs.Data;
  return UnicodeString(Result);
}

UnicodeString & UnicodeString::operator +=(const UnicodeString & rhs)
{
  Data.Append(rhs.Data.c_str(), rhs.Length());
  return *this;
}

UnicodeString & UnicodeString::operator +=(const wchar_t * rhs)
{
  Data.Append(rhs);
  return *this;
}

UnicodeString & UnicodeString::operator +=(const RawByteString & rhs)
{
  UnicodeString s(rhs.c_str(), rhs.Length());
  Data.Append(s.Data.c_str(), s.Length());
  return *this;
}

UnicodeString & UnicodeString::operator +=(const char Ch)
{
  Data.AppendChar(Ch);
  return *this;
}

UnicodeString & UnicodeString::operator +=(const wchar_t Ch)
{
  Data += Ch;
  return *this;
}

wchar_t UnicodeString::operator [](intptr_t Idx) const
{
  ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
  return Data.operator [](Idx-1);
}

wchar_t & UnicodeString::operator [](intptr_t Idx)
{
  ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
  return Data.GetBuffer()[Idx-1];
}

void UnicodeString::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

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
  return wcscmp(lhs.Data.c_str(), NullToEmpty(rhs)) == 0;
}

bool operator ==(const wchar_t * lhs, const UnicodeString & rhs)
{
  return wcscmp(NullToEmpty(lhs), rhs.Data.c_str()) == 0;
}

bool operator !=(const UnicodeString & lhs, const wchar_t * rhs)
{
  return wcscmp(lhs.Data.c_str(), NullToEmpty(rhs)) != 0;
}

bool operator !=(const wchar_t * lhs, const UnicodeString & rhs)
{
  return wcscmp(NullToEmpty(lhs), rhs.Data.c_str()) != 0;
}
