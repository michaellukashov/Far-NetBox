
#include <vcl.h>

#include "FormatUtils.h"

namespace nb {

UnicodeString Format(const UnicodeString & fmt, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  w.write(fmt.data(), args);
  try
  {
    return UnicodeString(w.data(), nb::ToInt32(w.size()));
  }
  catch(const fmt::FormatError & ex)
  {
    DEBUG_PRINTF("Error: %s", UnicodeString(ex.what()));
    DebugAssert(false);
  }
  return UnicodeString();
}

UnicodeString Sprintf(const UnicodeString & fmt, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  try
  {
    fmt::printf(w, fmt.data(), args);
    return UnicodeString(w.data(), nb::ToInt32(w.size()));
  }
  catch(const fmt::FormatError & ex)
  {
    DEBUG_PRINTF("Error: %s", UnicodeString(ex.what()));
    DebugAssert(false);
  }
  return UnicodeString();
}

UnicodeString FmtLoadStr(int32_t id, fmt::ArgList args)
{
  Expects(GetGlobals() != nullptr);
  const UnicodeString Fmt = GetGlobals()->GetMsg(id);
  if (!Fmt.IsEmpty())
  {
    const UnicodeString Result = Sprintf(Fmt, args);
    return Result;
  }
  DEBUG_PRINTF("Unknown resource string id: %d\n", id);
  return UnicodeString();
}

} // namespace nb
