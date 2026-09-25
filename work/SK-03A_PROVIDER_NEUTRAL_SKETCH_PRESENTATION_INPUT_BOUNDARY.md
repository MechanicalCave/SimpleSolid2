# SK-03A — Provider-neutral Sketch Presentation and Tool-input Boundary

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-25  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.1  
**Roadmap milestone:** R3 — Provider-neutral Sketch presentation and tool-input boundary

## 1. Goal

Establish the minimum provider-neutral presentation and spatial-input boundary required for the later interactive Sketcher without implementing the Line workflow itself.

R3 must make the existing durable R2 geometry visible in the active Sketch edit context and establish the runtime transport needed by future Select/create/edit tools:

```text
PartSketch + SketchModel
        ↓
Part/application presentation adapter
        ↓
neutral authored Sketch scene
        ↓
IDocumentViewport
        ↓
Qt/OCCT provider
```

and:

```text
Qt/OCCT pointer event
        ↓
neutral logical screen point + 3D ray
        ↓
Part/application Sketch-frame mapping
        ↓
active Sketch-local U/V
        ↓
future single ToolState boundary
```

R3 deliberately stops before a user can create, select or delete Line geometry through the Sketch tools.

## 2. Architectural decisions frozen by this contract

### 2.1 No universal Viewer scene framework

R3 extends the existing neutral Viewer contract with a bounded Sketch-specific presentation seam.

It does not replace `ReferenceScene` with a speculative universal scene graph and does not create PartViewer/SketchViewer stacks.

The existing shared Viewer remains one mechanism.

### 2.2 Authored Sketch presentation and runtime preview are separate channels

The Viewer receives authored Sketch geometry and runtime preview through distinct neutral contracts.

Required separation:

```text
authored Sketch scene
    stable semantic source: current Document state
    may carry runtime PresentationToken bindings

runtime preview scene
    transient interaction visualization
    no authored EntityId ownership
    no persistence
    no Undo/Redo
    no dirty/revision mutation
    independently replaceable/clearable
```

A preview object must never be mistaken for an authored Line merely because it has the same coordinates.

### 2.3 R3 presents the active Sketch only

R3 presents authored Line geometry for the currently active Part Sketch edit context.

It does not introduce the broader product policy for displaying all inactive/visible Sketches in a Part.

Outside Sketch edit context the new Sketch-authored scene is empty.

This keeps R3 bounded and leaves future inactive-Sketch visibility policy explicit.

### 2.4 PresentationToken remains runtime-only transport

Each presented authored Line may receive a runtime `viewer::PresentationToken`.

The token:

- is unique within the current Viewer presentation binding;
- is not persisted;
- is not derived by exposing or numerically encoding EntityId;
- may change when presentation is rebuilt;
- is mapped by the application/Part presentation adapter to the semantic address `SketchId + EntityId`;
- is never a replacement for SketchId/EntityId.

R3 may make the provider capable of returning the token through existing presentation-pick transport, but R3 does not create semantic Sketch selection from it. R4 owns that step.

### 2.5 Intrinsic Sketch Origin is an overlay, not authored Point geometry

While a Sketch is actively edited, its intrinsic Origin `(0,0)` is presented at the host frame origin.

The Origin overlay:

- is runtime presentation;
- is not inserted into SketchModel;
- has no EntityId;
- does not make the Sketch semantically non-empty;
- does not participate in profiles;
- is not made selectable/snappable by R3.

Exact glyph geometry, pixel size and color remain provider/UI detail.

### 2.6 Viewer spatial input is a neutral 3D ray

The Qt/OCCT provider converts a logical viewport position into a provider-neutral spatial ray.

Conceptual neutral values:

```text
ViewportPoint2
    logical x/y in viewport coordinates

Ray3
    finite 3D origin
    finite non-zero direction

SpatialPointerEvent
    phase
    logical viewport position
    Ray3
```

Required phases for R3:

```text
move
primary_press
primary_release
```

Qt event objects, OCCT view objects and device-native handles never leave the provider boundary.

Logical viewport coordinates are runtime screen-space values and never authored CAD coordinates.

### 2.7 Primary pointer routing has two neutral modes

R3 introduces a neutral runtime routing mode so a future create/edit tool does not accidentally trigger ordinary presentation selection with the same left click.

Conceptually:

```text
presentation_selection
    primary click follows existing PresentationToken pick path

spatial_tool_input
    primary pointer events are delivered as SpatialPointerEvent
    and do not also emit SelectionIntent for that same primary action
```

Middle-button navigation, Shift+Middle orbit, wheel zoom and other accepted global navigation remain available in both modes.

R3 does not decide Line commit-on-press versus commit-on-release. R4 decides tool semantics.

### 2.8 Cursor presentation is separate from routing and tolerances

R3 introduces neutral viewport cursor modes:

```text
system_default
select_pick_box
create_edit_crosshair
```

Entering Sketch edit defaults to:

```text
primary routing = presentation_selection
cursor = select_pick_box
```

Future create/edit tools may switch to:

```text
primary routing = spatial_tool_input
cursor = create_edit_crosshair
```

without changing the provider API.

Cursor glyph dimensions/colors/HiDPI artwork are not frozen.

The visible pick-box size is not defined to equal actual screen pick tolerance, snap tolerance or geometric tolerance.

### 2.9 Pointer-to-Sketch mapping belongs above the provider

The OCCT provider produces only the neutral 3D ray.

The Part/application presentation/input adapter resolves the currently active PartSketch and maps the ray to its host plane/frame.

For a valid metric orthonormal Sketch frame:

```text
N = UAxis × VAxis

ray-plane intersection
    → P3D

U = dot(P3D - Origin3D, UAxis)
V = dot(P3D - Origin3D, VAxis)
```

The mapping must remain correct after camera orbit because it depends on the spatial ray and Sketch frame, not on a fixed screen orientation.

Invalid/non-finite frames, invalid rays, parallel/no-forward-hit conditions fail cleanly and produce no resolved U/V position.

The exact numerical threshold used only to detect an effectively parallel ray is an implementation numerical-safety detail; it is not the Sketch geometric/snap/solver tolerance policy.

### 2.10 PartViewportController remains an adapter, not ToolState

The current `PartViewportController` may be extended to:

- resolve the active Sketch by SketchId;
- build active-Sketch authored presentation;
- maintain runtime PresentationToken ↔ SketchId/EntityId bindings;
- transform Sketch-local U/V to Viewer 3D presentation;
- map Viewer spatial rays back to active Sketch-local U/V;
- forward resolved runtime input through one callback/seam;
- configure Viewer routing/cursor presentation requested by the future tool layer.

It must not own:

- Line first/next point state;
- continuous-Line logic;
- Select semantic selection state for Sketch entities;
- rectangle selection;
- Delete;
- snapping/inference;
- constraints;
- Command Line parsing;
- dynamic input.

Those remain later milestones.

## 3. Proposed neutral Viewer contracts

Exact private implementation names may vary, but the following public semantics are part of R3.

### 3.1 Authored Sketch presentation

A neutral authored Sketch scene contains:

```text
0..N authored 3D line-segment presentations
    PresentationToken
    Start3D
    End3D

optional active-Sketch Origin overlay
    Point3
```

All values must be finite; exact-zero presented segments are invalid.

The scene is replacement-style: setting a new scene replaces the prior authored Sketch presentation.

### 3.2 Runtime preview presentation

A neutral preview scene contains zero or more finite 3D line segments without semantic PresentationTokens.

It is replacement-style and independently clearable.

R3 needs only line-segment preview capability because Line is the only implemented authored primitive. It does not introduce a universal preview-entity hierarchy.

### 3.3 Spatial pointer input

`IDocumentViewport` gains a provider-neutral spatial-pointer callback/handler seam carrying the event semantics from §2.6.

### 3.4 Runtime viewport interaction/cursor state

`IDocumentViewport` gains neutral setters for:

- primary-pointer routing mode;
- cursor presentation mode.

These are runtime presentation/input state only and are never persisted.

## 4. Part/application presentation behavior

### 4.1 Active Sketch scene

When no Sketch edit context is active:

```text
Sketch authored scene = empty
Sketch preview = empty
Sketch Origin overlay = absent
cursor/routing = ordinary non-Sketch viewport state
```

When Sketch edit context is active:

- resolve the Sketch by stable SketchId from the active PartDocument;
- rebuild authored Line presentation from its current `SketchModel`;
- transform each Line endpoint from U/V to 3D using the current `SketchPlacement`;
- present the intrinsic Origin overlay;
- keep the existing Sketch grid aligned to the same placement;
- default to presentation-selection routing and select-pick-box cursor.

Undo/Redo or other session refresh that changes the active SketchModel must rebuild presentation from current authoritative authored state.

If the active Sketch disappears, edit context closes fail-closed and Sketch presentation/preview are cleared.

### 4.2 U/V → 3D mapping

For each authored or future local preview point:

```text
P3D = Origin3D
    + U * UAxis3D
    + V * VAxis3D
```

No camera state participates in this authored presentation transform.

### 4.3 Runtime semantic binding

The adapter keeps runtime bindings conceptually equivalent to:

```text
PresentationToken
    ↔
SketchId + EntityId
```

Bindings are rebuilt with the presentation scene and are not persisted.

No universal durable SubElement API is introduced.

## 5. Qt/OCCT provider behavior

The native provider must:

- render the neutral authored Sketch Line scene;
- render/clear runtime preview independently;
- render active-Sketch Origin overlay;
- convert logical viewport positions to a valid provider-neutral 3D ray using current camera/view state;
- account for device pixel ratio inside the provider;
- dispatch spatial move/primary events only through the neutral handler;
- route primary action exclusively according to the active routing mode;
- preserve existing PresentationToken selection behavior in presentation-selection mode;
- preserve MMB pan, Shift+MMB orbit, wheel zoom, double-MMB Fit and reserved RMB behavior;
- apply select pick-box and create/edit crosshair cursor modes inside the viewport;
- clear transient OCCT detected state safely before callbacks that may synchronously rebuild scenes;
- contain recoverable OCCT/provider exceptions as existing Viewer policy requires.

No `AIS_InteractiveObject`, `V3d_View`, `gp_*`, Qt event pointer or provider-native identity may cross the neutral Viewer boundary.

## 6. Tool-state boundary established, tool behavior deferred

R3 establishes the one-way adapter seam future Shared 2D tool state will consume:

```text
mouse/provider
    ↓ SpatialPointerEvent
Viewer neutral boundary
    ↓ ray → active Sketch U/V
Part/application adapter
    ↓ resolved runtime Sketch pointer input
future one Sketch ToolState
```

and presentation returns through:

```text
future ToolState preview
    ↓ transient preview adapter
Viewer preview channel
```

R3 does not implement the future ToolState itself beyond configuring the default Sketch-edit viewport state.

In particular, R3 does not introduce a second Line implementation in CadWorkbench, Operations or Qt.

## 7. Deliberately OUT

```text
Line tool activation
first/next-point Line state
continuous Line creation
Line semantic commit from pointer events
commit/Undo grouping for continuous Line
semantic Sketch entity selection
primary/multi-selection for Sketch entities
rectangle/window/crossing selection
Delete selected Sketch geometry
Command Line coordinate grammar
Operations Line state/actions
dynamic input
grips/direct manipulation
Object Snap
geometric inference
constraints/dimensions/solver
measure/inspect
Circle/Arc
Trim/Extend/Offset
profiles/regions
planar-face Sketch support
projected/reference curves
inactive-Sketch display policy
Sketch visibility UI
universal scene graph
universal durable SubElement/reference API
```

No placeholders for these features are added.

## 8. Acceptance tests

At minimum prove:

1. Neutral authored Sketch scene validates finite non-zero 3D Line segments and unique valid runtime tokens.
2. Neutral preview scene is separate from authored scene and can be replaced/cleared independently.
3. Setting/clearing preview causes no Part revision, history or dirty-state mutation.
4. Entering an existing Sketch edit context presents all current authored Lines of that active Sketch.
5. No Sketch edit context produces an empty Sketch authored scene and no Origin overlay.
6. XY Sketch U/V endpoints map to expected 3D points.
7. XZ Sketch U/V endpoints map to expected 3D points.
8. YZ Sketch U/V endpoints map to expected 3D points.
9. Active Sketch intrinsic Origin is presented at host placement origin without creating an EntityId or authored entity.
10. Runtime Line PresentationTokens map back to the correct SketchId + EntityId without deriving token values from EntityId representation.
11. Presentation refresh after Add/Erase/Undo/Redo reflects current authored state while preserving CAD identity semantics.
12. Viewer spatial pointer event carries logical viewport coordinates and a finite non-zero neutral 3D ray.
13. Synthetic spatial rays map to correct active-Sketch U/V for XY/XZ/YZ frames.
14. Parallel/no-forward-hit/invalid ray or invalid frame fails cleanly without fabricated U/V.
15. The same geometric screen target maps correctly after camera orbit through provider ray generation.
16. presentation-selection routing preserves current reference PresentationToken selection behavior.
17. spatial-tool-input routing emits spatial primary events and does not also emit SelectionIntent for the same primary action.
18. Passive spatial pointer move is available for future rubber-band preview.
19. MMB pan, Shift+MMB orbit, wheel zoom and Fit remain operational in both routing modes.
20. Enter Sketch edit defaults to select-pick-box cursor mode.
21. create/edit crosshair mode can be requested through the neutral viewport contract without Qt types above the provider.
22. Leaving Sketch edit restores ordinary viewport cursor/routing and clears authored Sketch/preview/origin runtime presentation.
23. Sketch input/presentation activity does not dirty the Part or create Undo entries.
24. No Qt/OCCT/provider type leaks into neutral Viewer or Shared 2D contracts.
25. Existing reference scene/selection/navigation tests remain PASS.
26. Existing R1/R2 persistence/history tests remain PASS.
27. Internal and declared Product documentation are current.
28. FULL gate passes on the implementation head; completion bookkeeping may then use the accepted CI-01 CLOSURE tier.

Tests should prove neutral semantics and lifecycle rather than exact provider colors/pixel artwork.

## 9. Expected implementation surface

Expected production changes are bounded to:

```text
src/viewer/include/simplesolid2/viewer/**
src/viewer/**                         only neutral math/contracts if required
src/viewer_qt_occt/**
src/ui/part_viewport_controller.*
src/ui/cad_workbench.*
src/ui/viewport_surface.hpp           only if required by neutral boundary
src/CMakeLists.txt                    only if required
tests/**
docs/internal/CAD_WORKBENCH_VIEWER.md
docs/internal/SHARED_2D.md            current boundary update
docs/internal/PART_DOCUMENTS.md       current presentation/input boundary update
docs/internal/BUILD_AND_TEST.md
docs/product/pl/**                    only relevant current Sketch/Workbench page
docs/product/en/**                    paired equivalent
docs/browser/index.html               generated
work/ACTIVE.yaml
work/SK-03A_PROVIDER_NEUTRAL_SKETCH_PRESENTATION_INPUT_BOUNDARY.md
work/SKETCH_ROADMAP.md                completion bookkeeping only
```

No Part persistence schema change is expected.

No `src/sketch/**` authored-model mutation is expected. If implementation evidence requires a new Shared 2D public tool-input contract, stop and amend this Work Contract rather than adding it implicitly.

## 10. Suggested implementation slices

```text
Slice A
neutral Viewer Sketch scene + preview + ray/input/cursor contracts
and pure contract validation tests

Slice B
PartViewportController active-Sketch presentation adapter
U/V ↔ 3D mapping + runtime token bindings

Slice C
Qt/OCCT authored/preview/origin presentation
+ spatial ray generation + routing/cursor behavior

Slice D
CadWorkbench Sketch-edit integration
+ navigation/input/presentation regression tests

Slice E
as-built/product docs + full regression + completion bookkeeping
```

The slicing may change locally if dependency order requires it without changing contract semantics.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: R3 changes the as-built Viewer/input architecture and introduces visible active-Sketch authored geometry, Origin overlay and Sketch-edit cursor behavior even though interactive Line creation remains deferred.

## Roadmap impact

Roadmap milestone: R3 — Provider-neutral Sketch presentation and tool-input boundary  
Roadmap version: 1.1  
Roadmap change: none; this contract implements the accepted R3 milestone.

## 11. Completion

SK-03A completes only when:

- the neutral Viewer boundary supports authored Sketch scene, separate preview, spatial pointer rays, routing mode and cursor mode;
- active Part Sketch Lines and intrinsic Origin are presented through that boundary;
- pointer ray → active Sketch U/V works after orbit and fails cleanly when unresolved;
- navigation remains global and unchanged;
- no tool behavior or CAD mutation is hidden inside Viewer/provider/controller;
- runtime PresentationTokens remain non-durable semantic bindings;
- all acceptance tests and documentation pass;
- the implementation head passes FULL verification;
- completion bookkeeping passes the appropriate exact-head CI-01 tier.

Completion does not activate R4. R4 requires a separate explicit Owner-accepted Work Contract.
