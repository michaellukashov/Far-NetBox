
#include <vcl.h>
#pragma hdrstop

#ifdef _DEBUG
#include <stdio.h>
#include <rdestl/vector.h>
// TODO: remove src/core dep
#include <Interface.h>
#endif // ifdef _DEBUG

#include <Global.h>
//---------------------------------------------------------------------------
__removed #pragma package(smart_init)
//---------------------------------------------------------------------------
// TGuard
TGuard::TGuard(const TCriticalSection &ACriticalSection) :
  FCriticalSection(ACriticalSection)
{
  FCriticalSection.Enter();
}

TGuard::~TGuard()
{
  FCriticalSection.Leave();
}
//---------------------------------------------------------------------------
// TUnguard
//---------------------------------------------------------------------------
TUnguard::TUnguard(TCriticalSection &ACriticalSection) :
  FCriticalSection(ACriticalSection)
{
  FCriticalSection.Leave();
}
//---------------------------------------------------------------------------
TUnguard::~TUnguard()
{
  FCriticalSection.Enter();
}
//---------------------------------------------------------------------------
#ifdef _DEBUG

static HANDLE TraceFile = nullptr;
bool IsTracing = false;
const uintptr_t CallstackTlsOff = (uintptr_t) - 1;
uintptr_t CallstackTls = CallstackTlsOff;
TCriticalSection *TracingCriticalSection = nullptr;

bool TracingInMemory = false;
HANDLE TracingThread = nullptr;

typedef rde::vector<UTF8String> TTracesInMemory;
typedef rde::vector<TTracesInMemory *> TTracesInMemoryList;
TTracesInMemory *CurrentTracesInMemory = nullptr;
TTracesInMemoryList WriteTracesInMemory;
TTracesInMemoryList ReadTracesInMemory;
#define TracesInMemorySecond() (GetTickCount() % 1000)
int CurrentTracesInMemorySecond = -1;
std::unique_ptr<TCriticalSection> CurrentTracesInMemorySection;
std::unique_ptr<TCriticalSection> TracesInMemoryListsSection;

#define DirectTrace(MESSAGE) \
  DoDirectTrace(GetCurrentThreadId(), TEXT(__FILE__), TEXT(__FUNC__), __LINE__, (MESSAGE))
// Map to DirectTrace(MESSAGE) to enable tracing of in-memory tracing
#define MemoryTracingTrace(MESSAGE)

inline static UTF8String TraceFormat(TDateTime Time, DWORD Thread, const wchar_t *SourceFile,
  const wchar_t *Func, int Line, const wchar_t *Message)
{
  UnicodeString TimeString = DateTimeToString(Time); // TimeString, L"hh:mm:ss.zzz", Time);
  const wchar_t *Slash = wcsrchr(SourceFile, L'\\');
  if (Slash != nullptr)
  {
    SourceFile = Slash + 1;
  }
  UTF8String Buffer =
    UTF8String(FORMAT("[%s] [%.4X] [%s:%d:%s] %s\n",
        TimeString, int(Thread), SourceFile, Line, Func, Message));
  return Buffer;
}

inline static void WriteTraceBuffer(const char *Buffer, size_t Length)
{
  DWORD Written;
  ::WriteFile(TraceFile, Buffer, (DWORD)Length, &Written, nullptr);
}

inline static void DoDirectTrace(DWORD Thread, const wchar_t *SourceFile,
  const wchar_t *Func, int Line, const wchar_t *Message)
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

#ifdef TRACE_IN_MEMORY

struct TTraceInMemory
{
#ifdef TRACE_IN_MEMORY_NO_FORMATTING
  DWORD Ticks;
  DWORD Thread;
  const wchar_t *SourceFile;
  const wchar_t *Func;
  int Line;
  const wchar_t *Message;
#else
  UTF8String Message;
#endif // TRACE_IN_MEMORY_NO_FORMATTING
};
typedef rde::vector<TTraceInMemory> TTracesInMemory;
TTracesInMemory TracesInMemory;

int TraceThreadProc(void *)
{
  Trace(L">");
  try
  {
    do
    {
      Trace(L"2");
      TraceDumpToFile();
      Trace(L"3");
      ::Sleep(60000);
      Trace(L"4");
      // if resuming from sleep causes the previous Sleep to immediately break,
      // make sure we wait a little more before dumping
      ::Sleep(60000);
      Trace(L"5");
    }
    while (true);
  }
  catch (...)
  {
    Trace(L"E");
  }
  TraceExit();
  return 0;
}
#endif // TRACE_IN_MEMORY

#ifdef TRACE_IN_MEMORY_NO_FORMATTING

void DoTrace(const wchar_t *SourceFile, const wchar_t *Func,
  uintptr_t Line, const wchar_t *Message)
{
  if (TracingCriticalSection != nullptr)
  {
    TTraceInMemory TraceInMemory;
    TraceInMemory.Ticks = ::GetTickCount();
    TraceInMemory.Thread = ::GetCurrentThreadId();
    TraceInMemory.SourceFile = SourceFile;
    TraceInMemory.Func = Func;
    TraceInMemory.Line = Line;
    TraceInMemory.Message = Message;

    volatile TGuard Guard(TracingCriticalSection);

    if (TracesInMemory.capacity() == 0)
    {
      TracesInMemory.reserve(100000);
      TThreadID ThreadID;
      StartThread(nullptr, 0, TraceThreadProc, nullptr, 0, ThreadID);
    }

    TracesInMemory.push_back(TraceInMemory);
  }
}

void DoTraceFmt(const wchar_t *SourceFile, const wchar_t *Func,
  uintptr_t Line, const wchar_t *AFormat, TVarRec * /*Args*/, const int /*Args_Size*/)
{
  DoTrace(SourceFile, Func, Line, AFormat);
}

#endif // TRACE_IN_MEMORY_NO_FORMATTING

#ifdef TRACE_IN_MEMORY

void TraceDumpToFile()
{
  if (TraceFile != nullptr)
  {
    volatile TGuard Guard(TracingCriticalSection);

    DWORD Written;

    TDateTime N = Now();
#ifdef TRACE_IN_MEMORY_NO_FORMATTING
    DWORD Ticks = GetTickCount();
#endif

    const UnicodeString TimestampFormat = L"hh:mm:ss.zzz";
    UnicodeString TimeString = FormatDateTime(TimestampFormat, N);

    UTF8String Buffer = UTF8String(
        FORMAT("[%s] Dumping in-memory tracing =================================\n",
          (TimeString)));
    WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, nullptr);

    TTracesInMemory::const_iterator i = TracesInMemory.begin();
    while (i != TracesInMemory.end())
    {
#ifdef TRACE_IN_MEMORY_NO_FORMATTING
      const wchar_t *SourceFile = i->SourceFile;
      const wchar_t *Slash = wcsrchr(SourceFile, L'\\');
      if (Slash != nullptr)
      {
        SourceFile = Slash + 1;
      }

      TimeString =
        FormatDateTime(TimestampFormat,
          IncMilliSecond(N, -ToInt(Ticks - i->Ticks)));
      Buffer = UTF8String(FORMAT("[%s] [%.4X] [%s:%d:%s] %s\n",
            TimeString, int(i->Thread), SourceFile,
            i->Line, i->Func, i->Message));
      WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, nullptr);
#else
      WriteFile(TraceFile, i->Message.c_str(), i->Message.Length(), &Written, nullptr);
#endif
      ++i;
    }
    TracesInMemory.clear();

    TimeString = FormatDateTime(TimestampFormat, Now());
    Buffer = UTF8String(
        FORMAT("[%s] Done in-memory tracing =================================\n",
          (TimeString)));
    WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, nullptr);
  }
}

void TraceInMemoryCallback(const wchar_t *Msg)
{
  if (IsTracing)
  {
    DoTrace(L"PAS", L"unk", ::GetCurrentThreadId(), Msg);
  }
}
#endif // TRACE_IN_MEMORY

#ifndef TRACE_IN_MEMORY_NO_FORMATTING

void DoTrace(const wchar_t *SourceFile, const wchar_t *Func,
  uintptr_t Line, const wchar_t *Message)
{
  DebugAssert(IsTracing);

  UnicodeString TimeString;
  // DateTimeToString(TimeString, L"hh:mm:ss.zzz", Now());
  TODO("use Format");
  const wchar_t *Slash = wcsrchr(SourceFile, L'\\');
  if (Slash != nullptr)
  {
    SourceFile = Slash + 1;
  }
  UTF8String Buffer = UTF8String(FORMAT("[%s] [%.4X] [%s:%d:%s] %s\n",
        TimeString, int(::GetCurrentThreadId()), SourceFile,
        Line, Func, Message));
#ifdef TRACE_IN_MEMORY
  if (TracingCriticalSection != nullptr)
  {
    TTraceInMemory TraceInMemory;
    TraceInMemory.Message = Buffer;

    volatile TGuard Guard(TracingCriticalSection);

    if (TracesInMemory.capacity() == 0)
    {
      TracesInMemory.reserve(100000);
      TThreadID ThreadID;
      StartThread(nullptr, 0, TraceThreadProc, nullptr, 0, ThreadID);
    }

    TracesInMemory.push_back(TraceInMemory);
  }
#else
  DWORD Written;
  WriteFile(TraceFile, Buffer.c_str(), ToDWord(Buffer.Length()), &Written, nullptr);
#endif // TRACE_IN_MEMORY
}

void DoTraceFmt(const wchar_t *SourceFile, const wchar_t *Func,
  uintptr_t Line, const wchar_t *AFormat, fmt::ArgList args)
{
  DebugAssert(IsTracing);

  UnicodeString Message = nb::Format(AFormat, args);
  DoTrace(SourceFile, Func, Line, Message.c_str());
}

#endif // TRACE_IN_MEMORY_NO_FORMATTING

void DoAssert(const wchar_t *Message, const wchar_t *Filename, uintptr_t LineNumber)
{
  if (IsTracing)
  {
    DoTrace(Filename, L"assert", LineNumber, Message);
  }
  _wassert(Message, Filename, (unsigned int)LineNumber);
}

#endif // _DEBUG
