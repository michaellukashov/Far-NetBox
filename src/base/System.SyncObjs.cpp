#include <vcl.h>

#include "System.SyncObjs.hpp"

// TCriticalSection

TCriticalSection::TCriticalSection() noexcept
{
  //OutputDebugStringA(LOCATION_STR.c_str());
  //TINYLOG_TRACE_ENTER();
  InitializeCriticalSection(&FSection);
}

TCriticalSection::~TCriticalSection() noexcept
{
  DebugAssert(FAcquired == 0);
  DeleteCriticalSection(&FSection);
}

void TCriticalSection::Enter() const
{
#ifdef _DEBUG
  auto start = std::chrono::steady_clock::now();
#endif
  ::EnterCriticalSection(&FSection);
#ifdef _DEBUG
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - start);
  auto ms = elapsed.count();
  if (ms > 100)
  {
    TINYLOG_WARNING(g_tinylog) << "Lock contention: waited " << to_str(ms) << "ms";
  }
#endif
  ++FAcquired;
}

void TCriticalSection::Leave() const
{
  --FAcquired;
  ::LeaveCriticalSection(&FSection);
}

bool TCriticalSection::TryEnter()
{
  return ::TryEnterCriticalSection(&FSection) != FALSE;
}

void TCriticalSection::Release()
{
  ::LeaveCriticalSection(&FSection);
}
