---
name: aif-improve
description: Refine and enhance an existing implementation plan with a second iteration. Re-analyzes the codebase, checks for gaps, missing tasks, wrong dependencies, and improves the plan quality.
---

# Plan Refinement Report

**Plan:** `PLAN.md` (Pascal-to-C++ Port Implementation)  
**Refinement Date:** 2026-04-23  
**Analyzer:** aif-improve skill  
**Task Count:** 16 tasks across 4 phases

---

## 📊 Analysis Summary

### Codebase Re-Analysis Performed

✅ **Read project context**: DESCRIPTION.md (tech stack: C++17, MSVC, Far-NetBox plugin)  
✅ **Read architecture**: Not available (but inferred from DESCRIPTION)  
✅ **Read existing plan**: PLAN.md fully analyzed  
✅ **Git mode**: Enabled (create_branches: true)  
✅ **Deep codebase exploration**: Project is NetBox (not Far-NetBox) with target files in `src/base/` (verified against references in IMPROVED_PROMPT.md)  
✅ **Skill-context**: No project-specific overrides found

### Key Findings from Re-Analysis

The original PLAN.md is strong but reveals several gaps when cross-checked against the actual implementation context:

1. **File path precision**: Plan references generic `src/` but target files are specifically `src/base/Sysutils.cpp`, `src/base/Sysutils.hpp`, `src/base/Classes.cpp`, `src/base/Classes.hpp`
2. **Missing rollback/undo strategy**: No explicit procedure if implementation fails or produces bugs
3. **Missing code quality gate**: No pre-commit code review or formatting check
4. **Missing environment validation**: No verification of compiler/toolchain before starting
5. **Missing branch management**: Should verify current branch and optionally create feature branch
6. **Missing explicit error recovery**: Checkpoint failure procedures are vague
7. **Missing call site discovery detail**: Should use precise grep commands with path `src/base/` and `src/`
8. **Missing commit message structure**: Should provide exact template with placeholders
9. **Missing task dependencies**: Tasks within phases are sequential but not encoded as dependencies in the plan structure
10. **Missing Pascal file verification**: Should explicitly check the 4 Pascal reference files exist and are readable before Step 1

---

## 🆕 Missing Tasks (5 found)

### 1. Create Git Feature Branch

**Why:** Git mode is enabled with `create_branches: true`. Best practice is to isolate implementation on a feature branch.

**What:** Create and switch to feature branch `feat/pascal-rtl-conversion` (or derived from current branch name)

**Dependency:** Before Phase 1 Task 1.1

**Command:**
```bash
git checkout -b feat/pascal-rtl-conversion
# or if on existing branch: git checkout main && git pull && git checkout -b feat/pascal-rtl-conversion
```

---

### 2. Validate Build Environment

**Why:** Build requires MSVC, CMake, and specific tools. Should verify before spending 4 hours.

**What:** Check compiler version, CMake availability, and run dry-run build test.

**Dependency:** After Task 1.2 (pre-flight) but before 1.3

**Actions:**
```bash
# Check MSVC
cl.exe /Bv 2>&1 | findstr "Version"

# Check CMake
cmake --version

# Verify build script exists and is executable (dry-run)
cmd /c build-x64.bat /? 2>nul || echo "Build script not found or not working"
```

**Success:** All tools present, build script accessible

---

### 3. Establish Rollback Point

**Why:** If implementation fails or produces bugs, we need a clean revert point.

**What:** Create a git tag or branch after successful pre-flight but before any code modification.

**Dependency:** After Task 2.1 (before agent starts) OR at end of Phase 1

**Command:**
```bash
git add -A  # stage current clean state
git commit -m "chore: establish rollback point before Pascal RTL implementation"
# Tag as backup
git tag backup/pre-implementation-$(date +%Y%m%d-%H%M%S)
```

**Rollback procedure (to be documented in plan):**
```bash
git checkout main  # or original branch
git reset --hard backup/pre-implementation-*
git branch -D feat/pascal-rtl-conversion  # if created
```

---

### 4. Pre-Commit Code Quality Check

**Why:** The plan requires zero warnings. Should verify formatting and run static analysis before final build.

**What:** After Phase 3 Task 3.1 (build passes) but before Task 3.3 (checklist validation), run:

- Code formatting check (if clang-format configured)
- Static analysis (if available)
- File encoding check (CRLF verification)

**Dependency:** After Task 3.1 (build passes)

**Actions:**
```bash
# Check line endings (CRLF required)
git ls-files -z src/base/*.cpp src/base/*.hpp | xargs -0 file | findstr CRLF

# Optional: clang-format check if format file exists
if exist .clang-format (
  clang-format -dry-run --Werror src/base/Sysutils.cpp src/base/Sysutils.hpp src/base/Classes.cpp src/base/Classes.hpp
)

# Verify no trailing whitespace
git diff --check src/base/ | findstr "trailing whitespace"
```

**Success:** All checks pass, no formatting issues

---

### 5. Post-Implementation Validation (Expanded)

**Why:** Current post-implementation notes task is too vague. Should be structured with specific metrics to capture.

**What:** Detailed documentation of actual vs expected results.

**Dependency:** After Task 4.1 (git commit) or if commit fails

**Template:**
```markdown
# Implementation Report: Pascal-to-C++ Port

## Execution Metrics
- Start time: ___
- End time: ___
- Total duration: ___ (expected: ~4h)
- Agent iterations: ___ (should be 1)
- Checkpoints passed: 4/4

## Code Metrics
- Files modified: 4 (verify)
- Lines added: ___ (expected: ~650)
- Lines removed: ___
- Build warnings: 0 (verify)
- Build errors: 0

## Checkpoint Verification
- [ ] REFERENCES_LOADED observed at: ___
- [ ] TFormatSettings NO CRASH at: ___
- [ ] FormatFloat '1,234.50' at: ___
- [ ] TTimeSpan '00:01:30' at: ___

## Deviations
- Any deviations from prompt requirements: ___
- Any tasks failed/redone: ___
- Call site modifications: None / Some (list)

## Quality Assessment
- Code readability: ___
- Comments/docstrings: ___
- Error handling completeness: ___
- Thread safety observed: ___

## Lessons Learned
- What went well: ___
- What could be improved: ___
- Recommendations for future: ___
```

---

## 📝 Task Improvements (7 found)

### Task 1.2: Verify Pre-Flight Checklist

**Issue:** Checklist is good but missing verification of Pascal reference file readability.

**Fix:** Add:
```markdown
Additional items:
- [ ] Verify Pascal reference files exist and are readable:
  - `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\fmtflt.inc` (readable)
  - `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\sysinth.inc` (readable)
  - `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\dati.inc` (readable)
  - `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\datih.inc` (readable)
```

---

### Task 1.3: Prepare Claude Code

**Issue:** Should also verify that IMPROVED_PROMPT.md is the exact file being used (not a stale copy).

**Fix:** Add:
```markdown
Validation: Open IMPROVED_PROMPT.md, confirm header says "IMPROVED_PROMPT" and contains 6-step workflow
```

---

### Task 2.1: Execute Prompt

**Issue:** Missing explicit instruction to ensure agent reads Pascal references first (checkpoint 1 enforces this, but pre-emptive instruction helps).

**Fix:** Add to description:
```markdown
Pre-emptive instruction (if agent asks): "Please begin with Step 1: Read the Pascal reference files. Confirm when done with [CHECKPOINT] REFERENCES_LOADED."
```

---

### Task 2.2: Monitor Checkpoints

**Issue:** Checkpoint failure handling is under-specified. Should have explicit retry/recovery steps.

**Fix:** Enhance the "Action if checkpoint fails" section:

```markdown
**If checkpoint fails:**

1. **Crash or exception**: 
   - Immediately note error output
   - Command agent: "STOP. Revert changes using git and restart from the beginning of the current step."
   - If crash persists on second attempt: STOP entirely and mark plan as failed
   
2. **Wrong output (string mismatch)**:
   - Command: "Your output does not match expected. Please debug and retry."
   - Allow one retry
   - If still wrong: STOP and investigate root cause
   
3. **No checkpoint after expected time +10 min**:
   - Prompt: "Please output current status and checkpoint"
   - If no response within 2 min: assume agent hung, interrupt and restart
   
4. **Repeated failures**: Abort execution, rollback to backup tag, and flag for manual review
```

---

### Task 3.1: Build Verification

**Issue:** Should capture and log build output for audit trail.

**Fix:** Add:
```markdown
**Documentation:** Capture full build output to `build-output.log`:
```bash
cmd /c build-x64.bat > build-output.log 2>&1
```
Then review `build-output.log` for warnings/errors. Attach to post-implementation notes.
```

---

### Task 3.2: Runtime Integration Check

**Issue:** Grep commands are generic `src/` but call sites could be anywhere. Should use more precise patterns and check only existing usage (not new code).

**Fix:** Use precise grep patterns and filter to exclude newly modified files:

```bash
# Find existing call sites (exclude the 4 target files)
grep -r "FormatFloat(" src/ --include="*.cpp" --include="*.hpp" | grep -v "Sysutils.cpp" | grep -v "Sysutils.hpp"
grep -r "TFormatSettings" src/ --include="*.cpp" --include="*.hpp" | grep -v "Sysutils.cpp" | grep -v "Sysutils.hpp"
grep -r "TTimeSpan" src/ --include="*.cpp" --include="*.hpp" | grep -v "Classes.cpp" | grep -v "Classes.hpp"

# Verify none appear in git diff (only modifications to target files should show)
git diff --name-only | findstr /v "Sysutils.cpp Sysutils.hpp Classes.cpp Classes.hpp" && echo "Unexpected file modifications!" || echo "Only target files modified"
```

---

### Task 4.2: Create Post-Implementation Notes

**Issue:** Should also verify that the agent followed all prompt requirements (encoding, standards, etc.)

**Fix:** Expand template to include:

```markdown
## Prompt Compliance Checklist
- [ ] CRLF line endings verified (dos2unix check)
- [ ] Zero compiler warnings confirmed
- [ ] All 4 files modified (list them)
- [ ] No third-party library files touched (libs/ unchanged)
- [ ] Pascal references cited in commit message
- [ ] NetBox coding standards followed (C++17, UnicodeString, int64_t)
- [ ] Thread safety: No static mutable state, immutable objects
- [ ] Error handling: NaN/overflow/LCID handled per policy
```

---

## 🔗 Dependency Fixes (3 found)

### 1. Task 1.3 should depend on Task 1.2
**Reason:** Can't prepare Claude Code until pre-flight checklist is verified complete.

**Action:** Add to Task 1.3: `Dependencies: Task 1.2`

---

### 2. Task 2.1 should depend on Task 1.3
**Reason:** Execution can't start until Claude Code is prepared.

**Action:** Add to Task 2.1: `Dependencies: Task 1.3`

---

### 3. Task 3.3 should depend on Task 3.2
**Reason:** Can't validate final checklist until integration check confirms call sites untouched.

**Action:** Add to Task 3.3: `Dependencies: Task 3.2`

---

### 4. Task 4.1 should depend on Task 3.3
**Reason:** Git commit should only happen after checklist validation.

**Action:** Add to Task 4.1: `Dependencies: Task 3.3`

---

### 5. Task 4.2 should depend on Task 4.1
**Reason:** Post-impl notes should document the commit outcome.

**Action:** Add to Task 4.2: `Dependencies: Task 4.1`

---

**Note:** Also add missing dependency: Task 3.2 depends on Task 3.1 (build must pass before integration check).

---

## 🗑️ Removals (0 found)

No tasks are redundant or unnecessary. All tasks serve a clear purpose.

---

## 📋 Task Quality Issues (2 found)

### Task 2.3: Wait for Completion

**Issue:** Description says "Wait for completion signal" but doesn't specify what to do if agent produces unexpected output format.

**Fix:** Add:
```markdown
**Expected format:** Agent must output the exact 10-item checklist with ✅/❌ boxes. If format differs, command: "Please output checklist in exact format specified in prompt."
```

---

### Task 3.3: Completion Checklist Validation

**Issue:** The 10 items are listed but the plan doesn't provide the exact wording the agent should output.

**Fix:** Add reference to "See IMPROVED_PROMPT.md Section X for exact checklist wording. Agent must match exactly."

---

## 📌 Summary of Required Changes

| Category | Count | Impact |
|----------|-------|--------|
| **Missing Tasks** | 5 | High - adds essential safety/quality gates |
| **Task Improvements** | 7 | Medium - clarifies existing tasks |
| **Dependency Fixes** | 5 | High - ensures correct execution order |
| **Task Quality Issues** | 2 | Low - minor clarifications |
| **Removals** | 0 | N/A |

**Total Changes:** 19 improvements recommended

---

## 🎯 Recommended Action Plan

Apply the following improvements in order:

1. **Add missing tasks** (5 new tasks):
   - Create Git Feature Branch (Phase 1)
   - Validate Build Environment (Phase 1)
   - Establish Rollback Point (Phase 1)
   - Pre-Commit Code Quality Check (Phase 3)
   - Expanded Post-Implementation Validation (Phase 4)

2. **Fix task descriptions** (7 improvements):
   - 1.2: Add Pascal file verification
   - 1.3: Add IMPROVED_PROMPT.md validation
   - 2.1: Add pre-emptive instruction
   - 2.2: Add detailed checkpoint failure recovery
   - 3.1: Add build output logging
   - 3.2: Use precise grep commands with exclusions
   - 4.2: Expand template with compliance checklist

3. **Fix dependencies** (5 fixes):
   - Explicitly encode sequential dependencies within each phase and across phases
   - Ensure Phase 1 → Phase 2 → Phase 3 → Phase 4 linear flow

4. **Minor clarifications** (2 tasks):
   - 2.3: Add unexpected format handling
   - 3.3: Reference exact checklist wording

---

## ✨ Enhanced Plan Preview

After applying improvements, the plan will have:

- **Total Tasks:** 21 (up from 16)
- **Phases:** 4 (unchanged)
- **New Safety Gates:** 
  - Feature branch creation
  - Environment validation
  - Rollback point establishment
  - Code quality check
  - Detailed post-mortem
- **Better Dependencies:** All tasks encoded with proper `blockedBy` relationships
- **More Robust Checkpoint Handling:** Clear retry/recovery procedures
- **Audit Trail:** Build logs, compliance checklists, structured reporting

---

## ❓ Ask User

**Apply these 19 improvements to PLAN.md and save refined version to `.ai-factory/plans/`?**

**Options:**
1. ✅ Yes, apply all improvements (recommended)
2. ✏️ Let me pick which ones (review each change)
3. ❌ No, keep the plan as is

**Recommendation:** Option 1. All improvements are low-risk, high-value additions that enhance reliability and auditability without adding significant complexity.

---

**Plan file:** `PLAN.md`  
**Target plans directory:** `.ai-factory/plans/`  
**Suggested refined filename:** `pascal-rtl-implementation-2026-04-23-v2.md`
