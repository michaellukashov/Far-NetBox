//    wstring.h - support for delphi widestrings in c++
//                (WideString)
// 1.1
//    copyright (c) 1997, 1999 Borland International

#ifndef WSTRING_H
#define WSTRING_H

// #pragma delphiheader begin

// #include <sysmac.h>
#include <dstring.h>
#include <StrHlpr.hpp>

// Define this macro under Windows
#if defined(_Windows) && !defined(WIDESTRING_NO_BSTR)
#define WIDESTRING_IS_BSTR
#endif

//NOTE: sysmac.h defines WIDECHAR_IS_WCHAR when appropriate!

namespace System
{
  class RTL_DELPHIRETURN WideString
  {
  public:
    // the TStringFloatFormat enum is used by FloatToStrF
    enum TStringFloatFormat
    {sffGeneral, sffExponent, sffFixed, sffNumber, sffCurrency};

    // String of specified character
    static WideString __fastcall StringOfChar(WideChar ch, int count);

    // Delphi style 'Format'
    //
    static WideString __fastcall Format(const WideString& format,
                      const TVarRec *args, int size);

    // C style 'sprintf' (NOTE: Target buffer is the string)
    //
    WideString& __cdecl         sprintf(const wchar_t* format, ...); // Returns *this
    int         __cdecl          printf(const wchar_t* format, ...); // Returns formatted length
    int         __cdecl         vprintf(const wchar_t* format, va_list); // Returns formatted length


    // Like above, but appends to the string rather than overwrite
    WideString& __cdecl     cat_sprintf(const wchar_t* format, ...); // Returns *this
    int         __cdecl     cat_printf(const wchar_t* format, ...); // Returns formatted length
    int         __cdecl     cat_vprintf(const wchar_t* format, va_list); // Returns formatted length

    static WideString __fastcall FormatFloat(const WideString& format,
                                             const long double& value);
    static WideString __fastcall FloatToStrF(long double value,
                                             TStringFloatFormat format,
                                             int precision, int digits);
    static WideString __fastcall IntToHex(int value, int digits);
    static WideString __fastcall CurrToStr(Currency value);
    static WideString __fastcall CurrToStrF(Currency value,
                                            TStringFloatFormat format,
                                            int digits);

    // Constructors
    __fastcall WideString(): Data(0) {}
    __fastcall WideString(const char* src);
    __fastcall WideString(const WideString& src);
    __fastcall WideString(const WideChar* src, int len);
    __fastcall WideString(const char* src, int len);
    __fastcall WideString(const WideChar* src);
    // __fastcall WideString(const char16_t* src, int numChar16 = -1);
    __fastcall WideString(const char32_t* src, int numChar32 = -1);
    __fastcall explicit WideString(const WideChar src);
#if !defined(WIDECHAR_IS_WCHAR)
    __fastcall WideString(const wchar_t* src, int len);
    __fastcall WideString(const wchar_t* src);
    __fastcall explicit WideString(const wchar_t  src);
#endif
    __fastcall explicit WideString(char src);
    __fastcall explicit WideString(short src);
    // __fastcall explicit WideString(unsigned short);
    __fastcall explicit WideString(int src);
    __fastcall explicit WideString(unsigned int);
    __fastcall explicit WideString(long);
    __fastcall explicit WideString(unsigned long);
    __fastcall explicit WideString(__int64);
    __fastcall explicit WideString(unsigned __int64);
    __fastcall explicit WideString(float src);
    __fastcall explicit WideString(double src);
    __fastcall explicit WideString(long double src);
    __fastcall WideString(const UnicodeString& src);
#if !defined(ANSISTRING_AS_TEMPLATE)
    __fastcall WideString(const AnsiString &src): Data(0) {
      Strhlpr::WideFromAnsi(*this, *PRawByteString(&src));
    }
#else
    template <unsigned short codePage>
    __fastcall WideString(const AnsiStringT<codePage> &src): Data(0) {
      Strhlpr::WideFromAnsi(*this, *PRawByteString(&src));
    }
#endif

    // Destructor
    __fastcall ~WideString();

    // Assignments
    //
    WideString& __fastcall operator =(const WideString& rhs);
#ifdef WIDESTRING_IS_BSTR
    WideString& __fastcall operator =(BSTR rhs);
#endif
    WideString& __fastcall operator +=(const WideString& rhs);

    // Comparisons
    //
    bool __fastcall operator ==(const WideString& rhs) const;
    bool __fastcall operator !=(const WideString& rhs) const;
    bool __fastcall operator < (const WideString& rhs) const;
    bool __fastcall operator > (const WideString& rhs) const;
    bool __fastcall operator <=(const WideString& rhs) const;
    bool __fastcall operator >=(const WideString& rhs) const;

#ifdef WIDESTRING_IS_BSTR
#if !defined(WIDE_BSTR_GLOBAL_OPERATOR)
    bool __fastcall operator ==(const BSTR w) const;
    bool __fastcall operator !=(const BSTR w) const;
    bool __fastcall operator < (const BSTR w) const;
    bool __fastcall operator > (const BSTR w) const;
    bool __fastcall operator <=(const BSTR w) const;
    bool __fastcall operator >=(const BSTR w) const;
#endif // WIDE_BSTR_GLOBAL_OPERATOR
#endif

    bool __fastcall operator ==(const char* s) const
    { return operator ==(WideString(s)); }
    bool __fastcall operator !=(const char* s) const
    { return operator !=(WideString(s)); }
    bool __fastcall operator < (const char* s) const
    { return operator < (WideString(s)); }
    bool __fastcall operator > (const char* s) const
    { return operator > (WideString(s)); }
    bool __fastcall operator <=(const char* s) const
    { return operator <= (WideString(s)); }
    bool __fastcall operator >=(const char* s) const
    { return operator >= (WideString(s)); }

    // --------------------------------------------------------------------
    // Accessing character at specified index
    // NOTE: To be compatible with Delphi, 'idx' is one-based
    // --------------------------------------------------------------------
    WideChar & __fastcall operator [](const int idx) { return Data[idx-1]; }
    const WideChar& __fastcall operator [](const int idx) const { return Data[idx-1]; }

    // Concatenation
    //
    WideString __fastcall operator +(const WideString& rhs) const;

    // C string operator
    WideChar* __fastcall c_bstr() const            { return Data; }

    // Read access to raw Data ptr.
    WideChar* __fastcall data()                    { return Data; }

    // Query attributes of string
    int  __fastcall Length() const;
    bool __fastcall IsEmpty() const { return Data == 0; }

    // Modify string
    void __fastcall Insert(const WideString& str, int index);
    void __fastcall Delete(int index, int count);
    void __fastcall SetLength(int newLength);

    int          __fastcall Pos(const WideString& subStr) const;
    WideString   __fastcall LowerCase() const;
    WideString   __fastcall UpperCase() const;
    WideString   __fastcall Trim() const;
    WideString   __fastcall TrimLeft() const;
    WideString   __fastcall TrimRight() const;
    WideString   __fastcall SubString(int index, int count) const;

    int          __fastcall ToInt() const;
    int          __fastcall ToIntDef(int defaultValue) const;
    double       __fastcall ToDouble() const;

    bool         __fastcall IsDelimiter(const WideString& delimiters, int index) const;
    bool         __fastcall IsPathDelimiter(int index) const;
    int          __fastcall LastDelimiter(const WideString& delimiters) const;
    wchar_t*     __fastcall LastChar() const;

#ifdef WIDESTRING_IS_BSTR
    // The implicit operator BSTR() has been the source of many issues
    // mainly involving the compiler resorted to incorrrect/non-portable
    // pointer conversion/comparisons. For backward compatibility you can
    // enable the operator by defining NO_WIDESTRING_BSTR_OPERATOR_EXPLICIT
# if !defined(NO_WIDESTRING_BSTR_OPERATOR_EXPLICIT)
#   define WIDESTRING_BSTR_OPERATOR_EXPLICIT explicit
# else
#   define WIDESTRING_BSTR_OPERATOR_EXPLICIT
# endif
    WIDESTRING_BSTR_OPERATOR_EXPLICIT
    operator BSTR() const  { return Data; }
#endif

    // Access internal data (Be careful when using!!)
    //
#ifdef WIDESTRING_IS_BSTR
    // these are definitely going away:  we can either act like a BSTR
    // or a Pascal WideString.  the former is what we need.
    // --xmsb (2002-01-24 23:06)
    BSTR* __fastcall operator& ()
    {
      return &Data;
    }

    // Attach/Detach from BSTR, Empty Object
    //
    void __fastcall Attach(BSTR src);
    BSTR __fastcall Detach();
    void __fastcall Empty();

    // Retrieve copy of data
    //
    static wchar_t* __fastcall Copy(wchar_t* src);

    wchar_t* __fastcall Copy() const
    {
      return Copy(Data);
    }

#endif // WIDESTRING_IS_BSTR

    wchar_t* __fastcall Copy(wchar_t *, int len) const;

    WideString&    swap(WideString& other);

  private:
    // Initialize from a zero-terminated wide string
    void __fastcall InitStr(const WideChar*, int);
    WideChar* Data;
  };

  inline bool __fastcall operator ==(const char* str, const WideString& rhs) {
    return rhs.operator==(WideString(str));
  }

  inline bool __fastcall operator !=(const char* str, const WideString& rhs) {
    return rhs.operator!=(WideString(str));
  }

  inline bool __fastcall operator < (const char* str, const WideString& rhs) {
    return rhs.operator>(WideString(str));
  }

  inline bool __fastcall operator > (const char* str, const WideString& rhs) {
    return rhs.operator<(WideString(str));
  }

  inline bool __fastcall operator <=(const char* str, const WideString& rhs) {
    return rhs.operator>=(WideString(str));
  }

  inline bool __fastcall operator >=(const char* str, const WideString& rhs) {
    return rhs.operator<=(WideString(str));
  }


#ifdef WIDESTRING_IS_BSTR
  bool __fastcall operator ==(const BSTR w, const WideString& rhs);
  bool __fastcall operator !=(const BSTR w, const WideString& rhs);
  bool __fastcall operator < (const BSTR w, const WideString& rhs);
  bool __fastcall operator > (const BSTR w, const WideString& rhs);
  bool __fastcall operator <=(const BSTR w, const WideString& rhs);
  bool __fastcall operator >=(const BSTR w, const WideString& rhs);
#endif

}
// using namespace System;
// #pragma delphiheader end.

#endif
