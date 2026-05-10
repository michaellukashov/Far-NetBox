#include <Global.h>
#include <LogContext.h>
#include <sstream>

// Thread-local storage for context stack
thread_local nb::vector_t<TLogContext::ContextEntry> TLogContext::ContextStack_;

TLogContext::TLogContext(const wchar_t * key, const UnicodeString & value)
  : Key_(key)
{
  ContextStack_.push_back({key, value});
}

TLogContext::~TLogContext()
{
  if (!ContextStack_.empty())
  {
    ContextStack_.pop_back();
  }
}

UnicodeString TLogContext::Format()
{
  std::wostringstream oss;
  for (const auto & entry : ContextStack_)
  {
    oss << "[" << entry.Key << "=" << entry.Value << "]";
  }
  return UnicodeString(oss.str().c_str());
}
