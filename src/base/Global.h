
#pragma once

//#include "fmt/format.h"
#include "fmt/printf.h"

#include <nbcore.h>

#include <tchar.h>
#include <assert.h>

//#define FORMAT(S, ...) ::Format(S, ##__VA_ARGS__)
//#define FORMAT(S, ...) ::ToUnicodeString(fmt::format(S, __VA_ARGS__))
#define FORMAT(S, ...) ::ToUnicodeString(fmt::sprintf(S, __VA_ARGS__))
#define FMTLOAD(Id, ...) ::FmtLoadStr(Id, ##__VA_ARGS__)
#ifndef LENOF
#define LENOF(x) (_countof(X))
#endif
#define FLAGSET(SET, FLAG) (((SET) & (FLAG)) == (FLAG))
#define FLAGCLEAR(SET, FLAG) (((SET) & (FLAG)) == 0)
#define FLAGMASK(ENABLE, FLAG) ((ENABLE) ? (FLAG) : 0)

class NB_CORE_EXPORT TCriticalSection // : public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TCriticalSection)
public:
  TCriticalSection();
  ~TCriticalSection();

  void Enter() const;
  void Leave() const;

  int GetAcquired() const { return FAcquired; }

private:
  mutable CRITICAL_SECTION FSection;
  mutable int FAcquired;
};


class NB_CORE_EXPORT TGuard // : public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TGuard)
public:
  explicit TGuard(const TCriticalSection & ACriticalSection);
  ~TGuard();

private:
  const TCriticalSection & FCriticalSection;
};


class NB_CORE_EXPORT TUnguard // : public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TUnguard)
public:
  explicit TUnguard(TCriticalSection & ACriticalSection);
  ~TUnguard();

private:
  TCriticalSection & FCriticalSection;
};

#if !defined(_DEBUG)

#define DebugAssert(p) (void)(p)
#define DebugCheck(p) (p)
#define DebugFail() (void)0

#define DebugAlwaysTrue(p) (p)
#define DebugAlwaysFalse(p) (p)
#define DebugNotNull(p) (p)

#else // _DEBUG

NB_CORE_EXPORT void SetTraceFile(HANDLE ATraceFile);
NB_CORE_EXPORT void CleanupTracing();
#define TRACEENV "WINSCPTRACE"
NB_CORE_EXPORT extern bool IsTracing;
NB_CORE_EXPORT extern const uintptr_t CallstackTlsOff;
NB_CORE_EXPORT extern uintptr_t CallstackTls;
extern "C" NB_CORE_EXPORT void DoTrace(const wchar_t * SourceFile, const wchar_t * Func,
  uintptr_t Line, const wchar_t * Message);
NB_CORE_EXPORT void DoTraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  uintptr_t Line, const wchar_t * AFormat, va_list Args);

#ifdef TRACE_IN_MEMORY
NB_CORE_EXPORT void TraceDumpToFile();
NB_CORE_EXPORT void TraceInMemoryCallback(const wchar_t * Msg);

#endif // TRACE_IN_MEMORY

#define ACCESS_VIOLATION_TEST { (*((int*)nullptr)) = 0; }

NB_CORE_EXPORT void DoAssert(const wchar_t * Message, const wchar_t * Filename, uintptr_t LineNumber);

#define DebugAssert(p) ((p) ? (void)0 : DoAssert(TEXT(#p), TEXT(__FILE__), __LINE__))
#define DebugCheck(p) { bool __CHECK_RESULT__ = (p); DebugAssert(__CHECK_RESULT__); }
#define DebugFail() DebugAssert(false)

inline bool DoAlwaysTrue(bool Value, const wchar_t * Message, const wchar_t * Filename, uintptr_t LineNumber)
{
  if (!Value)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return Value;
}

inline bool DoAlwaysFalse(bool Value, const wchar_t * Message, const wchar_t * Filename, uintptr_t LineNumber)
{
  if (Value)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return Value;
}

template<typename T>
inline T * DoCheckNotNull(T * p, const wchar_t * Message, const wchar_t * Filename, uintptr_t LineNumber)
{
  if (p == nullptr)
  {
    DoAssert(Message, Filename, LineNumber);
  }
  return p;
}

#define DebugAlwaysTrue(p) DoAlwaysTrue((p), TEXT(#p), TEXT(__FILE__), __LINE__)
#define DebugAlwaysFalse(p) DoAlwaysFalse((p), TEXT(#p), TEXT(__FILE__), __LINE__)
#define DebugNotNull(p) DoCheckNotNull((p), TEXT(#p), TEXT(__FILE__), __LINE__)

#endif // _DEBUG

#define DebugUsedParam(p) (void)(p)

#define MB_TEXT(x) const_cast<wchar_t *>(::MB2W(x).c_str())

#define TShellExecuteInfoW _SHELLEXECUTEINFOW
#define TSHFileInfoW SHFILEINFOW
#define TVSFixedFileInfo VS_FIXEDFILEINFO
#define PVSFixedFileInfo VS_FIXEDFILEINFO*

