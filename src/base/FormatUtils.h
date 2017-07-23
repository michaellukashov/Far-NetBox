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


NB_CORE_EXPORT UnicodeString nb_format(fmt::WCStringRef format_str, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, nb_format, fmt::WCStringRef)

NB_CORE_EXPORT UnicodeString nb_sprintf(fmt::WCStringRef format, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, nb_sprintf, fmt::WCStringRef)

NB_CORE_EXPORT std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t>& os, const UnicodeString& value);
