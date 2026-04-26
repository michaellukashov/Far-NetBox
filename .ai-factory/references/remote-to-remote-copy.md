# Remote-to-Remote Copy

Far-NetBox supports copying files directly between remote directories without downloading them to the local machine first. This feature is available for all supported protocols.

## Supported Protocols

All protocols support remote-to-remote copy:

| Protocol | Implementation | Added | Method |
|----------|---------------|-------|--------|
| **SCP** | `TSCPFileSystem::CopyFile()` | 2011-2014 | Uses `cp -r` command with `-T` flag |
| **SFTP** | `TSFTPFileSystem::CopyFile()` | 2023-06-11 | Uses SFTP protocol extensions (`copy-file` or `copy-data`) |
| **WebDAV** | `TWebDAVFileSystem::CopyFile()` | 2017-03-19 | Uses HTTP COPY method |
| **S3** | `TS3FileSystem::CopyFile()` | 2018-01-19 | Uses S3 CopyObject API |

## How to Use

1. Connect to a remote server using any supported protocol
2. Navigate to the source file or directory
3. Press **F5** (Copy) or **F6** (Move)
4. Enter the destination path on the same remote server
5. The file will be copied directly on the remote server

## Benefits

- **Faster transfers** - No download/upload cycle required
- **Bandwidth savings** - Data stays on the remote server
- **Preserves metadata** - File permissions, timestamps, and attributes are maintained
- **Works with large files** - No local disk space needed

## Protocol-Specific Details

### SCP

Uses the standard Unix `cp` command on the remote server:
```bash
cp -r -T "source" "destination"
```

The `-T` flag treats the destination as a normal file (GNU coreutils). Falls back to `cp -r` if `-T` is not supported.

### SFTP

Uses SFTP protocol extensions when available:
- **copy-file extension** - Direct server-side copy (preferred)
- **copy-data extension** - Fallback method using file handles

Both methods preserve file attributes (permissions, timestamps).

### WebDAV

Uses the HTTP COPY method defined in RFC 4918:
```http
COPY /source HTTP/1.1
Destination: /destination
Overwrite: F
```

Supports both files and collections (directories).

### S3

Uses the S3 CopyObject API:
```c
S3_copy_object(source_bucket, source_key, dest_bucket, dest_key, ...)
```

Works across buckets and within the same bucket. Preserves object metadata.

## Limitations

- **Cross-protocol copy not supported** - Source and destination must use the same protocol
- **Cross-server copy not supported** - Source and destination must be on the same server
- **Directory copy** - Some protocols may have limitations on recursive directory copy

## Troubleshooting

### SCP: "cp: invalid option -- 'T'"

The remote server uses a non-GNU version of `cp` that doesn't support the `-T` flag. NetBox automatically falls back to `cp -r` without the flag.

### SFTP: "Operation not supported"

The SFTP server doesn't support the `copy-file` or `copy-data` extensions. NetBox will fall back to download/upload cycle.

### WebDAV: "Method not allowed"

The WebDAV server doesn't support the COPY method. Check server configuration or use a different protocol.

### S3: "Access denied"

Ensure your IAM policy includes the `s3:GetObject` and `s3:PutObject` permissions for both source and destination.

## See Also

- [Protocol Support](../README.md)
- [File Operations](../README.md)
- [Performance Tips](../README.md)
