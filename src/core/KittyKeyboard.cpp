#include <vcl.h>
#pragma hdrstop

#include "KittyKeyboard.h"
#include "nbcore.h"

void TKittyKeyboard::SetFlags(uint8_t Flags, TKittyKeyboardProtocolMode Mode) noexcept
{
  Flags &= TKittyKeyboardProtocolFlags::All;

  switch (Mode)
  {
    case TKittyKeyboardProtocolMode::Replace:
      FFlags = Flags;
      break;
    case TKittyKeyboardProtocolMode::Set:
      FFlags |= Flags;
      break;
    case TKittyKeyboardProtocolMode::Reset:
      FFlags &= ~Flags;
      break;
  }
}

void TKittyKeyboard::PushFlags(uint8_t Flags)
{
  Flags &= TKittyKeyboardProtocolFlags::All;

  auto & Stack = FMainStack; // Simplified: no alternate buffer tracking for now
  // KKP: If a push request is received and the stack is full,
  //      the oldest entry from the stack must be evicted.
  if (Stack.size() >= StackMaxSize)
  {
    Stack.erase(Stack.begin());
  }
  Stack.push_back(FFlags);
  FFlags = Flags;
}

void TKittyKeyboard::PopFlags(size_t Count)
{
  if (Count == 0)
  {
    return;
  }

  auto & Stack = FMainStack;

  if (Count >= Stack.size())
  {
    // KKP: If a pop request is received that empties the stack, all flags are reset.
    FFlags = 0;
    Stack.clear();
  }
  else
  {
    FFlags = Stack.at(Stack.size() - Count);
    Stack.erase(Stack.end() - Count, Stack.end());
  }
}

void TKittyKeyboard::UseAlternateBuffer() noexcept
{
  // When entering alternate screen buffer, reset flags
  FFlags = 0;
}

void TKittyKeyboard::UseMainBuffer() noexcept
{
  // When returning to main screen buffer, restore from stack
  if (!FMainStack.empty())
  {
    FFlags = FMainStack.back();
    FMainStack.pop_back();
  }
  else
  {
    FFlags = 0;
  }
}
