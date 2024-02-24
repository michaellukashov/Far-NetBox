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
  ::EnterCriticalSection(&FSection);
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
