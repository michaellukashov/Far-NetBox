#pragma once

namespace nb {

intptr_t __cdecl StrLength(const wchar_t * str);
wchar_t __cdecl Upper(wchar_t Ch);
wchar_t __cdecl Lower(wchar_t Ch);
int __cdecl StrCmpNNI(const wchar_t * s1, int n1, const wchar_t * s2, int n2);
int __cdecl StrLIComp(const wchar_t * s1, const wchar_t * s2, int n);
int __cdecl FarStrCmpI(const wchar_t * s1, const wchar_t * s2);
int __cdecl StrCmpNN(const wchar_t * s1, int n1, const wchar_t * s2, int n2);

enum TEncodeType
{
  etUSASCII,
  etUTF8,
  etANSI,
};

TEncodeType DetectUTF8Encoding(const uint8_t * str, intptr_t len);

} // namespace nb

