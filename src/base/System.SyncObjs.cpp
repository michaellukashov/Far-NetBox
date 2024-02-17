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
//  DEBUG_PRINTF("FAcquired: %d", FAcquired);
//  OutputDebugStringA(LOCATION_STR.c_str());
  if (FAcquired > 0)
  {
    TINYLOG_TRACE(g_tinylog) << repr("FAcquired: %d", FAcquired);
  }
  DebugAssert(FAcquired == 0);
  DeleteCriticalSection(&FSection);
}

void TCriticalSection::Enter() const
{
  ::EnterCriticalSection(&FSection);
  ++FAcquired;
//  DEBUG_PRINTF("FAcquired: %d", FAcquired);
  if (FAcquired == 3) // for debugging
  {
    TINYLOG_TRACE(g_tinylog) << repr("FAcquired: %d", FAcquired);
  }
}

void TCriticalSection::Leave() const
{
  --FAcquired;
//  DEBUG_PRINTF("FAcquired: %d", FAcquired);
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
