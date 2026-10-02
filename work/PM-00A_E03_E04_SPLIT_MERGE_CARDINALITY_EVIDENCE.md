# PM-00A — E03/E04 Split/Merge Cardinality Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-02  
**Exact source candidate:** `30886e0d47a8f0bbfba5ab9048cf22fb9fa7be25`  
**Kernel-focused:** #1274 — PASS  
**Windows FULL:** #1275 — PASS  
**Regression:** `pm00a.e03_e04_cardinality`  
**Matrix:** `work/PM-00A_E01_E10_EVIDENCE_MATRIX.md` v1.0

## Result

E03-01 through E03-04 and E04-01 through E04-04 PASS.

False-Resolved count: **0**.

The evidence demonstrates that provider Boolean history is useful evidence about cardinality and lineage, but it does not by itself define semantic identity. A singular reference fails closed when one semantic target has multiple plausible descendants, and merged provider history cannot choose a semantic winner without independent semantic-role/provenance evidence.

## E03 — split/delete evidence

### E03-01 — singular target splits

A target edge of a box is cut by a middle notch.

Observed OCCT history:

```text
Modified = 2
Generated = 0
Deleted = false
unchanged = false
unique descendants = 2
```

The singular semantic target resolves to **Ambiguous**.

No first/longest/nearest fragment is selected.

### E03-02 — technical/auxiliary candidate filtering

The cardinality contract separately verifies:

```text
semantic descendants = 1
technical/auxiliary candidates = 1
→ Resolved
```

The technical candidate is excluded only by independent semantic-role evidence. Provider order, length, position and proximity are not selection rules.

This row proves the resolver rule; it does not introduce a durable technical-candidate flag or persistent selector format.

### E03-03 — target removed

A cut removes the complete target edge.

Observed OCCT history:

```text
Modified = 0
Generated = 0
Deleted = true
unchanged = false
unique descendants = 0
```

The singular semantic target resolves to **Missing**.

### E03-04 — undeclared aggregate descendants

A request to reinterpret the singular reference as “all split descendants” returns **Unsupported**.

PM-00A does not introduce an aggregate/set persistent-reference type.

## E04 — merge evidence

### E04-01 — two prior meanings collapse to one provider descendant

Two coplanar source-face meanings are fused into one result region.

Observed OCCT history:

```text
first:  Modified = 1, descendants = 1
second: Modified = 1, descendants = 1
shared descendants = 1
```

Both source faces map to the same physical provider descendant.

With no independent semantic winner:

```text
first  → Ambiguous
second → Ambiguous
```

The fact that OCCT reports both inputs as Modified does not preserve the prior semantic distinction.

### E04-02 — asymmetric provider history cannot choose identity

A bounded Fuse probe produces one rectangular `50×20×10` result while:

- the first selected source face is modified by an extension of the result;
- the second selected source face is internal to the result and is deleted.

The final regression requires:

```text
first:
  Modified > 0
  Deleted = false
  unchanged = false
  unique descendants = 1

second:
  Deleted = true
  unique descendants = 0

result faces = 6
```

Provider bookkeeping therefore contains a “better-looking” history for the first source, but without independent semantic-role evidence it still may not choose a winner.

Expected and actual semantic outcome:

```text
first  → Ambiguous
second → Ambiguous
```

### E04-03 — semantic role preserves one meaning while another is genuinely removed

A fully internal lower box is absorbed by the outer solid.

Observed provider evidence in the bounded probe:

```text
outer selected face:
  unchanged = true
  unique descendants = 1

inner selected face:
  Deleted = true
  unique descendants = 0
```

When independent semantic-role evidence identifies the surviving outer meaning:

```text
outer role → Resolved
inner role → Missing
```

This demonstrates that provider history may support a decision, but does not create the role.

### E04-04 — undeclared merged aggregate

A request for an undeclared aggregate merged-reference meaning returns **Unsupported**.

## Cold replay

All four provider scenarios are reconstructed a second time after the first operation objects and provider handles leave scope.

The complete neutral history/cardinality evidence must compare equal across the two passes.

PASS confirms that the evidence result does not require previous provider objects or session handles.

## Architecture implications

E03/E04 strengthen O-05 with four concrete rules:

1. **Cardinality is semantic.** A singular selector with two valid semantic descendants is Ambiguous.
2. **Deletion is not retargeting.** No semantic descendant means Missing; another nearby shape is irrelevant.
3. **Provider history is evidence, not authority.** Modified/Generated/Deleted/unchanged observations cannot choose a semantic winner by themselves.
4. **Merge collapse destroys distinction unless semantics preserve it.** If two prior meanings map to one physical provider result, both fail closed unless independent role/provenance evidence still distinguishes one accepted meaning.

The provider-specific observation is also important: OCCT same-domain merging can report both contributing source faces as Modified to the same descendant. Therefore “Modified” is not equivalent to “identity preserved”.

## Scope boundary

E03/E04 introduced no:

- persistent topology selector schema;
- aggregate/set reference type;
- BodyId / FeatureId persistence;
- Part Feature Tree;
- product Boolean/Cut/Fillet command;
- persisted B-Rep or provider history;
- geometry-similarity fallback;
- topology ordinal or Viewer identity.

The next PM-00A package is **E02 — Extrude → Cut → Fillet-style multi-stage lineage**, now using the split/merge semantics established by E03/E04.
