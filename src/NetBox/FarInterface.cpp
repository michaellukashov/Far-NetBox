//---------------------------------------------------------------------------
#include "nbafx.h"

#include "Common.h"
#include "CoreMain.h"
#include "FarConfiguration.h"
#include "WinSCPPlugin.h"
#include "Queue.h"
//---------------------------------------------------------------------------

TConfiguration * __fastcall CreateConfiguration()
{
  return new TFarConfiguration(FarPlugin);
}
//---------------------------------------------------------------------------
void __fastcall ShowExtendedException(Exception * E)
{
  assert(FarPlugin != NULL);
  TWinSCPPlugin * WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
  assert(WinSCPPlugin != NULL);
  WinSCPPlugin->ShowExtendedException(E);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall AppNameString()
{
  return L"NetBox";
}

//---------------------------------------------------------------------------
UnicodeString __fastcall GetRegistryKey()
{
  // return L"Software\\Michael Lukashov\\FarNetBox";
  return L"Software\\Far2\\Plugins\\NetBox 2";
}
//---------------------------------------------------------------------------
void __fastcall Busy(bool /*Start*/)
{
  // nothing
}
//---------------------------------------------------------------------------
UnicodeString __fastcall SshVersionString()
{
  return FORMAT(L"NetBox-FAR-release-%s", Configuration->GetVersion().c_str());
}

//---------------------------------------------------------------------------
DWORD WINAPI threadstartroutine(void * Parameter)
{
  TSimpleThread * SimpleThread = static_cast<TSimpleThread *>(Parameter);
  return TSimpleThread::ThreadProc(SimpleThread);
}
//---------------------------------------------------------------------------
size_t BeginThread(void * SecurityAttributes, DWORD StackSize,
                   void * Parameter, DWORD CreationFlags,
                   DWORD & ThreadId)
{
  HANDLE Result = ::CreateThread(static_cast<LPSECURITY_ATTRIBUTES>(SecurityAttributes),
                                 static_cast<size_t>(StackSize),
                                 static_cast<LPTHREAD_START_ROUTINE>(&threadstartroutine),
                                 Parameter,
                                 CreationFlags, &ThreadId);
  // DEBUG_PRINTF(L"Result = %d, ThreadId = %d", Result, ThreadId);
  return reinterpret_cast<size_t>(Result);
}

void EndThread(int ExitCode)
{
  ::ExitThread(ExitCode);
}

//---------------------------------------------------------------------------
int __fastcall StartThread(void * SecurityAttributes, unsigned int StackSize,
                           void * Parameter, unsigned int CreationFlags,
                           DWORD & ThreadId)
{
  return BeginThread(SecurityAttributes, StackSize, Parameter,
                     CreationFlags, ThreadId);
}
//---------------------------------------------------------------------------
void __fastcall CopyToClipboard(const UnicodeString Text)
{
  assert(FarPlugin != NULL);
  FarPlugin->FarCopyToClipboard(Text);
}
