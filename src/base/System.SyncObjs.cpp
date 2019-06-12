#include <vcl.h>

#include "System.SyncObjs.hpp"

// TCriticalSection

TCriticalSection::TCriticalSection() noexcept
{
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
