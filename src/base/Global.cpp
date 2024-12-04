
#include <vcl.h>
//#pragma hdrstop

#ifdef _DEBUG
//#include <cstdio>
// TODO: remove src/core dep
#include <Interface.h>
#endif // ifdef _DEBUG

#include <Global.h>

#if defined(__BORLANDC__)
#pragma package(smart_init)

const UnicodeString EmptyString(TraceInitStr(L"\1\1\1")); // magic
#endif // defined(__BORLANDC__)

UnicodeString NormalizeString(const UnicodeString & S)
{
  UnicodeString Result = S;
  if (Result == L"\1\1\1")
  {
    Result = UnicodeString();
  }
  return Result;
}

// TGuard

TGuard::TGuard(const TCriticalSection & ACriticalSection) noexcept :
  FCriticalSection(ACriticalSection)
{
  FCriticalSection.Enter();
}

TGuard::~TGuard() noexcept
{
  FCriticalSection.Leave();
}

// TUnguard

TUnguard::TUnguard(TCriticalSection & ACriticalSection) noexcept :
  FCriticalSection(ACriticalSection)
{
  FCriticalSection.Leave();
}

TUnguard::~TUnguard() noexcept
{
  FCriticalSection.Enter();
}

#ifdef _DEBUG

static HANDLE TraceFile = nullptr;
bool IsTracing = false;
const uint32_t CallstackTlsOff = static_cast<uint32_t>(-1);
uint32_t CallstackTls = CallstackTlsOff;
TCriticalSection * TracingCriticalSection = nullptr;

bool TracingInMemory = false;
HANDLE TracingThread = nullptr;

#define DirectTrace(MESSAGE) \
  DoDirectTrace(GetCurrentThreadId(), TEXT(__FILE__), TEXT(__FUNC__), __LINE__, (MESSAGE))
// Map to DirectTrace(MESSAGE) to enable tracing of in-memory tracing
#define MemoryTracingTrace(MESSAGE)

inline static UTF8String TraceFormat(const TDateTime & Time, DWORD Thread, const wchar_t * SourceFile,
  const wchar_t * Func, int32_t Line, const wchar_t * Message)
{
  const UnicodeString TimeString = DateTimeToString(Time); // TimeString, L"hh:mm:ss.zzz", Time);
  const wchar_t * Slash = wcsrchr(SourceFile, Backslash);
  if (Slash != nullptr)
  {
    SourceFile = Slash + 1;
  }
  UTF8String Buffer =
    UTF8String(FORMAT("[%s] [%.4X] [%s:%d:%s] %s\n",
        TimeString, nb::ToInt32(Thread), SourceFile, Line, Func, Message));
  return Buffer;
}

inline static void WriteTraceBuffer(const char * Buffer, size_t Length)
{
  DWORD Written;
  ::WriteFile(TraceFile, Buffer, static_cast<DWORD>(Length), &Written, nullptr);
}

inline static void DoDirectTrace(DWORD Thread, const wchar_t * SourceFile,
  const wchar_t * Func, int32_t Line, const wchar_t * Message)
{
  UTF8String Buf = TraceFormat(Now(), Thread, SourceFile, Func, Line, Message);
  WriteTraceBuffer(Buf.c_str(), Buf.Length());
}

void SetTraceFile(HANDLE ATraceFile)
{
  TraceFile = ATraceFile;
  IsTracing = (TraceFile != nullptr);
  if (TracingCriticalSection == nullptr)
  {
    TracingCriticalSection = new TCriticalSection();
  }
}

void CleanupTracing()
{
  if (TracingCriticalSection != nullptr)
  {
    delete TracingCriticalSection;
    TracingCriticalSection = nullptr;
  }
}

#ifndef TRACE_IN_MEMORY_NO_FORMATTING

void DoTrace(const wchar_t * SourceFile, const wchar_t * Func,
  uint32_t Line, const wchar_t * Message)
{
  // DebugAssert(IsTracing);

  // const UnicodeString TimeString = DateTimeToString(L"hh:mm:ss.zzz", Now());
  const UnicodeString TimeString = DateTimeToString("%H:%M:%S", Now());
  // DEBUG_PRINTF("TimeString: %s", TimeString);
  const wchar_t * Slash = wcsrchr(SourceFile, Backslash);
  if (Slash != nullptr)
  {
    SourceFile = Slash + 1;
  }
  UTF8String Buffer = UTF8String(FORMAT("[%s] [%.4X] [%s:%d:%s] %s\n",
    TimeString, nb::ToInt32(::GetCurrentThreadId()), SourceFile,
    Line, Func, Message));
  DWORD Written;
  WriteFile(TraceFile, Buffer.c_str(), nb::ToDWord(Buffer.Length()), &Written, nullptr);
}

void DoTraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  uint32_t Line, const wchar_t * AFormat, fmt::ArgList args)
{
  DebugAssert(IsTracing);

  UnicodeString Message = nb::Format(AFormat, args);
  DoTrace(SourceFile, Func, Line, Message.c_str());
}

#endif // TRACE_IN_MEMORY_NO_FORMATTING

void DoAssert(const wchar_t * Message, const wchar_t * Filename, int32_t LineNumber)
{
  if (IsTracing)
  {
    DoTrace(Filename, L"assert", LineNumber, Message);
  }
  _wassert(Message, Filename, static_cast<uint32_t>(LineNumber));
}

NB_CORE_EXPORT extern "C" void DoAssertC(char * Message, char * Filename, int32_t LineNumber)
{
  DoTrace(UnicodeString(Filename).c_str(), L"assert", LineNumber, UnicodeString(Message).c_str());
  _wassert(UnicodeString(Message).c_str(), UnicodeString(Filename).c_str(), static_cast<uint32_t>(LineNumber));
}

#endif // _DEBUG

namespace os::debug
{

void SetThreadName(HANDLE ThreadHandle, const UnicodeString & Name)
{
  // from osquery\osquery\core\system.cpp
#if defined(WIN32)
  // SetThreadDescription is available in builds newer than 1607 of windows 10
  // and works even if there is no debugger.
  typedef HRESULT(WINAPI * PFNSetThreadDescription)(HANDLE hThread,
                                                    PCWSTR lpThreadDescription);
  auto pfnSetThreadDescription = reinterpret_cast<PFNSetThreadDescription>(
    GetProcAddress(GetModuleHandleA("Kernel32.dll"), "SetThreadDescription"));
  if (pfnSetThreadDescription != nullptr)
  {
    HRESULT hr = pfnSetThreadDescription(ThreadHandle, Name.c_str());
    if (!FAILED(hr))
    {
      // DEBUG_PRINTF("SetThreadDescription: success");
    }
  }
  else
  {
    // DEBUG_PRINTF("SetThreadName failed due to GetProcAddress returning null");
  }
#endif
}

UnicodeString GetThreadName(HANDLE ThreadHandle)
{
  UnicodeString Result;
#if defined(WIN32)
  typedef HRESULT(WINAPI * PFNGetThreadDescription)(HANDLE hThread, PWSTR* ThreadDescription);
  auto pfnGetThreadDescription = reinterpret_cast<PFNGetThreadDescription>(
    GetProcAddress(GetModuleHandleA("Kernel32.dll"), "GetThreadDescription"));
  if (pfnGetThreadDescription != nullptr)
  {
    wchar_t * ThreadDescription{};
    HRESULT hr = pfnGetThreadDescription(ThreadHandle, &ThreadDescription);
    if (!FAILED(hr))
    {
      // DEBUG_PRINTF("GetThreadDescription: success");
      Result = ThreadDescription;
    }
  }
  else
  {
    // DEBUG_PRINTF("GetThreadName failed due to GetProcAddress returning null");
  }
#endif

  return Result;
}

} // namespace os::debug
