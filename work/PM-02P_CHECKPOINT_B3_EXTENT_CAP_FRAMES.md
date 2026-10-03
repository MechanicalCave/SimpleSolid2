# PM-02P.B3 — Extent Cap Roles and Frame Closure

**Status:** COMPLETED — PASS  
**Date:** 2026-10-04  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact tested source:** `63a577c455b05943a79a5ad7b649088da856b3c8`  
**Windows FULL:** #1380 — PASS  
**Merged main:** `a2db078d01cbb1dccc7112fd22a6083e0266e3e8`

## 1. Result

PM-02P.B3 closes the remaining exact cap-role and cap-frame evidence needed by E17.

The evidence covers the production Extrude extent semantics rather than inventing an evidence-only replacement:

- OneSide Forward -> ProfileCap / ExtentCap;
- OneSide Reverse -> ExtentCap / ProfileCap;
- Midplane -> NegativeCap / PositiveCap.

## 2. Frame rule proven

For XY, XZ and YZ source support frames, both semantic cap frames are the authored source support frame translated by the exact semantic start/end offset.

U/V/N orientation is preserved. Reverse and negative extent do not silently mirror the canonical carrier frame.

Material/outward side sense remains distinct from canonical carrier-frame orientation.

## 3. Provider corroboration

For every tested extent mode:

- exactly one Resolved provider cap is published for each expected semantic cap role;
- no unrelated semantic cap role is falsely published Resolved;
- the carrier Surface is Plane;
- the provider Surface classification is Plane;
- cold reconstruction produces identical semantic outcomes and frames.

The matrix covers 9 cases:

```text
XY  x Forward / Reverse / Midplane
XZ  x Forward / Reverse / Midplane
YZ  x Forward / Reverse / Midplane
```

## 4. Verification

Windows PR gate #1380 passed on exact source `63a577c455b05943a79a5ad7b649088da856b3c8`.

The exact tested content was squash-merged as main `a2db078d01cbb1dccc7112fd22a6083e0266e3e8`.

## 5. Consequence

E17 cap-role/frame closure is evidenced. This does not introduce a production support selector, schema migration or face-supported Sketch behavior.

PM-02P.C may proceed on top of this baseline.
