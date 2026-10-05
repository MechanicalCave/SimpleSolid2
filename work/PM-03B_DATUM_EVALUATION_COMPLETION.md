# PM-03B — Datum Evaluator + Dependency / Cycle Semantics Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-03_DATUM_REFERENCE_GEOMETRY.md`  
**Exact runtime candidate:** `e12f6c606132eadab5283372dee1a9f3f3008abd`  
**Windows FULL:** #1491 — PASS  
**Merged main:** `42b36d427bdb4c5c75b4961444576c7905032eb2`  
**Date:** 2026-10-05

## Delivered

PM-03B establishes Part-owned derived evaluation for the accepted Offset Datum Plane package without adding commands, product UI or Datum-backed Sketch support.

Delivered production semantics:

- `DatumEvaluation` / `DatumPlaneEvaluation` are derived same-revision Part truth and are never persisted;
- Origin XY/XZ/YZ frames preserve the accepted right-handed O/U/V/N mappings;
- signed Datum offset derives the world frame from the semantic source frame;
- Body-Surface Datum sources resolve only through their durable PM-02 `SurfaceReference` at the exact declared `BodyStageRef`;
- Datum Plane sources chain through durable `DatumId`;
- a Datum chain carries its transitive required Body-stage dependency floor;
- Resolved / Missing / Ambiguous / Unsupported / Blocked outcomes are structured and expose no stale last-good frame;
- stale Part evaluation or unavailable source stage fails closed;
- runtime/provider tokens are disposable and do not affect equivalent semantic reevaluation.

## Verification

Exact-head Windows FULL #1491 PASS on `e12f6c606132eadab5283372dee1a9f3f3008abd` is the runtime verification authority for PM-03B.

The FULL run passed:

- complete desktop build/test graph;
- core-only build/test without Qt/OCCT;
- kernel-native Release build/test;
- FAST and SUBSYSTEM selector verification;
- complete desktop CTest;
- SR-02 latency evidence;
- CI-04 warm FULL parity/comparative evidence.

Dedicated PM-03B regression proves:

- deterministic XY/XZ/YZ Datum frames;
- positive, negative and zero signed offsets without identity aliasing;
- exact-stage planar Body Surface resolution;
- Datum-to-Datum frame chaining;
- transitive Body-stage dependency propagation;
- source Surface movement recomputes only derived Datum frames;
- Missing / Ambiguous / Unsupported / non-planar source failure with no frame;
- unavailable source stage blocks rather than rebinding;
- stale Body evaluation cannot resolve Body-dependent Datum meaning;
- fresh runtime/provider tokens reconstruct the same semantic Datum result;
- repeated same-state evaluation is deterministic.

## Architecture boundary

PM-03B does not add:

- Create/Edit/Delete/Show/Hide commands;
- runtime draft or preview;
- GUI / Command Line tool behavior;
- Viewer plane presentation or intersection overlay;
- Tree / Properties;
- Datum-backed Sketch support;
- Datum Axis / Datum Point;
- Projection.

Those remain owned by PM-03C and later checkpoints.

## Result

PM-03B is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-03C — commands + Extrude-style draft / Command Line parity** under the unchanged accepted PM-03 Work Contract.

## Documentation impact

Internal docs: required — this completion record plus ACTIVE/roadmap/Work Contract checkpoint synchronization.  
User/Product docs: not required — PM-03B introduces no user-facing Datum tool behavior.  
Product Browser: not required — no canonical product documentation changes in this checkpoint closure.
