# PM-02P.B2 — Boolean Surface Lineage Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-04  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact tested source:** `24a65c79c40061b7185f49463fe7a7838e1117f4`  
**Windows FULL:** #1379 — PASS  
**Merged main:** `5536e9be1c3da3b3e8f93c4dae229856e8221001`

## 1. Result

PM-02P.B2 proves the accepted distinction between bounded Face realization and semantic Surface carrier through real OCCT Boolean operations.

The evidence remains provider-neutral at the public evidence boundary and does not introduce a production topology API or durable selector.

## 2. Proven outcomes

The exact-head evidence proves:

- attached Add trimming an inherited planar Surface: strict Face Resolved, Surface Resolved;
- Cut trimming an inherited planar Surface: strict Face Resolved, Surface Resolved;
- one source Face split into two current Face realizations: strict Face Ambiguous, semantic Surface Resolved;
- complete Surface deletion: old Surface Missing;
- later geometrically identical replacement Surface: replacement Resolved under new provenance while the deleted old Surface remains Missing;
- coplanar alias/merge without independent semantic winner: Ambiguous rather than provider-history winner;
- independently justified survivor/absorbed-source case: survivor Resolved, absorbed source Missing;
- Cut-exposed planar tool Surface: Resolved with tool provenance and deterministic canonical frame;
- every successful B2 Boolean result used by the evidence has complete Face/Edge/Vertex topology accounting;
- cold reconstruction repeats the same neutral semantic outcomes.

## 3. Identity consequence

The B2 result confirms ADR-0016:

```text
bounded Face identity != semantic Surface carrier identity
```

A topology split is therefore not automatically a support failure for a Sketch hosted on the surviving semantic plane.

Provider `Generated/Modified/Deleted` history remains transient evidence. Geometry equality, centroid, area, plane equation and proximity remain diagnostics only and cannot rebind deleted semantic meaning.

## 4. Frame consequence

Planar carrier frames proven in B1 remain semantic through Boolean trim and split.

Cut-exposed planar Surfaces inherit their canonical frame from the semantic tool Surface provenance, not from current Face edges or provider UV.

## 5. Verification

Windows PR gate #1379 passed on exact source `24a65c79c40061b7185f49463fe7a7838e1117f4`.

The exact tested content was merged as main `5536e9be1c3da3b3e8f93c4dae229856e8221001`.

## 6. Remaining PM-02P work

PM-02P.B Face/Surface lineage evidence is complete for the activated Extrude Add/Cut universe.

Next is PM-02P.C Edge/Curve lineage:

- pristine Extrude material Edge ontology;
- inherited / trimmed / deleted / split Edge outcomes;
- new Boolean intersection Edge provenance;
- multiple intersection branches;
- periodic seam accounting as representation artifact.

No production EdgeReference/CurveReference type is authorized by B2.
