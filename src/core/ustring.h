// USTRING.H - Support for delphi UnicodeString in C++
// Copyright (c) 2007 Codegear Software Corporation

#ifndef USTRING_H
#define USTRING_H

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
  // UnicodeString: String class compatible with Delphi's Native 'string' type
  /////////////////////////////////////////////////////////////////////////////
  class RTL_DELPHIRETURN UnicodeString
  {
    friend UnicodeString __fastcall operator +(const char*, const UnicodeString& rhs);
    friend UnicodeString __fastcall operator +(const wchar_t*, const UnicodeString& rhs);

  public:
    // the TStringFloatFormat enum is used by FloatToStrF
    enum TStringFloatFormat
    {sffGeneral, sffExponent, sffFixed, sffNumber, sffCurrency};
    static UnicodeString __fastcall LoadStr(int ident);
    static UnicodeString __fastcall LoadStr(HINSTANCE hInstance, int ident);
    static UnicodeString __fastcall FmtLoadStr(int ident, const TVarRec *args,
                        int size);

    UnicodeString&       __fastcall LoadString(HINSTANCE hInstance, int ident);

    // String of specified character
    static UnicodeString __fastcall StringOfChar(wchar_t ch, int count);

    // Delphi style 'Format'
    //
    static UnicodeString __fastcall Format(const UnicodeString& format,
                                           const TVarRec *args, int size);

    // C style 'sprintf' (NOTE: Target buffer is the string)
    //
    UnicodeString& __cdecl         sprintf(const wchar_t* format, ...); // Returns *this
    int            __cdecl          printf(const wchar_t* format, ...); // Returns formatted length
    int            __cdecl         vprintf(const wchar_t* format, va_list); // Returns formatted length


    // Like above, but appends to the string rather than overwrite
    UnicodeString& __cdecl     cat_sprintf(const wchar_t* format, ...); // Returns *this
    int            __cdecl      cat_printf(const wchar_t* format, ...); // Returns formatted length
    int            __cdecl     cat_vprintf(const wchar_t* format, va_list); // Returns formatted length

    static UnicodeString __fastcall FormatFloat(const UnicodeString& format,
                                                const long double& value);
    static UnicodeString __fastcall FloatToStrF(long double value,
                                                TStringFloatFormat format,
                                                int precision, int digits);
    static UnicodeString __fastcall IntToHex(int value, int digits);
    static UnicodeString __fastcall CurrToStr(Currency value);
    static UnicodeString __fastcall CurrToStrF(Currency value,
                                               TStringFloatFormat format,
                                               int digits);

    // Constructors
    __fastcall UnicodeString(): Data(0) {}
    __fastcall UnicodeString(const char* src);
    __fastcall UnicodeString(const UnicodeString& src);
    __fastcall UnicodeString(const wchar_t* src, int len);
    __fastcall UnicodeString(const char* src, int len);
    __fastcall UnicodeString(const wchar_t* src);
    __fastcall UnicodeString(const char16_t* src, int numChar16 = -1);
    __fastcall UnicodeString(const char32_t* src, int numChar32 = -1);

    __fastcall UnicodeString(char src): Data(0) { sprintf(L"%c", src);}
    __fastcall UnicodeString(wchar_t src): Data(0) { SetLength(1)[1] = src; }
    __fastcall UnicodeString(short src): Data(0) { sprintf(L"%hd", src); }
    __fastcall UnicodeString(unsigned short src): Data(0) { sprintf(L"%hu", src); }
    __fastcall UnicodeString(int src): Data(0) { sprintf(L"%i", src); }
    __fastcall UnicodeString(unsigned int src): Data(0) { sprintf(L"%u", src); }
    __fastcall UnicodeString(long src): Data(0) { sprintf(L"%ld", src); }
    __fastcall UnicodeString(unsigned long src): Data(0) { sprintf(L"%lu", src); }
    __fastcall UnicodeString(__int64 src): Data(0) { sprintf(L"%Li", src); }
    __fastcall UnicodeString(unsigned __int64 src): Data(0) { sprintf(L"%Lu", src); }
    __fastcall UnicodeString(double src);
    __fastcall UnicodeString(const WideString &src);

#if !defined(ANSISTRING_AS_TEMPLATE)    
    __fastcall UnicodeString(const AnsiString &src): Data(0) {
      Strhlpr::UnicodeFromAnsi(*this, const_cast<AnsiString&>(str));
    }
#else
    template <unsigned short codePage>
    __fastcall UnicodeString(const AnsiStringT<codePage> &src): Data(0) {
      Strhlpr::UnicodeFromAnsi(*this, *PRawByteString(&src));
    }
#endif


    // Destructor
    __fastcall ~UnicodeString();

    // Assignments
    UnicodeString& __fastcall operator =(const UnicodeString& rhs);
    UnicodeString& __fastcall operator +=(const UnicodeString& rhs);

    // Comparisons
    bool __fastcall operator ==(const UnicodeString& rhs) const;
    bool __fastcall operator !=(const UnicodeString& rhs) const;
    bool __fastcall operator < (const UnicodeString& rhs) const;
    bool __fastcall operator > (const UnicodeString& rhs) const;
    bool __fastcall operator <=(const UnicodeString& rhs) const;
    bool __fastcall operator >=(const UnicodeString& rhs) const;
    int  __fastcall Compare(const UnicodeString& rhs) const;
    int  __fastcall CompareIC(const UnicodeString& rhs) const; //ignorecase

    // --------------------------------------------------------------------
    // Accessing character at specified index
    // NOTE: To be compatible with Delphi, 'idx' is one-based
    // --------------------------------------------------------------------
    wchar_t __fastcall operator [](const int idx) const
    {
      ThrowIfOutOfRange(idx);   // Should Range-checking be optional to avoid overhead ??
#if !defined(_STRINGCHECKS_OFF)
      const_cast<UnicodeString*>(this)->EnsureUnicode();
#endif
      return Data[idx-1];
    }

    wchar_t& __fastcall operator [](const int idx)
    {
      ThrowIfOutOfRange(idx);   // Should Range-checking be optional to avoid overhead ??
      Unique();                 // Ensure we're not ref-counted (and Unicode)
      return Data[idx-1];
    }

    // Concatenation
    UnicodeString __fastcall operator +(const UnicodeString& rhs) const;

    wchar_t* __fastcall c_str() const   { return (Data)? Data: const_cast<wchar_t*>(L"");}
    wchar_t* __fastcall w_str() const   { return (Data)? Data: const_cast<wchar_t*>(L"");}

#if defined(UNICODE) || defined(_UNICODE)
    wchar_t* __fastcall t_str() const   { return (Data)? Data: const_cast<wchar_t*>(L"");}
#else
    char*    __fastcall t_str();
#endif

    // Read access to raw Data ptr.  Will be NULL for an empty string.
    const void* __fastcall data() const   { return Data; }

    // Query attributes of string
    int  __fastcall Length()  const;
    bool __fastcall IsEmpty() const { return Data == NULL; }

    // Make string unique (refcnt == 1)
    UnicodeString&  __fastcall Unique();

    // Ensure our payload is Unicode
    UnicodeString&  __fastcall EnsureUnicode();

    // Modify string
    UnicodeString&  __fastcall Insert(const UnicodeString& str, int index);
    UnicodeString&  __fastcall Delete(int index, int count);
    UnicodeString&  __fastcall SetLength(int newLength);

    int             __fastcall Pos(const UnicodeString& subStr) const;
    UnicodeString   __fastcall LowerCase() const;
    UnicodeString   __fastcall UpperCase() const;
    UnicodeString   __fastcall Trim() const;
    UnicodeString   __fastcall TrimLeft() const;
    UnicodeString   __fastcall TrimRight() const;
    UnicodeString   __fastcall SubString(int index, int count) const;

    int          __fastcall ToInt() const;
    int          __fastcall ToIntDef(int defaultValue) const;
    double       __fastcall ToDouble() const;

    bool         __fastcall IsDelimiter(const UnicodeString& delimiters, int index) const;
    bool         __fastcall IsPathDelimiter(int index) const;
    int          __fastcall LastDelimiter(const UnicodeString& delimiters) const;
    wchar_t*     __fastcall LastChar() const;

    enum TStringLeadCharType
    {ctNotLeadChar, ctbLeadSurrogate, ctTrailSurrogate};

    TStringLeadCharType __fastcall ByteType(int index) const;
    bool         __fastcall IsLeadSurrogate(int index) const;
    bool         __fastcall IsTrailSurrogate(int index) const;

    // NOTE: Uses default TEncoding. For other encodings, use TEncoding->ByteOf(uStr) 
    DynamicArray<System::Byte> __fastcall BytesOf() const;

    unsigned short ElementSize() const { return Data ? GetRec().elemSize : (unsigned short)2; }
    int            RefCount()    const { return Data ? GetRec().refCnt   : 0; }

    unsigned short CodePage()    const { return Data ? GetRec().codePage : (unsigned short)System::DefaultUnicodeCodePage; }

    UnicodeString& swap(UnicodeString& other); 

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

  private:
    wchar_t *Data;
  };

  extern UnicodeString __fastcall operator +(const char*, const UnicodeString&);
  extern UnicodeString __fastcall operator +(const wchar_t*, const UnicodeString&);

#if defined(VCL_IOSTREAM)
  // see <sysclass.h>
  std::wostream& operator << (std::wostream& os, const UnicodeString& arg);
  std::wistream& operator >> (std::wistream& is, UnicodeString& arg);
#endif

#if !defined(__CODEGUARD__)

  // Codeguard is not very happy about our "reverse indexing" of the
  // Data pointer.  We'll address this by violating the ODR:  when
  // Codeguard compile checks are enabled, these methods will not be
  // inlined.  When building dstring.cpp, __USTRING_INLINE will be
  // defined to generate out-of-line implementations of these methods.

  #if !defined(__USTRING_INLINE)
  #define __USTRING_INLINE inline
  #endif

  __USTRING_INLINE const UnicodeString::StrRec &UnicodeString::GetRec() const
  {
    return reinterpret_cast<const StrRec *>(Data)[-1];
  }

  __USTRING_INLINE UnicodeString::StrRec &UnicodeString::GetRec()
  {
    return reinterpret_cast<StrRec *>(Data)[-1];
  }

  __USTRING_INLINE int __fastcall UnicodeString::Length() const
  {
    return (Data)? GetRec().length : 0;
  }

#undef __USTRING_INLINE
#endif // !defined(__CODEGUARD__)

} // namespace System

// #pragma delphiheader end.

#endif  // USTRING_H


