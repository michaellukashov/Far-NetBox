#include <vcl.h>
#pragma hdrstop

#ifdef _DEBUG
#include <stdio.h>
#include <rdestl/vector.h>
#include "Interface.h"
#endif // ifdef _DEBUG

#include <Global.h>

// TGuard

TGuard::TGuard(const TCriticalSection & ACriticalSection) :
  FCriticalSection(ACriticalSection)
{
  FCriticalSection.Enter();
}

TGuard::~TGuard()
{
  FCriticalSection.Leave();
}

// TUnguard

TUnguard::TUnguard(TCriticalSection & ACriticalSection) :
  FCriticalSection(ACriticalSection)
{
  FCriticalSection.Leave();
}

TUnguard::~TUnguard()
{
  FCriticalSection.Enter();
}

#ifdef _DEBUG

static HANDLE TraceFile = NULL;
BOOL IsTracing = false;
unsigned int CallstackTls = CallstackTlsOff;
TCriticalSection * TracingCriticalSection = NULL;

void SetTraceFile(HANDLE ATraceFile)
{
  TraceFile = ATraceFile;
  IsTracing = (TraceFile != 0);
  if (TracingCriticalSection == NULL)
  {
    TracingCriticalSection = new TCriticalSection();
  }
}

void CleanupTracing()
{
  if (TracingCriticalSection != NULL)
  {
    delete TracingCriticalSection;
    TracingCriticalSection = NULL;
  }
}

#ifdef TRACE_IN_MEMORY

struct TTraceInMemory
{
#ifdef TRACE_IN_MEMORY_NO_FORMATTING
  DWORD Ticks;
  DWORD Thread;
  const wchar_t * SourceFile;
  const wchar_t * Func;
  int Line;
  const wchar_t * Message;
#else
  UTF8String Message;
#endif // TRACE_IN_MEMORY_NO_FORMATTING
};
typedef rde::vector<TTraceInMemory> TTracesInMemory;
TTracesInMemory TracesInMemory;

int TraceThreadProc(void *)
{
  TRACE(L">");
  try
  {
    do
    {
      TRACE(L"2");
      TraceDumpToFile();
      TRACE(L"3");
      Sleep(60000);
      TRACE(L"4");
      // if resuming from sleep causes the previous Sleep to immediately break,
      // make sure we wait a little more before dumping
      Sleep(60000);
      TRACE(L"5");
    }
    while (true);
  }
  catch(...)
  {
    TRACE(L"E");
  }
  TRACE_EXIT;
  return 0;
}
#endif // TRACE_IN_MEMORY

#ifdef TRACE_IN_MEMORY_NO_FORMATTING

void Trace(const wchar_t * SourceFile, const wchar_t * Func,
  int Line, const wchar_t * Message)
{
  if (TracingCriticalSection != NULL)
  {
    TTraceInMemory TraceInMemory;
    TraceInMemory.Ticks = GetTickCount();
    TraceInMemory.Thread = GetCurrentThreadId();
    TraceInMemory.SourceFile = SourceFile;
    TraceInMemory.Func = Func;
    TraceInMemory.Line = Line;
    TraceInMemory.Message = Message;

    TGuard Guard(TracingCriticalSection);

    if (TracesInMemory.capacity() == 0)
    {
      TracesInMemory.reserve(100000);
      TThreadID ThreadID;
      StartThread(NULL, 0, TraceThreadProc, NULL, 0, ThreadID);
    }

    TracesInMemory.push_back(TraceInMemory);
  }
}

void TraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  int Line, const wchar_t * AFormat, TVarRec * /*Args*/, const int /*Args_Size*/)
{
  Trace(SourceFile, Func, Line, AFormat);
}

#endif // TRACE_IN_MEMORY_NO_FORMATTING

#ifdef TRACE_IN_MEMORY

void TraceDumpToFile()
{
  if (TraceFile != NULL)
  {
    TGuard Guard(TracingCriticalSection);

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
    WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, NULL);

    TTracesInMemory::const_iterator i = TracesInMemory.begin();
    while (i != TracesInMemory.end())
    {
      #ifdef TRACE_IN_MEMORY_NO_FORMATTING
      const wchar_t * SourceFile = i->SourceFile;
      const wchar_t * Slash = wcsrchr(SourceFile, L'\\');
      if (Slash != NULL)
      {
        SourceFile = Slash + 1;
      }

      TimeString =
        FormatDateTime(TimestampFormat,
          IncMilliSecond(N, -static_cast<int>(Ticks - i->Ticks)));
      Buffer = UTF8String(FORMAT(L"[%s] [%.4X] [%s:%d:%s] %s\n",
        (TimeString, int(i->Thread), SourceFile,
         i->Line, i->Func, i->Message)));
      WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, NULL);
      #else
      WriteFile(TraceFile, i->Message.c_str(), i->Message.Length(), &Written, NULL);
      #endif
      i++;
    }
    TracesInMemory.clear();

    TimeString = FormatDateTime(TimestampFormat, Now());
    Buffer = UTF8String(
      FORMAT("[%s] Done in-memory tracing =================================\n",
        (TimeString)));
    WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, NULL);
  }
}

void TraceInMemoryCallback(const wchar_t * Msg)
{
  if (IsTracing)
  {
    Trace(L"PAS", L"unk", GetCurrentThreadId(), Msg);
  }
}
#endif // TRACE_IN_MEMORY

#ifndef TRACE_IN_MEMORY_NO_FORMATTING

void Trace(const wchar_t * SourceFile, const wchar_t * Func,
  int Line, const wchar_t * Message)
{
  DebugAssert(IsTracing);

  UnicodeString TimeString;
  // DateTimeToString(TimeString, L"hh:mm:ss.zzz", Now());
  // TODO: use Format
  const wchar_t * Slash = wcsrchr(SourceFile, L'\\');
  if (Slash != NULL)
  {
    SourceFile = Slash + 1;
  }
  UTF8String Buffer = UTF8String(FORMAT(L"[%s] [%.4X] [%s:%d:%s] %s\n",
    (TimeString, int(GetCurrentThreadId()), SourceFile,
     Line, Func, Message)));
#ifdef TRACE_IN_MEMORY
  if (TracingCriticalSection != NULL)
  {
    TTraceInMemory TraceInMemory;
    TraceInMemory.Message = Buffer;

    TGuard Guard(TracingCriticalSection);

    if (TracesInMemory.capacity() == 0)
    {
      TracesInMemory.reserve(100000);
      TThreadID ThreadID;
      StartThread(NULL, 0, TraceThreadProc, NULL, 0, ThreadID);
    }

    TracesInMemory.push_back(TraceInMemory);
  }
#else
  DWORD Written;
  WriteFile(TraceFile, Buffer.c_str(), Buffer.Length(), &Written, NULL);
#endif TRACE_IN_MEMORY
}

void TraceFmt(const wchar_t * SourceFile, const wchar_t * Func,
  int Line, const wchar_t * AFormat, va_list Args)
{
  DebugAssert(IsTracing);

  UnicodeString Message = FormatV(AFormat, Args);
  Trace(SourceFile, Func, Line, Message.c_str());
}

#endif // TRACE_IN_MEMORY_NO_FORMATTING

void DoAssert(wchar_t * Message, wchar_t * Filename, int LineNumber)
{
  if (IsTracing)
  {
    Trace(Filename, L"assert", LineNumber, Message);
  }
  _wassert(Message, Filename, LineNumber);
}

#endif // _DEBUG
