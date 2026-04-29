# Manual Integration Test: TKeepAliveThread

## Prerequisites

- NetBox plugin built with `OPT_CREATE_PLUGIN_DIR=ON`
- Far Manager 3.0 x64 installed
- Access to an SFTP server that times out idle sessions after <60 seconds
- Debug build or RelWithDebugInfo with tinylog enabled

## Test Scenarios

### Scenario 1: Keepalive Enabled (PingInterval = 30s)

1. Open Far Manager, press F11, select NetBox
2. Create or edit an SFTP session
3. Set **Ping interval** to `30` seconds (Settings → Connection → Keepalives)
4. Connect to the SFTP server
5. Leave the session idle (no file operations) for **60 seconds**
6. Verify the session remains connected (no disconnect message)
7. Check logs at `%TEMP%\netbox-dbglog.txt` for:
   - `Keepalive thread created, interval=30s`
   - Periodic keepalive activity (protocol-dependent)
8. Disconnect via F7 or panel change
9. Verify no crash

**Expected Result:** Session stays alive; no crash on disconnect.

### Scenario 2: Keepalive Disabled (PingInterval = 0)

1. Open Far Manager, press F11, select NetBox
2. Create or edit an SFTP session
3. Set **Ping interval** to `0` (disabled)
4. Connect to the SFTP server
5. Check logs at `%TEMP%\netbox-dbglog.txt` for:
   - `Keepalive thread skipped, interval=0 or already running`
6. Leave idle for 60 seconds
7. Verify session either stays alive (server doesn't timeout) or disconnects cleanly
8. Disconnect

**Expected Result:** No keepalive thread created; no crash.

### Scenario 3: Rapid Connect/Disconnect Cycles

1. Connect to SFTP server with PingInterval = 30s
2. Immediately disconnect (within 5 seconds)
3. Repeat 5 times
4. Verify no crash or deadlock

**Expected Result:** No crash; thread terminates cleanly each time.

### Scenario 4: File Transfer with Keepalive Background Thread

1. Connect to SFTP server with PingInterval = 30s
2. Start a large file upload/download
3. During transfer, verify keepalive thread does not interfere
4. After transfer completes, leave idle for 60 seconds
5. Verify session remains connected

**Expected Result:** No interference between keepalive thread and transfer; session stays alive after transfer.

## Log Verification

Search logs for these expected messages:

| Message | Indicates |
|---------|-----------|
| `Keepalive thread created, interval=Ns` | Thread started successfully |
| `Keepalive thread skipped, interval=0 or already running` | Thread not needed or already exists |
| `Keepalive thread destroyed` | Thread terminated on disconnect |
| `Keepalive thread caught exception:` | Exception guard working (should NOT appear in normal operation) |

## Regression Check

- Open file via Enter (Edit mode), close editor, open second file WITHOUT Ctrl+R
- Verify no crash (regression for issue #508)
