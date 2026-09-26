# Shared 2D Authoring — As-built

<!-- doc-id: internal.shared-2d -->
<!-- document-kind: internal -->

<!-- section-id: internal.shared-2d.scope -->
## Current scope

The implemented Shared 2D / Sketch Core is the neutral `simplesolid2_sketch` target.

Its normative architecture is defined by ADR-0008 and ADR-0009. This document describes only the current implementation.

The target currently contains:

- durable host-level `SketchId`;
- Sketch-local `Point2` coordinate values;
- opaque model-local `EntityId`;
- canonical decimal identity transport for `EntityId` and `EntityIdCursor`;
- canonical authored primitives `Line`, `Circle` and `Arc`;
- the value-semantic mixed-primitive `SketchModel` plus validated state/restore transfer;
- host-neutral runtime `SketchInteractionState` for Select/Line/Circle/Arc creation, semantic selection, hover/grips, bounded direct manipulation and normal MOVE command stages;
- a provider-independent mixed-primitive translation core shared by Center-grip Move and normal MOVE.

Each persistent Part-hosted Sketch embeds one `SketchModel` by value. That host integration does not reverse the dependency: `simplesolid2_sketch` still has no dependency on Part, Application/DocumentSession, Persistence, Viewer, Qt, OCCT or filesystem paths.

<!-- section-id: internal.shared-2d.coordinates -->
## Sketch-local coordinates

`Point2` stores `u` and `v` as finite real values in Sketch-local coordinates.

These values represent physical model-length coordinates in the host Sketch frame. Shared 2D does not store the Part/3D placement, camera transform, screen coordinates, snap tolerance or display units.

The host-specific mapping from local U/V into 3D remains outside Shared 2D. SK-03A implements that mapping in the Part/UI adapter using `SketchPlacement`; the neutral Shared 2D model remains unaware of Viewer, camera and provider state.

<!-- section-id: internal.shared-2d.identity -->
## Entity identity

`EntityId` is an opaque stable identity scoped to one `SketchModel`.

The default-constructed `EntityId` is invalid. Valid IDs are allocated by `SketchModel`.

R2 adds canonical unsigned-decimal transport without exposing numeric arithmetic as CAD semantics: `EntityId::parse/serialized` and `EntityIdCursor::parse/serialized` accept canonical positive decimal strings only. Leading-zero, zero, non-decimal and overflow values fail closed.

Allocation is monotonic. `EntityIdCursor` records the next allocatable identity high-water. Erasing an entity never lowers it. `SketchModel::state/restore` validates uniqueness, geometry and the invariant that every stored EntityId is lower than the cursor.

Ordinary value-copy preserves EntityIds and cursor state while copying authored storage by value. Mutating one copied state therefore does not alias another copied state.

Semantic `SketchModel` equality intentionally compares authored entity content rather than the technical cursor. This allows Undo to return to a saved authored state without becoming falsely dirty solely because the live identity lineage has advanced.

<!-- section-id: internal.shared-2d.line -->
## Authored primitives

A `Line` owns:

```text
EntityId
Start(U,V)
End(U,V)
```

Start and End are authored values owned by that Line. Exact equal Start/End is rejected; finite non-zero Lines remain valid without an epsilon-length policy.

A `Circle` owns:

```text
EntityId
Center(U,V)
Radius
```

The center must be finite and radius must be finite and strictly greater than zero. Creation-method metadata is not authored.

An `Arc` owns:

```text
EntityId
Center(U,V)
Radius
StartAngle
SignedSweepAngle
```

Arc radius must be finite and strictly positive. Start angle and signed sweep must be finite, sweep must be non-zero and its magnitude must be strictly less than one full turn. Zero angle is +U and positive sweep is counter-clockwise in the Sketch frame; the sign and magnitude preserve CW/CCW plus short/long meaning.

Equal coordinates or equal canonical parameters do not imply shared identity. All three primitive kinds draw from one model-local EntityId namespace.

<!-- section-id: internal.shared-2d.model -->
## SketchModel operations

The current `SketchModel` owns typed Line/Circle/Arc collections behind one semantic entity namespace. Its authored lifecycle includes:

```text
addLine / findLine / updateLine
addCircle / findCircle / updateCircle
addArc / findArc / updateArc
erase(EntityId)
contains(EntityId)
entityCount()
entityIdCursor()
preserveEntityIdCursor(cursor)
state()
restore(state)
```

Add/update operations validate canonical primitive geometry before mutation. Update preserves the addressed EntityId. `erase` removes whichever supported primitive owns the EntityId and returns false for an invalid or unknown ID.

`EntityIdCursor` is shared across primitive kinds. Allocation is monotonic, erase never lowers the high-water cursor, and strict restore rejects duplicate IDs across any primitive collections, invalid geometry, or stored IDs that are not below the cursor.

Application-facing creation, delete and direct manipulation do not mutate the live `SketchModel` directly. They stage semantic command results through `DocumentSession` and the owning Part transaction/history path.

<!-- section-id: internal.shared-2d.interaction -->
## Sketch interaction state

One host-neutral `SketchInteractionState` remains the runtime interaction authority. It is not authored Sketch state and is never persisted.

The default tool is Select. Creation tools preserve the existing semantic selection while grips are hidden/inactive, and newly created geometry is not auto-selected.

Line keeps the continuous Start/Next-point grammar. Circle uses Center → Radius; exact zero radius produces no commit request. Arc uses Start → Through → End; duplicate accepted points, collinear triples or any invalid circumcircle/sweep fail closed.

The same state owns transient selection as `EntityId` values plus optional primary identity. Point selection is additive, Ctrl toggles membership, Window/Crossing adds or toggles, and provider result order never chooses primary.

SK-07A adds normal `MOVE` without introducing another selection owner. Selection-first MOVE immediately freezes the existing non-empty selection and asks for a Base Point. Command-first MOVE starts with Select objects; click/Ctrl/Window/Crossing reuse the existing semantic query bridge, blank LMB is a no-op, and Enter/Space/RMB completes object collection only when non-empty. After collection, the frozen semantic set cannot change until the command completes or is cancelled.

Normal MOVE stores a resolved Sketch-local Base Point and previews one translation delta `destination - base` from the interaction-start authored snapshot. Preview never compounds from a prior frame. Zero delta is a valid no-op completion.

Selected editable entities still expose semantic grips:

- Line: Start, Center, End;
- Circle: Center plus four quadrant radius grips;
- Arc: Center, Start, End and Arc/Mid.

Center grips perform the same mixed Line/Circle/Arc semantic translation as normal MOVE, using the grip's interaction-start point as the implicit base. Line Start/End, Circle quadrant and Arc Start/End/Mid remain owner-only Reshape.

Pointer values flow through the shared `ResolvedSketchInput` seam. Preview is runtime-only: it never mutates `SketchModel`, revision, dirty state, identity allocation or Undo history. Accepted non-zero MOVE/reshape commits through the host semantic geometry-update command and Part transaction. Esc cancels transient state and preserves the affected selection. Undo/Redo cancels any active transient MOVE/direct manipulation before global history.

<!-- section-id: internal.shared-2d.boundaries -->
## Deliberately not implemented yet

The current Part integration presents and edits authored Line/Circle/Arc through runtime adapters outside the Shared 2D target. Active authored geometry is mapped from Sketch U/V to the Part support frame, while the intrinsic Sketch Origin remains a runtime overlay.

R7 has started only through SK-07A. Normal MOVE and the shared translation core are implemented; the product still does not implement:

- Rotate, uniform Scale, Mirror or Copy/repeated Copy;
- grip Copy modifier, semantic Space CycleEditMode, ordinary-Select RMB context or Repeat Last Command;
- intrinsic Origin snapping;
- snapping/Object Snap, tracking, Ortho/Polar/Grid Snap or geometric inference;
- numeric coordinate/dynamic input;
- authored dimensions, constraints or solver evaluation;
- Rectangle/Polyline durable semantics;
- intersections, profiles/regions or projected/reference geometry;
- planar-face Sketch support.

Those capabilities remain governed by later accepted Work Contracts.

<!-- section-id: internal.shared-2d.tests -->
## Verification

Existing SK-02A through SK-06A tests continue to cover Shared 2D dependency boundaries, primitive semantics, Part hosting, persistence/history, provider-neutral presentation/input, selection, hover/grips and direct manipulation.

SK-07A adds `sk07a.transform_core` and `sk07a.move_controller`. They cover mixed Line/Circle/Arc translation, finite validation, EntityId preservation, parity between Center-grip Move and normal MOVE, selection-first and command-first staging, frozen selection, Base Point/destination preview, atomic commit, zero-delta no-op, stale revision failure, history cancellation and schema-v4 Save/reload identity preservation.

The existing Workbench Sketch-host regression also verifies toolbar/Command-Line MOVE activation and that Space in text-entry focus remains text input.

The exact runtime implementation head `bb218f8d9cff1c1c2ffc45e091a69c485190e97e` passed Windows FULL #475 with Build and 60/60 unfiltered CTest tests. FAST contained 59 tests; the native Workbench stress test remained FULL-only.
