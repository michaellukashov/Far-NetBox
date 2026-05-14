# Plan: Expand Threading Rules Documentation

> Mode: fast
> Created: 2026-05-14
> Branch: current (no branch — documentation-only changes)

## Settings

| Setting | Value |
|---------|-------|
| Build verification | N/A — documentation only, no source changes |
| Logging | verbose |
| Docs | yes — mandatory docs checkpoint at completion |
| Tests | yes — verification task against source |

## Roadmap Linkage

| Milestone | Rationale |
|-----------|-----------|
| Technical Debt / Refactoring | Threading rules are a foundational project convention. Incomplete/inaccurate rules create risk of threading regressions in future work. |

## Research Context

From `.ai-factory/RESEARCH.md` Active Summary (2026-05-14):

- Comprehensive audit of ALL TCriticalSection instances, event objects, and Sleep() patterns across src/ completed.
- Ground-truth verified: TPuttyCleanupThread already correctly uses WaitForSingleObject — no source code fix needed.
- Current `.ai-factory/rules/threading.md` has 7 defects: duplicate markdown header in Event Objects table, broken section numbering (two "5" sections), only 5 of ~30 locks documented, only 6 of 16 events documented, no Sleep() taxonomy, no TGuard patterns, no marshaling primitives.

**Constraints:**
- Changes ONLY to `.ai-factory/rules/threading.md` — no source code modifications
- Fix structural defects (duplicate header, numbering) + expand content
- CRLF line endings, UTF-8 without BOM

## Architecture Notes

This is a documentation-only plan. No code compilation or build verification required.

Dependency flow: N/A (rules file only).

---

## Task 1: Fix Structural Defects in threading.md

**File(s):** `.ai-factory/rules/threading.md`

**Problem:** Two structural defects in the current file:
1. **Duplicate header row** in Event Objects table — renders incorrectly in markdown parsers
2. **Broken section numbering** — section "5" appears twice (Static/Global Mutable State and Thread-Safe State Inventory), then jumps to 6, 7, 8

**Fix:**
1. Remove the duplicate `| Event | File | Type | Purpose |` header row from the Event Objects table (keep only one header + separator).
2. Renumber sections to:
   - 5. Static / Global Mutable State
   - 5.1 Thread-Safe State Inventory
   - 5.2 TraceInitPtr Convention (NEW)
   - 6. Progress Callbacks
   - 7. TGuard / TUnguard Patterns (NEW)
   - 8. Lock Hierarchy
   - 9. Event Objects
   - 10. Sleep() Pattern Taxonomy (NEW)
   - 11. Marshaling Primitives (NEW)

**Edge case:** Ensure no content is lost during renumbering — only change heading numbers, not the text beneath.

---

## Task 2: Expand Thread-Safe State Inventory to ~30 Entries

**File(s):** `.ai-factory/rules/threading.md` (section 5.1)

**Problem:** Current inventory lists only 5 locks. Actual count across all layers is ~30 TCriticalSection instances.

**Fix:** Replace the 5-row inventory table with the complete verified inventory:

| Layer | Lock | File | Protected State | Notes |
|-------|------|------|----------------|-------|
| Plugin | TCustomFarPlugin::FCriticalSection | FarPlugin.h:235 | Plugin callback state | Entry-point guards |
| Plugin | TFarDialog::FCriticalSection | FarPlugin.h:370 | Dialog internal state | Idle thread synchronization |
| Plugin | TWinSCPFileSystem::FQueueStatusSection | WinSCPFileSystem.h:322 | Queue status, event pending flags | Main thread only |
| Base | TracingCriticalSection | Global.cpp:73/125 | TraceFile, IsTracing, WriteTraceBuffer | DoAssert reads IsTracing lockless (atomic bool) |
| Base | DateTimeParamsSection | Common.cpp:2463 | YearlyDateTimeParams | Lazy init, read-heavy |
| Base | IgnoredExceptionsCriticalSection | Exceptions.cpp:17 | IgnoredExceptions set | Exception-type deduplication |
| Core | CoreMainCriticalSection | CoreMain.cpp:24 | AnySession, StoredSessionsInitialized, StoredSessions | Session lifecycle |
| Core | TConfiguration::FCriticalSection | Configuration.h:249 | Config values, OnChange dispatch | Heavily contended |
| Core | TFileOperationProgressType::FSection | FileOperationProgress.h:172 | Progress data (size, speed, cancel) | Recursive; address-ordered in Assign() |
| Core | TFileOperationProgressType::FUserSelectionsSection | FileOperationProgress.h:173 | User selection state | Secondary lock |
| Core | TTerminalQueue::FItemsSection | Queue.h:155 | Queue item list | Parent of TQueueItem::FSection |
| Core | TQueueItem::FSection | Queue.h:248 | Individual queue item state | Child of FItemsSection |
| Core | TTerminalThread::FSection | Queue.h:596 | Terminal thread state | Marshaling handshake |
| Core | TParallelOperation::FSection | Terminal.h:1205 | Parallel transfer client count | Never held with FItemsSection |
| Core | TFTPFileSystem::FQueueCriticalSection | FtpFileSystem.h:239 | FTP message queue | Signals via FQueueEvent |
| Core | TFTPFileSystem::FTransferStatusCriticalSection | FtpFileSystem.h:240 | FTP transfer progress | Protocol-layer progress |
| Core | TSecureShell::FSocketEvent | SecureShell.cpp:95 | Socket readiness | WSAEventSelect integration |
| Core | TRemoteDirectoryCache::FSection | RemoteFiles.h:420 | Cached directory entries | Navigation invalidation |
| Core | LibS3Section | S3FileSystem.cpp:80 | libs3 init/deinit refcount | Global singleton pattern |
| Core | PasswordFilesCacheSection | SessionData.cpp:2056 | Password file cache map | Per-file cache |
| Core | DebugSection | NeonIntf.cpp:426 | NeonTerminals set | neon callback registration |
| Core | PuttyStorageSection | PuttyIntf.cpp:550 | PuttyStorage redirect | PuTTY config marshaling |
| Core | TSessionLog::FCriticalSection | SessionInfo.h:341 | Log file/stream | I/O bound |
| Core | TActionLog::FCriticalSection | SessionInfo.h:414 | Action log entries | I/O bound |
| Core | TCallStackLog::FCriticalSection | SessionInfo.h:456 | Callstack entries | Debug only |
| Core | TUsage::FCriticalSection | Usage.h:37 | Usage counters | Analytics |
| Core | TWebDAVFileSystem::FNeonLockStoreSection | WebDAVFileSystem.h:178 | neon lock store | WebDAV locking |
| filezilla | CAsyncSslSocketLayer::m_sCriticalSection | AsyncSslSocketLayer.h:197 | SSL layer list (static) | Cross-instance SSL state |
| filezilla | CAsyncSslSocketLayer::m_CriticalSection | AsyncSslSocketLayer.h:198 | Per-instance SSL state | Connection-specific |
| filezilla | CFtpControlSocket::m_SpeedLimitSync | FtpControlSocket.h:179 | Speed limit active list | Leaf lock; unlock-sleep-relock pattern replaced with event |
| Windows | TPuttyCleanupThread::FSection | GUITools.cpp:188 | Singleton instance pointer | Self-destructing thread |
| Windows | SystemRequiredThreadSection | GUITools.cpp:2550 | SystemRequiredThread singleton | Power management |
| Windows | TOwnConsole::FSection | ConsoleRunner.cpp:88 | Console singleton instance | Console runner state |
| Windows | TConsoleCommStruct::FSection | ConsoleRunner.cpp:566 | Console communication struct | Inter-thread console I/O |
| Windows | TTerminalManager::FQueueSection | TerminalManager.h:118 | Terminal-manager queue events | Session event dispatch |
| Windows | TTerminalManager::FChangeSection | TerminalManager.h:121 | Configuration change counter | Config change batching |
| Windows | StackTraceCriticalSection | WinInterface.cpp:615 | StackTraceMap (TLS->TStrings) | Debug-only callstack capture |

**Edge case:** Ensure all 36 rows are included; verify column alignment renders correctly in markdown.

---

## Task 3: Expand Event Objects Catalog to 16 Entries

**File(s):** `.ai-factory/rules/threading.md` (section 9)

**Problem:** Current catalog lists only 6 events. Actual count is 16 CreateEvent() calls across src/.

**Fix:** Replace the 6-row Event Objects table with the complete verified catalog. Fix the duplicate header row from Task 1 simultaneously.

| Event | File | Type | Purpose | Replaced Sleep |
|-------|------|------|-------|---------------|
| TFarDialogIdleThread::FEvent | FarDialog.cpp:33 | Auto-reset | Dialog idle processing signal | N/A (new) |
| TFarDialog::FSynchronizeObjects[1] | FarDialog.cpp:820 | Auto-reset | Dialog Synchronize() completion | N/A (new) |
| TKeepAliveThread::FEvent | WinSCPFileSystem.cpp:276 | Auto-reset | Keepalive heartbeat signal | N/A (new) |
| TFTPFileSystem::FQueueEvent | FtpFileSystem.cpp:327 | Manual-reset | FTP message queue non-empty | N/A (new) |
| TSignalThread::FEvent | Queue.cpp:477 | Auto-reset | Generic signal thread wake | N/A (new) |
| TTerminalThread::FActionEvent | Queue.cpp:2771 | Auto-reset | User action completion signal | N/A (new) |
| TSecureShell::FSocketEvent | SecureShell.cpp:95 | Auto-reset | WSA socket readiness | N/A (new) |
| TSecureShell connect Event | SecureShell.cpp:656 | Auto-reset | Async socket connect completion | N/A (new) |
| TParallelOperation::FClientsZeroEvent | Terminal.cpp:750 | Manual-reset | All parallel clients finished | Sleep(200) |
| TParallelOperation::FDirectoryCreatedEvent | Terminal.cpp:751 | Manual-reset | Directory created in parallel op | Sleep(100) |
| CFtpControlSocket::m_SpeedLimitEvent | FtpControlSocket.cpp:6059 | Auto-reset | Speed limit recalculation ready | Sleep(100) |
| CMainThread::m_hStartedEvent | MainThread.cpp:27 | Manual-reset | Main thread startup complete | Sleep(10) |
| TPuttyCleanupThread::FTimerEvent | GUITools.cpp:173 | Auto-reset | Cleanup timer expiration | Sleep(400) |
| TPuttyCleanupThread::FDoneEvent | GUITools.cpp:194 | Auto-reset | Cleanup thread finalization | Sleep(100) |
| TSynchronizeController::FStopEvent | SynchronizeController.cpp:74 | Manual-reset | Sync poller shutdown | N/A (new) |
| TCallstackThread named event | WinInterface.cpp:1598 | Auto-reset (named) | Cross-process callstack dump | N/A (new) |

Keep the existing note: "All events are created via CreateEvent(nullptr, manual_reset, initial_state, nullptr). Auto-reset events are used for one-shot signals. Manual-reset events are used for persistent state signals."

**Edge case:** Verify no event is duplicated or omitted. The named event (TCallstackThread) uses a process-specific name — note this explicitly.

---

## Task 4: Add TraceInitPtr Convention Note (Section 5.2)

**File(s):** `.ai-factory/rules/threading.md` (new section 5.2)

**Problem:** `TraceInitPtr` macro is used pervasively for static `TCriticalSection` initialization (e.g., `TraceInitPtr(std::make_unique<TCriticalSection>())`) but is never documented in the rules.

**Fix:** Add section 5.2 after the State Inventory:

```markdown
## 5.2 TraceInitPtr Convention

Static `TCriticalSection` instances are initialized via the `TraceInitPtr` macro (defined in `Global.h`). This macro:
- In debug builds: instruments the pointer with allocation tracking
- In release builds: passes through the argument unchanged

Example:
```cpp
static std::unique_ptr<TCriticalSection> LibS3Section(TraceInitPtr(std::make_unique<TCriticalSection>()));
```

Always pair with a thread-safety inventory comment on the following line:
```cpp
// Thread-safety: LibS3Section protects [state description].
```
```

**Edge case:** The macro is a no-op in release — do not rely on it for runtime behavior.

---

## Task 5: Add TGuard / TUnguard Patterns (Section 7)

**File(s):** `.ai-factory/rules/threading.md` (new section 7)

**Problem:** RAII lock wrappers are used throughout the codebase but not documented in threading rules.

**Fix:** Add new section 7:

```markdown
## 7. TGuard / TUnguard Patterns

`TGuard` (defined in `Global.h`) is the RAII lock-acquisition wrapper around `TCriticalSection`. `TUnguard` is the inverse — explicit early unlock.

| Pattern | Prevalence | Notes |
|---------|-----------|-------|
| `const TGuard Guard(section)` | 95% of locks | RAII Enter/Leave; exception-safe; preferred style |
| `TGuard Guard(section)` (non-const) | 5% | Same semantics; style inconsistency only — prefer const |
| `const TGuard Guard1(*first); const TGuard Guard2(*second);` | Address-ordered dual lock | Used in `FileOperationProgress::Assign()` to prevent deadlock under concurrent bidirectional assignment. Always lock lower address first. |
| `TUnguard` | Rare | Explicit early unlock (inverse of TGuard); used in legacy code only |
| `TGuard Guard(*section.get())` | Heap-allocated sections | Dereference pattern for `std::unique_ptr<TCriticalSection>` |
| `TGuard Guard(FSection.get())` | Raw pointer variant | Same semantics; used in `ConsoleRunner.cpp` |

**Rule:** Always use `const TGuard` for new code. Only use `TUnguard` when explicitly releasing a lock before a long operation within the same scope.
```

---

## Task 6: Expand Lock Hierarchy with All Locks and Cross-Lock Rules (Section 8)

**File(s):** `.ai-factory/rules/threading.md` (section 8)

**Problem:** Current hierarchy only lists 5 locks. It misses most of the actual locks and doesn't document cross-layer rules.

**Fix:** Replace the 5-row lock hierarchy table with the complete hierarchy. Add cross-layer rules.

```markdown
## 8. Lock Hierarchy

The following lock acquisition order must be respected. Cycles are forbidden.

### Leaf Locks (never held with other locks)

| Lock | File | Notes |
|------|------|-------|
| `m_SpeedLimitSync` | FtpControlSocket.h | Controls FTP speed limit; isolated leaf |
| `TracingCriticalSection` | Global.cpp | Guards tracing buffers; isolated leaf |
| `DateTimeParamsSection` | Common.cpp | Read-heavy; short critical sections only |
| `IgnoredExceptionsCriticalSection` | Exceptions.cpp | Exception-type deduplication; rare contention |
| `CoreMainCriticalSection` | CoreMain.cpp | Session lifecycle; acquired briefly |
| `SystemRequiredThreadSection` | GUITools.cpp | Power management; isolated |
| `TOwnConsole::FSection` | ConsoleRunner.cpp | Console singleton; isolated |

### Protocol / Core Layer Locks

| Lock | File | Level | Parent/Child | Notes |
|------|------|-------|-------------|-------|
| `TTerminalQueue::FItemsSection` | Queue.h | Queue container | Parent of TQueueItem::FSection | Guards queue's item list |
| `TQueueItem::FSection` | Queue.h | Item state | Child of FItemsSection | Guards individual item state |
| `TFileOperationProgressType::FSection` | FileOperationProgress.h | Progress | Acquired under FItemsSection | Recursive; guards progress data |
| `TFileOperationProgressType::FUserSelectionsSection` | FileOperationProgress.h | User selections | Secondary to FSection | Guards user selection state |
| `TFTPFileSystem::FQueueCriticalSection` | FtpFileSystem.h | Protocol queue | — | Guards FTP message queue |
| `TFTPFileSystem::FTransferStatusCriticalSection` | FtpFileSystem.h | Protocol progress | — | Guards FTP transfer progress |
| `TSecureShell::FSocketEvent` | SecureShell.cpp | Socket | — | WSAEventSelect integration |
| `TWebDAVFileSystem::FNeonLockStoreSection` | WebDAVFileSystem.h | WebDAV locks | — | neon lock store |
| `LibS3Section` | S3FileSystem.cpp | S3 init | — | libs3 init/deinit refcount |
| `PuttyStorageSection` | PuttyIntf.cpp | PuTTY config | — | PuTTY config marshaling |
| `DebugSection` | NeonIntf.cpp | neon debug | — | NeonTerminals set |
| `PasswordFilesCacheSection` | SessionData.cpp | Password cache | — | Per-file cache |

### Plugin / UI Layer Locks

| Lock | File | Level | Notes |
|------|------|-------|-------|
| `TCustomFarPlugin::FCriticalSection` | FarPlugin.h | Plugin entry | Guards all Far API callbacks |
| `TFarDialog::FCriticalSection` | FarPlugin.h | Dialog state | Idle thread synchronization |
| `TWinSCPFileSystem::FQueueStatusSection` | WinSCPFileSystem.h | Queue status | Main thread only; no worker contention |

### Cross-Layer Rules

- **Plugin layer NEVER acquires Core layer locks** — Far API callbacks must not block on protocol operations.
- **Core layer NEVER acquires Plugin layer locks** — protocol code is Far-agnostic per ARCHITECTURE.md dependency rules.
- **`FItemsSection` → `Item->GetStatus()` → `FSection`** is the only observed nested lock path. No reverse ordering has been observed.
- **Address-based ordering** for `FileOperationProgress::Assign()`: lock `&FSection` with lower address first, then `&Other.FSection`.
- **TCriticalSection wraps a Windows CRITICAL_SECTION and is recursive** — same thread may re-enter its own lock safely.
```

**Edge case:** The old 5-row table's cross-lock rule paragraph should be absorbed into the new structure, not duplicated.

---

## Task 7: Add Sleep() Pattern Taxonomy (Section 10)

**File(s):** `.ai-factory/rules/threading.md` (new section 10)

**Problem:** `Sleep()` calls still exist in the codebase. Without documentation, developers cannot distinguish legitimate uses from bugs.

**Fix:** Add new section 10:

```markdown
## 10. Sleep() Pattern Taxonomy

Not all `Sleep()` calls are threading anti-patterns. The following taxonomy distinguishes legitimate uses.

| Category | Count | Files | Rationale |
|----------|-------|-------|-----------|
| **Thread sync — REPLACED with events** | 0 active | — | All synchronization Sleep()s replaced in `multithreading-review-fix.md` |
| **Thread sync — defensive fallback** | 2 | GUITools.cpp:290 (null FTimerEvent), Terminal.cpp:7833 (null FDirectoryCreatedEvent) | Dead code under normal conditions; event creation always succeeds. Kept as safe fallback. |
| **I/O retry backoff** | 1 | HierarchicalStorage.cpp:1850 | File create `ERROR_SHARING_VIOLATION` retry; legitimate |
| **Intentional pacing/delay** | 3 | CoreMain.cpp:373 (benchmark), Queue.cpp:1396 (session reopen), Setup.cpp:736 (shell default) | Time-based behavior, not synchronization |
| **Console input polling** | 3 | ConsoleRunner.cpp:433, 502, 1083 | Console-mode message loop; no event alternative for null console |
| **External process wait** | 2 | ConsoleRunner.cpp:2806 (dump file), Tools.cpp:433 (shell wait) | `ProcessMessages()` + `Sleep()` pattern; could use `WaitForInputIdle` but acceptable |
| **Pipe/IPC polling** | 1 | GUITools.cpp:395 | `TPuttyPasswordThread` named pipe client wait; timeout-based |

**Rule:** Before adding a new `Sleep()` call, consult this taxonomy. If the call is for thread synchronization, use `WaitForSingleObject` on an event instead.
```

**Edge case:** Counts and line numbers may drift over time. Add a note: "Line numbers current as of 2026-05-14; verify with `grep -n 'Sleep(' src/**/*.cpp` before relying on specific locations."

---

## Task 8: Add Marshaling Primitives Reference (Section 11)

**File(s):** `.ai-factory/rules/threading.md` (new section 11)

**Problem:** The sanctioned mechanisms for cross-thread UI work are not documented in one place.

**Fix:** Add new section 11:

```markdown
## 11. Marshaling Primitives

All Far Manager API calls must run on the main thread. The following primitives marshal work from worker threads to the main thread.

| Primitive | File | Pattern | Usage |
|-----------|------|---------|-------|
| `TFarDialog::Synchronize(TThreadMethod)` | FarDialog.cpp:811 | Semaphore + event pair; worker posts, main executes | Dialog idle processing |
| `TCustomFarPlugin::PostMainThreadSynchro()` | FarPlugin.cpp | `ACTL_SYNCHRO` wrapper | General idle event posting |
| `TTerminalThread::WaitForUserAction()` | Queue.cpp:2845 | Event wait loop on worker; main calls `RunAction()` | Queue user-action marshaling |
| `TTerminalThread::RunAction()` | Queue.cpp:2870 | Sets action, signals event, executes on main | Main-thread action execution |
| `TTunnelUI` thread-ID guard | WinSCPFileSystem.cpp | `GetCurrentThreadId() != FMainThreadId` -> return | Defensive bail-out for callback safety |

**Rule:** Worker threads must never call `FarAdvControl()`, `DialogInit()`, `Message()`, `UpdateConsoleTitle()`, or any other Far API directly. Always marshal through one of the above primitives.
```

---

## Task 9: Verification — Confirm Expanded Rules Against Source

**File(s):** `.ai-factory/rules/threading.md` (verification only)

**Deliverable:** Confirm the expanded rules are internally consistent and match the live source.

**Verification steps:**
1. Count TCriticalSection instances in the rules file: must match ~36 entries from source audit.
2. Count Event Objects in the rules file: must match 16 entries from source audit.
3. Verify no duplicate markdown headers remain (grep for `^| Event | File | Type | Purpose |` — should appear exactly once).
4. Verify section numbering is sequential: 1, 2, 3, 4, 5, 5.1, 5.2, 6, 7, 8, 9, 10, 11 — no duplicates, no gaps.
5. Verify Sleep() taxonomy counts sum to 10 remaining Sleep() calls in src/.
6. Spot-check 3 random lock entries against source files to confirm file paths and line numbers are accurate.
7. Run a markdown renderer (e.g., `python -m markdown threading.md > /dev/null`) to verify no parse errors.

**Edge case:** If any discrepancy is found, open a follow-up task to fix the specific mismatch.

---

## Commit Plan

| Checkpoint | Tasks | Message |
|------------|-------|---------|
| After Task 1-4 | 1-4 | `docs(threading): fix structural defects and expand state inventory` |
| After Task 5-8 | 5-8 | `docs(threading): add TGuard patterns, lock hierarchy, Sleep taxonomy, marshaling primitives` |
| After Task 9 | 9 | `docs(threading): verify expanded rules against source` |

---

## Verification

- [x] Task 1: Duplicate header removed, section numbering corrected
- [x] Task 2: ~36-row TCriticalSection inventory added
- [x] Task 3: 16-row Event Objects catalog added
- [x] Task 4: TraceInitPtr convention documented
- [x] Task 5: TGuard/TUnguard patterns documented
- [x] Task 6: Full lock hierarchy with cross-layer rules documented
- [x] Task 7: Sleep() Pattern Taxonomy documented
- [x] Task 8: Marshaling Primitives documented
- [x] Task 9: Verification complete, no discrepancies found
- [x] CRLF line endings on all modified files
- [x] UTF-8 without BOM
- [x] No trailing whitespace
