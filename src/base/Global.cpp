#include <vcl.h>
#pragma hdrstop

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

