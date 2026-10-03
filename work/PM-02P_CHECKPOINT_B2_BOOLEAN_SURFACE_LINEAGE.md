# PM-02P.B2 — Boolean Surface Lineage Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-03  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact tested source:** `24a65c79c40061b7185f49463fe7a7838e1117f4`  
**Windows FULL:** #1379 — PASS  
**Merged main:** `5536e9be1c3da3b3e8f93c4dae229856e8221001`

## 1. Result

PM-02P.B2 proves the accepted distinction between bounded Face topology and semantic Surface carrier continuity across the current Extrude Add/Cut Boolean universe.

The evidence remains provider-neutral outside the OCCT evidence adapter and introduces no production selector, persistence schema or user-facing topology behavior.

## 2. Proven outcomes

### Attached Add trim

A semantic planar source Surface survives an attached Add that changes its bounded Face boundary:

```text
strict Face -> Resolved
Surface     -> Resolved
realizations -> 1
canonical frame -> unchanged
```

### Cut trim

A pocket-style Cut trims the inherited planar source Face while preserving the semantic Surface and frame:

```text
strict Face -> Resolved
Surface     -> Resolved
realizations -> 1
canonical frame -> unchanged
```

### Face split

A Cut splits one previous bounded Face into two disconnected current Face realizations while preserving one semantic plane:

```text
strict Face -> Ambiguous
Surface     -> Resolved
realizations -> 2
canonical frame -> unchanged
```

No first/largest/nearest descendant is selected.

### Surface deletion and identical recreation

A Cut fully deletes the original semantic Surface:

```text
old Surface -> Missing
```

A later Add recreates provider geometry with the same plane geometry and the same canonical frame, but from new provenance:

```text
old Surface        -> Missing
replacement Surface -> Resolved
```

Geometry equality does not recreate lineage and cannot steal semantic identity.

### Alias / merge

Existing PM-00A Boolean-history evidence is reused for coplanar collapse:

- two semantic claims sharing one provider descendant remain Ambiguous without an independent semantic winner;
- provider history asymmetry alone does not choose a winner;
- when independent semantic evidence declares one surviving meaning and another deleted meaning, the survivor is Resolved and the deleted meaning is Missing.

### Cut-exposed Surface

A planar Surface authored by the Cut tool becomes a current Body boundary and remains:

- semantically provenance-bound;
- provider-classified as Plane;
- singularly Resolved;
- equipped with its provenance-derived canonical frame.

This proves that PM-02 cannot be limited to inherited upstream Faces.

## 3. Complete topology accounting

Every successful B2 Boolean result used by the new evidence path carries complete transient Face/Edge/Vertex inventory.

Delete/recreate and alias/merge scenarios were explicitly extended so that lineage evidence cannot pass while silently omitting current provider topology.

## 4. Provider policy

B2 uses the production zero-fuzzy Boolean policy.

OCCT Modified/Generated/Deleted history is transient evidence only. It never becomes durable identity.

No automatic rebinding uses:

- plane equality;
- centroid;
- area;
- nearest geometry;
- provider order;
- topology ordinal.

## 5. Cold reconstruction

All B2 scenarios are evaluated again from declared semantic inputs after the first provider objects are discarded.

Neutral semantic statuses and canonical frames repeat exactly.

## 6. Verification

Windows PR gate #1379 passed on exact source `24a65c79c40061b7185f49463fe7a7838e1117f4`.

The run passed:

- exact checkout and documentation dispatcher;
- complete desktop build graph;
- core-only verification;
- kernel-native Release including `pm02p.b2_boolean_surface_lineage`;
- FAST/SUBSYSTEM selector checks;
- desktop FULL tests;
- SR-02 latency evidence;
- CI-04 parity and comparative timing evidence.

## 7. Remaining PM-02P.B closure

B1 + B2 close E01-E10 Face/Surface behavior and E18/E19.

Before PM-02P.B as a whole is declared PASS, E17 still requires explicit extent/cap-frame evidence for:

- OneSide Forward: ProfileCap / ExtentCap;
- OneSide Reverse: ExtentCap / ProfileCap;
- Midplane: NegativeCap / PositiveCap.

That bounded closure is PM-02P.B3.
