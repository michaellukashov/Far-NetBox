#include "stdafx.h"

#include <WinUser.h>
#pragma hdrstop

namespace nb {

intptr_t __cdecl StrLength(const wchar_t * str) { return wcslen(NullToEmpty(str)); }

wchar_t __cdecl Upper(wchar_t Ch) { ::CharUpperBuff(&Ch, 1); return Ch; }

wchar_t __cdecl Lower(wchar_t Ch) { ::CharLowerBuff(&Ch, 1); return Ch; }

int __cdecl StrCmpNNI(const wchar_t * s1, int n1, const wchar_t * s2, int n2) { return ::CompareString(0, NORM_IGNORECASE|NORM_STOP_ON_NULL|SORT_STRINGSORT, s1, n1, s2, n2) - 2; }
int __cdecl StrLIComp(const wchar_t * s1, const wchar_t * s2, int n) { return StrCmpNNI(s1, n, s2, n); }

int __cdecl FarStrCmpI(const wchar_t * s1, const wchar_t * s2) { return ::CompareString(0, NORM_IGNORECASE|SORT_STRINGSORT, s1,-1, s2, -1) - 2; }

int __cdecl StrCmpNN(const wchar_t * s1, int n1, const wchar_t * s2, int n2) { return ::CompareString(0, NORM_STOP_ON_NULL|SORT_STRINGSORT, s1, n1, s2, n2) - 2; }

} // namespace nb
