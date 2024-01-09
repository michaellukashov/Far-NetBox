#pragma once

#include <nbsystem.h>

namespace nb {

NB_CORE_EXPORT int32_t StrLength(const wchar_t* str);
NB_CORE_EXPORT int32_t StrLength(const char* str);
NB_CORE_EXPORT wchar_t Upper(wchar_t Ch);
NB_CORE_EXPORT wchar_t Lower(wchar_t Ch);
NB_CORE_EXPORT int32_t StrCmpNNI(const wchar_t* s1, int32_t n1, const wchar_t* s2, int32_t n2);
NB_CORE_EXPORT int32_t StrLIComp(const wchar_t* s1, const wchar_t* s2, int32_t n);
NB_CORE_EXPORT int32_t FarStrCmpI(const wchar_t* s1, const wchar_t* s2);
NB_CORE_EXPORT int32_t StrCmpNN(const wchar_t* s1, int32_t n1, const wchar_t* s2, int32_t n2);

enum TEncodeType
{
  etUSASCII,
  etUTF8,
  etANSI,
};

TEncodeType DetectUTF8Encoding(const uint8_t * str, int32_t len);
//TEncodeType DetectUTF8Encoding(RawByteString & str, int32_t len);

} // namespace nb

