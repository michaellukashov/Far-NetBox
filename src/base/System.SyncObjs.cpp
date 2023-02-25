#include <vcl.h>

#include "System.SyncObjs.hpp"

// TCriticalSection

TCriticalSection::TCriticalSection() noexcept
{
  //OutputDebugStringA(LOCATION_STR.c_str());
  TINYLOG_TRACE_ENTER();
  InitializeCriticalSection(&FSection);
}

TCriticalSection::~TCriticalSection() noexcept
{
//  DEBUG_PRINTF("FAcquired: %d", FAcquired);
//  OutputDebugStringA(LOCATION_STR.c_str());
  if (FAcquired > 0)
  {
    tinylog::StackWalker sw;
    sw.ShowCallstack();
  }
  DebugAssert(FAcquired == 0);
  DeleteCriticalSection(&FSection);
}

void TCriticalSection::Enter() const
{
  ::EnterCriticalSection(&FSection);
  ++FAcquired;
//  DEBUG_PRINTF("FAcquired: %d", FAcquired);
}

void TCriticalSection::Leave() const
{
  --FAcquired;
//  DEBUG_PRINTF("FAcquired: %d", FAcquired);
  ::LeaveCriticalSection(&FSection);
}
