# PM-03E — Datum-Backed Sketch + Existing Extrude Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-03_DATUM_REFERENCE_GEOMETRY.md`  
**Exact runtime candidate:** `f4c691c50ce28bc311076b94925b719c889e8172`  
**Windows FULL:** #1520 — PASS  
**Merged main:** `66f31391894eb6a0adfb430d4dd78b37e749f150`  
**Date:** 2026-10-05

## Delivered

PM-03E closes the accepted Datum-backed Sketch / Profile / existing Extrude vertical slice.

Delivered semantics:

- native Part schema v11 extends durable Sketch support with semantic `DatumId` only;
- Datum-backed Sketch support stores no world frame, placement, Viewer token or provider identity;
- Create Sketch on Datum Plane and Change Sketch Support to/from Datum Plane use the existing Sketch support command path;
- re-support preserves `SketchId`, Sketch `EntityId` values and authored local U/V geometry;
- current Sketch placement is derived from the same-revision Datum evaluation;
- Datum-backed Profile materialization resolves the current Datum frame fail-closed;
- existing Extrude Add/Cut consume Datum-backed Profile input through the existing feature evaluator and Kernel input path;
- feature evaluation resolves Datum-backed support from the currently available Body prefix only;
- a Datum whose transitive required Body stage would depend on the consuming Sketch/Profile/Feature is rejected/blocked rather than forming a cycle;
- changing an upstream Datum source/offset recomputes only derived world placement and downstream modeling;
- Missing/Ambiguous/Unsupported/Blocked Datum support provides no stale last-good Sketch/Profile frame;
- Delete Datum rejects atomically while a Sketch depends on that Datum;
- Workbench support picking accepts existing Datum Plane through the same Create/Re-support interaction grammar as Origin/Body Face support.

## Verification

Exact-head Windows FULL #1520 PASS on `f4c691c50ce28bc311076b94925b719c889e8172`.

The selected FULL passed:

- complete desktop build/test graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test;
- FAST and SUBSYSTEM selector verification;
- complete desktop CTest;
- SR-02 latency evidence;
- CI-04 warm FULL parity/comparative evidence.

Dedicated PM-03E regression proves:

- existing Extrude Add and Cut both consume Datum-backed Profile input;
- Origin -> Datum -> Origin -> Datum re-support preserves SketchId, EntityIds and local U/V geometry;
- changing Datum offset moves the derived Sketch/Profile world frame without changing the authored local Sketch model;
- transitive Body-stage cycle rejection prevents a Sketch from depending on a Datum whose Body source depends on the Feature consuming that Sketch;
- Datum Delete rejects while a Sketch depends on it;
- loss of the Datum's upstream Body stage blocks Datum/Sketch/Profile/downstream Features and prevents stale Kernel calls;
- schema v11 persists only semantic DatumId support and cold load reconstructs the valid feature chain;
- Workbench Sketch support acquisition from Datum Plane follows the existing draft/Finish path.

## PM-03E gate closure

The accepted PM-03E gates are satisfied:

- local Sketch U/V geometry preserved — PASS;
- upstream Datum source/offset edit moves Datum-backed Sketch correctly — PASS;
- invalid source produces no stale downstream modeling — PASS;
- Body-stage cycle rejected — PASS.

## Architecture boundary

PM-03E does not add:

- a global dependency graph;
- durable provider/runtime topology identity;
- durable Viewer presentation identity;
- authored Sketch world placement;
- Datum Axis / Datum Point;
- Projection.

Lifecycle integration, final repair matrix, canonical documentation, Product Browser regeneration and Owner Windows acceptance remain PM-03F responsibilities.

## Result

PM-03E is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-03F — lifecycle / persistence / docs / Owner acceptance** under the unchanged Owner-accepted PM-03 Work Contract.

## Documentation impact

Internal docs: required — this completion record plus ACTIVE/roadmap/Work Contract checkpoint synchronization.  
User/Product docs: deferred to PM-03F final package documentation.  
Product Browser: deferred to PM-03F.
