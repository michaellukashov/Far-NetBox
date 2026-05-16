// Stubs for symbols tinylog needs from nbcore/base at link time.
// Includes real headers to ensure ABI compatibility with tinylog library.

#include <cstdlib>
#include <cstring>

// Include real headers for ABI compatibility
#include <nbstring.h>

// dlmalloc stubs
extern "C"
{
  void dlfree(void * ptr) { free(ptr); }
  void * dlcalloc(size_t n, size_t size) { return calloc(n, size); }
  void * dlmalloc(size_t size) { return malloc(size); }
  void * dlrealloc(void * ptr, size_t size) { return realloc(ptr, size); }
}

// nbstr_release stub
void nbstr_release(CMStringData *) {}

// UnicodeString constructor stub (real header provides class layout)
UnicodeString::UnicodeString(const wchar_t * Str)
{
  Init(Str, CMStringW::StringLength(Str));
}

// os::debug::SetThreadName stub
namespace os
{
namespace debug
{
  void SetThreadName(void *, const UnicodeString &) {}
} // namespace debug
} // namespace os
