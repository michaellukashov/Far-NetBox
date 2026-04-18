<!-- handoff:task:openssl-sync-2026 -->
# OpenSSL Synchronization: WinSCP → NetBox

**Created:** 2026-04-18  
**Branch:** N/A (library sync task)  
**Source:** `D:/Projects/WinSCP-work/winscp-master/libs/openssl/`  
**Target:** `D:/Projects/NetBox/NetBox-dev/libs/openssl-3/`

---

## Settings

- **Testing:** No (third-party library sync)
- **Logging:** N/A (file operations only)
- **Docs:** Yes (update patch documentation if needed)

---

## Research Context

### Source Analysis
- **WinSCP OpenSSL:** 1,336 files (clean upstream source)
- **NetBox OpenSSL:** 2,067 files (includes removed/ directory and build artifacts)
- **Custom patch:** `0001-openssl-NetBox-patches.patch` (279 lines, 8 file modifications)

### Key Differences
1. **NetBox-specific files:** `CMakeLists.txt`, `.gitignore`, `removed/` directory
2. **Architecture files:** NetBox has additional asm files for x86/x64/ARM64
3. **Patch targets:** Custom MSVC compatibility patches (tsan, RCU, configuration)

---

## Tasks

### Phase 1: Analysis & Backup

#### Task 1: Create file inventory
**Deliverable:** Two text files listing all source files  
**Files:**
- `.ai-factory/tmp/openssl-winscp-files.txt`
- `.ai-factory/tmp/openssl-netbox-files.txt`

**Steps:**
1. List all files in WinSCP source (exclude `.git/`)
2. List all files in NetBox target
3. Generate comparison report

**Logging:** N/A

---

#### Task 2: Backup current state
**Deliverable:** Git commit point or backup directory  
**Files:** Current `libs/openssl-3/` state

**Steps:**
1. Ensure git repo is clean
2. Create backup commit: `backup: openssl-3 pre-sync from WinSCP`
3. Document current patch status

**Logging:** N/A

---

### Phase 2: File Synchronization

#### Task 3: Copy core source files from WinSCP
**Deliverable:** Updated source files in `libs/openssl-3/`  
**Files to sync:**
- `crypto/` (all subdirectories)
- `ssl/` (all subdirectories)
- `include/` (all subdirectories)
- `providers/` (all subdirectories)

**Exclusions (NetBox specific):**
- `CMakeLists.txt` (keep NetBox version)
- `.gitignore` (keep NetBox version)
- `removed/` (keep NetBox version)
- `0001-openssl-NetBox-patches.patch` (preserve, will re-apply)

**Steps:**
1. Copy `crypto/*` from WinSCP → NetBox (overwrite existing)
2. Copy `ssl/*` from WinSCP → NetBox (overwrite existing)
3. Copy `include/*` from WinSCP → NetBox (overwrite existing)
4. Copy `providers/*` from WinSCP → NetBox (overwrite existing)
5. Copy root docs: `ACKNOWLEDGEMENTS.md`, `AUTHORS.md`, `INSTALL.md`, `LICENSE.txt`, `SUPPORT.md`

**Logging:**
- Log file copy count
- Log any files that fail to copy

---

#### Task 4: Preserve NetBox build configuration
**Deliverable:** Intact `CMakeLists.txt` and build files

**Files to preserve:**
- `libs/openssl-3/CMakeLists.txt` (NetBox build config)
- `libs/openssl-3/.gitignore` (NetBox gitignore)
- `libs/openssl-3/removed/` (NetBox-specific removals tracking)

**Steps:**
1. Verify `CMakeLists.txt` was not overwritten
2. Verify `.gitignore` was not overwritten
3. Verify `removed/` directory structure intact

**Logging:** N/A

---

### Phase 3: Patch Management

#### Task 5: Analyze patch compatibility
**Deliverable:** Patch applicability report  
**File:** `0001-openssl-NetBox-patches.patch`

**Current patches (from patch file):**
1. `crypto/core_namemap.c` - tsan_load cast for MSVC
2. `crypto/cryptlib.c` - OPENSSL_isservice FARPROC type
3. `crypto/objects/obj_dat.c` - TSAN_QUALIFIER LONG type
4. `crypto/property/property.c` - tsan_add cast for MSVC
5. `include/crypto/bn_conf.h` - x64 bitness detection
6. `include/internal/rcu.h` - WIN32 macro guard
7. `include/internal/tsan_assist.h` - _InterlockedExchangeAdd intrinsic
8. `include/openssl/configuration.h` - platform detection, feature flags
9. `ssl/ssl_lib.c` - tsan_load cast
10. `ssl/statem/extensions.c` - tsan_decr type changes

**Steps:**
1. Read current patch file
2. Check if patched files exist in new source
3. Identify files that changed significantly (patch may fail)
4. Document any files that need manual patch updates

**Logging:**
- List files where patch applies cleanly
- List files where patch fails or needs adjustment

---

#### Task 6: Re-apply or recreate patch
**Deliverable:** Updated `0001-openssl-NetBox-patches.patch`

**Steps:**
1. Try applying existing patch: `git apply 0001-openssl-NetBox-patches.patch`
2. If successful: verify changes are present
3. If fails: 
   - Identify rejected hunks
   - Manually create new patch with same logical changes
   - Test new patch applies cleanly

**Logging:**
- Document which hunks failed
- Document resolution approach

---

### Phase 4: Cleanup & Verification

#### Task 7: Identify files for removal
**Deliverable:** List of unused files to remove  
**File:** `.ai-factory/tmp/openssl-removal-candidates.txt`

**Candidates:**
- Files in `removed/` directory (already separated)
- ASM files not referenced in CMakeLists.txt
- Duplicate or obsolete files

**Steps:**
1. Check CMakeLists.txt for referenced source files
2. Identify files not in build (no CMake reference)
3. Cross-reference with WinSCP source (files only in NetBox)
4. Create removal candidate list

**Logging:** N/A

---

#### Task 8: Remove unused files (after confirmation)
**Deliverable:** Cleaned `libs/openssl-3/` directory

**Steps:**
1. Present removal candidate list to user
2. Wait for user confirmation
3. Move files to `removed/` directory (not delete)
4. Update `.gitignore` if needed

**Logging:**
- List files moved to removed/
- Document final directory state

---

#### Task 9: Build verification
**Deliverable:** Successful build or error report

**Steps:**
1. Clean build configuration: `cmake --build build-RelWithDebugInfo --clean-first`
2. Build OpenSSL target only (if possible)
3. Check for compilation errors
4. Check for missing symbols

**Logging:**
- Capture build output
- Document any errors or warnings

---

## Commit Plan

**Commit 1:** Backup current state  
```
backup: openssl-3 pre-sync from WinSCP
- Snapshot before synchronizing with WinSCP upstream
- Source: winscp-master/libs/openssl/
- Target: NetBox-dev/libs/openssl-3/
```

**Commit 2:** Sync core source files  
```
chore(openssl): synchronize source from WinSCP upstream

- Update crypto/, ssl/, include/, providers/ from WinSCP
- Copy documentation: ACKNOWLEDGEMENTS.md, AUTHORS.md, INSTALL.md, LICENSE.txt, SUPPORT.md
- Preserve NetBox CMakeLists.txt and .gitignore
- Source: winscp-master/libs/openssl/
```

**Commit 3:** Update patches  
```
chore(openssl): re-apply NetBox custom patches

- Re-apply 0001-openssl-NetBox-patches.patch after source sync
- Patches cover: MSVC compatibility, platform detection, tsan fixes
- Verify patch applies cleanly to new source revision
```

**Commit 4:** Cleanup unused files  
```
chore(openssl): move unused files to removed/

- Move files not referenced in CMake build to removed/
- Preserve removal history for audit trail
- Update documentation if needed
```

---

## Acceptance Criteria

- [x] All source files match WinSCP upstream version
- [x] Custom patch `0001-openssl-NetBox-patches.patch` applies cleanly
- [x] CMakeLists.txt remains functional (build succeeds)
- [x] NetBox-specific files preserved (CMakeLists.txt, .gitignore, removed/)
- [ ] Unused files identified and moved to removed/ (after confirmation)
- [ ] No unintended modifications to third-party code outside openssl-3/

---

## Notes

**Critical:** Do NOT modify files in `libs/` directly without going through this sync process. All upstream updates should come from WinSCP to maintain consistency.

**Patch Context:** The custom patches are MSVC-specific workarounds for:
- Thread sanitizer atomics on Windows
- Platform detection (x86/x64/ARM64)
- Feature flags for NetBox build configuration
