# PM-02P.B1 — Pristine Surface Carrier Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-03  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact tested source:** `dff5d6c3532358bbb4bfa11fb17d5bb1113ecb54`  
**Windows FULL:** #1378 — PASS  
**Merged main:** `6b7b967b5ee40ed87c48efba1607b0806b88524d`

## 1. Result

PM-02P.B1 proves complete semantic Face-to-Surface carrier coverage for pristine Extrude geometry without changing the production solid-modeling contract.

The evidence adapter accounts every unique provider Face exactly once as a semantic Surface claim:

- start cap;
- end cap;
- one side carrier per Profile boundary-use provenance.

Provider Surface classification is corroborating evidence only; identity remains semantic provenance.

## 2. Proven carrier classes

The exact-head evidence covers:

- rectangle Extrude: six planar Surface carriers;
- Circle Extrude: two planar caps plus one cylindrical side carrier;
- mixed Line + Arc Profile: planar Line side plus cylindrical Arc side;
- rectangle with circular hole: four outer planar sides plus one inner cylindrical side;
- all cap and side Faces remain fully accounted.

For all accepted B1 scenarios:

```text
unclaimed Faces = 0
multiply-claimed Faces = 0
claims outside Body = 0
false Resolved = 0
```

## 3. Canonical planar frame evidence

Cap frame:

```text
O = source Profile frame origin + signed extent offset * source N
U = source U
V = source V
N = source N
```

Planar side generated from authored straight `Line2`:

```text
O = authored source Line2 start mapped through Profile frame
U = normalized authored source Line2 direction in world space
V = source Profile/support N
N = U x V
```

The side frame is intentionally independent of signed Extrude distance, provider UV, current edge traversal, camera and Viewer state.

B1 passed XY, XZ and YZ source frames and legal reversed loop traversal while preserving authored `Line2` direction/provenance. No semantic frame flip occurred.

## 4. Non-planar boundary

Circle/Arc-generated cylindrical Surface carriers are semantically Resolved and provider-classified as Cylinder, but expose no planar canonical Sketch-support frame.

B1 therefore does not accidentally make a curved Face Sketch-support admissible.

## 5. Verification

Windows PR gate #1378 passed on exact source `dff5d6c3532358bbb4bfa11fb17d5bb1113ecb54`.

The run passed desktop build/test, core-only, kernel-native Release including `pm02p.b1_surface_carriers`, FAST/SUBSYSTEM selector checks, SR-02 latency evidence and CI-04 parity/timing evidence.

## 6. Remaining PM-02P.B scope

B1 does not prove Boolean carrier survival.

B2 must now cover:

- attached Add trimming an inherited Surface;
- Cut trimming an inherited planar Surface;
- strict Face split -> Ambiguous while semantic Surface remains Resolved;
- Surface deletion -> Missing;
- identical-geometry replacement does not steal old identity;
- semantic claim alias/merge -> Ambiguous unless independent winner exists;
- Cut-exposed planar tool Surface -> Resolved with provenance-derived canonical frame.

No production selector/API/schema or user-facing topology behavior is authorized by B1.
