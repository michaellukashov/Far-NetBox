
#pragma once

#include <nbcore.h>

#include <tchar.h>
#include <cassert>

#include <FormatUtils.h>
#include <tinylog/TinyLog.h>

#define FORMAT(S, ...) nb::Sprintf((S), __VA_ARGS__)
#define FMTLOAD(Id, ...) nb::FmtLoadStr((Id), __VA_ARGS__)
#ifndef LENOF
#define LENOF(x) (_countof((x)))
#endif
#define FLAGSET(SET, FLAG) (((SET) & (FLAG)) == (FLAG))
#define FLAGCLEAR(SET, FLAG) (((SET) & (FLAG)) == 0)
#define FLAGMASK(ENABLE, FLAG) ((ENABLE) ? (FLAG) : 0)

#include <System.SyncObjs.hpp>

// extern const UnicodeString EmptyString;

UnicodeString NormalizeString(const UnicodeString & S);

class NB_CORE_EXPORT TGuard
{
  CUSTOM_MEM_ALLOCATION_IMPL
  NB_DISABLE_COPY(TGuard)
public:
  TGuard() = delete;
  explicit TGuard(const TCriticalSection & ACriticalSection) noexcept;
  ~TGuard() noexcept;

private:
  const TCriticalSection & FCriticalSection;
};

class NB_CORE_EXPORT TUnguard
{
  CUSTOM_MEM_ALLOCATION_IMPL
  NB_DISABLE_COPY(TUnguard)
public:
  TUnguard() = delete;
  explicit TUnguard(TCriticalSection & ACriticalSection) noexcept;
  ~TUnguard() noexcept;

private:
  TCriticalSection & FCriticalSection;
};

//#include <assert.h>
#define ACCESS_VIOLATION_TEST { (*((int32_t*)nullptr)) = 0; }

#if !defined(_DEBUG) || defined(DESIGN_ONLY)

#define DebugAssert(p)   (void)(p)
#define DebugCheck(p)    (p)
#define DebugFail()      (void)0

#else // if !defined(_DEBUG) || defined(DESIGN_ONLY)

NB_CORE_EXPORT void DoAssert(const wchar_t * Message, const wchar_t * Filename, int32_t LineNumber);
NB_CORE_EXPORT extern "C" void DoAssertC(char * Message, char * Filename, int32_t LineNumber);
#define DebugAssert(p) ((p) ? (void)0 : DoAssert(NB_TEXT(#p), NB_TEXT(__FILE__), __LINE__))
#define DebugCheck(p) { bool __CHECK_RESULT__ = (p); DebugAssert(__CHECK_RESULT__); }
#define DebugFail() DebugAssert(false)
#endif // if !defined(_DEBUG) || defined(DESIGN_ONLY)

#define DebugAlwaysTrue(p) (p)
#define DebugAlwaysFalse(p) (p)
#define DebugNotNull(p) (p)
#define TraceInitPtr(p) (p)
#define TraceInitStr(p) (p)
#define DebugUsedParam(p) (void)(p)
#define DebugUsedArg(p)

#if defined(_DEBUG)

NB_CORE_EXPORT void SetTraceFile(HANDLE ATraceFile);
NB_CORE_EXPORT void CleanupTracing();
constexpr const char * TRACEENV = "WINSCPTRACE";
NB_CORE_EXPORT extern bool IsTracing;
NB_CORE_EXPORT extern const uint32_t CallstackTlsOff;
NB_CORE_EXPORT extern uint32_t CallstackTls;
extern "C" NB_CORE_EXPORT void DoTrace(const wchar_t * SourceFile, const wchar_t * Func,
  uint32_t Line, const wchar_t * Message);
NB_CORE_EXPORT void DoTraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  uint32_t Line, const wchar_t * AFormat, fmt::ArgList args);
FMT_VARIADIC_W(void, DoTraceFmt, const wchar_t *, const wchar_t *, uint32_t, const wchar_t *)

#ifdef TRACE_IN_MEMORY

NB_CORE_EXPORT void TraceDumpToFile();
NB_CORE_EXPORT void TraceInMemoryCallback(const wchar_t *Msg);

#endif // TRACE_IN_MEMORY

inline bool DoAlwaysTrue(bool Value, const wchar_t * Message, const wchar_t * Filename, uint32_t LineNumber)
{
  if (!Value)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return Value;
}

inline bool DoAlwaysFalse(bool Value, const wchar_t * Message, const wchar_t * Filename, uint32_t LineNumber)
{
  if (Value)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return Value;
}

template<typename T>
inline T * DoCheckNotNull(T * p, const wchar_t * Message, const wchar_t * Filename, uint32_t LineNumber)
{
  if (p == nullptr)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return p;
}

#undef DebugAlwaysTrue
#undef DebugAlwaysFalse
#undef DebugNotNull
#undef TraceInitPtr
#undef TraceInitStr

#define DebugAlwaysTrue(p) DoAlwaysTrue((p), TEXT(#p), TEXT(__FILE__), __LINE__)
#define DebugAlwaysFalse(p) DoAlwaysFalse((p), TEXT(#p), TEXT(__FILE__), __LINE__)
#define DebugNotNull(p) DoCheckNotNull((p), TEXT(#p), TEXT(__FILE__), __LINE__)
#define TraceInitPtr(p) (p)
#define TraceInitStr(p) (p)

#endif // #if defined(_DEBUG)

#define MB_TEXT(x) ::MB2W(x)

using TShellExecuteInfoW = struct _SHELLEXECUTEINFOW;
using TSHFileInfoW = struct _SHFILEINFOW;
using TVSFixedFileInfo = VS_FIXEDFILEINFO;
using PVSFixedFileInfo = VS_FIXEDFILEINFO*;

namespace os::debug {

void SetThreadName(HANDLE ThreadHandle, const UnicodeString & Name);
UnicodeString GetThreadName(HANDLE ThreadHandle);

} // namespace os::debug

