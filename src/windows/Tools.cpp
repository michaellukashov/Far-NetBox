//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <TextsCore.h>
#include <Exceptions.h>
#include <FileMasks.h>
#include <CoreMain.h>
#include <RemoteFiles.h>
//#include <PuttyTools.h>

#include "GUITools.h"
#include "Tools.h"
//---------------------------------------------------------------------------
void * BusyStart()
{
  void * Token = nullptr; // ToPtr(Screen->Cursor);
  // Screen->Cursor = crHourGlass;
  return Token;
}
//---------------------------------------------------------------------------
void BusyEnd(void * /* Token */)
{
  // Screen->Cursor = reinterpret_cast<TCursor>(Token);
}
