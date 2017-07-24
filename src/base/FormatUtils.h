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

NB_CORE_EXPORT UnicodeString Format(UnicodeString format_str, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, Format, UnicodeString)

NB_CORE_EXPORT UnicodeString Sprintf(UnicodeString format, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, Sprintf, UnicodeString)

NB_CORE_EXPORT UnicodeString FmtLoadStr(intptr_t Id, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, FmtLoadStr, intptr_t)

} // namespace nb

NB_CORE_EXPORT std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t>& os, const UnicodeString& value);
