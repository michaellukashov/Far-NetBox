#pragma once

#include <nbstring.h>

template<typename CharT>
class NB_CORE_EXPORT BaseUnicodeString
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  BaseUnicodeString() {}
  ~BaseUnicodeString() {}

  BaseUnicodeString(const BaseUnicodeString &rhs) :
    Data(rhs.c_str(), ToInt(rhs.Length()))
  {}
  BaseUnicodeString(const wchar_t *Str, int Length, int cp)
    Data(Str(), Length, cp)
  {}
  const CharT *c_str() const { return Data.c_str(); }
  const CharT *data() const { return Data.c_str(); }
  intptr_t Length() const { return Data.GetLength(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  CharT *SetLength(intptr_t nLength)
  {
    return Data.GetBufferSetLength(ToInt(nLength));
  }

  BaseUnicodeString &Delete(intptr_t Index, intptr_t Count)
  {
    Data.Delete(ToInt(Index) - 1, ToInt(Count));
    return *this;
  }
  BaseUnicodeString &Clear() { Data.Empty(); return *this; }

  BaseUnicodeString &Lower(intptr_t nStartPos = 1);
  BaseUnicodeString &Lower(intptr_t nStartPos, intptr_t nLength);
  BaseUnicodeString &Upper(intptr_t nStartPos = 1);
  BaseUnicodeString &Upper(intptr_t nStartPos, intptr_t nLength);

  BaseUnicodeString &LowerCase() { return Lower(); }
  BaseUnicodeString &UpperCase() { return Upper(); }
  BaseUnicodeString &MakeUpper() { Data.MakeUpper(); return *this; }
  BaseUnicodeString &MakeLower() { Data.MakeLower(); return *this; }

  intptr_t Compare(const BaseUnicodeString &Str) const;
  intptr_t CompareIC(const BaseUnicodeString &Str) const;
  intptr_t ToIntPtr() const;
  intptr_t FindFirstOf(const wchar_t Ch) const;
  intptr_t FindFirstOf(const wchar_t *Str, size_t Offset = 0) const;
//  intptr_t FindFirstNotOf(const wchar_t * Str) const { return (intptr_t)Data.find_first_not_of(Str); }

  BaseUnicodeString &Replace(intptr_t Pos, intptr_t Len, const wchar_t *Str, intptr_t DataLen);
  BaseUnicodeString &Replace(intptr_t Pos, intptr_t Len, const BaseUnicodeString &Str) { return Replace(Pos, Len, Str.c_str(), Str.GetLength()); }
  BaseUnicodeString &Replace(intptr_t Pos, intptr_t Len, const wchar_t *Str);
  BaseUnicodeString &Replace(intptr_t Pos, intptr_t Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }
  BaseUnicodeString &Replace(intptr_t Pos, wchar_t Ch) { return Replace(Pos, 1, &Ch, 1); }

  BaseUnicodeString &Append(const wchar_t *Str, intptr_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
  BaseUnicodeString &Append(const BaseUnicodeString &Str) { return Append(Str.c_str(), Str.GetLength()); }
  BaseUnicodeString &Append(const wchar_t *Str);
  BaseUnicodeString &Append(const wchar_t Ch) { return Append(&Ch, 1); }
  BaseUnicodeString &Append(const char *lpszAdd, UINT CodePage = CP_OEMCP);

  BaseUnicodeString &Insert(intptr_t Pos, const wchar_t *Str, intptr_t StrLen);
  BaseUnicodeString &Insert(intptr_t Pos, const BaseUnicodeString &Str) { return Insert(Pos, Str.c_str(), Str.Length()); }
  BaseUnicodeString &Insert(const CharT *Str, intptr_t Pos)
  {
    Data.Insert(ToInt(Pos) - 1, Str);
    return *this;
  }
  BaseUnicodeString &Insert(const wchar_t Ch, intptr_t Pos) { return Insert(Pos, &Ch, 1); }
  BaseUnicodeString &Insert(const BaseUnicodeString &Str, intptr_t Pos) { return Insert(Pos, Str); }

  intptr_t Pos(CharT Ch) const
  {
    return Data.Find(Ch) + 1;
  }
  intptr_t Pos(const BaseUnicodeString &Str) const
  {
    return Data.Find(Str.Data.c_str()) + 1;
  }

  intptr_t RPos(wchar_t Ch) const { return (intptr_t)Data.ReverseFind(Ch) + 1; }
  bool RPos(intptr_t &nPos, wchar_t Ch, intptr_t nStartPos = 0) const;

  BaseUnicodeString<CharT> SubStr(intptr_t Pos, intptr_t Len) const
  {
    string_t Str(Data.Mid(ToInt(Pos) - 1), ToInt(Len));
    return BaseUnicodeString<CharT>(Str.c_str(), Str.GetLength());
  }
  BaseUnicodeString SubStr(intptr_t Pos) const
  {
    string_t Str(Data.Mid(ToInt(Pos) - 1));
    return BaseUnicodeString<CharT>(Str.c_str(), Str.GetLength());
  }
  BaseUnicodeString SubString(intptr_t Pos, intptr_t Len) const
  {
    string_t Str(Data.Mid(ToInt(Pos) - 1), ToInt(Len));
    return BaseUnicodeString<CharT>(Str.c_str(), Str.GetLength());
  }
  BaseUnicodeString SubString(intptr_t Pos) const
  {
    string_t Str(Data.Mid(ToInt(Pos) - 1));
    return BaseUnicodeString<CharT>(Str.c_str(), Str.GetLength());
  }

  bool IsDelimiter(const BaseUnicodeString<CharT> &Chars, intptr_t Pos) const;
  intptr_t LastDelimiter(const BaseUnicodeString<CharT> &Delimiters) const;

  BaseUnicodeString<CharT> Trim() const;
  BaseUnicodeString<CharT> TrimLeft() const;
  BaseUnicodeString<CharT> TrimRight() const;

  void Unique();

public:

  friend bool operator==(const BaseUnicodeString<CharT> &lhs, const CharT *rhs)
  {
    return lhs.Data == rhs; // .Compare(NullToEmpty(rhs)) == 0;
  }
  bool operator==(const BaseUnicodeString<CharT> &Str) const { return Data == Str.Data; }
  bool operator!=(const BaseUnicodeString<CharT> &Str) const { return Data != Str.Data; }

  CharT operator[](intptr_t Idx) const
  {
    ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
    return Data.operator[](ToInt(Idx) - 1);
  }
  CharT &operator[](intptr_t Idx)
  {
    ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
    return Data.GetBuffer()[ToInt(Idx) - 1];
  }

private:
  void ThrowIfOutOfRange(intptr_t Idx) const
  {
    if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
      throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
  }
protected:
  typedef CMStringT< CharT, NBChTraitsCRT< CharT > > string_t;
  string_t Data;
};

//template<typename CharT>
//bool operator==(const BaseUnicodeString<CharT> &lhs, const CharT *rhs)
//{
//  return lhs.Data.operator==(NullToEmpty(rhs));
//}
//template<typename CharT>
//bool operator==(const CharT *lhs, const BaseUnicodeString<CharT> &rhs)
//{
//  return rhs.Data.operator==(NullToEmpty(lhs));
//}

class RawByteString;
class UnicodeString;
class AnsiString;

class NB_CORE_EXPORT UTF8String : public BaseUnicodeString<char>
{
CUSTOM_MEM_ALLOCATION_IMPL
  typedef BaseUnicodeString<char> Base;
public:
  UTF8String() {}
  UTF8String(const UTF8String &rhs);
  explicit UTF8String(const UnicodeString &Str);
  UTF8String(const wchar_t *Str);
  explicit UTF8String(const wchar_t *Str, intptr_t Length);
  explicit UTF8String(const char *Str, intptr_t Length);
  explicit UTF8String(const char *Str);

  ~UTF8String() {}

//  operator const char *() const { return this->c_str(); }
//  const char *c_str() const { return Data.c_str(); }
//  intptr_t Length() const { return Data.GetLength(); }
//  intptr_t GetLength() const { return Length(); }
//  bool IsEmpty() const { return Length() == 0; }
//  char *SetLength(intptr_t nLength);
//  UTF8String &Delete(intptr_t Index, intptr_t Count);
//  UTF8String &Insert(wchar_t Ch, intptr_t Pos);
//  UTF8String &Insert(const wchar_t *Str, intptr_t Pos);
//  UTF8String SubString(intptr_t Pos) const;
//  UTF8String SubString(intptr_t Pos, intptr_t Len) const;

//  intptr_t Pos(char Ch) const;

  int vprintf(const char *Format, va_list ArgList);

  void Unique() {}

public:
  UTF8String &operator=(const UnicodeString &StrCopy);
  UTF8String &operator=(const UTF8String &StrCopy);
  UTF8String &operator=(const RawByteString &StrCopy);
  UTF8String &operator=(const char *lpszData);
  UTF8String &operator=(const wchar_t *lpwszData);
  UTF8String &operator=(wchar_t chData);

  UTF8String operator+(const UTF8String &rhs) const;
  UTF8String operator+(const RawByteString &rhs) const;
  UTF8String operator+(const char *rhs) const;
  UTF8String &operator+=(const UTF8String &rhs);
  UTF8String &operator+=(const RawByteString &rhs);
  UTF8String &operator+=(const char Ch);
  UTF8String &operator+=(const char *rhs);

//  NB_CORE_EXPORT friend bool operator==(const UTF8String &lhs, const UTF8String &rhs);
//  NB_CORE_EXPORT friend bool operator!=(const UTF8String &lhs, const UTF8String &rhs);

private:
  void Init(const wchar_t *Str, intptr_t Length);
  void Init(const char *Str, intptr_t Length);

//  typedef CMStringA string_t;
//  string_t Data;
};

class NB_CORE_EXPORT UnicodeString
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  UnicodeString() {}
  UnicodeString(const wchar_t *Str);
  UnicodeString(const wchar_t *Str, intptr_t Length);
  UnicodeString(const wchar_t Src);
  explicit UnicodeString(const char *Str, intptr_t Length);
  explicit UnicodeString(const char *Str, intptr_t Length, int CodePage);
  UnicodeString(const char *Str);
  UnicodeString(intptr_t Length, wchar_t Ch) : Data(Ch, ToInt(Length)) {}

  UnicodeString(const UnicodeString &Str);
  explicit UnicodeString(const UTF8String &Str);
  explicit UnicodeString(const AnsiString &Str);

  ~UnicodeString() {}

  const wchar_t *c_str() const { return Data.c_str(); }
  const wchar_t *data() const { return Data.c_str(); }
  intptr_t Length() const { return Data.GetLength(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  wchar_t *SetLength(intptr_t nLength);
  UnicodeString &Delete(intptr_t Index, intptr_t Count);
  UnicodeString &Clear() { Data.Empty(); return *this; }

  UnicodeString &Lower(intptr_t nStartPos = 1);
  UnicodeString &Lower(intptr_t nStartPos, intptr_t nLength);
  UnicodeString &Upper(intptr_t nStartPos = 1);
  UnicodeString &Upper(intptr_t nStartPos, intptr_t nLength);

  UnicodeString &LowerCase() { return Lower(); }
  UnicodeString &UpperCase() { return Upper(); }
  UnicodeString &MakeUpper() { Data.MakeUpper(); return *this; }
  UnicodeString &MakeLower() { Data.MakeLower(); return *this; }

  intptr_t Compare(const UnicodeString &Str) const;
  intptr_t CompareIC(const UnicodeString &Str) const;
  intptr_t ToIntPtr() const;
  intptr_t FindFirstOf(const wchar_t Ch) const;
  intptr_t FindFirstOf(const wchar_t *Str, size_t Offset = 0) const;
//  intptr_t FindFirstNotOf(const wchar_t * Str) const { return (intptr_t)Data.find_first_not_of(Str); }

  UnicodeString &Replace(intptr_t Pos, intptr_t Len, const wchar_t *Str, intptr_t DataLen);
  UnicodeString &Replace(intptr_t Pos, intptr_t Len, const UnicodeString &Str) { return Replace(Pos, Len, Str.c_str(), Str.GetLength()); }
  UnicodeString &Replace(intptr_t Pos, intptr_t Len, const wchar_t *Str);
  UnicodeString &Replace(intptr_t Pos, intptr_t Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }
  UnicodeString &Replace(intptr_t Pos, wchar_t Ch) { return Replace(Pos, 1, &Ch, 1); }

  UnicodeString &Append(const wchar_t *Str, intptr_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
  UnicodeString &Append(const UnicodeString &Str) { return Append(Str.c_str(), Str.GetLength()); }
  UnicodeString &Append(const wchar_t *Str);
  UnicodeString &Append(const wchar_t Ch) { return Append(&Ch, 1); }
  UnicodeString &Append(const char *lpszAdd, UINT CodePage = CP_OEMCP);

  UnicodeString &Insert(intptr_t Pos, const wchar_t *Str, intptr_t StrLen);
  UnicodeString &Insert(intptr_t Pos, const UnicodeString &Str) { return Insert(Pos, Str.c_str(), Str.Length()); }
  UnicodeString &Insert(const wchar_t *Str, intptr_t Pos);
  UnicodeString &Insert(const wchar_t Ch, intptr_t Pos) { return Insert(Pos, &Ch, 1); }
  UnicodeString &Insert(const UnicodeString &Str, intptr_t Pos) { return Insert(Pos, Str); }

  intptr_t Pos(wchar_t Ch) const;
  intptr_t Pos(const UnicodeString &Str) const;

  intptr_t RPos(wchar_t Ch) const { return (intptr_t)Data.ReverseFind(Ch) + 1; }
  bool RPos(intptr_t &nPos, wchar_t Ch, intptr_t nStartPos = 0) const;

  UnicodeString SubStr(intptr_t Pos, intptr_t Len) const;
  UnicodeString SubStr(intptr_t Pos) const;
  UnicodeString SubString(intptr_t Pos, intptr_t Len) const;
  UnicodeString SubString(intptr_t Pos) const;

  bool IsDelimiter(const UnicodeString &Chars, intptr_t Pos) const;
  intptr_t LastDelimiter(const UnicodeString &Delimiters) const;

  UnicodeString Trim() const;
  UnicodeString TrimLeft() const;
  UnicodeString TrimRight() const;

  void Unique();

public:
  UnicodeString &operator=(const UnicodeString &StrCopy);
  UnicodeString &operator=(const RawByteString &StrCopy);
  UnicodeString &operator=(const AnsiString &StrCopy);
  UnicodeString &operator=(const UTF8String &StrCopy);
  UnicodeString &operator=(const wchar_t *Str);
  UnicodeString &operator=(const char *lpszData);
  UnicodeString &operator=(const wchar_t Ch);

  UnicodeString operator+(const UnicodeString &rhs) const;
  UnicodeString operator+(const RawByteString &rhs) const;
  UnicodeString operator+(const AnsiString &rhs) const;
  UnicodeString operator+(const UTF8String &rhs) const;

  NB_CORE_EXPORT friend UnicodeString operator+(const wchar_t lhs, const UnicodeString &rhs);
  NB_CORE_EXPORT friend UnicodeString operator+(const UnicodeString &lhs, wchar_t rhs);
  NB_CORE_EXPORT friend UnicodeString operator+(const wchar_t *lhs, const UnicodeString &rhs);
  NB_CORE_EXPORT friend UnicodeString operator+(const UnicodeString &lhs, const wchar_t *rhs);
  NB_CORE_EXPORT friend UnicodeString operator+(const UnicodeString &lhs, const char *rhs);

  UnicodeString &operator+=(const UnicodeString &rhs);
  UnicodeString &operator+=(const wchar_t *rhs);
  UnicodeString &operator+=(const UTF8String &rhs);
  UnicodeString &operator+=(const RawByteString &rhs);
  UnicodeString &operator+=(const char Ch);
  UnicodeString &operator+=(const char *Ch);
  UnicodeString &operator+=(const wchar_t Ch);

  bool operator==(const UnicodeString &Str) const { return Data == Str.Data; }
  bool operator!=(const UnicodeString &Str) const { return Data != Str.Data; }

  NB_CORE_EXPORT friend bool operator==(const UnicodeString &lhs, const wchar_t *rhs);
  NB_CORE_EXPORT friend bool operator==(const wchar_t *lhs, const UnicodeString &rhs);
  NB_CORE_EXPORT friend bool operator!=(const UnicodeString &lhs, const wchar_t *rhs);
  NB_CORE_EXPORT friend bool operator!=(const wchar_t *lhs, const UnicodeString &rhs);

  wchar_t operator[](intptr_t Idx) const;
  wchar_t &operator[](intptr_t Idx);

private:
  void Init(const wchar_t *Str, intptr_t Length);
  void Init(const char *Str, intptr_t Length, int CodePage);
  void ThrowIfOutOfRange(intptr_t Idx) const;

  typedef CMStringW wstring_t;
  wstring_t Data;
};

class RawByteString;

class NB_CORE_EXPORT AnsiString
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  AnsiString() {}
  AnsiString(const AnsiString &rhs);
  AnsiString(intptr_t Length, char Ch) : Data(Ch, ToInt(Length)) {}
  explicit AnsiString(const wchar_t *Str);
  explicit AnsiString(const wchar_t *Str, intptr_t Length);
  explicit AnsiString(const wchar_t *Str, intptr_t Length, int CodePage);
  AnsiString(const char *Str);
  explicit AnsiString(const char *Str, intptr_t Length);
  explicit AnsiString(const unsigned char *Str);
  explicit AnsiString(const unsigned char *Str, intptr_t Length);
  explicit AnsiString(const UnicodeString &Str);
  explicit AnsiString(const UTF8String &Str);
  explicit AnsiString(const RawByteString &Str);
  inline ~AnsiString() {}

  const char *c_str() const { return Data.c_str(); }
  intptr_t Length() const { return Data.GetLength(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  char *SetLength(intptr_t nLength);
  inline AnsiString &Delete(intptr_t Index, intptr_t Count);
  AnsiString &Clear();
  AnsiString &Insert(const char *Str, intptr_t Pos);
  AnsiString SubString(intptr_t Pos) const;
  AnsiString SubString(intptr_t Pos, intptr_t Len) const;

  intptr_t Pos(const AnsiString &Str) const;
  intptr_t Pos(char Ch) const;

  char operator[](intptr_t Idx) const;
  char &operator[](intptr_t Idx);

  AnsiString &Append(const char *Str, intptr_t StrLen);
  AnsiString &Append(const AnsiString &Str);
  AnsiString &Append(const char *Str);
  AnsiString &Append(const char Ch);

  void Unique() {}

public:
  AnsiString &operator=(const UnicodeString &StrCopy);
  AnsiString &operator=(const RawByteString &StrCopy);
  AnsiString &operator=(const AnsiString &StrCopy);
  AnsiString &operator=(const UTF8String &StrCopy);
  AnsiString &operator=(const char *Str);
  AnsiString &operator=(const wchar_t *Str);
  AnsiString &operator=(wchar_t chData);

  AnsiString operator+(const UnicodeString &rhs) const;
  AnsiString operator+(const AnsiString &rhs) const;

  AnsiString &operator+=(const AnsiString &rhs);
  AnsiString &operator+=(const char Ch);
  AnsiString &operator+=(const char *rhs);

  inline friend bool operator==(const AnsiString &lhs, const AnsiString &rhs)
  { return lhs.Data == rhs.Data; }
  inline friend bool operator!=(const AnsiString &lhs, const AnsiString &rhs)
  { return lhs.Data != rhs.Data; }

  inline friend bool operator==(const AnsiString &lhs, const char *rhs)
  { return lhs.Data == rhs; }
  inline friend bool operator==(const char *lhs, const AnsiString &rhs)
  { return lhs == rhs.Data; }
  inline friend bool operator!=(const AnsiString &lhs, const char *rhs)
  { return lhs.Data != rhs; }
  inline friend bool operator!=(const char *lhs, const AnsiString &rhs)
  { return lhs != rhs.Data; }

private:
  void Init(const wchar_t *Str, intptr_t Length);
  void Init(const char *Str, intptr_t Length);
  void Init(const unsigned char *Str, intptr_t Length);
  void ThrowIfOutOfRange(intptr_t Idx) const;

  typedef CMStringA string_t;
  string_t Data;
};

class NB_CORE_EXPORT RawByteString
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  RawByteString() {}
  explicit RawByteString(const wchar_t *Str);
  explicit RawByteString(const wchar_t *Str, intptr_t Length);
  RawByteString(const char *Str);
  explicit RawByteString(const char *Str, intptr_t Length);
  explicit RawByteString(const unsigned char *Str);
  explicit RawByteString(const unsigned char *Str, intptr_t Length);
  RawByteString(const UnicodeString &Str);
  RawByteString(const RawByteString &Str);
  RawByteString(const AnsiString &Str);
  RawByteString(const UTF8String &Str);
  ~RawByteString() {}

  operator const char *() const { return this->c_str(); }
  operator UnicodeString() const;
  const char *c_str() const { return Data.c_str(); }
  intptr_t Length() const { return Data.GetLength(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  char *SetLength(intptr_t nLength);
  RawByteString &Clear() { SetLength(0); return *this; }
  RawByteString &Delete(intptr_t Index, intptr_t Count);
  RawByteString &Insert(const char *Str, intptr_t Pos);
  RawByteString SubString(intptr_t Pos) const;
  RawByteString SubString(intptr_t Pos, intptr_t Len) const;

  intptr_t Pos(wchar_t Ch) const;
  intptr_t Pos(const wchar_t *Str) const;
  intptr_t Pos(const char Ch) const;
  intptr_t Pos(const char *Str) const;

  void Unique() {}

public:
  RawByteString &operator=(const UnicodeString &StrCopy);
  RawByteString &operator=(const RawByteString &StrCopy);
  RawByteString &operator=(const AnsiString &StrCopy);
  RawByteString &operator=(const UTF8String &StrCopy);
  RawByteString &operator=(const char *lpszData);
  RawByteString &operator=(const wchar_t *lpwszData);
  RawByteString &operator=(wchar_t chData);

  RawByteString operator+(const RawByteString &rhs) const;

  RawByteString &operator+=(const RawByteString &rhs);
  RawByteString &operator+=(const char Ch);

  bool operator==(const char *rhs) const
  { return Data == rhs; }
  inline friend bool operator==(RawByteString &lhs, RawByteString &rhs)
  { return lhs.Data == rhs.Data; }
  inline friend bool operator!=(RawByteString &lhs, RawByteString &rhs)
  { return lhs.Data != rhs.Data; }

private:
  void Init(const wchar_t *Str, intptr_t Length);
  void Init(const char *Str, intptr_t Length);
  void Init(const unsigned char *Str, intptr_t Length);

  typedef CMStringT< unsigned char, NBChTraitsCRT< unsigned char > > rawstring_t;
  rawstring_t Data;
};

// rde support

namespace rde {

template<typename S>
inline bool operator==(const S &lhs, const S &rhs)
{
  return lhs.Compare(rhs) == 0;
}

template<typename S>
inline bool operator!=(const S &lhs, const S &rhs)
{
  return !(lhs == rhs);
}

template<typename S>
inline bool operator<(const S &lhs, const S &rhs)
{
  return lhs.Compare(rhs) < 0;
}

template<typename S>
inline bool operator>(const S &lhs, const S &rhs)
{
  return lhs.Compare(rhs) > 0;
}

}  // namespace rde

// utility functions

template <class StringClass>
inline char *ToChar(StringClass &a) { return const_cast<char *>(a.c_str()); }
template <class StringClass>
inline wchar_t *ToWChar(StringClass &a) { return const_cast<wchar_t *>(a.c_str()); }
