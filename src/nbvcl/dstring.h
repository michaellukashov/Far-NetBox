// DSTRING.H - Support for delphi AnsiString in C++
//            (AnsiString and template<sz> SmallString)
// Copyright (c) 1997, 2002 Borland Software Corporation

#ifndef DSTRING_H
#define DSTRING_H

// #pragma delphiheader begin

// #include <sysmac.h>
// #include <stdarg.h>
#include <StrHlpr.hpp>

namespace System
{
  class                  TVarRec;
  class RTL_DELPHIRETURN Currency;
  class RTL_DELPHIRETURN WideString;


  /////////////////////////////////////////////////////////////////////////////
  // AnsiStringBase: Base type of operations common to all
  //                 specialization of AnsiStringT<CP>
  /////////////////////////////////////////////////////////////////////////////
  class RTL_DELPHIRETURN AnsiStringBase
  {
  public:
    // the TStringFloatFormat enum is used by FloatToStrF
    enum TStringFloatFormat
    {sffGeneral, sffExponent, sffFixed, sffNumber, sffCurrency};
    static AnsiStringBase __fastcall LoadStr(int ident, int codePage);
    static AnsiStringBase __fastcall LoadStr(HINSTANCE hInstance, int ident, int codePage);
    static AnsiStringBase __fastcall FmtLoadStr(int ident, const TVarRec *args,
                        int size, int codePage);

    AnsiStringBase& __fastcall       LoadString(HINSTANCE hInstance, int ident, int codePage);

    // String of specified character
    static AnsiStringBase __fastcall StringOfChar(char ch, int count);

    // Delphi style 'Format'
    //
    static AnsiStringBase __fastcall Format(const AnsiStringBase& format,
                                            const TVarRec *args, int size,
                                            int codePage);

    int         __cdecl         vprintf(int codePage, const char* format, va_list); // Returns formatted length
    int         __cdecl     cat_vprintf(int codePage, const char* format, va_list); // Returns formatted length

    static AnsiStringBase __fastcall FormatFloat(const AnsiStringBase& format,
                                                 const long double& value,
                                                 int codePage);
    static AnsiStringBase __fastcall FloatToStrF(long double value,
                                                 TStringFloatFormat format,
                                                 int precision, int digits,
                                                 int codePage);
    static AnsiStringBase __fastcall IntToHex(int value, int digits, int codePage);
    static AnsiStringBase __fastcall CurrToStr(Currency value, int codePage);
    static AnsiStringBase __fastcall CurrToStrF(Currency value,
                                                TStringFloatFormat format,
                                                int digits, int codePage);
    // Constructors
  protected:
    __fastcall AnsiStringBase(): Data(0) {}
    __fastcall AnsiStringBase(const AnsiStringBase& src);
    __fastcall AnsiStringBase(const char* src, int codePage);
    __fastcall AnsiStringBase(const char* src, int byteLen, int codePage);
    __fastcall AnsiStringBase(const wchar_t* src, int numwchar, int codePage);
    __fastcall AnsiStringBase(const char16_t* src, int numChar16, int codePage);
    __fastcall AnsiStringBase(const char32_t* src, int numChar32, int codePage);

    __fastcall AnsiStringBase(double src, int codePage);
    __fastcall AnsiStringBase(wchar_t src, int codePage);
    __fastcall AnsiStringBase(const WideString &src, int codePage);
    __fastcall AnsiStringBase(const UnicodeString &src, int codePage);
#if !defined(_DELPHI_STRING_UNICODE)
    __fastcall AnsiStringBase(const AnsiString &src, int /*codePage*/): Data(0) {
      *this = *((AnsiStringBase*)&src);
    }
#endif

    // Destructor
  public:
    __fastcall ~AnsiStringBase();

  protected:
    // Assignments
    AnsiStringBase& __fastcall operator =(const AnsiStringBase& rhs);

    // Flat helpers
    static void __fastcall _AnsiCat(AnsiStringBase& dst, const AnsiStringBase& src);
    static void __fastcall _AnsiCat(AnsiStringBase& dst, const AnsiStringBase& src1, const AnsiStringBase& src2);
    static void __fastcall _AnsiFromPWChar(AnsiStringBase& dst, const wchar_t* src, int len, int codePage);

    // Comparisons
    bool __fastcall operator ==(const AnsiStringBase& rhs) const;
    bool __fastcall operator !=(const AnsiStringBase& rhs) const;
    bool __fastcall operator <(const AnsiStringBase& rhs) const;
    bool __fastcall operator >(const AnsiStringBase& rhs) const;
    bool __fastcall operator <=(const AnsiStringBase& rhs) const { return !operator>(rhs); }
    bool __fastcall operator >=(const AnsiStringBase& rhs) const { return !operator<(rhs); }
    int  __fastcall AnsiCompare(const AnsiStringBase& rhs) const;
    int  __fastcall AnsiCompareIC(const AnsiStringBase& rhs) const; //ignorecase

  public:
    // --------------------------------------------------------------------
    // Accessing character at specified index
    // NOTE: To be compatible with Delphi, 'idx' is one-based
    // --------------------------------------------------------------------
    char __fastcall operator [](const int idx) const
    {
      ThrowIfOutOfRange(idx);   // Should Range-checking be optional to avoid overhead ??
#if !defined(_STRINGCHECKS_OFF)
      const_cast<AnsiStringBase*>(this)->EnsureAnsi();
#endif
      return Data[idx-1];
    }

#if defined(ANSISTRING_USE_PROXY_FOR_SUBSCRIPT)

    // The use of a proxy class optimizes the case where Unique() must be called
    // when accessing the string via the subscript operator. However, the use of
    // of the proxy class has some drawbacks. First, it breaks code that apply
    // operators to the return value. For example, &MyString[i]. Second, it
    // fails in cases where a implicit conversion was relied upon. For example,
    //       callFuncThatTakesAnObjectWithACharCtr(MyString[i]);
    // In that case, two implicit conversions would be required...
    // The first issue can be remedied by enhancing the proxy class to support
    // all valid operators. The second issue can be lessened but not completely
    // eliminated. Hence, the use of the PROXY class is not the default!
    //
  private:
    class  TCharProxy;
    friend TCharProxy;
    class  TCharProxy
    {
      public:
        TCharProxy(AnsiStringBase& strRef, int index) : m_Ref(strRef), m_Index(index) {}
        TCharProxy& operator=(char c) { m_Ref.Unique(); m_Ref.Data[m_Index-1] = c; return *this; }
        operator char() const         { return m_Ref.Data[m_Index-1]; }

      protected:
        AnsiStringBase&     m_Ref;
        int                 m_Index;
    };

  public:
    TCharProxy __fastcall operator [](const int idx)
    {
      ThrowIfOutOfRange(idx);   // Should Range-checking be optional to avoid overhead ??
      return TCharProxy(*this, idx);
    }

#else

    char& __fastcall operator [](const int idx)
    {
      ThrowIfOutOfRange(idx);   // Should Range-checking be optional to avoid overhead ??
      Unique();                 // Ensure we're not ref-counted (and Ansi)
      return Data[idx-1];
    }

#endif

    // Concatenation
    AnsiStringBase __fastcall operator +(const AnsiStringBase& rhs) const;

    // Query attributes of string
    int  __fastcall Length()  const;
    bool __fastcall IsEmpty() const { return Data == NULL; }

    // Make string unique (refcnt == 1)
    AnsiStringBase&  __fastcall Unique() {
      System::UniqueString(*PAnsiString(this));
      return *this;
    }

    // Ensure our payload is Ansi
    AnsiStringBase&  __fastcall EnsureAnsi() {
      if (Data && (((reinterpret_cast<StrRec*>(Data)[-1]).elemSize) == 2))
        Strhlpr::AnsiFromUnicode(*PRawByteString(this), *(reinterpret_cast<UnicodeString*>(this)));
      return *this;
    }

    // Modify string
    AnsiStringBase&  __fastcall Insert(const AnsiStringBase& str, int index);
    AnsiStringBase&  __fastcall Delete(int index, int count);
    AnsiStringBase&  __fastcall SetLength(int newLength, int codePage);

    int              __fastcall Pos(const AnsiStringBase& subStr) const;
    AnsiStringBase   __fastcall LowerCase(int codePage) const;
    AnsiStringBase   __fastcall UpperCase(int codePage) const;
    AnsiStringBase   __fastcall Trim(int codePage) const;
    AnsiStringBase   __fastcall TrimLeft(int codePage) const;
    AnsiStringBase   __fastcall TrimRight(int codePage) const;
    AnsiStringBase   __fastcall SubString(int index, int count) const;

    int          __fastcall ToInt() const;
    int          __fastcall ToIntDef(int defaultValue) const;
    double       __fastcall ToDouble() const;

    bool         __fastcall IsDelimiter(const AnsiStringBase& delimiters, int index) const;
    bool         __fastcall IsPathDelimiter(int index) const;
    int          __fastcall LastDelimiter(const AnsiStringBase& delimiters) const;
    char*        __fastcall LastChar() const;

    // Convert to Unicode
    int          __fastcall WideCharBufSize(int codePage) const;
    wchar_t*     __fastcall WideChar(wchar_t* dest, int destSize, int codePage) const;

    // MBCS support
    enum TStringMbcsByteType
    {mbSingleByte, mbLeadByte, mbTrailByte};

    TStringMbcsByteType __fastcall ByteType(int index) const;
    bool         __fastcall IsLeadByte(int index) const;
    bool         __fastcall IsTrailByte(int index) const;
    int          __fastcall AnsiPos(const AnsiStringBase& subStr) const;
    char*        __fastcall AnsiLastChar() const;

    DynamicArray<System::Byte> __fastcall BytesOf() const;

    unsigned short ElementSize() const { return Data ? GetRec().elemSize : (unsigned short)1; }
    int            RefCount()    const { return Data ? GetRec().refCnt   : 0; }

    unsigned short CodePage()    const { return Data ? GetRec().codePage : (unsigned short)System::DefaultSystemCodePage; }

    AnsiStringBase&    swap(AnsiStringBase& other);

  protected:
    void  __cdecl ThrowIfOutOfRange(int idx) const;

    struct StrRec {
      unsigned short codePage;
      unsigned short elemSize;
      int refCnt;
      int length;
    };

    const StrRec& GetRec() const;
    StrRec&       GetRec();

  protected:
    char *Data;
  };


  /////////////////////////////////////////////////////////////////////////////
  // AnsiStringT<CP> - C++ implementation of Delphi's AnsiString(CP) type
  /////////////////////////////////////////////////////////////////////////////
  template <unsigned short CP>
  class RTL_DELPHIRETURN AnsiStringT : public AnsiStringBase
  {
  public:
    static AnsiStringT __fastcall LoadStr(int ident) {
      return AnsiStringBase::LoadStr(ident, CP);
    }

    static AnsiStringT __fastcall LoadStr(HINSTANCE hInstance, int ident) {
      return AnsiStringBase::LoadStr(hInstance, ident, CP);
    }

    static AnsiStringT __fastcall FmtLoadStr(int ident, const TVarRec *args,
                                             int size) {
      return AnsiStringBase::FmtLoadStr(ident, args, size, CP);
    }

    AnsiStringT& __fastcall LoadString(HINSTANCE hInstance, int ident) {
      AnsiStringBase::LoadString(hInstance, ident, CP);
      return *this;
    }

    // String of specified character
    static AnsiStringT __fastcall StringOfChar(char ch, int count) {
      return AnsiStringBase::StringOfChar(ch, count);
    }

    // Delphi style 'Format'
    //
    static AnsiStringT __fastcall Format(const AnsiStringT& format,
                                         const TVarRec *args, int size) {
      return AnsiStringBase::Format(format, args, size, CP);
    }

    // C style 'sprintf' (NOTE: Target buffer is the string)
    //
    AnsiStringT& __cdecl sprintf(const char* format, ...) {
      va_list paramList;
      va_start(paramList, format);
      AnsiStringBase::vprintf(CP, format, paramList);
      va_end(paramList);
      return *this;
    }

    int __cdecl  printf(const char* format, ...) {
      int rc;
      va_list paramList;
      va_start(paramList, format);
      rc = AnsiStringBase::vprintf(CP, format, paramList);
      va_end(paramList);
      return rc;
    }

    int __cdecl  vprintf(const char* format, va_list list) {
      return AnsiStringBase::vprintf(CP, format, list);
    }

    // Like above, but appends to the string rather than overwrite
    AnsiStringT& __cdecl cat_sprintf(const char* format, ...) {
      va_list paramList;
      va_start(paramList, format);
      AnsiStringBase::cat_vprintf(CP, format, paramList);
      va_end(paramList);
      return *this;
    }

    int __cdecl cat_printf(const char* format, ...) {
      int rc;
      va_list paramList;
      va_start(paramList, format);
      rc = AnsiStringBase::cat_vprintf(CP, format, paramList);
      va_end(paramList);
      return rc;
    }

    int __cdecl cat_vprintf(const char* format, va_list list) {
      return AnsiStringBase::cat_vprintf(CP, format, list);
    }

    static AnsiStringT __fastcall FormatFloat(const AnsiStringT& format,
                                              const long double& value) {
      return AnsiStringBase::FormatFloat(format, value, CP);
    }

    static AnsiStringT __fastcall FloatToStrF(long double value,
                                              TStringFloatFormat format,
                                              int precision, int digits) {
      return AnsiStringBase::FloatToStrF(value, format, precision, digits, CP);
    }

    static AnsiStringT __fastcall IntToHex(int value, int digits) {
      return AnsiStringBase::IntToHex(value, digits, CP);
    }

    static AnsiStringT __fastcall CurrToStr(Currency value) {
      return AnsiStringBase::CurrToStr(value, CP);
    }

    static AnsiStringT __fastcall CurrToStrF(Currency value,
                                             TStringFloatFormat format,
                                             int digits) {
      return AnsiStringBase::CurrToStrF(value, format, digits, CP);
    }

    // Constructors
    __fastcall AnsiStringT() : AnsiStringBase() {}
    __fastcall AnsiStringT(const char* src) : AnsiStringBase(src, CP){}

    __fastcall AnsiStringT(const AnsiStringT& src) : AnsiStringBase(*((AnsiStringBase*)(&src))){}
    __fastcall AnsiStringT(const AnsiStringBase& src): AnsiStringBase(src) {}

    // Construct one AnsiStringT<CP> from one with another codepage affinity
    template <unsigned short OTHER_CP>
    AnsiStringT<CP>(const AnsiStringT<OTHER_CP>& src) : AnsiStringBase() {
      // Short-circuit case of LStrAddRef to preserve
      // payload as it could be in Unicode format
      // #pragma option push -w-8011
      if (this == &src) {
        if (Data && reinterpret_cast<const StrRec*>(Data)[-1].refCnt > 0) {
          reinterpret_cast<StrRec*>(Data)[-1].refCnt++;
        }
        return;
      }
      // #pragma option pop

      //  CodePage FFFF is a general receiver requiring no conversion
      if ((OTHER_CP == CP) || (CP == 0xFFFF)) {
        Data = static_cast<char*>(const_cast<void*>(src.data()));
        if (Data &&  reinterpret_cast<const StrRec*>(Data)[-1].refCnt > 0) {
          reinterpret_cast<StrRec*>(Data)[-1].refCnt++;
        }
        return;
      }

      // Up & down convert for other other code pages
      if (src.data()) {
        UnicodeString ustr(src);
        *this = ustr;
      }
    }

    __fastcall AnsiStringT(const char* src, int byteLen) : AnsiStringBase(src, byteLen, CP){}
    __fastcall AnsiStringT(const wchar_t* src, int numwchar = -1): AnsiStringBase(src, numwchar, CP){}
    __fastcall AnsiStringT(const char16_t* src, int numChar16 = -1) : AnsiStringBase(src, numChar16, CP){}
    __fastcall AnsiStringT(const char32_t* src, int numChar32 = -1) : AnsiStringBase(src, numChar32, CP){}

    __fastcall AnsiStringT(char src) { sprintf("%c", src);}
    __fastcall AnsiStringT(wchar_t src): AnsiStringBase(src, CP) {}
    __fastcall AnsiStringT(short src) { sprintf("%hd", src); }
    __fastcall AnsiStringT(unsigned short src) { sprintf("%hu", src); }
    __fastcall AnsiStringT(int src) { sprintf("%i", src); }
    __fastcall AnsiStringT(unsigned int src) { sprintf("%u", src); }
    __fastcall AnsiStringT(long src) { sprintf("%ld", src); }
    __fastcall AnsiStringT(unsigned long src) { sprintf("%lu", src); }
    __fastcall AnsiStringT(__int64 src) { sprintf("%Li", src); }
    __fastcall AnsiStringT(unsigned __int64 src) { sprintf("%Lu", src); }

    __fastcall AnsiStringT(double src): AnsiStringBase(src, CP){}
    __fastcall AnsiStringT(const WideString &src): AnsiStringBase(src, CP){}
    __fastcall AnsiStringT(const UnicodeString &src): AnsiStringBase(src, CP){}

    __fastcall ~AnsiStringT() {}

    // Assignments
    AnsiStringT& __fastcall operator =(const AnsiStringT& rhs) {
      AnsiStringBase::operator =(rhs);
      return *this;
    }

    AnsiStringT& __fastcall operator +=(const AnsiStringT& rhs) {
      _AnsiCat(*this, rhs);
      return *this;
    }

    // Comparisons
    bool __fastcall operator ==(const AnsiStringT& rhs) const {
      return AnsiStringBase::operator==(rhs);
    }
    bool __fastcall operator !=(const AnsiStringT& rhs) const {
      return AnsiStringBase::operator !=(rhs);
    }
    bool __fastcall operator <(const AnsiStringT& rhs) const {
      return AnsiStringBase::operator<(rhs);
    }
    bool __fastcall operator >(const AnsiStringT& rhs) const {
      return AnsiStringBase::operator >(rhs);
    }
    bool __fastcall operator <=(const AnsiStringT& rhs) const {
      return !operator>(rhs);
    }
    bool __fastcall operator >=(const AnsiStringT& rhs) const {
      return !operator<(rhs);
    }
    int  __fastcall AnsiCompare(const AnsiStringT& rhs) const {
      return AnsiStringBase::AnsiCompare(rhs);
    }
    int  __fastcall AnsiCompareIC(const AnsiStringT& rhs) const {
      return AnsiStringBase::AnsiCompareIC(rhs);
    }

    // Concatenation
    AnsiStringT __fastcall operator +(const AnsiStringT& rhs) const {
      AnsiStringT<CP> dst;
      _AnsiCat(dst, *this, rhs);
      return dst;
    }

    // C string operator
    char* __fastcall c_str() const {
      if (!Data) {
        return const_cast<char*>("");
      }
#if !defined(_STRINGCHECKS_OFF)
      const_cast<AnsiStringT*>(this)->EnsureAnsi();
#endif
      return Data;
    }

    // Read access to raw Data ptr.  Will be NULL for an empty string.
    const void* __fastcall data() const   { return Data; }

    // Make string unique (refcnt == 1)
    AnsiStringT&  __fastcall Unique() {
      System::UniqueString(*PAnsiString(this));
      return *this;
    }

    // Ensure our payload is narrow
    AnsiStringT&  __fastcall EnsureAnsi() {
      if (Data && (((reinterpret_cast<StrRec*>(Data)[-1]).elemSize) == 2)) {
        UniqueString(*reinterpret_cast<UnicodeString*>(this));
        Strhlpr::AnsiFromUnicode(*PRawByteString(this), *(reinterpret_cast<UnicodeString*>(this)));
      }
      return *this;
    }

    // Modify string
    AnsiStringT&  __fastcall Insert(const AnsiStringT& str, int index) {
      AnsiStringBase::Insert(str, index);
      return *this;
    }

    AnsiStringT&  __fastcall Delete(int index, int count) {
      AnsiStringBase::Delete(index, count);
      return *this;
    }

    AnsiStringT&  __fastcall SetLength(int newLength) {
      AnsiStringBase::SetLength(newLength, CP);
      return *this;
    }

    int __fastcall Pos(const AnsiStringT& subStr) const {
      return AnsiStringBase::Pos(subStr);
    }

    AnsiStringT __fastcall LowerCase() const {
      return AnsiStringBase::LowerCase(CP);
    }

    AnsiStringT __fastcall UpperCase() const {
      return AnsiStringBase::UpperCase(CP);
    }

    AnsiStringT __fastcall Trim() const {
      return AnsiStringBase::Trim(CP);
    }

    AnsiStringT __fastcall TrimLeft() const {
      return AnsiStringBase::TrimLeft(CP);
    }

    AnsiStringT __fastcall TrimRight() const {
      return AnsiStringBase::TrimRight(CP);
    }

    AnsiStringT __fastcall SubString(int index, int count) const {
      return AnsiStringBase::SubString(index, count);
    }

    bool __fastcall IsDelimiter(const AnsiStringT& delimiters, int index) const {
      return AnsiStringBase::IsDelimiter(delimiters, index);
    }

    int  __fastcall LastDelimiter(const AnsiStringT& delimiters) const {
      return AnsiStringBase::LastDelimiter(delimiters);
    }

    int  __fastcall WideCharBufSize() const {
      return AnsiStringBase::WideCharBufSize(CP);
    }

    wchar_t* __fastcall WideChar(wchar_t* dest, int destSize) const {
      return AnsiStringBase::WideChar(dest, destSize, CP);
    }

    int __fastcall AnsiPos(const AnsiStringT& subStr) const {
      return AnsiStringBase::AnsiPos(subStr);
    }

    unsigned short ElementSize() const { return Data ? GetRec().elemSize : (unsigned short)1; }
    int            RefCount()    const { return Data ? GetRec().refCnt   : 0; }

    unsigned short CodePage() const {
      return Data ? GetRec().codePage :
                   (CP ? (unsigned short)CP : (unsigned short)System::DefaultSystemCodePage);
    }

    AnsiStringT&   swap(AnsiStringT& other) {
      AnsiStringBase::swap(other);
      return *this;
    }
  };

  template <unsigned short CP>
  AnsiStringT<CP> __fastcall operator+(const char* p, const AnsiStringT<CP>& str) {
    AnsiStringT<CP> tmp(p);
    return tmp + str;
  }

#if defined(VCL_IOSTREAM)
  // see <sysclass.h>
  std::ostream& operator << (std::ostream& os, const AnsiString& arg);
  std::istream& operator >> (std::istream& is, AnsiString& arg);
#endif

#if !defined(__CODEGUARD__)

  // Codeguard is not very happy about our "reverse indexing" of the
  // Data pointer.  We'll address this by violating the ODR:  when
  // Codeguard compile checks are enabled, these methods will not be
  // inlined.  When building dstring.cpp, __DSTRING_INLINE will be
  // defined to generate out-of-line implementations of these methods.

  #if !defined(__DSTRING_INLINE)
  #define __DSTRING_INLINE inline
  #endif

  __DSTRING_INLINE const AnsiStringBase::StrRec &AnsiStringBase::GetRec() const
  {
    return reinterpret_cast<const StrRec *>(Data)[-1];
  }

  __DSTRING_INLINE AnsiStringBase::StrRec& AnsiStringBase::GetRec()
  {
    return reinterpret_cast<StrRec *>(Data)[-1];
  }

  __DSTRING_INLINE int __fastcall AnsiStringBase::Length() const
  {
    return (Data)? GetRec().length : 0;
  }

#undef __DSTRING_INLINE
#endif // !defined(__CODEGUARD__)

  /////////////////////////////////////////////////////////////////////////////
  // SmallStringBase
  /////////////////////////////////////////////////////////////////////////////
  template <unsigned char sz> class SmallStringBase
  {
  protected:
    unsigned char Len;
    char Data[sz];
  };


  /////////////////////////////////////////////////////////////////////////////
  // SmallString
  /////////////////////////////////////////////////////////////////////////////
  template <unsigned char sz> class SmallString : SmallStringBase<sz>
  {

  public:
    __fastcall SmallString() { Len = 0; }
    __fastcall SmallString(const SmallString& src);
    __fastcall SmallString(const char* src);

    __fastcall SmallString(const AnsiString& src)
    {
      long len = src.Length();
      Len = (unsigned char)((len > sz) ? sz : len);
      strncpy(Data, src.c_str(), Len);
    }

    char& __fastcall operator [](const unsigned char idx)
    { return Data[idx-1]; }

    SmallString& __fastcall operator =(const SmallString& rhs);

    __fastcall operator AnsiString() const;
  };

  template<unsigned char sz> __fastcall
  SmallString<sz>::SmallString(const char* src)
  {
    long len = strlen(src);
    Len = (unsigned char)((len > sz)? sz: len);
    strncpy(Data, src, Len);
  }

  template<unsigned char sz> __fastcall
  SmallString<sz>::SmallString(const SmallString& src)
  {
    Len = src.Len;
    for (int i = 0; i < Len; i++)
      Data[i] = src.Data[i];
  }

  template<unsigned char sz> SmallString<sz>& __fastcall
  SmallString<sz>::operator =(const SmallString& rhs)
  {
    if (this != &rhs)
    {
      Len = rhs.Len;
      for (int i = 0; i < Len; i++)
        Data[i] = rhs.Data[i];
    }
    return *this;
  }

  template<unsigned char sz>
  inline __fastcall SmallString<sz>::operator AnsiString() const
  {
    return AnsiString(Data, Len);
  }

#if defined(VCL_IOSTREAM)
  // see sysclass.h
  template<unsigned char sz>
  std::ostream& operator <<(std::ostream& os, const SmallString<sz>& arg);

  template<unsigned char sz>
  std::istream& operator >>(std::istream& is, SmallString<sz>& arg);
#endif

}
// using namespace System;

// The following is provided for backward compatibility.
// Otherwise, the new IntToStr(__int64) causes ambiguity for old code
// that used other integral types.
//
namespace Sysutils
{
  extern PACKAGE System::String __fastcall IntToStr(int Value)/* overload */;
  extern PACKAGE System::String __fastcall IntToStr(__int64 Value)/* overload */;
}

// #pragma option push -w-inl
#pragma warning(push, 1)

#if !defined(_DELPHI_STRING_UNICODE)
inline AnsiString __fastcall IntToStr(bool value)
{
  return Sysutils::IntToStr(int(value));
}
inline AnsiString __fastcall IntToStr(unsigned int value)
{
  return Sysutils::IntToStr(int(value));
}
inline AnsiString __fastcall IntToStr(long value)
{
  return Sysutils::IntToStr(int(value));
}
inline AnsiString __fastcall IntToStr(unsigned long value)
{
  return Sysutils::IntToStr(int(value));
}
#endif

// #pragma option pop
#pragma warning(pop)

// #pragma delphiheader end.

#endif  // DSTRING_H


