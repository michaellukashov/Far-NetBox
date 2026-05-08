#include <Global.h>
#include <LogContext.h>
#include <sstream>

// Thread-local storage for context stack
thread_local nb::vector_t<TLogContext::ContextEntry> TLogContext::ContextStack_;

TLogContext::TLogContext(const char* key, const std::string& value)
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

std::string TLogContext::Format()
{
  std::ostringstream oss;
  for (const auto& entry : ContextStack_)
  {
    oss << "[" << entry.Key << "=" << entry.Value << "]";
  }
  return oss.str();
}
