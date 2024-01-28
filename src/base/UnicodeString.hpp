#pragma once

// #include <string>
#include <nbstring.h>

class RawByteString;
class UnicodeString;
class AnsiString;

class UTF8String
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  UTF8String() = default;
  UTF8String(UTF8String && rhs) noexcept : Data(rhs.Data) {}
  UTF8String(const UTF8String & rhs);
  explicit UTF8String(const UnicodeString & Str);
  UTF8String(const wchar_t * Str);
  explicit UTF8String(const wchar_t * Str, int32_t Length);
  explicit UTF8String(const char * Str, int32_t Length);
  explicit UTF8String(const char * Str);
  explicit UTF8String(int32_t Length) = delete;
  explicit UTF8String(int32_t Length, char Ch) : Data(Ch, Length) {}

  ~UTF8String() = default;

  operator const char *() const = delete; // { return this->c_str(); }
  const char * c_str() const { return Data.c_str(); }
  const char * data() const { return Data.c_str(); }
  int32_t Length() const { return static_cast<int32_t>(Data.length()); }
  int32_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  char * SetLength(int32_t nLength);
  UTF8String & Delete(int32_t Index, int32_t Count);
  UTF8String & Insert(wchar_t Ch, int32_t Pos);
  UTF8String & Insert(const wchar_t * Str, int32_t Pos);
  UTF8String SubString(int32_t Pos) const;
  UTF8String SubString(int32_t Pos, int32_t Len) const;

  int32_t Pos(char Ch) const;

  int32_t vprintf(const char * Format, va_list ArgList);

  void Unique() { Init(Data.c_str(), static_cast<int32_t>(Data.length())); }

public:
  UTF8String & operator =(UTF8String && rhs) noexcept { Data = std::move(rhs.Data); return *this; }
  UTF8String & operator =(const UnicodeString & StrCopy);
  UTF8String & operator =(const UTF8String & StrCopy);
  UTF8String & operator =(const RawByteString & StrCopy);
  UTF8String & operator =(const char * lpszData);
  UTF8String & operator =(const wchar_t * lpwszData);
  // UTF8String & operator =(wchar_t chData);

  UTF8String operator +(const UTF8String & rhs) const;
  // UTF8String operator +(const RawByteString & rhs) const;
  UTF8String operator +(const char * rhs) const;
  UTF8String & operator +=(const UTF8String & rhs);
  UTF8String & operator +=(const RawByteString & rhs);
  UTF8String & operator +=(char Ch);
  UTF8String & operator +=(const char * rhs);

  friend bool operator ==(const UTF8String & lhs, const UTF8String & rhs);
  friend bool operator !=(const UTF8String & lhs, const UTF8String & rhs);

  char operator [](int32_t Idx) const;
  char & operator [](int32_t Idx);

private:
  void Init(const wchar_t * Str, int32_t Length);
  void Init(const char * Str, int32_t Length);
  void ThrowIfOutOfRange(int32_t Idx) const;

  using string_t = CMStringA;
  // using string_t = std::basic_string<char, std::char_traits<char>, nb::custom_nballocator_t<char>>;
  string_t Data;
};

class UnicodeString
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  UnicodeString() = default;
  UnicodeString(UnicodeString && rhs) noexcept : Data(rhs.Data) {}
  UnicodeString(const wchar_t * Str);
  UnicodeString(const wchar_t * Str, int32_t Length);
  explicit UnicodeString(wchar_t Src) = delete;
  UnicodeString(const char * Str, int32_t Length);
  UnicodeString(const char * Str, int32_t Length, int32_t CodePage);
  UnicodeString(const char * Str);
  explicit UnicodeString(int32_t Length) = delete;
  explicit UnicodeString(int32_t Length, wchar_t Ch) : Data(Ch, Length) {}

  UnicodeString(const UnicodeString & Str);
  explicit UnicodeString(const UTF8String & Str);
  explicit UnicodeString(const AnsiString & Str);

  ~UnicodeString() = default;

  const wchar_t * c_str() const { return Data.c_str(); }
  const wchar_t * data() const { return Data.c_str(); }
  int32_t Length() const { return static_cast<int32_t>(Data.length()); }
  int32_t GetLength() const { return Length(); }
  int32_t GetBytesCount() const { return (Length() + 1) * sizeof(wchar_t); }
  bool IsEmpty() const { return Length() == 0; }
  wchar_t * SetLength(int32_t nLength);
  UnicodeString & Delete(int32_t Index, int32_t Count);
  UnicodeString & Clear() { Data.clear(); return *this; }

  UnicodeString & Lower(int32_t nStartPos = 1);
  UnicodeString & Lower(int32_t nStartPos, int32_t nLength);
  UnicodeString & Upper(int32_t nStartPos = 1);
  UnicodeString & Upper(int32_t nStartPos, int32_t nLength);

  UnicodeString & LowerCase() { return Lower(); }
  UnicodeString & UpperCase() { return Upper(); }
  UnicodeString & MakeUpper() { Data.MakeUpper(); return *this; }
  UnicodeString & MakeLower() { Data.MakeLower(); return *this; }

  int32_t Compare(const UnicodeString & Str) const;
  int32_t CompareIC(const UnicodeString & Str) const;
  int32_t ToInt32() const;
  int32_t FindFirstOf(wchar_t Ch) const;
  int32_t FindFirstOf(const wchar_t * Str, int32_t Offset = 0) const;
//  int32_t FindFirstNotOf(const wchar_t * Str) const { return static_cast<int32_t>(Data.find_first_not_of(Str)); }

  UnicodeString & Replace(int32_t Pos, int32_t Len, const wchar_t * Str, int32_t DataLen);
  UnicodeString & Replace(int32_t Pos, int32_t Len, const UnicodeString & Str) { return Replace(Pos, Len, Str.c_str(), Str.GetLength()); }
  UnicodeString & Replace(int32_t Pos, int32_t Len, const wchar_t * Str);
  UnicodeString & Replace(int32_t Pos, int32_t Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }
  UnicodeString & Replace(int32_t Pos, wchar_t Ch) { return Replace(Pos, 1, &Ch, 1); }

  UnicodeString & Append(const wchar_t * Str, int32_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
  UnicodeString & Append(const UnicodeString & Str) { return Append(Str.c_str(), Str.GetLength()); }
  UnicodeString & Append(const wchar_t * Str);
  UnicodeString & Append(const wchar_t Ch) { return Append(&Ch, 1); }
  UnicodeString & Append(const char * lpszAdd, int32_t CodePage = CP_OEMCP);

  UnicodeString & Insert(int32_t Pos, const wchar_t * Str, int32_t StrLen);
  UnicodeString & Insert(int32_t Pos, const UnicodeString & Str) { return Insert(Pos, Str.c_str(), Str.Length()); }
  UnicodeString & Insert(const wchar_t * Str, int32_t Pos);
  UnicodeString & Insert(const wchar_t Ch, int32_t Pos) { return Insert(Pos, &Ch, 1); }
  UnicodeString & Insert(const UnicodeString & Str, int32_t Pos) { return Insert(Pos, Str); }

  int32_t Pos(wchar_t Ch) const;
  int32_t Pos(const UnicodeString & Str) const;

  int32_t RPos(wchar_t Ch) const { return static_cast<int32_t>(Data.ReverseFind(Ch)) + 1; }
  bool RPos(int32_t & nPos, wchar_t Ch, int32_t nStartPos = 0) const;

  UnicodeString SubStr(int32_t Pos, int32_t Len) const;
  UnicodeString SubStr(int32_t Pos) const;
  UnicodeString SubString(int32_t Pos, int32_t Len) const;
  UnicodeString SubString(int32_t Pos) const;

  bool IsDelimiter(const UnicodeString & Chars, int32_t Pos) const;
  int32_t LastDelimiter(const UnicodeString & Delimiters) const;

  UnicodeString Trim() const;
  UnicodeString TrimLeft() const;
  UnicodeString TrimRight() const;

  void Unique();
  static UnicodeString StringOfChar(wchar_t Ch, int32_t Len);

public:
  UnicodeString & operator =(UnicodeString && rhs) noexcept { Data = std::move(rhs.Data); return *this; }
  UnicodeString & operator =(const UnicodeString & StrCopy);
  UnicodeString & operator =(const RawByteString & StrCopy);
  UnicodeString & operator =(const AnsiString & StrCopy);
  UnicodeString & operator =(const UTF8String & StrCopy);
  UnicodeString & operator =(const wchar_t * Str);
  UnicodeString & operator =(const char * lpszData);
  UnicodeString & operator =(wchar_t Ch);

  UnicodeString operator +(const UnicodeString & rhs) const;
  // UnicodeString operator +(const RawByteString & rhs) const;
  // UnicodeString operator +(const AnsiString & rhs) const;
  // UnicodeString operator +(const UTF8String & rhs) const;

  friend UnicodeString operator +(wchar_t lhs, const UnicodeString & rhs);
  friend UnicodeString operator +(const UnicodeString & lhs, wchar_t rhs);
  friend UnicodeString operator +(const wchar_t * lhs, const UnicodeString & rhs);
  friend UnicodeString operator +(const UnicodeString & lhs, const wchar_t * rhs);
  friend UnicodeString operator +(const UnicodeString & lhs, const char * rhs);

  UnicodeString & operator +=(const UnicodeString & rhs);
  UnicodeString & operator +=(const wchar_t * rhs);
  // UnicodeString & operator +=(const UTF8String & rhs);
  UnicodeString & operator +=(const RawByteString & rhs);
  UnicodeString & operator +=(char Ch);
  UnicodeString & operator +=(const char * Ch);
  UnicodeString & operator +=(wchar_t Ch);

  bool operator ==(const UnicodeString & Str) const { return Data == Str.Data; }
  bool operator !=(const UnicodeString & Str) const { return Data != Str.Data; }

  friend bool operator <(const UnicodeString & Str1, const UnicodeString & Str2) { return Str1.Compare(Str2) < 0; }

  friend bool operator ==(const UnicodeString & lhs, const wchar_t * rhs);
  friend bool operator ==(const wchar_t * lhs, const UnicodeString & rhs);
  friend bool operator !=(const UnicodeString & lhs, const wchar_t * rhs);
  friend bool operator !=(const wchar_t * lhs, const UnicodeString & rhs);

  wchar_t operator [](int32_t Idx) const;
  wchar_t & operator [](int32_t Idx);

private:
  void Init(const wchar_t * Str, int32_t Length);
  void Init(const char * Str, int32_t Length, int32_t CodePage);
  void ThrowIfOutOfRange(int32_t Idx) const;

  using string_t = CMStringW;
  // using string_t = std::basic_string<wchar_t, std::char_traits<wchar_t>, nb::custom_nballocator_t<wchar_t>>;
  string_t Data;
};

class RawByteString;

class AnsiString
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  AnsiString() = default;
  AnsiString(AnsiString && rhs) noexcept : Data(rhs.Data) {}

  AnsiString(const AnsiString & rhs);
  AnsiString(int32_t Length, char Ch) : Data(Ch, Length) {}
  explicit AnsiString(const wchar_t * Str);
  explicit AnsiString(const wchar_t * Str, int32_t Length);
  explicit AnsiString(const wchar_t * Str, int32_t Length, int32_t CodePage);
  AnsiString(const char * Str);
  explicit AnsiString(const char * Str, int32_t Length);
  explicit AnsiString(const unsigned char * Str);
  explicit AnsiString(const unsigned char * Str, int32_t Length);
  explicit AnsiString(const UnicodeString & Str);
  explicit AnsiString(const UTF8String & Str);
  explicit AnsiString(const RawByteString & Str);
  ~AnsiString() = default;

  const char * c_str() const { return Data.c_str(); }
  const char * data() const { return Data.c_str(); }
  int32_t Length() const { return static_cast<int32_t>(Data.length()); }
  int32_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  char * SetLength(int32_t nLength);
  inline AnsiString & Delete(int32_t Index, int32_t Count);
  AnsiString & Clear();
  AnsiString & Insert(const char * Str, int32_t Pos);
  AnsiString SubString(int32_t Pos) const;
  AnsiString SubString(int32_t Pos, int32_t Len) const;

  int32_t Pos(const AnsiString & Str) const;
  int32_t Pos(char Ch) const;

  char operator [](int32_t Idx) const;
  char & operator [](int32_t Idx);

  AnsiString & Append(const char * Str, int32_t StrLen);
  AnsiString & Append(const AnsiString & Str);
  AnsiString & Append(const char * Str);
  AnsiString & Append(char Ch);

  void Unique() { Init(Data.c_str(), static_cast<int32_t>(Data.length())); }

public:
  AnsiString & operator =(AnsiString && rhs) noexcept { Data = std::move(rhs.Data); return *this; }
  AnsiString & operator =(const UnicodeString & StrCopy);
  AnsiString & operator =(const AnsiString & StrCopy);
  AnsiString & operator =(const UTF8String & StrCopy);
  AnsiString & operator =(const char * Str);
  AnsiString & operator =(const wchar_t * Str);

  AnsiString & operator +=(const AnsiString & rhs);
  AnsiString & operator +=(char Ch);
  AnsiString & operator +=(const char * rhs);

  inline friend bool operator ==(const AnsiString & lhs, const AnsiString & rhs)
  { return lhs.Data == rhs.Data; }
  inline friend bool operator !=(const AnsiString & lhs, const AnsiString & rhs)
  { return lhs.Data != rhs.Data; }

  inline friend bool operator ==(const AnsiString & lhs, const char * rhs)
  { return lhs.Data == rhs; }
  inline friend bool operator ==(const char * lhs, const AnsiString & rhs)
  { return lhs == rhs.Data; }
  inline friend bool operator !=(const AnsiString & lhs, const char * rhs)
  { return lhs.Data != rhs; }
  inline friend bool operator !=(const char * lhs, const AnsiString & rhs)
  { return lhs != rhs.Data; }

private:
  void Init(const wchar_t * Str, int32_t Length);
  void Init(const char * Str, int32_t Length);
  void Init(const unsigned char * Str, int32_t Length);
  void ThrowIfOutOfRange(int32_t Idx) const;

  using string_t = CMStringA;
  // using string_t = std::basic_string<char, std::char_traits<char>, nb::custom_nballocator_t<char>>;
  string_t Data;
};

class RawByteString
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  RawByteString() = default;
  RawByteString(RawByteString && rhs) noexcept : Data(rhs.Data) {}

  explicit RawByteString(const wchar_t * Str);
  explicit RawByteString(const wchar_t * Str, int32_t Length);
  RawByteString(const char * Str);
  explicit RawByteString(const char * Str, int32_t Length);
  explicit RawByteString(const unsigned char * Str);
  explicit RawByteString(const unsigned char * Str, int32_t Length);
  RawByteString(const UnicodeString & Str);
  explicit RawByteString(int32_t Length) = delete;
  explicit RawByteString(int32_t Length, unsigned char Ch) : Data(static_cast<char>(Ch), Length) {}
  RawByteString(const RawByteString & Str);
  RawByteString(const AnsiString & Str);
  RawByteString(const UTF8String & Str);
  ~RawByteString() = default;

  // operator const char *() const { return this->c_str(); }
  operator UnicodeString() const;
  const char * c_str() const { return Data.c_str(); }
  const char * data() const { return Data.c_str(); }
  int32_t Length() const { return static_cast<int32_t>(Data.length()); }
  int32_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  char * SetLength(int32_t nLength);
  RawByteString & Clear() { SetLength(0); return *this; }
  RawByteString & Delete(int32_t Index, int32_t Count);
  RawByteString & Insert(const char * Str, int32_t Pos);
  RawByteString SubString(int32_t Pos) const;
  RawByteString SubString(int32_t Pos, int32_t Len) const;

  int32_t Pos(wchar_t Ch) const;
  // int32_t Pos(const wchar_t * Str) const;
  int32_t Pos(char Ch) const;
  int32_t Pos(const char * Str) const;

  unsigned char operator [](int32_t Idx) const;
  unsigned char & operator [](int32_t Idx);

  RawByteString Trim() const;
  RawByteString TrimLeft() const;
  RawByteString TrimRight() const;
  void Unique() { Init(Data.c_str(), static_cast<int32_t>(Data.length())); }

public:
  RawByteString & operator =(RawByteString && rhs) noexcept { Data = std::move(rhs.Data); return *this; }
  RawByteString & operator =(const UnicodeString & StrCopy);
  RawByteString & operator =(const RawByteString & StrCopy);
  RawByteString & operator =(const AnsiString & StrCopy);
  RawByteString & operator =(const UTF8String & StrCopy);
  RawByteString & operator =(const char * lpszData);
  RawByteString & operator =(const wchar_t * lpwszData);
  // RawByteString & operator =(wchar_t chData);

  RawByteString operator +(const RawByteString & rhs) const;

  RawByteString & operator +=(const RawByteString & rhs);
  RawByteString & operator +=(char Ch);
  RawByteString & operator +=(uint8_t Ch);

  bool operator ==(const RawByteString & rhs) const
  { return Data == rhs.Data; }
  // bool operator ==(const char * rhs) const
  // { return Data == rhs; }
  bool operator !=(const RawByteString & rhs) const
  { return Data != rhs.Data; }
  // bool operator !=(const char * rhs) const
  // { return Data != rhs; }
  // inline friend bool operator ==(const RawByteString & lhs, const RawByteString & rhs)
  // { return lhs.Data == rhs.Data; }
  // inline friend bool operator !=(const RawByteString & lhs, const RawByteString & rhs)
  // { return lhs.Data != rhs.Data; }

private:
  void Init(const wchar_t * Str, int32_t Length);
  void Init(const char * Str, int32_t Length);
  void Init(const unsigned char * Str, int32_t Length);
  void ThrowIfOutOfRange(int32_t Idx) const;

  using string_t = CMStringT<unsigned char, NBChTraitsCRT<unsigned char>>;
  // using string_t = std::basic_string<unsigned char, std::char_traits<unsigned char>, nb::custom_nballocator_t<unsigned char>>;
  string_t Data;
};


// std support

namespace nb {

/*template<typename S>
inline bool operator ==(const S & lhs, const S & rhs)
{
  return lhs.Compare(rhs) == 0;
}

template<typename S>
inline bool operator !=(const S & lhs, const S & rhs)
{
  return !(lhs == rhs);
}

template<typename S>
inline bool operator <(const S & lhs, const S & rhs)
{
  return lhs.Compare(rhs) < 0;
}

template<typename S>
inline bool operator >(const S & lhs, const S & rhs)
{
  return lhs.Compare(rhs) > 0;
}*/

}  // namespace nb

// utility functions

template <class StringClass>
inline char * ToCharPtr(StringClass & a) { return const_cast<char *>(a.c_str()); }
template <class StringClass>
inline wchar_t * ToWCharPtr(StringClass & a) { return const_cast<wchar_t *>(a.c_str()); }

// template<int s> struct GetSizeT;
// GetSizeT<sizeof(UnicodeString)> sz;
