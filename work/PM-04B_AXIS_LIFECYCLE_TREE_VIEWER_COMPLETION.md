# PM-04B — Axis Lifecycle / Tree / Properties / Viewer Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Checkpoint split:** B1 lifecycle/shared draft + B2 Tree/Properties/Viewer/Workbench  
**Date:** 2026-10-06

## Runtime evidence

### PM-04B1 — lifecycle / shared draft

- exact candidate: `37bdbd55cbb8ee2fc2120c88d0de80358c47b6aa`;
- Windows FULL #1533: PASS;
- merged main: `3d6d36c7e2253c0f071090d96d836abfe6c44d57`.

B1 delivered:

- Create/Edit/Delete/Show/Hide Axis semantic commands;
- AxisId high-water preservation across Undo/Redo and Undo branching;
- explicit re-source repair preserving AxisId and authored visibility;
- Regular/Construction Sketch Line source admission;
- deleted source Line -> durable repairable Missing Axis;
- shared `AxisDraft` for selection-first and command-first paths;
- command-line SOURCE / FINISH / CANCEL routing through the same semantic path;
- revision/generation exactness and stale draft rejection.

### PM-04B2 — Tree / Properties / Viewer / Workbench

- exact candidate: `da5e64dcaca0238391e4710f2edaacdfd198413b`;
- Windows FULL #1544: PASS;
- final aggregate `windows-msvc`: PASS;
- merged main: `e6660287c19ab245d622fad470e6d58323aeca24`.

B2 delivered:

- authored Axis as a child of its source Sketch;
- Missing-source Axis remains visible and repairable in the Tree;
- Axis Properties with AxisId, source Sketch/Line, visibility, current status/diagnostic and derived origin/direction;
- finite Part-level Axis presentation derived from the infinite semantic line;
- independent authored Show/Hide with no effect on Axis evaluation;
- Viewer `PresentationToken -> AxisId` picking; source Sketch EntityId is never pick identity;
- hidden/unresolved Axis publishes no stale successful Viewer line;
- Tree/Viewport/Properties Axis selection synchronization;
- Axis tool in Part Modeling toolbar + Operations surface;
- GUI selection-first and command-first workflows use the shared B1 `AxisDraft`;
- Edit/Cancel/Finish use the same semantic command path;
- active Axis draft is mutually exclusive with conflicting Datum/Extrude/Sketch-create/re-support contexts;
- global CAD Input routes `AXIS` to the same runtime draft.

## Verification

Windows FULL #1544 on exact B2 candidate `da5e64dcaca0238391e4710f2edaacdfd198413b` passed:

- complete desktop build graph;
- core-only build/test without Qt/OCCT;
- kernel-native Release build/test without Qt;
- FAST/SUBSYSTEM selector verification;
- complete desktop CTest;
- SR-02 latency benchmark evidence;
- CI-04 warm FULL parity/comparative evidence;
- final `windows-msvc` aggregate.

Dedicated regressions:

- `pm04b.axis_lifecycle_draft` proves lifecycle, repair, identity/high-water, stale-draft and shared-draft semantics;
- `pm04b2.axis_tree_viewer` proves Sketch -> Axis Tree placement, AxisId-based Tree/Viewer/Properties synchronization, finite presentation, Show/Hide, command-first runtime, Missing-source status and zero stale Viewer line.

## Architecture boundary

PM-04B does **not** add:

- Revolve Feature semantics;
- Revolve kernel/provider operation;
- Revolve topology catalog roles;
- Datum Axis;
- Body Edge/Curve AxisReference;
- Projection;
- any new Shared-2D Axis entity role.

Those remain outside PM-04B and are governed by later checkpoints.

## Result

PM-04B is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-04C — Revolve semantic/kernel operation + topology catalog** under the unchanged Owner-accepted PM-04 Work Contract.
