# Deduplication Report

**Date:** 2026-04-26
**Scope:** `.ai-factory/` and `docs/` directories (all subdirectories)

## Summary

| Metric | Session 1 | Session 2 | Total |
|--------|----------|----------|-------|
| Documents reviewed | 105 | 96 | 96 (final) |
| Exact duplicates found | 1 pair (2 files) | 0 | 2 |
| Superseded iterations removed | 7 files | — | 7 |
| Redundant summaries/reports removed | 3 files | — | 3 |
| Obsolete artifacts removed | 9 files | — | 9 |
| Stale/internal metadata removed | — | 12 files | 12 |
| Broken references fixed | — | 3 files | 3 |
| **Total files removed** | **20** | **12** | **32** |
| **Final document count** | **85** | **74** | **74** |

## Removal Details

### Exact Duplicates (1 pair)

| Removed | Kept | Reason |
|---------|------|--------|
| `PLAN.md` | `plans/fix-farplugin-assert-shutdown.md` | Identical content (same MD5 hash, 65 lines, 385 words) |

### Superseded Iterations (7 files)

| Removed | Kept | Similarity | Reason |
|---------|------|-----------|--------|
| `plans/fix-ftp-port-preservation.md` (8KB) | `plans/fix-ftp-port-preservation-improved.md` (18KB) | 49.7% | Original superseded by improved |
| `plans/fix-ftp-port-preservation-reviewed.md` (14KB) | `plans/fix-ftp-port-preservation-improved.md` (18KB) | 88.1% | Reviewed superseded by improved |
| `plans/folder-history-navigation.md` (9KB) | `plans/folder-history-navigation-v2.md` (11KB) | 70.7% | Original superseded by v2 |
| `plans/kitty-keyboard-protocol-implementation.md` (11KB) | `plans/kitty-keyboard-protocol-implementation-v2.md` (19KB) | 54.5% | Original superseded by v2 |
| `plans/kitty-keyboard-protocol-implementation-aif.md` (11KB) | `plans/kitty-keyboard-protocol-implementation-v2.md` (19KB) | 20.8% | Intermediate iteration, superseded by v2 |
| `plans/gui-certificate-editor-webdav-s3.md` (6KB) | `plans/gui-certificate-editor-webdav-s3-improved.md` (10KB) | 9.8% | Original superseded by improved |
| `plans/winxp-compatibility-verification-vs2022.md` (10KB) | `plans/winxp-compatibility-verification-vs2022-improved.md` (13KB) | 84.3% | Original superseded by improved |

### Redundant Summaries/Reports (3 files)

| Removed | Redundant With | Reason |
|---------|---------------|--------|
| `plans/logging-thread-safety-summary.md` | `plans/logging-thread-safety.md` (26KB) | Summary redundant with full plan |
| `plans/implementation-summary.md` | `plans/logging-thread-safety-implementation.md` | Summary redundant with implementation report |
| `plans/final-verification-report.md` | `plans/logging-thread-safety-verification.md` | Verification report duplicate |

### Obsolete Artifacts (9 files)

| Removed | Reason |
|---------|--------|
| `QUICK_START.md` | Obsolete prompt package artifact (Pascal-to-C++ port session) |
| `VISUAL_SUMMARY.md` | Obsolete prompt package artifact |
| `PROMPT_PACKAGE_README.md` | Obsolete prompt package artifact |
| `PROMPT_SUMMARY.md` | Obsolete prompt package artifact |
| `IMPROVED_PROMPT.md` | Obsolete prompt package artifact |
| `PROMPT_REVIEW.md` | Obsolete prompt package artifact |
| `PLAN_REFINEMENT_REPORT.md` | Obsolete prompt package artifact |
| `INDEX.md` (root) | Root index superseded by `plans/INDEX.md` |
| `plans/pascal-rtl-implementation-2026-04-23-v2.md` | Obsolete Pascal-to-C++ port plan |

### Session 2: Stale/Internal Metadata Removal (12 files)

| Removed | Reason |
|---------|--------|
| `evolutions/2026-04-16-08.00.md` | AI evolution log — internal metadata, not project documentation |
| `evolutions/2026-04-20-12.00.md` | AI evolution log — internal metadata, not project documentation |
| `evolutions/2026-04-20-13.20.md` | AI evolution log — internal metadata, not project documentation |
| `patches/2026-04-18-12.00.md` | Superseded by `MASTER-PASSWORD-IMPLEMENTATION.md` |
| `patches/2026-04-20-13.00.md` | One-time GitHub Actions fix — no longer relevant |
| `storage/archive/26-04-13-upgrade-openssl-6.5.6-analysis.md` | Superseded by `openssl-sync.md` and `openssl-sync-cleanup.md` plans |
| `qa/dev-38f8e886/change-summary.md` | QA artifact for old branch — specific to stale commit |
| `qa/dev-38f8e886/test-cases.md` | QA artifact for old branch — specific to stale commit |
| `qa/dev-38f8e886/test-plan.md` | QA artifact for old branch — specific to stale commit |
| `qa/feature-remote-to-remote-copy-f9761995/change-summary.md` | QA artifact for old branch — specific to stale commit |
| `RESEARCH.md` | Nearly empty stub (474B) — just date and heading, no actual research content |
| `plans/prepare-environment-for-winxp-build.md` | Empty YAML frontmatter only (411B) — no actual plan content |

### Cross-Directory Analysis (docs/ vs .ai-factory/)

All files in `docs/` were compared with `.ai-factory/` files. No duplicates found — content similarity <10% in all cases.
The two directories serve different audiences:

| Directory | Audience | Purpose |
|-----------|----------|---------|
| `docs/` | Human developers | User-facing documentation, feature specs, i18n |
| `.ai-factory/` | AI agents | Implementation plans, references, agent rules, skill context |

### Broken References Fixed

| File | Fix |
|------|-----|
| `plans/INDEX.md` | Removed references to deleted `final-verification-report.md` and `implementation-summary.md` |
| `rules/base.md` | Replaced specific `PLAN.md` references with generic "plan file" references |

### Empty Directories Removed

| Directory | Reason |
|-----------|--------|
| `evolutions/` | All contents removed |
| `patches/` | All contents removed |
| `qa/dev-38f8e886/` | All contents removed |
| `qa/feature-remote-to-remote-copy-f9761995/` | All contents removed |
| `storage/archive/` | All contents removed |

### Files Reviewed But Kept (different content despite similar names)

| Files | Similarity | Reason Kept |
|-------|-----------|-------------|
| `plans/webdav-overwrite-fix.md` + `plans/webdav-overwrite-fix-plan-2026-04-22.md` | 14.0% | Different documents |
| `plans/openssl-sync.md` + `plans/openssl-sync-cleanup.md` | 5.4% | Different documents |
| `plans/win32-kitty-input-mode.md` + `plans/win32-kitty-input-mode-verified.md` | 2.3% | Plan vs verification report |
| `plans/winxp-compatibility-verification.md` + `plans/winxp-compatibility-verification-vs2022-improved.md` | 1.8% | Different plans |
| `plans/logging-thread-safety*.md` (4 files) | 2-6% | Different document types (plan, verify, implementation) |
| `docs/*.md` vs `.ai-factory/*.md` (7 pairs) | 0.7-9.3% | Different audiences (human vs AI agent) |
| `skill-context/*.md` vs `RULES.md` | 1.4-5.8% | Skill-specific rules, not redundant |

## Final File Inventory

| Category | Count |
|----------|-------|
| Root documents | 16 |
| Plans | 40 |
| References | 7 |
| Analysis | 2 |
| Rules | 1 |
| Skill context | 7 |
| **docs/** (human-facing) | 10 |
| **Total** | **83** |

## Methodology

1. **Exact duplicate detection** — MD5 hash comparison across all files
2. **Near-duplicate detection** — SequenceMatcher similarity scoring on full file content
3. **Cross-directory comparison** — Files with same/similar names in `docs/` vs `.ai-factory/`
4. **Staleness detection** — Size analysis, completion status, supersession indicators
5. **Broken reference scanning** — Search for references to removed files in remaining documents
