#pragma once

#include <nbstring.h>

void ThrowIfOutOfRange(intptr_t Idx, intptr_t Length);

template<typename CharT>
class BaseStringT : public CMStringT< CharT, NBChTraitsCRT< CharT> >
{
  typedef CMStringT< CharT, NBChTraitsCRT <CharT > > BaseT;
  typedef typename BaseT::XCHAR XCHAR;
  typedef typename BaseT::PXSTR PXSTR;
  typedef typename BaseT::PCXSTR PCXSTR;
  typedef typename BaseT::YCHAR YCHAR;
  typedef typename BaseT::PYSTR PYSTR;
  typedef typename BaseT::PCYSTR PCYSTR;
  typedef CMStringT< YCHAR, NBChTraitsCRT <YCHAR > > BaseY;

public:
  BaseStringT() {}
  BaseStringT(BaseStringT&&) = default;

  BaseStringT(const BaseStringT &rhs) :
    BaseT(rhs.c_str(), ToInt(rhs.GetLength()))
  {}
  BaseStringT(const BaseY &rhs) :
    BaseT(rhs.c_str(), ToInt(rhs.GetLength()))
  {}
  BaseStringT(const XCHAR *Str) :
    BaseT(Str, ToInt(BaseT::StringLength(Str)))
  {}
  BaseStringT(const XCHAR Ch) :
    BaseT(&Ch, 1)
  {}
  BaseStringT(const YCHAR *Str) :
    BaseT(Str, ToInt(BaseT::StringLength(Str)))
  {}
  BaseStringT(const YCHAR Ch) :
    BaseT(&Ch, 1)
  {}
  explicit BaseStringT(const XCHAR *Str, intptr_t Length, int CodePage) :
    BaseT(Str, ToInt(Length), CodePage)
  {}
  explicit BaseStringT(const XCHAR *Str, intptr_t Length) :
    BaseT(Str, ToInt(Length))
  {}
  explicit BaseStringT(const YCHAR *Str, intptr_t Length, int CodePage) :
    BaseT(Str, ToInt(Length), CodePage)
  {}
  explicit BaseStringT(const YCHAR *Str, intptr_t Length) :
    BaseT(Str, ToInt(Length))
  {}
  explicit BaseStringT(intptr_t Length, CharT Ch) : BaseT(Ch, ToInt(Length)) {}
  ~BaseStringT() {}

  const CharT *c_str() const { return BaseT::c_str(); }
  const CharT *data() const { return BaseT::c_str(); }
  intptr_t Length() const { return GetLength(); }
  intptr_t GetLength() const { return ::ToIntPtr(BaseT::GetLength()); }
  bool IsEmpty() const { return BaseT::IsEmpty(); }
  CharT *SetLength(intptr_t nLength)
  {
    return BaseT::GetBufferSetLength(ToInt(nLength));
  }

  BaseStringT &Delete(intptr_t Index, intptr_t Count)
  {
    BaseT::Delete(ToInt(Index) - 1, ToInt(Count));
    return *this;
  }
  BaseStringT &Clear() { BaseT::Empty(); return *this; }

  BaseStringT &Lower(intptr_t nStartPos = 1)
  {
    *this = BaseT::Mid(ToInt(nStartPos)).MakeLower();
    return *this;
  }

  BaseStringT &Lower(intptr_t nStartPos, intptr_t nLength)
  {
    *this = BaseT::Mid(ToInt(nStartPos), ToInt(nLength)).MakeLower();
    return *this;
  }
  BaseStringT &Upper(intptr_t nStartPos = 1)
  {
    *this = BaseT::Mid(ToInt(nStartPos)).MakeUpper();
    return *this;
  }
  BaseStringT &Upper(intptr_t nStartPos, intptr_t nLength)
  {
    *this = BaseT::Mid(nStartPos, nLength).MakeUpper();
    return *this;
  }

  BaseStringT &LowerCase() { BaseT::MakeLower(); return *this; }
  BaseStringT &UpperCase() { BaseT::MakeUpper(); return *this; }
  BaseStringT &MakeUpper() { BaseT::MakeUpper(); return *this; }
  BaseStringT &MakeLower() { BaseT::MakeLower(); return *this; }

  // intptr_t Compare(const BaseStringT &Str) const;
  intptr_t CompareIC(const BaseStringT &Str) const
  { return ::AnsiCompareIC(*this, Str); }
  intptr_t ToIntPtr() const
  { return ::StrToIntDef(*this, 0); }
  intptr_t FindFirstOf(const CharT Ch) const
  { return ::ToIntPtr(BaseT::Find(Ch, 0)) + 1; }
  intptr_t FindFirstOf(const CharT *AStr, size_t Offset = 0) const
  {
    if (!AStr || !*AStr)
    {
      return NPOS;
    }
    int Res = BaseT::Mid(ToInt(Offset)).FindOneOf(AStr);
    if (Res != -1)
      return Res + Offset;
    return NPOS;
  }

//  intptr_t FindFirstNotOf(const wchar_t * Str) const { return (intptr_t)Data.find_first_not_of(Str); }

  BaseStringT &Replace(intptr_t Pos, intptr_t Len, const wchar_t *Str, intptr_t DataLen)
  {
    BaseT::Delete(ToInt(Pos) - 1, ToInt(Len));
    BaseT::Insert(ToInt(Pos) - 1, BaseStringT(Str, ToInt(DataLen)).c_str());
    return *this;
  }
  BaseStringT &Replace(intptr_t Pos, intptr_t Len, const BaseStringT &Str) { return Replace(Pos, Len, Str.c_str(), Str.GetLength()); }
  BaseStringT &Replace(intptr_t Pos, intptr_t Len, const CharT *Str)
  {
    return Replace(Pos, Len, Str, BaseT::StringLength(Str));
  }
  BaseStringT &Replace(intptr_t Pos, intptr_t Len, CharT Ch) { return Replace(Pos, Len, &Ch, 1); }
  BaseStringT &Replace(intptr_t Pos, CharT Ch) { return Replace(Pos, 1, &Ch, 1); }

  BaseStringT &Append(const CharT *Str, intptr_t StrLen) { BaseT::Append(Str, ToInt(StrLen)); return *this; } // Replace(GetLength(), 0, Str, StrLen); }
  BaseStringT &Append(const BaseStringT &Str) { return Append(Str.c_str(), Str.GetLength()); }
  BaseStringT &Append(const CharT *Str)
  {
    return Append(Str, BaseT::StringLength(Str));
  }
  BaseStringT &Append(const CharT Ch) { return Append(&Ch, 1); }
  // BaseStringT &Append(const char *lpszAdd, UINT CodePage = CP_OEMCP);

  BaseStringT &Insert(intptr_t Pos, const wchar_t *Str, intptr_t StrLen)
  {
    BaseT::Insert(ToInt(Pos) - 1, BaseStringT(Str, ToInt(StrLen)).c_str());
    return *this;
  }
  BaseStringT &Insert(intptr_t Pos, const BaseStringT &Str) { return Insert(Pos, Str.c_str(), Str.Length()); }
  BaseStringT &Insert(const CharT *Str, intptr_t Pos)
  {
    BaseT::Insert(ToInt(Pos) - 1, Str);
    return *this;
  }
  BaseStringT &Insert(const wchar_t Ch, intptr_t Pos) { return Insert(Pos, &Ch, 1); }
  BaseStringT &Insert(const BaseStringT &Str, intptr_t Pos) { return Insert(Pos, Str); }

  intptr_t Pos(CharT Ch) const
  {
    return BaseT::Find(Ch) + 1;
  }
  intptr_t Pos(const CharT *Str) const
  {
    return BaseT::Find(Str) + 1;
  }
  intptr_t Pos(const BaseStringT &Str) const
  {
    return BaseT::Find(Str.c_str()) + 1;
  }

  intptr_t RPos(wchar_t Ch) const { return ::ToIntPtr(BaseT::ReverseFind(Ch)) + 1; }
  bool RPos(intptr_t &nPos, wchar_t Ch, intptr_t nStartPos = 0) const
  {
    int Pos = BaseT::ReverseFind(Ch); //, Data.size() - nStartPos);
    nPos = Pos + 1;
    return Pos != -1;
  }

  BaseStringT SubStr(intptr_t Pos, intptr_t Len) const
  {
    return BaseStringT(BaseT::Mid(ToInt(Pos) - 1, ToInt(Len)));
  }
  BaseStringT SubStr(intptr_t Pos) const
  {
    return BaseStringT(BaseT::Mid(ToInt(Pos) - 1));
  }
  BaseStringT SubString(intptr_t Pos, intptr_t Len) const
  {
    return SubStr(Pos, Len);
  }
  BaseStringT SubString(intptr_t Pos) const
  {
    return SubStr(Pos);
  }

  bool IsDelimiter(const BaseStringT &Delimiters, intptr_t Pos) const
  {
    if (Pos <= GetLength())
    {
      CharT Ch = operator[](Pos);
      for (intptr_t Index = 1; Index <= Delimiters.GetLength(); ++Index)
      {
        if (Delimiters[Index] == Ch)
        {
          return true;
        }
      }
    }
    return false;
  }

  intptr_t LastDelimiter(const BaseStringT &Delimiters) const
  {
    if (GetLength())
    {
      for (intptr_t Index = GetLength(); Index >= 1; --Index)
      {
        if (IsDelimiter(Delimiters, Index))
        {
          return Index;
        }
      }
    }
    return 0;
  }

  BaseStringT Trim() const
  {
    return TrimLeft().TrimRight();
  }

  BaseStringT TrimLeft() const
  {
    BaseStringT Result = *this;
    intptr_t Len = Result.GetLength();
    intptr_t Pos = 1;
    while ((Pos <= Len) && (Result[Pos] == L' '))
      Pos++;
    if (Pos > 1)
      return SubStr(Pos, Len - Pos + 1);
    return Result;
  }

  BaseStringT TrimRight() const
  {
    BaseStringT Result = *this;
    intptr_t Len = Result.GetLength();
    while (Len > 0 &&
      ((Result[Len] == L' ') || (Result[Len] == L'\n') || (Result[Len] == L'\r') || (Result[Len] == L'\x00')))
    {
      Len--;
    }
    Result.SetLength(Len);
    return Result;
  }

  void Unique() { this->SetString(BaseT::c_str(), BaseT::GetLength()); }

  int vprintf(const char *Format, va_list ArgList)
  {
    char *Buf = BaseT::GetBufferSetLength(32 * 1024);
    int Size = vsnprintf_s(Buf, ::ToSizeT(GetLength()), _TRUNCATE, Format, ArgList);
    BaseT::Truncate(Size);
    return Size;
  }

public:
  friend bool inline operator==(const BaseStringT &lhs, const BaseStringT &rhs)
  {
    return lhs.Compare(rhs) == 0;
  }
  friend bool inline operator==(const BaseStringT &lhs, const CharT *rhs)
  {
    return lhs.Compare(rhs) == 0;
  }
  friend bool inline operator==(const BaseStringT &lhs, const YCHAR *rhs)
  {
    return lhs.Compare(BaseStringT(rhs).c_str()) == 0;
  }
  friend bool inline operator==(const YCHAR *lhs, const BaseStringT &rhs)
  {
    return rhs.Compare(BaseStringT(lhs).c_str()) == 0;
  }
  friend bool inline operator==(const CharT *lhs, const BaseStringT &rhs)
  {
    return rhs.Compare(lhs) == 0;
  }
  friend bool inline operator!=(const BaseStringT &lhs, const BaseStringT &rhs)
  {
    return lhs.Compare(rhs) != 0;
  }
  friend bool inline operator!=(const BaseStringT &lhs, const CharT *rhs)
  {
    return lhs.Compare(rhs) != 0;
  }
  friend bool inline operator!=(const BaseStringT &lhs, const YCHAR *rhs)
  {
    return lhs.Compare(BaseStringT(rhs).c_str()) != 0;
  }
  friend bool inline operator!=(const YCHAR *lhs, const BaseStringT &rhs)
  {
    return rhs.Compare(BaseStringT(lhs).c_str()) != 0;
  }
  friend bool inline operator!=(const CharT *lhs, const BaseStringT &rhs)
  {
    return rhs.Compare(lhs) != 0;
  }

  CharT operator[](intptr_t Idx) const
  {
    ThrowIfOutOfRange(Idx, Length()); // Should Range-checking be optional to avoid overhead ??
    return BaseT::operator[](ToInt(Idx) - 1);
  }
  CharT &operator[](intptr_t Idx)
  {
    ThrowIfOutOfRange(Idx, Length()); // Should Range-checking be optional to avoid overhead ??
    return BaseT::GetBuffer()[ToInt(Idx) - 1];
  }

public:
  BaseStringT &operator=(BaseStringT&&) = default;
  BaseStringT &operator=(const CharT *Str) { SetString(Str, ToInt(BaseStringT::StringLength(Str))); return *this; }
  BaseStringT &operator=(const XCHAR Ch) { SetString(&Ch, 1); return *this; }
  BaseStringT &operator=(const BaseStringT &StrCopy) { SetString(StrCopy.c_str(), ToInt(StrCopy.GetLength())); return *this; }

  template<typename StringT>
  BaseStringT operator+(const StringT &rhs) const
  {
    BaseStringT Result(*this);
    Result += rhs;
    return Result;
  }

  BaseStringT &operator+=(const BaseStringT &rhs)
  {
    BaseT::Append(rhs.c_str(), ToInt(rhs.Length()));
    return *this;
  }

  friend BaseStringT inline operator+(const wchar_t lhs, const BaseStringT &rhs)
  { return BaseStringT(&lhs, 1) + rhs; }
  friend BaseStringT inline operator+(const BaseStringT &lhs, wchar_t rhs)
  { return lhs + BaseStringT(rhs); }
  friend BaseStringT inline operator+(const wchar_t *lhs, const BaseStringT &rhs)
  { return BaseStringT(lhs) + rhs; }
  friend BaseStringT inline operator+(const BaseStringT &lhs, const wchar_t *rhs)
  { return lhs + BaseStringT(rhs); }
  friend BaseStringT inline operator+(const BaseStringT &lhs, const char *rhs)
  { return lhs + BaseStringT(rhs); }
};

typedef BaseStringT<wchar_t> UnicodeString;
typedef BaseStringT<char> UTF8String;
typedef BaseStringT<char> AnsiString;

class NB_CORE_EXPORT RawByteString : public BaseStringT<char>
{
  typedef BaseStringT<char> BaseT;
public:
  RawByteString() {}
  RawByteString(const BaseStringT<wchar_t> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  RawByteString(const BaseStringT<char> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  RawByteString(const char *Str) : BaseT(Str, BaseT::StringLength(Str)) {}
  RawByteString(const RawByteString &Str) : BaseT(Str.c_str(), ToInt(Str.Length())) {}
  explicit RawByteString(const wchar_t *Str) : BaseT(Str, BaseT::StringLength(Str)) {}
  explicit RawByteString(const wchar_t *Str, intptr_t Length) : BaseT(Str, ToInt(Length)) {}
  explicit RawByteString(const char *Str, intptr_t Length) : BaseT(Str, BaseT::StringLength(Str)) {}
  explicit RawByteString(const unsigned char *Str) :
    BaseT(reinterpret_cast<const char *>(Str), BaseT::StringLength(reinterpret_cast<const char *>(Str)))
  {}
  explicit RawByteString(const unsigned char *Str, intptr_t Length) :
    BaseT(reinterpret_cast<const char *>(Str), ToInt(Length))
  {}
  ~RawByteString() {}

  inline operator BaseT &() { return *static_cast<BaseT *>(this); }

  operator const char *() const { return this->c_str(); }
  operator UnicodeString() const { return UnicodeString(reinterpret_cast<const char *>(c_str()), GetLength()); }
  const char *c_str() const { return reinterpret_cast<const char *>(BaseT::c_str()); }

  unsigned char operator[](intptr_t Idx) const
  {
    ThrowIfOutOfRange(Idx, Length()); // Should Range-checking be optional to avoid overhead ??
    return BaseT::operator[](ToInt(Idx) - 1);
  }

  unsigned char &operator[](intptr_t Idx)
  {
    ThrowIfOutOfRange(Idx, Length()); // Should Range-checking be optional to avoid overhead ??
    return reinterpret_cast<unsigned  char *>(BaseT::GetBuffer())[ToInt(Idx) - 1];
  }

public:
  RawByteString &operator=(const BaseT &Str)
  { Init(Str.c_str(), Str.GetLength()); return *this; }
  RawByteString &operator=(const UnicodeString &StrCopy) { Init(StrCopy.c_str(), StrCopy.Length()); return *this; }
  RawByteString &operator=(const RawByteString &StrCopy) { Init(StrCopy.c_str(), StrCopy.Length()); return *this; }
  RawByteString &operator=(const char *lpszData) { Init(lpszData, BaseT::StringLength(lpszData)); return *this; }
  RawByteString &operator=(const wchar_t *lpwszData) { Init(lpwszData, CMStringW::StringLength(lpwszData)); return *this; }
  RawByteString &operator=(wchar_t chData) { Init(&chData, 1); return *this; }

  RawByteString &operator+=(const BaseStringT &rhs) { Append(rhs); return *this; }
  RawByteString &operator+=(const RawByteString &rhs) { BaseT::Append(rhs.data(), ToInt(rhs.Length())); return *this; }
  RawByteString &operator+=(const char Ch) { BaseT::AppendChar(Ch); return *this; }
  RawByteString &operator+=(const char *Str) { BaseT::Append(Str); return *this; }

  inline friend bool operator==(const UnicodeString &lhs, const RawByteString &rhs)
  { return lhs == UnicodeString(rhs); }
  inline friend bool operator==(const RawByteString &lhs, const UnicodeString &rhs)
  { return rhs == UnicodeString(lhs); }
  inline friend bool operator==(const RawByteString &lhs, const RawByteString &rhs)
  { return lhs.Compare(rhs.c_str()) == 0; }
  inline friend bool operator==(const RawByteString &lhs, const char *rhs)
  { return lhs.Compare(rhs) == 0; }
  inline friend bool operator==(const char *lhs, const RawByteString &rhs)
  { return rhs.Compare(lhs) == 0; }
  inline friend bool operator!=(const UnicodeString &lhs, const RawByteString &rhs)
  { return lhs != UnicodeString(rhs); }
  inline friend bool operator!=(const RawByteString &lhs, const UnicodeString &rhs)
  { return rhs != UnicodeString(lhs); }
  inline friend bool operator!=(const RawByteString &lhs, const RawByteString &rhs)
  { return lhs.Compare(rhs.c_str()) != 0; }
  inline friend bool operator!=(const RawByteString &lhs, const char *rhs)
  { return lhs.Compare(rhs) != 0; }
  inline friend bool operator!=(const char *lhs, const RawByteString &rhs)
  { return rhs.Compare(lhs) != 0; }

private:
  void Init(const wchar_t *Str, intptr_t Length) { this->operator=(BaseT(Str, ToInt(Length))); }
  void Init(const char *Str, intptr_t Length) { SetString(Str, ToInt(Length)); }
  void Init(const unsigned char *Str, intptr_t Length) { SetString(reinterpret_cast<const char *>(Str), ToInt(Length)); }
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
