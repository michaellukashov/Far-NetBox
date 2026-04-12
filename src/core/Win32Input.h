#pragma once

#include <Common.h>
#include <windows.h>

// Win32 Input Mode encoder/decoder for CSI _ VT sequences.
// Format: CSI Vk ; Sc ; Uc ; Kd ; Cs ; Rc _
// Where:
//   Vk = wVirtualKeyCode
//   Sc = wVirtualScanCode
//   Uc = UnicodeChar (decimal)
//   Kd = bKeyDown (0 or 1)
//   Cs = dwControlKeyState
//   Rc = wRepeatCount
//
// Reference: WindowsTerminal src/terminal/input/terminalInput.cpp

class NB_CORE_EXPORT TWin32Input
{
public:
  // Encode INPUT_RECORD to CSI _ sequence
  static UnicodeString Encode(const INPUT_RECORD & Record);

  // Decode CSI _ sequence to INPUT_RECORD
  static bool Decode(const UnicodeString & Sequence, INPUT_RECORD & Record);

  // Check if data starts with Win32 input sequence (ESC [ ... _)
  static bool IsWin32Sequence(const char * Data, size_t Length);
  static bool IsWin32SequenceW(const wchar_t * Data, size_t Length);

private:
  // CSI = ESC [ = \x1b[
  static constexpr const char * CSIPrefix = "\x1b[";
  static constexpr wchar_t CSIPrefixW[] = L"\x1b[";
  static constexpr char Win32Final = '_';
};
