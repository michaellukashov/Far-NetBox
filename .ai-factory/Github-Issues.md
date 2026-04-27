# Far-NetBox Issue Prioritization & Task List

> Generated: 2026-04-25 (updated)
> Source: <https://github.com/michaellukashov/Far-NetBox/issues>
> Total issues analyzed: **100** (last 100 by creation date)

---

## Stats Summary

| Metric | Count |
|--------|-------|
| Total analyzed | **100** |
| Open | **20** |
| Closed | **80** |
| Pull Requests | **60** (58 closed, 2 open: [#504](https://github.com/michaellukashov/Far-NetBox/pull/504), [#500](https://github.com/michaellukashov/Far-NetBox/pull/500)) |
| Non-PR Issues | **40** (18 open, 22 closed) |

### Issue Category Breakdown (non-PR only: 40)

| Category | Count | Description |
|----------|-------|-------------|
| **Bug** | 23 | Crashes, errors, not working, data corruption |
| **Integration** | 11 | S3, FTP listings, TLS, certs, codepage, file copy |
| **Feature** | 3 | New capabilities, enhancements |
| **Performance** | 3 | Speed, hangs, idle thread |

---

## Priority-Sorted Task Table (Open Issues)

### CRITICAL — Severity 9-10 — Immediate Action Required

| # | Issue | Type | Summary | Impact | Est. Time |
|---|-------|------|---------|--------|-----------|
| 1 | [#432](https://github.com/michaellukashov/Far-NetBox/issues/432) | Bug | CLOSED — Fixed by PR [#433](https://github.com/michaellukashov/Far-NetBox/pull/433). Far silent close on FTP connection. | Resolved. | — |
| 2 | [#513](https://github.com/michaellukashov/Far-NetBox/issues/513) | Bug | FTP hangs then crashes with `STATUS_STACK_OVERFLOW` on BusyBox FTP (cameras). 14 comments. | Crash + data loss risk. | 4-8h |
| 3 | [#506](https://github.com/michaellukashov/Far-NetBox/issues/506) | Bug | Far crashes on FTP connect to specific servers; scans all directories before crash. | Complete app crash.
| 4 | [#508](https://github.com/michaellukashov/Far-NetBox/issues/508) | Bug | Far crashes after 2nd file open via SFTP unless Ctrl+R refresh between opens. | Workflow-breaking crash. | FIXED: 26.04.2026
| 5 | [#497](https://github.com/michaellukashov/Far-NetBox/issues/497) | `STATUS_STACK_OVERFLOW` on F3 (file info) via SFTP to certain servers. | Crash on info view. | FIXED: 26.04.2026
| 6 | [#501](https://github.com/michaellukashov/Far-NetBox/issues/501) | Bug | Slow SSH/SCP copy + corrupted files on certain SSH servers. | Data corruption risk. | 4-6h |

### HIGH — Severity 7-8 — Short-term Resolution

| 7 | [#515](https://github.com/michaellukashov/Far-NetBox/issues/515) | Bug | F7 directory creation ignores autocomplete text; creates partial name. | Broken directory creation UX. | FIXED: 27.04.2026
| 8 | [#514](https://github.com/michaellukashov/Far-NetBox/issues/514) | Bug | S3: lists buckets but cannot enter any bucket. | S3 protocol unusable.
| 9 | [#510](https://github.com/michaellukashov/Far-NetBox/issues/510) | Bug | Amazon S3 connects but shows empty directory; can't upload; time encode error. | S3 unusable on AWS.
| 10 | [#512](https://github.com/michaellukashov/Far-NetBox/issues/512) | Bug | IdleThread starts too early; crashes on plugin unload (`EXCEPTION_ACCESS_VIOLATION`). By contributor @alabuzhev. | Crash on plugin load/unload. | FIXED
| 11 | [#511](https://github.com/michaellukashov/Far-NetBox/issues/511) | Perf | Download speed limit in transfer dialog has no effect (SSH). | Cannot throttle transfers. | FIXED: 27.04.2026
| 12 | [#507](https://github.com/michaellukashov/Far-NetBox/issues/507) | Bug | FTP directory listing hangs; treats every file as directory (vsftpd). | Impossibly slow listing.
| 13 | [#486](https://github.com/michaellukashov/Far-NetBox/issues/486) | Bug | Dialog size overflow with proxy+tunnel settings in key exchange panel. | UI corruption.

### MEDIUM — Severity 5-6 — Planned Enhancement

| # | Issue | Type | Summary | Impact | Est. Time |
|---|-------|------|---------|--------|-----------|
| 14 | [#509](https://github.com/michaellukashov/Far-NetBox/issues/509) | Feature | Support user-provided auth certificates (PuTTY cert auth). 19 comments. | Missing auth method. | 4-8h |
| 15 | [#505](https://github.com/michaellukashov/Far-NetBox/issues/505) | Feature | Fall back to opening FTP URL as file if directory access fails. | UX improvement. | 3-5h |
| 16 | [#481](https://github.com/michaellukashov/Far-NetBox/issues/481) | Bug | FTP codepage ISO-8859-5 copy fails (filename encoding). | Broken non-UTF8 FTP. | 3-5h |
| 17 | [#472](https://github.com/michaellukashov/Far-NetBox/issues/472) | Bug | False positive session import prompts on every start. | Annoyance. | 2-3h |
| 18 | [#443](https://github.com/michaellukashov/Far-NetBox/issues/443) | Feature | Various UX improvements. | Quality-of-life. | Varies |

### LOW — Severity 1-4 — Backlog / Nice-to-have

| # | Issue | Type | Summary |
|---|-------|------|---------|
| 19 | [#502](https://github.com/michaellukashov/Far-NetBox/pull/502) | PR | Patch for NetBox.rc version string (open). |
| 20 | [#500](https://github.com/michaellukashov/Far-NetBox/pull/500) | PR | Open PR. |
| 21 | [#504](https://github.com/michaellukashov/Far-NetBox/pull/504) | PR | Fix for [#390]: DateTimeToTimeStamp timestamp clamping (open). |

### FIXED (in this analysis batch)

| # | Fix | Status |
|---|-----|--------|
| [#485](https://github.com/michaellukashov/Far-NetBox/issues/485) | SFTP "Cannot create remote folder" | FIXED — `lmv/dev` branch, `SftpFileSystem.cpp:3147-3173` |
| [#432](https://github.com/michaellukashov/Far-NetBox/issues/432) | Far silent close on FTP connect | CLOSED — Fixed by PR [#433](https://github.com/michaellukashov/Far-NetBox/pull/433) |
| [#390](https://github.com/michaellukashov/Far-NetBox/issues/390) | Invalid argument to time encode | PR open — [#504](https://github.com/michaellukashov/Far-NetBox/pull/504) |
| [#503](https://github.com/michaellukashov/Far-NetBox/issues/503) | FTPS implicit doesn't work | CLOSED |

### OLDER ISSUES (not in last 100, from original PLAN-Issues.md)

These issues are older than the 100-issue window and need re-verification:

| # | Title | Status |
|---|-------|--------|
| [#184](https://github.com/michaellukashov/Far-NetBox/issues/184) | FTP AUTO/ASCII transfer mode download failure | Unverified |
| [#202](https://github.com/michaellukashov/Far-NetBox/issues/202) | Pure-FTPd TLS connection failure | Unverified |
| [#329](https://github.com/michaellukashov/Far-NetBox/issues/329) | KeepAlive broken since FAR 3 (2021) | Unverified |
| [#380](https://github.com/michaellukashov/Far-NetBox/issues/380) | Slow FTP download of many files | Unverified |
| [#382](https://github.com/michaellukashov/Far-NetBox/issues/382) | ~/.ssh/config file reading | Unverified |
| [#385](https://github.com/michaellukashov/Far-NetBox/issues/385) | Add ed25519 key support | Unverified |
| [#389](https://github.com/michaellukashov/Far-NetBox/issues/389) | Pure-FTPd TLS connection failure | Unverified |
| [#396](https://github.com/michaellukashov/Far-NetBox/issues/396) | FTP non-default port not preserved | Unverified |

---

## Summary by Category

| Category | Count | Priority Range |
|----------|-------|----------------|
| **Bug** | 23 (of 40 non-PR) | Critical → High |
| **Integration** | 11 | High → Medium |
| **Performance** | 3 | High |
| **Feature** | 3 | Medium → Low |

## Summary by Urgency

| Urgency | Count | Key Issues |
|---------|-------|------------|
| **Immediate** | 6 | #513, #506, #508, #497, #501 — crashes + data corruption |
| **Short-term** | 7 | #515, #514, #510, #512, #511, #507, #486 |
| **Medium-term** | 5 | #509, #505, #481, #472, #443 |
| **Backlog** | 3 (open PRs) | #502, #500, #504 |

## Recommended Execution Order

### Phase 1: Stabilization — Critical Crash Bugs

1. [#513](https://github.com/michaellukashov/Far-NetBox/issues/513) — FTP stack overflow crash (highest recent activity, 14 comments)
2. [#506](https://github.com/michaellukashov/Far-NetBox/issues/506) — FTP connect crash
3. [#508](https://github.com/michaellukashov/Far-NetBox/issues/508) — SFTP double-open crash
4. [#497](https://github.com/michaellukashov/Far-NetBox/issues/497) — F3 file info stack overflow
5. [#512](https://github.com/michaellukashov/Far-NetBox/issues/512) — IdleThread startup crash (simple fix)

### Phase 2: Protocol Functionality — S3 + FTP

1. [#514](https://github.com/michaellukashov/Far-NetBox/issues/514) / [#510](https://github.com/michaellukashov/Far-NetBox/issues/510) — S3 bucket access (combine investigation)
2. [#507](https://github.com/michaellukashov/Far-NetBox/issues/507) — FTP directory listing hang
3. [#515](https://github.com/michaellukashov/Far-NetBox/issues/515) — F7 autocomplete directory name

### Phase 3: UX & Polish

1. [#511](https://github.com/michaellukashov/Far-NetBox/issues/511) — Speed limit not working
2. [#486](https://github.com/michaellukashov/Far-NetBox/issues/486) — Dialog overflow
3. [#505](https://github.com/michaellukashov/Far-NetBox/issues/505) — FTP URL file fallback
4. [#472](https://github.com/michaellukashov/Far-NetBox/issues/472) — False import prompts

### Phase 4: Features & Integration

1. [#509](https://github.com/michaellukashov/Far-NetBox/issues/509) — Auth certificate support
2. [#481](https://github.com/michaellukashov/Far-NetBox/issues/481) — FTP codepage fix
3. [#501](https://github.com/michaellukashov/Far-NetBox/issues/501) — Corrupted file copy

---

## Risk Assessment

| Risk | Likelihood | Mitigation |
|-------|------------|------------|
| Multiple stack overflow crashes suggest concurrency bugs | High | Audit threading model; IdleThread fix may help |
| S3 protocol broken for multiple users | High | Isolate S3-specific issues from TLS/HTTP layer |
| FTP crash reports overlap (#513, #506, #432) | Medium | Check if all share same root cause |
| Open PRs accumulating without review | Medium | Review and merge or close stale PRs |

---

## Change Log

| Date | Change |
|------|--------|
| 2026-04-25 | Full re-analysis of last 100 issues; expanded from 10 to all open issues; added links; marked #485 as fixed, #432 as closed |
