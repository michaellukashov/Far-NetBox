# 🎯 IMPLEMENTATION PLAN: Pascal-to-C++ Port (Refined v2)

**Project:** NetBox - FormatFloat, TFormatSettings, TTimeSpan  
**Method:** Prompt-driven Claude Code execution  
**Scope:** 4 files only (Sysutils.cpp/hpp, Classes.cpp/hpp)  
**Status:** Ready for execution  
**Confidence:** 95/100  
**Created:** 2026-04-23  
**Refined:** 2026-04-23 (aif-improve v2)  
**Plan ID:** pascal-rtl-implementation-v2  
**Git Branch:** feat/pascal-rtl-conversion (recommended)

---

## 📋 Executive Summary

Implement three critical RTL components from FreePascal to C++17:
- **FormatFloat** - Full float formatting with format string support
- **TFormatSettings** - Complete locale-aware formatting settings class
- **TTimeSpan** - Full time span arithmetic (22 methods + constructors)

**Expected Output:** ~650 lines of production C++ code, zero warnings  
**Expected Duration:** ~4 hours (monitored execution)  
**Success Criteria:** 10-item binary checklist (see Phase 6)

**Prerequisites:**
- FreePascal reference source accessible
- NetBox project structure understood
- Claude Code agent ready
- ~4 hours available for execution
- MSVC 2022, CMake 3.15+, build-x64.bat working

---

## 📦 Deliverables

### Primary
- ✅ IMPROVED_PROMPT.md (14.8 KB) - Copy into Claude Code

### Supporting (already generated)
- INDEX.md - Navigation hub
- QUICK_START.md - 5-minute usage guide
- VISUAL_SUMMARY.md - Quality transformation overview
- PROMPT_PACKAGE_README.md - Package overview
- PROMPT_SUMMARY.md - Executive improvements
- PROMPT_REVIEW.md - Full audit (24 findings)
- PLAN_REFINEMENT_REPORT.md - This refinement analysis

**Location:** `D:\Projects\NetBox\NetBox-other\`

---

## 🔄 Execution Overview (Enhanced)

```
Phase 0: Git Setup & Validation (15 min)
  ├─ Create feature branch
  ├─ Validate build environment
  └─ Establish rollback point

Phase 1: Preparation & Understanding (5-15 min)
  ├─ Review documentation package
  ├─ Verify pre-flight checklist (enhanced)
  └─ Prepare Claude Code (with validation)

Phase 2: Claude Code Execution (3.5-4 hours)
  ├─ Paste prompt → Enter
  ├─ Monitor 4 checkpoints (with failure recovery)
  └─ Wait for completion signal

Phase 3: Verification (30-45 min)
  ├─ Build verification (with logging)
  ├─ Runtime integration check (precise grep)
  ├─ Pre-commit quality check (NEW)
  └─ Completion checklist validation

Phase 4: Finalization (15 min)
  ├─ Git commit (with template)
  └─ Expanded post-implementation notes

Total Time:** ~4.5-5 hours (including safety gates)
```

---

## 🌿 Phase 0: Git Setup & Environment Validation

**Duration:** 10-15 minutes  
**Status:** Must complete before Phase 1

### Task 0.1: Create Git Feature Branch

**Details:**
```bash
# Ensure on main branch with latest
git checkout main
git pull origin main

# Create feature branch
git checkout -b feat/pascal-rtl-conversion
```

**Success:** Branch created, working on `feat/pascal-rtl-conversion`

**Why:** Isolate implementation, enable easy rollback, follow git workflow

---

### Task 0.2: Validate Build Environment

**Details:**
```bash
# Check MSVC compiler
cl.exe /Bv 2>&1 | findstr "Version"
# Expected: Microsoft (R) C/C++ Optimizing Compiler Version 19.xx

# Check CMake
cmake --version
# Expected: cmake version 3.15+

# Verify build script exists
if exist build-x64.bat (
  echo Build script found
) else (
  echo ERROR: build-x64.bat not found
  exit /b 1
)

# Optional: Test build script help
cmd /c build-x64.bat /? >nul 2>&1 && echo Build script accessible || echo Build script may have issues
```

**Success:** All tools present and accessible

**Why:** Prevents mid-implementation discovery of missing toolchain

---

### Task 0.3: Establish Rollback Point

**Details:**
```bash
# Stage current clean state
git add -A
git commit -m "chore: establish rollback point before Pascal RTL implementation" --allow-empty

# Create backup tag
set TIMESTAMP=%date:~-4%%date:~4,2%%date:~7,2%-%time:~0,2%%time:~3,2%
git tag backup/pre-implementation-%TIMESTAMP%
```

**Success:** Tag created with format `backup/pre-implementation-YYYYMMDD-HHMM`

**Rollback procedure (if needed):**
```bash
git checkout main
git reset --hard backup/pre-implementation-*
git branch -D feat/pascal-rtl-conversion
```

**Why:** Clean restore point before any code changes

---

## 📋 Phase 1: Preparation & Understanding (Enhanced)

**Duration:** 15-25 minutes  
**Status:** Ready to start

### Task 1.1: Review Documentation Package

**Details:** Read (at minimum):
- QUICK_START.md (5 min) - Essential usage instructions
- VISUAL_SUMMARY.md (10 min) - Key improvements and quality metrics
- IMPROVED_PROMPT.md (15 min) - Full prompt content

**Success:** Can describe:
- Why this prompt is production-ready (90-95% failure rate reduction)
- The 4 checkpoints and their expected outputs
- The 10-item completion checklist
- The 6-step sequential workflow
- Error handling policy (NaN, overflow, LCID)
- Thread safety requirements
- CRLF line ending requirement

**Output:** Mental model of the execution process

**Dependencies:** None (first task)

---

### Task 1.2: Verify Enhanced Pre-Flight Checklist

**Details:** Confirm all items:

- [ ] FreePascal source exists at `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\`
- [ ] Verify Pascal reference files exist and are readable:
  - `fmtflt.inc` (readable, ~409 lines)
  - `sysinth.inc` (readable, TFormatSettings def)
  - `dati.inc` (readable, date/time utils)
  - `datih.inc` (readable, date/time headers)
- [ ] NetBox project at `D:\Projects\NetBox\NetBox-other\`
- [ ] `AGENTS-Standards.md` exists in project root
- [ ] `build-x64.bat` exists and works (verified in Phase 0)
- [ ] Git working directory clean (or acceptable changes)
- [ ] ~4.5 hours available (or pausing capability)
- [ ] Claude Code installed and ready
- [ ] Feature branch created (`git branch --show-current` == `feat/pascal-rtl-conversion`)
- [ ] Rollback tag created (`git tag -l "backup/pre-implementation-*"` shows one)

**If any fails:** Resolve before proceeding or adjust plan

**Dependencies:** Task 0.1, 0.2, 0.3 (Git setup must complete first)

---

### Task 1.3: Prepare Claude Code (With Validation)

**Details:**
1. Open Claude Code
2. Set working directory to NetBox project root
3. Copy entire contents of IMPROVED_PROMPT.md to clipboard
4. Validate: Open IMPROVED_PROMPT.md, confirm:
   - Header says "IMPROVED_PROMPT" or similar
   - Contains 6-step workflow section
   - Contains 4 checkpoints
   - Contains 10-item completion checklist
5. Position cursor in input field

**Success:** Claude Code ready, prompt validated

**Dependencies:** Task 1.2 (pre-flight must pass)

---

## 🤖 Phase 2: Claude Code Execution (Enhanced Monitoring)

**Duration:** ~4 hours  
**Status:** Waiting for execution

### Task 2.1: Execute Prompt (With Pre-emptive Guidance)

**Action:**
1. Paste IMPROVED_PROMPT.md content into Claude Code
2. Press Enter to start execution
3. If agent asks for guidance, respond: "Please follow the 6-step workflow in the prompt. Begin with Step 1: Read the Pascal reference files."
4. Monitor output continuously

**Expected Behavior:**
- Agent will follow 6-step sequential workflow
- Agent will output checkpoints at precise moments
- Agent will NOT modify call sites (verified in Phase 3)
- Agent will build and verify zero warnings

**Monitoring Requirements:**
- Stay present (or check every 15-20 minutes)
- Watch for checkpoint signals
- Do NOT interrupt unless stalled >10 minutes on a checkpoint

**Dependencies:** Task 1.3 (Claude Code prepared)

---

### Task 2.2: Monitor Checkpoints (With Failure Recovery)

**Critical Checkpoints (must appear in order):**

| # | Checkpoint | Expected Output | Duration | Verification |
|---|------------|----------------|----------|--------------|
| 1 | REFERENCES_LOADED | `[CHECKPOINT] REFERENCES_LOADED` | ~30 min | Agent confirms reading 4 Pascal files |
| 2 | TFormatSettings | `[CHECKPOINT] TFormatSettings: NO CRASH` | ~45 min | Create(LOCALE_USER_DEFAULT) works |
| 3 | FormatFloat | `[CHECKPOINT] FormatFloat: '1,234.50'` | ~90 min | Exact string match |
| 4 | TTimeSpan | `[CHECKPOINT] TTimeSpan: '00:01:30'` | ~60 min | Zero-padded HH:MM:SS |

**Action if checkpoint missing:**
- If 10+ minutes past expected time without checkpoint: Prompt agent "Please output checkpoint status"
- If no response within 2 minutes: Assume agent hung, interrupt (Ctrl+C) and restart from beginning of current step

**Action if checkpoint fails:**

1. **Crash or exception:**
   - Immediately note error output (screenshot/copy)
   - Command agent: "STOP. Revert changes using git and restart from the beginning of the current step."
   - If crash persists on second attempt: STOP entirely, rollback to backup tag, mark plan as failed

2. **Wrong output (string mismatch):**
   - Command: "Your output does not match expected. Please debug and retry."
   - Allow one retry
   - If still wrong: STOP, investigate root cause, consider manual intervention

3. **Repeated failures:** Abort execution, rollback to backup tag, flag for manual review

**Dependencies:** Task 2.1 (execution started)

---

### Task 2.3: Wait for Completion (Validate Format)

**Signal:**
```
[CHECKPOINT] ALL_STEPS_COMPLETE
✅ IMPLEMENTATION_COMPLETE

CHECKLIST:
✅ FormatFloat function fully implemented (all format codes)
✅ TFormatSettings class complete (all fields + Create + constructor)
✅ TTimeSpan class complete (22 methods + constructors)
✅ Build passes with zero warnings
✅ Thread safety ensured (immutable + no static buffers)
✅ Error handling policy followed (NaN, overflow, LCID failures)
✅ CRLF line endings used
✅ NetBox coding standards followed (C++17, UnicodeString, int64_t)
✅ Pascal reference implementations consulted
✅ No files modified outside allowed scope
```

**Expected format:** Exact 10-item checklist with ✅/❌ boxes. If format differs, command: "Please output checklist in exact format specified in prompt."

**If all 10 items ✅:** Proceed to Phase 3  
**If any ❌ or missing:** Request agent to complete missing items

**Dependencies:** Task 2.2 (all checkpoints passed)

---

## ✅ Phase 3: Verification (Enhanced)

**Duration:** 45-60 minutes  
**Status:** Pending execution completion

### Task 3.1: Build Verification (With Logging)

**Actions:**
1. Open command prompt in NetBox project root
2. Capture full build output:
   ```bash
   cmd /c build-x64.bat > build-output.log 2>&1
   ```
3. Review `build-output.log`:
   - Search for "warning" (case-insensitive): `findstr /i "warning" build-output.log`
   - Search for "error": `findstr /i "error" build-output.log`
   - Verify final line contains "Build succeeded" or similar success indicator

**Success Criteria:**
- Build completes successfully (exit code 0)
- Zero warnings (MSVC W4 level)
- No errors in modified files
- `build-output.log` captured for audit

**Document:**
- Build duration (from log timestamps or manual timing)
- Warning count (should be 0)
- Error count (should be 0)
- Store `build-output.log` in project root for reference

**If failures:**
- Build error → Reject completion, return to Phase 2
- Warnings present → Reject completion, request agent to fix

**Dependencies:** Task 2.3 (completion signal received)

---

### Task 3.2: Runtime Integration Check (With Precise Grep)

**Actions:**

1. Find existing call sites (exclude target files):
   ```bash
   grep -r "FormatFloat(" src/ --include="*.cpp" --include="*.hpp" | grep -v "Sysutils.cpp" | grep -v "Sysutils.hpp"
   grep -r "TFormatSettings" src/ --include="*.cpp" --include="*.hpp" | grep -v "Sysutils.cpp" | grep -v "Sysutils.hpp"
   grep -r "TTimeSpan" src/ --include="*.cpp" --include="*.hpp" | grep -v "Classes.cpp" | grep -v "Classes.hpp"
   ```
   Save output to `call-sites-before.txt` (if exists) or just count

2. Verify only target files modified:
   ```bash
   git diff --name-only | findstr /v "Sysutils.cpp Sysutils.hpp Classes.cpp Classes.hpp" && echo "Unexpected file modifications!" || echo "Only target files modified"
   ```

3. Verify call sites unchanged:
   ```bash
   # Compare if we have before snapshot, otherwise just verify no new call sites broken
   # The build already verified compilation, so focus on unintended modifications
   git diff src/ | findstr "FormatFloat\|TFormatSettings\|TTimeSpan" && echo "Call sites modified!" || echo "Call sites untouched"
   ```

**Expected:** Call sites unchanged (agent followed "Check existing usage first")

**Document:** Count of existing call sites found (should be at least 3 total across the three components)

**If call sites modified:**
- Reject completion
- Request agent to revert call site changes
- Investigate why agent violated scope

**Dependencies:** Task 3.1 (build passes)

---

### Task 3.3: Pre-Commit Code Quality Check (NEW)

**Details:** Run before final checklist validation

**Actions:**
```bash
# 1. Verify CRLF line endings
git ls-files -z src/base/*.cpp src/base/*.hpp | xargs -0 -n1 file | findstr "CRLF" > line-endings-check.txt
# All files should report "CRLF line terminators"
findstr "ASCII\|LF" line-endings-check.txt && echo "ERROR: Non-CRLF files found!" || echo "All CRLF OK"

# 2. Check for trailing whitespace
git diff --check src/base/ > trailing-whitespace.txt 2>&1
type trailing-whitespace.txt && echo "Trailing whitespace issues found!" || echo "No trailing whitespace"

# 3. Verify file encodings (UTF-8 without BOM or Windows-1252)
# Use: file command or manual inspection
```

**Success:** All checks pass, no formatting issues

**Document:** Create `code-quality-check.txt` with results

**If failures:** Request agent to fix formatting before proceeding

**Dependencies:** Task 3.2 (integration check passed)

---

### Task 3.4: Completion Checklist Validation

**Action:** Review agent's final output, count checklist items:

**Must have all 10 ✅:**
1. ✅ FormatFloat function fully implemented (all format codes)
2. ✅ TFormatSettings class complete (all fields + Create + constructor)
3. ✅ TTimeSpan class complete (22 methods + constructors)
4. ✅ Build passes with zero warnings
5. ✅ Thread safety ensured (immutable + no static buffers)
6. ✅ Error handling policy followed (NaN, overflow, LCID failures)
7. ✅ CRLF line endings used
8. ✅ NetBox coding standards followed (C++17, UnicodeString, int64_t)
9. ✅ Pascal reference implementations consulted
10. ✅ No files modified outside allowed scope

**Reference:** See IMPROVED_PROMPT.md Section [X] for exact checklist wording. Agent must match exactly.

**If any missing:** Request agent to complete and re-verify

**Dependencies:** Task 3.3 (quality check passed)

---

## 🎯 Phase 4: Finalization (Expanded)

**Duration:** 15-20 minutes  
**Status:** Pending all verification passed

### Task 4.1: Git Commit (With Exact Template)

**Pre-conditions:** All Phase 3 verification passed

**Command:**
```bash
git add src/base/Sysutils.cpp src/base/Sysutils.hpp src/base/Classes.cpp src/base/Classes.hpp

# Use exact commit message template
git commit -m "feat: Implement FormatFloat, TFormatSettings, TTimeSpan (Pascal→C++)

- Full FormatFloat with format string support (F, N, E, G, etc.)
- Complete TFormatSettings with locale initialization
- Full TTimeSpan class (22 methods + constructors)

Reference: FreePascal RTL objpas/sysutils (fmtflt.inc, sysinth.inc, dati.inc, datih.inc)
Prompt: IMPROVED_PROMPT.md (checkpoints: 4/4 passed)
Checklist: 10/10 items verified

Build: Zero warnings (MSVC W4)
Files: 4 modified (Sysutils.cpp/hpp, Classes.cpp/hpp)
Lines: ~650 added
Checkpoints: REFERENCES_LOADED, TFormatSettings NO CRASH, FormatFloat '1,234.50', TTimeSpan '00:01:30'
"
```

**Optional:** `git push origin feat/pascal-rtl-conversion`

**Success:** Commit created with hashed ID

**Dependencies:** Task 3.4 (checklist validated)

---

### Task "validated post-Implem Notes) with structured template (see Section 4.2). Must capture actual execution metrics, code metrics, checkpoint verification timestamps, deviations, and prompt compliance checklist. Output to `docs/implementation_notes_2026-04-23.md`. Should also attach `build-output.log`, `code-quality-check.txt`, and any checkpoint logs if captured. Dependencies: Task 4.1 (commit).—

**Notes:** `Create`, `Build` sections of report should be filled automatically from earlier tasks. Human adds qualitative assessment (code readability, comments, error handling completeness, thread safety observation) and lessons learned. This becomes organizational knowledge for future porting tasks.

**Output:** `docs/implementation_notes_2026-04-23.md` (or similar)

**Dependencies:** Task 4.1 (git commit)

---

## 📊 Success Criteria (Enhanced)

### Must have (all required):
- ✅ All 4 files modified (Sysutils.cpp, Sysutils.hpp, Classes.cpp, Classes.hpp)
- ✅ Zero compiler warnings (MSVC W4)
- ✅ Build passes cleanly with log captured
- ✅ No call sites modified (verified via grep diff)
- ✅ All 10 checklist items ✅ from agent
- ✅ 4 checkpoints observed in order (documented)
- ✅ ~650 lines of code (reasonable range: 600-700)
- ✅ CRLF line endings verified (file command or dos2unix)
- ✅ No trailing whitespace
- ✅ Feature branch created and on it
- ✅ Rollback tag exists
- ✅ Build environment validated (MSVC, CMake present)

### Nice to have:
- Agent reported reading Pascal references explicitly
- Agent self-corrected any issues before checkpoint
- Completion within 4.5 hours ±30 min
- Post-implementation notes comprehensive
- Build-output.log attached to notes

---

## 🚨 Enhanced Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Agent ignores checkpoints | Low | High | Monitor output, prompt if stalled >10min |
| Agent modifies call sites | Low | Medium | Integration check catches, revert & restart |
| Build failures | Low | High | Stop, fix, restart that component |
| Agent deviates from prompt | Low | Medium | Checkpoint verification ensures adherence |
| Time overrun | Medium | Low | Accept if all quality criteria met |
| Pascal reference not read | Very Low | Medium | Checkpoint 1 + pre-emptive instruction |
| Environment issues (MSVC/CMake missing) | Low | High | Phase 0 validation before starting |
| No rollback point | Very Low | High | Phase 0 Task 0.3 mandatory |
| Formatting issues (LF vs CRLF) | Low | Medium | Phase 3 Task 3.3 quality check |
| Wrong branch | Low | Medium | Phase 0 Task 0.1 verifies branch |

---

## 📈 Quality Gates (Expanded)

1. **Phase 0 Complete:** Branch created, environment validated, rollback tag exists
2. **Pre-flight:** All pre-flight items checked (Pascal files readable)
3. **Checkpoint 1:** Pascal references read (mandatory)
4. **Checkpoint 2:** TFormatSettings functional (no crash)
5. **Checkpoint 3:** FormatFloat correct output
6. **Checkpoint 4:** TTimeSpan correct output
7. **Build:** Zero warnings, log captured
8. **Integration:** Call sites unchanged, only target files modified
9. **Quality:** CRLF verified, no trailing whitespace, encoding correct
10. **Checklist:** 10/10 items ✅
11. **Commit:** Clean commit with proper message template
12. **Documentation:** Post-implementation notes complete

**Any gate failure:** Return to appropriate phase, do NOT proceed

---

## 🔗 Complete Task Dependency Graph

```
Phase 0:
  Task 0.1 (Create branch)
  Task 0.2 (Validate env) ──┐
  Task 0.3 (Rollback) ──────┤
                            ↓
Phase 1:
  Task 1.1 (Review docs) ───┼───→ (no dependencies)
  Task 1.2 (Pre-flight) ────┼───→ depends on Phase 0 tasks
  Task 1.3 (Prepare Claude) ──────────────→ depends on Task 1.2
                            ↓
Phase 2:
  Task 2.1 (Execute) ──────────────────────→ depends on Task 1.3
  Task 2.2 (Monitor checkpoints) ─────────→ depends on Task 2.1
  Task 2.3 (Wait completion) ─────────────→ depends on Task 2.2
                            ↓
Phase 3:
  Task 3.1 (Build verify) ────────────────→ depends on Task 2.3
  Task 3.2 (Integration check) ───────────→ depends on Task 3.1
  Task 3.3 (Quality check) ───────────────→ depends on Task 3.2
  Task 3.4 (Checklist validation) ────────→ depends on Task 3.3
                            ↓
Phase 4:
  Task 4.1 (Git commit) ───────────────────→ depends on Task 3.4
  Task 4.2 (Post-impl notes) ─────────────→ depends on Task 4.1
```

**Note:** Tasks within same phase may be sequential but not necessarily blocking unless specified.

---

## 📝 Enhanced Notes

- **Handoff Mode:** Not applicable (single human → Claude Code)
- **Branch Strategy:** Feature branch required (`feat/pascal-rtl-conversion`)
- **Parallelism:** None (sequential 6-step workflow + prep)
- **Rollback:** Two mechanisms:
  1. Git commit revert if pre-commit: `git reset --hard backup/pre-implementation-*`
  2. If post-commit: `git revert <commit-hash>` or `git reset --hard HEAD~1`
- **Timeboxing:** Each step has approximate duration; respect but don't enforce strictly
- **Audit Trail:** Captured files:
  - `build-output.log` (build capture)
  - `code-quality-check.txt` (formatting verification)
  - `call-sites-before.txt` (existing usage snapshot)
  - `docs/implementation_notes_2026-04-23.md` (final report)
- **Safety Gates:** 12 quality gates (up from 10) including environment, branch, rollback, formatting
- **Dependencies:** Explicitly encoded above, enforce before starting each task

---

## ✅ Final Acceptance (Enhanced)

Plan is approved for execution when:

1. ✅ Phase 0 complete (branch, env, rollback)
2. ✅ Documentation reviewed (Phase 1 Task 1.1)
3. ✅ Pre-flight checklist passed (enhanced with Pascal files)
4. ✅ IMPROVED_PROMPT.md copied and validated
5. ✅ All 4 checkpoints observed in order (with timestamps)
6. ✅ Build passes with zero warnings (log captured)
7. ✅ Call sites verified unchanged (precise grep)
8. ✅ Code quality checks passed (CRLF, no trailing whitespace)
9. ✅ 10-item completion checklist all ✅
10. ✅ Git commit created with exact template
11. ✅ Post-implementation notes complete with template

**Status:** READY TO EXECUTE

---

**Plan Version:** 2.0 (refined)  
**Created:** 2026-04-23  
**Refined:** 2026-04-23 (aif-improve)  
**Confidence:** 95/100  
**Estimated Duration:** 4.5-5 hours  
**Next Action:** Begin Phase 0 Task 0.1

**Plan ID:** `pascal-rtl-implementation-v2`  
**Recommended Filename for archive:** `.ai-factory/plans/pascal-rtl-implementation-2026-04-23-v2.md`

---

## 📦 Deliverables of This Refinement

1. ✅ **PLAN.md** - Updated in-place with all 19 improvements
2. ✅ **.ai-factory/plans/pascal-rtl-implementation-2026-04-23-v2.md** - Archived timestamped copy
3. ✅ **PLAN_REFINEMENT_REPORT.md** - Detailed analysis of all improvements

**Changes Applied:**
- Added 5 missing tasks (safety/quality gates)
- Fixed 7 task descriptions (clarity, validation)
- Fixed 5 dependency chains (correct order)
- Added 2 minor clarifications (format handling)
- **Total improvements:** 19

**Improvement Impact:**
- **Risk Reduction:** High (branch isolation + rollback)
- **Quality Assurance:** Higher (format checks, logging)
- **Auditability:** Much higher (logs, structured reports)
- **Execution Reliability:** Higher (explicit retry/recovery)

---

**Ready to execute the refined plan!**
