---
title: OpenSSL Sync, Cleanup, and Patch Management
branch: feature/openssl-sync-cleanup
created: 2026-04-17
---

## Settings

- **Testing:** yes (include verification scripts)
- **Logging:** verbose (DEBUG level throughout)
- **Docs:** mandatory docs checkpoint
- **Roadmap Linkage:** none

## Tasks

### Phase 1: Analysis and Sync Preparation

- [x] **Task 1.1: Inventory Directories**
  - Create complete recursive listings of:
    - `/run/media/user/SSD1TB/projects/WinSCP-work/winscp-master/libs/openssl/` → `analysis/winscp_files.txt` (relative paths)
    - `libs/openssl-3/` → `analysis/netbox_files.txt` (relative paths)
  - Count files by extension: `.c`, `.h`, `.asm`, others.
  - Compute size summaries: `du -sh` per top-level directory (crypto, ssl, providers, include, etc.).
  - Output: `analysis/inventory_winscp.txt`, `analysis/inventory_netbox.txt`.
  - Log: INFO level summaries.

- [x] **Task 1.2: Extract NetBox Compiled Sources**
  - Parse `libs/openssl-3/CMakeLists.txt` and included modules (`cmake/OpenSSL.cmake`, `cmake/ucm.cmake`) to determine the **final** list of source files compiled into `ssleay32` and `libeay32`.
  - Account for:
    - `SSL_SOURCES` (41 .c files)
    - `CRYPTO_SOURCES` base set and exclusions via `ucm_remove_files`
    - `CRYPTO_PLATFORM_SOURCES` added for specific platform (x64, x86, ARM64)
    - `ASM_OBJECTS` produced by `openssl_setup_asm_files` (approx 31 for x64, 26 for x86, 0 for ARM64)
  - Ignore commented-out entries.
  - Output: `analysis/netbox_compiled_sources.txt` (relative paths, include both .c and .asm files).
  - Log: DEBUG for each source file added/removed during parsing.

- [x] **Task 1.3: Identify Candidate Removal Set (Conservative)**
  - From `analysis/netbox_files.txt`, subtract `analysis/netbox_compiled_sources.txt` → `analysis/unused_by_build.txt`.
  - From `unused_by_build.txt`, keep any file that exists in WinSCP (`analysis/winscp_files.txt`). Rationale: WinSCP files may be needed indirectly or for future compatibility; NetBox should not have extra unused files that WinSCP doesn't have.
  - Also exclude essential items (even if unused):
    - `CMakeLists.txt`
 - `0001-openssl-apply-NetBox-patches.patch`
    - `ACKNOWLEDGEMENTS.md`, `AUTHORS.md`, `INSTALL.md`, `LICENSE.txt`, `SUPPORT.md`, `Makefile`, `.gitignore`
    - Entire `include/` directory (headers)
    - Any `cmake/` helper modules if present
    - Any NetBox-specific build scripts or configs not in WinSCP
  - The remaining set is `analysis/removal_candidates.txt`.
  - For each candidate, record: size, extension, parent directory.
  - Generate summary: total count, total size, breakdown by extension/directory.
  - Log: INFO.

- [x] **Task 1.4: Identify Files to Sync**
  - Compute intersection of `analysis/winscp_files.txt` ∩ `analysis/netbox_files.txt` → `analysis/files_to_sync.txt`.
  - For each file in the intersection, compare checksums (SHA256) to detect modifications:
    - If different, add to `analysis/modified_files.txt` (NetBox version differs from WinSCP).
  - Also compute WinSCP-only files: `analysis/winscp_missing_in_netbox.txt` (files present in WinSCP but absent in NetBox). These may be copied if sync strategy includes adding missing files.
  - Output: `analysis/files_to_sync.txt`, `analysis/modified_files.txt`, `analysis/winscp_missing_in_netbox.txt`.
  - Log: INFO with counts and sample paths.

- [x] **Task 1.5: Analyze NetBox-Only Files**
  - Compute `analysis/netbox_only_files.txt` = `analysis/netbox_files.txt` - `analysis/winscp_files.txt` (files present in NetBox but not in WinSCP).
  - Cross-reference with `analysis/netbox_compiled_sources.txt`:
    - Flag which NetBox-only files ARE compiled.
    - Flag which NetBox-only files are NOT compiled.
  - These NetBox-only files are **not** removal candidates per the conservative rule, but should be reported for human review (they could be necessary extensions or orphaned).
  - Output: `analysis/netbox_only_compiled.txt`, `analysis/netbox_only_unused.txt`.
  - Log: INFO with counts and examples.

### Phase 2: Safety Backup

- [x] **Task 2.1: Backup Entire OpenSSL Directory**
  - Create a full backup of `libs/openssl-3/` to `libs/openssl-3-backup/` (preserving directory structure, all files).
  - Use `cp -r` or `rsync -a` to ensure complete copy.
  - Verify backup integrity (compare file counts or checksums of key files).
  - Log: INFO with backup size and location.

- [x] **Task 2.2: Create Removal Script**
  - Write `scripts/backup_openssl_removals.sh` to move `analysis/removal_candidates.txt` files to `libs/openssl-3-backup/removed/`.
  - The script should:
    - Read `analysis/removal_candidates.txt` (one path per line, relative to `libs/openssl-3/`)
    - Create destination parent directories under `libs/openssl-3-backup/removed/`
    - `mv` each file, logging: `MOVED: <src> -> <dest>`
    - If a file is already missing, log `MISSING: <src>`
    - Exit non-zero on any critical error
  - Make the script executable: `chmod +x scripts/backup_openssl_removals.sh`.
  - Log: DEBUG for each file operation.

### Phase 3: File Synchronization

- [x] **Task 3.1: Sync Files from WinSCP to NetBox**
  - For each file in `analysis/files_to_sync.txt`:
    - Copy from WinSCP source (`/run/media/user/SSD1TB/projects/WinSCP-work/winscp-master/libs/openssl/<path>`) to NetBox target (`libs/openssl-3/<path>`), **overwriting** existing files.
    - Preserve directory structure.
    - Log: `SYNC: <path> (overwritten)`.
  - Use `cp -f` or `rsync -c` to ensure exact copies.
  - After sync, verify that all target files exist and match WinSCP checksums.
  - Log: INFO summary of synced files count and total size.

- [x] **Task 3.2: Copy Missing WinSCP Files (Optional)**
  - If `analysis/winscp_missing_in_netbox.txt` is non-empty and sync strategy includes adding missing files:
    - Copy each missing file from WinSCP to NetBox (creating directories).
    - Log: `ADD: <path> (new)`.
  - Else: leave `analysis/winscp_missing_in_netbox.txt` as-is for manual review.
  - Log: INFO count of added files.

- [x] **Task 3.3: Re-Apply NetBox Custom Patches**
  - Apply the NetBox patch file to the synced tree:
    ```bash
    cd libs/openssl-3
 git apply --check 0001-openssl-apply-NetBox-patches.patch
    ```
  - If check succeeds, apply:
    ```bash
 git apply 0001-openssl-apply-NetBox-patches.patch
    ```
  - Log: DEBUG each patch hunk applied.
  - If any hunk fails:
    - Log: ERROR with details.
    - Generate `analysis/patch_conflicts.txt` listing conflicted files.
    - Continue to Task 3.4.

- [x] **Task 3.4: Handle Patch Conflicts**
  - If patch application failed:
 - Attempt three-way merge: `git apply --3way 0001-openssl-apply-NetBox-patches.patch`.
    - If still failing, create a new patch that reflects current state:
      - Use `git diff` after manual adjustments (if user chooses to fix).
 - Or generate `0001-openssl-apply-NetBox-patches-updated.patch` with adjusted hunks.
    - Document the conflicts and resolution steps in `docs/patch_conflict_report.md`.
  - If patch applied cleanly, verify no unintended rejections: `git status` should show no unmerged paths.
  - Log: INFO with outcome.

### Phase 4: Verification and Documentation

- [ ] **Task 4.1: Execute Removal (After Sync & Patch)**
  - Only after sync and patching are successful and verified, run `scripts/backup_openssl_removals.sh` to move removal candidates to backup.
  - Verify that `libs/openssl-3/` no longer contains files from `analysis/removal_candidates.txt`.
  - Log: INFO with list of actually removed files.

- [ ] **Task 4.2: Build Verification (on Windows)** (FAILED - see build/verification.log)
  - Clean build directory: `rm -rf build/` or `cmake --build build --target clean`.
  - Reconfigure: `cmake -B build -S . -G Ninja` (or the generator appropriate for the environment).
  - Build: `cmake --build build --config Release`.
  - Capture full output to `build/verification.log`.
  - Check for errors/warnings. If build fails:
    - Log: ERROR with build log excerpt.
    - Mark task failed and abort further tasks; user must resolve.
  - If build succeeds: log SUCCESS.
  - Additional: Run any post-build sanity checks if available (e.g., plugin loads in Far, basic connect test).

- [ ] **Task 4.3: Generate Comprehensive Report**
  - Write `docs/openssl_sync_cleanup_report.md` with sections:
    - **Summary**: Date, directories compared, sync strategy, patch status.
    - **Inventory**: File counts (WinSCP vs NetBox before/after), size difference.
    - **Sync Results**: Number of files overwritten, number of missing files added (if any), list of modified files.
    - **Patch Application**: Successfully applied? If not, conflict details and resolution.
    - **Removal Results**: Number of files removed, total disk space freed, list of removed files (grouped by directory/extension).
    - **NetBox-Only Files** (informational): Count of NetBox-only files, how many are compiled, how many are unused. No action taken but flagged for future review.
    - **Build Verification**: Success/failure; if failure, rebuild attempts and rollback.
  - Log: INFO report generated.

- [ ] **Task 4.4: Update Project Documentation**
  - Append to `.ai-factory/DESCRIPTION.md` a brief note:
    ```markdown
    ## OpenSSL Sync and Cleanup (2026-04-17)

    - Synchronized NetBox's OpenSSL 3 library with WinSCP baseline.
    - Re-applied NetBox-specific patches (TSAN, Windows compatibility).
    - Removed X unused files, freeing Y MB.
    - NetBox-only files (238 .c files) flagged for future review.
    - Build verified successfully.
    - See `docs/openssl_sync_cleanup_report.md` for details.
    ```
  - If any architectural changes (e.g., new OpenSSL configuration flags) were made, update `ARCHITECTURE.md` Third-Party Libraries section accordingly.
 - Ensure patch file `0001-openssl-apply-NetBox-patches.patch` still applies cleanly to the reduced tree (it should, as we only removed unneeded files, not patched ones).
  - Log: INFO docs updated.

## Commit Plan

- **After Phase 1** (Task 1.5 completed): commit analysis artifacts.
  ```
  git add analysis/ removal_candidates.txt netbox_compiled_sources.txt .ai-factory/plans/openssl-sync-cleanup.md
  git commit -m "refactor(openssl): analyze OpenSSL baseline, sync list, and removal candidates"
  ```

- **After Phase 3** (Task 3.4 completed): commit sync and patch changes.
  ```
  git add -A libs/openssl-3/
  git commit -m "feat(openssl): sync with WinSCP baseline and re-apply NetBox patches"
  ```

- **After Phase 4.1** (removal executed): commit removal.
  ```
  git add -A libs/openssl-3/ scripts/backup_openssl_removals.sh
  git commit -m "chore(openssl): remove unused files; backup created"
  ```

- **After Phase 4.4** (docs updated): commit documentation.
  ```
  git add .ai-factory/DESCRIPTION.md ARCHITECTURE.md docs/openssl_sync_cleanup_report.md
  git commit -m "docs: record OpenSSL sync/cleanup outcome and report"
  ```

## Notes

- **Destructive operations**: Full backup is created in Phase 2.1; removal only happens after successful sync and patch re-application.
- **Conservative removal**: Only files that are both (a) unused by NetBox build and (b) absent from WinSCP are removed. NetBox-only files (even unused) are retained but reported.
- **Patch management**: If the patch no longer applies cleanly, conflicts are documented and an updated patch is generated for future maintenance.
- **Build verification**: Mandatory; failure triggers rollback and abort.
- **Progress tracking**: Each task updates the checklist immediately upon completion.
- **Logging**: All scripts and operations use verbose DEBUG-level logging; logs are kept in `build/` and `analysis/` directories.
