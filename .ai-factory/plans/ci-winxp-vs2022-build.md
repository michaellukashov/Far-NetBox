# Implementation Plan: GitHub Actions WinXP Build with VS2022 (v143 + Subsystem 5.01)

**Branch:** feature/ci-winxp-vs2022-build
**Created:** 2026-04-29
**Mode:** Full

---

## Problem Statement

The existing `release.yml` builds x64, x86, and ARM64 on the `windows-2022` runner using the default v143 toolset. While `cmake/NetBox.cmake` sets `_WIN32_WINNT=0x0501` and `src/NetBox/vc_crt_fix_impl.cpp` provides XP API shims, the linker flag `/SUBSYSTEM:WINDOWS` (line 223) omits a version suffix, causing the linker to default to subsystem `6.00` (Vista+) on modern toolsets. This prevents the resulting x86 binary from loading on Windows XP regardless of the API shims.

Additionally, the v141_xp toolset (also known as `vs14_xp`) — which historically automated the XP subsystem version — is **not available in Visual Studio 2022** and cannot be installed on the `windows-2022` GitHub Actions runner. Microsoft removed Windows XP targeting support from VS2022.

**Goal:** Add a dedicated WinXP x86 build job to the GitHub Actions `release.yml` workflow that produces a correctly labeled artifact with subsystem version `5.01`, verifies binary compatibility via `dumpbin`, and gracefully handles the unavailable v141_xp toolset by falling back to the v143 toolset with manual linker flags.

---

## Settings

| Setting | Value |
|---------|-------|
| **Testing** | Yes — manual verification of binary compatibility via `dumpbin` |
| **Logging** | Verbose — full CMake configure logs, build step diagnostics, dumpbin output |
| **Docs** | Yes — update `docs/contributing.md` with WinXP CI build notes |

---

## Roadmap Linkage

**Milestone:** Version 1.3 — Win32/KiTTY input mode, WinXP compatible builds

**Rationale:** This CI workflow is a prerequisite for reliably producing WinXP-compatible release binaries. Without automated WinXP builds, compatibility fixes (like `vc_crt_fix_impl.cpp`) cannot be validated in CI.

---

## Research Context

**Active Summary (input for /aif-plan):**
- Topic: GitHub Actions workflow to build WinXP-compatible x86 binaries
- Existing `release.yml` uses `ilammy/msvc-dev-cmd@v1` + Ninja generator for x64/x86/ARM64
- `cmake/NetBox.cmake` already sets `_WIN32_WINNT=0x0501`
- `cmake/NetBox.cmake:223` sets `/SUBSYSTEM:WINDOWS` without version — defaults to `6.00` on modern toolsets
- `src/NetBox/vc_crt_fix_impl.cpp` provides XP API shims (ported from Far3)
- VS2022 `windows-2022` runner does **not** include v141_xp; Microsoft removed XP targeting from VS2022
- v141 (without _xp) may be available as optional component but does **not** auto-set subsystem `5.01`
- WinXP build is x86-only; x64 WinXP is out of scope
- AppVeyor already builds x86 with v143 (default toolset) via `vcvarsall.bat amd64_x86`
- `ilammy/msvc-dev-cmd` supports `toolset: 14.1` parameter for v141 selection; falls back to v143 if unavailable
---

## Tasks

### Phase 1: Build System Fix

- [x] **Task 1: Fix `/SUBSYSTEM:WINDOWS,5.01` in `cmake/NetBox.cmake`**
  - **File:** `cmake/NetBox.cmake`
  - **Goal:** Ensure x86 builds link with subsystem version `5.01` for Windows XP compatibility
  - **Change:**
    - Modify `NETBOX_DLL_LINK_FLAGS` to conditionally append `,5.01` when `PROJECT_PLATFORM STREQUAL "x86"`
    - Result: `/SUBSYSTEM:WINDOWS,5.01` for x86; `/SUBSYSTEM:WINDOWS` unchanged for x64/ARM64
  - **Verification:** After change, build x86 locally and run `dumpbin /headers src/NetBox.dll | findstr /i "subsystem"` — expect `5.01`
  - **Blocked by:** None

### Phase 2: CI Workflow — WinXP Build Job

- [x] **Task 2: Add WinXP x86 build matrix entry to `release.yml`**
  - **File:** `.github/workflows/release.yml`
  - **Goal:** Add a dedicated `cl_x86_winxp_release` job that produces a WinXP-labeled artifact
  - **Matrix entry:**
    - `build: cl_x86_winxp_release`
    - `arch: amd64_x86`
    - `platform: x86`
    - `toolset: 14.1` (v141, preferred; falls back to v143 if unavailable)
  - **Notes:**
    - Add `toolset: default` to existing x64/x86/ARM64 `include:` entries for clarity
    - Existing jobs remain unchanged except for the added `toolset` field
  - **Blocked by:** Task 1

- [x] **Task 3: Detect v141 availability and conditionally select toolset in CI**
  - **File:** `.github/workflows/release.yml`
  - **Goal:** Try v141 first; if unavailable, fall back to v143 with a warning
  - **Approach:**
    - Update `ilammy/msvc-dev-cmd@v1` step to pass `toolset: ${{ matrix.toolset }}`
    - Add `continue-on-error: true` to the v141 setup step
    - Add a follow-up step that detects whether v141 was successfully activated (check `cl.exe -Bv` output)
    - If v141 is not available, emit `::warning::v141 toolset not available; falling back to v143` and proceed with v143
  - **Logging:** Echo `cl.exe -Bv` output to the GitHub Actions log for diagnostics
  - **Blocked by:** Task 2

- [x] **Task 4: Configure CMake with isolated build directory for WinXP job**
  - **File:** `.github/workflows/release.yml`
  - **Goal:** Prevent CMake cache conflicts between Ninja (existing jobs) and any VS-generator fallback
  - **Changes:**
    - Use `build-${{ matrix.build }}` as the build directory instead of hardcoded `build/`
    - Update the Cache CMake step to use the same dynamic path
    - Keep existing `-G "Ninja"` for all jobs (v143 fallback also uses Ninja; no VS generator needed)
    - Keep `-DCMAKE_BUILD_TYPE=${{ env.BUILD_TYPE }}` (Ninja is single-config)
    - Keep `-DOPT_CREATE_PLUGIN_DIR=ON` and `-DPROJECT_PLATFORM=x86`
  - **Blocked by:** Task 3

- [x] **Task 5: Add binary compatibility verification step**
  - **File:** `.github/workflows/release.yml`
  - **Goal:** After the WinXP build, verify the DLL is XP-compatible
  - **Checks:**
    1. `dumpbin /headers Far3_x86/Plugins/NetBox/NetBox.dll | findstr /i "subsystem"` — verify `5.01` (not `6.00`)
    2. `dumpbin /dependents Far3_x86/Plugins/NetBox/NetBox.dll` — check no Vista+ system DLL dependencies
    3. `dumpbin /imports Far3_x86/Plugins/NetBox/NetBox.dll | findstr /i "api-ms-win-core"` — verify no API set dependencies
  - **Failure condition:** If subsystem version is not `5.01`, fail the build step
  - **Logging:** Print full `dumpbin /headers` output for diagnostics
  - **Blocked by:** Task 4

### Phase 3: Artifacts and Release

- [x] **Task 6: Package WinXP-specific artifacts**
  - **File:** `.github/workflows/release.yml`
  - **Goal:** Produce a separately named artifact for the WinXP build
  - **Artifact naming:**
    - Main: `NetBox.WinXP.x86.${version}.7z`
    - PDB: `NetBox.WinXP.x86.${version}.pdb.7z`
  - **Upload:** Use `actions/upload-artifact@v5` with `name: FarNetBox.WinXP.x86`
  - **Blocked by:** Task 5

- [x] **Task 7: Include WinXP artifacts in release job**
  - **File:** `.github/workflows/release.yml`
  - **Goal:** Update the `create-release` job to also download and include WinXP artifacts
  - **Changes:**
    - Update `download-artifact` pattern `FarNetBox.*` already matches `FarNetBox.WinXP.x86`; verify this
    - Ensure WinXP archives are included in the `gh release create` command
  - **Blocked by:** Task 6

### Phase 4: Documentation

- [x] **Task 8: Document WinXP CI build in contributing guide**
  - **File:** `docs/contributing.md`
  - **Goal:** Add a section explaining the WinXP build configuration
  - **Content:**
    - Why WinXP compatibility matters (Far Manager ecosystem)
    - v141_xp unavailability in VS2022 and the v143 fallback strategy
    - How the `/SUBSYSTEM:WINDOWS,5.01` linker flag works
    - Where to find WinXP artifacts in GitHub Releases
    - How to verify XP compatibility locally (`dumpbin /headers`)
  - **Blocked by:** Task 7
---

## Commit Plan

| Checkpoint | Tasks | Message |
|------------|-------|---------|
| Checkpoint 1 | 1–3 | `build(winxp): fix subsystem version and add WinXP CI matrix entry` |
| Checkpoint 2 | 4–6 | `ci(winxp): add toolset detection, build isolation, and binary verification` |
| Checkpoint 3 | 7–8 | `ci(winxp): package WinXP artifacts and document build configuration` |
---

## Risks & Mitigations

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| v141 toolset not available on windows-2022 runner | Medium | `ilammy/msvc-dev-cmd` with `toolset: 14.1` and `continue-on-error`; fallback to v143 with manual `/SUBSYSTEM:WINDOWS,5.01` |
| v143 CRT uses Vista+ APIs not shimmed by `vc_crt_fix_impl.cpp` | Medium | Verify via `dumpbin /imports`; if found, document as known limitation or add shim |
| CMake cache conflict between matrix jobs | Low | Use `build-${{ matrix.build }}` for isolated directories; update cache key accordingly |
| Existing matrix `include:` conflicts with new entry | Low | Add explicit `toolset: default` to existing entries; new entry uses `toolset: 14.1` |
| `dumpbin` not found on runner | Low | `ilammy/msvc-dev-cmd` already sets up PATH; verify with `where dumpbin` step |

---

## Notes

- **CMake source modification required** — Task 1 modifies `cmake/NetBox.cmake` to add `,5.01` subsystem version for x86 builds
- **No modifications to `libs/` or third-party source code** — this is a build-system and CI configuration change
- **Unity build:** Keep `OPT_USE_UNITY_BUILD=OFF` for the WinXP build (same as existing)
- **Build type:** Use `RelWithDebInfo` (same as existing)
- **Only x86 Win32:** WinXP x64 builds are intentionally out of scope
- **All jobs use Ninja generator** — Even the v141 toolset (if available) is used with `-G "Ninja"`; no Visual Studio generator needed
- **`ilammy/msvc-dev-cmd` toolset parameter:** Pass `toolset: ${{ matrix.toolset }}` to the action; `14.1` selects v141, `14.3` (or omitted) selects v143
- **AppVeyor already builds x86 with v143** — The existing `appveyor.yml` uses the default toolset; this plan adds the subsystem fix and CI verification that were previously missing