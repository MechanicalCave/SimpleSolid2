# SK-05A — Line Direct Manipulation and Primary Selection

**Status:** PROPOSED  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.1  
**Roadmap milestone:** R5 — Sketch direct manipulation and selection refinement  
**R5 delivery:** first bounded user-facing slice; R5 remains incomplete after SK-05A unless all R5 goals are later proven

## 1. Context

R4 established one semantic Sketch interaction authority, transient semantic selection by `EntityId`, point and Window/Crossing selection, continuous Line creation, atomic Delete and predictable Undo/Redo.

R5 adds direct manipulation without changing the accepted identity model:

- Line endpoints are authored coordinates, not separate durable Point entities;
- grips are runtime interaction handles;
- a center grip may translate a whole Line without creating midpoint identity;
- provider presentation tokens never become durable CAD identity;
- direct authored mutation must still pass through the ordinary command/transaction/document path.

SK-05A is deliberately limited to the existing Line primitive and existing Part-hosted active Sketch edit context.

## 2. Goal

Deliver the first complete direct-manipulation workflow for an authored Line:

```text
Select Line
→ expose Start / End / Center runtime grips
→ drag Start or End
→ runtime preview follows pointer
→ release commits one authored Line geometry update
→ same EntityId is preserved
→ one Undo step restores previous geometry

Select Line
→ drag Center grip
→ preview translates whole Line
→ release commits one authored geometry update
→ same EntityId is preserved
→ one Undo step restores previous geometry
```

No constraints, snapping, numeric entry or new geometry primitive is introduced.

## 3. Selection refinement

SK-05A uses the existing semantic selection state.

Rules:

- ordinary Left click on a Line replaces selection with that Line and makes it primary;
- Ctrl+Left click toggles Line membership;
- when Ctrl adds a Line, the added Line becomes primary;
- when Ctrl removes the primary Line, primary becomes empty rather than being inferred from provider/result ordering;
- Window/Crossing rectangle selection continues to replace the selection and assigns no primary;
- direct grips are shown only for a valid primary Line that is currently selected;
- clicking ordinary blank space clears selection and primary under the existing R4 grammar.

SK-05A does not add Shift selection, additive rectangle selection, selection cycling or filters.

## 4. Runtime grip semantics

The runtime semantic grip roles are finite:

```text
LineStart
LineEnd
LineCenter
```

A grip reference conceptually contains:

```text
SketchId
EntityId
GripRole
```

It is runtime interaction identity only and must never be serialized or used as durable CAD identity.

For a Line with authored/evaluated endpoints `A` and `B`:

- Start grip location = A;
- End grip location = B;
- Center grip location = (A + B) / 2.

The center position is derived. It does not create a midpoint entity, sub-entity or persistent reference.

## 5. Provider-neutral Viewer boundary

The common Viewer boundary may be extended only with the smallest finite contract required to:

- present runtime Sketch grips at provider-neutral Sketch/spatial positions;
- visually distinguish normal/hover/active grip state as runtime presentation;
- hit-test grips;
- emit neutral grip press/move/release/cancel interaction carrying semantic grip identity or an equivalent provider-neutral handle;
- keep Qt and OCCT types out of public contracts.

The Qt/OCCT provider owns pixels, sizing, HiDPI behavior, hit testing and native presentation.

The provider does not own selected EntityIds, authored Line geometry, command execution, Undo history or durable grip identity.

If implementation requires a generic overlay framework or provider-native identity in the semantic boundary, stop and propose a new D2 decision rather than broadening this contract.

## 6. Direct-manipulation runtime state

Direct manipulation is an adapter layered on the single active `SketchInteractionState`; it must not become a second authored interaction authority.

While one grip drag is active, runtime state may contain only what is needed to resolve that gesture, conceptually:

```text
active SketchId
Line EntityId
GripRole
authored geometry at drag start
current preview geometry
```

This state is transient and cleared on commit, cancel, edit-context loss, Document switch/close or fail-closed invalidation.

Only one grip drag may be active at a time.

## 7. Pointer mapping

Grip drag uses the already accepted provider-neutral spatial pointer/ray → active Sketch plane/frame mapping.

The pointer must resolve to valid Sketch-local U/V.

Failure to resolve a current plane intersection:

- does not guess from screen X/Y;
- does not commit geometry;
- keeps or cancels the runtime preview according to the bounded interaction implementation;
- reports a runtime diagnostic when appropriate.

No snap/inference tolerance is introduced.

## 8. Endpoint drag semantics

Dragging `LineStart`:

```text
original Line = A → B
pointer = P
preview = P → B
```

Dragging `LineEnd`:

```text
original Line = A → B
pointer = P
preview = A → P
```

The opposite endpoint remains unchanged.

Equal coordinates do not imply shared endpoint identity with any other Line. Moving one endpoint does not move geometrically coincident endpoints of other entities.

Constraints do not exist in SK-05A and therefore cannot propagate the edit.

## 9. Center drag semantics

For a center-grip drag:

```text
drag start center = C0
current pointer = P
delta = P - C0

preview start = A + delta
preview end   = B + delta
```

The whole Line translates rigidly.

The operation does not create or persist a midpoint entity.

## 10. Preview versus authored mutation

Pointer movement during a grip drag is runtime preview only.

It must not:

- increment DocumentRevision;
- set dirty/needsSave;
- create Undo entries;
- mutate the authoritative SketchModel;
- change EntityId.

On successful primary-button release, exactly one authored update command is executed if resulting geometry differs from the authored drag-start geometry.

No command is executed for an exact no-op release.

## 11. Semantic command and transaction path

Authored direct manipulation must use a semantic command through the existing document transaction path.

Preferred bounded semantic shape:

```text
UpdateSketchLineGeometryCommand
    SketchId
    EntityId
    replacement Line geometry
```

The command expresses resulting authored Line geometry, not a runtime grip operation.

The owning Part/Document layer validates:

- Sketch exists;
- EntityId exists in that Sketch;
- entity is a Line;
- replacement geometry is valid;
- mutation preserves the existing EntityId.

One successful drag release:

```text
neutral grip release
→ validated resulting Line geometry
→ semantic command
→ PartDocumentTransaction
→ owning PartDocument / embedded SketchModel
→ presentation refresh
```

No Viewer/provider object participates in persistence or authored identity.

## 12. Identity and history

A direct edit preserves the Line `EntityId`.

Undo restores the prior coordinates with the same EntityId.

Redo reapplies the edited coordinates with the same EntityId.

Direct manipulation must not erase-and-recreate a Line merely to change its coordinates.

A semantic future Copy operation remains responsible for fresh identity; SK-05A does not implement Copy.

## 13. Drag commit/cancel behavior

Primary press on a visible grip begins a possible drag.

During drag:

- the grip becomes active visually;
- authored Line presentation remains authoritative;
- a separate runtime preview shows proposed geometry;
- selection remains the same semantic EntityId set.

Primary release with changed valid geometry:

- executes one semantic update command;
- clears preview/active-grip runtime state;
- refreshes authored presentation;
- preserves the edited Line selection and primary identity;
- creates one Undo entry.

Primary release with unchanged geometry:

- clears runtime drag state;
- creates no authored mutation/history.

Esc while dragging:

- cancels the drag;
- clears preview;
- restores ordinary selected-Line presentation;
- creates no authored mutation/history.

## 14. Interaction with tools and navigation

Direct manipulation is available only while Sketch `Select` is active.

Activating Line or leaving Select cancels any uncommitted grip drag first.

While no grip drag is active, existing R4 navigation remains unchanged:

- MMB Pan;
- Shift+MMB Orbit;
- wheel Zoom;
- Navigation Cube.

A primary-button grip drag owns that primary gesture and must not also trigger ordinary Line point selection.

SK-05A does not add free-drag movement by clicking Line bodies. Movement is only through explicit grips.

## 15. Presentation rules

For the primary selected Line, present exactly three runtime grips:

- Start;
- End;
- Center.

Grips must remain visibly and hit-testably anchored to current projected evaluated geometry after camera navigation and viewport resize.

Exact visual style is provider presentation detail, but must:

- remain legible at supported Windows DPI scales;
- distinguish endpoint grips from the center grip enough to avoid ambiguous hit testing;
- avoid introducing QWidget-over-native-viewport composition;
- render inside the provider/native graphics path or another already accepted stable provider presentation path.

No grip styling becomes CAD semantics.

## 16. Selection/presentation coherence

Semantic `EntityId` selection remains authoritative.

After direct edit, Undo/Redo, camera navigation or authored scene rebuild:

- the selected Line remains selected if the EntityId still exists;
- primary remains the same EntityId if still selected;
- grip positions are regenerated from current evaluated geometry;
- provider-local presentation handles/tokens may change freely.

If history removes the active Line or active Sketch, runtime selection/grips are reconciled or cleared fail-closed.

## 17. Persistence

SK-05A introduces no new persistence schema and no durable grip state.

Edited Line coordinates already belong to authored Sketch geometry and must therefore survive the existing:

```text
Save → Close → Reopen
```

lifecycle with the same `SketchId` and `EntityId`.

## 18. Properties and Operations

SK-05A does not introduce property-grid numeric geometry editing.

Properties/Operations may reflect the current selected/primary Line and runtime grip-drag status, but they remain adapters and must not own another direct-edit state machine.

Minimum Operations behavior may remain the existing Select context plus status text; new permanent controls are not required for grip dragging.

## 19. Deliberately out of scope

```text
snapping / Object Snap
geometric inference
constraints / solver
dimensions
numeric coordinate/value entry
dynamic input
dragging Line body without a grip
multi-entity Move
rotation / scale
Circle / Arc grips
Trim / Extend / Offset
selection cycling
Shift-selection grammar
Ctrl-additive rectangle selection
selection filters
hover sub-element semantic selection outside grip hit testing
durable midpoint entity
new universal SubElement persistence API
construction geometry
profiles/regions
projected/reference geometry
planar-face Sketch support
Body/Feature/Extrude architecture
generic Viewer overlay framework
```

## 20. Expected implementation surface

Expected production changes are bounded to:

```text
src/sketch/** for Line geometry replacement semantics if required
src/part/** for semantic update command / transaction integration
src/viewer/include/simplesolid2/viewer/** for finite neutral grip presentation/input
src/viewer_qt_occt/** for native grip rendering/hit testing only
src/ui/** for the existing Sketch interaction coordinator and selection projection
tests/**
docs/internal/SHARED_2D.md
docs/internal/CAD_WORKBENCH_VIEWER.md
docs/internal/PART_DOCUMENTS.md
docs/internal/BUILD_AND_TEST.md as needed
docs/product/en/PARTS.md
docs/product/pl/PARTS.md
work/**
```

No persistence schema migration is expected.

If implementation requires a new durable reference model, constraints, snapping, generic overlay subsystem or provider-native identity in public contracts, stop.

## 21. Verification

Automated coverage must prove at minimum:

1. ordinary Line selection produces a primary selected Line;
2. Ctrl-add sets the added Line primary;
3. Ctrl-removing primary leaves no inferred primary;
4. rectangle selection keeps no primary;
5. grips are exposed only for a valid selected primary Line;
6. exactly Start/End/Center grip roles exist for a Line;
7. grip positions derive from current Line geometry;
8. provider hit testing returns semantic neutral grip interaction rather than OCCT identity;
9. grip press does not also trigger ordinary primary selection input;
10. endpoint preview moves only the dragged endpoint;
11. center preview translates both endpoints by one delta;
12. preview movement creates no revision/dirty/Undo mutation;
13. Esc cancels drag with no authored mutation;
14. exact no-op release creates no command/history;
15. valid release executes exactly one semantic geometry-update command;
16. direct edit preserves EntityId;
17. Undo restores prior coordinates and preserves EntityId;
18. Redo reapplies edited coordinates and preserves EntityId;
19. selection/primary survive a successful edit by EntityId;
20. grip positions refresh from current geometry after edit and Undo/Redo;
21. active Sketch/Document loss clears drag preview and grips fail-closed;
22. Save → Close → Reopen preserves edited coordinates and identity;
23. Pan/Orbit/Zoom/Navigation Cube remain functional outside active LMB grip drag;
24. provider/native stress covers repeated show/hit/drag-preview/clear lifecycle and resize;
25. exact-head Windows FULL CI passes.

Manual Windows verification is required for the final exact-head candidate because grip usability and native presentation cannot be proven fully by semantic tests. Verify at least:

- endpoint and center grips are visually distinct and clickable;
- repeated endpoint/center drags do not produce stale pixels or viewport disappearance;
- grips remain anchored after orbit, zoom, splitter resize and supported DPI scaling;
- drag preview follows the intended Sketch-plane location;
- release lands on the same geometry shown by preview;
- Undo/Redo visibly restores/reapplies geometry;
- Cube/Pan/Orbit/Zoom remain coherent after grip use.

## 22. Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-05A adds a new user-visible editing workflow and extends provider-neutral runtime Viewer/interaction contracts.

## 23. Acceptance

SK-05A is complete only when:

1. the Owner has explicitly accepted this D2 contract;
2. implementation remains within the bounded Line-only direct-manipulation scope;
3. direct edits use the semantic command/transaction path;
4. no grip/provider token becomes durable identity;
5. all required automated coverage passes;
6. exact-head Windows FULL passes;
7. required internal and PL/EN product documentation is current;
8. generated Documentation Browser is current;
9. explicit manual Windows verification passes on the exact final implementation candidate;
10. closeout CLOSURE gate passes;
11. R5 roadmap status is not marked completed unless the remaining R5 goals are separately proven;
12. no R6+ capability is activated implicitly.
