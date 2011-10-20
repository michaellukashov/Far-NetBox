//---------------------------------------------------------------------------
#include "stdafx.h"

#include "Common.h"
#include "CoreMain.h"
#include "FarConfiguration.h"
#include "WinSCPPlugin.h"
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
  return L"Software\\Michael Lukashov\\TestNetBox";
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
struct TThreadRec
{
    TThreadRec(const threadfunc_slot_type &Func, void *Parameter) :
        Func(Func),
        Parameter(Parameter)
    {}
    const threadfunc_slot_type &Func;
    void *Parameter;
};
//---------------------------------------------------------------------------
DWORD WINAPI threadstartroutine(TThreadRec *rec)
{
    threadfunc_signal_type sig;
    sig.connect(rec->Func);
    return sig(rec->Parameter);
}
//---------------------------------------------------------------------------
int BeginThread(void *SecurityAttributes, DWORD StackSize,
  const threadfunc_slot_type &ThreadFunc, void *Parameter, DWORD CreationFlags,
  DWORD &ThreadId)
{
  TThreadRec *P = new TThreadRec(ThreadFunc, Parameter);
  HANDLE Result = ::CreateThread((LPSECURITY_ATTRIBUTES)SecurityAttributes,
    StackSize,
    (LPTHREAD_START_ROUTINE)&threadstartroutine,
    P,
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
  const threadfunc_slot_type &ThreadFunc, void *Parameter, unsigned CreationFlags,
  DWORD &ThreadId)
{
  return BeginThread(SecurityAttributes, StackSize, ThreadFunc, Parameter,
    CreationFlags, ThreadId);
}
//---------------------------------------------------------------------------
void CopyToClipboard(std::wstring Text)
{
  assert(FarPlugin != NULL);
  FarPlugin->FarCopyToClipboard(Text);
}
