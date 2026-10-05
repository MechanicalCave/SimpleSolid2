# PM-03C — Commands + Extrude-Style Draft / Command Line Parity Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-03_DATUM_REFERENCE_GEOMETRY.md`  
**C1 exact runtime candidate:** `c633a9714602801e1253d119198ea9e46f73f9a9`  
**C1 Windows FULL:** #1496 — PASS  
**C1 merged main:** `4147eba3697dfc345f49762508e92293b6373ff5`  
**C2 exact runtime candidate:** `8b9185261e2cf32006f6af7a8c8a9da5e7c9ec04`  
**C2 Windows FULL:** #1507 — PASS  
**C2 merged main:** `ad9e64df059091556bca2dc18515fa25a5cc5a32`  
**Date:** 2026-10-05

## Delivered

PM-03C closes the semantic command/draft and user-input parity layer for the accepted Offset Datum Plane package.

C1 delivered:

- semantic Create / Edit / Delete / Show / Hide Datum Plane commands through `DocumentSession`;
- candidate reconstruction plus current Part/Datum evaluation before Create/Edit commit;
- session-local `DatumId` high-water preservation across normal commits and Undo/Redo branching;
- one shared `DatumPlaneDraft` for GUI/Command Line adapters;
- default new-draft Offset = 10 mm;
- signed Offset as the only authored direction truth, with Reverse implemented as sign negation;
- generation-bound preview/evaluation so stale draft evidence cannot Finish;
- one successful mutating Finish = one transaction / one Undo step;
- Cancel, rejected Finish and semantic no-op = zero authored history;
- conservative atomic Delete rejection when another Datum Plane depends on the producer.

C2 delivered:

- one top-level `Datum Plane` tool in CadWorkbench;
- Operations panel with extensible Constructor selector, currently containing only `Offset`;
- semantic Source display, signed Offset, Reverse, Finish Datum Plane and Cancel;
- selection-first Origin XY/XZ/YZ and planar Body Face source acquisition;
- command-first `DATUM PLANE` / `DATUMPLANE`;
- global Command Line and Dynamic Input operating on the same shared Datum draft/evaluation path;
- signed Offset text input, `REVERSE`, `FINISH` / empty Enter and `CANCEL` / Esc behavior;
- one active semantic modeling-tool context at a time;
- active-document switching/deactivation clears only transient Datum runtime context.

## Verification

C1 exact-head Windows FULL #1496 PASS on `c633a9714602801e1253d119198ea9e46f73f9a9`.

Dedicated C1 regression proves:

- preview is non-authoring and does not consume persistent history;
- default 10 mm and signed Reverse semantics;
- stale evaluation / revision drift cannot commit;
- Edit preserves DatumId;
- visibility no-op creates no history;
- referenced producer Delete rejects atomically;
- unreferenced Delete is Undo/Redo correct;
- Cancel/rejected Finish creates no authored mutation;
- session-local DatumId high-water prevents ID reuse after Undo branching.

C2 exact-head Windows FULL #1507 PASS on `8b9185261e2cf32006f6af7a8c8a9da5e7c9ec04`.

The selected C2 FULL passed:

- complete desktop build/test graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test;
- FAST and SUBSYSTEM selector verification;
- complete desktop CTest;
- SR-02 latency evidence;
- CI-04 warm FULL parity/comparative evidence.

Dedicated C2 offscreen regression proves:

- selection-first GUI creation from XY Plane;
- Constructor = Offset and default 10 mm;
- Reverse modifies only the same signed Offset;
- command-first Datum Plane activation;
- XZ/YZ Origin source acquisition through existing semantic selection;
- Command Line signed Offset / Reverse / Cancel;
- Dynamic Input exposes the same Offset field;
- empty Enter Finish through global `CadInputSession`;
- Finish authors exactly one Datum transaction;
- Cancel leaves authored state/history unchanged.

## Architecture boundary

PM-03C does not deliver or claim:

- Datum Plane patch/border Viewer presentation;
- plane/current-Body virtual intersection overlay;
- Datum presentation tokens or Viewer picking;
- `Reference Geometry` Tree group;
- Datum Tree selection as a source;
- Datum Properties page;
- authored bulk group visibility UI;
- Datum-backed Sketch support;
- Datum Axis / Datum Point;
- Projection.

Those remain owned by PM-03D and later checkpoints.

## Result

PM-03C is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-03D — Viewer / intersection overlay / Tree / Properties** under the unchanged Owner-accepted PM-03 Work Contract.

## Documentation impact

Internal docs: required — this completion record plus ACTIVE/roadmap/Work Contract checkpoint synchronization.  
User/Product docs: deferred to the accepted PM-03F documentation closure; PM-03C does not claim package completion.  
Product Browser: not required for this work/governance-only checkpoint closure.
