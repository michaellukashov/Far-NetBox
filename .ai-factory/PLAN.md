# Plan: Separate Agent and User Documentation

**Branch:** N/A (fast mode)

**Created:** 2026-04-13

## Settings
- **Testing:** No (documentation reorganization)
- **Logging:** Standard (record actions in task completion notes)
- **Docs:** N/A

## Tasks

### Task 1: Inventory and analysis
- [x] Task 1: Inventory and analysis
- **Deliverable:** List all markdown files, classify as agent/user, identify link dependencies.
- **Logging:** Record inventory in this task's notes.
- **Steps:**
  - Find all `.md` files (exclude third-party).
  - Classify: agent-related (`.ai-factory/`, `AGENTS-*`, planning/research) vs user docs.
  - Identify Markdown links referencing files to be moved.
  - Create move plan:
    - `AGENTS-Overview.md` → `.ai-factory/AGENTS-Overview.md`
    - `AGENTS-Standards.md` → `.ai-factory/AGENTS-Standards.md`
    - `AGENTS-Structure.md` → `.ai-factory/AGENTS-Structure.md`
    - `AGENTS-Workflows.md` → `.ai-factory/AGENTS-Workflows.md`
    - `01-RESEARCH.md` → `.ai-factory/RESEARCH.md` (rename)
    - `REFACTORING_PLAN.md` → `.ai-factory/plans/REFACTORING_PLAN.md`
    - `2026-02-28-prepare-environment-for-winxp-build.md` → `.ai-factory/plans/prepare-environment-for-winxp-build.md` (rename)
- **Dependencies:** none

### Task 2: Move AGENTS supplements and update links
- [x] Task 2: Move AGENTS supplements and update links
- **Deliverable:** AGENTS-*.md relocated; AGENTS.md links updated; correct link to AGENTS.md in each moved file.
- **Logging:** Log each move and text replacement.
- **Steps:**
  1. Move `AGENTS-Overview.md`, `AGENTS-Standards.md`, `AGENTS-Structure.md`, `AGENTS-Workflows.md` to `.ai-factory/`.
  2. In each moved file, replace `[AGENTS.md](AGENTS.md)` with `[AGENTS.md](../AGENTS.md)`.
  3. In root `AGENTS.md`, replace:
     - `[AGENTS-Overview.md](AGENTS-Overview.md)` → `[AGENTS-Overview.md](.ai-factory/AGENTS-Overview.md)`
     - `[AGENTS-Standards.md](AGENTS-Standards.md)` → `[AGENTS-Standards.md](.ai-factory/AGENTS-Standards.md)`
     - `[AGENTS-Structure.md](AGENTS-Structure.md)` → `[AGENTS-Structure.md](.ai-factory/AGENTS-Structure.md)`
     - `[AGENTS-Workflows.md](AGENTS-Workflows.md)` → `[AGENTS-Workflows.md](.ai-factory/AGENTS-Workflows.md)`
- **Dependencies:** Task 1

### Task 3: Move research and plan artifacts
- [x] Task 3: Move research and plan artifacts
- **Deliverable:** Research and plan files under `.ai-factory/`; update CMakeLists.txt.
- **Logging:** Log moves and renames.
- **Steps:**
  1. `01-RESEARCH.md` → `.ai-factory/RESEARCH.md` (rename)
  2. `REFACTORING_PLAN.md` → `.ai-factory/plans/REFACTORING_PLAN.md`
  3. `2026-02-28-prepare-environment-for-winxp-build.md` → `.ai-factory/plans/prepare-environment-for-winxp-build.md` (rename)
  4. Update `src/CMakeLists.txt`: change `REFACTORING_PLAN.md` path to `.ai-factory/plans/REFACTORING_PLAN.md`
- **Dependencies:** Task 1

### Task 4: Update cross-references
- [x] Task 4: Update cross-references
- **Deliverable:** All markdown links to old paths corrected; no broken links.
- **Logging:** List updated files and changes.
- **Steps:**
  - Search all `.md` files for links to moved files.
  - Update each to new path.
  - Spot-check key files for remaining stale links.
- **Dependencies:** Task 2, Task 3

### Task 5: Verify separation
- [x] Task 5: Verify separation
- **Deliverable:** Confirmation that agent docs are only in `.ai-factory/` (plus root `AGENTS.md`), and `docs/` contains only user docs.
- **Logging:** Provide verification summary.
- **Steps:**
  - Ensure root has no `AGENTS-*.md`; only `AGENTS.md` and user docs.
  - Ensure `docs/` contains only user-facing files.
  - Grep for any lingering references to moved files.
- **Dependencies:** Task 4

## Commit Plan
- After Task 5: Commit all changes with message:
```
docs: separate agent and user documentation

- Move AGENTS-*.md to .ai-factory/
- Update AGENTS.md links to new locations
- Fix AGENTS-* "See also" links to point to ../AGENTS.md
- Move research and plan artifacts into .ai-factory/ with proper naming
- Update CMakeLists.txt to reference relocated REFACTORING_PLAN.md
- Verify clean separation: root only user docs, agent docs centralized
```