# PM-02P.B3 — Extent Cap-Role and Frame Closure

**Status:** COMPLETED — PASS  
**Date:** 2026-10-03  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact tested source:** `63a577c455b05943a79a5ad7b649088da856b3c8`  
**Windows FULL:** #1380 — PASS  
**Merged main:** `a2db078d01cbb1dccc7112fd22a6083e0266e3e8`

## Result

B3 closes E17 against production-valid `LinearExtrudeInput` semantics.

Nine cases passed: OneSide Forward, OneSide Reverse and Midplane on XY, XZ and YZ source frames.

Exact semantic cap roles:

```text
Forward:
  start = ProfileCap
  end   = ExtentCap

Reverse:
  start = ExtentCap
  end   = ProfileCap

Midplane:
  start = NegativeCap
  end   = PositiveCap
```

For every case the production OCCT kernel published exactly one Resolved Face for each expected cap role and no false Resolved cap role.

Canonical cap carrier frames satisfy:

```text
O(offset) = authored support O + offset * authored support N
U(offset) = authored support U
V(offset) = authored support V
N(offset) = authored support N
```

Negative/reverse extent changes the cap position and semantic role, not the canonical carrier orientation.

Cold replay reproduced all nine neutral outcomes.

## Verification

Windows FULL #1380 passed on exact source `63a577c455b05943a79a5ad7b649088da856b3c8`, including kernel-native Release with `pm02p.b3_extent_cap_frames`, desktop FULL, selector verification and parity/benchmark evidence.

## Consequence

Together B1, B2 and B3 close the PM-02P.B Face/Surface evidence scope E01-E10 plus E17-E19.

No production topology API, persistence schema, Viewer picking, face-supported Sketch or Datum is authorized by this PASS.
