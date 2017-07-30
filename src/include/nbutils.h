#pragma once

#include <headers.hpp>

namespace nb {

NB_CORE_EXPORT intptr_t __cdecl StrLength(const wchar_t * str);
NB_CORE_EXPORT wchar_t __cdecl Upper(wchar_t Ch);
NB_CORE_EXPORT wchar_t __cdecl Lower(wchar_t Ch);
NB_CORE_EXPORT intptr_t __cdecl StrCmpNNI(const wchar_t * s1, intptr_t n1, const wchar_t * s2, intptr_t n2);
NB_CORE_EXPORT intptr_t __cdecl StrLIComp(const wchar_t * s1, const wchar_t * s2, intptr_t n);
NB_CORE_EXPORT intptr_t __cdecl FarStrCmpI(const wchar_t * s1, const wchar_t * s2);
NB_CORE_EXPORT intptr_t __cdecl StrCmpNN(const wchar_t * s1, intptr_t n1, const wchar_t * s2, intptr_t n2);

enum TEncodeType
{
  etUSASCII,
  etUTF8,
  etANSI,
};

TEncodeType DetectUTF8Encoding(const uint8_t * str, intptr_t len);

} // namespace nb

