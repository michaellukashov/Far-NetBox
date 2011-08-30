//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <CoreMain.h>
#include "FarConfiguration.h"
#include "WinSCPPlugin.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
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
AnsiString __fastcall GetRegistryKey()
{
  return "Software\\Martin Prikryl\\WinSCP 2";
}
//---------------------------------------------------------------------------
void __fastcall Busy(bool /*Start*/)
{
  // nothing
}
//---------------------------------------------------------------------------
AnsiString __fastcall SshVersionString()
{
  return FORMAT("WinSCP-FAR-release-%s", (Configuration->Version));
}
//---------------------------------------------------------------------------
int __fastcall StartThread(void * SecurityAttributes, unsigned StackSize,
  TThreadFunc ThreadFunc, void * Parameter, unsigned CreationFlags,
  unsigned & ThreadId)
{
  return BeginThread(SecurityAttributes, StackSize, ThreadFunc, Parameter,
    CreationFlags, ThreadId);
}
//---------------------------------------------------------------------------
void __fastcall CopyToClipboard(AnsiString Text)
{
  assert(FarPlugin != NULL);
  FarPlugin->FarCopyToClipboard(Text);
}
