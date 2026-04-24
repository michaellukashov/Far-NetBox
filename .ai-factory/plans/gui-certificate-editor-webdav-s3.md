<!-- handoff:task: -->
# Implementation Plan: GUI for Editing cacert.pem in WEBDAV/S3 Session Dialog

Branch: none (stored plan)
Created: 2026-04-24

## Settings
- Testing: yes (include unit tests for certificate parsing/validation)
- Logging: verbose (DEBUG logs for file I/O, UI events, certificate validation)
- Docs: yes (update user documentation and changelog)

## Roadmap Linkage
Milestone: "none"
Rationale: Feature not explicitly listed in ROADMAP.md; standalone security/usability enhancement for SSL certificate management.

## Research Context
Source: .ai-factory/RESEARCH.md (Active Summary)
Goal: No active research; this is a new feature implementation.
Constraints:
- Build must pass MSVC W4 with zero warnings
- All modified files use CRLF line endings
- Follow NetBox C++17 standards and T/F prefix naming
- Do not modify any files under libs/
- Plugin DLL output to Far3_<platform>/Plugins/NetBox/ (requires OPT_CREATE_PLUGIN_DIR=ON)
- Avoid Unity build issues (may need OPT_USE_UNITY_BUILD=OFF if symbol redefinition occurs)

## Project-Specific Constraints (from skill-context)
- WinSCP porting: document any WinSCP-originating code patterns and NetBox adaptations
- Dependency flow: changes must respect Plugin → Core → Base → Third-Party layering
- Third-party boundaries: libs/ are immutable; use patch files if upstream changes needed
- CMake awareness: target RelWithDebugInfo, consider x86/x64/ARM64 differences

## Commit Plan
- **Commit 1** (after tasks 1-4): "feat: add certificate editor infrastructure"
- **Commit 2** (after tasks 5-8): "feat: integrate certificate editor into WEBDAV/S3 session dialogs"

## Tasks

### Phase 1: Investigation and Design

- [ ] Task 1: Analyze existing session dialog implementations for WEBDAV and S3 protocols in `src/windows/`. Identify the dialog class, controls layout, and how settings are stored/retrieved. LOGGING: Document findings in a brief report (no code yet). Files: `src/windows/` (search for WebDAV/S3 session dialogs).
- [ ] Task 2: Locate current certificate/SSL handling code in the codebase. Search for OpenSSL integration, certificate loading, and `cacert.pem` references. LOGGING: Log all findings including file paths and function names. Files: `src/core/`, `src/nbcore/`, possibly `libs/openssl-3/` (do not modify).
- [ ] Task 3: Research Far Manager dialog control capabilities (multi-line edit, buttons, file picker). Determine if existing UI utilities can be reused. LOGGING: Record control IDs and styles used in similar dialogs (e.g., proxy settings). Files: `src/windows/controls/`, `src/windows/dialogs/`.
- [ ] Task 4: Design the certificate editor UI component: layout (edit control + Load/Save buttons), keyboard shortcuts, validation feedback. Create a sketch or description. LOGGING: Output design document to console for review. Done when design is approved for implementation.

### Phase 2: Core Certificate Operations

- [ ] Task 5: Implement `nbcore::CertificateManager` class with methods: `LoadFromFile(path)`, `SaveToFile(path, content)`, `ValidatePEM(content)`. Use RAII, proper error handling, and follow NetBox memory safety patterns. LOGGING: DEBUG on entry/exit, INFO on success, WARN on validation issues, ERROR on I/O failures. Files: `src/nbcore/certificate.cpp`, `src/include/certificate.hpp`. Unit tests: `tests/nbcore/CertificateTest.cpp`.
- [ ] Task 6: Add certificate editor control as a reusable dialog page or embedded panel. Implement Load/Save button handlers, file open/save dialogs (use FarManager services), and status feedback. LOGGING: Each button click logs action and result (success/failure with reason). Files: `src/windows/CertificateEditor.cpp`, `src/windows/CertificateEditor.hpp`.
- [ ] Task 7: Integrate certificate management with session configuration storage: map session settings to certificate file path and content. Ensure persistence when dialog is accepted. LOGGING: Log when settings are read from/written to session config, including session name and protocol. Files: Modify `src/core/session_*.cpp` (WEBDAV/S3), `src/windows/` session dialog class.

### Phase 3: Integration and Testing

- [ ] Task 8: Wire the certificate editor into the WEBDAV and S3 session properties dialogs. Add a new tab or section accessible via a "Certificates" button or similar. Ensure correct initialization (load existing certificate) and cleanup. LOGGING: Log dialog initialization, editor show/hide events, and final save/cancel actions. Files: `src/windows/WebDAVSessionDialog.cpp`, `src/windows/S3SessionDialog.cpp`, and their headers.
- [ ] Task 9: Build the solution in RelWithDebugInfo configuration. Fix any warnings (treat warnings as errors). Verify plugin DLL appears in `Far3_x64/Plugins/NetBox/` (or appropriate platform). LOGGING: Record build command and output. Files: CMakeLists.txt if configuration changes needed (e.g., add new source files).
- [ ] Task 10: Manual testing: create a WEBDAV/S3 session, edit a certificate, save, and verify persistence. Test error paths (invalid PEM, read-only file). LOGGING: Use verbose logging during test; capture logs to file for review. No new files; use existing test sessions.
- [ ] Task 11: Update documentation: describe the new certificate editor in the user guide (docs/), and add a changelog entry. LOGGING: Commit message must reference docs changes. Files: `docs/user-guide/sessions.md` (or equivalent), `CHANGELOG.md`.

## Dependencies
- Task 5 depends on Task 2 (need to know existing SSL/certificate handling)
- Task 7 depends on Task 5 (need CertificateManager)
- Task 8 depends on Task 6 (editor control) and Task 7 (config integration)
- Task 9 depends on all implementation tasks (5-8)

## Success Criteria
- All tasks complete and build passes with zero warnings
- Certificate editor loads/saves PEM files correctly
- Session settings persist across dialog invocations
- Unit tests for CertificateManager pass
- Documentation updated and visible in built plugin help (if applicable)
