
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

RawByteString::operator UnicodeString() const
{
  return UnicodeString(reinterpret_cast<const char *>(c_str()), GetLength());
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

RawByteString &RawByteString::operator+=(const RawByteString &rhs)
{
  BaseT::Append(rhs.data(), ToInt(rhs.Length()));
  return *this;
}

RawByteString &RawByteString::operator+=(const char Ch)
{
  BaseT::AppendChar(Ch);
  return *this;
}

RawByteString &RawByteString::operator+=(const char *Str)
{
  BaseT::Append(Str);
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

int UTF8String::vprintf(const char *Format, va_list ArgList)
{
  char *Buf = SetLength(32 * 1024);
  int Size = vsnprintf_s(Buf, ::ToSizeT(GetLength()), _TRUNCATE, Format, ArgList);
  BaseT::Truncate(Size);
  return Size;
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
  BaseT::AppendChar(Ch);
  return *this;
}
