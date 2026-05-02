# Far-NetBox Issue Prioritization & Task List

> Generated: 2026-04-30 (updated)
> Source: <https://github.com/michaellukashov/Far-NetBox/issues?q=is%3Aissue+state%3Aopen>
> Total issues analyzed: **28 open issues** (all currently open)

---

## Stats Summary

| Metric | Count |
|--------|-------|
| Total open issues | **28** |
| Open Pull Requests | **3** ([#502], [#500], [#504]) |
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
| 3 | [#508](https://github.com/michaellukashov/Far-NetBox/issues/508) | Bug | Far crashes after 2nd file open via SFTP unless Ctrl+R refresh between opens. | Workflow-breaking crash | 2 comments |
| 4 | [#497](https://github.com/michaellukashov/Far-NetBox/issues/497) | Bug | `STATUS_STACK_OVERFLOW` on F3 (file info) via SFTP to certain servers. | Crash on info view | 2 comments |
| 5 | [#393](https://github.com/michaellukashov/Far-NetBox/issues/393) | Bug | FAR Manager crash after SSH chmod. | Crash after remote operation | By @alabuzhev |
| 6 | [#501](https://github.com/michaellukashov/Far-NetBox/issues/501) | Bug | Slow SSH/SCP copy + corrupted files on certain SSH servers. | Data corruption risk | |

### HIGH — Severity 7-8 — Short-term Resolution

| # | Issue | Type | Summary | Impact | Notes |
|---|-------|------|---------|--------|-------|
| 7 | [#515](https://github.com/michaellukashov/Far-NetBox/issues/515) | Bug | F7 directory creation ignores autocomplete text; creates partial name. | Broken directory creation UX | Still open |
| 8 | [#514](https://github.com/michaellukashov/Far-NetBox/issues/514) | Bug | S3: lists buckets but cannot enter any bucket. | S3 protocol unusable | |
| 9 | [#510](https://github.com/michaellukashov/Far-NetBox/issues/510) | Bug | Amazon S3 connects but shows empty directory; can't upload; time encode error. | S3 unusable on AWS | 1 comment |
| 10 | [#512](https://github.com/michaellukashov/Far-NetBox/issues/512) | Bug | IdleThread starts too early; crashes on plugin unload (`EXCEPTION_ACCESS_VIOLATION`). | Crash on plugin load/unload | By @alabuzhev; still open |
| 11 | [#511](https://github.com/michaellukashov/Far-NetBox/issues/511) | Perf | Download speed limit in transfer dialog has no effect (SSH). | Cannot throttle transfers | Still open |
| 12 | [#507](https://github.com/michaellukashov/Far-NetBox/issues/507) | Bug | FTP directory listing hangs; treats every file as directory (vsftpd). | Impossibly slow listing | Still open |
| 13 | [#486](https://github.com/michaellukashov/Far-NetBox/issues/486) | Bug | Dialog size overflow with proxy+tunnel settings in key exchange panel. | UI corruption | |
| 14 | [#485](https://github.com/michaellukashov/Far-NetBox/issues/485) | Bug | Can not create a remote folder (SFTP). | SFTP folder creation broken | 7 comments; still open |

### MEDIUM — Severity 5-6 — Planned Enhancement

| # | Issue | Type | Summary | Impact | Est. Time |
|---|-------|------|---------|--------|-----------|
| 15 | [#509](https://github.com/michaellukashov/Far-NetBox/issues/509) | Feature | Support user-provided auth certificates (PuTTY cert auth). | Missing auth method | 4-8h |
| 16 | [#505](https://github.com/michaellukashov/Far-NetBox/issues/505) | Feature | Fall back to opening FTP URL as file if directory access fails. | UX improvement | 3-5h |
| 17 | [#481](https://github.com/michaellukashov/Far-NetBox/issues/481) | Bug | FTP codepage ISO-8859-5 copy fails (filename encoding). | Broken non-UTF8 FTP | 3-5h |
| 18 | [#472](https://github.com/michaellukashov/Far-NetBox/issues/472) | Bug | False positive session import prompts on every start. | Annoyance | 2-3h |
| 19 | [#396](https://github.com/michaellukashov/Far-NetBox/issues/396) | Bug | FTP connection doesn't preserve non-default port number. | Port configuration lost | By @alabuzhev |
| 20 | [#392](https://github.com/michaellukashov/Far-NetBox/issues/392) | Bug | Unable to connect with private key certificates. | Certificate auth broken | By @alabuzhev; 2 comments |
| 21 | [#391](https://github.com/michaellukashov/Far-NetBox/issues/391) | Bug | Daylight Saving Time bug. | Time handling issue | By @alabuzhev |
| 22 | [#390](https://github.com/michaellukashov/Far-NetBox/issues/390) | Bug | NetBox/WebDav sometimes rejects valid SSL certificates. | TLS certificate validation | By @alabuzhev; PR [#504] open |
| 23 | [#389](https://github.com/michaellukashov/Far-NetBox/issues/389) | Integration | ~~Unable to connect to Pure-FTPd with TLS enabled.~~ **FIXED** — AUTH TLS first for explicit SSL. | Pure-FTPd TLS failure | By @alabuzhev; fixed in `d3c3aa8` |
| 24 | [#388](https://github.com/michaellukashov/Far-NetBox/issues/388) | Feature | RSA-SHA256 support. | Missing crypto algorithm | By @alabuzhev; 2 comments |

### LOW — Severity 1-4 — Backlog / Nice-to-have

| # | Issue | Type | Summary | Notes |
|---|-------|------|---------|-------|
| 25 | [#502](https://github.com/michaellukashov/Far-NetBox/pull/502) | PR | Patch for NetBox.rc version string (open). | By @VictorVG |
| 26 | [#500](https://github.com/michaellukashov/Far-NetBox/pull/500) | PR | Open PR. | |
| 27 | [#504](https://github.com/michaellukashov/Far-NetBox/pull/504) | PR | Fix for [#390]: DateTimeToTimeStamp timestamp clamping (open). | |
| 28 | [#395](https://github.com/michaellukashov/Far-NetBox/issues/395) | Bug | Some misaligned text in localization (for example Russian). | UI layout |
| 29 | [#394](https://github.com/michaellukashov/Far-NetBox/issues/394) | Bug | Getting `"; echo "NetBox: this is end-of-file:$?"` in interactive bash shell. | SSH command artifact |
| 30 | [#387](https://github.com/michaellukashov/Far-NetBox/issues/387) | Bug | Bug in display dialogs. | UI rendering |
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
| **Immediate** | 6 | [#513], [#506], [#508], [#497], [#393], [#501] — crashes + data corruption |
| **Short-term** | 7 | [#515], [#514], [#510], [#512], [#511], [#507], [#486], [#485] — protocol/UX broken |
| **Medium-term** | 8 | [#509], [#505], [#481], [#472], [#396], [#392], [#391], [#390], [#388] — features + integration (~~#389~~ fixed) |
| **Backlog** | 6 | [#502], [#500], [#504], [#395], [#394], [#387] — PRs + minor UI |

---

## Recommended Execution Order

### Phase 1: Stabilization — Critical Crash Bugs

1. [#513](https://github.com/michaellukashov/Far-NetBox/issues/513) — FTP stack overflow crash (highest recent activity, 14 comments)
2. [#506](https://github.com/michaellukashov/Far-NetBox/issues/506) — FTP connect crash
3. [#508](https://github.com/michaellukashov/Far-NetBox/issues/508) — SFTP double-open crash
4. [#497](https://github.com/michaellukashov/Far-NetBox/issues/497) — F3 file info stack overflow
5. [#393](https://github.com/michaellukashov/Far-NetBox/issues/393) — SSH chmod crash (newly surfaced)
6. [#512](https://github.com/michaellukashov/Far-NetBox/issues/512) — IdleThread startup crash

### Phase 2: Protocol Functionality — S3 + FTP

1. [#514](https://github.com/michaellukashov/Far-NetBox/issues/514) — S3 cannot enter buckets
2. [#510](https://github.com/michaellukashov/Far-NetBox/issues/510) — Amazon S3 empty directory / time encode
3. [#507](https://github.com/michaellukashov/Far-NetBox/issues/507) — FTP directory listing hang
4. [#515](https://github.com/michaellukashov/Far-NetBox/issues/515) — F7 autocomplete directory name
5. [#485](https://github.com/michaellukashov/Far-NetBox/issues/485) — SFTP cannot create remote folder

### Phase 3: UX & Polish

1. [#511](https://github.com/michaellukashov/Far-NetBox/issues/511) — Speed limit not working
2. [#486](https://github.com/michaellukashov/Far-NetBox/issues/486) — Dialog overflow
3. [#505](https://github.com/michaellukashov/Far-NetBox/issues/505) — FTP URL file fallback
4. [#472](https://github.com/michaellukashov/Far-NetBox/issues/472) — False import prompts
5. [#395](https://github.com/michaellukashov/Far-NetBox/issues/395) — Localization text alignment
6. [#387](https://github.com/michaellukashov/Far-NetBox/issues/387) — Display dialog bug

### Phase 4: Features & Integration

1. [#509](https://github.com/michaellukashov/Far-NetBox/issues/509) — Auth certificate support
2. [#481](https://github.com/michaellukashov/Far-NetBox/issues/481) — FTP codepage fix
3. [#501](https://github.com/michaellukashov/Far-NetBox/issues/501) — Corrupted file copy (SSH/SCP)
4. [#396](https://github.com/michaellukashov/Far-NetBox/issues/396) — FTP non-default port not preserved
5. [#392](https://github.com/michaellukashov/Far-NetBox/issues/392) — Private key certificate connection
6. [#391](https://github.com/michaellukashov/Far-NetBox/issues/391) — Daylight Saving Time bug
7. [#390](https://github.com/michaellukashov/Far-NetBox/issues/390) — WebDav SSL certificate rejection
8. ~~[#389](https://github.com/michaellukashov/Far-NetBox/issues/389)~~ — ~~Pure-FTPd TLS connection failure~~ **FIXED** (`d3c3aa8`)
9. [#388](https://github.com/michaellukashov/Far-NetBox/issues/388) — RSA-SHA256 support

---

## Risk Assessment

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| Multiple stack overflow crashes suggest concurrency / recursion bugs | High | Audit threading model; check for infinite recursion in directory traversal |
| S3 protocol broken for multiple users ([#514], [#510]) | High | Isolate S3-specific issues from TLS/HTTP layer; test against AWS and MinIO |
| FTP crash reports overlap ([#513], [#506]) | Medium | Check if all share same root cause in FTP directory listing code |
| Open PRs accumulating without review ([#502], [#500], [#504]) | Medium | Review and merge or close stale PRs |
| Certificate/TLS issues spread across protocols ([#390], [#392], [#388]; ~~#389~~ fixed) | Medium | Review OpenSSL and certificate validation pipeline |

---

## Suggestions for Next Steps

Based on the current open issue landscape, here are the concrete recommendations:

1. **Merge or close stale PRs first** ([#502], [#500], [#504]). PR [#504] fixes [#390] (timestamp clamping) and appears ready for review. Closing PR debt reduces noise.

2. **Investigate the stack-overflow cluster** ([#513], [#497]). Both are `STATUS_STACK_OVERFLOW` but in different protocols (FTP and SFTP). This suggests a shared recursion pattern — likely in directory traversal or file info gathering. A single fix may resolve both.

3. **Re-test issues previously claimed as FIXED** ([#485], [#515], [#511], [#507], [#512]). These are still open on GitHub. Verify whether fixes were merged to `main` or only exist on feature branches (e.g., `lmv/dev`). If fixes are ready, close the issues.

4. **Prioritize the newly surfaced @alabuzhev batch** ([#387]-[#396]). These 10 issues were reported by a contributor in Feb 2024 and cover TLS, certificates, dialogs, DST, and FTP ports. Many appear to be quick wins:
   - [#396] (port preservation) and [#391] (DST) are likely one-line fixes.
   - [#395] (misaligned text) and [#387] (display dialogs) are UI-only.
   - ~~[#389] (Pure-FTPd TLS)~~ **FIXED** (`d3c3aa8`); and [#390] (WebDAV SSL) may be related to certificate validation logic.

5. **S3 needs dedicated attention** ([#514], [#510]). Two separate S3 issues suggest the S3 backend may have regressed after dependency updates. Consider adding debug logging to the S3 path-normalization and bucket-listing code.

6. **Create a tracking issue or milestone** for the crash bugs. With 6 crash-related issues open, users perceive instability. A milestone focused on "Crash & Stability" would communicate progress.

---

## Change Log

| Date | Change |
|------|--------|
| 2026-04-25 | Full re-analysis of last 100 issues; expanded from 10 to all open issues; added links; marked [#485] as fixed, [#432] as closed |
| 2026-04-30 | Re-analyzed all **28 currently open issues** (previous count was 20 within last-100 window). Added 10 newly visible issues ([#387]-[#396]). Updated status of issues previously marked FIXED that remain open on GitHub ([[#485]], [[#515]), [[#511]], [[#507]], [[#512]]). Added Suggestions for Next Steps section. |
