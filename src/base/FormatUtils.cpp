
#include <vcl.h>

#include "FormatUtils.h"

namespace nb {

UnicodeString Format(const UnicodeString & fmt, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  w.write(fmt.data(), args);
  return UnicodeString(w.data(), ToIntPtr(w.size()));
}

UnicodeString Sprintf(const UnicodeString & fmt, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  fmt::printf(w, fmt.data(), args);
  return UnicodeString(w.data(), ToIntPtr(w.size()));
}

UnicodeString FmtLoadStr(int32_t id, fmt::ArgList args)
{
  Expects(GetGlobals() != nullptr);
  UnicodeString Fmt = GetGlobals()->GetMsg(id);
  if (!Fmt.IsEmpty())
  {
    UnicodeString Result = Sprintf(Fmt, args);
    return Result;
  }
  DEBUG_PRINTF("Unknown resource string id: %d\n", id);
  return UnicodeString();
}

} // namespace nb
