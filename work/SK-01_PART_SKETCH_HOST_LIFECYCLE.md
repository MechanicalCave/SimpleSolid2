# SK-01 — Part Sketch Host Lifecycle & 3D Edit Context

**Status:** ACCEPTED — IMPLEMENTED  
**Owner acceptance:** 2026-09-24  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related ADRs:** ADR-0002, ADR-0003, ADR-0004  
**Accepted ADR:** ADR-0005

## 1. Goal

Implement the smallest real Part-hosted Sketch vertical slice:

```text
open Part
→ invoke Sketch tool
→ select XY/XZ/YZ Origin plane
→ create durable empty Sketch
→ enter Sketch edit context in same 3D Viewport
→ camera aligns to Sketch plane
→ user may freely Pan/Zoom/Orbit
→ Finish Sketch
→ Save / Close / Reopen
→ same SketchId + support + placement + visibility
```

SK-01 intentionally proves host ownership, lifecycle, persistence and 3D editor integration before any 2D drawing entity exists.

## 2. Scope IN

### Domain

Implement a Part-owned hosted Sketch record containing:

- stable SketchId;
- semantic support restricted to XY/XZ/YZ built-in Origin planes;
- explicit validated SketchPlacement (origin + local U/V axes);
- persistent visibility;
- ordered collection embedded in Part authored state.

Implement semantic creation through DocumentSession/Part transaction with Undo/Redo.

### Persistence

Advance Part domain schema to v2.

Persist the hosted Sketch collection in `authored/document.json`.

Keep schema v1 readable as `sketches = []`; do not rewrite on load. Successful Save writes v2.

Reject malformed IDs, invalid support roles, duplicate SketchIds, non-finite/degenerate placement and support/placement mismatch.

### Workbench / UX

Add a user-facing `Sketch` tool in Operations.

Interaction:

1. activate `Sketch`;
2. status/Operations asks for a Sketch plane;
3. select XY/XZ/YZ Origin plane in Tree or 3D Viewport;
4. command creates the Sketch and enters edit context;
5. `Finish Sketch` leaves edit context.

Invalid supports do not create authored state.

Sketch nodes appear in the normal Part Document Tree.

### 3D edit context

While a Sketch is active:

- the existing Document Viewport remains the editor;
- reference grid is aligned to the Sketch placement;
- camera aligns normal to the plane on entry and performs Fit;
- normal ViewCube/Pan/Zoom/Orbit remains available after entry;
- navigation does not alter SketchPlacement or dirty state.

Finishing restores ordinary runtime context/grid without deleting the Sketch.

## 3. Scope OUT

```text
2D entities
Line / Arc / Circle
entity identity
constraints / dimensions / solver
snapping / inference
trim / extend / offset
profiles
Body / Feature / Extrude
Datum / Construction Plane implementation
planar face support
topology naming
external/projected references
Assembly Sketch host implementation
Drawing Sketch host implementation
Sketch delete/rename/reorder UX
thumbnail/tile work
```

No placeholder implementation may fake these features.

## 4. Architecture boundaries

- Sketch Core must not depend on Part/Assembly/Drawing/Qt/OCCT/filesystem.
- SK-01 may introduce only identity primitives needed for an empty reusable Sketch model; no speculative 2D framework.
- Part owns support/placement/persistence.
- UI selection is input only; command execution revalidates the selected semantic Origin plane.
- Viewer tokens and OCCT objects are never persisted.
- Camera/grid edit state is runtime-only.
- Persistent Sketch creation/visibility changes use Commands/Transactions.

If implementation requires topology identity, Datum semantics, Body/Feature structure or a new cross-domain dependency rule, stop and amend the contract.

## 5. Acceptance tests

At minimum prove:

1. Sketch tool can enter support-pick mode for an active Part.
2. XY, XZ and YZ Origin planes are valid supports.
3. Origin Point and X/Y/Z axes are rejected as Sketch supports.
4. Create command produces one durable empty Sketch with unique stable SketchId.
5. Sketch placement matches the selected Origin plane.
6. creation increments authored state once and is one Undo entry.
7. Undo removes the created Sketch; Redo restores the same SketchId.
8. duplicate SketchId in restored/persisted state is rejected.
9. Part schema v2 round-trips SketchId/support/placement/visibility.
10. Part schema v1 still opens with an empty Sketch collection and is not rewritten on load.
11. Save → Close → Reopen preserves the same Sketch semantic state.
12. Tree displays the Sketch as a normal Part object.
13. creation enters edit context in the same Workbench/Viewport.
14. grid frame changes to the Sketch placement while editing.
15. entry camera is aligned normal to XY/XZ/YZ support as appropriate.
16. Orbit/Pan/Zoom after entry remains available and does not mutate Part authored state.
17. Finish Sketch exits runtime edit context but keeps the durable Sketch.
18. switching/closing Documents clears runtime Sketch edit context safely.
19. existing Origin selection/visibility, ViewCube and native Viewer stress tests remain PASS.
20. exact-head Windows docs/verify/build/CTest gate is PASS.

## 6. Delivery slices

```text
Slice A
Sketch identity + Part-host support/placement model + semantic tests

Slice B
Part schema v2 + v1 reader compatibility + lifecycle tests

Slice C
Tree + Sketch tool + runtime edit context + grid/camera integration

Slice D
native/workbench regressions + docs + exact-head gate
```

Each slice must remain revertible without requiring implementation of the next one.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-01 introduces the first durable Sketch object, Part persistence schema v2, Sketch creation workflow and in-Viewport Sketch edit lifecycle.

Internal docs must describe identity/host ownership, support/placement, schema v1→v2 compatibility, command/undo behavior and runtime edit context.

Product docs PL/EN must describe the currently available workflow truthfully: create an empty Sketch only on XY/XZ/YZ Origin plane, same 3D Viewport, automatic alignment, free navigation and Finish Sketch. They must not claim that 2D geometry tools or model-face supports already exist.

Generated Product Browser must be regenerated and Git-clean.

## 7. Completion

SK-01 is complete only when the acceptance tests and documentation pass on exact head.

Completion does not activate SK-02 automatically. A later explicit contract is required for the first 2D entity model.


## 8. Completion record

SK-01 implementation is complete and ready for the final exact-head Windows gate.

Implemented state:

- Part hosts durable empty Sketch records with stable SketchId;
- SK-01 support is restricted to semantic XY/XZ/YZ built-in Origin planes;
- SketchPlacement is explicit authored Part state and is validated against its support;
- Sketch creation executes through DocumentSession and a Part transaction as one Undo/Redo entry;
- Undo removes the created Sketch and Redo restores the same SketchId/state;
- Part domain schema is v2 and persists SketchId, support, placement and visibility;
- Part schema v1 remains readable as an empty Sketch collection and upgrades only on a later successful Save;
- duplicate SketchId, invalid support and support/placement mismatch fail closed;
- the normal Part Tree presents hosted Sketches;
- the Operations surface provides Sketch / Cancel Sketch and Finish Sketch;
- successful creation enters a runtime Sketch edit context in the existing 3D Document Viewport;
- the runtime grid aligns to the Sketch U/V frame and the camera aligns normal to XY/XZ/YZ support with Fit;
- ViewCube and normal navigation remain available and do not mutate SketchPlacement or Part authored state;
- Finish Sketch, Document switching and close clear runtime edit context without deleting authored Sketch state;
- no 2D entities, constraints, solver, Datum, planar-face support, topology naming, Body/Feature, Assembly or Drawing scope was introduced;
- internal and PL/EN product documentation describe the as-built capability;
- the compiled CTest suite contains 35 tests, including the SK-01 host and Workbench lifecycle regressions.

The final completion condition is the exact-head Windows docs/verify/build/CTest gate on this completed contract state.
