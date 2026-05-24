#include <Global.h>
#include <LogContext.h>
#include <sstream>

// TLS index — allocated once during first EnsureStack call across all threads.
// TlsAlloc/TlsGetValue/TlsSetValue are reliable in DLLs unlike thread_local
// which can fail dynamic-init in static-CRT plugin builds.
DWORD TLogContext::TlsIndex_ = TLS_OUT_OF_INDEXES;

nb::vector_t<TLogContext::ContextEntry> & TLogContext::EnsureStack()
{
  if (TlsIndex_ == TLS_OUT_OF_INDEXES)
  {
    TlsIndex_ = ::TlsAlloc();
  }
  auto * stack = static_cast<nb::vector_t<ContextEntry> *>(::TlsGetValue(TlsIndex_));
  if (!stack)
  {
    stack = new nb::vector_t<ContextEntry>();
    ::TlsSetValue(TlsIndex_, stack);
  }
  return *stack;
}

TLogContext::TLogContext(const wchar_t * key, const UnicodeString & value)
  : Key_(key)
{
  EnsureStack().push_back({key, value});
}

TLogContext::~TLogContext()
{
  auto & Stack = EnsureStack();
  if (!Stack.empty())
  {
    Stack.pop_back();
  }
}

std::string TLogContext::Format()
{
  std::wostringstream woss;
  auto & Stack = EnsureStack();
  for (const auto & entry : Stack)
  {
    woss << L"[" << entry.Key << L"=" << entry.Value << L"]";
  }
  std::wstring wstr = woss.str();
  int len = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
  std::string result(len > 0 ? len - 1 : 0, '\0');
  if (len > 1)
    ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
  else if (!wstr.empty())
    OutputDebugStringA("TLogContext::Format: WideCharToMultiByte failed\n");
  return result;
}
