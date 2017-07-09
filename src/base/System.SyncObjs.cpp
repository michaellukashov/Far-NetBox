#include <System.SyncObjs.hpp>

// TCriticalSection

TCriticalSection::TCriticalSection() :
  FAcquired(0)
{
  InitializeCriticalSection(&FSection);
}

TCriticalSection::~TCriticalSection()
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
