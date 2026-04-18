# Plan - Investigate Static RTTI Improvements

**Feature**: Investigate replacing dynamic_cast/typeid with static alternatives for better performance and smaller binaries  
**Created**: 2025-04-18  
**Mode**: Fast  

---

## Research Context

### Current Architecture
- **Object System**: NetBox uses `TObject` base class with virtual functions and class ID system (`OBJECT_CLASS_*` enums)
- **Existing Infrastructure**: `classof()` and `is()` methods already provide compile-time type identification
- **RTTI Usage**: `dynamic_cast` and `typeid` are used in several places, potentially adding bloat and runtime overhead

### Potential Benefits
- Reduce binary size (RTTI metadata can be 10-40KB)
- Improve runtime performance (dynamic_cast is slower than static_cast)
- Maintain type safety through compile-time checks using existing class ID system

---

## Settings
- **Testing**: N/A (investigation only)
- **Logging**: Standard
- **Docs**: Yes — create investigation report
- **Roadmap Linkage**: none (skipped)

---

## Tasks

### Task 1: Catalog all RTTI usage

**Status**: ✅ Completed

**Action**: Search codebase for `dynamic_cast` and `typeid` occurrences.

**Findings**:
- `dynamic_cast`: 18 occurrences (downcasts from TObject* to concrete types)
- `typeid`: 4 occurrences (for ClassName() and error messages)

**Report**: See `.ai-factory/plans/rtti-survey.md` for detailed locations.

---

### Task 2: Analyze class hierarchy

**Status**: ✅ Completed

**Observations**:
- All `dynamic_cast` sites are downcasts from `TObject*` to known derived types
- No cross-casts (sibling-to-sibling) detected
- Existing `OBJECT_CLASS_*` enums and `is()` method enable safe replacement
- Pattern: `if (Obj->is(OBJECT_CLASS_Target)) { static_cast<TTarget*>(Obj); }`

**Deliverable**: Analysis table showing each hierarchy and replacement strategy.

---

### Task 3: Identify replacement opportunities

**Status**: ✅ Completed

**Recommendation**:
- Replace all `dynamic_cast` with `is()` + `static_cast` (high ROI, low risk)
- Replace `typeid` with virtual `GetClassName()` method
- Build with `/GR-` to verify no RTTI leakage

**Estimated Effort**: 1-2 developer days  
**Expected Benefit**: 15-40KB binary size reduction, faster casts

---

## Task 4: Create final report

**Status**: ✅ Completed

**Deliverable**: Full investigation report at `.ai-factory/plans/improve-static-rtti-report.md`

**Contents**:
- RTTI usage inventory with file:line references
- Feasibility analysis
- Binary size/performance impact estimates
- Migration strategy and detailed change list
- Risk assessment

---

## Plan

All tasks completed in a single investigation session. No implementation required — the report provides a clear path forward should the team decide to proceed.

---

## Commit Plan

(No commits needed — investigation/documentation only. If report is committed later, use:)

```
docs: add static RTTI investigation report

- Document current RTTI usage patterns
- Provide recommendations for eliminating RTTI
- Include change list and migration strategy
```