# FTP vsftpd Directory Listing Hang / File Misidentification — Issue #507

## Summary

Fixed FTP directory listing behavior with vsftpd servers where files could be incorrectly treated as directories, causing slow or hanging listings.

## Root Cause Analysis

The primary issues were in the FTP listing parser (`CFtpListResult`) and pipeline:

1. **`parseAsUnix` missing `bUnsure` reset**: All other parsers (`parseAsMlsd`, `parseAsDos`, `parseAsEPLF`, `parseAsVMS`, etc.) reset `direntry.bUnsure = FALSE` on success, but `parseAsUnix` did not. This left `bUnsure = TRUE` (from default constructor) after parsing, which could affect downstream behavior in the FileZilla control layer.

2. **`parseLine` not resetting `direntry` state**: `parseLine` only reset `ownergroup`, `owner`, and `group` before trying sub-parsers. If a parser partially modified `direntry` (e.g., set `dir = TRUE`) and then failed, the next parser would inherit stale values, causing cross-contamination.

3. **Defensive coding gaps**: `parseAsMlsd` correctly reset `dir = FALSE` at the start, but did not explicitly handle `type=file` in the `type` fact switch. Added explicit `dir = FALSE` for `type=file` as defensive coding.

## Changes Made

### `src/filezilla/FtpListResult.cpp`

- **`parseLine`**: Added full `direntry` reset using default constructor before calling sub-parsers. Added `parserName` out parameter to track which parser succeeded.
- **`AddData`**: Added verbose debug logging for first 20 successfully parsed entries (parser name, `dir`, `link`, `size`, `name`) and parse failure warnings.
- **`parseAsUnix`**: Added `direntry.bUnsure = FALSE` to match all other parsers.
- **`parseAsMlsd`**: Added explicit `direntry.dir = FALSE` for `type=file` fact.

### `src/core/FtpFileSystem.cpp`

- **`DoReadDirectory`**: Added INFO-level logging for listing start (MLSD mode) and end (entry count).
- **`HandleListData`**: Added DEBUG-level logging for each received `TListDataEntry` (`Name`, `Dir`, `Link`, `Size`, `Permissions`). Added file-extension heuristic guard: if an entry is marked `Dir=true` but has a known file extension (e.g., `.exe`, `.zip`, `.txt`), log a warning and force `SetType(FILETYPE_DEFAULT)`.
- **`ReadDirectory`**: Added path-based heuristic guard: if `ReadDirectory` is called on a path whose last component has a known file extension, log a warning and return early to prevent recursive directory reads on misclassified files.

## Parser Ordering Verification

- `parseAsDos` and `parseAsOther` both reject lines starting with Unix permission characters (`b`, `c`, `d`, `l`, `p`, `s`, `-`).
- Standard vsftpd LIST output (starting with `-` or `d`) is correctly passed to `parseAsUnix`.
- No parser ordering changes were needed.

## Testing Recommendations

1. Connect to a vsftpd server and browse directories with mixed files and subdirectories.
2. Enable debug logging (`Log Protocol >= 2`) and verify parser names and `dir` flags in logs.
3. Verify that files with extensions (e.g., `test.txt`, `setup.exe`) are not displayed as directories.
4. Verify that actual directories with dots in their names (e.g., `node_modules`, `src.test`) are still correctly displayed as directories.

## References

- GitHub Issue [#507](https://github.com/michaellukashov/Far-NetBox/issues/507)
- `src/filezilla/FtpListResult.cpp` — `parseLine`, `parseAsUnix`, `parseAsMlsd`, `AddData`
- `src/core/FtpFileSystem.cpp` — `ReadDirectory`, `DoReadDirectory`, `HandleListData`
