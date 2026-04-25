// Minimal linker stubs for symbols tinylog needs from the base/UnicodeString module.
// Includes real header for ABI compatibility, provides only the used methods.

#pragma push_macro("ERROR")
#undef ERROR
#include <UnicodeString.hpp>
#pragma pop_macro("ERROR")

// UnicodeString::UnicodeString(const wchar_t *) — the only ctor tinylog calls
UnicodeString::UnicodeString(const wchar_t * Str)
  : Data(Str, CMStringW::StringLength(Str))
{
}

// os::debug::SetThreadName — TinyLog.cpp calls this once during init
namespace os
{
namespace debug
{
  void SetThreadName(void *, const UnicodeString &) {}
}
}
