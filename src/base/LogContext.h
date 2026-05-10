#pragma once

#include <nbcore.h>
#include <tinylog/TinyLog.h>
#include <UnicodeString.hpp>
// #include <string>
// #include <vector>

// Thread-local structured logging context
// Usage:
//   TLogContext ctx1("session", sessionId);
//   TLogContext ctx2("operation", "upload");
//   TINYLOG_INFO(g_tinylog) << TLogContext::Format() << " Starting operation";

class NB_CORE_EXPORT TLogContext
{
public:
  // Push a context value onto the thread-local stack
  TLogContext(const wchar_t * key, const UnicodeString & value);
  
  // Pop the context value when destroyed
  ~TLogContext();

  // Format all current context values as "[key1=val1][key2=val2]..."
  static UnicodeString Format();

private:
  struct ContextEntry
  {
    const wchar_t * Key;
    UnicodeString Value;
  };

  thread_local static nb::vector_t<ContextEntry> ContextStack_;
  const wchar_t * Key_{nullptr};
};

// Convenience macros for structured logging
#define LOG_CONTEXT(key, value) TLogContext _ctx_##key(key, value)
#define LOG_OPERATION(op, path) \
  TLogContext __ctx_op("op", op); TLogContext __ctx_path("path", path);
#define LOG_FILE(path) LOG_CONTEXT("file", path)
#define LOG_SESSION(id) LOG_CONTEXT("session", id)
