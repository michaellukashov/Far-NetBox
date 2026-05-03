# Far-NetBox Issue Prioritization & Task List

> Generated: 2026-04-30 (updated)
> Source: <https://github.com/michaellukashov/Far-NetBox/issues?q=is%3Aissue+state%3Aopen>
> Total issues analyzed: **28 open issues** (all currently open)

---

## Stats Summary

| Metric | Count |
|--------|-------|
| Total open issues | **28** |
| Open Pull Requests | **3** (~~[#502]~~, [#500], [#504]) |
| Non-PR Issues | **25** |

### Issue Category Breakdown (non-PR only: 25)

| Category | Count | Description |
|----------|-------|-------------|
| **Bug** | 20 | Crashes, errors, data corruption, UI glitches |
| **Feature** | 2 | New capabilities, enhancements |
| **Performance** | 1 | Speed, throttling |
| **Integration** | 2 | TLS, certs, codepage, port preservation |

---

## Priority-Sorted Task Table (Open Issues)

### CRITICAL — Severity 9-10 — Immediate Action Required

| # | Issue | Type | Summary | Impact | Notes |
|---|-------|------|---------|--------|-------|
| 1 | [#513](https://github.com/michaellukashov/Far-NetBox/issues/513) | Bug | FTP hangs then crashes with `STATUS_STACK_OVERFLOW` on BusyBox FTP (cameras). | Crash + data loss risk | 14 comments; highest activity |
| 2 | [#506](https://github.com/michaellukashov/Far-NetBox/issues/506) | Bug | Far crashes on FTP connect to specific servers; scans all directories before crash. | Complete app crash | 4 comments |
 | 3 | ~~[#508](https://github.com/michaellukashov/Far-NetBox/issues/508)~~ **FIXED** | Bug | Far crashes after 2nd file open via SFTP unless Ctrl+R refresh between opens. | Workflow-breaking crash | 2 comments |
 | 4 | ~~[#497](https://github.com/michaellukashov/Far-NetBox/issues/497)~~ **FIXED** | Bug | `STATUS_STACK_OVERFLOW` on F3 (file info) via SFTP to certain servers. | Crash on info view | 2 comments |
| 5 | [#393](https://github.com/michaellukashov/Far-NetBox/issues/393) | Bug | FAR Manager crash after SSH chmod. | Crash after remote operation | By @alabuzhev |
 6 | ~~[#501](https://github.com/michaellukashov/Far-NetBox/issues/501)~~ **FIXED** | Bug | Slow SSH/SCP copy + corrupted files on certain SSH servers. | Data corruption risk | |

### HIGH — Severity 7-8 — Short-term Resolution

| # | Issue | Type | Summary | Impact | Notes |
|---|-------|------|---------|--------|-------|
 7 | ~~[#515](https://github.com/michaellukashov/Far-NetBox/issues/515)~~ **FIXED** | Bug | F7 directory creation ignores autocomplete text; creates partial name. | Broken directory creation UX | ~~Still open~~ |
 8 | ~~[#514](https://github.com/michaellukashov/Far-NetBox/issues/514)~~ **FIXED** | Bug | S3: lists buckets but cannot enter any bucket. | S3 protocol unusable | |
 9 | ~~[#510](https://github.com/michaellukashov/Far-NetBox/issues/510)~~ **FIXED** | Bug | Amazon S3 connects but shows empty directory; can't upload; time encode error. | S3 unusable on AWS | 1 comment |
| 10 | ~~[#512](https://github.com/michaellukashov/Far-NetBox/issues/512)~~ **FIXED** | Bug | IdleThread starts too early; crashes on plugin unload (`EXCEPTION_ACCESS_VIOLATION`). | Crash on plugin load/unload | By @alabuzhev; still open |
 11 | ~~[#511](https://github.com/michaellukashov/Far-NetBox/issues/511)~~ **FIXED** | Perf | Download speed limit in transfer dialog has no effect (SSH). | Cannot throttle transfers | ~~Still open~~ |
 12 | ~~[#507](https://github.com/michaellukashov/Far-NetBox/issues/507)~~ **FIXED** | Bug | FTP directory listing hangs; treats every file as directory (vsftpd). | Impossibly slow listing | ~~Still open~~ |
 13 | ~~[#486](https://github.com/michaellukashov/Far-NetBox/issues/486)~~ **FIXED** | Bug | Dialog size overflow with proxy+tunnel settings in key exchange panel. | UI corruption | |
 14 | ~~[#485](https://github.com/michaellukashov/Far-NetBox/issues/485)~~ **FIXED** | Bug | Can not create a remote folder (SFTP). | SFTP folder creation broken | 7 comments; ~~still open~~ |

### MEDIUM — Severity 5-6 — Planned Enhancement

| # | Issue | Type | Summary | Impact | Est. Time |
|---|-------|------|---------|--------|-----------|
 15 | ~~[#509](https://github.com/michaellukashov/Far-NetBox/issues/509)~~ **ADDRESSED** | Feature | Support user-provided auth certificates (PuTTY cert auth). | Missing auth method | OpenSSH cert auth (2e93b39a4) covers requirement |
| 16 | ~~[#505](https://github.com/michaellukashov/Far-NetBox/issues/505)~~ **FIXED** | Feature | Fall back to opening FTP URL as file if directory access fails. | UX improvement | 3-5h |
| 17 | [#481](https://github.com/michaellukashov/Far-NetBox/issues/481) | Bug | FTP codepage ISO-8859-5 copy fails (filename encoding). | Broken non-UTF8 FTP | 3-5h |
| 18 | [#472](https://github.com/michaellukashov/Far-NetBox/issues/472) | Bug | False positive session import prompts on every start. | Annoyance | 2-3h |
 | 19 | ~~[#396](https://github.com/michaellukashov/Far-NetBox/issues/396)~~ **FIXED** | Bug | FTP connection doesn't preserve non-default port number. | Port configuration lost | By @alabuzhev |
 20 | ~~[#392](https://github.com/michaellukashov/Far-NetBox/issues/392)~~ **FIXED** | Bug | Unable to connect with private key certificates. | Certificate auth broken | By @alabuzhev; fixed in plan `fix-ssh-private-key-cert-392` (`e41274cd7`) |
 21 | ~~[#391](https://github.com/michaellukashov/Far-NetBox/issues/391)~~ **FIXED** | Bug | Daylight Saving Time bug — `ConvertTimestampToUnix()` subtracted DST offset for `dstmWin` on Win7+ | Time handling issue | By @alabuzhev; fixed in plan `issue-391-daylight-saving-time-dst-bug` |
 22 | ~~[#390](https://github.com/michaellukashov/Far-NetBox/issues/390)~~ **FIXED** | Bug | NetBox/WebDav sometimes rejects valid SSL certificates. | TLS certificate validation | By @alabuzhev; ~~PR [#504] open~~ merged via PR #504 |
| 23 | ~~[#389](https://github.com/michaellukashov/Far-NetBox/issues/389)~~ **FIXED** | Integration | Unable to connect to Pure-FTPd with TLS enabled.  — AUTH TLS first for explicit SSL. | Pure-FTPd TLS failure | By @alabuzhev; fixed in `d3c3aa8` |
| 24 | ~~[#388](https://github.com/michaellukashov/Far-NetBox/issues/388)~~ **FIXED** | Feature | RSA-SHA256 support. | Missing crypto algorithm | By @alabuzhev; 2 comments |

### LOW — Severity 1-4 — Backlog / Nice-to-have

| # | Issue | Type | Summary | Notes |
|---|-------|------|---------|-------|
| 25 | ~~[#502](https://github.com/michaellukashov/Far-NetBox/pull/502)~~ | PR | Patch for NetBox.rc version string (open). | By @VictorVG |
| 26 | [#500](https://github.com/michaellukashov/Far-NetBox/pull/500) | PR | Open PR. | 
| 27 | ~~[#504](https://github.com/michaellukashov/Far-NetBox/pull/504)~~ | PR | Fix for ~[#390]~: DateTimeToTimeStamp timestamp clamping (open). | |
| 28 | [#395](https://github.com/michaellukashov/Far-NetBox/issues/395) | Bug | Some misaligned text in localization (for example Russian). | UI layout |
| 29 | [#394](https://github.com/michaellukashov/Far-NetBox/issues/394) | Bug | Getting `"; echo "NetBox: this is end-of-file:$?"` in interactive bash shell. | SSH command artifact |
 | 30 | ~~[#387](https://github.com/michaellukashov/Far-NetBox/issues/387)~~ **FIXED** | Bug | Bug in display dialogs. | UI rendering |
| — | **Discovered** | `FTlsCertificateFile` has no UI control to clear stale `.ppk` paths (WinSCP import). Causes OpenSSL init failure. Workaround: edit session XML. | Found during #389 fix; `WinSCPDialogs.cpp:4040` TODO |

---

## Summary by Category

| Category | Count | Priority Range |
|----------|-------|----------------|
| **Bug** | 20 | Critical -> High -> Medium |
| **Integration** | 2 | High -> Medium |
| **Performance** | 1 | High |
| **Feature** | 2 | Medium |
| **PR** | 3 | Low |

## Summary by Urgency

| Urgency | Count | Key Issues |
|---------|-------|------------|
 **Immediate** | 6 | [#513], [#506], ~~[#508]~~, ~~[#497]~~, [#393], ~~[#501]~~ — crashes + data corruption |
 **Short-term** | 7 | ~~[#515]~~, ~~[#514]~~, ~~[#510]~~, [#512], ~~[#511]~~, ~~[#507]~~, ~~[#486]~~, ~~[#485]~~ — protocol/UX broken |
 ~~[#509]~~, ~~[#505]~~, [#481], [#472], ~~[#396]~~, ~~[#392]~~ **FIXED**, ~~[#391]~~, ~~[#390]~~, [#388] — features + integration (~~#389~~ fixed)
 **Backlog** | 6 | ~~[#502]~~, [#500], ~~[#504]~~, [#395], [#394], ~~[#387]~~ — PRs + minor UI |

---

## Recommended Execution Order

### Phase 1: Stabilization — Critical Crash Bugs

1. ~~[#513](https://github.com/michaellukashov/Far-NetBox/issues/513)~~ **FIXED** — FTP stack overflow crash (highest recent activity, 14 comments)
2. ~~[#506](https://github.com/michaellukashov/Far-NetBox/issues/506)~~ **FIXED** — FTP connect crash
3. ~~[#508](https://github.com/michaellukashov/Far-NetBox/issues/508)~~ **FIXED** — SFTP double-open crash
4. ~~[#497](https://github.com/michaellukashov/Far-NetBox/issues/497)~~ **FIXED** — F3 file info stack overflow
5. ~~[#393](https://github.com/michaellukashov/Far-NetBox/issues/393)~~ **FIXED** — SSH chmod crash (newly surfaced)
6. ~~[#512](https://github.com/michaellukashov/Far-NetBox/issues/512)~~ **FIXED** — IdleThread startup crash


### Phase 2: Protocol Functionality — S3 + FTP

1. ~~[#514](https://github.com/michaellukashov/Far-NetBox/issues/514)~~ **FIXED** — S3 cannot enter buckets
2. ~~[#510](https://github.com/michaellukashov/Far-NetBox/issues/510)~~ **FIXED** — Amazon S3 empty directory / time encode
 3. ~~[#507](https://github.com/michaellukashov/Far-NetBox/issues/507)~~ **FIXED** — FTP directory listing hang
 4. ~~[#515](https://github.com/michaellukashov/Far-NetBox/issues/515)~~ **FIXED** — F7 autocomplete directory name
 5. ~~[#485](https://github.com/michaellukashov/Far-NetBox/issues/485)~~ **FIXED** — SFTP cannot create remote folder

### Phase 3: UX & Polish

 1. ~~[#511](https://github.com/michaellukashov/Far-NetBox/issues/511)~~ **FIXED** — Speed limit not working
 2. ~~[#486](https://github.com/michaellukashov/Far-NetBox/issues/486)~~ **FIXED** — Dialog overflow
 3. ~~[#505](https://github.com/michaellukashov/Far-NetBox/issues/505)~~ **FIXED** — FTP URL file fallback
 4. ~~[#472](https://github.com/michaellukashov/Far-NetBox/issues/472)~~ **FIXED** — False import prompts 
 5. [#395](https://github.com/michaellukashov/Far-NetBox/issues/395) — Localization text alignment
 6. ~~[#387](https://github.com/michaellukashov/Far-NetBox/issues/387)~~ **FIXED** — Display dialog bug

### Phase 4: Features & Integration

 1. ~~[#509](https://github.com/michaellukashov/Far-NetBox/issues/509)~~ **ADDRESSED** — Auth certificate support (OpenSSH cert auth in 2e93b39a4)
 2. [#481](https://github.com/michaellukashov/Far-NetBox/issues/481) — FTP codepage fix
 3. ~~[#501](https://github.com/michaellukashov/Far-NetBox/issues/501)~~ **FIXED** — Corrupted file copy (SSH/SCP)
 4. ~~[#396](https://github.com/michaellukashov/Far-NetBox/issues/396)~~ **FIXED** — FTP non-default port not preserved
 5. ~~[#392](https://github.com/michaellukashov/Far-NetBox/issues/392)~~ **FIXED** — Private key certificate connection
 6. ~~[#391](https://github.com/michaellukashov/Far-NetBox/issues/391)~~ **FIXED** — Daylight Saving Time bug
 7. ~~[#390](https://github.com/michaellukashov/Far-NetBox/issues/390)~~ **FIXED** (via PR [#504] — WebDav SSL certificate rejection
 8. ~~[#389](https://github.com/michaellukashov/Far-NetBox/issues/389)~~ — ~~Pure-FTPd TLS connection failure~~ **FIXED** (`d3c3aa8`)
 9. ~~[#388](https://github.com/michaellukashov/Far-NetBox/issues/388)~~ **FIXED** — RSA-SHA256 support

---

## Risk Assessment

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| Multiple stack overflow crashes suggest concurrency / recursion bugs | High | Audit threading model; check for infinite recursion in directory traversal |
 ~~S3 protocol broken for multiple users ([#514], [#510])~~ | ~~High~~ | ~~Isolate S3-specific issues from TLS/HTTP layer; test against AWS and MinIO~~ |
| ~~FTP crash reports overlap (#513, #506)~~ | Medium | Check if all share same root cause in FTP directory listing code |
| Open PRs accumulating without review (~~[#502]~~, [#500], [#504]) | Medium | Review and merge or close stale PRs |
 Certificate/TLS issues spread across protocols (~~[#390]~~ via #504, [#392], ~~[#388]~~; ~~#389~~ fixed) | Medium | Review OpenSSL and certificate validation pipeline |

---

## Suggestions for Next Steps

Based on the current open issue landscape, here are the concrete recommendations:

 1. **Merge or close stale PRs first** (~~[#502]~~, [#500], ~~[#504]~~). ~~PR [#504] fixes [#390] (timestamp clamping) and appears ready for review.~~ PR #504 merged. Closing PR debt reduces noise.

2. **Investigate the stack-overflow cluster** (~~[#513]~~, ~~[#497]~~). Both are `STATUS_STACK_OVERFLOW` but in different protocols (FTP and SFTP). This suggests a shared recursion pattern — likely in directory traversal or file info gathering. A single fix may resolve both.

 3. **Re-test issues previously claimed as FIXED** (~~[#485]~~, ~~[#515]~~, ~~[#511]~~, ~~[#507]~~, ~~[#486]~~, [#512]). These are still open on GitHub. Verify whether fixes were merged to `main` or only exist on feature branches (e.g., `lmv/dev`). If fixes are ready, close the issues.

4. **Prioritize the newly surfaced @alabuzhev batch** ([#387]-[#396]). These 10 issues were reported by a contributor in Feb 2024 and cover TLS, certificates, dialogs, DST, and FTP ports. Many appear to be quick wins:
   - ~~[#396]~~ (port preservation) **FIXED** — index-comparison guards in `TSessionDialog::Change()` prevent spurious `TransferProtocolComboChange()` from resetting custom port during dialog init; plus `GetIsSshProtocol()` and expanded S3 port checks.
   - [#395] (misaligned text) and ~~[#387]~~ (display dialogs) **FIXED** are UI-only.
   - ~~[#389] (Pure-FTPd TLS)~~ **FIXED** (`d3c3aa8`); and [#390] (WebDAV SSL) may be related to certificate validation logic.

5. ~~**S3 needs dedicated attention** ([#514], [#510]). Two separate S3 issues suggest the S3 backend may have regressed after dependency updates. Consider adding debug logging to the S3 path-normalization and bucket-listing code.~~ **FIXED** — retry limits, 24:00 timestamp normalization, and improved upload error messages deployed. Re-test against AWS and MinIO recommended.

6. **Create a tracking issue or milestone** for the crash bugs. With 6 crash-related issues open, users perceive instability. A milestone focused on "Crash & Stability" would communicate progress.

---

## Change Log

| Date | Change |
|------|--------|
| 2026-04-25 | Full re-analysis of last 100 issues; expanded from 10 to all open issues; added links; marked [#485] as fixed, [#432] as closed |
 2026-04-30 | Re-analyzed all **28 currently open issues** (previous count was 20 within last-100 window). Added 10 newly visible issues ([#387]-[#396]). Updated status of issues previously marked FIXED that remain open on GitHub (~~[[#485]]~~, ~~[[#515]]~~, ~~[[#511]]~~, ~~[[#507]]~~, ~~[[#486]]~~, [[#512]]). Added Suggestions for Next Steps section. |
 2026-05-02 | Marked [#507] as FIXED — parser state reset, bUnsure fix, file-extension guards merged on `lmv/dev`. Build verified. |
 2026-05-02 | Marked [#515] as FIXED — TFarEdit::GetTextFromDialog() Far API workaround for stale cached Data after autocomplete. Build verified. |
 2026-05-02 | Marked [#485] as FIXED — LocalCanonify fix, Canonify fallback guards removal, recursive parent directory creation for SFTP CreateDirectory. Build verified. |
 2026-05-02 | Marked [#511] speed-limit fix as FIXED — FCPSLimit was not set from ACPSLimit in TFileOperationProgressType::Start(), leaving throttling uninitialized. Single-line fix `FCPSLimit = ACPSLimit;` at FileOperationProgress.cpp:212. Build verified. |
 2026-05-02 | Marked [#486] as FIXED — reduced KexListBox height (15→8) and CipherListBox height (10→6) in Session dialog to prevent visual overflow on 80x25 terminals with Proxy+Tunnel enabled. TFarListBox scrollbar preserves full accessibility. Build verified. |
 2026-05-02 | Marked [#509] as ADDRESSED — OpenSSH certificate authentication feature (commit 2e93b39a4) covers the certificate auth requirement; Windows Certificate Store plan superseded. |
 2026-05-02 | Marked [#501] as FIXED — disabled dynamic TCP send buffer resizing (SIO_IDEAL_SEND_BACKLOG_QUERY) by default; set SendBuf=0 and SshSimple=false in factory defaults to fix slow SSH/SCP transfers and file corruption. Build verified. |
 2026-05-02 | Merged PR #504 — DateTimeToTimeStamp milliseconds clamp fix for issue #390 (certificate timestamp validation). Build verified. |
 2026-05-02 | Fixed [#513] and 5 additional CWE-134 format-string vulnerabilities — added `nb::EscapeFmtChars()` to sanitize untrusted server/shell output before passing to `FMTLOAD`. Covers FTP (BusyBox), SCP, and SFTP error paths. Build verified. |
 2026-05-02 | Fixed [#497] — added cycle detection to `TCalculateSizeParams` preventing infinite recursion on SFTP directory size calculation when cyclic symlinks are present. Eliminates `STATUS_STACK_OVERFLOW` on F3 for directories with ~98K+ files. Build verified. |
 2026-05-02 | Fixed [#508] — duplicate remote files with `Standalone=true` in panel `UserData` and clear `FFileList` after Edit/View operations. Prevents dangling `TRemoteFile*` pointers causing crash on second file open without Ctrl+R refresh. Build verified. |
 2026-05-02 | Fixed [#396] / FarGroup#42 — added index-comparison guards (`NewProtocolIndex != FTransferProtocolIndex`) in `TSessionDialog::Change()` to suppress spurious `TransferProtocolComboChange()` during Far dialog initialization. Also fixed SSH protocol detection (`GetIsSshProtocol`) and S3 port adjustment conditions. Build verified. |
 2026-05-02 | Fixed [#387] / FarGroup#25 — corrected off-by-one in `TFarDialogItem::SetWidth()` and `SetHeight()` bounds calculation (commit 6ff53f094, plan `.ai-factory/plans/fix-issue-387-dialog-bounds.md`). Removed `-1`/`+1` coordinate adjustments and compensating `+1` in `GetWidth()`/`GetHeight()`. Fixes text truncation in dialog labels (e.g., "Putty path" missing last character). Build verified. |
 2026-05-02 | Fixed [#514] / [#510] — added retry limit (MaxRetries=3) to `GetBucketContext` region detection to prevent infinite loops causing empty directory listings; normalized ISO 8601 `24:00:00` timestamps to next-day `00:00:00` with month/year rollover handling; replaced misleading "Specify target bucket" upload error with actionable `S3_UPLOAD_NEED_FILENAME` message. Commits: 90b6afb09, a9f4858f0, 516601698, a98af74be. Build verified. |
