# PM-03C — Datum Commands + Draft / Command Line Parity Completion

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

PM-03C closes the semantic command/draft and product binding layer for the accepted Offset Datum Plane workflow without entering the PM-03D Viewer/Tree/Properties surface.

C1 delivered:

- one shared `DatumPlaneDraft` for create/edit with generation-bound stale-evaluation protection;
- accepted new-draft default Offset = 10 mm;
- signed Offset with Reverse implemented only as sign negation;
- derived draft evaluation with current frame/status and zero authored mutation before Finish;
- Create/Edit Datum Plane through `DocumentSession` with same-revision revalidation before commit;
- persistent Show/Hide command;
- conservative dependency-safe Delete command with atomic rejection;
- session-local DatumId high-water behavior across commit, Undo/Redo and history branching;
- bounded active-draft Command Line grammar for Offset, signed Length, Reverse, Finish and Cancel.

C2 delivered:

- one top-level `Datum Plane` tool using the same C1 draft;
- Operations surface with Constructor = Offset, semantic Source, signed Offset, Reverse, Finish and Cancel;
- selection-first Origin XY/XZ/YZ source acquisition;
- selection-first planar Body Face acquisition converted to the durable semantic `SurfaceReference`;
- command-first `DATUM PLANE` / `DATUMPLANE` activation waiting on the same semantic selection path;
- global Command Line and Dynamic Input bound to the same draft/evaluation/Finish path as GUI;
- one-active-modeling-tool ownership, Enter/Esc behavior and stale-context rejection;
- transient semantic preview readiness from current draft evaluation, with no authored mutation before Finish.

The graphical Datum plane patch/border and plane/current-Body intersection overlay are intentionally not part of PM-03C; those remain PM-03D presentation responsibilities.

## Verification

C1 exact-head Windows FULL #1496 PASS on `c633a9714602801e1253d119198ea9e46f73f9a9`.

C2 exact-head Windows FULL #1507 PASS on `8b9185261e2cf32006f6af7a8c8a9da5e7c9ec04`.

Both FULL runs passed the repository full Windows verification path, including complete desktop build/test, core-only and kernel-native verification, selector verification, complete CTest and CI parity evidence.

Dedicated C1 regression proves:

- preview/Cancel/rejected/stale Finish create zero authored mutation/history;
- create/edit Finish uses one transaction;
- edit preserves DatumId;
- Show/Hide is authored and no-op-safe;
- dependent Datum Delete rejects atomically;
- Undo/Redo preserves expected authored semantics;
- abandoned Datum identities are not reused after history branching.

Dedicated C2 offscreen regression proves:

- GUI selection-first Origin source acquisition;
- one Datum Plane tool with Constructor = Offset and default 10 mm;
- Reverse changes only the signed Offset;
- command-first source acquisition through ordinary semantic Tree selection;
- Command Line and Dynamic Input update the same draft;
- Cancel authors nothing;
- empty Enter Finish through the global CAD input endpoint commits through the same semantic path;
- Origin-backed Datum creation does not invoke unrelated solid-modeling Kernel work.

## PM-03C gate closure

The accepted PM-03C gates are satisfied:

- GUI / Command Line semantic parity — PASS;
- Cancel / rejected Finish = zero mutation/history — PASS;
- edit preserves DatumId — PASS;
- Delete dependency rejection is atomic — PASS;
- stale draft cannot commit — PASS.

## Architecture boundary

PM-03C does not add:

- Datum plane Viewer patch/border presentation;
- plane/current-Body virtual intersection overlay;
- Datum presentation tokens or picking;
- `Reference Geometry` Tree group;
- Datum Properties surface;
- authored bulk visibility UI;
- Datum-backed Sketch support;
- Datum Axis / Datum Point;
- Projection.

Those remain owned by PM-03D and later ordered checkpoints under the unchanged accepted PM-03 Work Contract.

## Result

PM-03C is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-03D — Viewer / intersection overlay / Tree / Properties** under the unchanged accepted PM-03 Work Contract.

## Documentation impact

Internal docs: required — this completion record plus ACTIVE/roadmap/Work Contract checkpoint synchronization.  
User/Product docs: not required — final canonical user documentation remains a PM-03F completion obligation after the complete Datum-backed Sketch vertical slice exists.  
Product Browser: not required for this checkpoint-state closure.
