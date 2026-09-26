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
- host-neutral runtime `SketchInteractionState` for Select/Line/Circle/Arc creation, semantic selection, hover/grips and bounded direct manipulation.

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

Line keeps the continuous Start/Next-point grammar. Circle uses Center → Radius; exact zero radius produces no commit request. Arc uses Start → Through → End; duplicate accepted points, collinear triples or any invalid circumcircle/sweep fail closed. Circle and Arc remain active after a successful commit awaiting the next primitive until Esc, Select or another tool is chosen.

The same state owns transient selection as `EntityId` values plus optional primary identity. Point selection is additive, Ctrl toggles membership, Window/Crossing adds (or toggles with Ctrl), provider result order does not choose primary, and blank LMB / Select-mode Esc follow the established clear hierarchy across Line/Circle/Arc.

Selected editable entities expose semantic grips:

- Line: Start, Center, End;
- Circle: Center plus four quadrant radius grips;
- Arc: Center, Start, End and Arc/Mid.

Grip references contain semantic `EntityId + SketchGripRole`; Viewer presentation tokens remain outside Shared 2D.

A `DirectManipulationSession` freezes the current selection and interaction-start geometry. Center grips perform a common Move of the complete frozen mixed selection. Line Start/End, Circle quadrant, and Arc Start/End/Mid perform owner-only Reshape. Circle radius reshape preserves center; Arc Mid changes radius only; Arc Start/End preserve center/radius and the required fixed endpoint/branch semantics, failing closed on ambiguous invalid boundaries.

Pointer values flow through the shared `ResolvedSketchInput` seam. Preview geometry is transient only: it never mutates `SketchModel`, revision, dirty state, identity allocation or Undo history. LMB/Enter commit through the host semantic command; Esc cancels the manipulation and preserves selection. Selection mutation is rejected while manipulation is active.

<!-- section-id: internal.shared-2d.boundaries -->
## Deliberately not implemented yet

The current Part integration presents and edits authored Line/Circle/Arc through runtime adapters outside the Shared 2D target. Active authored geometry is mapped from Sketch U/V to the Part support frame, while the intrinsic Sketch Origin remains a runtime overlay.

R6 intentionally stops at primitive breadth and bounded grips/direct manipulation. The product still does not implement:

- intrinsic Origin snapping;
- snapping/Object Snap, tracking, Ortho/Polar/Grid Snap or geometric inference;
- numeric coordinate/dynamic input;
- authored dimensions, constraints or solver evaluation;
- Rectangle/Polyline durable semantics or R7 common Move/Copy/Rotate/Scale/Mirror command grammar;
- intersections, profiles/regions or projected/reference geometry;
- planar-face Sketch support.

Those capabilities remain governed by later roadmap milestones and separate accepted Work Contracts.

<!-- section-id: internal.shared-2d.tests -->
## Verification

Existing SK-02A through SK-05A tests continue to cover Shared 2D dependency boundaries, Line semantics, Part hosting, persistence/history, provider-neutral presentation/input, selection, hover/grips and Line direct manipulation.

SK-06A adds `sk06a.circle_arc_model_persistence` and `sk06a.circle_arc_interaction_state`. They cover Circle/Arc canonical validation, a shared mixed-primitive EntityId cursor, strict state/restore, schema-v4 persistence and malformed-kind rejection, Circle Center+Radius creation state, Arc Start/Through/End short/long and CW/CCW canonicalization, duplicate/collinear failure, mixed frozen-selection Move, owner-only Circle/Arc reshape and transient/history cancellation semantics.

Existing controller/native tests exercise the generalized semantic token bridge, grip scene lifecycle/hit testing and camera navigation. The Qt/OCCT provider now executes those tests with semantic curve presentation plus DPI-aware square grip aspects.

The exact implementation head `518e8f3a5feb56a04d2c2067553d8697e02ceadf` passed Windows FULL gate #440 with 58/58 CTest tests. Owner manual Windows verification remains the final runtime acceptance step before closeout.
