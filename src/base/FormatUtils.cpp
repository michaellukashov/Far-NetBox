
#include <vcl.h>
#pragma hdrstop

#include "FormatUtils.h"

UnicodeString nb_format(fmt::WCStringRef format_str, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  w.write(format_str, args);
  return UnicodeString(w.c_str(), w.size());
}

UnicodeString nb_sprintf(fmt::WCStringRef format, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  fmt::printf(w, format, args);
  return UnicodeString(w.c_str(), w.size());
}

std::basic_ostream<wchar_t> &operator<<(std::basic_ostream<wchar_t> &os, const UnicodeString &value)
{
  os << value.c_str();
  return os;
}
