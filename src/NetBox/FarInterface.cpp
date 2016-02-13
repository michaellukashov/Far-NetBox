#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Queue.h>

#include "CoreMain.h"
#include "FarConfiguration.h"
#include "WinSCPPlugin.h"

TConfiguration * CreateConfiguration()
{
  return new TFarConfiguration(FarPlugin);
}

void ShowExtendedException(Exception * E)
{
  DebugAssert(FarPlugin != nullptr);
  TWinSCPPlugin * WinSCPPlugin = NB_STATIC_DOWNCAST(TWinSCPPlugin, FarPlugin);
  DebugAssert(WinSCPPlugin != nullptr);
  WinSCPPlugin->ShowExtendedException(E);
}

UnicodeString GetAppNameString()
{
  return "NetBox";
}

UnicodeString GetRegistryKey()
{
  return "NetBox 3"; // TODO: output MainGuid
}

void Busy(bool /*Start*/)
{
  // nothing
}

UnicodeString GetSshVersionString()
{
  return FORMAT(L"NetBox-FAR-release-%s", GetConfiguration()->GetProductVersion().c_str());
}

DWORD WINAPI threadstartroutine(void * Parameter)
{
  TSimpleThread * SimpleThread = NB_STATIC_DOWNCAST(TSimpleThread, Parameter);
  return TSimpleThread::ThreadProc(SimpleThread);
}

HANDLE BeginThread(void * SecurityAttributes, DWORD StackSize,
  void * Parameter, DWORD CreationFlags,
  DWORD & ThreadId)
{
  HANDLE Result = ::CreateThread(static_cast<LPSECURITY_ATTRIBUTES>(SecurityAttributes),
    static_cast<size_t>(StackSize),
    static_cast<LPTHREAD_START_ROUTINE>(&threadstartroutine),
    Parameter,
    CreationFlags, &ThreadId);
  return Result;
}

void EndThread(DWORD ExitCode)
{
  ::ExitThread(ExitCode);
}

HANDLE StartThread(void * SecurityAttributes, DWORD StackSize,
  void * Parameter, DWORD CreationFlags,
  TThreadID & ThreadId)
{
  return BeginThread(SecurityAttributes, StackSize, Parameter,
    CreationFlags, ThreadId);
}

void CopyToClipboard(const UnicodeString & AText)
{
  DebugAssert(FarPlugin != nullptr);
  FarPlugin->FarCopyToClipboard(AText);
}
