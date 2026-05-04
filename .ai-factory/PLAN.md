# Fast Plan: Consolidate TODO Items into Tracked Tasks

## Settings
- **Mode**: Fast
- **Testing**: No
- **Logging**: Verbose
- **Docs**: Yes (inventory document)
- **Milestone**: Technical Debt / Refactoring

## Roadmap Linkage
- **Milestone**: "Technical Debt / Refactoring"
- **Rationale**: Consolidating TODOs into a tracked inventory enables systematic cleanup and prevents TODOs from being forgotten.

## Context
The NetBox `src/` directory contains ~50+ TODO/FIXME/HACK markers scattered across ~25 files. These span NetBox UI (`src/NetBox/`), base utilities (`src/base/`), core protocols (`src/core/`), filezilla integration (`src/filezilla/`), and Windows UI (`src/windows/`). There is already a partial plan at `.ai-factory/plans/implement-todos.md` (2026-04-20) covering 4 TODOs in `src/base/SysUtils.cpp`.

This plan does **not** implement the TODOs — it inventories and catalogs them so they can be scheduled and tracked in future `/aif-plan` / `/aif-implement` cycles.

## Tasks

### Task 1: Create TODO inventory document
- **Files**: `.ai-factory/TODO-INVENTORY.md` (new)
- **Action**: Create a categorized inventory of all TODO/FIXME/HACK markers found in `src/`
- **Categories**:
  - `NetBox UI` — Far dialog, plugin interface, WinSCP dialogs/filesystem
  - `Base/Core` — SysUtils, Classes, Common, nbstring, rtti
  - `Protocols` — Terminal, SFTP, SCP, FTP, WebDAV, S3, Queue
  - `FileZilla` — AsyncSslSocketLayer, FtpControlSocket
  - `Windows UI` — GUI tools, Synchronize controller, VCL, WinConfiguration
- **Columns per entry**: File, Line, Type (TODO/FIXME/HACK), Summary, Priority (High/Medium/Low based on user-facing impact), Existing Plan (if any)
- **Note**: Mark `implement-todos.md` entries as "Plan exists" and cross-reference

### Task 2: Update ROADMAP.md status
### [x] Task 2: Update ROADMAP.md status
- **Action**: Mark `Consolidate TODO items into tracked tasks` as complete (date: 2026-05-04)
- **Reference**: Point to `.ai-factory/TODO-INVENTORY.md` and `.ai-factory/plans/implement-todos.md`

### [x] Task 3: Update implement-todos.md with inventory cross-reference
- **Files**: `.ai-factory/plans/implement-todos.md`
- **Action**: Add a note at the top linking back to the master inventory

## Commit Plan
- Single commit after Task 1 + Task 2 + Task 3
- Commit message: `docs(todo): consolidate TODO items into tracked inventory`

## Next Step
Run `/aif-implement` to execute this plan.
