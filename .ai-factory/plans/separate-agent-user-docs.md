# Separate Agent and User Documentation

**Branch**: separate-agent-user-docs
**Created**: 2026-04-17

## Settings

- **Testing**: No
- **Logging**: Verbose (detailed DEBUG logs for each file operation and link update)
- **Docs**: Yes (mandatory documentation checkpoint at completion)
- **Roadmap**: None

---

## Research Context

The project currently intermingles agent-facing documentation (AGENTS.md series for AI coding assistants) with user-facing documentation (README.md, ARCHITECTURE.md, PROJECT.md, etc.) in the repository root. The goal is to cleanly separate:

- **Agent-related info**: stays in `.ai-factory/` and the `AGENTS*.md` files at the root.
- **User documentation**: all `docs/*.md` files must be user-only.

This reorganization improves clarity for both humans and AI agents.

---

## Tasks

- [x] Create docs directory structure: Create `docs/features/` and `docs/i18n/` (docs/ already exists), log each operation with full path, ensure parent directories exist.
- [x] Move core documentation files: Move from root to `docs/`: `README.md`, `ARCHITECTURE.md`, `SOURCE_ORGANIZATION.md`, `DEPENDENCIES.md`, `PROJECT_RULES.md`, `PROJECT.md`, `REFACTORING_PLAN.md`, `01-RESEARCH.md`, `2026-02-28-prepare-environment-for-winxp-build.md`. Log each move (source → destination). Verify success (file exists at destination, absent at source).
- [x] Move i18n files: Move `README.PL.md` → `docs/i18n/README.PL.md`; `README.RU.md` → `docs/i18n/README.RU.md`. Log each move, verify.
- [x] Relocate CODEBASE.md and fix links: Move `CODEBASE.md` to `docs/CODEBASE.md`. Update all `./ARCHITECTURE.md`, `./SOURCE_ORGANIZATION.md`, `./DEPENDENCIES.md`, `./PROJECT_RULES.md`, `./README.md` to `./` (same-directory) links. Update `./AGENTS.md` to `../AGENTS.md`. Log each link change (old URL → new URL). Save file.
- [x] Write root README redirect: Create `README.md` at root with concise content: one-sentence project description, "For user documentation, see `docs/README.md`." and "For AI assistant guidelines, see `AGENTS.md`." Log creation. Ensure CRLF line endings.
- [x] Link validation: Search all markdown files (excluding libs/, build/, .ai-factory/) for Markdown links to moved files. Verify target exists relative to linking file. Log any broken links. (No broken links found.)
- [x] Root cleanup verification: List all `*.md` files in root (excluding AGENTS*.md and README.md). Only AGENTS series and new README remain. Confirmed separation.

---

## Commit Plan

This plan comprises 8 tasks. Commit checkpoints:

1. After Task 2 (move core docs): commit message `docs: reorganize core documentation into docs/`
2. After Task 4 (move i18n): commit message `docs: reorganize language-specific docs into docs/i18n/`
3. After Task 6 (root README): commit message `docs: add root redirect README for documentation separation`

Final commit after Task 8: `docs: complete agent/user docs separation`

---

## Notes

- The AGENTS.md series remains at the root untouched; they define agent-facing guidelines and link to `.ai-factory/` config which is already agent-focused.
- CODEBASE.md is moved to `docs/` to serve as the central index for user documentation; its links are updated accordingly.
- No modifications will be made to `.ai-factory/` except possibly verifying that AI-specific architecture files there (.ai-factory/ARCHITECTURE.md, DESCRIPTION.md) are distinct from user-facing `docs/ARCHITECTURE.md`.
- All file moves are within the same repository; no data loss.
- Use verbose logging for each file operation (move/copy) and line edit to track changes.
- At the mandatory docs checkpoint, verify that all intended moves are complete and that CODEBASE.md links are functional.
