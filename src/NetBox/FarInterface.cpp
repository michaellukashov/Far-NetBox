//---------------------------------------------------------------------------
#include "stdafx.h"

#include "Common.h"
#include "CoreMain.h"
#include "FarConfiguration.h"
#include "WinSCPPlugin.h"
#include "Queue.h"
//---------------------------------------------------------------------------

TConfiguration * CreateConfiguration()
{
  return new TFarConfiguration(FarPlugin);
}
//---------------------------------------------------------------------------
void ShowExtendedException(const std::exception * E)
{
  assert(FarPlugin != NULL);
  TWinSCPPlugin * WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
  assert(WinSCPPlugin != NULL);
  WinSCPPlugin->ShowExtendedException(E);
}
std::wstring AppNameString()
{
  return L"WinSCP";
}

//---------------------------------------------------------------------------
std::wstring GetRegistryKey()
{
#ifndef NETBOX_DEBUG
  return L"Software\\Martin Prikryl\\WinSCP 2";
#else
  return L"Software\\Michael Lukashov\\FarNetBox";
#endif
}
//---------------------------------------------------------------------------
void Busy(bool /*Start*/)
{
  // nothing
}
//---------------------------------------------------------------------------
std::wstring SshVersionString()
{
  return FORMAT(L"WinSCP-FAR-release-%s", Configuration->GetVersion().c_str());
}

//---------------------------------------------------------------------------
DWORD WINAPI threadstartroutine(void *Parameter)
{
    TSimpleThread *SimpleThread = (TSimpleThread *)Parameter;
    return TSimpleThread::ThreadProc(SimpleThread);
}
//---------------------------------------------------------------------------
int BeginThread(void *SecurityAttributes, DWORD StackSize,
  void *Parameter, DWORD CreationFlags,
  DWORD &ThreadId)
{
  HANDLE Result = ::CreateThread((LPSECURITY_ATTRIBUTES)SecurityAttributes,
    StackSize,
    (LPTHREAD_START_ROUTINE)&threadstartroutine,
    Parameter,
    CreationFlags, &ThreadId);
  // DEBUG_PRINTF(L"Result = %d, ThreadId = %d", Result, ThreadId);
  return (int)Result;
}

void EndThread(int ExitCode)
{
  ::ExitThread(ExitCode);
}

//---------------------------------------------------------------------------
int StartThread(void *SecurityAttributes, unsigned StackSize,
  void *Parameter, unsigned CreationFlags,
  DWORD &ThreadId)
{
  return BeginThread(SecurityAttributes, StackSize, Parameter,
    CreationFlags, ThreadId);
}
//---------------------------------------------------------------------------
void CopyToClipboard(std::wstring Text)
{
  assert(FarPlugin != NULL);
  FarPlugin->FarCopyToClipboard(Text);
}
