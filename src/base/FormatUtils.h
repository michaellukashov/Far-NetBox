#pragma once

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "fmt/format.h"
#include "fmt/printf.h"

#include "UnicodeString.hpp"


//namespace nb {

inline UnicodeString nbformat(fmt::WCStringRef format_str, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  w.write(format_str, args);
  return UnicodeString(w.c_str(), w.size());
}
FMT_VARIADIC_W(UnicodeString, nbformat, fmt::WCStringRef)

inline UnicodeString nbsprintf(fmt::WCStringRef format, fmt::ArgList args)
{
  fmt::WMemoryWriter w;
  fmt::printf(w, format, args);
  return UnicodeString(w.c_str(), w.size());
}
FMT_VARIADIC_W(UnicodeString, nbsprintf, fmt::WCStringRef)

//} // namespace nb

//template <class T>
//inline UnicodeString ToUnicodeString(const T &a) { return UnicodeString(a.c_str(), (intptr_t)a.size()); }

inline std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t>& os, const UnicodeString& value)
{
  os << value.c_str();
  return os;
}
