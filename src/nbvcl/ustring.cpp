//  ustring.cpp - support for delphi Unicode strings in cpp
//  Copyright (c) 2007 Codegear Software Corp

#define __USTRING_INLINE

#include <System.hpp>
#include <StrHlpr.hpp>
// #include <windows.hpp>
// #include <sysutils.hpp>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

#include <ustring.h>

#if defined(INEFFICIENT_COPY_OF_CONST_DELPHIRETURN_TYPES)
#define _STR_CAST(type, arg) const_cast<type>(arg)
#else
#define _STR_CAST(type, arg) arg
#endif

namespace System
{
  class ERangeError : public std::exception
  {
    public:
        ERangeError(const char *str) : std::exception(str)
        {}
  };

  static int __fastcall wchar_tLen(const wchar_t* src)
  {
    int len = 0;
    if (src)
    {
       while (*src++)
         ++len;
    }
    return len;
  }

  static int UTF32ToUTF16(const char32_t* src, wchar_t* dst, int len)
  {
    int count = 0;
    while ((len != 0) && *src) {
      char32_t ch = *src++;
      if (ch < 0x0000FFFF) {
        count++;
        if (dst)
          *dst++ = (ch & 0x0000FFFF);
      }
      else if (ch < 0x0010FFFF) {
        count += 2;
        if (dst) {
          ch -= 0x10000;
          *dst++ = (ch >> 10)  | 0xD800;
          *dst++ = (ch & 0x3FF)| 0xDC00;
        }
      } else {
        count++;
        if (dst)
          *dst++ = 0x0000FFFD;
      }
      if (len > 0)
        len--;
    }
    return count;
  }

  void  __cdecl UnicodeString::ThrowIfOutOfRange(int idx) const
  {
    if (idx < 1 || idx > Length())    // NOTE: UnicodeString is 1-based !!
      throw ERangeError(Sysconst_SRangeError);
  }

  __fastcall UnicodeString::UnicodeString(const char* src): Data(0)
  {
    Strhlpr::UnicodeFromPChar(*this, const_cast<char*>(src));
  }

  __fastcall UnicodeString::UnicodeString(const UnicodeString& src) : Data(src.Data)
  {
    if (Data && reinterpret_cast<const StrRec*>(Data)[-1].refCnt > 0) {
      InterlockedIncrement((long*)&(reinterpret_cast<StrRec*>(Data)[-1].refCnt));

      // Short-circuit case of UStrAddRef
      if (this == &src)
        return;
      EnsureUnicode();
    }
  }

  __fastcall UnicodeString::UnicodeString(const WideString& src): Data(0)
  {
    Strhlpr::UnicodeFromWide(*this, _STR_CAST(WideString&, src));
  }

  __fastcall UnicodeString::UnicodeString(const wchar_t* src, int len) : Data(0)
  {
    if (len == -1) {
      len = wchar_tLen(src);
    }
    if (src && (len > 0))
    {
      SetLength(len);
      memcpy(Data, src, len*sizeof(wchar_t));
    }
  }

  __fastcall UnicodeString::UnicodeString(const wchar_t* src) : Data(0)
  {
    if (src && *src)
    {
      int len = wchar_tLen(src);
      SetLength(len);
      memcpy(Data, src, len*sizeof(wchar_t));
    }
  }

  __fastcall UnicodeString::UnicodeString(const char16_t* src, int numChar16/*= -1*/) : Data(0)
  {
    if (src && (numChar16 != 0))
    {
      int len = (numChar16 == -1) ? wchar_tLen((wchar_t*)src) : numChar16;
      SetLength(len);
      memcpy(Data, src, len*sizeof(char16_t));
    }
  }

  __fastcall UnicodeString::UnicodeString(const char32_t* src, int numChar32/*= -1*/) : Data(0)
  {
    if (src && (numChar32 != 0))
    {
      int len = UTF32ToUTF16(src, 0, numChar32);
      SetLength(len);
      UTF32ToUTF16(src, Data, numChar32);
    }
  }

  __fastcall UnicodeString::UnicodeString(const char* src, int len) : Data(0)
  {
    AnsiString str(src, len);
    Strhlpr::UnicodeFromAnsi(*this, *PRawByteString(&str));
  }

#if 0
  __fastcall UnicodeString::UnicodeString(char src) : Data(0)
  {
    SetLength(1);
    (*this)[1] = src;
  }

  __fastcall UnicodeString::UnicodeString(short src) : Data(0)
  {
     sprintf(L"%hd", src);
  }

  __fastcall UnicodeString::UnicodeString(unsigned short src) : Data(0)
  {
    sprintf(L"%hu", src);
  }

  __fastcall UnicodeString::UnicodeString(int src) : Data(0)
  {
    sprintf(L"%i", src);
  }

  __fastcall UnicodeString::UnicodeString(unsigned int src) : Data(0)
  {
    sprintf(L"%u", src);
  }

  __fastcall UnicodeString::UnicodeString(long src) : Data(0)
  {
    sprintf(L"%ld", src);
  }

  __fastcall UnicodeString::UnicodeString(unsigned long src) : Data(0)
  {
    sprintf(L"%lu", src);
  }

  __fastcall UnicodeString::UnicodeString(__int64 src) : Data(0)
  {
    sprintf(L"%Li", src);
  }

  __fastcall UnicodeString::UnicodeString(unsigned __int64 src) : Data(0)
  {
    sprintf(L"%Lu", src);
  }
#endif

  __fastcall UnicodeString::UnicodeString(double src) : Data(0)
  {
     *this = Sysutils::FloatToStr(src);
  }

  __fastcall UnicodeString::~UnicodeString()
  {
    Strhlpr::UnicodeFree(*this);
  }

  UnicodeString& __fastcall UnicodeString::operator=(const UnicodeString& src)
  {
    Strhlpr::UnicodeAssign(*this, const_cast<UnicodeString&>(src));
    return *this;
  }

  UnicodeString& __fastcall UnicodeString::operator+=(const UnicodeString& src)
  {
    Strhlpr::UnicodeAppend(*this, _STR_CAST(UnicodeString&, src));
    return *this;
  }

  UnicodeString __fastcall UnicodeString::operator+(const UnicodeString& rhs) const
  {
    UnicodeString tmp(*this);
    Strhlpr::UnicodeAppend(tmp, _STR_CAST(UnicodeString&, rhs));
    return tmp;
  }

  bool __fastcall UnicodeString::operator==(const UnicodeString& other) const
  {
    return Strhlpr::UnicodeEqual(_STR_CAST(UnicodeString&, *this), _STR_CAST(UnicodeString&, other));
  }

  bool __fastcall UnicodeString::operator!=(const UnicodeString& other) const
  {
    return !Strhlpr::UnicodeEqual(_STR_CAST(UnicodeString&, *this), _STR_CAST(UnicodeString&, other));
  }

  bool __fastcall UnicodeString::operator<(const UnicodeString& other) const
  {
    return Strhlpr::UnicodeLess(_STR_CAST(UnicodeString&, *this), _STR_CAST(UnicodeString&, other));
  }

  bool __fastcall UnicodeString::operator>(const UnicodeString& other) const
  {
    return Strhlpr::UnicodeGreater(_STR_CAST(UnicodeString&, *this), _STR_CAST(UnicodeString&, other));
  }

  bool __fastcall UnicodeString::operator<=(const UnicodeString& rhs) const
  {
    return !operator>(rhs);
  }

  bool __fastcall UnicodeString::operator >=(const UnicodeString& rhs) const
  {
    return !operator<(rhs);
  }

  int __fastcall UnicodeString::Compare(const UnicodeString& rhs) const
  {
    if (rhs.Data == Data) {
      return 0;
    }
    if (!Data || !rhs.Data) {
      return Data ? 1 : -1;
    }
    return ::CompareStringW(LOCALE_USER_DEFAULT, 0, Data, Length(),
                            rhs.Data, rhs.Length()) - CSTR_EQUAL;
  }

  int __fastcall UnicodeString::CompareIC(const UnicodeString& rhs) const
  {
    if (rhs.Data == Data) {
      return 0;
    }
    if (!Data || !rhs.Data) {
      return Data ? 1 : -1;
    }
    return ::CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, Data, Length(),
                            rhs.Data, rhs.Length()) - CSTR_EQUAL;
  }

  UnicodeString __fastcall UnicodeString::StringOfChar(wchar_t ch, int count)
  {
    UnicodeString tmp;
    tmp.SetLength(count);
    wchar_t* p = tmp.Data;
    while (count--)
      *p++ = ch;
    return tmp;
  }

  UnicodeString __fastcall UnicodeString::LoadStr(int ident)
  {
#if defined(_DELPHI_STRING_UNICODE)
    return Sysutils::LoadStr(ident);
#else
    // In ANSI mode VCL does not expose this
    // this functionality for UnicodeStrings.
    // So there's potential data loss
    return Sysutils::LoadStr(ident);
#endif
  }

  UnicodeString __fastcall UnicodeString::LoadStr(HINSTANCE hInstance, int ident)
  {
    UnicodeString str;
    str.LoadString(hInstance, ident);
    return str;
  }

  UnicodeString& __fastcall UnicodeString::LoadString(HINSTANCE hInstance, int id)
  {
    HRSRC resHdl = ::FindResource(hInstance, MAKEINTRESOURCE(id/16+1), RT_STRING);
    if (resHdl)
    {
      HGLOBAL gblHdl = ::LoadResource(hInstance, resHdl);
      if (gblHdl)
      {
        WCHAR* resData = (WCHAR*)::LockResource(gblHdl);
        if (resData)
        {
          unsigned int len;
          for (int cnt = id % 16; len = *resData++, cnt--; resData += len)
            ;
          if (len != 0)
          {
            SetLength(len);
            wchar_t* p = Data;
            int i = len;
            while (i--)
              *p++ = *resData++;
            if (len > 0)
              Data[len] = 0;
          }
          /* Unnecessary in Win32
          ::UnlockResource(gblHdl);
          */
        }
        ::FreeResource(resHdl);
      }
    }
    return *this;
  }

  UnicodeString __fastcall UnicodeString::FmtLoadStr(int ident, const TVarRec* args, int size)
  {
#if defined(_DELPHI_STRING_UNICODE)
    return Sysutils::FmtLoadStr(ident, args, size);
#else
    // In ANSI mode VCL does not expose this
    // this functionality for UnicodeStrings.
    // Hence, potential data loss!
    return Sysutils::FmtLoadStr(ident, args, size);
#endif
  }

  UnicodeString __fastcall UnicodeString::Format(const UnicodeString& format, const TVarRec *args, int size)
  {
#if defined(_DELPHI_STRING_UNICODE)
    return Sysutils::Format(format, args, size);
#else
    // In ANSI mode VCL does not expose this
    // this functionality for UnicodeStrings.
    // We use WideFormat instead
    return Sysutils::WideFormat(WideString(format), args, size);
#endif
  }

  UnicodeString __fastcall UnicodeString::FormatFloat(const UnicodeString& format, const long double& value)
  {
#if defined(_DELPHI_STRING_UNICODE)
    return Sysutils::FormatFloat(format, value);
#else
    // In ANSI mode VCL does not expose this
    // this functionality for UnicodeStrings.
    return Sysutils::FormatFloat(format, value);
#endif
  }


#undef vprintf
#undef printf
#undef sprintf

  int __cdecl UnicodeString::vprintf(const wchar_t* format, va_list paramList)
  {
    int size = vsnwprintf(NULL, 0, format, paramList);
    SetLength(size);
    return size ? vsnwprintf(Data, size + 1, format, paramList) : 0;
  }

  int __cdecl UnicodeString::cat_vprintf(const wchar_t* format, va_list paramList)
  {
    int size = vsnwprintf(NULL, 0, format, paramList);

    if (!size)
      return 0;

    int len = Length();
    SetLength(len + size);
    return vsnwprintf(Data + len, size + 1, format, paramList);
  }

  int __cdecl UnicodeString::printf(const wchar_t* format, ...)
  {
    int rc;
    va_list paramList;
    va_start(paramList, format);
    rc = vprintf(format, paramList);
    va_end(paramList);
    return rc;
  }

  UnicodeString& __cdecl UnicodeString::sprintf(const wchar_t* format, ...)
  {
    va_list paramList;
    va_start(paramList, format);
    vprintf(format, paramList);
    va_end(paramList);
    return *this;
  }

  int __cdecl UnicodeString::cat_printf(const wchar_t* format, ...)
  {
    int rc;
    va_list paramList;
    va_start(paramList, format);
    rc = cat_vprintf(format, paramList);
    va_end(paramList);
    return rc;
  }

  UnicodeString& __cdecl UnicodeString::cat_sprintf(const wchar_t* format, ...)
  {
    va_list paramList;
    va_start(paramList, format);
    cat_vprintf(format, paramList);
    va_end(paramList);
    return *this;
  }


  UnicodeString __fastcall UnicodeString::FloatToStrF(long double value, TStringFloatFormat format, int precision, int digits)
  {
    return Sysutils::FloatToStrF(value, TFloatFormat(format), precision, digits);
  }

  UnicodeString __fastcall UnicodeString::IntToHex(int value, int digits)
  {
    return Sysutils::IntToHex(value, digits);
  }

  UnicodeString __fastcall UnicodeString::CurrToStr(Currency value)
  {
    return Sysutils::CurrToStr(value);
  }

  UnicodeString __fastcall UnicodeString::CurrToStrF(Currency value, TStringFloatFormat format, int digits)
  {
    return Sysutils::CurrToStrF(value, TFloatFormat(format), digits);
  }

  UnicodeString& __fastcall UnicodeString::Unique()
  {
    System::UniqueString(*this);
    return *this;
  }

  UnicodeString& __fastcall UnicodeString::EnsureUnicode()
  {
    if (Data && (((reinterpret_cast<StrRec*>(Data)[-1]).elemSize) == 1)) {
      UniqueString(*PAnsiString(this));
      Strhlpr::UnicodeFromAnsi(*this, *PRawByteString(this));
    }
    return *this;
  }

  UnicodeString& __fastcall UnicodeString::Insert(const UnicodeString& source, int index)
  {
    Strhlpr::UnicodeInsert(*this, _STR_CAST(UnicodeString&, source), index);
    return *this;
  }

  UnicodeString& __fastcall UnicodeString::Delete(int index, int count)
  {
    Strhlpr::UnicodeDelete(*this, index, count);
    return *this;
  }

  UnicodeString& __fastcall UnicodeString::SetLength(int newLength)
  {
    Strhlpr::UnicodeSetLength(*this, newLength);
    return *this;
  }

  int __fastcall UnicodeString::Pos(const UnicodeString& subStr) const
  {
    return Strhlpr::UnicodePos(_STR_CAST(UnicodeString&, *this), _STR_CAST(UnicodeString&, subStr));
  }

  UnicodeString __fastcall UnicodeString::LowerCase() const
  {
    return Sysutils::AnsiLowerCase(*this);
  }

  UnicodeString __fastcall UnicodeString::UpperCase() const
  {
    return Sysutils::AnsiUpperCase(*this);
  }

  UnicodeString __fastcall UnicodeString::Trim() const
  {
    return Sysutils::Trim(*this);
  }

  UnicodeString __fastcall UnicodeString::TrimLeft() const
  {
    return Sysutils::TrimLeft(*this);
  }

  UnicodeString __fastcall UnicodeString::TrimRight() const
  {
    return Sysutils::TrimRight(*this);
  }

  UnicodeString __fastcall UnicodeString::SubString(int index, int count) const
  {
    // This method is intended to be compatible with Delphi's Copy().
    // Be careful when reordering parameter validation to maintain the
    // semantics!
    const int len = Length();
    if (index > len || count < 1)
      return UnicodeString();
    if (index < 1)
      index = 1;
    int n = len - index + 1;
    if (n > count)
      n = count;
    return UnicodeString(Data + index - 1, n);
  }

  int __fastcall UnicodeString::ToInt() const
  {
    return Sysutils::StrToInt(*this);
  }

  int __fastcall UnicodeString::ToIntDef(int defaultValue) const
  {
    return Sysutils::StrToIntDef(*this, defaultValue);
  }

  double __fastcall UnicodeString::ToDouble() const
  {
    return Sysutils::StrToFloat(*this);
  }

  bool __fastcall UnicodeString::IsDelimiter(const UnicodeString& delimiters, int index) const
  {
    return Sysutils::IsDelimiter(delimiters, *this, index);
  }

  bool __fastcall UnicodeString::IsPathDelimiter(int index) const
  {
    return Sysutils::IsPathDelimiter(*this, index);
  }

  int __fastcall UnicodeString::LastDelimiter(const UnicodeString& delimiters) const
  {
    return Sysutils::LastDelimiter(delimiters, *this);
  }

  UnicodeString::TStringLeadCharType __fastcall UnicodeString::ByteType(int index) const
  {
    return UnicodeString::TStringLeadCharType(Sysutils::ByteType(*this, index));
  }

  bool __fastcall UnicodeString::IsLeadSurrogate(int index) const
  {
    return ByteType(index) == ctbLeadSurrogate;
  }

  bool __fastcall UnicodeString::IsTrailSurrogate(int index) const
  {
    return ByteType(index) == ctTrailSurrogate;
  }

  DynamicArray<System::Byte> __fastcall UnicodeString::BytesOf() const
  {
    return Sysutils::BytesOf(*this);
  }

  UnicodeString __fastcall operator+(const char* lhs, const UnicodeString& rhs)
  {
    UnicodeString tmp(lhs);
    return Strhlpr::UnicodeCat(tmp, _STR_CAST(UnicodeString&, rhs));
  }

  UnicodeString __fastcall operator+(const wchar_t* lhs, const UnicodeString& rhs)
  {
    UnicodeString tmp(lhs);
    return Strhlpr::UnicodeCat(tmp, _STR_CAST(UnicodeString&, rhs));
  }

  wchar_t* __fastcall UnicodeString::LastChar() const
  {
    return Sysutils::AnsiLastChar(*PUnicodeString(this));
  }

  UnicodeString& UnicodeString::swap(UnicodeString& other)
  {
    Data = reinterpret_cast<wchar_t*>(InterlockedExchangePointer(&other.Data, Data));
    return *this;
  }

#if !defined(UNICODE) && !defined(_UNICODE)
  char*  __fastcall UnicodeString::t_str()
  {
    if (Data && (((reinterpret_cast<StrRec*>(Data)[-1]).elemSize) == 2)) {
      System::UniqueString(*this);
      Strhlpr::AnsiFromUnicode(*PRawByteString(this), *this);
    }
    return (Data) ? reinterpret_cast<char*>(Data) : const_cast<char*>("");
  }
#endif

} // System Namespace
