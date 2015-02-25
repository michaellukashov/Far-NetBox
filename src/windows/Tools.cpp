
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <TextsCore.h>
#include <Exceptions.h>
#include <FileMasks.h>
#include <CoreMain.h>
#include <RemoteFiles.h>
#include <Interface.h>

#include "GUITools.h"
#include "Tools.h"

void * BusyStart()
{
  void * Token = nullptr; // ToPtr(Screen->Cursor);
  // Screen->Cursor = crHourGlass;
  return Token;
}

void BusyEnd(void * /*Token*/)
{
  // Screen->Cursor = reinterpret_cast<TCursor>(Token);
}

static DWORD MainThread = 0;
static TDateTime LastGUIUpdate(0.0);
static double GUIUpdateIntervalFrac = static_cast<double>(MSecsPerSec / 1000 * GUIUpdateInterval);  // 1/5 sec

bool ProcessGUI(bool Force)
{
  assert(MainThread != 0);
  bool Result = false;
  if (MainThread == ::GetCurrentThreadId())
  {
    TDateTime N = Now();
    if (Force ||
        (double(N) - double(LastGUIUpdate) > GUIUpdateIntervalFrac))
    {
      LastGUIUpdate = N;
//      Application->ProcessMessages();
      Result = true;
    }
  }
  return Result;
}

TOptions * GetGlobalOptions()
{
  return nullptr; // TProgramParams::Instance();
}

void WinInitialize()
{
//  if (JclHookExceptions())
//  {
//    JclStackTrackingOptions << stAllModules;
//    JclAddExceptNotifier(DoExceptNotify, npFirstChain);
//  }

  SetErrorMode(SEM_FAILCRITICALERRORS);
//  OnApiPath = ::ApiPath;
  MainThread = GetCurrentThreadId();

}
//---------------------------------------------------------------------------
void WinFinalize()
{
  // JclRemoveExceptNotifier(DoExceptNotify);
}
