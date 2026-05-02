# Project Rules

> Short, actionable rules and conventions for this project. Loaded automatically by /aif-implement.

## Rules

- Always use CRLF line endings when editing files
- Always read AGENTS.md for project rules before making changes
- do not use any Unix-style redirection (`< /dev/null`, `>/dev/null`, `2>/dev/null`) on Windows; use `NUL` or omit
- use skill `caveman` if available
- Use skills if available: `cpp-coding-standards`, `cpp-expert`, `memory-safety-patterns`, `pp-modern-features`, `git-commit`

- After adding/deleting IDs in `src/base/MsgIDs.h`, update all language files:
  - `src/NetBox/NetBoxEng.lng`: add English message
  - `src/NetBox/NetBoxRus.lng`: add Russian translation
  - and so on for all `*.lng` files
  - All `*.lng` files must have the same number of lines
  - All `*.lng` files must have Encoding: UTF-8 with BOM
  - Ensure proper line endings (CRLF) and newline at end of file

- The code should remain as similar as possible to the source code of the WinSCP project
  - WinSCP code for reference: `https://github.com/winscp/winscp`
  - Follow WinSCP coding patterns and conventions when making changes
  - Keep compatibility with WinSCP codebase for easier upstream merging

- Markdown documentation follows markdownlint rules:
  - MD060: Enabled - No spaces inside code span elements (e.g., `` `code` `` not `` ` code ` ``)
  - MD033: Enabled with allowed elements `["br", "img", "a"]` - Only these HTML elements are permitted
  - Configuration in `.vscode/settings.json` for VS Code users
  

- **CWE-134 Format String Prevention**: Any string argument passed to `FMTLOAD` or `FORMAT` that originates from an untrusted source (remote server response, shell output, redirect URI, user input, or error text from a third-party library) MUST be wrapped with `nb::EscapeFmtChars()` before passing. This prevents `%` characters in untrusted data from being interpreted as `fmt::printf` format specifiers, which causes crashes or information disclosure. When in doubt, escape.