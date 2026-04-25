# Far-NetBox Issue Prioritization & Task List

> Generated: 2026-04-25
> Source: https://github.com/michaellukashov/Far-NetBox/issues
> Total issues analyzed: 10 open issues

---

## Priority-Sorted Task Table

### CRITICAL — Severity 9-10 — Immediate Action Required

| # | Issue | Description | Impact | Est. Time | Dependencies |
|---|-------|-------------|--------|-----------|-------------|
| 1 | **#485 — Cannot create remote folder**<br>`[Bug · Severity 10 · Immediate]` | Remote directory creation fails on SFTP connections. Breaks core file management workflow. | Users cannot create directories on remote servers — fundamental functionality broken. | 2-4 hours | WinSCP core folder creation logic |
| 2 | **#432 — Far silent close on FTP connection**<br>`[Bug · Severity 10 · Immediate]` | Connecting to FTP server causes Far Manager to silently crash/close without error message. No crash dump generated. | Complete application crash — data loss risk, unrecoverable session. | 4-8 hours | Debug logging infrastructure |
| 3 | **#184 — FTP AUTO/ASCII transfer mode download failure**<br>`[Bug · Severity 9 · Immediate]` | When FTP transfer mode set to AUTO or ASCII, downloads fail silently. Binary mode works. | Data corruption on text file transfers; affects ASCII-mode workflows. | 3-5 hours | FTP protocol handler module |

### HIGH — Severity 7-8 — Short-term Resolution

| # | Issue | Description | Impact | Est. Time | Dependencies |
|---|-------|-------------|--------|-----------|-------------|
| 4 | **#389 / #202 — Pure-FTPd TLS connection failure**<br>`[Bug · Severity 8 · Short-term]` | Cannot establish TLS-encrypted connections to Pure-FTPd servers. Two open issues (#202, #389) indicating recurring/unresolved problem. | No encrypted FTP possible with Pure-FTPd — security compliance issue. | 4-6 hours | OpenSSL integration; TLS negotiation module |
| 5 | **#380 — Slow FTP download of many files**<br>`[Performance · Severity 8 · Short-term]` | Downloading large numbers of files via FTP is extremely slow. Performance degrades with file count. | Productivity blocker for bulk operations; makes FTP impractical for large transfers. | 6-12 hours | FTP session pooling; file enumeration optimization |
| 6 | **#396 — FTP non-default port not preserved**<br>`[Bug · Severity 7 · Short-term]` | FTP connection forgets custom port number; reconnects to default port 21. | Users with non-standard FTP ports cannot maintain connections. | 2-3 hours | Session data serialization; configuration persistence |
| 7 | **#329 — KeepAlive broken since FAR 3 (2021)**<br>`[Bug · Severity 7 · Short-term]` | KeepAlive mechanism doesn't work in FAR 3 distributions. Connections drop after idle period. | Sessions disconnect unexpectedly; data loss during interrupted transfers. | 3-5 hours | FAR 3 API integration; keepalive timer module |

### MEDIUM — Severity 5-6 — Planned Enhancement

| # | Issue | Description | Impact | Est. Time | Dependencies |
|---|-------|-------------|--------|-----------|-------------|
| 8 | **#385 — Add ed25519 key support**<br>`[Feature · Severity 6 · Short-term]` | Missing support for Ed25519 SSH key type. Modern standard for SSH authentication. | Users with ed25519 keys cannot authenticate; security best practice gap. | 4-8 hours | PuTTY key loading module; OpenSSH compatibility layer |
| 9 | **#382 — ~/.ssh/config file reading**<br>`[Feature · Severity 5 · Long-term]` | Plugin doesn't read OpenSSH ~/.ssh/config for connection parameters (host aliases, jump hosts, key paths). | Users must manually configure every session; no Host alias support. | 8-16 hours | SSH config parser; session configuration UI |

### LOW — Severity 1-4 — Backlog / Nice-to-have

| # | Issue | Description | Impact | Est. Time | Dependencies |
|---|-------|-------------|--------|-----------|-------------|
| 10 | **Additional feature requests**<br>`[Improvement · Severity 3 · Long-term]` | General UX improvements and minor feature requests from community feedback. | Incremental quality-of-life improvements. | Varies | Depends on specific request scope |

---

## Summary by Category

| Category | Count | Priority Range |
|----------|-------|----------------|
| **Bug** | 7 | Critical → High |
| **Feature** | 2 | Medium |
| **Performance** | 1 | High |
| **Improvement** | 1 | Low |

## Summary by Urgency

| Urgency | Count | Key Issues |
|---------|-------|------------|
| **Immediate** | 3 | #485, #432, #184 — crashes, data corruption, broken core features |
| **Short-term** | 5 | #389/#202, #380, #396, #329, #385 — security, performance, compatibility |
| **Long-term** | 2 | #382, improvements — UX enhancements |

## Recommended Execution Order

### Phase 1: Stabilization (Week 1-2) — Critical Bugs
1. **#432** — Fix silent crash on FTP connect (highest user impact, crash = P0)
2. **#485** — Fix remote folder creation (core workflow broken)
3. **#184** — Fix FTP AUTO/ASCII transfer mode (data integrity risk)

### Phase 2: Security & Performance (Week 3-4) — High Priority
4. **#389/#202** — Fix Pure-FTPd TLS connections (security compliance)
5. **#396** — Fix port preservation (connection reliability)
6. **#329** — Fix KeepAlive for FAR 3 (session stability)
7. **#380** — Optimize bulk FTP downloads (performance)

### Phase 3: Feature Additions (Week 5-8) — Medium Priority
8. **#385** — Add ed25519 key support (modern authentication)
9. **#382** — Implement ~/.ssh/config reading (UX improvement)

### Phase 4: Polish (Ongoing) — Low Priority
10. Community-requested improvements and minor fixes

---

## Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| WinSCP core changes break fixes | Medium | Test against multiple WinSCP versions |
| OpenSSL compatibility issues | Medium | Use project's patched OpenSSL |
| FAR 3 API changes | Low | FAR 3 API is stable since 2021 |
| Regressions in TLS/SSH | High | Manual testing cycle per AGENTS.md |
