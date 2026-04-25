---
name: aif-commit
---

Always use English to generate commit messages.
Use structured commits.
Use skill `git-commit` if available.

## Windows cmd.exe commit rules

**CRITICAL:** When running on Windows (`win32`), `git commit` messages MUST avoid multi-line strings in a single `-m` flag.
The `cmd.exe /c` shell parses quotes incorrectly, treating body words as file pathspecs (error: `pathspec 'word' did not match any file(s) known to git`).

### Rules

1. **Single-line commits** — use `-m "type(scope): subject"` with NO body. This always works.
   ```
   git commit -m "ci(release): improve release workflow"
   ```

2. **Multi-line commit**: Use `git commit -m "title{CRLF}line1{CRLF}line2"` instead of heredoc
   ```

3. **NEVER** use escaped quotes (`\"`) inside `-m` on Windows — they break `cmd.exe` parsing.

4. **Alternative:** If the message is complex, write it to a temp file and use `git commit -F <file>`.

### Correct pattern for this skill

When committing:
```
git commit -m "<subject>" -m "title{CRLF}line1{CRLF}line2"
```
Or if the body is long, prefer subject-line only and let the diff speak for itself.
