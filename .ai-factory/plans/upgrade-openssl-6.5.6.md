# Plan: Upgrade OpenSSL from 6.5.5 to 6.5.6

**Branch:** upgrade-openssl-6.5.6  
**Created:** 2026-04-12  
**Settings:** Testing: yes, Logging: verbose, Docs: yes

## Tasks

### Phase 1: Analyze Changes

1. **Analyze OpenSSL 6.5.5 → 6.5.6 diff in WinSCP** - [x]
2. **Identify files that need to be added to CMakeLists.txt** - [x] (0 files needed)
3. **Identify files that can be removed from CMakeLists.txt** - [x] (34 files removed, 2 added: asn1_parse.c, i2d_evp.c)

### Phase 2: Apply Changes

4. **Copy changed source files from WinSCP to NetBox** - [x] (1,232 files updated via robocopy)
5. **Add new files to libs/openssl-3** - [x] (asn1_parse.c, i2d_evp.c added to CMakeLists.txt)
6. **Remove unused files from libs/openssl-3** - [x] (34 files removed from CMakeLists.txt)

### Phase 3: Verification

7. **Verify OpenSSL build succeeds** - [x] (libeay32.lib, ssleay32.lib built successfully)
8. **Build NetBox plugin** - [x] (NetBox.dll 11,954,688 bytes, x64 RelWithDebugInfo)

### Phase 4: Documentation

9. **Document changes** - [x] (DEPENDENCIES.md updated)

## Commit Plan

- **Commit 1:** Phase 1 tasks - Analysis results (mapping files)
- **Commit 2:** Phase 2 tasks - Apply changes (copy, add, remove files)
- **Commit 3:** Phase 3 tasks - Verification (build succeeds)
- **Commit 4:** Phase 4 tasks - Documentation
