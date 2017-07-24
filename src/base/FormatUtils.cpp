
#include <vcl.h>
#pragma hdrstop

#include "FormatUtils.h"

namespace nb {

UnicodeString Format(UnicodeString format_str, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  w.write(format_str.c_str(), args);
  return UnicodeString(w.c_str(), w.size());
}

UnicodeString Sprintf(UnicodeString format, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  fmt::printf(w, format.c_str(), args);
  return UnicodeString(w.c_str(), w.size());
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

std::basic_ostream<wchar_t> &operator<<(std::basic_ostream<wchar_t> &os, const UnicodeString &value)
{
  os << value.c_str();
  return os;
}

