#include "stdafx.h"

#include <WinUser.h>
#pragma hdrstop

namespace nb {

intptr_t __cdecl StrLength(const wchar_t *str) { return wcslen(NullToEmpty(str)); }

wchar_t __cdecl Upper(wchar_t Ch)
{
  ::CharUpperBuff(&Ch, 1);
  return Ch;
}

wchar_t __cdecl Lower(wchar_t Ch)
{
  ::CharLowerBuff(&Ch, 1);
  return Ch;
}

intptr_t __cdecl StrCmpNNI(const wchar_t *s1, intptr_t n1, const wchar_t *s2, intptr_t n2)
{
  return ::CompareString(0, NORM_IGNORECASE | NORM_STOP_ON_NULL | SORT_STRINGSORT, s1, ToInt(n1), s2, ToInt(n2)) - 2;
}

intptr_t __cdecl StrLIComp(const wchar_t *s1, const wchar_t *s2, intptr_t n)
{
  return StrCmpNNI(s1, n, s2, n);
}

intptr_t __cdecl FarStrCmpI(const wchar_t *s1, const wchar_t *s2)
{
  return ::CompareString(0, NORM_IGNORECASE | SORT_STRINGSORT, s1, -1, s2, -1) - 2;
}

intptr_t __cdecl StrCmpNN(const wchar_t *s1, intptr_t n1, const wchar_t *s2, intptr_t n2)
{
  return ::CompareString(0, NORM_STOP_ON_NULL | SORT_STRINGSORT, s1, ToInt(n1), s2, ToInt(n2)) - 2;
}


TEncodeType DetectUTF8Encoding(const uint8_t *str, intptr_t len)
{
  const uint8_t *buf = str;
  const uint8_t *endbuf = buf + len;
  uint8_t byte2mask = 0x00;
  int trailing = 0; // trailing (continuation) bytes to follow

  while (buf != endbuf)
  {
    uint8_t c = *buf++;
    if (trailing)
    {
      if ((c & 0xC0) == 0x80) // Does trailing byte follow UTF-8 format?
      {
        if (byte2mask) // Need to check 2nd byte for proper range?
        {
          if (c & byte2mask) // Are appropriate bits set?
            byte2mask = 0x00;
          else
            return etANSI;
        }
        trailing--;
      }
      else
        return etANSI;
    }
    else
    {
      if ((c & 0x80) == 0x00)
        continue; // valid 1 byte UTF-8
      if ((c & 0xE0) == 0xC0) // valid 2 byte UTF-8
      {
        if (c & 0x1E) // Is UTF-8 byte in
          // proper range?
          trailing = 1;
        else
          return etANSI;
      }
      else if ((c & 0xF0) == 0xE0) // valid 3 byte UTF-8
      {
        if (!(c & 0x0F)) // Is UTF-8 byte in
          // proper range?
          byte2mask = 0x20; // If not set mask
        // to check next byte
        trailing = 2;
      }
      else if ((c & 0xF8) == 0xF0) // valid 4 byte UTF-8
      {
        if (!(c & 0x07)) // Is UTF-8 byte in
        {
          // proper range?

          byte2mask = 0x30; // If not set mask
        }
        // to check next byte
        trailing = 3;
      }
      else if ((c & 0xFC) == 0xF8) // valid 5 byte UTF-8
      {
        if (!(c & 0x03)) // Is UTF-8 byte in
          // proper range?
          byte2mask = 0x38; // If not set mask
        // to check next byte
        trailing = 4;
      }
      else if ((c & 0xFE) == 0xFC) // valid 6 byte UTF-8
      {
        if (!(c & 0x01)) // Is UTF-8 byte in
          // proper range?
          byte2mask = 0x3C; // If not set mask
        // to check next byte
        trailing = 5;
      }
      else
        return etANSI;
    }
  }
  return trailing == 0 ? etUTF8 : etANSI;
}

} // namespace nb
