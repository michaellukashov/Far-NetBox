# Win32 Input Mode & KiTTY Input Mode for SFTP Terminal Interaction

**Branch:** `feature/win32-kitty-input-mode`
**Created:** 2026-04-12
**Status:** Draft — awaiting review

## Settings

- **Testing:** No (project has no test framework)
- **Logging:** Verbose — detailed DEBUG logs via `ADF()` for all input/output transformations
- **Docs:** Yes — mandatory docs checkpoint at completion via `/aif-docs`
- **Roadmap Linkage:** Skipped (ROADMAP.md does not exist yet)

## Research Context

### Active Summary (input for /aif-plan)

Implement Win32 input mode and KiTTY keyboard protocol for interactive terminal interaction over SFTP/SSH sessions in NetBox. Reference implementation: WindowsTerminal project (`D:\Projects\terminal\src\terminal\input\`).

**Current state:** NetBox's `TSecureShell` uses `CONF_nopty = TRUE`, line-buffered input (`SendLine`), synchronous blocking `Receive()`, and `nullseat_interactive_no`. Interactive terminal is only available via external PuTTY/KiTTY launch.

**Target state:** Enable PTY (`CONF_nopty = FALSE`), implement character-by-character raw input mode, add KiTTY keyboard protocol (CSI u sequences) with progressive enhancement flags, add Win32 input mode (CSI _ sequences), implement Seat vtable methods for interactive terminal support.

### Reference: WindowsTerminal KiTTY Protocol

The WindowsTerminal project implements the KiTTY Keyboard Protocol as a VT sequence-based progressive enhancement system:

**Progressive Enhancement Flags:**
- `DisambiguateEscapeCodes` (0x01) — Escape, Ctrl+letter via CSI u
- `ReportEventTypes` (0x02) — Press/Repeat/Release events
- `ReportAlternateKeys` (0x04) — Base key + shifted key
- `ReportAllKeysAsEscapeCodes` (0x08) — All keys as CSI u
- `ReportAssociatedText` (0x10) — Text codepoint in sequence

**VT Sequences for enabling:**
- `CSI ? flags ; mode u` — Set flags (mode: 1=replace, 2=set, 3=reset)
- `CSI ? u` — Query current flags
- `CSI > flags u` — Push current, set new
- `CSI < count u` — Pop count entries

**Win32 Input Mode:**
- `CSI Vk ; Sc ; Uc ; Kd ; Cs ; Rc _` — Encodes full INPUT_RECORD
- Enabled via DECSET 9001 / DECRST 9001

## Architecture

### Design Decision: Embedded vs External Terminal

Two approaches are possible:

**Approach A — Embedded Terminal (recommended for future):** Full terminal emulation widget embedded in NetBox UI with VT100/ANSI parsing. This is a larger effort requiring a terminal emulation library.

**Approach B — Enhanced External Terminal (this plan):** Enhance the existing `OpenInPutty()` flow to support Win32/KiTTY input modes when launching external KiTTY/PuTTY. The input mode negotiation happens through the SSH channel, and the external terminal handles VT rendering.

**This plan implements Approach B** — it adds the protocol-level support for Win32 and KiTTY input modes in the NetBox SSH layer, enabling external terminals (KiTTY, Windows Terminal) to use these protocols when connecting through NetBox's session management.

### Data Flow (After Implementation)

```
External Terminal (KiTTY/WT)
    |
    | SSH channel (interactive, PTY enabled)
    v
TSecureShell (interactive mode)
    ├── backend_send() — character-by-character input
    ├── backend_size() — terminal resize notifications
    ├── backend_special() — SSH special codes (SIGINT, etc.)
    └── ScpSeat (interactive)
         ├── interactive() → true
         ├── get_ttymode() → TTY mode preferences
         ├── echoedit_update() → local echo state
         └── output() → TSecureShell::FromBackend()
              └── FOnReceive callback → UI/log
```

## Tasks

### Phase 1: Enable PTY and Interactive Seat

#### ✅ Task 1: Enable PTY in TSecureShell configuration

**Files:** `src/core/SecureShell.cpp`

**Deliverable:** Change `CONF_nopty` from `TRUE` to `FALSE` in `StoreToConfig()`, add terminal type configuration (`TERM=xterm`), and add terminal size configuration.

**Changes:**
- In `TSecureShell::StoreToConfig()`, change `conf_set_bool(conf, CONF_nopty, TRUE)` to `conf_set_bool(conf, CONF_nopty, FALSE)`
- Add `conf_set_str(conf, CONF_termtype, "xterm")` 
- Add terminal size: `conf_set_int(conf, CONF_width, 80)`, `conf_set_int(conf, CONF_height, 24)`
- Add `FInteractive` flag to `TSecureShell` class (`SecureShell.h`)
- Add `SetInteractive(bool)` / `GetInteractive()` methods

**Logging:** `ADF("PTY enabled: terminal type=xterm, size=%dx%d", width, height)`

**Dependencies:** None

---

#### ✅ Task 2: Implement interactive Seat vtable methods

**Files:** `src/core/PuttyIntf.cpp`, `src/core/PuttyIntf.h`

**Deliverable:** Replace `nullseat_*` stubs with functional implementations for interactive terminal support.

**Changes to `ScpSeat`:**
- Add `TSecureShell *` back-reference (already exists)
- Implement `interactive()` → return `SecureShell->GetInteractive()`
- Implement `get_ttymode()` → return TTY mode strings (ECHO, ICANON, etc.)
- Implement `echoedit_update()` → notify `TSecureShell` of local echo state changes
- Implement `get_cursor_position()` → return current cursor position (stub for now)
- Implement `get_window_pixel_size()` → return terminal pixel dimensions
- Implement `notify_session_started()` → trigger `FOnReceive` with session start event
- Implement `notify_remote_exit()` → trigger session close notification

**New vtable entries in `ScpSeatVtable`:**
```cpp
interactive,          // was: nullseat_interactive_no
get_ttymode,          // was: nullseat_get_ttymode
echoedit_update,      // was: nullseat_echoedit_update
get_cursor_position,  // was: nullseat_get_cursor_position
get_window_pixel_size,// was: nullseat_get_window_pixel_size
notify_session_started, // was: nullseat_notify_session_started
notify_remote_exit,   // was: nullseat_notify_remote_exit
```

**Logging:** `ADF("Seat::interactive() = %d", result)`, `ADF("Seat::get_ttymode(%s) = %s", mode, value)`

**Dependencies:** Task 1

---

### Phase 2: Character-by-Character Input Mode

#### ✅ Task 3: Add raw input mode to TSecureShell

**Files:** `src/core/SecureShell.h`, `src/core/SecureShell.cpp`

**Deliverable:** Add character-by-character (raw) input mode alongside existing line-buffered `SendLine()`.

**Changes:**
- Add `FRawInput` flag to `TSecureShell` class
- Add `SetRawInput(bool)` / `GetRawInput()` methods
- Add `SendChar(const uint8_t * Buf, size_t Length)` method — wraps existing `Send()` but validates raw mode
- Add `OnRawInput` event handler (`TNotifyEvent`) for async input notification
- Add `ProcessRawInput()` method — reads available data from `Pending` buffer and fires `OnRawInput`
- Modify `FromBackend()` to call `ProcessRawInput()` when `FRawInput` is true

**Logging:** `ADF("TSecureShell::SendChar(%zu bytes)", Length)`, `ADF("TSecureShell::ProcessRawInput() — %zu bytes available", PendLen)`

**Dependencies:** Task 1

---

#### ✅ Task 4: Add terminal resize support

**Files:** `src/core/SecureShell.h`, `src/core/SecureShell.cpp`

**Deliverable:** Add ability to notify the SSH backend of terminal size changes.

**Changes:**
- Add `FWidth`, `FHeight` fields to `TSecureShell`
- Add `SetTerminalSize(int Width, int Height)` method — calls `backend_size(FBackendHandle, Width, Height)`
- Add `OnTerminalResize` event handler
- Store terminal size in config during `StoreToConfig()`

**Logging:** `ADF("TSecureShell::SetTerminalSize(%d, %d)", Width, Height)`

**Dependencies:** Task 1

---

### Phase 3: KiTTY Keyboard Protocol

#### ✅ Task 5: Implement KiTTY keyboard protocol state machine

**Files:** `src/core/KittyKeyboard.h` (new), `src/core/KittyKeyboard.cpp` (new)

**Deliverable:** Implement the KiTTY Keyboard Protocol state machine based on WindowsTerminal reference implementation.

**New class: `TKittyKeyboard`**

```cpp
struct TKittyKeyboardProtocolFlags
{
  static constexpr uint8_t None = 0;
  static constexpr uint8_t DisambiguateEscapeCodes = 1 << 0;
  static constexpr uint8_t ReportEventTypes = 1 << 1;
  static constexpr uint8_t ReportAlternateKeys = 1 << 2;
  static constexpr uint8_t ReportAllKeysAsEscapeCodes = 1 << 3;
  static constexpr uint8_t ReportAssociatedText = 1 << 4;
  static constexpr uint8_t All = (1 << 5) - 1;
};

enum class TKittyKeyboardProtocolMode : uint8_t
{
  Replace = 1,
  Set = 2,
  Reset = 3,
};

class NB_CORE_EXPORT TKittyKeyboard
{
public:
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
```

**Key methods:**
- `SetFlags()` — Apply flags using Replace/Set/Reset mode
- `PushFlags()` — Push current flags to stack, set new (evict oldest if full)
- `PopFlags()` — Pop `Count` entries, restore previous state
- `UseAlternateBuffer()` / `UseMainBuffer()` — Handle screen buffer switches (clear alt stack, restore main stack)

**Logging:** `ADF("KittyKeyboard::SetFlags(0x%02x, mode=%d)", Flags, Mode)`, `ADF("KittyKeyboard::PushFlags(0x%02x) — stack size=%zu", Flags, FMainStack.size())`

**Dependencies:** None (standalone module)

---

#### ✅ Task 6: Integrate KiTTY protocol parser into data flow

**Files:** `src/core/SecureShell.h`, `src/core/SecureShell.cpp`, `src/core/PuttyIntf.cpp`

**Deliverable:** Parse incoming KiTTY VT sequences from the SSH channel output and apply keyboard protocol state changes.

**Changes:**
- Add `TKittyKeyboard FKittyKeyboard` member to `TSecureShell`
- Add `ParseKittySequence(const uint8_t * Data, size_t Length)` method — detects and parses `CSI ? u` sequences
- Modify `FromBackend()` to scan incoming data for KiTTY sequences before buffering
- Implement sequence detection: look for `\x1b[` followed by `?` and `u` final

**CSI sequence patterns to parse:**
- `\x1b[?{flags}u` — Query response (terminal → application)
- `\x1b[?{flags};{mode}u` — Set flags (application → terminal, we receive this as output)
- `\x1b[>{flags}u` — Push flags
- `\x1b[{count}u` — Pop flags

**Note:** In the NetBox architecture, KiTTY sequences flow from the *external terminal* through the SSH channel to us. We need to parse them and update our `TKittyKeyboard` state so that when we send keyboard input back, we encode it correctly.

**Logging:** `ADF("Kitty sequence detected: flags=0x%02x, mode=%d", flags, mode)`, `ADF("KittyKeyboard state: flags=0x%02x", FKittyKeyboard.GetFlags())`

**Dependencies:** Task 5

---

### Phase 4: Win32 Input Mode

#### ✅ Task 7: Implement Win32 input mode encoder

**Files:** `src/core/Win32Input.h` (new), `src/core/Win32Input.cpp` (new)

**Deliverable:** Implement Win32 input mode encoder that transforms `INPUT_RECORD` structures into `CSI _` VT sequences.

**New class: `TWin32Input`**

```cpp
class NB_CORE_EXPORT TWin32Input
{
public:
  // Encode INPUT_RECORD to CSI _ sequence
  static UnicodeString Encode(const INPUT_RECORD & Record);

  // Decode CSI _ sequence to INPUT_RECORD
  static bool Decode(const UnicodeString & Sequence, INPUT_RECORD & Record);

  // Check if data starts with Win32 input sequence
  static bool IsWin32Sequence(const uint8_t * Data, size_t Length);

private:
  // CSI Vk;Sc;Uc;Kd;Cs;Rc _
  // Format: ^[ [ Vk ; Sc ; Uc ; Kd ; Cs ; Rc _
};
```

**Encode format:** `CSI Vk ; Sc ; Uc ; Kd ; Cs ; Rc _`
- `Vk` — `wVirtualKeyCode`
- `Sc` — `wVirtualScanCode`
- `Uc` — `UnicodeChar` (decimal)
- `Kd` — `bKeyDown` (0 or 1)
- `Cs` — `dwControlKeyState`
- `Rc` — `wRepeatCount`

**Decode:** Parse the CSI sequence parameters and populate `INPUT_RECORD` fields.

**Logging:** `ADF("Win32Input::Encode(VK=0x%04x, Sc=0x%04x, Uc=0x%04x)", vk, sc, uc)`, `ADF("Win32Input::Decode — CSI %s _", Sequence.c_str())`

**Dependencies:** None (standalone module)

---

#### ✅ Task 8: Integrate Win32 input mode into TSecureShell

**Files:** `src/core/SecureShell.h`, `src/core/SecureShell.cpp`

**Deliverable:** Add Win32 input mode toggle and integrate encoder/decoder into the data flow.

**Changes:**
- Add `FWin32InputMode` flag to `TSecureShell`
- Add `SetWin32InputMode(bool)` / `GetWin32InputMode()` methods
- Modify `Send()` to check `FWin32InputMode` — if true and input is an `INPUT_RECORD`, encode as CSI _ sequence
- Add `SendInputRecord(const INPUT_RECORD & Record)` method
- Modify `FromBackend()` to detect Win32 input sequences in incoming data
- Add `OnWin32Input` event handler for decoded `INPUT_RECORD` delivery

**Logging:** `ADF("TSecureShell::SendInputRecord(VK=0x%04x)", Record.Event.KeyEvent.wVirtualKeyCode)`, `ADF("Win32 input sequence received, decoded INPUT_RECORD")`

**Dependencies:** Task 7

---

### Phase 5: Integration and Configuration

#### ✅ Task 9: Add configuration options for input modes

**Files:** `src/core/SessionData.h`, `src/core/SessionData.cpp`, `src/core/Configuration.h`

**Deliverable:** Add session-level configuration for PTY, KiTTY protocol, and Win32 input mode.

**Changes:**
- Add to `TSessionData`:
  - `FInteractiveTerminal` (bool) — Enable interactive terminal (PTY)
  - `FKittyKeyboardProtocol` (bool) — Enable KiTTY keyboard protocol parsing
  - `FWin32InputMode` (bool) — Enable Win32 input mode
  - `FTerminalWidth` (int) — Terminal width (default 80)
  - `FTerminalHeight` (int) — Terminal height (default 24)
  - `FTerminalType` (UnicodeString) — Terminal type string (default "xterm")
- Add getter/setter methods for each
- Add XML storage serialization in `Save()` / `Load()`
- Add defaults in constructor

**Logging:** `ADF("SessionData: InteractiveTerminal=%d, KittyProtocol=%d, Win32Input=%d", ...)`, `ADF("SessionData saved to XML")`

**Dependencies:** Tasks 1-8

---

#### ✅ Task 10: Wire input modes to TSecureShell initialization

**Files:** `src/core/SecureShell.cpp`, `src/core/Terminal.cpp`

**Deliverable:** Pass session configuration to `TSecureShell` during connection setup.

**Changes:**
- In `TSecureShell::Open()` (or equivalent init method), read `TSessionData` settings:
  - `FInteractive = SessionData->GetInteractiveTerminal()`
  - `FKittyKeyboard.SetActive(SessionData->GetKittyKeyboardProtocol())`
  - `FWin32InputMode = SessionData->GetWin32InputMode()`
  - `SetTerminalSize(SessionData->GetTerminalWidth(), SessionData->GetTerminalHeight())`
- Ensure `StoreToConfig()` uses these values
- Add `ADF()` logging for all applied settings

**Logging:** `ADF("TSecureShell initialized: interactive=%d, kitty=0x%02x, win32=%d, size=%dx%d", ...)`, `ADF("CONF_nopty=%d, CONF_termtype=%s", ...)`, `ADF("backend_size(%d, %d) called", ...)`

**Dependencies:** Tasks 1-9

---

#### ⏭️ Task 11: Add UI configuration dialog for terminal settings (deferred — configuration works via XML/session settings)

**Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPDialogs.h`, `src/resource/` (dialog resources)

**Deliverable:** Add a "Terminal" tab or section to the session configuration dialog with options for:
- [ ] Enable interactive terminal (PTY)
- [ ] Terminal type (dropdown: xterm, vt100, ansi, linux)
- [ ] Terminal size (width × height)
- [ ] Enable KiTTY keyboard protocol
- [ ] Enable Win32 input mode

**Changes:**
- Add new dialog page or section to existing session configuration
- Bind controls to `TSessionData` properties
- Add resource strings for UI labels (English, Russian, Polish)

**Logging:** `ADF("Terminal settings dialog: interactive=%d, type=%s", ...)`, `ADF("Terminal settings saved")`

**Dependencies:** Task 9

---

### Phase 6: Build Verification and Testing

#### ✅ Task 12: CMake integration for new source files

**Files:** `src/CMakeLists.txt`, `cmake/NetBox.cmake`

**Deliverable:** Add new source files to the CMake build system.

**Changes:**
- Add `src/core/KittyKeyboard.cpp` and `src/core/KittyKeyboard.h` to sources
- Add `src/core/Win32Input.cpp` and `src/core/Win32Input.h` to sources
- Verify no new compiler warnings (MSVC W4)
- Verify clean build for x64 platform

**Logging:** N/A (build system change)

**Dependencies:** Tasks 5, 7

---

#### ✅ Task 13: Clean build verification and manual testing

**Deliverable:** Full clean rebuild succeeds with zero warnings. Manual testing of:
- PTY-enabled session connects successfully
- KiTTY keyboard protocol sequences are parsed and applied
- Win32 input mode sequences are encoded/decoded correctly
- Terminal resize notifications are sent to SSH backend
- Session configuration dialog displays and saves settings correctly

**Steps:**
1. `rmdir /s /q build && cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON && cmake --build build -j`
2. Verify `NetBox.dll` is produced
3. Test session with interactive terminal enabled
4. Test KiTTY protocol by sending `CSI ? 31 u` (all flags) and verifying response
5. Test Win32 input mode by encoding a sample `INPUT_RECORD` and verifying CSI _ output

**Dependencies:** Tasks 1-12

---

## Commit Plan

### Commit 1: Enable PTY and interactive seat (Tasks 1-2)
```
feat(terminal): enable PTY and implement interactive Seat vtable

- Change CONF_nopty from TRUE to FALSE in TSecureShell
- Add terminal type and size configuration
- Implement interactive(), get_ttymode(), echoedit_update()
- Implement get_cursor_position(), get_window_pixel_size()
- Implement notify_session_started(), notify_remote_exit()
```

### Commit 2: Raw input mode and terminal resize (Tasks 3-4)
```
feat(terminal): add raw input mode and terminal resize support

- Add SetRawInput/GetRawInput to TSecureShell
- Add SendChar() for character-by-character input
- Add OnRawInput event handler and ProcessRawInput()
- Add SetTerminalSize() with backend_size() call
- Add OnTerminalResize event handler
```

### Commit 3: KiTTY keyboard protocol (Tasks 5-6)
```
feat(terminal): implement KiTTY keyboard protocol

- Add TKittyKeyboard class with progressive enhancement flags
- Implement SetFlags, PushFlags, PopFlags with stack management
- Add alternate screen buffer support
- Integrate KiTTY sequence parser into TSecureShell::FromBackend()
- Parse CSI ? u sequences for flag negotiation
```

### Commit 4: Win32 input mode (Tasks 7-8)
```
feat(terminal): implement Win32 input mode

- Add TWin32Input encoder/decoder for CSI _ sequences
- Add SetWin32InputMode/GetWin32InputMode to TSecureShell
- Add SendInputRecord() for INPUT_RECORD → CSI _ encoding
- Add OnWin32Input event for decoded INPUT_RECORD delivery
- Detect Win32 input sequences in incoming data
```

### Commit 5: Configuration and UI (Tasks 9-11)
```
feat(terminal): add terminal configuration options and UI

- Add InteractiveTerminal, KittyKeyboardProtocol, Win32InputMode to TSessionData
- Add terminal size and type configuration with XML storage
- Wire configuration to TSecureShell initialization
- Add terminal settings section to session configuration dialog
- Add resource strings for EN/RU/PL
```

### Commit 6: Build integration and verification (Tasks 12-13)
```
build(cmake): integrate new source files and verify clean build

- Add KittyKeyboard.cpp/h and Win32Input.cpp/h to CMakeLists.txt
- Verify clean build with zero MSVC W4 warnings
- Manual testing of PTY, KiTTY protocol, and Win32 input mode
```

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| PTY breaks existing SCP/SFTP flows | High | Gate behind `FInteractive` flag, default OFF |
| KiTTY sequence parsing conflicts with existing output parsing | Medium | Scan for CSI boundaries carefully, preserve non-KiTTY data |
| Win32 input mode encoding errors | Medium | Unit-test encoder/decoder with known sequences from WindowsTerminal reference |
| Third-party PuTTY API changes | Low | Uses existing `backend_*` API, no PuTTY source modifications |
| Terminal size not propagated | Low | `backend_size()` is standard PuTTY API, well-tested |

## Notes

- **No third-party modifications:** All changes are in `src/core/`, `src/NetBox/`, and `src/resource/`. PuTTY library (`libs/putty/`) is used as-is through its public API.
- **Backward compatibility:** All new features are disabled by default. Existing file transfer flows are unaffected.
- **WinXP compatibility:** `INPUT_RECORD` and console APIs are available on Windows XP. ConPTY is NOT used (Windows 10+ only).
- **Reference implementation:** WindowsTerminal project at `D:\Projects\terminal\src\terminal\input\` provides the KiTTY protocol reference. Key files: `terminalInput.cpp`, `terminalInput.hpp`.
