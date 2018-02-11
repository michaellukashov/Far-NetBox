#pragma once

#include <nbstring.h>

class RawByteString;
class UTF8String;
class AnsiString;

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

  template<typename BaseType>
  inline operator BaseStringT<BaseType> &()
  {
    return *static_cast<BaseStringT<BaseType> *>(this);
  }
/*
  template<typename StringT> // , typename std::enable_if<std::is_base_of<BaseStringT, StringT>::value, StringT>::type>
  inline operator StringT &()
  {
    return *static_cast<StringT *>(this);
  }

  template<typename StringT> // , typename std::enable_if<std::is_base_of<BaseStringT, StringT>::value, StringT>::type>
  inline operator const StringT &() const
  {
    return *static_cast<const StringT *>(this);
  }
*/
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
    // *this = ::UpperCase(SubString(nStartPos)).c_str();
    *this = BaseT::Mid(ToInt(nStartPos)).MakeUpper();
    return *this;
  }
  BaseStringT &Upper(intptr_t nStartPos, intptr_t nLength)
  {
//    *this = ::UpperCase(SubString(nStartPos, nLength)).c_str();
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

  // template<typename CharT>
  // BaseStringT<CharT> SubStr(intptr_t Pos, intptr_t Len) const
  // template<typename StringT>
  BaseStringT SubStr(intptr_t Pos, intptr_t Len) const
  {
    return BaseStringT(BaseT::Mid(ToInt(Pos) - 1, ToInt(Len)));
    // StringT Str(BaseT::Mid(ToInt(Pos) - 1, ToInt(Len)));
    // return StringT(Str.c_str(), Str.GetLength());
  }
  // template<typename StringT>
  BaseStringT SubStr(intptr_t Pos) const
  {
    return BaseStringT(BaseT::Mid(ToInt(Pos) - 1));
//    StringT Str(BaseT::Mid(ToInt(Pos) - 1));
//    return StringT(Str.c_str(), Str.GetLength());
  }
  // template<typename StringT>
  BaseStringT SubString(intptr_t Pos, intptr_t Len) const
  {
//    BaseStringT<CharT> Str(BaseT::Mid(ToInt(Pos) - 1), ToInt(Len));
//    return BaseStringT<CharT>(Str.c_str(), Str.GetLength());
    return SubStr(Pos, Len);
  }
  // template<typename StringT>
  BaseStringT SubString(intptr_t Pos) const
  {
//    BaseStringT<CharT> Str(BaseT::Mid(ToInt(Pos) - 1));
//    return BaseStringT<CharT>(Str.c_str(), Str.GetLength());
    return SubStr(Pos);
  }

  // template<typename StringT>
  bool IsDelimiter(const BaseStringT &Delimiters, intptr_t Pos) const
  {
    // return ::IsDelimiter(Chars, *this, Pos);
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
//  template<typename StringT>
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

  void Unique() { this->operator=(BaseStringT(this->c_str(), this->GetLength())); }

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

//  bool inline operator==(const CharT *rhs) const { return operator==(rhs, *this); }
//  bool inline operator!=(const CharT *rhs) const { return BaseT::operator!=(rhs); }
//  bool inline operator==(const BaseStringT<CharT> &rhs) const { return BaseT::operator==(rhs); }
//  bool inline operator!=(const BaseStringT<CharT> &rhs) const { return BaseT::operator!=(rhs); }
//  template<typename StringT>
//  bool inline operator==(const StringT &rhs) const { return ::operator==(rhs, *this); }
//  template<typename StringT>
//  bool inline operator!=(const StringT &rhs) const { return ::operator!=(rhs, *this); }

  CharT operator[](intptr_t Idx) const
  {
    ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
    return BaseT::operator[](ToInt(Idx) - 1);
  }
  CharT &operator[](intptr_t Idx)
  {
    ThrowIfOutOfRange(Idx); // Should Range-checking be optional to avoid overhead ??
    return BaseT::GetBuffer()[ToInt(Idx) - 1];
  }

public:
  BaseStringT &operator=(const CharT *Str)
  {
    BaseT::operator=(BaseT(Str, ToInt(BaseStringT::StringLength(Str))));
    return *this;
  }
  BaseStringT &operator=(const BaseStringT &StrCopy)
  {
    BaseT::operator=(BaseT(StrCopy.c_str(), ToInt(StrCopy.GetLength())));
    return *this;
  }

  template<typename StringT>
  BaseStringT operator+(const StringT &rhs) const
  {
    BaseStringT Result(*this);
    Result += rhs;
    return Result;
  }

  // template<typename StringT>
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

protected:
  void ThrowIfOutOfRange(intptr_t Idx) const
  {
    if (Idx < 1 || Idx > Length()) // NOTE: UnicodeString is 1-based !!
      throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
  }
};

class NB_CORE_EXPORT UnicodeString : public BaseStringT<wchar_t> // CMStringT< wchar_t, NBChTraitsCRT<wchar_t> >
{
  typedef BaseStringT<wchar_t> BaseT; // CMStringT< wchar_t, NBChTraitsCRT<wchar_t> > BaseT;
public:
  UnicodeString() {}
  UnicodeString(const wchar_t *Str);
  UnicodeString(const char *Str);
  UnicodeString(const BaseStringT<wchar_t> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  UnicodeString(const BaseStringT<char> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  UnicodeString(const wchar_t Src);
  UnicodeString(const UnicodeString &Str);
  explicit UnicodeString(const wchar_t *Str, intptr_t Length);
  explicit UnicodeString(const char *Str, intptr_t Length);
  explicit UnicodeString(const char *Str, intptr_t Length, int CodePage);
  explicit UnicodeString(intptr_t ALength, wchar_t Ch) : BaseT(Ch, ToInt(ALength)) {}

  explicit UnicodeString(const UTF8String &Str);
  explicit UnicodeString(const AnsiString &Str);

  ~UnicodeString() {}

  // template<typename StringT> // , typename std::enable_if<std::is_base_of<BaseStringT, StringT>::value, StringT>::type>
  inline operator BaseT &() { return *static_cast<BaseT *>(this); }

//  template<typename BaseT> // , typename std::enable_if<std::is_base_of<BaseStringT, StringT>::value, StringT>::type>
//  inline operator const BaseT &() const
//  {
//    return *static_cast<const BaseT *>(this);
//  }
  // using BaseT::Insert;

//  const wchar_t *c_str() const { return BaseT::c_str(); }
//  const wchar_t *data() const { return BaseT::c_str(); }
//  intptr_t Length() const { return GetLength(); }
//  intptr_t GetLength() const { return BaseT::GetLength(); }
//  bool IsEmpty() const { return Length() == 0; }
//  wchar_t *SetLength(intptr_t nLength);

//  UnicodeString &Delete(intptr_t Index, intptr_t Count);
//  UnicodeString &Clear() { BaseT::Empty(); return *this; }

//  UnicodeString &Lower(intptr_t nStartPos = 1);
//  UnicodeString &Lower(intptr_t nStartPos, intptr_t nLength);
//  UnicodeString &Upper(intptr_t nStartPos = 1);
//  UnicodeString &Upper(intptr_t nStartPos, intptr_t nLength);

//  UnicodeString &LowerCase() { return Lower(); }
//  UnicodeString &UpperCase() { return Upper(); }
//  UnicodeString &MakeUpper() { BaseT::MakeUpper(); return *this; }
//  UnicodeString &MakeLower() { BaseT::MakeLower(); return *this; }

//  intptr_t CompareIC(const UnicodeString &Str) const;
  // intptr_t ToIntPtr() const;
//  intptr_t FindFirstOf(const wchar_t Ch) const;
//  intptr_t FindFirstOf(const wchar_t *Str, size_t Offset = 0) const;
//  intptr_t FindFirstNotOf(const wchar_t * Str) const { return (intptr_t)Data.find_first_not_of(Str); }

//  UnicodeString &Replace(intptr_t Pos, intptr_t Len, const wchar_t *Str, intptr_t DataLen);
//  UnicodeString &Replace(intptr_t Pos, intptr_t Len, const UnicodeString &Str) { return Replace(Pos, Len, Str.c_str(), Str.GetLength()); }
//  UnicodeString &Replace(intptr_t Pos, intptr_t Len, const wchar_t *Str);
//  UnicodeString &Replace(intptr_t Pos, intptr_t Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }
//  UnicodeString &Replace(intptr_t Pos, wchar_t Ch) { return Replace(Pos, 1, &Ch, 1); }

//  UnicodeString &Append(const wchar_t *Str, intptr_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
//  UnicodeString &Append(const UnicodeString &Str) { return Append(Str.c_str(), Str.GetLength()); }
//  UnicodeString &Append(const wchar_t *Str);
//  UnicodeString &Append(const wchar_t Ch) { return Append(&Ch, 1); }
//  UnicodeString &Append(const char *lpszAdd, UINT CodePage = CP_OEMCP);

//  UnicodeString &Insert(intptr_t Pos, const wchar_t *Str, intptr_t StrLen);
//  UnicodeString &Insert(intptr_t Pos, const UnicodeString &Str) { return Insert(Pos, Str.c_str(), Str.Length()); }
//  UnicodeString &Insert(const wchar_t *Str, intptr_t Pos);
//  UnicodeString &Insert(const wchar_t Ch, intptr_t Pos) { return Insert(Pos, &Ch, 1); }
//  UnicodeString &Insert(const UnicodeString &Str, intptr_t Pos) { return Insert(Pos, Str); }

//  intptr_t Pos(wchar_t Ch) const { return BaseT::Pos(Ch) + 1; }
//  intptr_t Pos(const UnicodeString &Str) const { return BaseT::Pos(Str); }

//  intptr_t RPos(wchar_t Ch) const { return ::ToIntPtr(BaseT::ReverseFind(Ch)) + 1; }
//  bool RPos(intptr_t &nPos, wchar_t Ch, intptr_t nStartPos = 0) const;

//  UnicodeString SubStr(intptr_t Pos, intptr_t Len) const;
//  UnicodeString SubStr(intptr_t Pos) const;
//  UnicodeString SubString(intptr_t Pos, intptr_t Len) const;
//  UnicodeString SubString(intptr_t Pos) const;

//  bool IsDelimiter(const UnicodeString &Chars, intptr_t Pos) const;
//  intptr_t LastDelimiter(const UnicodeString &Delimiters) const;

//  UnicodeString Trim() const;
//  UnicodeString TrimLeft() const;
//  UnicodeString TrimRight() const;

  // void Unique();

public:
  //  bool inline operator!=(const BaseStringT<CharT> &rhs) const { return BaseT::operator!=(rhs); }
//  template<typename StringT>
  UnicodeString &operator=(const BaseT &Str)
  { Init(Str.c_str(), Str.GetLength()); return *this; }
  UnicodeString &operator=(const UnicodeString &StrCopy);
  UnicodeString &operator=(const RawByteString &StrCopy);
  UnicodeString &operator=(const AnsiString &StrCopy);
  UnicodeString &operator=(const UTF8String &StrCopy);
  UnicodeString &operator=(const wchar_t *Str);
  UnicodeString &operator=(const char *lpszData);
  UnicodeString &operator=(const wchar_t Ch);

//  UnicodeString operator+(const UnicodeString &rhs) const;
//  UnicodeString operator+(const RawByteString &rhs) const;
//  UnicodeString operator+(const AnsiString &rhs) const;
//  UnicodeString operator+(const UTF8String &rhs) const;

//  NB_CORE_EXPORT friend UnicodeString operator+(const wchar_t lhs, const UnicodeString &rhs);
//  NB_CORE_EXPORT friend UnicodeString operator+(const UnicodeString &lhs, wchar_t rhs);
//  NB_CORE_EXPORT friend UnicodeString operator+(const wchar_t *lhs, const UnicodeString &rhs);
//  NB_CORE_EXPORT friend UnicodeString operator+(const UnicodeString &lhs, const wchar_t *rhs);
//  NB_CORE_EXPORT friend UnicodeString operator+(const UnicodeString &lhs, const char *rhs);

  UnicodeString &operator+=(const BaseStringT &rhs)
  { Append(rhs); return *this; }
  UnicodeString &operator+=(const UnicodeString &rhs)
  { Append(rhs); return *this; }
  UnicodeString &operator+=(const wchar_t *rhs);
  UnicodeString &operator+=(const UTF8String &rhs);
  UnicodeString &operator+=(const RawByteString &rhs);
  UnicodeString &operator+=(const char Ch);
  UnicodeString &operator+=(const char *Ch);
  UnicodeString &operator+=(const wchar_t Ch);

//  friend bool inline operator==(const UnicodeString &lhs, const char *rhs)
//  { return lhs == UnicodeString(rhs); }
//  friend bool inline operator==(const char *lhs, const UnicodeString &rhs)
//  { return rhs == UnicodeString(lhs); }

//  friend bool inline operator!=(const UnicodeString &lhs, const char *rhs)
//  { return lhs != UnicodeString(rhs); }
//  friend bool inline operator!=(const char *lhs, const UnicodeString &rhs)
//  { return rhs != UnicodeString(lhs); }

//  wchar_t operator[](intptr_t Idx) const;
//  wchar_t &operator[](intptr_t Idx);

private:
  void Init(const wchar_t *Str, intptr_t Length);
  void Init(const char *Str, intptr_t Length, int CodePage);
//  void ThrowIfOutOfRange(intptr_t Idx) const;
};

class NB_CORE_EXPORT UTF8String : public BaseStringT<char> // CMStringT< char, NBChTraitsCRT<char> >
{
//  typedef CMStringT< char, NBChTraitsCRT<char> > BaseT;
  typedef BaseStringT<char> BaseT;
public:
  UTF8String() {}
  UTF8String(const BaseStringT<wchar_t> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  UTF8String(const BaseStringT<char> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  UTF8String(const UTF8String &rhs);
  UTF8String(const wchar_t *Str);
  explicit UTF8String(const UnicodeString &Str);
  explicit UTF8String(const wchar_t *Str, intptr_t Length);
  explicit UTF8String(const char *Str, intptr_t Length);
  explicit UTF8String(const char *Str);

  ~UTF8String() {}

  inline operator BaseT &() { return *static_cast<BaseT *>(this); }

//  operator const char *() const { return this->c_str(); }
//  const char *c_str() const { return BaseT::c_str(); }
//  intptr_t Length() const { return GetLength(); }
//  intptr_t GetLength() const { return BaseT::GetLength(); }
//  bool IsEmpty() const { return Length() == 0; }
//  char *SetLength(intptr_t nLength);
//  UTF8String &Delete(intptr_t Index, intptr_t Count);
//  UTF8String &Insert(wchar_t Ch, intptr_t Pos);
//  UTF8String &Insert(const wchar_t *Str, intptr_t Pos);
//  UTF8String SubString(intptr_t Pos) const;
//  UTF8String SubString(intptr_t Pos, intptr_t Len) const;

//  intptr_t Pos(char Ch) const;

  int vprintf(const char *Format, va_list ArgList);

//  void Unique() { }

public:
  UTF8String &operator=(const BaseT &Str)
  { Init(Str.c_str(), Str.GetLength()); return *this; }
  UTF8String &operator=(const UnicodeString &StrCopy);
  UTF8String &operator=(const UTF8String &StrCopy);
  UTF8String &operator=(const RawByteString &StrCopy);
  UTF8String &operator=(const char *lpszData);
  UTF8String &operator=(const wchar_t *lpwszData);
  UTF8String &operator=(wchar_t chData);

//  UTF8String operator+(const UTF8String &rhs) const;
//  UTF8String operator+(const RawByteString &rhs) const;
//  UTF8String operator+(const char *rhs) const;

  UTF8String &operator+=(const BaseStringT &rhs)
  { Append(rhs); return *this; }
  UTF8String &operator+=(const UTF8String &rhs)
  { Append(rhs); return *this; }
  UTF8String &operator+=(const RawByteString &rhs);
  UTF8String &operator+=(const char Ch);
  UTF8String &operator+=(const char *rhs);

//  friend bool inline operator==(const UTF8String &lhs, const UTF8String &rhs)
//  {
//    return lhs.Compare(rhs.c_str()) == 0;
//  }
//  friend bool inline operator!=(const UTF8String &lhs, const UTF8String &rhs)
//  {
//    return lhs.Compare(rhs.c_str()) != 0;
//  }

//  char operator[](intptr_t Idx) const;
//  char &operator[](intptr_t Idx);

private:
  void Init(const wchar_t *Str, intptr_t Length);
  void Init(const char *Str, intptr_t Length);

//  void ThrowIfOutOfRange(intptr_t Idx) const;
};

class NB_CORE_EXPORT AnsiString : public BaseStringT<char> // public CMStringT< char, NBChTraitsCRT<char> >
{
//  typedef CMStringT< char, NBChTraitsCRT<char> > BaseT;
  typedef BaseStringT<char> BaseT;
public:
  AnsiString() {}
  AnsiString(const BaseStringT<wchar_t> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  AnsiString(const BaseStringT<char> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  AnsiString(const AnsiString &rhs);
  AnsiString(intptr_t Length, char Ch) : BaseT(Ch, ToInt(Length)) {}
  AnsiString(const char *Str);
  explicit AnsiString(const wchar_t *Str);
  explicit AnsiString(const wchar_t *Str, intptr_t Length);
  explicit AnsiString(const wchar_t *Str, intptr_t Length, int CodePage);
  explicit AnsiString(const char *Str, intptr_t Length);
  explicit AnsiString(const unsigned char *Str);
  explicit AnsiString(const unsigned char *Str, intptr_t Length);
  explicit AnsiString(const UnicodeString &Str);
  explicit AnsiString(const UTF8String &Str);
  explicit AnsiString(const RawByteString &Str);
  inline ~AnsiString() {}

  inline operator BaseT &() { return *static_cast<BaseT *>(this); }

//  const char *c_str() const { return BaseT::c_str(); }
//  intptr_t Length() const { return GetLength(); }
//  intptr_t GetLength() const { return BaseT::GetLength(); }
//  bool IsEmpty() const { return Length() == 0; }
//  char *SetLength(intptr_t nLength);
//  inline AnsiString &Delete(intptr_t Index, intptr_t Count);
//  AnsiString &Clear();
//  AnsiString &Insert(const char *Str, intptr_t Pos);
//  AnsiString SubString(intptr_t Pos) const;
//  AnsiString SubString(intptr_t Pos, intptr_t Len) const;

//  intptr_t Pos(const AnsiString &Str) const;
//  intptr_t Pos(char Ch) const;

//  char operator[](intptr_t Idx) const;
//  char &operator[](intptr_t Idx);

//  AnsiString &Append(const char *Str, intptr_t StrLen);
//  AnsiString &Append(const AnsiString &Str);
//  AnsiString &Append(const char *Str);
//  AnsiString &Append(const char Ch);

//  void Unique() {}

public:
  AnsiString &operator=(const BaseT &Str)
  { Init(Str.c_str(), Str.GetLength()); return *this; }
  AnsiString &operator=(const UnicodeString &StrCopy);
  AnsiString &operator=(const RawByteString &StrCopy);
  AnsiString &operator=(const AnsiString &StrCopy);
  AnsiString &operator=(const UTF8String &StrCopy);
  AnsiString &operator=(const char *Str);
  AnsiString &operator=(const wchar_t *Str);
  AnsiString &operator=(wchar_t chData);

//  AnsiString operator+(const UnicodeString &rhs) const;
//  AnsiString operator+(const AnsiString &rhs) const;

  AnsiString &operator+=(const BaseStringT &rhs)
  { Append(rhs); return *this; }
  AnsiString &operator+=(const AnsiString &rhs);
  AnsiString &operator+=(const char Ch);
  AnsiString &operator+=(const char *rhs);

//  inline friend bool operator==(const AnsiString &lhs, const AnsiString &rhs)
//  { return lhs.Compare(rhs.c_str()) == 0; }
//  inline friend bool operator!=(const AnsiString &lhs, const AnsiString &rhs)
//  { return lhs.Compare(rhs.c_str()) != 0; }

//  inline friend bool operator==(const AnsiString &lhs, const char *rhs)
//  { return lhs.Compare(rhs) == 0; }
//  inline friend bool operator==(const char *lhs, const AnsiString &rhs)
//  { return rhs.Compare(lhs) == 0; }
//  inline friend bool operator!=(const AnsiString &lhs, const char *rhs)
//  { return lhs.Compare(rhs) != 0; }
//  inline friend bool operator!=(const char *lhs, const AnsiString &rhs)
//  { return rhs.Compare(lhs) != 0; }

private:
  void Init(const wchar_t *Str, intptr_t Length);
  void Init(const char *Str, intptr_t Length);
  void Init(const unsigned char *Str, intptr_t Length);
//  void ThrowIfOutOfRange(intptr_t Idx) const;
};

class NB_CORE_EXPORT RawByteString : public BaseStringT<char> // public CMStringT< unsigned char, NBChTraitsCRT<unsigned char> >
{
//  typedef CMStringT< unsigned char, NBChTraitsCRT<unsigned char> > BaseT;
  typedef BaseStringT<char> BaseT;
public:
  RawByteString() {}
  RawByteString(const BaseStringT<wchar_t> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  RawByteString(const BaseStringT<char> &Str) :
    BaseT(Str.c_str(), ToInt(Str.GetLength()))
  {}
  RawByteString(const char *Str);
  RawByteString(const UnicodeString &Str);
  RawByteString(const RawByteString &Str);
  RawByteString(const AnsiString &Str);
  RawByteString(const UTF8String &Str);
  explicit RawByteString(const wchar_t *Str);
  explicit RawByteString(const wchar_t *Str, intptr_t Length);
  explicit RawByteString(const char *Str, intptr_t Length);
  explicit RawByteString(const unsigned char *Str);
  explicit RawByteString(const unsigned char *Str, intptr_t Length);
  ~RawByteString() {}

  inline operator BaseT &() { return *static_cast<BaseT *>(this); }

  operator const char *() const { return this->c_str(); }
  operator UnicodeString() const;
  const char *c_str() const { return reinterpret_cast<const char *>(BaseT::c_str()); }
//  intptr_t Length() const { return GetLength(); }
//  intptr_t GetLength() const { return BaseT::GetLength(); }
//  bool IsEmpty() const { return Length() == 0; }
//  char *SetLength(intptr_t nLength);
//  RawByteString &Clear() { SetLength(0); return *this; }
//  RawByteString &Delete(intptr_t Index, intptr_t Count);
//  RawByteString &Insert(const char *Str, intptr_t Pos);
//  RawByteString SubString(intptr_t Pos) const;
//  RawByteString SubString(intptr_t Pos, intptr_t Len) const;

  unsigned char operator[](intptr_t Idx) const;
  unsigned char &operator[](intptr_t Idx);

//  intptr_t Pos(wchar_t Ch) const;
//  intptr_t Pos(const wchar_t *Str) const;
//  intptr_t Pos(const char Ch) const;
//  intptr_t Pos(const char *Str) const;

//  void Unique() {}

public:
  RawByteString &operator=(const BaseT &Str)
  { Init(Str.c_str(), Str.GetLength()); return *this; }
  RawByteString &operator=(const UnicodeString &StrCopy);
  RawByteString &operator=(const RawByteString &StrCopy);
  RawByteString &operator=(const AnsiString &StrCopy);
  RawByteString &operator=(const UTF8String &StrCopy);
  RawByteString &operator=(const char *lpszData);
  RawByteString &operator=(const wchar_t *lpwszData);
  RawByteString &operator=(wchar_t chData);

//  RawByteString operator+(const RawByteString &rhs) const;

  RawByteString &operator+=(const BaseStringT &rhs)
  { Append(rhs); return *this; }
  RawByteString &operator+=(const RawByteString &rhs);
  RawByteString &operator+=(const char Ch);
  RawByteString &operator+=(const char *Str);

  bool operator==(const char *rhs) const
  { return BaseT::Compare(rhs) == 0; }
  inline friend bool operator==(const RawByteString &lhs, RawByteString &rhs)
  { return lhs.Compare(rhs.c_str()) == 0; }
  inline friend bool operator!=(const RawByteString &lhs, const RawByteString &rhs)
  { return lhs.Compare(rhs.c_str()) != 0; }
  inline friend bool operator!=(const UnicodeString &lhs, const RawByteString &rhs)
  { return lhs != UnicodeString(rhs); }
  inline friend bool operator!=(const RawByteString &rhs, const UnicodeString &lhs)
  { return lhs != UnicodeString(rhs); }

private:
  void Init(const wchar_t *Str, intptr_t Length);
  void Init(const char *Str, intptr_t Length);
  void Init(const unsigned char *Str, intptr_t Length);
//  void ThrowIfOutOfRange(intptr_t Idx) const;
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
