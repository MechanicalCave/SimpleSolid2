# SK-04B — Provider-neutral Sketch Selection Query, Box Overlay and Token Bridge

**Status:** PROPOSED  
**Owner acceptance:** pending  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.1  
**Roadmap milestone:** R4 — First complete continuous Line workflow  
**R4 delivery split:** SK-04A / SK-04B / SK-04C

## 1. Context

R3 completed the provider-neutral active-Sketch presentation and spatial-pointer boundary.

SK-04A completed the host-neutral interaction authority:

```text
SketchInteractionState
  Select
  Line / AwaitFirstPoint
  Line / AwaitNextPoint

semantic transient selection
  EntityId[]
  optional primary EntityId
```

The missing bridge for the final R4 UI is view-dependent selection:

```text
logical screen point
  → current active-Sketch presentation hit
  → PresentationToken
  → SketchId + EntityId
  → SketchInteractionState selection

logical screen rectangle
  → current projected active-Sketch presentation hits
  → PresentationToken[]
  → EntityId[]
```

The provider must answer view-space questions without becoming the semantic owner of selection.

## 2. Goal

Implement the smallest provider-neutral boundary needed by SK-04C to build point and rectangle selection for authored active-Sketch Lines.

SK-04B establishes:

- synchronous point-hit query for active-Sketch authored presentation;
- synchronous rectangle-hit query with explicit Window/Crossing rule;
- runtime selection-box overlay presentation;
- current-scene PresentationToken ↔ SketchId + EntityId bridge in the Part/UI adapter;
- reverse semantic EntityId → current PresentationToken mapping for highlight projection;
- independent Sketch pointer-routing and cursor configuration in the Part/UI adapter;
- Qt/OCCT implementation of those runtime services;
- no user-facing Select drag state and no mutation of `SketchInteractionState`.

## 3. Architectural boundary

The intended flow is:

```text
future SK-04C Select adapter
    │
    ├── point query
    │     ↓
    │   IDocumentViewport
    │     ↓
    │   Qt/OCCT current-view hit test
    │     ↓
    │   PresentationToken
    │     ↓
    │   PartViewportController bridge
    │     ↓
    │   EntityId
    │
    └── rectangle query
          ↓
        IDocumentViewport
          ↓
        provider current-view projection query
          ↓
        PresentationToken[]
          ↓
        PartViewportController bridge
          ↓
        EntityId[]
```

The provider answers only "which current presentations are hit?"

The provider does not decide:

- replace/toggle/clear semantics;
- semantic selected EntityIds;
- primary semantic selection;
- Ctrl behavior;
- drag threshold;
- Delete;
- Line tool state;
- Undo/Redo.

Those remain above the provider.

## 4. Query scope

### 4.1 Active-Sketch authored Lines only

SK-04B queries target only current authored `SketchLinePresentation` objects from the active Sketch scene.

Queries do not return:

- built-in Part Origin references;
- grid presentation;
- intrinsic Sketch Origin overlay;
- transient Sketch preview Lines;
- inactive Sketch geometry;
- QWidget/UI controls.

This is deliberately a bounded Sketch-specific seam rather than a universal scene-query framework.

Future projected/reference geometry may extend selection semantics under its own contract.

### 4.2 Runtime-only identity

Query results contain only runtime `PresentationToken` values.

A token:

- is valid only for the current presentation binding;
- is never persisted;
- may change after any scene rebuild;
- is never copied into `SketchInteractionState`;
- must be mapped immediately to semantic `SketchId + EntityId` by the Part/UI adapter.

## 5. Provider-neutral screen-space types

SK-04B may add small neutral runtime types under Viewer for selection queries.

Conceptually:

```text
ViewportRect2
{
    min logical viewport point
    max logical viewport point
}

SketchRectangleSelectionRule
{
    window
    crossing
}

SketchSelectionBoxOverlay
{
    anchor logical point
    current logical point
    rule
}
```

Rules:

- coordinates are logical viewport coordinates, not device pixels;
- all coordinates must be finite;
- query rectangles are normalized, axis-aligned screen-space rectangles;
- rectangle query requires non-zero width and height;
- overlay may temporarily have zero width/height while a future drag begins;
- device-pixel-ratio conversion remains provider-internal;
- screen-space rectangle size is not a CAD/geometric tolerance.

Exact public C++ names may differ if the same semantics remain explicit and bounded.

## 6. Point-hit query

The neutral Viewer boundary must support a synchronous point query conceptually equivalent to:

```text
query active-Sketch presentation at logical viewport point
→ completed + optional PresentationToken
```

The result must distinguish:

```text
completed, no hit
completed, one hit
query failed / unavailable
```

A provider failure must not be silently treated as a semantic clear-selection action.

### 6.1 Point semantics

- only authored active-Sketch Line presentations are eligible;
- no hit returns a successful empty result;
- one provider-selected top hit is sufficient for R4;
- overlap cycling/disambiguation is deferred;
- exact pixel pick tolerance is provider/runtime policy and remains distinct from pick-box cursor size, snap tolerance and geometric tolerance;
- point query creates no semantic selection by itself.

### 6.2 Query side effects

A point query must not:

- call `SelectionIntentHandler`;
- mutate `PresentationSelection`;
- mutate Sketch/Core/Part state;
- create Undo/dirty/revision effects;
- leave provider detected/highlight state that becomes a second selection authority.

Temporary provider detection may be used internally but must be cleared before returning or before any callback could rebuild the scene.

## 7. Rectangle query

The neutral Viewer boundary must support a synchronous rectangle query conceptually equivalent to:

```text
query active-Sketch presentations in ViewportRect2
using explicit Window or Crossing rule
→ completed + unique PresentationToken[]
```

The result must distinguish provider failure from a successful empty result.

Result order is runtime/provider detail and must not become semantic selection ordering or identity.

### 7.1 Window rule

```text
Window
→ authored Sketch Line presentation is returned only when
  its current projected line segment lies fully inside the screen rectangle
```

For a straight Line and convex axis-aligned rectangle this is equivalent to both projected endpoints being inside the rectangle.

### 7.2 Crossing rule

```text
Crossing
→ authored Sketch Line presentation is returned when
  its current projected line segment intersects/touches the rectangle
  or lies inside it
```

### 7.3 View-space meaning

Rectangle selection is explicitly view-space behavior:

- it uses the current camera/projection;
- it must remain correct after orbit;
- it does not map the screen rectangle into an authored U/V region;
- it does not create or modify Sketch geometry;
- it does not use solver/snap/geometric tolerance as selection-box semantics.

The R4 direction grammar is already accepted:

```text
future drag Left → Right  => Window
future drag Right → Left  => Crossing
```

SK-04B receives the explicit rule. It does not own drag direction state.

### 7.4 Current-scope occlusion policy

R4 currently presents only active-Sketch authored Lines without modeled solid B-Rep.

SK-04B therefore defines rectangle membership from the current projected Line presentation and does not freeze a future solid-occlusion/hidden-geometry selection policy.

A later modeling/visibility contract may refine occlusion semantics without changing semantic EntityId selection identity.

## 8. Selection-box overlay

The Viewer boundary gains one runtime selection-box overlay channel.

The overlay:

- is independent from authored Sketch scene;
- is independent from Sketch preview scene;
- contains no PresentationToken;
- contains no EntityId;
- creates no dirty/revision/Undo effect;
- is replaceable/clearable;
- uses logical viewport coordinates;
- carries explicit Window/Crossing rule so the provider can visually distinguish the modes.

Exact:

- colors;
- alpha;
- fill;
- dashed/solid border;
- pixel width;
- HiDPI rendering details

remain provider presentation details.

The overlay must not be implemented as authored CAD geometry or an OCCT semantic selection object.

## 9. Part/UI semantic token bridge

`PartViewportController` already owns current runtime bindings:

```text
PresentationToken → SketchId + EntityId
```

SK-04B extends that adapter to support the reverse current-scene direction needed for selection highlight:

```text
active SketchId + EntityId → current PresentationToken
```

### 9.1 No semantic selection ownership

The controller must not become a second `SketchInteractionState`.

It may:

- map one current token to `SketchId + EntityId`;
- map current query token arrays to semantic EntityIds;
- map semantic active-Sketch EntityIds back to current tokens;
- construct/apply current `PresentationSelection` from caller-supplied semantic IDs;
- combine that presentation projection with existing built-in reference presentation selection as needed.

It must not persist or independently author the semantic selected EntityId set.

### 9.2 Fail-closed mapping

Mapping fails cleanly when:

- token is invalid;
- token no longer belongs to current scene;
- token belongs to another presentation category;
- EntityId does not belong to the active Sketch;
- a selected EntityId has no current presentation binding;
- supplied primary EntityId is not in the supplied selected set.

No stale token may silently select a different entity after presentation rebuild.

### 9.3 Rebuild behavior

After authored presentation rebuild:

- old Sketch PresentationTokens are stale;
- semantic EntityIds remain the selection authority in future SK-04C;
- caller may project the same EntityIds back to newly allocated tokens;
- no semantic selection depends on token stability.

## 10. Reference selection compatibility

Existing Part Origin/reference point-selection behavior remains supported.

The legacy `SelectionIntent` path remains valid for built-in reference selection.

SK-04B does not reinterpret a Sketch Line token delivered through that legacy path as semantic R4 Sketch selection.

Future SK-04C Select uses spatial press/move/release plus explicit point/rectangle queries for Sketch geometry.

## 11. Pointer routing and cursor must remain independent

R3 made these neutral Viewer concepts independent:

```text
PrimaryPointerRouting
ViewportCursorMode
```

The current Part adapter helper couples:

```text
spatial_tool_input
↔ create_edit_crosshair
```

That coupling is insufficient for future R4 Select.

SK-04B must allow this valid combination:

```text
Select
→ spatial_tool_input routing
→ select_pick_box cursor
```

and also preserve:

```text
Line/create-edit
→ spatial_tool_input routing
→ create_edit_crosshair cursor
```

No Part/UI adapter API may require spatial routing to imply crosshair cursor.

Exact final tool activation wiring remains SK-04C.

## 12. Qt/OCCT provider requirements

The Qt/OCCT provider must:

- accept logical viewport coordinates and perform DPR conversion internally;
- query only current active-Sketch authored Line presentation for the new Sketch queries;
- return runtime PresentationTokens only;
- keep query operations independent from semantic selection callbacks;
- avoid persistent mutation of OCCT selection state;
- clear/restore temporary detected/query state safely;
- support current camera orientation/projection including after orbit;
- implement Window and Crossing according to section 7;
- render/clear the neutral selection-box overlay;
- contain provider exceptions at the existing boundary;
- preserve MMB Pan, Shift+MMB Orbit, wheel Zoom and ViewCube behavior.

Provider-native Qt/OCCT objects must not escape into Viewer/UI semantic APIs.

## 13. Deliberately OUT of SK-04B

```text
SketchInteractionState ownership in CadWorkbench
actual Select click state machine
mouse drag threshold
Ctrl modifier transport / Ctrl-toggle wiring
rectangle drag start/update/end state
Left→Right / Right→Left gesture detection
semantic replace/toggle/clear invocation
Line toolbar activation
continuous Line mouse workflow
Line preview wiring from SketchInteractionState
AddSketchLineCommand execution from pointer clicks
Delete key/action wiring
EraseSketchEntitiesCommand invocation from UI
Operations Line UI
Command Line UI
Esc keyboard plumbing
Undo/Redo tool cancellation wiring
dynamic input
Object Snap
inference
grips
constraints/dimensions/solver
Circle/Arc/Trim/etc.
projected/reference geometry selection
inactive Sketch display/selection
universal Viewer scene/query framework
```

No placeholders for these features are added.

## 14. Expected implementation surface

Expected production changes are bounded to:

```text
src/viewer/include/simplesolid2/viewer/**
src/viewer_qt_occt/**
src/ui/part_viewport_controller.*
tests/**
docs/internal/CAD_WORKBENCH_VIEWER.md
docs/internal/BUILD_AND_TEST.md
work/**
```

No authored Sketch model, DocumentSession command, Part persistence schema or Product documentation change is expected.

If implementation requires changing `SketchInteractionState` semantics, DocumentSession authored commands or final Workbench tool behavior, stop and move that work to SK-04C/amendment rather than expanding this contract silently.

## 15. Acceptance tests

At minimum prove:

1. Neutral point-query input rejects non-finite logical coordinates.
2. Completed point query can represent no hit distinctly from provider failure.
3. Point query returns only active-Sketch authored Line tokens.
4. Built-in Origin/reference presentation is ignored by Sketch point query.
5. Intrinsic Sketch Origin overlay is ignored by Sketch point query.
6. Sketch preview presentation is ignored by Sketch point query.
7. Point query invokes no `SelectionIntentHandler`.
8. Point query does not mutate `PresentationSelection`.
9. Point query leaves no persistent provider detected-selection state.
10. Valid rectangle query requires finite non-zero logical width/height.
11. Window query returns a projected Line fully inside the rectangle.
12. Window query excludes a Line that only crosses/touches the rectangle.
13. Crossing query returns a Line crossing/touching the rectangle.
14. Crossing query returns a Line fully inside the rectangle.
15. Rectangle results contain unique valid PresentationTokens.
16. Rectangle query ignores reference/grid/origin/preview presentation.
17. Rectangle query remains correct after camera orbit for a known projected setup.
18. Query result order is not used as semantic identity.
19. Selection-box overlay accepts finite logical anchor/current points.
20. Selection-box overlay can represent both Window and Crossing modes.
21. Overlay replace/clear does not alter authored/preview scenes.
22. Overlay operations produce no Part revision/dirty/Undo change.
23. Current token maps to the correct active `SketchId + EntityId`.
24. Stale token after presentation rebuild does not map to a semantic entity.
25. Active EntityId maps back to its current runtime token.
26. Unknown/stale EntityId fails reverse mapping.
27. Semantic selected EntityIds plus valid primary can be projected to current `PresentationSelection`.
28. Primary outside selected set fails closed.
29. Existing built-in reference presentation selection remains functional.
30. Reference and Sketch presentation highlight projection can coexist without making token identity semantic.
31. Part/UI adapter can configure `spatial_tool_input + select_pick_box`.
32. Part/UI adapter can configure `spatial_tool_input + create_edit_crosshair`.
33. Routing change does not force cursor change and cursor change does not force routing change.
34. Middle-button Pan remains available under the new selection-query boundary.
35. Shift+MMB Orbit remains available.
36. Wheel Zoom remains available.
37. Existing R1/R2/R3/SK-04A tests remain PASS.
38. No Viewer/provider type leaks into Sketch Core.
39. Internal as-built documentation is current.
40. FULL exact-head gate passes on implementation.
41. Completion bookkeeping uses the appropriate CI-01 non-build tier.

## Documentation Impact

Internal docs: required  
User/Product docs: not required  
Reason: SK-04B establishes provider-neutral selection infrastructure and presentation adapters but does not yet expose the final user-facing Select/rectangle workflow.

## 17. Roadmap impact

Roadmap milestone: R4 — First complete continuous Line workflow  
Roadmap version: 1.1  
Roadmap change: none.

SK-04B is the second bounded R4 contract. R4 remains incomplete after SK-04B.

## 18. Completion

SK-04B completes only when:

- active-Sketch point and rectangle queries exist behind the neutral Viewer boundary;
- Window/Crossing view-space behavior is proven;
- selection-box overlay exists as runtime presentation only;
- current token ↔ semantic Sketch entity bridging works both directions without semantic token ownership;
- Select spatial routing can coexist with pick-box cursor;
- legacy Part reference selection and navigation remain intact;
- provider-specific details remain contained in Qt/OCCT;
- all acceptance tests and internal docs pass;
- implementation exact head passes FULL verification;
- completion bookkeeping passes the appropriate exact-head CI tier.

Completion of SK-04B does not activate SK-04C automatically. SK-04C requires a separate explicit Owner-accepted Work Contract.
