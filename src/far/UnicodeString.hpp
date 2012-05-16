#pragma once

#include <string>

#include "headers.hpp"
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
  UTF8String(const wchar_t * Str, int Size) { Init(Str, Size); }
  UTF8String(const char * Str, int Size) { Init(Str, Size); }
  UTF8String(const UnicodeString & Str);
  UTF8String(const std::wstring & Str) { Init(Str.c_str(), Str.size()); }

  ~UTF8String() {}

  operator const wchar_t * () const { return c_str(); }
  int size() const { return Length(); }
  const wchar_t * c_str() const { return Data.c_str(); }
  int Length() const { return Data.size(); }
  int GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(int nLength) { Data.resize(nLength); }
  UTF8String & Delete(int Index, int Count) { Data.erase(Index - 1, Count); return *this; }

  UTF8String & Insert(int Pos, const wchar_t * Str, int StrLen) { return Insert(Str, Pos); }
  UTF8String & Insert(const wchar_t * Str, int Pos);

  UTF8String SubString(int Pos, int Len = -1) const { return std::wstring(Data.substr(Pos - 1, Len)); }

  int Pos(wchar_t Ch) const;

public:
  const UTF8String & operator=(const UnicodeString & strCopy);
  const UTF8String & operator=(const UTF8String & strCopy);
  const UTF8String & operator=(const RawByteString & strCopy);
  const UTF8String & operator=(const char * lpszData);
  const UTF8String & operator=(const wchar_t * lpwszData);
  const UTF8String & operator=(wchar_t chData);

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
  void Init(const wchar_t * Str, int Length)
  {
    Data.resize(Length);
    if (Length > 0)
    {
        memmove(const_cast<wchar_t *>(Data.c_str()), Str, Length * sizeof(wchar_t));
    }
    Data = Data.c_str();
  }
  void Init(const char * Str, int Length)
  {
    int Size = MultiByteToWideChar(CP_UTF8, 0, Str, Length > 0 ? Length : -1, NULL, 0);
    Data.resize(Size);
    if (Size > 0)
    {
      MultiByteToWideChar(CP_UTF8, 0, Str, -1, const_cast<wchar_t *>(Data.c_str()), Size);
      Data[Size-1] = 0;
    }
    Data = Data.c_str();
  }

  typedef std::basic_string<wchar_t> wstring_t;
  wstring_t Data;
};

//------------------------------------------------------------------------------

class UnicodeString
{
public:
  UnicodeString() {}
  UnicodeString(const wchar_t * Str) { Init(Str, StrLength(Str)); }
  UnicodeString(const wchar_t * Str, int Size) { Init(Str, Size); }
  UnicodeString(const wchar_t Src) { Init(&Src, 1); }
  UnicodeString(const char * Str, int Size) { Init(Str, Size); }
  UnicodeString(const char * Str) { Init(Str, Str ? strlen(Str) : 0); }
  UnicodeString(int Size, wchar_t Ch) : Data(Size, Ch) {}

  UnicodeString(const UnicodeString & Str) { Init(Str.c_str(), Str.GetLength()); }
  UnicodeString(const UTF8String & Str) { Init(Str.c_str(), Str.GetLength()); }
  UnicodeString(const std::wstring & Str) { Init(Str.c_str(), Str.size()); }

  ~UnicodeString() {}

  int size() const { return Length(); }
  const wchar_t * c_str() const { return Data.c_str(); }
  const wchar_t * data() const { return Data.c_str(); }
  int Length() const { return Data.size(); }
  int GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(int nLength) { Data.resize(nLength); }
  UnicodeString & Delete(int Index, int Count) { Data.erase(Index - 1, Count); return *this; }
  UnicodeString & Clear() { Data.clear(); return *this; }

  UnicodeString & Lower(int nStartPos = 1, int nLength = -1);
  UnicodeString & Upper(int nStartPos = 1, int nLength = -1);

  UnicodeString & LowerCase() { return Lower(); }
  UnicodeString & UpperCase() { return Upper(); }

  int CompareIC(const UnicodeString str) const;
  int ToInt() const;

  UnicodeString & Replace(int Pos, int Len, const wchar_t * Str, int DataLen);
  UnicodeString & Replace(int Pos, int Len, const UnicodeString & Str) { return Replace(Pos, Len, Str.c_str(), Str.GetLength()); }
  UnicodeString & Replace(int Pos, int Len, const wchar_t * Str) { return Replace(Pos, Len, Str, StrLength(NullToEmpty(Str))); }
  UnicodeString & Replace(int Pos, int Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }
  UnicodeString & Replace(int Pos, wchar_t Ch) { return Replace(Pos, 1, &Ch, 1); }

  UnicodeString & Append(const wchar_t * Str, int StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
  UnicodeString & Append(const UnicodeString & Str) { return Append(Str.c_str(), Str.GetLength()); }
  UnicodeString & Append(const wchar_t * Str) { return Append(Str, StrLength(NullToEmpty(Str))); }
  UnicodeString & Append(const wchar_t Ch) { return Append(&Ch, 1); }
  UnicodeString & Append(const char * lpszAdd, UINT CodePage=CP_OEMCP);

  UnicodeString & Insert(int Pos, const wchar_t * Str, int StrLen);
  UnicodeString & Insert(int Pos, const UnicodeString Str) { return Insert(Pos, Str.c_str(), Str.Length()); }
  UnicodeString & Insert(const wchar_t * Str, int Pos) { return Insert(Pos, Str, wcslen(Str)); }
  UnicodeString & Insert(const wchar_t Ch, int Pos) { return Insert(Pos, &Ch, 1); }
  UnicodeString & Insert(const UnicodeString Str, int Pos) { return Insert(Pos, Str); }

  int Pos(wchar_t Ch) const { return (int)Data.find(Ch) + 1; }
  int Pos(UnicodeString Str) const { return (int)Data.find(Str.Data) + 1; }

  int RPos(wchar_t Ch) const { return (int)Data.find_last_of(Ch) + 1; }
  bool RPos(int & nPos, wchar_t Ch, int nStartPos = 0) const;

  UnicodeString SubStr(int Pos, int Len = -1) const;
  UnicodeString SubString(int Pos, int Len = -1) const { return SubStr(Pos, Len); }

  bool IsDelimiter(UnicodeString Chars, int Pos) const;
  int LastDelimiter(const UnicodeString & delimiters) const;

  UnicodeString Trim() const;
  UnicodeString TrimLeft() const;
  UnicodeString TrimRight() const;

  void Unique() {}

public:
  operator std::wstring () const { return Data; }
  operator LPCWSTR () const { return Data.c_str(); }

  const UnicodeString & operator=(const UnicodeString & strCopy);
  const UnicodeString & operator=(const RawByteString & strCopy);
  const UnicodeString & operator=(const AnsiString & strCopy);
  const UnicodeString & operator=(const UTF8String & strCopy);
  const UnicodeString & operator=(const std::wstring & strCopy);
  const UnicodeString & operator=(const wchar_t * lpwszData);
  const UnicodeString & operator=(const char * lpszData);
  const UnicodeString & operator=(const wchar_t Ch);

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

  wchar_t __fastcall operator [](const int idx) const
  {
    ThrowIfOutOfRange(idx);   // Should Range-checking be optional to avoid overhead ??
    return Data[idx-1];
  }

  wchar_t & __fastcall operator [](const int idx)
  {
    ThrowIfOutOfRange(idx);   // Should Range-checking be optional to avoid overhead ??
    Unique();                 // Ensure we're not ref-counted (and Unicode)
    return Data[idx-1];
  }

private:
  void Init(const wchar_t * Str, int Length)
  {
    Data.resize(Length);
    if (Length > 0)
    {
        memmove(const_cast<wchar_t *>(Data.c_str()), Str, Length * sizeof(wchar_t));
    }
    Data = Data.c_str();
  }
  void Init(const char * Str, int Length)
  {
    int Size = MultiByteToWideChar(CP_AUTODETECT, 0, Str, Length > 0 ? Length : -1, NULL, 0);
    Data.resize(Size);
    if (Size > 0)
    {
      MultiByteToWideChar(CP_AUTODETECT, 0, Str, -1, const_cast<wchar_t *>(Data.c_str()), Size);
    }
    Data = Data.c_str();
  }

  void  __cdecl ThrowIfOutOfRange(int idx) const;

  std::wstring Data;
};

//------------------------------------------------------------------------------
class RawByteString;

class AnsiString
{
public:
  AnsiString() {}
  AnsiString(const wchar_t * Str) { Init(Str, StrLength(Str)); }
  AnsiString(const wchar_t * Str, int Size) { Init(Str, Size); }
  AnsiString(const char * Str) { Init(Str, Str ? strlen(Str) : 0); }
  AnsiString(const char * Str, int Size) { Init(Str, Size); }
  AnsiString(const unsigned char * Str) { Init(Str, Str ? strlen(reinterpret_cast<const char *>(Str)) : 0); }
  AnsiString(const unsigned char * Str, int Size) { Init(Str, Size); }
  AnsiString(const UnicodeString & Str) { Init(Str.c_str(), Str.GetLength()); }
  AnsiString(const UTF8String & Str) { Init(Str.c_str(), Str.GetLength()); }
  ~AnsiString() {}

  operator const char * () const { return Data.c_str(); }
  operator UnicodeString() const;
  int size() const { return Data.size(); }
  const char * c_str() const { return Data.c_str(); }
  // const unsigned char * c_str() const { return Data.c_str(); }
  int Length() const { return Data.size(); }
  int GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(int nLength) { Data.resize(nLength); }
  AnsiString & Delete(int Index, int Count) { Data.erase(Index - 1, Count); return *this; }

  AnsiString & Insert(const char * Str, int Pos);

  AnsiString SubString(int Pos, int Len = -1) const;

  int Pos(wchar_t Ch) const;
  int Pos(const wchar_t * Str) const;
public:
  const AnsiString & operator=(const UnicodeString & strCopy);
  const AnsiString & operator=(const RawByteString & strCopy);
  const AnsiString & operator=(const AnsiString & strCopy);
  const AnsiString & operator=(const UTF8String & strCopy);
  const AnsiString & operator=(const std::wstring & strCopy);
  const AnsiString & operator=(const char * lpszData);
  const AnsiString & operator=(const wchar_t * lpwszData);
  const AnsiString & operator=(wchar_t chData);

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

  void Unique() {}

private:
  void Init(const wchar_t * Str, int Length)
  {
    int Size = WideCharToMultiByte(CP_UTF8, 0, Str, Length, nullptr, 0, nullptr, nullptr) + 1;
    Data.resize(Size);
    if (Size > 0)
    {
      WideCharToMultiByte(CP_UTF8, 0, Str, Length,
        reinterpret_cast<LPSTR>(const_cast<char *>(Data.c_str())), Size-1, nullptr, nullptr);
      Data[Size-1] = 0;
    }
    Data = Data.c_str();
  }
  void Init(const char * Str, int Length)
  {
    Data.resize(Length);
    if (Length > 0)
    {
      memmove(const_cast<char *>(Data.c_str()), Str, Length);
    }
    Data = Data.c_str();
  }
  void Init(const unsigned char * Str, int Length)
  {
    Data.resize(Length);
    if (Length > 0)
    {
      memmove(const_cast<char *>(Data.c_str()), Str, Length);
    }
    Data = Data.c_str();
  }

  std::string Data;
};

//------------------------------------------------------------------------------

class RawByteString
{
public:
  RawByteString() {}
  RawByteString(const wchar_t * Str) { Init(Str, StrLength(Str)); }
  RawByteString(const wchar_t * Str, int Size) { Init(Str, Size); }
  RawByteString(const char * Str) { Init(Str, Str ? strlen(Str) : 0); }
  RawByteString(const char * Str, int Size) { Init(Str, Size); }
  RawByteString(const unsigned char * Str) { Init(Str, Str ? strlen(reinterpret_cast<const char *>(Str)) : 0); }
  RawByteString(const unsigned char * Str, int Size) { Init(Str, Size); }
  RawByteString(const UnicodeString & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const RawByteString & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const AnsiString & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const UTF8String & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const std::wstring & Str) { Init(Str.c_str(), Str.size()); }
  ~RawByteString() {}

  operator const char * () const { return reinterpret_cast<const char *>(Data.c_str()); }
  operator UnicodeString() const;
  int size() const { return Data.size(); }
  const char * c_str() const { return reinterpret_cast<const char *>(Data.c_str()); }
  int Length() const { return Data.size(); }
  int GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(int nLength) { Data.resize(nLength); }
  RawByteString & Delete(int Index, int Count) { Data.erase(Index - 1, Count); return *this; }

  RawByteString & Insert(const char * Str, int Pos);

  RawByteString SubString(int Pos, int Len = -1) const;

  int Pos(wchar_t Ch) const;
  int Pos(const wchar_t * Str) const;
  int Pos(const char Ch) const;
  int Pos(const char * Ch) const;
public:
  const RawByteString & operator=(const UnicodeString & strCopy);
  const RawByteString & operator=(const RawByteString & strCopy);
  const RawByteString & operator=(const AnsiString & strCopy);
  const RawByteString & operator=(const UTF8String & strCopy);
  const RawByteString & operator=(const std::wstring & strCopy);
  const RawByteString & operator=(const char * lpszData);
  const RawByteString & operator=(const wchar_t * lpwszData);
  const RawByteString & operator=(wchar_t chData);

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

  bool __fastcall operator ==(char * rhs)
  { return (char *)Data.c_str() == rhs; }
  friend bool __fastcall operator ==(RawByteString & lhs, RawByteString & rhs)
  { return lhs.Data == rhs.Data; }
  friend bool __fastcall operator !=(RawByteString & lhs, RawByteString & rhs)
  { return lhs.Data != rhs.Data; }

  void Unique() {}

private:
  void Init(const wchar_t * Str, int Length)
  {
    int Size = WideCharToMultiByte(CP_UTF8, 0, Str, Length, nullptr, 0, nullptr, nullptr) + 1;
    Data.resize(Size);
    if (Size > 0)
    {
      WideCharToMultiByte(CP_UTF8, 0, Str, Length,
        reinterpret_cast<LPSTR>(const_cast<unsigned char *>(Data.c_str())), Size-1, nullptr, nullptr);
      Data.resize(Size - 1);
    }
  }
  void Init(const char * Str, int Length)
  {
    Data.resize(Length);
    if (Length > 0)
    {
      memmove(const_cast<unsigned char *>(Data.c_str()), Str, Length);
      // Data[Length-1] = 0;
    }
  }
  void Init(const unsigned char * Str, int Length)
  {
    Data.resize(Length);
    if (Length > 0)
    {
      memmove(const_cast<unsigned char *>(Data.c_str()), Str, Length);
      // Data[Length-1] = 0;
    }
  }

  typedef std::basic_string<unsigned char> rawstring_t;
  rawstring_t Data;
};

//------------------------------------------------------------------------------

