# RWProperty Implementation Review

> Source: NetBox `src/base/Property.hpp`, `src/base/CppProperties.h`, `src/core/Configuration.h`
> Date: 2026-05-07
> Scope: Full audit of property system architecture, usage patterns, bugs, and improvement opportunities

---

## Architecture

NetBox has **two parallel property systems** coexisting:

### System 1 — Legacy (CppProperties.h)

Template-based with member function pointers as template parameters. Borland C++/CLI emulation.

```cpp
// Declaration:
RWProperty<T, Object, &Object::getter, &Object::setter> prop;

// Requires explicit binding:
prop(&myObject);

// Access:
T val = prop();          // function call
prop.set(value);         // set()
prop = value;            // operator=
T val = prop;            // implicit conversion
```

**Classes:** `Property<T>`, `ROProperty<T,Obj,getter>`, `WOProperty<T,Obj,setter>`, `RWProperty<T,Obj,getter,setter>`, `IndexedProperty<Key,T,Obj,getter,setter>`, `IndexedProperty2<Key,T,Obj,getter&,setter>`, `IndexedPropertyVoid<Key,Obj,getter,setter>`

**Size:** ~24 bytes (object pointer + potential padding)

**Status:** Included via `src/base/Classes.hpp:19`. Likely only needed for Borland compatibility; unused by modern code paths.

### System 2 — Modern (Property.hpp)

FastDelegate-based with `nb::bind()` runtime binding. Primary active system.

```cpp
// Declaration (in-class member initializer):
RWProperty<T> Name{nb::bind(&Class::Getter, this), nb::bind(&Class::Setter, this)};

// Access:
T val = Name();          // function call
Name(value);             // function call set
Name = value;            // operator=
T val = Name;            // implicit conversion
```

**Five variant classes** (all in `src/base/Property.hpp`):

| Class | Purpose | Storage | Size |
|-------|---------|---------|------|
| `ROProperty<T>` | Read-only, getter via FastDelegate | `FastDelegate0<T>` | 16B |
| `ROIndexedProperty<T>` | Read-only indexed, getter via FastDelegate | `FastDelegate1<T,int32_t>` | 16B |
| `ROProperty2<T>` | Read-only, direct pointer to member | `const T*` | 8B |
| `RWProperty<T>` | Read-write via getter+setter delegates | inherits ROProperty + `FastDelegate1<void,ValueType>` | ~32B |
| `RWProperty2<T>` | Read-write via direct member pointer, no callback | `T*` | 8B |
| `RWPropertySimple<T>` | Read-write via direct pointer + setter callback | `T*` + `FastDelegate1<void,ValueType>` | ~24B |

**ValueType optimization:** For `sizeof(T) <= 2*sizeof(void*) && is_trivially_copyable_v<T>`, returns `T` by value; otherwise returns `const T&`.

---

## Usage Patterns (by class)

### TConfiguration (`src/core/Configuration.h:393-504`)
- **~60 properties total**
- Only **5 modern RWProperty**: `CollectUsage`, `PuttyRegistryStorageKey`, `Logging`, `LogProtocol`, `OptionsStorage`
- Only **5 modern ROProperty**: `StoredSessionsSubKey`, `PuttySessionsKey`, `LogToFile`, `ActualLogProtocol`, `Persistent`
- **1 RWPropertySimple**: `QueueTransfersLimit`
- **Remaining ~50**: Borland `__property` only, no modern counterpart
- **2 dead declarations** (commented out, lines 476-477): `PuttySshHostCAList`, `ActiveSshHostCAList`

### TSessionData (`src/core/SessionData.h:639-910`)
- Heaviest consumer: **~80 properties**
- ~30 `RWProperty` (getter+setter via bind)
- ~40 `RWPropertySimple` (pointer + setter callback)
- ~8 `ROProperty` (getter only)
- ~5 `RWProperty2` (pointer only, no callback)

### TCopyParamType (`src/core/CopyParam.h:133-193`)
- **Three different access patterns in one class:**
  1. Borland `__property` (lines 133-192)
  2. Modern `RWProperty` (PreserveTime, ReplaceInvalidChars, LocalInvalidChars, TransferSkipList)
  3. **Raw reference aliases** (10+ entries): `bool& PreserveReadOnly{FPreserveReadOnly}` — bypasses SET_CONFIG_PROPERTY Changed() notification

### TFarDialog Controls (`src/NetBox/FarDialog.h`)
- `TFarDialogItem::Enabled` — `RWProperty<bool>` binding to GetEnabled/SetEnabled
- `TFarCheckBox::Checked` — `RWProperty<bool>`
- `TFarEdit::Text` — `RWProperty<UnicodeString>`
- `TFarComboBox::ItemIndex` — `RWProperty<int32_t>`
- `TFarComboBox::Text` — `RWProperty<UnicodeString>`

### TRemoteFile (`src/core/RemoteFiles.h:178-216`)
- `RWProperty<UnicodeString>` FileName
- `RWProperty2<int64_t>` CalculatedSize, Size

### TBookmarkList (`src/core/Bookmarks.h:24-85`)
- `ROProperty<int32_t>` Count + `bool& Modified{FModified}` raw reference alias

---

## Known Bugs

### Bug 1: ROProperty2::operator->() — compile error
**File:** `src/base/Property.hpp:122-125`
```cpp
constexpr T operator ->() const {
    DebugCheck(_value);
    return _value();  // BUG: _value is const T*, not callable
}
```
Same bug in `RWProperty2::operator->()` at line 239. The getter `operator->()` on line 125 shadows the setter `operator->()` on line 128, but both have the same bug. This has likely never been instantiated — it would fail at compile time on first use.

**Fix:** Change `_value()` to `*_value`.

### Bug 2: VCL IsValidPassword threshold accepts weak passwords
**File:** `src/windows/UserInterface.cpp:1347-1351`
```cpp
CanSubmit = (!UseMP || IsValidPassword(CurrentEdit->Text) >= 0) && ...
```
`IsValidPassword` returns: `-1` = empty/too long, `0` = weak (<6 chars or <2 classes), `1` = valid. Using `>= 0` enables OK for 5-character passwords. NetBox's Far-UI correctly uses `Valid <= 0` for rejection.

---

## Design Issues

### Issue 1: operator= semantics are surprising
```cpp
// RWProperty (line 174): assigning from another RWProperty copies the VALUE,
// not the property object:
RWProperty & operator =(const RWProperty & Value) {
    _setter(Value());  // reads rhs value, sets on self
    return *this;
}
```
Standard C++: `operator=(const T&)` should assign the object. This copies values between properties, which is non-obvious. Same pattern in RWProperty2 (line 213) and RWPropertySimple (line 295).

### Issue 2: No move semantics
All property classes delete move constructor/assignment:
```cpp
RWProperty(const RWProperty &) = delete;
RWProperty(RWProperty &&) noexcept = delete;
```
This makes any object containing RWProperty members immovable, preventing use in `std::vector`, `std::map` value types, etc. TSessionData (80+ properties) cannot be stored in a `std::vector`.

### Issue 3: Missing comparison operators
Only `==` and `!=` are defined. `<`, `>`, `<=`, `>=` are absent. For numeric properties (`QueueTransfersLimit`, `LogMaxSize`, etc.), range checks like `if (prop > 10)` require calling `prop()` explicitly.

### Issue 4: Duplicate declaration burden
Every property requires two declarations side by side (Borland + modern). ~60 properties × 2 lines = 120 lines of boilerplate in TConfiguration alone.

### Issue 5: Inconsistent migration
TConfiguration has ~60 properties but only 10 with modern wrappers. TSessionData has ~80 with full migration. TCopyParamType has only 4 migrated plus raw references. No consistent policy.

### Issue 6: RWProperty inherits ROProperty without benefit
`RWProperty<T>` inherits `ROProperty<T>` to reuse the getter storage and operators. But `ROProperty` has no virtual destructor and `RWProperty` doesn't override any virtual methods. The inheritance adds complexity and size overhead without polymorphism benefits.

### Issue 7: Raw reference aliases bypass safety
```cpp
// CopyParam.h — 10+ instances:
bool& PreserveReadOnly{FPreserveReadOnly};
```
These bypass `SET_CONFIG_PROPERTY`'s `Changed()` notification, meaning config changes via these references are silent.

---

## Improvement Summary

| # | Priority | Change | Effort | Lines |
|---|----------|--------|--------|-------|
| 1 | P0 | Fix `operator->()` bug in ROProperty2/RWProperty2 | 1 min | 2 |
| 2 | P0 | Delete/rename misleading `operator=(const RWProperty&)` | 5 min | ~5 |
| 3 | P1 | Add move semantics (or document why impossible) | 30 min | ~30 |
| 4 | P1 | Macro-generate dual declarations | 30 min | ~15 + migration |
| 5 | P2 | Add `<`, `>`, `<=`, `>=` operators | 20 min | ~20 × 5 |
| 6 | P2 | Complete TConfiguration migration | 1 hr | ~55 |
| 7 | P3 | Remove dead code (commented ROProperty, unused CppProperties) | 5 min | ~10 |
| 8 | P3 | Replace raw references with RWProperty2 in TCopyParamType | 30 min | ~120 |

---

## Key Files

| File | Lines | Role |
|------|-------|------|
| `src/base/Property.hpp` | 1-356 | Modern property classes (5 variants) |
| `src/base/CppProperties.h` | 1-319 | Legacy Borland property emulation (7 variants) |
| `src/base/Classes.hpp` | 19,27-39 | Include site for both, `nb::bind` re-export |
| `src/include/FastDelegateBind.h` | — | FastDelegate binding helpers |
| `src/core/Configuration.h` | 14-25,305-504 | SET_CONFIG_PROPERTY macros, ~60 property declarations |
| `src/core/SessionData.h` | 639-910 | Heaviest consumer: ~80 modern properties |
| `src/core/CopyParam.h` | 133-193 | Mixed pattern: Borland + modern + raw refs |
| `src/NetBox/FarDialog.h` | 218,432,479,662-663 | Dialog control property bindings |