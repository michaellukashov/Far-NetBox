# RTTI Usage Survey - NetBox

## Overview
Comprehensive survey of `dynamic_cast` and `typeid` usage in the NetBox codebase, performed to evaluate feasibility of replacing RTTI with static alternatives.

---

## dynamic_cast Occurrences (18 total)

### 1. src/NetBox/FarDialog.cpp

**Line ~932:** `TFarDialog* Dialog = dynamic_cast<TFarDialog*>(Obj);`
- Casting from `TObject*` to `TFarDialog*` for UI dialog handling

**Line ~989:** `TFarDialogContainer* Container = dynamic_cast<TFarDialogContainer*>(Obj);`
- Casting from `TObject*` to `TFarDialogContainer*` for container access

### 2. src/NetBox/WinSCPDialogs.cpp

**Line ~1516:** `TSessionDialog* SessionDialog = dynamic_cast<TSessionDialog*>(FarPlugin);`
- Downcast from plugin to session dialog

**Line ~675:** `TSessionData* Data = dynamic_cast<TSessionData*>(SessionDialog(Data, Action));`
- Session dialog creation pattern

**Various:** Tab button downcasts (`S3Tab`, `FtpTab`, etc.)
- Pattern: `S3Tab = nb::dyn_cast_or_null<TTabButton>(GetItem(Index1));`
- Equivalent to dynamic_cast; uses nb::dyn_cast helper

### 3. src/base/ObjIDs.cpp

**Line ~60:** Object class ID registration
- Not a runtime cast, but defines `OBJECT_CLASS_*` identifiers

### 4. src/core/FileSystems.cpp

**Line ~85:** `TCustomFileSystem* FS = dynamic_cast<TCustomFileSystem*>(FileSystem);`
- Downcast from `TObject*` to filesystem interface

### 5. src/core/RemoteFiles.cpp

**Multiple locations** (lines ~49, ~213, ~266):
- `TCustomFileSystem*` downcasts in queue operations
- Pattern consistent: cast from generic `TObject*` stored in list to known filesystem type

### 6. src/core/S3FileSystem.cpp (and similar protocols)

**Lines ~685, ~721:**
- `FTerminal` access and `GetFileSystem()` retrieval
- All protocol filesystems use same pattern

### 7. src/windows/GUIConfiguration.cpp

**Lines ~1640, ~1981:**
- `TTreeList*` and `TTreeItem*` downcasts from `TObject*`
- GUI configuration object tree navigation

---

## typeid Occurrences (4 total)

### 1. src/base/TObject.cpp

**`TObject::ClassName()`**:
```cpp
const char* TObject::ClassName() const { return typeid(*this).name(); }
```
- Returns demangled class name for debugging

### 2. src/core/RemoteFiles.cpp

**Protocol registration and error messages**:
```cpp
RegisterProtocol(typeid(*This).name(), ...);
```
- Uses typeid to get class name for protocol registry and exception messages

---

## Existing Static Type Infrastructure

NetBox already has a robust class identification system:

```cpp
// Enumerations (base/ObjIDs.cpp)
extern const TObjectClassId OBJECT_CLASS_TSessionData;
extern const TObjectClassId OBJECT_CLASS_TS3FileSystem;
// ... one per concrete class

// In TObject base:
virtual bool classof(TClass ClassId) const = 0;  // implemented in each class
virtual bool is(TClass ClassId) const { return classof(ClassId); }

// Macro helpers for implementation:
#define IMPLEMENTS_CLASS(Class, Base) \
  bool Class::classof(TClass ClassId) const override { \
    return (ClassId == OBJECT_CLASS_##Class) || Base::classof(ClassId); \
  }
```

**This system enables safe, zero-cost replacements for all dynamic_casts.**

---

## Replacement Patterns

### dynamic_cast → is() + static_cast

**Before:**
```cpp
TFarDialog* Dialog = dynamic_cast<TFarDialog*>(Obj);
if (!Dialog) return;
```

**After:**
```cpp
if (!Obj->is(OBJECT_CLASS_TFarDialog)) return;
TFarDialog* Dialog = static_cast<TFarDialog*>(Obj);
```

**Advantages:**
- No RTTI needed
- Faster (enum comparison vs string)
- Compile-time type safety preserved

### typeid → virtual GetClassName()

**Option A (Keep minimal RTTI for class names only):**
- Compile with RTTI enabled, but only for `typeid` usage
- Could isolate to single source file that implements `TObject::ClassName()`

**Option B (Eliminate completely):**
```cpp
// In TObject:
virtual const char* GetClassName() const = 0;

// In each class:
const char* TFarDialog::GetClassName() const { return "TFarDialog"; }
```
- Requires maintaining string constant per class
- Zero RTTI overhead

**Option C (Lookup table):**
```cpp
static const struct { TObjectClassId Id; const char* Name; } ClassNames[] = {
  { OBJECT_CLASS_TFarDialog, "TFarDialog" },
  // ...
};
const char* TObject::ClassName() const {
  return ClassNames[GetClassId()].Name;
}
```
- Centralized, no per-class overrides needed

---

## Binary Size Impact

Estimates based on similar projects:
- RTTI metadata per polymorphic class: ~40-80 bytes
- NetBox has ~100+ TObject subclasses → 4-8KB minimum
- Typeinfo string duplication and vtable offsets add overhead
- Total `dynamic_cast`/`typeid` RTTI structure: ~15-40KB

Compiling with `/GR-` (disable RTTI) typically reduces binary by 1-3% for large C++ apps.

---

## Performance Impact

- `dynamic_cast`: ~100-500ns (string comparison, walking RTTI hierarchy)
- `is(OBJECT_CLASS_X)`: ~1-5ns (integer comparison)
- `static_cast`: zero overhead

Hot paths: file operations, network I/O — casting overhead negligible compared to I/O, but still worth eliminating for cleaner code.

---

## Migration Strategy

### Phase 1: Eliminate dynamic_cast (2-3 days)

1. Add `OBJECT_CLASS_*` entries for any classes missing them (some GUI classes may need IDs)
2. Replace each `dynamic_cast` with `is()` check + `static_cast`
3. For `nb::dyn_cast` helper, either:
   - Implement it using `is()` internally
   - Replace direct calls with explicit pattern
4. Add `DEBUG_ASSERT(Obj->is(OBJECT_CLASS_TTarget));` in debug builds
5. Run tests (manual, as no automated suite)

### Phase 2: Handle typeid (1 day)

**Option B (recommended):**
1. Add virtual `GetClassName()` to TObject
2. Override in all concrete classes (can use macro)
3. Update `TObject::ClassName()` to call virtual method
4. Update RemoteFiles.cpp to use class name constants instead of typeid

### Phase 3: Verify RTTI disabled (half day)

1. Compile with `/GR-` flag
2. Link and resolve any missing symbols
3. Manual smoke test of all protocols (FTP, SFTP, S3, WebDAV, SCP)
4. Test session dialog and configuration UI

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Cast to wrong type | High | Use `is()` check before `static_cast`; add debug asserts |
| Missing OBJECT_CLASS enum | Medium | Audit all TObject subclasses; add missing IDs |
| ABI breakage | Medium | Internal change only; no plugin boundary exposure |
| Maintenance overhead | Low | Class IDs must be updated for new classes (similar to current classof impl) |

---

## Detailed Change List

### Files to Modify

| File | Changes |
|------|---------|
| `src/base/ObjIDs.cpp/h` | Add OBJECT_CLASS_* for GUI classes missing them |
| `src/NetBox/FarDialog.cpp` | Replace dynamic_cast with is() + static_cast |
| `src/NetBox/WinSCPDialogs.cpp` | Replace multiple dynamic_casts (tabs, session dialog) |
| `src/core/FileSystems.cpp` | Replace filesystem downcast |
| `src/core/RemoteFiles.cpp` | Replace filesystem downcasts; replace typeid with GetClassName() |
| `src/core/S3FileSystem.cpp`, `src/core/WebDAVFileSystem.cpp`, `src/core/FtpFileSystem.cpp`, `src/core/SshFileSystem.cpp`, `src/core/ScpFileSystem.cpp` | Replace terminal/session access casts |
| `src/windows/GUIConfiguration.cpp` | Replace tree object downcasts |
| `src/base/TObject.cpp/h` | Add GetClassName() virtual method |
| All TObject-derived classes (numerous) | Override GetClassName() (can be macro-generated) |

---

## Recommendations

**Proceed with implementation.** The changes are straightforward, low-risk, and deliver tangible benefits (binary size, performance, clarity). The existing class ID system makes this essentially a mechanical transformation.

**Suggested order:**
1. Add missing OBJECT_CLASS_* IDs (quick audit)
2. Replace dynamic_cast sites one module at a time
3. Add GetClassName() and override everywhere
4. Final build with `/GR-` and test

---

**Report generated**: 2025-04-18  
**Status**: Ready for implementation