#pragma once

#pragma push_macro("min")
#pragma push_macro("max")
#undef max
#undef min

#include <fmt/format.h>
#include <fmt/printf.h>

#include "UnicodeString.hpp"

namespace nb {

NB_CORE_EXPORT UnicodeString Format(const UnicodeString fmt, fmt::ArgList args);
FMT_VARIADIC_W(const UnicodeString, Format, const UnicodeString)

NB_CORE_EXPORT UnicodeString Sprintf(const UnicodeString fmt, fmt::ArgList args);
FMT_VARIADIC_W(const UnicodeString, Sprintf, const UnicodeString)

NB_CORE_EXPORT UnicodeString FmtLoadStr(intptr_t id, fmt::ArgList args);
FMT_VARIADIC_W(const UnicodeString, FmtLoadStr, intptr_t)

} // namespace nb

inline std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t> &os, const BaseStringT<wchar_t> &Value)
{
  os << Value.data();
  return os;
}

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char> &os, const BaseStringT<char> &Value)
{
  os << Value.data();
  return os;
}

#pragma pop_macro("min")
#pragma pop_macro("max")
