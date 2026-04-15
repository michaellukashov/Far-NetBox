# Documentation Style Guide

This document defines the standards for writing and maintaining documentation in the NetBox project.

## File Format

- All documentation files must use **UTF-8 encoding without BOM**
- All documentation files must use **CRLF line endings**
- All documentation files must end with a single blank line

## Writing Standards

- Use **clear, concise language** - avoid unnecessary jargon
- Use **imperative mood** for instructions ("Use", "Do", "Avoid")
- Use **consistent terminology** - define terms when first used
- Use **code blocks** for all commands, paths, and code snippets
- Use **backticks** for inline code: `command`, `path`, `variable`
- Use **bold** for emphasis and **italics** for definitions
- Use **bullet points** for lists of items
- Use **tables** for comparisons and reference data

## Tool Usage

- Use `read` to inspect file content - never use `cat`, `type`, or `more`
- Use `grep` to search content - never use `findstr`, `find`, or `grep` in shell
- Use `edit` to modify files - never use `sed`, `awk`, or manual text replacement
- Use `find` to locate files - never use `dir` or `ls`
- Use `bash` only for simple commands - never use for file operations

## Cross-References

- Always use the format: "See [file] → \"section\""
- Always verify that cross-references are valid
- Never use relative paths in cross-references

## Review Process

- All documentation changes must be reviewed by at least one other team member
- Documentation is as important as code - treat it with the same rigor
- Update documentation when code changes - never let them diverge

## Maintenance

- Keep documentation up-to-date with the latest code changes
- Remove outdated or redundant information
- Add new documentation when new features are added

## Version Control

- All documentation files are tracked in Git
- Documentation changes should be committed with the code changes they relate to
- Use conventional commit messages for documentation changes: `docs: description`

## Common Mistakes to Avoid

- Using `read_file` or `write_file` (use `read` and `write` instead)
- Using PowerShell for build operations (always use .bat files)
- Using backslashes in paths (use forward slashes: `src/NetBox/`)
- Using inconsistent formatting for code blocks
- Adding duplicate information across multiple files
- Not verifying cross-references after editing

This guide should be referenced by all contributors when writing or reviewing documentation.