# Implementation Plan: Esc-based Session Initialization Interrupt

Branch: main
Created: 2026-05-10
Refined: 2026-05-10 (aif-improve: corrected virtual dispatch analysis)

## Settings
- Testing: yes (manual regression for all 4 protocols)
- Logging: verbose (DEBUG-level trace for Esc detection, cancel propagation, and cleanup)
- Docs: yes (mandatory docs checkpoint at completion)

## Roadmap Linkage
Milestone: "none"
Rationale: Bug fix / UX improvement, not a milestone-scoped feature.

## Research Context

Source: User-provided analysis of far2l's Editor Esc interrupt mechanism + aif-improve codebase deep-trace

Goal:
Make remote session initialization interruptible by pressing Esc, with progress indication —
matching the pattern used by far2l's Editor `LoadFile()`.

Constraints:
- No modifications to `libs/` — use patches only
- Far Manager API calls on main thread only
- MSVC /W4 zero warnings
- WinXP compatibility (`_WIN32_WINNT=0x0501`)
- Incremental evolution — no architectural rewrites
- C++17 standard only (no `std::filesystem`, `std::variant`)

### Architecture Discovery (aif-improve correction)

The original plan assumed `TSecureShell` needed new `FTerminal->CheckForEsc()` calls.
Deep-trace revealed this is already handled by virtual dispatch:

```
TSessionUI::ProcessGUI() = 0           <- abstract interface (SessionInfo.h:91)
    |
TTerminal::ProcessGUI() override       <- checks Esc when FStatus == ssOpening (Terminal.cpp:1996)
    |
TTunnelUI::ProcessGUI() override       <- NOOP — the bug! (Terminal.cpp:362)
```

`TSecureShell` is constructed with `TSessionUI*`:
- Direct SSH: `TSecureShell(this, ...)` -> `FUI = TTerminal*` -> `ProcessGUI()` checks Esc v
- Tunnel:    `TSecureShell(FTunnelUI, ...)` -> `FUI = TTunnelUI*` -> `ProcessGUI()` is NOOP x

When `EventSelectLoop()` (SecureShell.cpp:2506) calls `FUI->ProcessGUI()`, the virtual
dispatch already routes to the correct implementation. The only break is `TTunnelUI`.

S3 and WebDAV do NOT use `TSecureShell` at all — they use synchronous neon/libS3 C calls
with no event loop, no `ProcessGUI()`, and zero `CheckForEsc()` calls.

### Corrected State Table

| Protocol | ProcessGUI path | Esc checked? |
|----------|----------------|-------------|
| SSH/SFTP/SCP (direct) | `EventSelectLoop` -> `FUI=TTerminal*` | v Already works |
| SSH Tunnel | `EventSelectLoop` -> `FUI=TTunnelUI*` -> noop | x Root cause: TTunnelUI::ProcessGUI() |
| FTP | `WaitForMessages()` -> `FTerminal->ProcessGUI()` | v Already works |
| S3 | `S3_list_service/bucket` (sync C, no loop) | x No ProcessGUI path |
| WebDAV | `ne_options2()` (sync neon, no loop) | x No ProcessGUI path |

### Decisions
- Tunnel: Fix `TTunnelUI::ProcessGUI()` noop -> delegate to `FTerminal->ProcessGUI()` (1 line)
- S3/WebDAV: Add explicit `FTerminal->CheckForEsc()` before each blocking call; cannot
  interrupt during the synchronous C library call itself (known limitation)
- FTP: Add early-exit `CheckForEsc()` at `Open()` entry (existing WaitForMessages path already checks Esc)
- Do NOT add any Esc checks to `TSecureShell::Init()`/`EventSelectLoop()` — already handled

Open questions:
- None

## Anti-Patterns (from esc-cancellation-comprehensive-fix.md)
- NEVER use `PeekConsoleInput(&Rec, 1, ...)` to check for Esc — use full-buffer scan
- NEVER throw `EAbort(EXCEPTION_MSG_REPLACED)` for user cancel — use `EAbort("")`
- NEVER call `ReadCommandOutput(coWaitForLastLine)` during user cancel — skip it
- NEVER check only `csCancelTransfer` in SCP loops — also check `csCancel`

## Commit Plan
- **Commit 1** (after task 1): "fix: make TTunnelUI::ProcessGUI delegate to FTerminal for Esc during tunnel connect"
- **Commit 2** (after tasks 2-3): "feat: add Esc interrupt during S3 and WebDAV session initialization"
- **Commit 3** (after tasks 4-5): "feat: add explicit Esc check for FTP session initialization"
- **Commit 4** (after task 6): "docs: add Esc session interrupt documentation"

## Tasks

### Phase 1: Tunnel — Fix `TTunnelUI::ProcessGUI()` noop

  - [x] Task 1: Make `TTunnelUI::ProcessGUI()` delegate to `FTerminal->ProcessGUI()`

  **Target:** `src/core/Terminal.cpp` — `TTunnelUI::ProcessGUI()` (line 362-365)

  **Change:**
  `TTunnelUI::ProcessGUI()` is currently a noop, which means Esc is never checked during
  the SSH tunnel handshake (PuTTY `backend_init` + `Init` -> `EventSelectLoop` -> `FUI->ProcessGUI()`).

  Replace the noop body with delegation to `FTerminal->ProcessGUI()`:

  ```cpp
  // BEFORE:
  void TTunnelUI::ProcessGUI()
  {
      // noop
  }

  // AFTER:
  void TTunnelUI::ProcessGUI()
  {
      FTerminal->ProcessGUI();
  }
  ```

  This mirrors the pattern already used by `TTunnelUI::Information()` (line 274-280) which
  delegates to `FTerminal->Information()` when on the same thread. Since the tunnel event
  loop runs on the same thread as the plugin, the thread-ID check is not needed here.

  **How it works end-to-end:**
  ```
  TTerminal::InternalDoTryOpen() (line 1709: CheckForEsc before tunnel v)
    -> OpenTunnel() -> FTunnel->Open()
      -> TSecureShell::Open() -> backend_init() (TCP+SSH handshake)
      -> Init() -> while(!MAIN_CHANNEL) WaitForData()
        -> EventSelectLoop() -> FUI->ProcessGUI()
          -> TTunnelUI::ProcessGUI() -> FTerminal->ProcessGUI()
            -> if (FStatus==ssOpening && CheckForEsc()) throw EAbort("")
            -> caught by TTerminal::Open() at line 1588: "Connection cancelled by user"
  ```

  **Logging:**
  - Existing `TTerminal::ProcessGUI()` throws `EAbort("")` — no additional logging needed
  - `TTerminal::Open()` catch at line 1588 already logs "Connection cancelled by user (Esc)"
  - Tunnel setup log at line 1693: "Opening tunnel." -> Esc after this will now be detected

  **Files:**
  - `src/core/Terminal.cpp` — replace `TTunnelUI::ProcessGUI()` body (1 line change)

  **Edge cases:**
  - `FTerminal` is always valid in TTunnelUI (set in constructor at line 268)
  - Thread safety: `CheckForEsc()` uses `FOnCheckForEsc` callback -> `GetWinSCPPlugin()->CheckForEsc()`
  - The existing `CheckForEsc()` at line 1709 handles Esc BEFORE tunnel open; this fix handles Esc DURING tunnel open
  - If Esc is pressed, `EAbort("")` propagates through the session open -> `FStatus` reset -> panel stays open

### Phase 2: S3 — Add Esc checks to `TS3FileSystem::Open()`

  - [x] Task 2: Add Esc check before S3 blocking HTTP calls

  **Target:** `src/core/S3FileSystem.cpp` — `TS3FileSystem::Open()` (line 539-692),
  `TS3FileSystem::TryOpenDirectory()` (line 1448-1453)

  **Change:**
  S3 uses synchronous `S3_list_service`/`S3_list_bucket` C library calls with no event loop.
  Esc can only be checked BEFORE these calls, not during them (known limitation).

  Add `FTerminal->CheckForEsc()` at the entry to `Open()` and before `TryOpenDirectory()`:

  ```cpp
  void TS3FileSystem::Open()
  {
      // ... existing: RequireNeon, credentials setup ...

      // Check for user cancellation before S3 HTTP request
      FTerminal->CheckForEsc();

      FActive = false;
      try
      {
          UnicodeString Path = Data->RemoteDirectory;
          if (base::IsUnixRootPath(Path))
          {
              Path = ROOTDIRECTORY;
          }
          // Check Esc again right before the blocking HTTP call
          FTerminal->CheckForEsc();
          TryOpenDirectory(Path);
      }
      catch (Exception & E)
      {
          LibS3Deinitialize();
          FTerminal->Closed();
          FTerminal->FatalError(&E, LoadStr(CONNECTION_FAILED));
      }
      FActive = true;
  }
  ```

  **Logging:**
  - `FTerminal->LogEvent("S3 init: checking Esc before TryOpenDirectory")` — trace level
  - `FTerminal->LogEvent("S3 init: Esc pressed, cancelling connection")` — if Esc detected
  - Existing `TTerminal::Open()` catch at line 1588 already logs the final cancel

  **Files:**
  - `src/core/S3FileSystem.cpp` — add `FTerminal->CheckForEsc()` in `Open()` (before `FActive=false` and before `TryOpenDirectory()`)

  **Edge cases:**
  - `FTerminal` is valid via `TS3FileSystem::FTerminal` member (set during construction)
  - `CheckForEsc()` throttled to 500ms — safe to call multiple times
  - `S3_list_service`/`S3_list_bucket` blocks until HTTP response or timeout (Data->Timeout, default 15s)
  - If Esc is pressed DURING the S3 call, it won't be detected until the call returns
  - After Esc abort, `FStatus` is still `ssOpening` -> exception propagates -> `InternalTryOpen()` catch resets state

### Phase 3: WebDAV — Add Esc checks to `TWebDAVFileSystem::Open()`

  - [x] Task 3: Add Esc checks before WebDAV blocking neon calls

  **Target:** `src/core/WebDAVFileSystem.cpp` — `TWebDAVFileSystem::Open()` (line 213-260),
  `TWebDAVFileSystem::NeonClientOpenSessionInternal()` (line 294-334),
  `TWebDAVFileSystem::ExchangeCapabilities()` (line 447-460)

  **Change:**
  WebDAV uses synchronous neon calls (`ne_session_create`, `ne_options2`) with no event loop.
  Add Esc checks at strategic points in the connection flow.

  **1) In `Open()`, before `OpenUrl()`:**
  ```cpp
  void TWebDAVFileSystem::Open()
  {
      // ... existing setup ...
      FTerminal->Information(LoadStr(STATUS_CONNECT));
      FActive = false;
      try
      {
          FTerminal->CheckForEsc();  // <- ADD
          OpenUrl(Url);
      }
      ...
  }
  ```

  **2) In `NeonClientOpenSessionInternal()` while-loop (before each redirect attempt):**
  ```cpp
  while (true)
  {
      FTerminal->CheckForEsc();  // <- ADD

      FSessionInfo.CSCipher = EmptyStr;
      // ... existing: NeonOpen, ExchangeCapabilities ...
      if (CorrectedUrl.IsEmpty()) break;
      CloseNeonSession();
      CheckRedirectLoop(CorrectedUrl, AttemptedUrls.get());
      Url = CorrectedUrl;
  }
  ```

  **3) In `ExchangeCapabilities()`, before `ne_options2()`:**
  ```cpp
  void TWebDAVFileSystem::ExchangeCapabilities(const char * APath, UnicodeString & CorrectedUrl)
  {
      ClearNeonError();
      int32_t NeonStatus;
      do
      {
          FAuthenticationRetry = false;
          FTerminal->CheckForEsc();  // <- ADD (before each auth retry)
          NeonStatus = ne_options2(FSessionContext->NeonSession, APath, &FCapabilities);
      }
      while ((NeonStatus == NE_AUTH) && FAuthenticationRetry);
      ...
  }
  ```

  **Logging:**
  - `FTerminal->LogEvent("WebDAV init: checking Esc before connect/retry")` — trace level

  **Files:**
  - `src/core/WebDAVFileSystem.cpp` — 3 insertion points (Open, NeonClientOpenSessionInternal, ExchangeCapabilities)

  **Edge cases:**
  - Redirect loop: Esc check inside `while(true)` catches mid-redirect cancellation
  - Auth retry: Esc check inside `do-while` catches mid-auth cancellation
  - `ne_options2()` is synchronous — Esc not detected during the call itself (known limitation)
  - Same post-abort cleanup as S3: `EAbort("")` -> `InternalTryOpen()` catch -> state reset

### Phase 4: FTP — Add explicit Esc check at `Open()` entry

  - [x] Task 4: Add Esc check at `TFTPFileSystem::Open()` entry for early abort

  **Target:** `src/core/FtpFileSystem.cpp` — `TFTPFileSystem::Open()` (line 451-660)

  **Change:**
  FTP already checks Esc during connect via `WaitForMessages()` -> `FTerminal->ProcessGUI()` (line 3224).
  Add an explicit Esc check at the entry point for immediate rejection:

  ```cpp
  void TFTPFileSystem::Open()
  {
      FTerminal->CheckForEsc();  // <- ADD
      // ... existing: FFileZillaIntf->Connect(), WaitForCommandReply ...
  }
  ```

  This provides immediate feedback — if the user pressed Esc while the session dialog was
  being configured, the connection won't even be attempted.

  **No additional logging needed** — existing ProcessGUI path already logs.

  **Files:**
  - `src/core/FtpFileSystem.cpp` — add `FTerminal->CheckForEsc()` as first line of `Open()`

  **Edge cases:**
  - `WaitForMessages()` already polls `FTerminal->ProcessGUI()` every `GUIUpdateInterval` ms
  - The explicit check here catches Esc before any network I/O begins
  - No new cleanup path needed — existing catch handlers cover abort

  - [x] Task 5: Verify existing FTP Esc path through `WaitForMessages()`

  **Target:** `src/core/FtpFileSystem.cpp` — `DoWaitForReply()` / `WaitForMessages()` (line 3200-3230)

  **Change:** Verification only — confirm the existing path works.

  `WaitForMessages()` at line 3224: `FTerminal->ProcessGUI()` is already called in the
  polling loop. `TTerminal::ProcessGUI()` checks `if (FStatus == ssOpening && CheckForEsc()) throw EAbort("")`.
  Since `FStatus = ssOpening` during `TTerminal::Open()` -> `FFileSystem->Open()`, this path works.

  Add trace log for confirmation:
  ```cpp
  // In WaitForMessages, after ProcessGUI():
  FTerminal->ProcessGUI();
  // Trace: if Esc was pressed, we wouldn't reach here
  ```

  No code changes needed — this is a verification-only task.

### Phase 5: Documentation

  - [x] Task 6: Add documentation for Esc session interrupt

  **Target:** `docs/user-guide.md`

  **Change:**
  Add a section documenting the Esc interrupt behavior during session initialization:

  ```markdown
  ## Esc Cancel During Session Initialization

  When connecting to a remote server, you can press **Esc** to cancel the connection attempt:

  | Protocol | Behavior |
  |----------|---------|
  | **SSH/SFTP/SCP** | Esc is checked repeatedly during the SSH handshake and authentication. Press Esc to abort. |
  | **SSH with tunnel** | Esc is checked repeatedly during the tunnel SSH handshake. Press Esc to abort both tunnel and session. |
  | **FTP/FTPS** | Esc is checked during the FTP connection and authentication. |
  | **S3** | Esc is checked before the initial HTTP request. Note: the HTTP call itself cannot be interrupted mid-flight. |
  | **WebDAV** | Esc is checked before each HTTP request (including redirects and auth retries). |

  A progress indicator shows the connection status. When Esc is pressed, the connection is
  cancelled, the panel remains open, and the log records "Connection cancelled by user (Esc)".
  ```

  **Logging:** N/A (documentation only)

  **Files:**
  - `docs/user-guide.md` — add Esc interrupt section

## Verification

### Manual Testing Protocol
For each protocol:
1. **SSH (direct):** Connect to slow server -> press Esc during handshake -> verify abort + panel stays open
2. **SSH (tunnel):** Configure tunnel -> connect -> press Esc during tunnel setup -> verify abort
3. **FTP:** Connect to slow FTP -> press Esc during connect -> verify abort
4. **S3:** Configure S3 session -> press Esc before/during connect -> verify abort at nearest check point
5. **WebDAV:** Configure WebDAV -> press Esc during connect/redirect -> verify abort

### Build Verification
- Clean build with zero warnings (`/W4`)
- Plugin DLL in `Far3_<platform>/Plugins/NetBox/`
- CRLF line endings on all modified files
- UTF-8 without BOM in all text files
- No trailing whitespace
- Naming conventions followed (T/F prefixes, PascalCase)
- No spelling errors in comments

### Quality Gates
- [ ] Zero build warnings under MSVC `/W4`
- [ ] No modifications to `libs/` directory
- [ ] CRLF line endings on all modified files
- [ ] UTF-8 without BOM in all text files
- [ ] No trailing whitespace
- [ ] Naming conventions followed (T/F prefixes, PascalCase)
- [ ] No spelling errors in comments
