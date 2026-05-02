# GitHub Issues Execution Plan

> Generated: 2026-05-02
> Source: `.ai-factory/Github-Issues.md`
> Total open issues: 28

---

## Current Situation

As of 2026-05-02, a large batch of critical and high issues have been marked **FIXED** in the tracking document but some remain open on GitHub or lack confirmation of merge to `main`. The S3 protocol is completely broken for multiple users. This plan prioritizes verification of crash fixes and restoration of S3 functionality above all else.

---

## Phase 1: Stabilization — Verify Phantom Fixed Crash Bugs (Immediate)

Several critical crashes are marked FIXED in the tracker but are **still open on GitHub** or lack a 2026-05-02 changelog entry confirming merge to `main`. These represent the highest user-perceived instability and must be verified first.

| Priority | Issue | Severity | Action Required |
|----------|-------|----------|-----------------|
| 1 | #513 | Critical | Re-test FTP `STATUS_STACK_OVERFLOW` on BusyBox. Likely shares recursion root cause with #497. |
| 2 | #497 | Critical | Re-test SFTP `STATUS_STACK_OVERFLOW` on F3 file info. If root cause is shared with #513, a single traversal recursion fix may close both. |
| 3 | #512 | Critical/High | Confirm IdleThread startup crash fix is on `main`, not just `lmv/dev`. No changelog entry on 2026-05-02. |
| 4 | #393 | Critical | Confirm SSH chmod crash fix is merged to `main`. No changelog entry. |
| 5 | #506 | Critical | Re-test FTP connect crash (directory scan before crash). Overlaps with #513 traversal pattern. |
| 6 | #508 | Critical | Re-test SFTP double-open crash. Workflow-breaking; no confirmed merge noted. |

### Phase 1 Exit Criteria

- [ ] All six issues above are confirmed fixed on `main` branch.
- [ ] If fixed only on feature branches, merge to `main` immediately.
- [ ] Close GitHub issues once verified.

---

## Phase 2: Protocol Functionality — Fix S3 Regression (Short-term)

S3 is completely unusable for AWS and bucket access. These issues have **no fix changelog entries** and are confirmed open.

| Priority | Issue | Impact |
|----------|-------|--------|
| 1 | #514 | Cannot enter any S3 bucket after listing them. |
| 2 | #510 | AWS S3 shows empty directory; time-encode error blocks upload. |

### Phase 2 Exit Criteria

- [ ] S3 path-normalization and bucket-listing logic audited.
- [ ] Debug logging added to S3 entry path.
- [ ] Both issues verified against AWS and MinIO.

---

## Phase 3: Quick Wins — @alabuzhev Batch (Medium-term)

Low-effort, high-polish fixes reported by @alabuzhev. Many appear to be one-line or UI-only changes.

| Priority | Issue | Estimate | Notes |
|----------|-------|----------|-------|
| 1 | #396 | ~1h | FTP non-default port not preserved (likely one-line fix). |
| 2 | #391 | ~1h | Daylight Saving Time bug (likely one-line fix). |
| 3 | #395 | ~2h | Misaligned Russian localization text (UI-only). |
| 4 | #387 | ~2h | Display dialog rendering bug (UI-only). |
| 5 | #472 | 2-3h | False positive session import prompts on every start. |

---

## Phase 4: Merge Debt & Remaining Medium Issues (Backlog)

| Priority | Issue | Action |
|----------|-------|--------|
| 1 | PR #502 | Merge or close stale PR (NetBox.rc version string). |
| 2 | PR #500 | Merge or close stale PR. |
| 3 | #481 | Investigate FTP codepage ISO-8859-5 filename encoding failure. |
| 4 | #392 | Private key certificate connection failure (may relate to TLS pipeline). |
| 5 | #394 | Fix SSH command artifact (`; echo "NetBox: this is end-of-file:$?"`) in interactive bash. |
| 6 | Discovered | Add UI control to clear stale `.ppk` paths for `FTlsCertificateFile` (`WinSCPDialogs.cpp:4040` TODO). |

---

## Risk Assessment

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| Multiple stack-overflow crashes share a recursion root cause | High | Audit directory traversal and file-info gathering for infinite recursion. |
| S3 backend regressed after dependency updates | High | Isolate S3 path logic from TLS/HTTP layer; test against real AWS and MinIO. |
| Fixes exist only on `lmv/dev` or other feature branches | Medium | Enforce verification on `main` before marking FIXED in tracker. |
| Open PRs accumulate without review | Medium | Close or merge PR #502 and #500 this week. |

---

## Summary

1. **Verify crash fixes first.** The stack-overflow cluster (#513 + #497) is the top user pain point.
2. **Restore S3.** Two separate S3 failures suggest a backend regression.
3. **Sweep quick wins.** The @alabuzhev batch (#396, #391, #395, #387) is low-hanging fruit.
4. **Clear merge debt.** Stale PRs create noise and confusion.

