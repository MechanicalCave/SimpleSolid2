# PM-03D — Viewer / Intersection Overlay / Tree / Properties Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-03_DATUM_REFERENCE_GEOMETRY.md`  
**D1 exact runtime candidate:** `b236e54028a64ba00569a995b592b4ab8de2cd17`  
**D1 Windows FULL:** #1511 — PASS  
**D1 merged main:** `28c86f2cb62c5c9156cd2d1ca3f305432957eedf`  
**D2 exact runtime candidate:** `adc9dd5c1f970932a03aa43422a105643db9052d`  
**D2 Windows FULL:** #1514 — PASS  
**D2 merged main:** `3aa23632892853dbf9bf51574e5b2c762c2ac426`  
**Date:** 2026-10-05

## Delivered

PM-03D closes the accepted Datum Plane presentation, semantic selection, Tree and Properties slice.

D1 delivered:

- finite Datum Plane patch and border presentation from current derived Datum frames;
- presentation-only plane/current-Body intersection overlay when an intersection exists;
- owner-bound overlay semantics so intersection graphics select only the owning Datum Plane;
- semantic Viewer picking mapped from disposable presentation tokens back to durable `DatumId`;
- runtime-token refresh/reallocation without changing CAD identity;
- no authored plane-size state and no Edge/Curve/Projection authority in the overlay.

D2 delivered:

- `Reference Geometry` directly below `Origin` in the Part Tree;
- authored Datum Plane children identified by durable `DatumId`;
- Tree <-> Viewer Datum selection through existing semantic selection state;
- Datum Properties for identity, Offset constructor, semantic Source, signed Offset, authored visibility and current evaluation status/diagnostic;
- Edit Datum Plane through the existing shared C1/C2 draft while preserving `DatumId`;
- dependency-safe Delete through the existing semantic command;
- selection of an earlier Datum Plane as Source for Create/Edit;
- individual Show/Hide and bulk Reference Geometry Show/Hide using the existing per-Datum authored visibility command;
- no persisted visibility flag on the Reference Geometry group;
- Datum Tree/Properties status derived from the same current PartEvaluation revision used by the Workbench snapshot.

## Verification

D1 exact-head Windows FULL #1511 PASS on `b236e54028a64ba00569a995b592b4ab8de2cd17`.

Dedicated D1 regression proves:

- Datum presentation has a valid semantic owner and disposable presentation token;
- finite patch/border size is presentation-only;
- current-Body virtual intersection overlay is owned by the Datum token;
- overlay picking resolves only to the owning `DatumId`;
- scene refresh reallocates runtime tokens while semantic Datum selection survives;
- stale/missing Datum evaluation fails closed without publishing a stale Datum frame/presentation.

D2 exact-head Windows FULL #1514 PASS on `adc9dd5c1f970932a03aa43422a105643db9052d`.

The D2 desktop regression proves:

- Part Tree order places `Reference Geometry` directly below `Origin`;
- Datum Properties expose the authored semantic Source, signed Offset, visibility and current derived status;
- per-Datum Show/Hide changes the single authored visibility truth;
- an earlier Datum Plane can source a new Datum Plane through the same shared draft;
- Edit preserves durable `DatumId`;
- Reference Geometry group Hide/Show bulk-updates all child Datum visibility in one transaction per operation;
- no second persisted group visibility state is introduced.

The selected D1/D2 FULL gates also cover the complete desktop build/test graph, semantic/core mode, kernel-native Release mode, FAST/SUBSYSTEM verification and existing regression suites.

## PM-03D gate closure

The accepted PM-03D gates are satisfied:

- display size does not become authored plane size — PASS;
- intersection overlay never becomes Edge/Curve/reference/Projection authority — PASS;
- clicking overlay selects only the owning Datum Plane — PASS;
- group visibility introduces no second persisted visibility truth — PASS;
- stale presentation cannot commit authored CAD state — PASS;
- Viewer token never becomes CAD identity — PASS.

## Architecture boundary

PM-03D does not deliver or claim:

- Datum-backed Sketch support;
- Change Sketch Support to/from Datum Plane;
- stage-aware Datum-backed Profile evaluation;
- downstream Extrude Add/Cut from Datum-backed Sketch;
- Datum Axis / Datum Point;
- Projection.

Those remain owned by PM-03E and later checkpoints.

## Result

PM-03D is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-03E — Datum-backed Sketch + existing Extrude** under the unchanged Owner-accepted PM-03 Work Contract.

## Documentation impact

Internal docs: required — this completion record plus ACTIVE/roadmap/Work Contract checkpoint synchronization.  
User/Product docs: deferred to the accepted PM-03F documentation closure; PM-03D does not claim package completion.  
Product Browser: not required for this work/governance-only checkpoint closure.
