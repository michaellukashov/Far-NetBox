// Minimal linker stubs for tinylog test executable.
// Provides only the symbols needed by libtinylog.a that come from
// the NetBox base/nbcore libraries, avoiding full base compilation.
//
// Includes real headers for ABI compatibility.

#include <nbstring.h>

// ---- dlmalloc stubs (nbcore uses dlmalloc) ----
extern "C"
{
  void dlfree(void * ptr) { free(ptr); }
  void * dlcalloc(size_t n, size_t size) { return calloc(n, size); }
  void * dlmalloc(size_t size) { return malloc(size); }
  void * dlrealloc(void * ptr, size_t size) { return realloc(ptr, size); }
}

// ---- nbstring stub ----
void nbstr_release(CMStringData *) {}

// ---- UnicodeString minimal ctor (ABI-compatible via real header) ----
UnicodeString::UnicodeString(const wchar_t * Str)
  : Data(Str, static_cast<unsigned int>(wcslen(Str)))
{
}

// ---- os::debug::SetThreadName stub ----
namespace os
{
namespace debug
{
  void SetThreadName(void *, const UnicodeString &) {}
}
}
