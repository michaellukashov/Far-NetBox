#pragma once

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

//#include "fmt/format.h"
#include "fmt/printf.h"

#include "UnicodeString.hpp"

template <class T>
inline UnicodeString ToUnicodeString(const T &a) { return UnicodeString(a.c_str(), (intptr_t)a.size()); }

inline std::basic_ostream<wchar_t>& operator << (std::basic_ostream<wchar_t>& os, const UnicodeString& value)
{
  os << std::wstring(value.c_str());
  return os;
}
