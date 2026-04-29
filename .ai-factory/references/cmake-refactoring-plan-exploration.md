# CMake Refactoring Plan Exploration

**Date:** 2026-04-29
**Scope:** Reconcile `.ai-factory/plans/REFACTORING_PLAN.md` against actual codebase state after multi-phase CMake modularization
**Triggered by:** `/aif-improve @.ai-factory/plans/REFACTORING_PLAN.md`

---

## Findings Summary

The plan claimed a 7-phase refactoring (Phases 1-7, all completed 2025-01-15) but documented an **intermediate state** rather than the final codebase. Four infrastructure modules and significant line-count corrections were missing from the plan.

---

## Missing Documented Modules

### 1. `cmake/Libraries.cmake` (Infrastructure)

- **Status:** Present in codebase, absent from plan
- **Purpose:** Centralizes all `add_subdirectory()` calls for 11 libraries via `netbox_configure_libraries()` and `netbox_print_library_summary()`
- **Lines:** 71
- **Impact:** Replaced scattered `add_subdirectory()` calls in main `CMakeLists.txt` with a single function call
- **Phase:** Should be documented under Phase 1 (Infrastructure Setup)

### 2. `cmake/PlatformDetection.cmake` (Infrastructure)

- **Status:** Present in codebase, absent from plan
- **Purpose:** Auto-detects target platform (x86, x64, ARM64) and sets `PROJECT_PLATFORM` cache variable
- **Lines:** 21
- **Function:** `netbox_detect_platform()`
- **Phase:** Should be documented under Phase 1 (Infrastructure Setup)

### 3. `cmake/SourceGroups.cmake` (Phase 4 Extension)

- **Status:** Present in codebase, documented as inline content of `src/CMakeLists.txt` in plan
- **Purpose:** Centralizes all source file group definitions (`netbox_define_base_sources`, `netbox_define_filezilla_sources`, `netbox_define_core_sources`, `netbox_define_windows_sources`, `netbox_define_netbox_sources`, `netbox_define_resource_sources`)
- **Lines:** 232
- **Impact:** The plan claimed `src/CMakeLists.txt` contained ~340 lines of source definitions; in reality they were extracted to this module

### 4. `cmake/TargetConfiguration.cmake` (Phase 4 Extension)

- **Status:** Present in codebase, completely absent from plan
- **Purpose:** Centralizes target configuration (`netbox_configure_target`) dispatching include directories, library dependencies, compiler options, and platform settings
- **Lines:** 147
- **Functions:**
  - `netbox_configure_target(TARGET)`
  - `netbox_configure_include_directories(TARGET)`
  - `netbox_configure_library_dependencies(TARGET)`
  - `netbox_configure_compiler_options(TARGET)`
  - `netbox_configure_platform_settings(TARGET)`

---

## Line Count Corrections

| File | Plan Claimed | Actual | Delta |
|------|------------|--------|-------|
| `CMakeLists.txt` | 102 lines | **81 lines** | -21 |
| `src/CMakeLists.txt` | 340 / 530 lines | **91 lines** | -259 to -439 |
| `cmake/NetBox.cmake` | ~200 lines | **366 lines** | +166 |
| `cmake/Install.cmake` | ~60 lines | **109 lines** | +49 |
| `cmake/README.md` | ~120 lines | **189 lines** | +69 |
| `cmake/` modules | 13 files | **17 files** | +4 |

**Actual total reduction:** ~2397 lines (2478 -> 81) = **97% reduction** in main `CMakeLists.txt`.

---

## Structural Changes Not Documented

### `baselib` Target Removed

- Plan documented `baselib` as a separate STATIC target (~3 + ~25 files)
- Actual code merges all base sources directly into `NetBox` SHARED target
- No `baselib` target exists in current `src/CMakeLists.txt`

### `libs/putty/` Case Sensitivity

- Plan documented `libs/PuTTY/CMakeLists.txt` (mixed case)
- Actual filesystem path is `libs/putty/` (all lowercase)
- Critical for case-sensitive build environments (Linux cross-compilation, CI containers)

### `libs/tinylog.backup/` Leftover

- Stale backup directory exists with complete `CMakeLists.txt` and `src/` subtree
- Not referenced by `cmake/Libraries.cmake`
- Should be removed to prevent confusion

---

## Files Examined

| File | Purpose | Lines |
|------|---------|-------|
| `CMakeLists.txt` | Main entry point | 81 |
| `src/CMakeLists.txt` | Plugin orchestrator | 91 |
| `cmake/Libraries.cmake` | Library subdirectory management | 71 |
| `cmake/PlatformDetection.cmake` | Platform auto-detection | 21 |
| `cmake/SourceGroups.cmake` | Source file group definitions | 232 |
| `cmake/TargetConfiguration.cmake` | Target config (includes, libs, flags) | 147 |
| `cmake/NetBox.cmake` | Compiler/linker flags and options | 366 |
| `cmake/Install.cmake` | Post-build installation logic | 109 |
| `cmake/README.md` | CMake module documentation | 189 |
| `libs/openssl-3/CMakeLists.txt` | OpenSSL build config | 931 |
| `libs/putty/CMakeLists.txt` | PuTTY build config | 17 |
| `libs/neon/CMakeLists.txt` | neon build config | 13 |
| `libs/zlib-ng/CMakeLists.txt` | zlib-ng build config | 14 |
| `libs/*/CMakeLists.txt` | Other 8 library configs | 11-14 each |

---

## Recommendations

1. **Keep plan in sync with codebase** — After any future refactoring, update the plan file to reflect actual file states, not intermediate targets.
2. **Remove `libs/tinylog.backup/`** — Safe deletion; not referenced by build system.
3. **Standardize path casing in docs** — Use actual lowercase `libs/putty/` to prevent build failures on case-sensitive systems.
4. **Document orchestrator pattern** — `src/CMakeLists.txt` is a lean orchestrator (91 lines) delegating to modules; emphasize this pattern over line-count claims.
