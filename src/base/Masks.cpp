
#include <vcl.h>

#include <Masks.hpp>
#include <nbutils.h>

namespace Masks {

static int32_t CmpName_Body(const wchar_t * pattern, const wchar_t * str, bool CmpNameSearchMode)
{
  for (;; ++str)
  {
    const wchar_t Stringc = nb::Upper(*str);

    switch (const wchar_t Patternc = nb::Upper(*pattern++))
    {
      case 0:
        return !Stringc;

      case L'?':
        if (!Stringc)
          return FALSE;
        break;

      case L'*':
        if (!*pattern)
          return TRUE;

        if (*pattern == L'.')
        {
          if (pattern[1] == L'*' && !pattern[2])
            return TRUE;

          if (!wcspbrk(pattern, L"*?["))
          {
            const wchar_t * dot = wcsrchr(str, L'.');

            if (!pattern[1])
              return !dot || !dot[1];

            const wchar_t * Pathdot = wcschr(pattern + 1, L'.');

            if (Pathdot && !dot)
              return FALSE;

            if (!Pathdot && dot )
              return !nb::FarStrCmpI(pattern + 1, dot + 1);
          }
        }

        do
        {
          if (CmpName(pattern, str, CmpNameSearchMode))
            return TRUE;
        }
        while (*str++);

        return FALSE;

      case L'[':
      {
        if (!wcschr(pattern, L']'))
        {
          if (Patternc != Stringc)
            return FALSE;

          break;
        }

        if (*pattern && *(pattern + 1) == L']')
        {
          if (*pattern != *str)
            return FALSE;

          pattern += 2;
          break;
        }

        int32_t match = 0;
        wchar_t Rangec;
        while ((Rangec = nb::Upper(*pattern++)) != 0)
        {
          if (Rangec == L']')
          {
            if (match)
              break;
            return FALSE;
          }

          if (match)
            continue;

          if (Rangec == L'-' && *(pattern - 2) != L'[' && *pattern != L']')
          {
            match = (Stringc <= nb::Upper(*pattern) &&
                     nb::Upper(*(pattern - 2)) <= Stringc);
            pattern++;
          }
          else
            match = (Stringc == Rangec);
        }

        if (!Rangec)
          return FALSE;
      }
      break;

      default:
        if (Patternc != Stringc)
        {
          if (Patternc == L'.' && !Stringc && !CmpNameSearchMode)
            return *pattern != L'.' && CmpName(pattern, str, CmpNameSearchMode);
          else
            return FALSE;
        }
        break;
    }
  }
}

int32_t CmpName(const wchar_t * pattern, const wchar_t * str, bool CmpNameSearchMode)
{
  if (!pattern || !str)
    return FALSE;

  //if (skippath)
  //  str=PointToName(str);

  return CmpName_Body(pattern, str, CmpNameSearchMode);
}

TMask::TMask(const UnicodeString & Mask) noexcept :
  FMask(Mask)
{
}

bool TMask::Matches(const UnicodeString & Str) const
{
  return CmpName(FMask.c_str(), Str.c_str()) != FALSE;
}

} // namespace Masks
