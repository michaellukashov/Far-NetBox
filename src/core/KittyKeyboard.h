#pragma once

#include <Common.h>

// KiTTY Keyboard Protocol progressive enhancement flags
// https://sw.kovidgoyal.net/kitty/keyboard-protocol/
struct TKittyKeyboardProtocolFlags
{
  static constexpr uint8_t None = 0;
  static constexpr uint8_t DisambiguateEscapeCodes = 1 << 0;   // 0x01
  static constexpr uint8_t ReportEventTypes = 1 << 1;          // 0x02
  static constexpr uint8_t ReportAlternateKeys = 1 << 2;       // 0x04
  static constexpr uint8_t ReportAllKeysAsEscapeCodes = 1 << 3; // 0x08
  static constexpr uint8_t ReportAssociatedText = 1 << 4;      // 0x10
  static constexpr uint8_t All = (1 << 5) - 1;                 // 0x1F
};

enum class TKittyKeyboardProtocolMode : uint8_t
{
  Replace = 1,
  Set = 2,
  Reset = 3,
};

class NB_CORE_EXPORT TKittyKeyboard
{
  NB_DISABLE_COPY(TKittyKeyboard)
public:
  TKittyKeyboard() = default;

  void SetFlags(uint8_t Flags, TKittyKeyboardProtocolMode Mode) noexcept;
  uint8_t GetFlags() const noexcept { return FFlags; }
  void PushFlags(uint8_t Flags);
  void PopFlags(size_t Count);
  void Reset() noexcept { FFlags = 0; FMainStack.clear(); FAltStack.clear(); }
  void UseAlternateBuffer() noexcept;
  void UseMainBuffer() noexcept;
  bool IsActive() const noexcept { return FFlags != 0; }

private:
  static constexpr size_t StackMaxSize = 8;
  uint8_t FFlags{0};
  nb::vector_t<uint8_t> FMainStack;
  nb::vector_t<uint8_t> FAltStack;
};
