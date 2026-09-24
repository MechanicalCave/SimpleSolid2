# SK-01A — Workbench Host Corrections & Close Safety

**Status:** ACCEPTED — IMPLEMENTED  
**Owner acceptance:** 2026-09-24  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related ADRs:** ADR-0002, ADR-0003, ADR-0005  
**Accepted ADR:** ADR-0006

## 1. Goal

Correct the first manual-test issues found immediately after SK-01 without beginning SK-02.

Priority order:

```text
P0  close lifetime crash
P1  Sketch re-entry from Tree
P1  toolbar vs contextual Operations UX
P1  neutral empty Project Workspace
```

## 2. P0 — active Document / Project close safety

Fix the confirmed lifetime ordering hazard where ProjectSession may erase a DocumentSession while Workbench controllers still hold non-owning pointers to it.

Required invariant:

```text
detach runtime UI/Viewer bindings
→ destroy/erase DocumentSession
→ update tabs/Workspace presentation
```

The same detach-before-destroy rule applies to closing the whole Project.

Add regression coverage that closes an active Part through the Workbench and closes a Project with an active Part without abort/use-after-free.

## 3. Sketch Tree edit entry

For existing Part-hosted Sketches:

- double-click Sketch Tree item → enter Sketch edit context;
- context menu on Sketch Tree item contains `Edit Sketch`;
- command uses stable SketchId from transient item metadata and revalidates against active Part Document;
- if Sketch no longer exists, edit entry fails closed/no-op with diagnostic status;
- entering edit does not create authored mutation or Undo entry.

No delete/rename/reorder Sketch UX is added.

## 4. Editor toolbar and contextual Operations

Add a toolbar region directly above the 3D editor surface.

Move the `Sketch` launcher from Operations to that editor toolbar.

Operations becomes context-only:

- idle Part edit → no permanent Sketch launcher;
- Sketch support-pick → guidance plus Cancel;
- active Sketch edit → Finish Sketch;
- leaving/cancelling the context restores idle Operations state.

No 2D geometry tools are added.

## 5. Neutral Project Workspace

Opening/creating a Project with no open CAD Document shows a neutral Workspace document-launch surface, not an inactive Part editor.

Current Workspace launch actions:

- `New Part…`;
- neutral `Open…`;
- `Refresh` if useful for document discovery.

Creating/opening a Part activates the existing Part Workbench.

Closing the last Part returns to neutral Workspace while keeping the Project open.

The architecture must route future document types by DocumentKind, but Assembly/Drawing creation/editors are outside SK-01A.

## 6. Scope OUT

```text
SK-02 2D entities
constraints / solver
Datum / Construction Plane
planar face support
Body / Feature / Extrude
Assembly implementation
Drawing implementation
Sketch delete / rename / reorder
new persistent schemas
```

## 7. Acceptance tests

At minimum prove:

1. closing an active clean Part through Workbench does not abort and removes its DocumentSession;
2. closing an active dirty Part after Save or Discard detaches runtime bindings safely;
3. closing Project detaches Workbench before ProjectSession destruction;
4. double-click existing Sketch Tree item enters edit context without authored mutation;
5. Sketch context menu exposes Edit Sketch and enters the same edit context;
6. invalid/stale SketchId cannot enter edit;
7. Sketch launch button is in editor toolbar, not Operations;
8. idle Operations has no persistent Sketch launcher;
9. support-pick Operations shows contextual cancel/guidance;
10. active Sketch edit Operations shows Finish Sketch;
11. Finish returns Operations to idle state;
12. Project with zero open Documents shows neutral Workspace;
13. creating/opening Part switches to Part Workbench;
14. closing last Part returns to neutral Workspace;
15. existing SK-01 persistence, camera/grid, Undo/Redo and native stress regressions remain PASS;
16. exact-head Windows docs/verify/build/CTest gate is PASS.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: changes Workspace/Workbench ownership, tool-launch placement, Operations semantics, Sketch edit entry and close lifecycle behavior.

## 8. Completion

SK-01A completes only after the manual P0 scenario is covered by automated regression and the exact-head Windows gate passes.

Completion does not activate SK-02.


## 9. Completion record

SK-01A implementation is complete and ready for the final exact-head Windows gate.

Implemented state:

- active Document close detaches Workbench/Tree/Viewer runtime bindings before ProjectSession erases the owning DocumentSession;
- Project close detaches CadWorkbench before ProjectHubController destroys the owning ProjectSession;
- the P0 close regression reported during manual SK-01 validation is covered by automated Workbench and Project Hub lifecycle tests;
- a Project with zero open CAD Documents shows a neutral Workspace instead of an inactive Part editor;
- current Workspace launch actions are New Part, neutral Open and Refresh;
- creating/opening a Part activates the Part Workbench; closing the last open Part returns to neutral Workspace while keeping the Project open;
- the Part Sketch launcher lives in the editor tool strip above the 3D Viewport;
- Operations is contextual: idle has no permanent Sketch launcher, support-pick exposes contextual Cancel/guidance, and active Sketch edit exposes Finish Sketch;
- existing Sketches re-enter the same runtime edit context by Tree double-click or Edit Sketch context menu;
- Tree routing uses transient stable SketchId data and revalidates the ID against the active Part before edit entry;
- re-entering Sketch edit is runtime-only and does not mutate authored state, revision or dirty state;
- no SK-02 geometry, constraints/solver, Datum/face support, Body/Feature, Assembly or Drawing implementation was introduced;
- internal and PL/EN product documentation describe the corrected Workspace/Workbench/Sketch UX and lifecycle ordering;
- the compiled CTest suite remains 35 tests;
- exact-head implementation gate #183 passed docs/verify/build and 35/35 tests, including native Workbench stress and the corrected Sketch double-click regression.

The final completion condition is one final exact-head Windows docs/verify/build/CTest gate on this completed governance state.
