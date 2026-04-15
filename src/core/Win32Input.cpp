#include <vcl.h>
#pragma hdrstop

#include "Win32Input.h"
#include "nbcore.h"

UnicodeString TWin32Input::Encode(const INPUT_RECORD & Record)
{
  if (Record.EventType != KEY_EVENT)
  {
    return UnicodeString();
  }

  const KEY_EVENT_RECORD & Key = Record.Event.KeyEvent;

  // CSI Vk ; Sc ; Uc ; Kd ; Cs ; Rc _
  const uint16_t Vk = Key.wVirtualKeyCode;
  const uint16_t Sc = Key.wVirtualScanCode;
  const uint16_t Uc = static_cast<uint16_t>(Key.uChar.UnicodeChar);
  const uint16_t Kd = Key.bKeyDown ? 1 : 0;
  const uint16_t Cs = static_cast<uint16_t>(Key.dwControlKeyState);
  const uint16_t Rc = Key.wRepeatCount;

  UnicodeString Result;
  Result = FORMAT(L"\x1b[%hu;%hu;%hu;%hu;%hu;%hu_", Vk, Sc, Uc, Kd, Cs, Rc);

  return Result;
}

bool TWin32Input::Decode(const UnicodeString & Sequence, INPUT_RECORD & Record)
{
  // Expected format: CSI Vk ; Sc ; Uc ; Kd ; Cs ; Rc _
  // CSI = \x1b[
  // Minimum length: \x1b[1_ = 5 chars
  if (Sequence.Length() < 5)
  {
    return false;
  }

  // Verify CSI prefix
  if (Sequence[1] != L'\x1b' || Sequence[2] != L'[')
  {
    return false;
  }

  // Verify final character
  if (Sequence[Sequence.Length()] != L'_')
  {
    return false;
  }

  // Extract parameters between '[' and '_'
  UnicodeString Params = Sequence.SubString(3, Sequence.Length() - 3);

  // Parse Vk ; Sc ; Uc ; Kd ; Cs ; Rc
  nb::vector_t<int32_t> SemicolonPositions;
  for (int32_t i = 1; i <= Params.Length() && SemicolonPositions.size() < 5; i++)
  {
    if (Params[i] == L';')
    {
      SemicolonPositions.push_back(i);
    }
  }

  // We need exactly 5 semicolons for 6 parameters
  if (SemicolonPositions.size() < 5)
  {
    return false;
  }

  try
  {
    auto SubStrInt = [&](int32_t Start, int32_t Len) -> int32_t
    {
      return StrToIntDef(Params.SubString(Start, Len), 0);
    };

    uint16_t Vk = static_cast<uint16_t>(SubStrInt(1, SemicolonPositions[0] - 1));
    uint16_t Sc = static_cast<uint16_t>(SubStrInt(SemicolonPositions[0] + 1, SemicolonPositions[1] - SemicolonPositions[0] - 1));
    uint16_t Uc = static_cast<uint16_t>(SubStrInt(SemicolonPositions[1] + 1, SemicolonPositions[2] - SemicolonPositions[1] - 1));
    uint16_t Kd = static_cast<uint16_t>(SubStrInt(SemicolonPositions[2] + 1, SemicolonPositions[3] - SemicolonPositions[2] - 1));
    uint16_t Cs = static_cast<uint16_t>(SubStrInt(SemicolonPositions[3] + 1, SemicolonPositions[4] - SemicolonPositions[3] - 1));
    uint16_t Rc = static_cast<uint16_t>(SubStrInt(SemicolonPositions[4] + 1, Params.Length() - SemicolonPositions[4]));

    ZeroMemory(&Record, sizeof(Record));
    Record.EventType = KEY_EVENT;
    Record.Event.KeyEvent.bKeyDown = (Kd != 0);
    Record.Event.KeyEvent.wRepeatCount = Rc;
    Record.Event.KeyEvent.wVirtualKeyCode = Vk;
    Record.Event.KeyEvent.wVirtualScanCode = Sc;
    Record.Event.KeyEvent.uChar.UnicodeChar = static_cast<wchar_t>(Uc);
    Record.Event.KeyEvent.dwControlKeyState = Cs;

    return true;
  }
  catch (Exception &)
  {
    return false;
  }
}

bool TWin32Input::IsWin32Sequence(const char * Data, size_t Length)
{
  if (Length < 3)
  {
    return false;
  }
  return (Data[0] == '\x1b' && Data[1] == '[' && Data[Length - 1] == '_');
}

bool TWin32Input::IsWin32SequenceW(const wchar_t * Data, size_t Length)
{
  if (Length < 3)
  {
    return false;
  }
  return (Data[0] == L'\x1b' && Data[1] == L'[' && Data[Length - 1] == L'_');
}
