#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Queue.h>

#include "CoreMain.h"
#include "FarConfiguration.h"
#include "WinSCPPlugin.h"
#include "FarDialog.h"
#include "FarInterface.h"
#include <process.h>

TConfiguration * CreateConfiguration()
{
  TConfiguration * Result = new TFarConfiguration(FarPlugin);
  Result->ConfigurationInit();
  Result->Default();
  DebugAssert(WinConfiguration == nullptr);
  WinConfiguration = new TWinConfiguration();
  WinConfiguration->ConfigurationInit();
  WinConfiguration->Default();
  return Result;
}

void ShowExtendedException(Exception * E)
{
  Ensures(FarPlugin != nullptr);
  TWinSCPPlugin * WinSCPPlugin = nb::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
  Ensures(WinSCPPlugin != nullptr);
  WinSCPPlugin->ShowExtendedException(E);
}

UnicodeString GetAppNameString()
{
  return "NetBox";
}

UnicodeString GetRegistryKey()
{
  return "NetBox 3"; // TODO: output NetBoxPluginGuid
}

void Busy(bool /*Start*/)
{
  // nothing
}

UnicodeString GetSshVersionString()
{
  UnicodeString Result = UnicodeString("NetBox-Far");
  const UnicodeString ProductVersion = GetConfiguration()->GetProductVersion();
  if (!ProductVersion.IsEmpty())
    Result += FORMAT("-%s", ProductVersion);
  return Result;
}

static DWORD WINAPI threadstartroutine(void * Parameter)
{
  TSimpleThread * SimpleThread = static_cast<TSimpleThread *>(Parameter);
  return TSimpleThread::ThreadProc(SimpleThread);
}

HANDLE BeginThread(void * SecurityAttributes, DWORD StackSize,
  void * Parameter, DWORD CreationFlags,
  DWORD & ThreadId)
{
  const HANDLE Result = reinterpret_cast<HANDLE>(::_beginthreadex(
    static_cast<LPSECURITY_ATTRIBUTES>(SecurityAttributes),
    static_cast<unsigned int>(StackSize),
    reinterpret_cast<_beginthreadex_proc_type>(&threadstartroutine),
    Parameter,
    CreationFlags,
    reinterpret_cast<unsigned int *>(&ThreadId)));
  return Result;
}

[[noreturn]] void EndThread(DWORD ExitCode)
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

//from windows/GUITools.cpp
template <class TEditControl>
void ValidateMaskEditT(const UnicodeString & Mask, TEditControl * Edit, int32_t ForceDirectoryMasks)
{
  DebugAssert(Edit != nullptr);
  TFileMasks Masks(ForceDirectoryMasks);
  try
  {
    Masks = Mask;
  }
  catch (EFileMasksException & E)
  {
    ShowExtendedException(&E);
    Edit->SetFocus();
    // This does not work for TEdit and TMemo (descendants of TCustomEdit) anymore,
    // as it re-selects whole text on exception in TCustomEdit.CMExit
//    Edit->SelStart = E.ErrorStart - 1;
//    Edit->SelLength = E.ErrorLen;
    Abort();
  }
}

void ValidateMaskEdit(TFarComboBox * Edit)
{
  Expects(Edit);
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}

void ValidateMaskEdit(TFarEdit * Edit)
{
  Expects(Edit);
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}
