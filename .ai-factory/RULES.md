# Project Rules

> Short, actionable rules and conventions for this project. Loaded automatically by /aif-implement.

## Rules

- Always use CRLF line endings when editing files
- Always read AGENTS.md for project rules before making changes
- do not use any Unix-style redirection (`< /dev/null`, `>/dev/null`, `2>/dev/null`) on Windows; use `NUL` or omit
- use skill `caveman` if available
- Use skills if available: `cpp-coding-standards`, `cpp-expert`, `memory-safety-patterns`, `pp-modern-features`, `git-commit`

- After adding/deleting IDs in `src/base/MsgIDs.h`, update **all five** language files at the **same zero-based index**:
  - `src/NetBox/NetBoxEng.lng`
  - `src/NetBox/NetBoxRus.lng`
  - `src/NetBox/NetBoxFr.lng`
  - `src/NetBox/NetBoxPol.lng`
  - `src/NetBox/NetBoxSpa.lng`
  - **Alignment rule:** Every `NB_*` / `MSG_*` enum identifier maps to a quoted string by zero-based index. The Nth enum member must be the Nth quoted string (lines starting with `"`) in every `.lng` file. Blank lines and the `.Language=` header do **not** count.
  - **Verification:** Run `python scripts/verify_lng_alignment.py` before committing. Exit code 0 = aligned; anything else = fix before commit.
  - All `.lng` files must have the **same count of quoted strings** (not necessarily same total line count).
  - Encoding: UTF-8 **without BOM**.
  - Line endings: CRLF with newline at end of file.
  - **Crash risk:** Misaligned strings cause `GetMsg(MsgId)` to return wrong text or crash Far Manager at runtime.

- The code should remain as similar as possible to the source code of the WinSCP project
  - WinSCP code for reference: `https://github.com/winscp/winscp`
  - Follow WinSCP coding patterns and conventions when making changes
  - Keep compatibility with WinSCP codebase for easier upstream merging

- Markdown documentation follows markdownlint rules:
  - MD060: Enabled - No spaces inside code span elements (e.g., `` `code` `` not `` ` code ` ``)
  - MD033: Enabled with allowed elements `["br", "img", "a"]` - Only these HTML elements are permitted
  - Configuration in `.vscode/settings.json` for VS Code users
  

- **CWE-134 Format String Prevention**: Any string argument passed to `FMTLOAD` or `FORMAT` that originates from an untrusted source (remote server response, shell output, redirect URI, user input, or error text from a third-party library) MUST be wrapped with `nb::EscapeFmtChars()` before passing. This prevents `%` characters in untrusted data from being interpreted as `fmt::printf` format specifiers, which causes crashes or information disclosure. When in doubt, escape.