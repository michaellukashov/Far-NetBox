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

NB_CORE_EXPORT UnicodeString fmtformat(fmt::WCStringRef format_str, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, fmtformat, fmt::WCStringRef)

//NB_CORE_EXPORT UnicodeString fmtsprintf(fmt::WCStringRef format, fmt::ArgList args);
//FMT_VARIADIC_W(UnicodeString, fmtsprintf, fmt::WCStringRef)
NB_CORE_EXPORT UnicodeString fmtsprintf(UnicodeString format, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, fmtsprintf, UnicodeString)

NB_CORE_EXPORT UnicodeString fmtloadstr(intptr_t Id, fmt::ArgList args);
FMT_VARIADIC_W(UnicodeString, fmtloadstr, intptr_t)

} // namespace nb

NB_CORE_EXPORT std::basic_ostream<wchar_t>& operator<<(std::basic_ostream<wchar_t>& os, const UnicodeString& value);
