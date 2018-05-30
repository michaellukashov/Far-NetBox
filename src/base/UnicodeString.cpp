#include <vcl.h>

#include "UnicodeString.hpp"
#include <Sysutils.hpp>

void ThrowIfOutOfRange(intptr_t Idx, intptr_t Length)
{
  if (Idx < 1 || Idx > Length) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}


template<typename CharT>
intptr_t BaseStringT<CharT>::CompareIC(const BaseStringT<CharT>& Str) const
{
  return ::AnsiCompareIC(*this, Str);
}

template<typename CharT>
intptr_t BaseStringT<CharT>::ToIntPtr() const
{
  return ::StrToIntDef(*this, 0);
}
