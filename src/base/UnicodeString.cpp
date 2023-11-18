#include <vcl.h>

#include <Sysutils.hpp>
#include "UnicodeString.hpp"


AnsiString::AnsiString(const AnsiString & rhs) :
  Data(rhs.c_str(), rhs.Length())
{
}

AnsiString::AnsiString(const wchar_t * Str) :
  Data(Str, string_t::StringLength(Str), CP_UTF8)
{
}

AnsiString::AnsiString(const wchar_t * Str, int32_t Length) :
  Data(Str, Length, CP_UTF8)
{
}

AnsiString::AnsiString(const wchar_t * Str, int32_t Length, int32_t CodePage) :
  Data(Str, Length, CodePage)
{
}

AnsiString::AnsiString(const char * Str) :
  Data(Str, string_t::StringLength(Str))
{
}

AnsiString::AnsiString(const char * Str, int32_t Length) :
  Data(Str, Length)
{
}

AnsiString::AnsiString(const unsigned char * Str) :
  Data(reinterpret_cast<const char *>(Str), string_t::StringLength(reinterpret_cast<const char *>(Str)))
{
}

AnsiString::AnsiString(const unsigned char * Str, int32_t Length) :
  Data(reinterpret_cast<const char *>(Str), Length)
{
}

AnsiString::AnsiString(const UnicodeString & Str) :
  Data(Str.c_str(), Str.Length())
{
}

AnsiString::AnsiString(const UTF8String & Str) :
  Data(Str.c_str(), Str.Length())
{
}

AnsiString::AnsiString(const RawByteString & Str) :
  Data(Str.c_str(), Str.Length())
{
}

char * AnsiString::SetLength(int32_t nLength)
{
  return Data.GetBufferSetLength(nLength);
}

AnsiString & AnsiString::Delete(int32_t Index, int32_t Count)
{
  Data.Delete(Index - 1, Count);
  return *this;
}

AnsiString & AnsiString::Clear()
{
  Data.Empty();
  return *this;
}

void AnsiString::Init(const wchar_t * Str, int32_t Length)
{
  Data = string_t(Str, Length);
}

void AnsiString::Init(const char * Str, int32_t Length)
{
  Data = string_t(Str, Length);
}

void AnsiString::Init(const unsigned char * Str, int32_t Length)
{
  Data = string_t(reinterpret_cast<const char *>(Str), Length);
}

int32_t AnsiString::Pos(const AnsiString & Str) const
{
  return Data.Find(Str.c_str()) + 1;
}

int32_t AnsiString::Pos(char Ch) const
{
  return Data.Find(Ch) + 1;
}

char AnsiString::operator [](int32_t Idx) const
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return Data.operator[](Idx - 1);
}

char & AnsiString::operator [](int32_t Idx)
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return Data.GetBuffer()[Idx - 1];
}

AnsiString & AnsiString::Append(const char * Str, int32_t StrLen)
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

AnsiString & AnsiString::Insert(const char * Str, int32_t Pos)
{
  Data.Insert(Pos - 1, Str);
  return *this;
}

AnsiString AnsiString::SubString(int32_t Pos) const
{
  string_t Str(Data.Mid(Pos - 1));
  return AnsiString(Str.c_str(), Str.GetLength());
}

AnsiString AnsiString::SubString(int32_t Pos, int32_t Len) const
{
  string_t Str(Data.Mid(Pos - 1), Len);
  return AnsiString(Str.c_str(), Str.GetLength());
}

AnsiString & AnsiString::operator =(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString & AnsiString::operator =(const AnsiString & StrCopy)
{
  if (*this != StrCopy)
  {
    Init(StrCopy.c_str(), StrCopy.Length());
  }
  return *this;
}

AnsiString & AnsiString::operator =(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString & AnsiString::operator =(const char * Str)
{
  Init(Str, string_t::StringLength(Str));
  return *this;
}

AnsiString & AnsiString::operator =(const wchar_t * Str)
{
  Init(Str, CMStringW::StringLength(Str));
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

void AnsiString::ThrowIfOutOfRange(int32_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}


RawByteString::RawByteString(const wchar_t * Str) :
  Data(Str, rawstring_t::StringLength(Str))
{
}

RawByteString::RawByteString(const wchar_t * Str, int32_t Length) :
  Data(Str, Length)
{
}

RawByteString::RawByteString(const char * Str) :
  Data(Str, rawstring_t::StringLength(Str))
{
}

RawByteString::RawByteString(const char * Str, int32_t Length) :
  Data(Str, Length)
{
}

RawByteString::RawByteString(const unsigned char * Str) :
  Data(reinterpret_cast<const char *>(Str), rawstring_t::StringLength(reinterpret_cast<const char *>(Str)))
{
}

RawByteString::RawByteString(const unsigned char * Str, int32_t Length) :
  Data(reinterpret_cast<const char *>(Str), Length)
{
}

RawByteString::RawByteString(const UnicodeString & Str) :
  Data(Str.c_str(), Str.Length())
{
}

RawByteString::RawByteString(const RawByteString & Str) :
  Data(Str.c_str(), Str.Length())
{
}

RawByteString::RawByteString(const AnsiString & Str) :
  Data(Str.c_str(), Str.Length())
{
}

RawByteString::RawByteString(const UTF8String & Str) :
  Data(Str.c_str(), Str.Length())
{
}

void RawByteString::Init(const wchar_t * Str, int32_t Length)
{
  Data = rawstring_t(Str, Length);
}

void RawByteString::Init(const char * Str, int32_t Length)
{
  Data = rawstring_t(Str, Length);
}

void RawByteString::Init(const unsigned char * Str, int32_t Length)
{
  Data = rawstring_t(reinterpret_cast<const char *>(Str), Length);
}

RawByteString::operator UnicodeString() const
{
  return UnicodeString(reinterpret_cast<const char *>(Data.c_str()), Data.GetLength());
}

int32_t RawByteString::Pos(wchar_t Ch) const
{
  return Data.Find(static_cast<unsigned char>(Ch)) + 1;
}

int32_t RawByteString::Pos(const char Ch) const
{
  return Data.Find(static_cast<unsigned char>(Ch)) + 1;
}

int32_t RawByteString::Pos(const char * Str) const
{
  return Data.Find(reinterpret_cast<const char *>(Str)) + 1;
}

char * RawByteString::SetLength(int32_t nLength)
{
  return Data.GetBufferSetLength(nLength);
}

RawByteString & RawByteString::Delete(int32_t Index, int32_t Count)
{
  Data.Delete(Index - 1, Count);
  return *this;
}

RawByteString & RawByteString::Insert(const char * Str, int32_t Pos)
{
  Data.Insert(Pos - 1, static_cast<const char *>(Str));
  return *this;
}

RawByteString RawByteString::SubString(int32_t Pos) const
{
  rawstring_t Str(Data.Mid(Pos - 1));
  return UTF8String(Str.c_str(), Str.GetLength());
}

RawByteString RawByteString::SubString(int32_t Pos, int32_t Len) const
{
  rawstring_t s = Data.Mid(Pos - 1, Len);
  RawByteString Result(s.c_str(), s.GetLength());
  return Result;
}

void RawByteString::ThrowIfOutOfRange(int32_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

unsigned char RawByteString::operator [](int32_t Idx) const
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return Data.operator[](Idx - 1);
}

RawByteString RawByteString::Trim() const
{
  return ::Trim(*this);
}

RawByteString RawByteString::TrimLeft() const
{
  return ::TrimLeft(*this);
}

RawByteString RawByteString::TrimRight() const
{
  return ::TrimRight(*this);
}

unsigned char & RawByteString::operator [](int32_t Idx)
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return nb::ToUInt8Ptr(Data.GetBuffer())[Idx - 1];
}

RawByteString & RawByteString::operator =(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator =(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator =(const AnsiString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator =(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString & RawByteString::operator =(const char * lpszData)
{
  Init(lpszData, rawstring_t::StringLength(lpszData));
  return *this;
}

RawByteString & RawByteString::operator =(const wchar_t * lpwszData)
{
  Init(lpwszData, CMStringW::StringLength(lpwszData));
  return *this;
}

RawByteString RawByteString::operator +(const RawByteString & rhs) const
{
  rawstring_t Result = Data + rhs.Data;
  return RawByteString(Result.c_str(), Result.GetLength());
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


UTF8String::UTF8String(const UTF8String & rhs) :
  Data(rhs.c_str(), rhs.Length())
{
}

UTF8String::UTF8String(const UnicodeString & Str) :
  Data(Str.c_str(), Str.Length(), CP_UTF8)
{
}

UTF8String::UTF8String(const wchar_t * Str) :
  Data(Str, string_t::StringLength(Str))
{
}

UTF8String::UTF8String(const wchar_t * Str, int32_t Length) :
  Data(Str, Length)
{
}

UTF8String::UTF8String(const char * Str, int32_t Length) :
  Data(Str, Length)
{
}

UTF8String::UTF8String(const char * Str) :
  Data(Str, string_t::StringLength(Str))
{
}

void UTF8String::Init(const wchar_t * Str, int32_t Length)
{
  Data = string_t(Str, Length);
}

void UTF8String::Init(const char * Str, int32_t Length)
{
  Data = string_t(Str, Length);
}

char * UTF8String::SetLength(int32_t nLength)
{
  return Data.GetBufferSetLength(nLength);
}

UTF8String & UTF8String::Delete(int32_t Index, int32_t Count)
{
  Data.Delete(Index - 1, Count);
  return *this;
}

int32_t UTF8String::Pos(char Ch) const
{
  return Data.Find(Ch) + 1;
}

int32_t UTF8String::vprintf(const char * Format, va_list ArgList)
{
  char * Buf = SetLength(32 * 1024);
  int32_t Size = vsnprintf_s(Buf, Data.GetLength(), _TRUNCATE, Format, ArgList);
  Data.Truncate(Size);
  return Size;
}

UTF8String & UTF8String::Insert(wchar_t Ch, int32_t Pos)
{
  UTF8String UTF8(&Ch, 1);
  Data.Insert(Pos - 1, UTF8.c_str());
  return *this;
}

UTF8String & UTF8String::Insert(const wchar_t * Str, int32_t Pos)
{
  UTF8String UTF8(Str);
  Data.Insert(Pos - 1, UTF8.c_str());
  return *this;
}

UTF8String UTF8String::SubString(int32_t Pos) const
{
  string_t Str(Data.Mid(Pos - 1));
  return UTF8String(Str.c_str(), Str.GetLength());
}

UTF8String UTF8String::SubString(int32_t Pos, int32_t Len) const
{
  string_t Str(Data.Mid(Pos - 1), Len);
  return UTF8String(Str.c_str(), Str.GetLength());
}

UTF8String & UTF8String::operator =(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String & UTF8String::operator =(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String & UTF8String::operator =(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String & UTF8String::operator =(const char * lpszData)
{
  Init(lpszData, string_t::StringLength(lpszData));
  return *this;
}

UTF8String & UTF8String::operator =(const wchar_t * lpwszData)
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

UTF8String UTF8String::operator +(const char * rhs) const
{
  string_t Result(Data);
  Result += rhs;
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

UTF8String & UTF8String::operator +=(const char * rhs)
{
  Data.Append(rhs);
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

UnicodeString::UnicodeString(const UnicodeString & Str) :
  Data(Str.c_str(), Str.GetLength())
{
}

UnicodeString::UnicodeString(const UTF8String & Str) :
  Data(Str.c_str(), Str.GetLength(), CP_UTF8)
{
}

UnicodeString::UnicodeString(const wchar_t * Str) :
  Data(Str, wstring_t::StringLength(Str))
{
}

UnicodeString::UnicodeString(const wchar_t * Str, int32_t Length) :
  Data(Str, Length)
{
}

/*UnicodeString::UnicodeString(const wchar_t Src) :
  Data(&Src, 1)
{
}*/

UnicodeString::UnicodeString(const char * Str, int32_t Length) :
  Data(Str, Length)
{
}

UnicodeString::UnicodeString(const char * Str, int32_t Length, int32_t CodePage) :
  Data(Str, Length, CodePage)
{
}

UnicodeString::UnicodeString(const char * Str) :
  Data(Str, wstring_t::StringLength(Str))
{
}

UnicodeString::UnicodeString(const AnsiString & Str) :
  Data(Str.c_str(), Str.Length())
{
}

void UnicodeString::Init(const wchar_t * Str, int32_t Length)
{
  Data = wstring_t(Str, Length);
}

void UnicodeString::Init(const char * Str, int32_t Length, int32_t CodePage)
{
  Data = wstring_t(Str, Length, CodePage);
}

wchar_t * UnicodeString::SetLength(int32_t nLength)
{
  return Data.GetBufferSetLength(nLength);
}

UnicodeString & UnicodeString::Delete(int32_t Index, int32_t Count)
{
  Data.Delete(Index - 1, Count);
  return *this;
}

UnicodeString & UnicodeString::Lower(int32_t nStartPos)
{
  Data = ::LowerCase(SubString(nStartPos)).c_str();
  return *this;
}

UnicodeString & UnicodeString::Lower(int32_t nStartPos, int32_t nLength)
{
  Data = ::LowerCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

UnicodeString & UnicodeString::Upper(int32_t nStartPos)
{
  Data = ::UpperCase(SubString(nStartPos)).c_str();
  return *this;
}

UnicodeString & UnicodeString::Upper(int32_t nStartPos, int32_t nLength)
{
  Data = ::UpperCase(SubString(nStartPos, nLength)).c_str();
  return *this;
}

int32_t UnicodeString::Compare(const UnicodeString & Str) const
{
  return ::AnsiCompare(*this, Str);
}

int32_t UnicodeString::CompareIC(const UnicodeString & Str) const
{
  return ::AnsiCompareIC(*this, Str);
}

int32_t UnicodeString::ToIntPtr() const
{
  return ::StrToIntDef(*this, 0);
}

int32_t UnicodeString::FindFirstOf(const wchar_t Ch) const
{
  return static_cast<int32_t>(Data.Find(Ch, 0)) + 1;
}

int32_t UnicodeString::FindFirstOf(const wchar_t * Str, int32_t Offset) const
{
  if (!Str || !*Str)
    return nb::NPOS;
  // int32_t Length = wstring_t::StringLength(Str);
  wstring_t str = Data.Mid(Offset);
  int32_t Res = str.FindOneOf(Str);
  if (Res != -1)
    return Res + Offset;
  return nb::NPOS;
}

UnicodeString & UnicodeString::Replace(int32_t Pos, int32_t Len, const wchar_t * Str, int32_t DataLen)
{
  wstring_t NewData = Data;
  NewData.Delete(Pos - 1, Len);
  NewData.Insert(Pos - 1, wstring_t(Str, DataLen).c_str());
  Data = NewData;
  return *this;
}

UnicodeString & UnicodeString::Replace(int32_t Pos, int32_t Len, const wchar_t * Str)
{
  return Replace(Pos, Len, Str, wstring_t::StringLength(Str));
}

UnicodeString & UnicodeString::Append(const wchar_t * Str)
{
  return Append(Str, wstring_t::StringLength(Str));
}

UnicodeString & UnicodeString::Append(const char * lpszAdd, int32_t CodePage)
{
  Data.Append(wstring_t(lpszAdd, wstring_t::StringLength(lpszAdd), CodePage));
  return *this;
}

UnicodeString & UnicodeString::Insert(int32_t Pos, const wchar_t * Str, int32_t StrLen)
{
  Data.Insert(Pos - 1, wstring_t(Str, StrLen).c_str());
  return *this;
}

UnicodeString & UnicodeString::Insert(const wchar_t * Str, int32_t Pos)
{
  return Insert(Pos, Str, wstring_t::StringLength(Str));
}

int32_t UnicodeString::Pos(wchar_t Ch) const
{
  return Data.Find(Ch) + 1;
}

int32_t UnicodeString::Pos(const UnicodeString & Str) const
{
  return Data.Find(Str.Data.c_str()) + 1;
}

bool UnicodeString::RPos(int32_t & nPos, wchar_t Ch, int32_t /*nStartPos*/) const
{
  size_t Pos = Data.ReverseFind(Ch); //, Data.size() - nStartPos);
  nPos = Pos + 1;
  return Pos != std::wstring::npos;
}

UnicodeString UnicodeString::SubStr(int32_t Pos, int32_t Len) const
{
  wstring_t Str(Data.Mid(Pos - 1, Len));
  return UnicodeString(Str.c_str(), Str.GetLength());
}

UnicodeString UnicodeString::SubStr(int32_t Pos) const
{
  wstring_t Str(Data.Mid(Pos - 1));
  return UnicodeString(Str.c_str(), Str.GetLength());
}

UnicodeString UnicodeString::SubString(int32_t Pos, int32_t Len) const
{
  return SubStr(Pos, Len);
}

UnicodeString UnicodeString::SubString(int32_t Pos) const
{
  return SubStr(Pos);
}

bool UnicodeString::IsDelimiter(const UnicodeString & Chars, int32_t Pos) const
{
  return ::IsDelimiter(Chars, *this, Pos);
}

int32_t UnicodeString::LastDelimiter(const UnicodeString & Delimiters) const
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

void UnicodeString::Unique()
{
  Init(Data.c_str(), Data.GetLength());
}

UnicodeString UnicodeString::StringOfChar(const wchar_t Ch, int32_t Len)
{
  return ::StringOfChar(Ch, Len);
}

UnicodeString & UnicodeString::operator =(const UnicodeString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.GetLength());
  return *this;
}

UnicodeString & UnicodeString::operator =(const RawByteString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length(), CP_UTF8);
  return *this;
}

UnicodeString & UnicodeString::operator =(const AnsiString & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length(), CP_THREAD_ACP);
  return *this;
}

UnicodeString & UnicodeString::operator =(const UTF8String & StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length(), CP_UTF8);
  return *this;
}

UnicodeString & UnicodeString::operator =(const wchar_t * Str)
{
  Init(Str, wstring_t::StringLength(Str));
  return *this;
}

UnicodeString & UnicodeString::operator =(const wchar_t Ch)
{
  Init(&Ch, 1);
  return *this;
}

UnicodeString & UnicodeString::operator =(const char * lpszData)
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

UnicodeString & UnicodeString::operator +=(const char * Ch)
{
  Data += Ch;
  return *this;
}

UnicodeString & UnicodeString::operator +=(const wchar_t Ch)
{
  Data += Ch;
  return *this;
}

wchar_t UnicodeString::operator [](int32_t Idx) const
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return Data.operator[](Idx - 1);
}

wchar_t & UnicodeString::operator [](int32_t Idx)
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return Data.GetBuffer()[Idx - 1];
}

void UnicodeString::ThrowIfOutOfRange(int32_t Idx) const
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
  return lhs + UnicodeString(1, rhs);
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
