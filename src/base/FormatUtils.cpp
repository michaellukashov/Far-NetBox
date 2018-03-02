
#include <vcl.h>
#pragma hdrstop

#include "FormatUtils.h"

namespace nb {

UnicodeString Format(const UnicodeString format_str, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  w.write(format_str.c_str(), args);
  return UnicodeString(w.c_str(), ToIntPtr(w.size()));
}

UnicodeString Sprintf(const UnicodeString format_str, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  fmt::printf(w, format_str.c_str(), args);
  return UnicodeString(w.c_str(), ToIntPtr(w.size()));
}

UnicodeString FmtLoadStr(intptr_t Id, fmt::ArgList args)
{
  UnicodeString Fmt = GetGlobals()->GetMsg(Id);
  if (!Fmt.IsEmpty())
  {
    UnicodeString Result = Sprintf(Fmt, args);
    return Result;
  }
  DEBUG_PRINTF("Unknown resource string id: %d\n", Id);
  return UnicodeString();
}

} // namespace nb
