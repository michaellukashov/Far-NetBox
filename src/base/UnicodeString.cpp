
#include <vcl.h>
#pragma hdrstop

#include <Sysutils.hpp>
#include "UnicodeString.hpp"


AnsiString::AnsiString(const AnsiString &rhs) :
  BaseT(rhs.c_str(), ToInt(rhs.Length()))
{
}

AnsiString::AnsiString(const wchar_t *Str) :
  BaseT(Str, BaseT::StringLength(Str), CP_UTF8)
{
}

AnsiString::AnsiString(const wchar_t *Str, intptr_t Length) :
  BaseT(Str, ToInt(Length), CP_UTF8)
{
}

AnsiString::AnsiString(const wchar_t *Str, intptr_t Length, int CodePage) :
  BaseT(Str, ToInt(Length), CodePage)
{
}

AnsiString::AnsiString(const char *Str) :
  BaseT(Str, BaseT::StringLength(Str))
{
}

AnsiString::AnsiString(const char *Str, intptr_t Length) :
  BaseT(Str, ToInt(Length))
{
}

AnsiString::AnsiString(const unsigned char *Str) :
  BaseT(reinterpret_cast<const char *>(Str), BaseT::StringLength(reinterpret_cast<const char *>(Str)))
{
}

AnsiString::AnsiString(const unsigned char *Str, intptr_t Length) :
  BaseT(reinterpret_cast<const char *>(Str), ToInt(Length))
{
}

AnsiString::AnsiString(const UnicodeString &Str) :
  BaseT(Str.c_str(), ToInt(Str.Length()))
{
}

AnsiString::AnsiString(const UTF8String &Str) :
  BaseT(Str.c_str(), ToInt(Str.GetLength()))
{
}

AnsiString::AnsiString(const RawByteString &Str) :
  BaseT(Str.c_str(), ToInt(Str.GetLength()))
{
}

char *AnsiString::SetLength(intptr_t nLength)
{
  return BaseT::GetBufferSetLength(ToInt(nLength));
}

AnsiString &AnsiString::Delete(intptr_t Index, intptr_t Count)
{
  BaseT::Delete(ToInt(Index) - 1, ToInt(Count));
  return *this;
}

AnsiString &AnsiString::Clear()
{
  BaseT::Empty();
  return *this;
}

void AnsiString::Init(const wchar_t *Str, intptr_t Length)
{
  BaseT::operator=(BaseT(Str, ToInt(Length)));
}

void AnsiString::Init(const char *Str, intptr_t Length)
{
  BaseT::operator=(BaseT(Str, ToInt(Length)));
}

void AnsiString::Init(const unsigned char *Str, intptr_t Length)
{
  BaseT::operator=(BaseT(reinterpret_cast<const char *>(Str), ToInt(Length)));
}

intptr_t AnsiString::Pos(const AnsiString &Str) const
{
  return ToIntPtr(BaseT::Find(Str.c_str())) + 1;
}

intptr_t AnsiString::Pos(char Ch) const
{
  return ToIntPtr(BaseT::Find(Ch)) + 1;
}

char AnsiString::operator[](intptr_t Idx) const
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return BaseT::operator[](ToInt(Idx) - 1);
}

char &AnsiString::operator[](intptr_t Idx)
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return BaseT::GetBuffer()[Idx - 1];
}

AnsiString &AnsiString::Append(const char *Str, intptr_t StrLen)
{
  BaseT::Append(Str, ToInt(StrLen));
  return *this;
}

AnsiString &AnsiString::Append(const AnsiString &Str)
{
  return Append(Str.c_str(), Str.GetLength());
}

AnsiString &AnsiString::Append(const char *Str)
{
  return Append(Str, BaseT::StringLength(Str));
}

AnsiString &AnsiString::Append(const char Ch)
{
  BaseT::AppendChar(Ch);
  return *this;
}

AnsiString &AnsiString::Insert(const char *Str, intptr_t Pos)
{
  BaseT::Insert(ToInt(Pos) - 1, Str);
  return *this;
}

AnsiString AnsiString::SubString(intptr_t Pos) const
{
  BaseT Str(BaseT::Mid(ToInt(Pos) - 1));
  return AnsiString(Str.c_str(), Str.GetLength());
}

AnsiString AnsiString::SubString(intptr_t Pos, intptr_t Len) const
{
  BaseT Str(BaseT::Mid(ToInt(Pos) - 1, ToInt(Len)));
  return AnsiString(Str.c_str(), Str.GetLength());
}

AnsiString &AnsiString::operator=(const UnicodeString &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

AnsiString &AnsiString::operator=(const AnsiString &StrCopy)
{
  if (*this != StrCopy)
  {
    Init(StrCopy.c_str(), StrCopy.Length());
  }
  return *this;
}

AnsiString &AnsiString::operator=(const UTF8String &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.GetLength());
  return *this;
}

AnsiString &AnsiString::operator=(const char *Str)
{
  Init(Str, BaseT::StringLength(Str));
  return *this;
}

AnsiString &AnsiString::operator=(const wchar_t *Str)
{
  Init(Str, BaseT::StringLength(Str));
  return *this;
}

AnsiString &AnsiString::operator+=(const AnsiString &rhs)
{
  BaseT::Append(rhs.c_str(), ToInt(rhs.Length()));
  return *this;
}

AnsiString &AnsiString::operator+=(const char Ch)
{
  BaseT::AppendChar(Ch);
  return *this;
}

AnsiString &AnsiString::operator+=(const char *rhs)
{
  BaseT::Append(rhs);
  return *this;
}

void AnsiString::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}


RawByteString::RawByteString(const wchar_t *Str) :
  BaseT(Str, BaseT::StringLength(Str))
{
}

RawByteString::RawByteString(const wchar_t *Str, intptr_t Length) :
  BaseT(Str, ToInt(Length))
{
}

RawByteString::RawByteString(const char *Str) :
  BaseT(Str, BaseT::StringLength(Str))
{
}

RawByteString::RawByteString(const char *Str, intptr_t Length) :
  BaseT(Str, ToInt(Length))
{
}

RawByteString::RawByteString(const unsigned char *Str) :
  BaseT(reinterpret_cast<const char *>(Str), BaseT::StringLength(reinterpret_cast<const char *>(Str)))
{
}

RawByteString::RawByteString(const unsigned char *Str, intptr_t Length) :
  BaseT(reinterpret_cast<const char *>(Str), ToInt(Length))
{
}

RawByteString::RawByteString(const UnicodeString &Str) :
  BaseT(Str.c_str(), ToInt(Str.Length()))
{
}

RawByteString::RawByteString(const RawByteString &Str) :
  BaseT(Str.c_str(), ToInt(Str.Length()))
{
}

RawByteString::RawByteString(const AnsiString &Str) :
  BaseT(Str.c_str(), ToInt(Str.Length()))
{
}

RawByteString::RawByteString(const UTF8String &Str) :
  BaseT(Str.c_str(), ToInt(Str.Length()))
{
}

void RawByteString::Init(const wchar_t *Str, intptr_t Length)
{
  BaseT::operator=(BaseT(Str, ToInt(Length)));
}

void RawByteString::Init(const char *Str, intptr_t Length)
{
  BaseT::operator=(BaseT(Str, ToInt(Length)));
}

void RawByteString::Init(const unsigned char *Str, intptr_t Length)
{
  BaseT::operator=(BaseT(reinterpret_cast<const char *>(Str), ToInt(Length)));
}

void RawByteString::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

RawByteString::operator UnicodeString() const
{
  return UnicodeString(reinterpret_cast<const char *>(c_str()), GetLength());
}

intptr_t RawByteString::Pos(wchar_t Ch) const
{
  return BaseT::Find(static_cast<unsigned char>(Ch)) + 1;
}

intptr_t RawByteString::Pos(const char Ch) const
{
  return BaseT::Find(static_cast<unsigned char>(Ch)) + 1;
}

intptr_t RawByteString::Pos(const char *Str) const
{
  return BaseT::Find(reinterpret_cast<const char *>(Str)) + 1;
}

char *RawByteString::SetLength(intptr_t nLength)
{
  return BaseT::GetBufferSetLength(ToInt(nLength));
}

RawByteString &RawByteString::Delete(intptr_t Index, intptr_t Count)
{
  BaseT::Delete(ToInt(Index) - 1, ToInt(Count));
  return *this;
}

RawByteString &RawByteString::Insert(const char *Str, intptr_t Pos)
{
  BaseT::Insert(ToInt(Pos) - 1, static_cast<const char *>(Str));
  return *this;
}

RawByteString RawByteString::SubString(intptr_t Pos) const
{
  BaseT Str(BaseT::Mid(ToInt(Pos) - 1));
  return UTF8String(Str.c_str(), Str.GetLength());
}

RawByteString RawByteString::SubString(intptr_t Pos, intptr_t Len) const
{
  BaseT Str = BaseT::Mid(ToInt(Pos) - 1, ToInt(Len));
  RawByteString Result(Str.c_str(), Str.GetLength());
  return Result;
}

unsigned char RawByteString::operator[](intptr_t Idx) const
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return BaseT::operator[](ToInt(Idx) - 1);
}

unsigned char &RawByteString::operator[](intptr_t Idx)
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return reinterpret_cast<unsigned  char *>(BaseT::GetBuffer())[ToInt(Idx) - 1];
}

RawByteString &RawByteString::operator=(const UnicodeString &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString &RawByteString::operator=(const RawByteString &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString &RawByteString::operator=(const AnsiString &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString &RawByteString::operator=(const UTF8String &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

RawByteString &RawByteString::operator=(const char *lpszData)
{
  Init(lpszData, BaseT::StringLength(lpszData));
  return *this;
}

RawByteString &RawByteString::operator=(const wchar_t *lpwszData)
{
  Init(lpwszData, CMStringW::StringLength(lpwszData));
  return *this;
}

RawByteString RawByteString::operator+(const RawByteString &rhs) const
{
  RawByteString Result = *this;
  Result.Append(rhs.c_str(), ToInt(rhs.GetLength()));
  return Result;
}

RawByteString &RawByteString::operator+=(const RawByteString &rhs)
{
  BaseT::Append(rhs.c_str(), ToInt(rhs.Length()));
  return *this;
}

RawByteString &RawByteString::operator+=(const char Ch)
{
  BaseT::AppendChar(Ch);
  return *this;
}


UTF8String::UTF8String(const UTF8String &rhs) :
  BaseT(rhs.c_str(), ToInt(rhs.Length()))
{
}

UTF8String::UTF8String(const UnicodeString &Str) :
  BaseT(Str.c_str(), ToInt(Str.Length()), CP_UTF8)
{
}

UTF8String::UTF8String(const wchar_t *Str) :
  BaseT(Str, BaseT::StringLength(Str))
{
}

UTF8String::UTF8String(const wchar_t *Str, intptr_t Length) :
  BaseT(Str, ToInt(Length))
{
}

UTF8String::UTF8String(const char *Str, intptr_t Length) :
  BaseT(Str, ToInt(Length))
{
}

UTF8String::UTF8String(const char *Str) :
  BaseT(Str, BaseT::StringLength(Str))
{
}

void UTF8String::Init(const wchar_t *Str, intptr_t Length)
{
  BaseT::operator=(BaseT(Str, ToInt(Length)));
}

void UTF8String::Init(const char *Str, intptr_t Length)
{
  BaseT::operator=(BaseT(Str, ToInt(Length)));
}

char *UTF8String::SetLength(intptr_t nLength)
{
  return BaseT::GetBufferSetLength(ToInt(nLength));
}

UTF8String &UTF8String::Delete(intptr_t Index, intptr_t Count)
{
  BaseT::Delete(ToInt(Index) - 1, ToInt(Count));
  return *this;
}

intptr_t UTF8String::Pos(char Ch) const
{
  return BaseT::Find(Ch) + 1;
}

int UTF8String::vprintf(const char *Format, va_list ArgList)
{
  char *Buf = SetLength(32 * 1024);
  int Size = vsnprintf_s(Buf, ::ToSizeT(GetLength()), _TRUNCATE, Format, ArgList);
  BaseT::Truncate(Size);
  return Size;
}

UTF8String &UTF8String::Insert(wchar_t Ch, intptr_t Pos)
{
  UTF8String UTF8(&Ch, 1);
  BaseT::Insert(ToInt(Pos) - 1, UTF8.c_str());
  return *this;
}

UTF8String &UTF8String::Insert(const wchar_t *Str, intptr_t Pos)
{
  UTF8String UTF8(Str);
  BaseT::Insert(ToInt(Pos) - 1, UTF8.c_str());
  return *this;
}

UTF8String UTF8String::SubString(intptr_t Pos) const
{
  BaseT Str(BaseT::Mid(ToInt(Pos) - 1));
  return UTF8String(Str.c_str(), Str.GetLength());
}

UTF8String UTF8String::SubString(intptr_t Pos, intptr_t Len) const
{
  BaseT Str(BaseT::Mid(ToInt(Pos) - 1, ToInt(Len)));
  return UTF8String(Str.c_str(), Str.GetLength());
}

UTF8String &UTF8String::operator=(const UnicodeString &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String &UTF8String::operator=(const UTF8String &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String &UTF8String::operator=(const RawByteString &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length());
  return *this;
}

UTF8String &UTF8String::operator=(const char *lpszData)
{
  Init(lpszData, BaseT::StringLength(lpszData));
  return *this;
}

UTF8String &UTF8String::operator=(const wchar_t *lpwszData)
{
  Init(lpwszData, BaseT::StringLength(lpwszData));
  return *this;
}

UTF8String UTF8String::operator+(const UTF8String &rhs) const
{
  BaseT Result(*this);
  Result += rhs;
  return UTF8String(Result);
}

UTF8String UTF8String::operator+(const char *rhs) const
{
  BaseT Result(*this);
  Result += rhs;
  return UTF8String(Result);
}

UTF8String &UTF8String::operator+=(const UTF8String &rhs)
{
  BaseT::Append(rhs.c_str(), ToInt(rhs.Length()));
  return *this;
}

UTF8String &UTF8String::operator+=(const RawByteString &rhs)
{
  UTF8String Str(rhs.c_str(), rhs.Length());
  BaseT::Append(Str.c_str(), ToInt(Str.Length()));
  return *this;
}

UTF8String &UTF8String::operator+=(const char Ch)
{
  BaseT::AppendChar(Ch);
  return *this;
}

UTF8String &UTF8String::operator+=(const char *rhs)
{
  BaseT::Append(rhs);
  return *this;
}

char UTF8String::operator[](intptr_t Idx) const
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return BaseT::operator[](ToInt(Idx) - 1);
}

char &UTF8String::operator[](intptr_t Idx)
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return BaseT::GetBuffer()[ToInt(Idx) - 1];
}

void UTF8String::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}

UnicodeString::UnicodeString(const UnicodeString &Str) :
  BaseT(Str.c_str(), ToInt(Str.GetLength()))
{
}

UnicodeString::UnicodeString(const UTF8String &Str) :
  BaseT(Str.c_str(), ToInt(Str.GetLength()), CP_UTF8)
{
}

UnicodeString::UnicodeString(const wchar_t *Str) :
  BaseT(Str, BaseT::StringLength(Str))
{
}

UnicodeString::UnicodeString(const wchar_t *Str, intptr_t Length) :
  BaseT(Str, ToInt(Length))
{
}

UnicodeString::UnicodeString(const wchar_t Src) :
  BaseT(&Src, 1)
{
}

UnicodeString::UnicodeString(const char *Str, intptr_t Length) :
  BaseT(Str, ToInt(Length))
{
}

UnicodeString::UnicodeString(const char *Str, intptr_t Length, int CodePage) :
  BaseT(Str, ToInt(Length), CodePage)
{
}

UnicodeString::UnicodeString(const char *Str) :
  BaseT(Str, BaseT::StringLength(Str))
{
}

UnicodeString::UnicodeString(const AnsiString &Str) :
  BaseT(Str.c_str(), ToInt(Str.Length()))
{
}

void UnicodeString::Init(const wchar_t *Str, intptr_t Length)
{
  BaseT::operator=(BaseT(Str, ToInt(Length)));
}

void UnicodeString::Init(const char *Str, intptr_t Length, int CodePage)
{
  BaseT::operator=(BaseT(Str, ToInt(Length), CodePage));
}

//wchar_t *UnicodeString::SetLength(intptr_t nLength)
//{
//  return BaseT::GetBufferSetLength(ToInt(nLength));
//}

//UnicodeString &UnicodeString::Delete(intptr_t Index, intptr_t Count)
//{
//  BaseT::Delete(ToInt(Index) - 1, ToInt(Count));
//  return *this;
//}

//UnicodeString &UnicodeString::Lower(intptr_t nStartPos)
//{
//  *this = ::LowerCase(SubString(nStartPos)).c_str();
//  return *this;
//}

//UnicodeString &UnicodeString::Lower(intptr_t nStartPos, intptr_t nLength)
//{
//  *this = ::LowerCase(SubString(nStartPos, nLength)).c_str();
//  return *this;
//}

//UnicodeString &UnicodeString::Upper(intptr_t nStartPos)
//{
//  *this = ::UpperCase(SubString(nStartPos)).c_str();
//  return *this;
//}

//UnicodeString &UnicodeString::Upper(intptr_t nStartPos, intptr_t nLength)
//{
//  *this = ::UpperCase(SubString(nStartPos, nLength)).c_str();
//  return *this;
//}

intptr_t UnicodeString::CompareIC(const UnicodeString &Str) const
{
  return ::AnsiCompareIC(*this, Str);
}

intptr_t UnicodeString::ToIntPtr() const
{
  return ::StrToIntDef(*this, 0);
}

//intptr_t UnicodeString::FindFirstOf(const wchar_t Ch) const
//{
//  return ::ToIntPtr(BaseT::Find(Ch, 0)) + 1;
//}

//intptr_t UnicodeString::FindFirstOf(const wchar_t *AStr, size_t Offset) const
//{
//  if (!AStr || !*AStr)
//  {
//    return NPOS;
//  }
//  // int Length = BaseT::StringLength(Str);
//  UnicodeString Str = BaseT::Mid(ToInt(Offset));
//  int Res = Str.FindOneOf(AStr);
//  if (Res != -1)
//    return Res + Offset;
//  return NPOS;
//}

//UnicodeString &UnicodeString::Replace(intptr_t Pos, intptr_t Len, const wchar_t *Str, intptr_t DataLen)
//{
//  BaseT::Delete(ToInt(Pos) - 1, ToInt(Len));
//  BaseT::Insert(ToInt(Pos) - 1, UnicodeString(Str, ToInt(DataLen)).c_str());
//  return *this;
//}

//UnicodeString &UnicodeString::Replace(intptr_t Pos, intptr_t Len, const wchar_t *Str)
//{
//  return Replace(Pos, Len, Str, BaseT::StringLength(Str));
//}

//UnicodeString &UnicodeString::Append(const wchar_t *Str)
//{
//  return Append(Str, BaseT::StringLength(Str));
//}

//UnicodeString &UnicodeString::Append(const char *lpszAdd, UINT CodePage)
//{
//  BaseT::Append(BaseT(lpszAdd, BaseT::StringLength(lpszAdd), CodePage));
//  return *this;
//}

//UnicodeString &UnicodeString::Insert(intptr_t Pos, const wchar_t *Str, intptr_t StrLen)
//{
//  BaseT::Insert(ToInt(Pos) - 1, UnicodeString(Str, ToInt(StrLen)).c_str());
//  return *this;
//}

//UnicodeString &UnicodeString::Insert(const wchar_t *Str, intptr_t Pos)
//{
//  return Insert(Pos, Str, BaseT::StringLength(Str));
//}

//intptr_t UnicodeString::Pos(wchar_t Ch) const
//{
//  return BaseT::Find(Ch) + 1;
//}

//intptr_t UnicodeString::Pos(const UnicodeString &Str) const
//{
//  return BaseT::Find(Str.c_str()) + 1;
//}

//bool UnicodeString::RPos(intptr_t &nPos, wchar_t Ch, intptr_t /*nStartPos*/) const
//{
//  int Pos = BaseT::ReverseFind(Ch); //, Data.size() - nStartPos);
//  nPos = Pos + 1;
//  return Pos != -1;
//}

UnicodeString UnicodeString::SubStr(intptr_t Pos, intptr_t Len) const
{
  UnicodeString Str(BaseT::Mid(ToInt(Pos) - 1, ToInt(Len)));
  return Str; // UnicodeString(Str.c_str(), Str.GetLength());
}

UnicodeString UnicodeString::SubStr(intptr_t Pos) const
{
  UnicodeString Str(BaseT::Mid(ToInt(Pos) - 1));
  return Str; // UnicodeString(Str.c_str(), Str.GetLength());
}

UnicodeString UnicodeString::SubString(intptr_t Pos, intptr_t Len) const
{
  return SubStr(Pos, Len);
}

UnicodeString UnicodeString::SubString(intptr_t Pos) const
{
  return SubStr(Pos);
}

//bool UnicodeString::IsDelimiter(const UnicodeString &Chars, intptr_t Pos) const
//{
//  return ::IsDelimiter(Chars, *this, Pos);
//}

//intptr_t UnicodeString::LastDelimiter(const UnicodeString &Delimiters) const
//{
//  return ::LastDelimiter(Delimiters, *this);
//}

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

//void UnicodeString::Unique()
//{
//  Init(c_str(), GetLength());
//}

UnicodeString &UnicodeString::operator=(const UnicodeString &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.GetLength());
  return *this;
}

UnicodeString &UnicodeString::operator=(const RawByteString &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length(), CP_UTF8);
  return *this;
}

UnicodeString &UnicodeString::operator=(const AnsiString &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length(), CP_THREAD_ACP);
  return *this;
}

UnicodeString &UnicodeString::operator=(const UTF8String &StrCopy)
{
  Init(StrCopy.c_str(), StrCopy.Length(), CP_UTF8);
  return *this;
}

UnicodeString &UnicodeString::operator=(const wchar_t *Str)
{
  Init(Str, BaseT::StringLength(Str));
  return *this;
}

UnicodeString &UnicodeString::operator=(const wchar_t Ch)
{
  Init(&Ch, 1);
  return *this;
}

UnicodeString &UnicodeString::operator=(const char *lpszData)
{
  Init(lpszData, BaseT::StringLength(lpszData), CP_UTF8);
  return *this;
}

UnicodeString UnicodeString::operator+(const UnicodeString &rhs) const
{
  UnicodeString Result(*this);
  Result += rhs;
  return Result;
}

UnicodeString &UnicodeString::operator+=(const UnicodeString &rhs)
{
  BaseT::Append(rhs.c_str(), ToInt(rhs.Length()));
  return *this;
}

UnicodeString &UnicodeString::operator+=(const wchar_t *rhs)
{
  BaseT::Append(rhs);
  return *this;
}

UnicodeString &UnicodeString::operator+=(const RawByteString &rhs)
{
  UnicodeString Str(rhs.c_str(), rhs.Length());
  BaseT::Append(Str.c_str(), ToInt(Str.Length()));
  return *this;
}

UnicodeString &UnicodeString::operator+=(const char Ch)
{
  BaseT::AppendChar(Ch);
  return *this;
}

UnicodeString &UnicodeString::operator+=(const wchar_t Ch)
{
  BaseT::operator+=(Ch);
  return *this;
}
/*
wchar_t UnicodeString::operator[](intptr_t Idx) const
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return BaseT::operator[](ToInt(Idx) - 1);
}

wchar_t &UnicodeString::operator[](intptr_t Idx)
{
  ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
  return BaseT::GetBuffer()[ToInt(Idx) - 1];
}

void UnicodeString::ThrowIfOutOfRange(intptr_t Idx) const
{
  if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}
*/
UnicodeString operator+(const wchar_t lhs, const UnicodeString &rhs)
{
  return UnicodeString(&lhs, 1) + rhs;
}

UnicodeString operator+(const UnicodeString &lhs, const wchar_t rhs)
{
  return lhs + UnicodeString(rhs);
}

UnicodeString operator+(const wchar_t *lhs, const UnicodeString &rhs)
{
  return UnicodeString(lhs) + rhs;
}

UnicodeString operator+(const UnicodeString &lhs, const wchar_t *rhs)
{
  return lhs + UnicodeString(rhs);
}

UnicodeString operator+(const UnicodeString &lhs, const char *rhs)
{
  return lhs + UnicodeString(rhs);
}
