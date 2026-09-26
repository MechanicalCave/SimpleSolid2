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
- host-neutral runtime `SketchInteractionState` for Select/Line/Circle/Arc creation, semantic selection, hover/grips, bounded direct manipulation and common Move/Rotate/Scale/Mirror command stages;
- a provider-independent mixed-primitive transform core for translation, rotation, positive uniform scale and reflection.

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

Move, Rotate, Scale and Mirror reuse one common transform lifecycle. Selection-first activation immediately freezes an existing non-empty semantic selection. Command-first activation starts with Select objects; click/Ctrl/Window/Crossing reuse the existing semantic query bridge, blank LMB is a no-op, and Enter/Space/RMB completes object collection only when non-empty. After collection, the frozen semantic set cannot change until the transform completes or is cancelled.

Move accepts a finite Base Point and previews one translation delta `destination - base`.

Rotate accepts Base Point → Reference Point → destination. The reference vector must be non-zero. Rotation is the signed angle from `reference - base` to `destination - base` in the Sketch frame, with positive counter-clockwise rotation.

Scale accepts Base Point → Reference Point → destination. The reference distance must be non-zero and the runtime factor is:

```text
factor = |destination - base| / |reference - base|
```

Only finite factors strictly greater than zero are valid. Factors between 0 and 1 reduce geometry, factor 1 is a no-op, and negative scale is not represented by Scale.

Mirror accepts two distinct finite points defining an infinite axis and previews reflection of the complete frozen selection. Reflection preserves Line/Circle/Arc EntityIds and reverses Arc signed sweep orientation as required by the reflected directed arc.

All common-transform preview is recomputed from the interaction-start authored geometry snapshot; preview never compounds from a previous frame. Preview is runtime-only and never mutates `SketchModel`, revision, dirty state, identity allocation or Undo history.

Selected editable entities still expose semantic grips:

- Line: Start, Center, End;
- Circle: Center plus four quadrant radius grips;
- Arc: Center, Start, End and Arc/Mid.

Center grips perform the same mixed Line/Circle/Arc semantic translation as normal Move, using the grip's interaction-start point as the implicit base. Line Start/End, Circle quadrant and Arc Start/End/Mid remain owner-only Reshape.

Pointer values flow through the shared `ResolvedSketchInput` seam. Accepted non-no-op transforms/reshape commit through the host semantic geometry-update command and Part transaction. One accepted common transform is atomic, preserves existing EntityIds and creates at most one revision/history entry. Exact semantic no-ops create no revision, dirty-state or Undo change. Esc cancels transient state and preserves the affected selection. Undo/Redo cancels an active transient common transform/direct manipulation before global history.

Numeric angles, distances and scale factors are not parsed by the current interaction state. Number keys do not override the pointer-derived preview; Enter at a final transform stage commits the current valid preview.

<!-- section-id: internal.shared-2d.boundaries -->
## Deliberately not implemented yet

The current Part integration presents and edits authored Line/Circle/Arc through runtime adapters outside the Shared 2D target. Active authored geometry is mapped from Sketch U/V to the Part support frame, while the intrinsic Sketch Origin remains a runtime overlay.

The current common transform set is Move, Rotate, positive uniform Scale and Mirror. The product still does not implement:

- Copy or repeated Copy, including transform-created fresh EntityIds;
- grip Copy modifier, semantic Space CycleEditMode, ordinary-Select RMB context or Repeat Last Command;
- intrinsic Origin snapping;
- snapping/Object Snap, tracking, Ortho/Polar/Grid Snap or geometric inference;
- numeric angle/distance/scale input, coordinate input or Dynamic Input;
- authored dimensions, constraints or solver evaluation;
- Rectangle/Polyline durable semantics;
- intersections, profiles/regions or projected/reference geometry;
- planar-face Sketch support.

Those capabilities remain governed by later accepted Work Contracts.

<!-- section-id: internal.shared-2d.tests -->
## Verification

Existing Sketch regressions continue to cover Shared 2D dependency boundaries, primitive semantics, Part hosting, persistence/history, provider-neutral presentation/input, selection, hover/grips and direct manipulation.

The common-transform regression set includes `sk07b.transform_core`, `sk07b.common_transform_state` and `sk07b.transform_controller`. Together they cover mixed Line/Circle/Arc Rotate/Scale/Mirror geometry, positive-only Scale validation, Arc mirror orientation, EntityId preservation, selection-first and command-first staging, frozen selection, reference-point and preview behavior, exact no-op handling, atomic commit, stale-revision failure, cancellation/history interaction and schema-v4 Save/reload identity preservation.

The Workbench Sketch-host regression also verifies the grouped Select / Create / Modify surface, toolbar/Command-Line transform activation and text-focus Space behavior. Final work-item completion additionally requires the repository's exact-head Windows FULL gate and Owner manual Windows verification.

