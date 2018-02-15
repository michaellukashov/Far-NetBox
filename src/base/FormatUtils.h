#pragma once

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include <fmt/format.h>
#include <fmt/printf.h>

#include "UnicodeString.hpp"

namespace nb {

NB_CORE_EXPORT UnicodeString Format(const UnicodeString format_str, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, Format, UnicodeString)

NB_CORE_EXPORT UnicodeString Sprintf(const UnicodeString format_str, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, Sprintf, UnicodeString)

NB_CORE_EXPORT UnicodeString FmtLoadStr(intptr_t Id, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, FmtLoadStr, intptr_t)

} // namespace nb

inline std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t> &os, const BaseStringT<wchar_t> &Value)
{
  os << Value.c_str();
  return os;
}

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char> &os, const BaseStringT<char> &Value)
{
  os << Value.c_str();
  return os;
}

//NB_CORE_EXPORT std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t> &os, const UnicodeString &Value);
//NB_CORE_EXPORT std::basic_ostream<char>& operator<<(std::basic_ostream<char> &os, const AnsiString &Value);
