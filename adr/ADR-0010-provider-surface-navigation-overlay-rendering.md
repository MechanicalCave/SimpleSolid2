# ADR-0010 — Provider-surface 3D Navigation Cube

**Status:** ACCEPTED  
**Date:** 2026-09-25  
**Owner acceptance:** 2026-09-25  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** ADR-0003, ADR-0006, WB-01B

## Context

The current Windows Qt/OCCT Workbench renders the CAD scene in a native `WA_PaintOnScreen` viewport while ViewCube is implemented as a QWidget overlay.

Manual WB-01B verification established that this composition is not reliably artifact-free. Two bounded D1 alternatives were tested and rejected:

1. ViewCube as a native child of the native OCCT viewport;
2. ViewCube and OCCT viewport as native siblings under a native Editor Surface host.

Both retained stale/overdrawn ViewCube artifacts in manual Windows testing.

By contrast, Sketch rectangle selection became visually stable after moving its rectangle presentation into the OCCT render surface with `AIS_RubberBand`.

The current ViewCube is also functionally too limited: it is a mostly static UI drawing with a small set of navigation actions and does not act as a complete camera-orientation manipulator.

WB-01B therefore exposed both an implementation problem and an opportunity to replace the limited overlay with the intended CAD navigation behavior rather than reproducing the old widget inside the provider.

## Decision

Replace the visible QWidget ViewCube with a provider-surface **3D Navigation Cube** rendered and hit-tested inside the same native graphics surface as the CAD view.

The Cube is a runtime camera-navigation control. It is not CAD geometry and never becomes authored or persisted model state.

The common Viewer boundary owns the provider-neutral navigation meaning and interaction intent. The Qt/OCCT provider owns the native in-surface presentation, animation frames and hit testing.

Conceptually:

```text
Workbench / camera navigation semantics
        ↓
provider-neutral Navigation Cube contract
        ↓
Qt/OCCT provider
        ↓
in-surface 3D Cube + labels + navigation arrows + hit testing
        ↓
neutral navigation intent
        ↓
camera transition
```

The public contract must not expose Qt widgets, OCCT handles, raw window handles, topology objects or provider-native identity.

## Navigation Cube behavior

### Camera synchronization

The Cube orientation is always derived from the current camera orientation.

When the user orbits the main view by ordinary viewport navigation, the Cube rotates correspondingly so that it continuously represents the camera orientation relative to the model/world axes.

The Cube does not rotate or transform the model. It only visualizes and changes the camera.

### Face picking

All six faces are interactive and labeled:

- Front
- Back
- Left
- Right
- Top
- Bottom

Clicking a face animates the camera to the corresponding canonical face orientation while preserving the current projection mode.

The opposite faces remain reachable naturally as the Cube rotates; they are not hidden semantically just because they are initially on the far side.

### Edge picking

All 12 Cube edges are interactive.

Clicking an edge animates the camera to the canonical orientation halfway between the two adjacent face directions.

Examples:

- Top + Front;
- Front + Right;
- Bottom + Back.

### Corner picking

All 8 Cube corners are interactive.

Clicking a corner animates the camera to the canonical three-axis isometric orientation represented by those three adjacent faces.

This provides all eight isometric directions, including the orientations opposite the initial Top-Front-Right view.

### Adjacent-view arrows

When the Cube is aligned to a canonical face-oriented view, directional arrows around the Cube provide discrete 90-degree navigation to the adjacent face views.

These actions are semantic 90-degree camera changes, not arbitrary Euler-angle accumulation.

Examples include:

- Front → Right;
- Front → Left;
- Front → Top;
- Front → Bottom.

### Roll arrows

Dedicated roll arrows rotate the camera by exactly +90 or -90 degrees around the current viewing axis while preserving the current viewing direction.

For example, Front remains Front while screen-up rotates by 90 degrees.

The implementation must use a stable camera basis/orientation representation rather than cumulative unconstrained Euler angles.

### Home

A dedicated Home action always performs:

```text
Top-Front-Right isometric orientation
+
Fit All
+
preserve current projection
```

Home is fixed in this stage. User-defined Home views are out of scope.

### Animation

Face, edge, corner, adjacent-view, roll and Home orientation changes are animated.

The animation is runtime-only and must be short, deterministic and interruptible by a subsequent navigation command. Intermediate animation frames must never mutate authored CAD state or create Undo history.

Exact timing/easing is an implementation detail as long as the final camera state is exact and deterministic.

### Projection policy

Camera orientation and camera projection are independent.

Face, edge, corner, adjacent-view, roll and Home actions preserve the current projection mode. A user in Perspective remains in Perspective after Navigation Cube orientation actions; a user in Orthographic remains in Orthographic.

A separate provider-surface `ORTHO/PERSP` control is presented next to the Cube. It changes only the camera projection and does not change view direction, target, up orientation or fit.

The Cube itself remains an orientation indicator/manipulator and does not need a perspective-distorted visual representation.

### No Cube drag-orbit

Dragging the Cube itself to perform free camera orbit is deliberately not supported.

Free orbit remains available through the normal viewport navigation grammar.

## Provider-neutral contract

The common Viewer layer may define finite semantic types for:

- Cube visibility;
- current camera-derived Cube orientation;
- face identifiers;
- edge identifiers;
- corner identifiers;
- adjacent-view 90-degree actions;
- roll +90 / -90 actions;
- Home;
- neutral activation callbacks;
- animation target semantics.

The public contract must stay finite and semantic. It must not become a generic arbitrary-overlay/widget framework.

## Provider ownership

The Qt/OCCT provider owns:

- in-surface visual rendering of Cube geometry;
- face labels;
- edge/corner/face highlight presentation;
- navigation-arrow presentation;
- pixel geometry, anchoring and DPI behavior;
- provider-local hit testing;
- provider-local animation-frame rendering;
- native redraw/z-order integration.

The provider does **not** own:

- Part or Sketch domain state;
- CAD commands/history;
- persistence;
- durable identity;
- application document lifecycle.

## Workbench ownership

The Workbench/Application side remains responsible for document/view navigation intent and existing camera semantics.

Provider hit testing emits only neutral Navigation Cube actions.

There must be exactly one visible Navigation Cube authority. The old QWidget Cube must not remain visible alongside the in-surface Cube after migration.

## Non-goals

This ADR does not authorize:

- durable Navigation Cube state;
- CAD model mutation;
- Qt/OCCT types in public Viewer contracts;
- a general arbitrary-widget overlay framework;
- moving Properties/Operations/Command Line into the Viewer;
- changing Sketch selection semantics;
- changing camera persistence policy;
- free drag-orbit on the Cube;
- user-defined Home orientation;
- configurable Cube skins/themes beyond what is needed for legibility;
- navigation compass/ring features beyond the explicitly listed 90-degree arrows.

## Consequences

The ViewCube visual surface stops depending on QWidget-over-`WA_PaintOnScreen` composition.

A bounded provider-neutral Navigation Cube contract becomes part of the common Viewer API, so this is a D2 public boundary change.

The Cube becomes functionally complete enough to replace the old static UI rather than merely reproducing it.

The existing QWidget ViewCube implementation may be used as donor/reference during migration, but the completed implementation must remove it as a competing visible authority.

## Alternatives rejected

### Keep adding repaint/invalidation hooks

Rejected because manual WB-01B evidence shows the defect persists despite explicit underlay refresh and native-window composition variants.

### Native QWidget child/sibling composition

Rejected by manual Windows verification under WB-01B.

### Reproduce only the old static Cube inside OCCT

Rejected because the current static Cube does not represent full camera orientation and cannot expose all six faces, twelve edges and eight corners in the intended CAD navigation workflow.

### Move all Workbench overlays into the provider

Rejected as speculative and too broad. Only the demonstrated native-surface Navigation Cube responsibility is added.

## Verification

An implementing Work Contract must prove:

- Cube orientation follows ordinary camera orbit continuously;
- all six labeled faces map to exact canonical orthographic views;
- all 12 edges map to exact canonical two-axis views;
- all 8 corners map to exact canonical isometric views;
- opposite/back-side orientations are reachable;
- adjacent-view arrows produce exact 90-degree camera changes;
- roll arrows produce exact ±90-degree roll while preserving viewing direction;
- Home produces exact Top-Front-Right ISO + Fit All while preserving the current projection;
- every Cube orientation action preserves the current Orthographic/Perspective mode;
- the adjacent `ORTHO/PERSP` control toggles only projection and preserves camera orientation;
- all Cube navigation transitions animate and land on deterministic exact final camera states;
- Cube navigation creates no CAD authored state, dirty state or Undo history;
- ordinary Pan/Orbit/Zoom remains functional;
- pointer→ray mapping and Sketch selection remain correct;
- correct DPI/resize anchoring;
- no stale Cube artifacts during resize, navigation or animation;
- exact-head FULL CI;
- explicit manual Windows verification of the final visual behavior.

## Documentation impact

Internal docs: required  
User/Product docs: required in PL and EN  
Reason: this changes both Viewer/UI ownership and the visible navigation workflow.

## Completion boundary

Acceptance of this ADR authorizes a bounded WB-01B implementation amendment for the provider-surface 3D Navigation Cube described above.

It does not activate R5.


## Accepted amendment — independent ORTHO/PERSP projection

**Owner acceptance:** 2026-09-25

The Owner clarified and accepted that projection is independent from Navigation Cube orientation.

This amendment supersedes the earlier Orthographic-only wording in this ADR:

- Navigation Cube orientation actions preserve the current camera projection;
- `Home` remains Top-Front-Right ISO + Fit All and preserves projection;
- a separate provider-surface `ORTHO/PERSP` control sits next to the Cube;
- that control changes only the projection mode;
- the Cube visual itself remains the same orientation cube in both projection modes.

No other ADR-0010 behavior is changed.
