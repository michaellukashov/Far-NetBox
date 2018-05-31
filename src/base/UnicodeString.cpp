#include <vcl.h>

#include "UnicodeString.hpp"
#include <Sysutils.hpp>

void ThrowIfOutOfRange(intptr_t Idx, intptr_t Length)
{
  if (Idx < 1 || Idx > Length) // NOTE: UnicodeString is 1-based !!
    throw Exception("Index is out of range"); // ERangeError(Sysconst_SRangeError);
}
