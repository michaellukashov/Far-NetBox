# Plan: Update Documentation Versions and Add Maintenance Guide

**Branch:** N/A (fast mode)  
**Created:** 2026-04-18  
**Settings:** Testing: no, Logging: standard, Docs: yes, Roadmap: none

---

## Research Context

This is a straightforward documentation and version update task:
- Update WinSCP version references from 6.5.1 to 6.5.6
- Update OpenSSL version references from 3.3.2/3.3.7 to 3.5.6
- Remove developer build instructions from user-facing READMEs and move them to a dedicated `DEVELOPER.md`
- Add guidance on updating WinSCP and OpenSSL in the developer guide
- Update ChangeLog with the version upgrades

No prior research required.

---

## Tasks

### Phase 1: Create Developer Guide

- [x] **Task 1: Create `docs/DEVELOPER.md`**
   - This file will house all developer-oriented information currently in READMEs, plus new maintenance procedures.
   - Include sections:
     - Prerequisites (VS2022, CMake, Ninja)
     - Building from source (quick batch files and manual CMake)
     - Updating WinSCP (download sources, copy to `libs/openssl-3/`, update version strings in `resource.h` and `.lng` files, rebuild, test)
     - Updating OpenSSL (replace `libs/openssl-3/` with new version, update `opensslv.h`, update version strings, rebuild)
     - Running tests (if any)
     - Submitting changes
   - Use content from the current `docs/README.md` (full build section) and `docs/i18n/*.md` to draft the build parts.
   - Write clear step-by-step instructions for the update processes, referencing specific files.
   - Also mention where to update version numbers in documentation (ARCHITECTURE.md, PROJECT.md, etc.).
   - **Deliverable:** `docs/DEVELOPER.md` created with comprehensive developer documentation.

### Phase 2: Simplify README Files

- [x] **Task 2: Update `docs/README.md` (English)**
   - Remove the entire "How to build from source" section (including prerequisites and both manual and batch instructions).
   - Add a short note at the end or appropriate section:
     ```
     ### Building

     For instructions on how to build NetBox from source, please see the [Developer Guide](DEVELOPER.md).
     ```
   - Update the "Based on" lines:
     - Change `WinSCP ... 6.5.1` → `6.5.6`
     - After FileZilla line, add: `Cryptography based on OpenSSL 3.5.6 Copyright (c) 1998-2025 The OpenSSL Project`
   - Keep badges, links, and license unchanged.
   - **Deliverable:** `docs/README.md` cleaned up, versions updated.

- [x] **Task 3: Update `docs/i18n/README.PL.md` (Polish)**
   - Remove the detailed build section (starting from "Jak skompilować ze źródeł" down to the end of build instructions). Replace with:
     ```
     ### Kompilacja

     Instrukcje kompilacji znajdują się w [Przewodniku Dewelopera](DEVELOPER.md).
     ```
   - Update version numbers:
     - WinSCP: `6.5.1` → `6.5.6`
     - PuTTY: if still `0.70` → `0.81` (to match others)
     - Add OpenSSL line after FileZilla: `Kryptografia oparta na OpenSSL 3.5.6 Copyright (c) 1998-2025 The OpenSSL Project`
   - Also update the AppVeyor badge URL to match the main project (use same badge as English README) if currently different.
   - **Deliverable:** Polish README simplified and versions updated.

- [x] **Task 4: Update `docs/i18n/README.RU.md` (Russian)**
   - Remove the entire "6. Сборка из исходников" section (including all bullet points and code blocks). Replace with:
     ```
     ### Сборка

     Инструкции по сборке см. в [Руководстве разработчика](DEVELOPER.md).
     ```
   - Update version numbers:
     - WinSCP: `6.5.1` → `6.5.6`
     - PuTTY: if still `0.70` → `0.81`
     - Add OpenSSL line after FileZilla: `Криптография основана на OpenSSL 3.5.6 Copyright (c) 1998-2025 The OpenSSL Project`
   - Add GitHub Actions and AppVeyor badges at the top (like English version) if missing.
   - Ensure the footer license still uses correct copyright years (already 2000-2025).
   - **Deliverable:** Russian README simplified and versions updated.

### Phase 3: Update Version Strings in Source

- [x] **Task 5: Update `src/NetBox/resource.h`**
   - Change `#define WINSCP_VERSION_WTXT L"6.5.1"` → `L"6.5.6"`
   - Change `#define OPENSSL_VERSION_WTXT L"3.3.2"` → `L"3.5.6"`
   - Keep other version defines unchanged.
   - **Deliverable:** resource.h updated.

- [x] **Task 6: Update `src/NetBox/resource.h.template`** (if present)
   - Apply same changes as Task 5.
   - **Deliverable:** resource.h.template updated.

- [x] **Task 7: Update language resource files**
   For each of: `src/NetBox/NetBoxEng.lng`, `src/NetBox/NetBoxRus.lng`, `src/NetBox/NetBoxPol.lng`, `src/NetBox/NetBoxSpa.lng`:
   - Find the line containing `"6.5.1"` (WinSCP version) and replace with `"6.5.6"`.
   - Find the line containing `"3.3.2"` (OpenSSL version) and replace with `"3.5.6"`.
   - These lines typically appear around line numbers 1278 and 1266 respectively, but use text search to be safe.
   - Preserve surrounding quotes and structure.
   - **Deliverable:** All .lng files updated.

### Phase 4: Update OpenSSL Header

- [x] **Task 8: Update `libs/openssl-3/include/openssl/opensslv.h`**
   - Change:
     ```c
     # define OPENSSL_VERSION_MAJOR  3
     # define OPENSSL_VERSION_MINOR  3
     # define OPENSSL_VERSION_PATCH  7
     ```
     to
     ```c
     # define OPENSSL_VERSION_MAJOR  3
     # define OPENSSL_VERSION_MINOR  5
     # define OPENSSL_VERSION_PATCH  6
     ```
   - Update version strings:
     `# define OPENSSL_VERSION_STR "3.3.7"` → `"3.5.6"`
     `# define OPENSSL_FULL_VERSION_STR "3.3.7"` → `"3.5.6"`
   - Update `OPENSSL_VERSION_TEXT` accordingly, e.g., `"OpenSSL 3.5.6 <release date>"`. Use a plausible date such as `"7 Apr 2026"` (from current header) or adjust if needed.
   - **Do not modify other parts.**
   - **Deliverable:** opensslv.h updated to 3.5.6.

### Phase 5: Update Documentation Version References

- [x] **Task 9: Update `docs/ARCHITECTURE.md`**
   - Find: `"based on WinSCP v6.5.1 codebase"` → change to `"based on WinSCP v6.5.6 codebase"`.
   - **Deliverable:** ARCHITECTURE.md updated.

- [x] **Task 10: Update `docs/CODEBASE.md`**
    - Change: `- **Based On**: WinSCP 6.5.1, ...` → `WinSCP 6.5.6`.
    - **Deliverable:** CODEBASE.md updated.

- [x] **Task 11: Update `docs/PROJECT.md`**
    - Change: `"Based on WinSCP version 6.5.1"` → `"6.5.6"`.
    - **Deliverable:** PROJECT.md updated.

- [x] **Task 12: Update `docs/PROJECT_RULES.md`**
    - Change: `"- **WinSCP** version 6.5.1"` → `"6.5.6"`.
    - **Deliverable:** PROJECT_RULES.md updated.

- [x] **Task 13: Update `.ai-factory/DESCRIPTION.md`**
    - Change: `"Built on proven codebases (WinSCP 6.5.1, ...)"` → `"WinSCP 6.5.6"`.
    - Also update OpenSSL version if mentioned elsewhere in the file.
    - **Deliverable:** DESCRIPTION.md updated.

- [x] **Task 14: Update `.ai-factory/AGENTS-Overview.md`**
    - In the table, change WinSCP version `6.5.1` → `6.5.6`.
    - Also update OpenSSL version `3.3.2` → `3.5.6` if present.
    - **Deliverable:** AGENTS-Overview.md updated.

### Phase 6: Update ChangeLog

- [x] **Task 15: Update `ChangeLog`**
    - In the `[Unreleased]` section, under "### Security" or "### Dependencies" (or create a "### Dependencies" heading if none), add:
      ```
       * Upgrade WinSCP sources: WinSCP 6.5.6
       * Upgrade openssl sources: openssl 3.5.6
      ```
    - Follow the existing indentation and style (use `*` with two spaces before text, as in existing entries).
    - Do not modify older version entries.
    - **Deliverable:** ChangeLog updated with new version upgrades.

### Phase 7: Final Review

- [x] **Task 16: Verify consistency**
    - Run a repository-wide search for `6.5.1` and `3.3.2` (or old OpenSSL versions like 3.3.7) to ensure no remaining references in documentation and header files.
    - Ensure no `.hlf` files contain version strings (they typically don't).
    - Confirm that all README files (`docs/README.md`, `docs/i18n/*.md`) no longer contain detailed build instructions, only a link to `DEVELOPER.md`.
    - Check that the new `docs/DEVELOPER.md` file exists and includes both build instructions and update procedures.
    - Ensure that the `docs/` directory contains the updated files and that the developer guide is properly linked.
    - If any stray old versions found, correct them.
    - **Deliverable:** All version references consistent; READMEs simplified; DEVELOPER.md present.

---

## Commit Plan

This plan has 16 tasks that can be grouped into 3 commits:

- **Commit 1:** Phase 1 + Phase 2
  Create `docs/DEVELOPER.md` and simplify all README files; update README version numbers and add OpenSSL credits.
  Suggested message:
  ```
  docs: add developer guide and simplify user READMEs

  - Create docs/DEVELOPER.md with build and maintenance instructions
  - Remove build sections from docs/README.md and i18n READMEs
  - Add link to DEVELOPER.md in user READMEs
  - Update WinSCP version to 6.5.6 in all user-facing docs
  - Add OpenSSL 3.5.6 credit in all user-facing docs
  - Fix Polish and Russian translations (versions, badges)
  ```

- **Commit 2:** Phase 3 + Phase 4
  Update version strings in source (resource.h, .lng files) and OpenSSL header (opensslv.h).
  Suggested message:
  ```
  build: update version strings to WinSCP 6.5.6 and OpenSSL 3.5.6

  - Update resource.h: WINSCP_VERSION_WTXT and OPENSSL_VERSION_WTXT
  - Update resource.h.template accordingly
  - Update all language .lng files (Eng, Rus, Pol, Spa)
  - Update libs/openssl-3/include/openssl/opensslv.h to 3.5.6
  ```

- **Commit 3:** Phase 5 + Phase 6 + Phase 7
  Update remaining documentation references, update ChangeLog, and final verification.
  Suggested message:
  ```
  docs: update remaining references to WinSCP 6.5.6 and OpenSSL 3.5.6

  - Update ARCHITECTURE.md, CODEBASE.md, PROJECT.md, PROJECT_RULES.md
  - Update .ai-factory/DESCRIPTION.md and AGENTS-Overview.md
  - Add upgrade entries to ChangeLog
  - Verify no stale version references remain
  ```

---

## Next Steps

After this plan is approved, execute tasks sequentially using `/aif-implement` (or implement manually). Ensure all changes are staged and committed according to the commit plan.