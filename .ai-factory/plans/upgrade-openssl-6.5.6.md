# Plan: Upgrade OpenSSL from 6.5.5 to 6.5.6

**Branch:** upgrade-openssl-6.5.6  
**Created:** 2026-04-12  
**Settings:** Testing: yes, Logging: verbose, Docs: yes

## Tasks

### Phase 1: Analyze Changes

1. **Analyze OpenSSL 6.5.5 → 6.5.6 diff in WinSCP**
   - Get list of all changed files between tags 6.5.5 and 6.5.6 in WinSCP's libs/openssl
   - Filter to only files that exist in libs/openssl-3 (NetBox version)
   - Create a mapping of source file → target file
   - **Logging:** DEBUG output for each file pair mapping

2. **Identify files that need to be added to CMakeLists.txt**
   - Compare list of files in WinSCP's libs/openssl/ vs NetBox's libs/openssl-3
   - Identify new files present in WinSCP but missing in NetBox
   - Filter to only files that are actually used in WinSCP build (not excluded)
   - **Logging:** INFO for each new file identified

3. **Identify files that can be removed from CMakeLists.txt**
   - Find files in NetBox's libs/openssl-3 that are NOT in WinSCP's version
   - Verify these files are truly unused (check if they're compiled in WinSCP)
   - **Logging:** INFO for each file proposed for removal

### Phase 2: Apply Changes

4. **Copy changed source files from WinSCP to NetBox**
   - For each file that exists in both repositories:
     - Copy content from winscp-master/libs/openssl/ to libs/openssl-3/
   - Use git diff to get exact changes
   - **Logging:** DEBUG for each file copied, include diff stats

5. **Add new files to libs/openssl-3**
   - For each new file identified in task 2:
     - Copy file from WinSCP
     - Add to appropriate CRYPTO_SOURCES or SSL_SOURCES list in CMakeLists.txt
   - **Logging:** INFO for each file added

6. **Remove unused files from libs/openssl-3**
   - For each file identified in task 3:
     - Remove from CRYPTO_SOURCES or SSL_SOURCES in CMakeLists.txt
     - Optionally delete the source file if it's truly unused
   - **Logging:** INFO for each file removed

### Phase 3: Verification

7. **Verify OpenSSL build succeeds**
   - Run cmake configuration
   - Build the OpenSSL libraries (libeay32, ssleay32)
   - Fix any compilation errors
   - **Logging:** ERROR for build failures, INFO for success

8. **Build NetBox plugin**
   - Build the full NetBox plugin to verify OpenSSL integration
   - Ensure no linker errors
   - **Logging:** ERROR for build failures, INFO for success

### Phase 4: Documentation

9. **Document changes**
   - Create a summary of what was updated
   - Note any compatibility considerations
   - Update DEPENDENCIES.md if needed
   - **Logging:** INFO for documentation created

## Commit Plan

- **Commit 1:** Phase 1 tasks - Analysis results (mapping files)
- **Commit 2:** Phase 2 tasks - Apply changes (copy, add, remove files)
- **Commit 3:** Phase 3 tasks - Verification (build succeeds)
- **Commit 4:** Phase 4 tasks - Documentation
