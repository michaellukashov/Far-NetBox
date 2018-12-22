
#include <vcl.h>
#pragma hdrstop

#include <Sysutils.hpp>

#include "Script.h"
void __fastcall TScript::NoMatch(const UnicodeString & Mask, const UnicodeString & Error)
{
  UnicodeString Message = FMTLOAD(SCRIPT_MATCH_NO_MATCH, (Mask));
  if (!Error.IsEmpty())
  {
    Message += FORMAT(L" (%s)", (Error));
  }

  NoMatch(Message);
}
//---------------------------------------------------------------------------
      TDifferenceSessionAction Action(FTerminal->ActionLog, Item);


const wchar_t * ToggleNames[] = { L"off", L"on" };
