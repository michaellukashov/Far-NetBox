#pragma once

#include <string>

#include "local.hpp"

//------------------------------------------------------------------------------

class RawByteString;
class UnicodeString;
class AnsiString;

class UTF8String
{
public:
  UTF8String() {}
  UTF8String(const wchar_t * Str) { Init(Str, StrLength(Str)); }
  UTF8String(const wchar_t * Str, intptr_t Size) { Init(Str, Size); }
  UTF8String(const char * Str, intptr_t Size) { Init(Str, Size); }
  UTF8String(const UnicodeString & Str);
  UTF8String(const std::wstring & Str) { Init(Str.c_str(), Str.size()); }

  ~UTF8String() {}

  operator const wchar_t * () const { return c_str(); }
  intptr_t size() const { return Length(); }
  const wchar_t * c_str() const { return Data.c_str(); }
  intptr_t Length() const { return Data.size(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(intptr_t nLength) { Data.resize(nLength); }
  UTF8String & Delete(intptr_t Index, intptr_t Count) { Data.erase(Index - 1, Count); return *this; }

  UTF8String & Insert(intptr_t Pos, const wchar_t * Str, intptr_t StrLen) { return Insert(Str, Pos); }
  UTF8String & Insert(const wchar_t * Str, intptr_t Pos);

  UTF8String SubString(intptr_t Pos, intptr_t Len = -1) const { return std::wstring(Data.substr(Pos - 1, Len)); }

  intptr_t Pos(wchar_t Ch) const;

public:
  UTF8String & operator=(const UnicodeString & strCopy);
  UTF8String & operator=(const UTF8String & strCopy);
  UTF8String & operator=(const RawByteString & strCopy);
  UTF8String & operator=(const char * lpszData);
  UTF8String & operator=(const wchar_t * lpwszData);
  UTF8String & operator=(wchar_t chData);

  UTF8String __fastcall operator +(const UTF8String & rhs) const;
  UTF8String __fastcall operator +(const std::wstring & rhs) const;
  UTF8String __fastcall operator +(const RawByteString & rhs) const;
  const UTF8String & __fastcall operator +=(const UTF8String & rhs);
  const UTF8String & __fastcall operator +=(const RawByteString & rhs);
  const UTF8String & __fastcall operator +=(const char rhs);
  const UTF8String & __fastcall operator +=(const char * rhs);

  friend bool __fastcall operator ==(const UTF8String & lhs, const UTF8String & rhs);
  friend bool __fastcall operator !=(const UTF8String & lhs, const UTF8String & rhs);

private:
  void Init(const wchar_t * Str, intptr_t Length);
  void Init(const char * Str, intptr_t Length);

  typedef std::basic_string<wchar_t> wstring_t;
  wstring_t Data;
};

//------------------------------------------------------------------------------

class UnicodeString
{
public:
  UnicodeString() {}
  UnicodeString(const wchar_t * Str) { Init(Str, StrLength(Str)); }
  UnicodeString(const wchar_t * Str, intptr_t Size) { Init(Str, Size); }
  UnicodeString(const wchar_t Src) { Init(&Src, 1); }
  UnicodeString(const char * Str, intptr_t Size) { Init(Str, Size); }
  UnicodeString(const char * Str) { Init(Str, Str ? strlen(Str) : 0); }
  UnicodeString(intptr_t Size, wchar_t Ch) : Data(Size, Ch) {}

  UnicodeString(const UnicodeString & Str) { Init(Str.c_str(), Str.GetLength()); }
  UnicodeString(const UTF8String & Str) { Init(Str.c_str(), Str.GetLength()); }
  UnicodeString(const std::wstring & Str) { Init(Str.c_str(), Str.size()); }

  ~UnicodeString() {}

  intptr_t size() const { return Length(); }
  const wchar_t * c_str() const { return Data.c_str(); }
  const wchar_t * data() const { return Data.c_str(); }
  intptr_t Length() const { return Data.size(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(intptr_t nLength) { Data.resize(nLength); }
  UnicodeString & Delete(intptr_t Index, intptr_t Count) { Data.erase(Index - 1, Count); return *this; }
  UnicodeString & Clear() { Data.clear(); return *this; }

  UnicodeString & Lower(int nStartPos = 1, int nLength = -1);
  UnicodeString & Upper(int nStartPos = 1, int nLength = -1);

  UnicodeString & LowerCase() { return Lower(); }
  UnicodeString & UpperCase() { return Upper(); }

  int Compare(const UnicodeString & Str) const;
  int CompareIC(const UnicodeString & Str) const;
  int ToInt() const;

  UnicodeString & Replace(intptr_t Pos, intptr_t Len, const wchar_t * Str, intptr_t DataLen);
  UnicodeString & Replace(intptr_t Pos, intptr_t Len, const UnicodeString & Str) { return Replace(Pos, Len, Str.c_str(), Str.GetLength()); }
  UnicodeString & Replace(intptr_t Pos, intptr_t Len, const wchar_t * Str) { return Replace(Pos, Len, Str, StrLength(NullToEmpty(Str))); }
  UnicodeString & Replace(intptr_t Pos, intptr_t Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }
  UnicodeString & Replace(intptr_t Pos, wchar_t Ch) { return Replace(Pos, 1, &Ch, 1); }

  UnicodeString & Append(const wchar_t * Str, intptr_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
  UnicodeString & Append(const UnicodeString & Str) { return Append(Str.c_str(), Str.GetLength()); }
  UnicodeString & Append(const wchar_t * Str) { return Append(Str, StrLength(NullToEmpty(Str))); }
  UnicodeString & Append(const wchar_t Ch) { return Append(&Ch, 1); }
  UnicodeString & Append(const char * lpszAdd, UINT CodePage=CP_OEMCP);

  UnicodeString & Insert(intptr_t Pos, const wchar_t * Str, intptr_t StrLen);
  UnicodeString & Insert(intptr_t Pos, const UnicodeString & Str) { return Insert(Pos, Str.c_str(), Str.Length()); }
  UnicodeString & Insert(const wchar_t * Str, intptr_t Pos) { return Insert(Pos, Str, wcslen(NullToEmpty(Str))); }
  UnicodeString & Insert(const wchar_t Ch, intptr_t Pos) { return Insert(Pos, &Ch, 1); }
  UnicodeString & Insert(const UnicodeString & Str, intptr_t Pos) { return Insert(Pos, Str); }

  intptr_t Pos(wchar_t Ch) const { return Data.find(Ch) + 1; }
  intptr_t Pos(UnicodeString Str) const { return Data.find(Str.Data) + 1; }

  intptr_t RPos(wchar_t Ch) const { return Data.find_last_of(Ch) + 1; }
  bool RPos(intptr_t & nPos, wchar_t Ch, intptr_t nStartPos = 0) const;

  UnicodeString SubStr(intptr_t Pos, intptr_t Len = -1) const;
  UnicodeString SubString(intptr_t Pos, intptr_t Len = -1) const { return SubStr(Pos, Len); }

  bool IsDelimiter(UnicodeString Chars, intptr_t Pos) const;
  intptr_t LastDelimiter(const UnicodeString & Delimiters) const;

  UnicodeString Trim() const;
  UnicodeString TrimLeft() const;
  UnicodeString TrimRight() const;

  void Unique() const {}
  void sprintf(const wchar_t * fmt, ...);

public:
  operator std::wstring () const { return Data; }
  operator LPCWSTR () const { return Data.c_str(); }

  UnicodeString & operator=(const UnicodeString & strCopy);
  UnicodeString & operator=(const RawByteString & strCopy);
  UnicodeString & operator=(const AnsiString & strCopy);
  UnicodeString & operator=(const UTF8String & strCopy);
  UnicodeString & operator=(const std::wstring & strCopy);
  UnicodeString & operator=(const wchar_t * lpwszData);
  UnicodeString & operator=(const char * lpszData);
  UnicodeString & operator=(const wchar_t Ch);

  UnicodeString __fastcall operator +(const UnicodeString & rhs) const;
  UnicodeString __fastcall operator +(const RawByteString & rhs) const;
  UnicodeString __fastcall operator +(const AnsiString & rhs) const;
  UnicodeString __fastcall operator +(const UTF8String & rhs) const;
  UnicodeString __fastcall operator +(const std::wstring & rhs) const;

  friend UnicodeString __fastcall operator +(const wchar_t lhs, const UnicodeString & rhs);
  friend UnicodeString __fastcall operator +(const UnicodeString & lhs, wchar_t rhs);
  friend UnicodeString __fastcall operator +(const wchar_t * lhs, const UnicodeString & rhs);
  friend UnicodeString __fastcall operator +(const UnicodeString & lhs, const wchar_t * rhs);
  friend UnicodeString __fastcall operator +(const UnicodeString & lhs, const char * rhs);

  const UnicodeString & __fastcall operator +=(const UnicodeString & rhs);
  const UnicodeString & __fastcall operator +=(const wchar_t * rhs);
  const UnicodeString & __fastcall operator +=(const UTF8String & rhs);
  const UnicodeString & __fastcall operator +=(const RawByteString & rhs);
  const UnicodeString & __fastcall operator +=(const std::wstring & rhs);
  const UnicodeString & __fastcall operator +=(const char rhs);
  const UnicodeString & __fastcall operator +=(const char * rhs);
  const UnicodeString & __fastcall operator +=(const wchar_t rhs);

  bool operator ==(const UnicodeString & Str) const { return Data == Str.Data; }
  bool operator ==(const wchar_t * Str) const { return wcscmp(Data.c_str(), Str) == 0; }
  bool operator !=(const UnicodeString & Str) const { return Data != Str.Data; }
  bool operator !=(const wchar_t * Str) const { return wcscmp(Data.c_str(), Str) != 0; }

  wchar_t __fastcall operator [](intptr_t Idx) const
  {
    ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
    return Data[Idx-1];
  }

  wchar_t & __fastcall operator [](intptr_t Idx)
  {
    ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
    Unique();                 // Ensure we're not ref-counted (and Unicode)
    return Data[Idx-1];
  }

private:
  void Init(const wchar_t * Str, intptr_t Length);
  void Init(const char * Str, intptr_t Length);
  void  __cdecl ThrowIfOutOfRange(intptr_t Idx) const;

  std::wstring Data;
};

//------------------------------------------------------------------------------
class RawByteString;

class AnsiString
{
public:
  AnsiString() {}
  AnsiString(const wchar_t * Str) { Init(Str, StrLength(Str)); }
  AnsiString(const wchar_t * Str, intptr_t Size) { Init(Str, Size); }
  AnsiString(const char * Str) { Init(Str, Str ? strlen(Str) : 0); }
  AnsiString(const char * Str, intptr_t Size) { Init(Str, Size); }
  AnsiString(const unsigned char * Str) { Init(Str, Str ? strlen(reinterpret_cast<const char *>(Str)) : 0); }
  AnsiString(const unsigned char * Str, intptr_t Size) { Init(Str, Size); }
  AnsiString(const UnicodeString & Str) { Init(Str.c_str(), Str.GetLength()); }
  AnsiString(const UTF8String & Str) { Init(Str.c_str(), Str.GetLength()); }
  ~AnsiString() {}

  operator const char * () const { return Data.c_str(); }
  operator UnicodeString() const;
  operator std::string() const { return std::string(operator const char *()); }
  intptr_t size() const { return Data.size(); }
  const char * c_str() const { return Data.c_str(); }
  intptr_t Length() const { return Data.size(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(intptr_t nLength) { Data.resize(nLength); }
  AnsiString & Delete(intptr_t Index, intptr_t Count) { Data.erase(Index - 1, Count); return *this; }
  AnsiString & Clear() { Data.clear(); return *this; }

  AnsiString & Insert(const char * Str, intptr_t Pos);

  AnsiString SubString(intptr_t Pos, intptr_t Len = -1) const;

  intptr_t Pos(wchar_t Ch) const;
  intptr_t Pos(const wchar_t * Str) const;

  char __fastcall operator [](intptr_t Idx) const
  {
    ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
    return Data[Idx-1];
  }

  char & __fastcall operator [](intptr_t Idx)
  {
    ThrowIfOutOfRange(Idx);   // Should Range-checking be optional to avoid overhead ??
    Unique();                 // Ensure we're not ref-counted (and Unicode)
    return Data[Idx-1];
  }

  AnsiString & Append(const char * Str, intptr_t StrLen) { Data.append(Str, StrLen); return *this; }
  AnsiString & Append(const AnsiString & Str) { return Append(Str.c_str(), Str.GetLength()); }
  AnsiString & Append(const char * Str) { return Append(Str, strlen(Str ? Str : "")); }
  AnsiString & Append(const char Ch) { return Append(&Ch, 1); }

public:
  AnsiString & operator=(const UnicodeString & strCopy);
  AnsiString & operator=(const RawByteString & strCopy);
  AnsiString & operator=(const AnsiString & strCopy);
  AnsiString & operator=(const UTF8String & strCopy);
  AnsiString & operator=(const std::wstring & strCopy);
  AnsiString & operator=(const char * lpszData);
  AnsiString & operator=(const wchar_t * lpwszData);
  AnsiString & operator=(wchar_t chData);

  AnsiString __fastcall operator +(const UnicodeString & rhs) const;
  AnsiString __fastcall operator +(const RawByteString & rhs) const;
  AnsiString __fastcall operator +(const AnsiString & rhs) const;
  AnsiString __fastcall operator +(const UTF8String & rhs) const;
  AnsiString __fastcall operator +(const std::wstring & rhs) const;

  const AnsiString & __fastcall operator +=(const UnicodeString & rhs);
  const AnsiString & __fastcall operator +=(const RawByteString & rhs);
  const AnsiString & __fastcall operator +=(const AnsiString & rhs);
  const AnsiString & __fastcall operator +=(const UTF8String & rhs);
  const AnsiString & __fastcall operator +=(const char Ch);
  const AnsiString & __fastcall operator +=(const char * rhs);

  friend bool __fastcall operator ==(const AnsiString & lhs, const AnsiString & rhs)
  { return lhs.Data == rhs.Data; }
  friend bool __fastcall operator !=(const AnsiString & lhs, const AnsiString & rhs)
  { return lhs.Data != rhs.Data; }

  void Unique() const {}

private:
  void Init(const wchar_t * Str, intptr_t Length);
  void Init(const char * Str, intptr_t Length);
  void Init(const unsigned char * Str, intptr_t Length);
  void  __cdecl ThrowIfOutOfRange(intptr_t Idx) const;

  std::string Data;
};

//------------------------------------------------------------------------------

class RawByteString
{
public:
  RawByteString() {}
  RawByteString(const wchar_t * Str) { Init(Str, StrLength(Str)); }
  RawByteString(const wchar_t * Str, intptr_t Size) { Init(Str, Size); }
  RawByteString(const char * Str) { Init(Str, Str ? strlen(Str) : 0); }
  RawByteString(const char * Str, intptr_t Size) { Init(Str, Size); }
  RawByteString(const unsigned char * Str) { Init(Str, Str ? strlen(reinterpret_cast<const char *>(Str)) : 0); }
  RawByteString(const unsigned char * Str, intptr_t Size) { Init(Str, Size); }
  RawByteString(const UnicodeString & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const RawByteString & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const AnsiString & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const UTF8String & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const std::wstring & Str) { Init(Str.c_str(), Str.size()); }
  ~RawByteString() {}

  operator const char * () const { return reinterpret_cast<const char *>(Data.c_str()); }
  operator UnicodeString() const;
  intptr_t size() const { return Data.size(); }
  const char * c_str() const { return reinterpret_cast<const char *>(Data.c_str()); }
  intptr_t Length() const { return Data.size(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(intptr_t nLength) { Data.resize(nLength); }
  RawByteString & Clear() { SetLength(0); return *this; }
  RawByteString & Delete(intptr_t Index, intptr_t Count) { Data.erase(Index - 1, Count); return *this; }

  RawByteString & Insert(const char * Str, intptr_t Pos);

  RawByteString SubString(intptr_t Pos, intptr_t Len = -1) const;

  intptr_t Pos(wchar_t Ch) const;
  intptr_t Pos(const wchar_t * Str) const;
  intptr_t Pos(const char Ch) const;
  intptr_t Pos(const char * Ch) const;

public:
  RawByteString & operator=(const UnicodeString & strCopy);
  RawByteString & operator=(const RawByteString & strCopy);
  RawByteString & operator=(const AnsiString & strCopy);
  RawByteString & operator=(const UTF8String & strCopy);
  RawByteString & operator=(const std::wstring & strCopy);
  RawByteString & operator=(const char * lpszData);
  RawByteString & operator=(const wchar_t * lpwszData);
  RawByteString & operator=(wchar_t chData);

  RawByteString __fastcall operator +(const UnicodeString & rhs) const;
  RawByteString __fastcall operator +(const RawByteString & rhs) const;
  RawByteString __fastcall operator +(const AnsiString & rhs) const;
  RawByteString __fastcall operator +(const UTF8String & rhs) const;
  RawByteString __fastcall operator +(const std::wstring & rhs) const;

  const RawByteString & __fastcall operator +=(const UnicodeString & rhs);
  const RawByteString & __fastcall operator +=(const RawByteString & rhs);
  const RawByteString & __fastcall operator +=(const AnsiString & rhs);
  const RawByteString & __fastcall operator +=(const UTF8String & rhs);
  const RawByteString & __fastcall operator +=(const std::wstring & rhs);
  const RawByteString & __fastcall operator +=(const char Ch);
  const RawByteString & __fastcall operator +=(const char * rhs);

  bool __fastcall operator ==(char * rhs) const
  { return (char *)Data.c_str() == rhs; }
  friend bool __fastcall operator ==(RawByteString & lhs, RawByteString & rhs)
  { return lhs.Data == rhs.Data; }
  friend bool __fastcall operator !=(RawByteString & lhs, RawByteString & rhs)
  { return lhs.Data != rhs.Data; }

  void Unique() const {}

private:
  void Init(const wchar_t * Str, intptr_t Length);
  void Init(const char * Str, intptr_t Length);
  void Init(const unsigned char * Str, intptr_t Length);

  typedef std::basic_string<unsigned char> rawstring_t;
  rawstring_t Data;
};

//------------------------------------------------------------------------------

