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
- authored per-entity `Regular` / `Construction` role;
- the value-semantic mixed-primitive `SketchModel` plus validated state/restore transfer;
- provider-neutral Line/Circle/Arc curve-relation and bounded-region analysis, point-in-region picking, exact region composition and nested-island discovery;
- provider-neutral read-only whole-entity measurement for Line/Circle/Arc;
- host-neutral runtime `SketchInteractionState` for Select/Line/Circle/Arc/Rectangle creation, semantic selection, hover/grips, bounded direct manipulation and common Move/Copy/Rotate/Scale/Mirror command stages;
- a provider-independent mixed-primitive transform core for translation, rotation, positive uniform scale and reflection.

Each persistent Part-hosted Sketch embeds one `SketchModel` by value. That host integration does not reverse the dependency: `simplesolid2_sketch` still has no dependency on Part, Application/DocumentSession, Persistence, Viewer, Qt, OCCT or filesystem paths.

Normal COPY is implemented above this neutral transform layer: its preview reuses translation geometry, while the owning application/Part command path performs semantic duplication and fresh identity allocation.

<!-- section-id: internal.shared-2d.coordinates -->
## Sketch-local coordinates

`Point2` stores `u` and `v` as finite real values in Sketch-local coordinates.

These values represent physical model-length coordinates in the host Sketch frame. Shared 2D does not store the Part/3D placement, camera transform, screen coordinates, snap tolerance or display units.

The host-specific mapping from local U/V into 3D remains outside Shared 2D. The Part/UI adapter performs that mapping through `SketchPlacement`; the neutral Shared 2D model remains unaware of Viewer, camera and provider state.

<!-- section-id: internal.shared-2d.identity -->
## Entity identity

`EntityId` is an opaque stable identity scoped to one `SketchModel`.

The default-constructed `EntityId` is invalid. Valid IDs are allocated by `SketchModel`.

Canonical unsigned-decimal transport is provided by `EntityId::parse/serialized` and `EntityIdCursor::parse/serialized`. Leading-zero, zero, non-decimal and overflow values fail closed.

Allocation is monotonic. `EntityIdCursor` records the next allocatable identity high-water. Erasing an entity never lowers it. `SketchModel::state/restore` validates uniqueness, geometry and the invariant that every stored EntityId is lower than the cursor.

Ordinary value-copy and Undo/Redo state replication preserve existing EntityIds. Semantic duplication is different: every accepted COPY placement creates fresh EntityIds for its duplicated Line/Circle/Arc entities.

`DocumentSession` preserves the highest observed cursor for each Sketch across history movement. Undo of a COPY placement removes the copied entities but does not make their committed IDs allocatable again. Redo restores the same copied IDs. A new COPY after Undo allocates above the preserved high-water rather than aliasing the abandoned IDs.

The session-local high-water cursor survives Undo even when authored geometry returns exactly to the last saved state. Existing semantic dirty-state behavior is preserved: Undo back to the saved authored state remains clean. A new COPY in that continuing session still allocates above the preserved high-water, and saving the later committed authored state persists the resulting `next_entity_id` under schema v4.

Schema-v4 persistence stores `next_entity_id`, preserving the model-local EntityId high-water across Save/reopen without an additional persistence concept for Copy.

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
setEntityRole(EntityId, EntityRole)
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

Rectangle uses **First Corner → Opposite Corner** in Sketch-local U/V. `RectangleIntent` derives the exact perimeter `A→B`, `B→C`, `C→D`, `D→A` with `B=(u1,v0)` and `D=(u0,v1)`. Zero U or V extent is rejected exactly, with no Product epsilon. Rectangle remains active after a successful commit so another rectangle can start from a fresh First Corner.

Rectangle is creation grammar, not a durable primitive. The neutral interaction state owns only the two-corner stage and exact derived geometry. The Part/UI adapter owns two runtime edit-session options: **Creation Role** (`Regular | Construction`) for future Line/Circle/Arc and Rectangle perimeter commits, and Rectangle **Draw Diagonals**. Draw Diagonals adds the exact `A→C` and `B→D` preview/commit lines as `Construction` regardless of perimeter Creation Role. Neither option is persisted or creates revision/dirty/Undo state by itself.

At First Corner the Part adapter captures the current `DocumentRevision`. Final Rectangle commit uses that captured revision through one atomic application command. A normal Rectangle creates four fresh ordinary Line entities; Draw Diagonals creates six, with the last two Construction. All entities commit together in one transaction, one revision increment and one Undo entry. Stale revision, invalid geometry or transaction failure creates none. Undo removes the whole logical creation and Redo restores the same committed EntityIds. No RectangleId, center point, endpoint-sharing identity, group membership or hidden constraint is created.

The 3-Point Arc circumcenter is evaluated from `Through - Start` and `End - Start` in a translation-local frame. Those local vectors are normalized by one finite common scale before determinant and squared-norm arithmetic, then the center offset is restored to Sketch coordinates. This removes the prior dependence on absolute-coordinate squares and protects representable very small/large constructions from avoidable intermediate underflow/overflow.

This is numerical conditioning, not a Product geometric tolerance. E1 does not introduce an epsilon for coincidence or collinearity, a minimum feature size or a maximum coordinate/radius policy. Exact duplicate points and a zero normalized cross product still fail closed; non-finite or unrepresentable intermediate/output values also fail closed. Near-collinear but representable input is not rejected by a new fixed cutoff.

The same state owns transient selection as `EntityId` values plus optional primary identity. Point selection is additive, Ctrl toggles membership, Window/Crossing adds or toggles, and provider result order never chooses primary.

Move, Copy, Rotate, Scale and Mirror reuse one common transform lifecycle. Selection-first activation immediately freezes an existing non-empty semantic selection. Command-first activation starts with Select objects; click/Ctrl/Window/Crossing reuse the existing semantic query bridge, blank LMB is a no-op, and Enter/Space/RMB completes object collection only when non-empty. After collection, the frozen semantic set cannot change until the operation completes or is cancelled.

Move accepts a finite Base Point and previews one translation delta `destination - base`.

COPY uses the same Base Point + translation preview but has duplication semantics at commit. Its complete original source selection and authored geometry snapshot remain frozen for the whole COPY session. An accepted non-zero placement creates fresh semantic entities while leaving the source unchanged and selected. The created copies do not take over selection. After a successful placement, COPY remains at the placement stage with the same Base Point/source snapshot, so every repeated placement derives from the original source rather than the previous copy.

Exact zero-displacement COPY is deliberately not a placement: it creates no entities, consumes no IDs, changes no revision/dirty/history state and leaves COPY active awaiting another placement.

Rotate accepts Base Point → Reference Point → destination. The reference vector must be non-zero. Rotation is the signed angle from `reference - base` to `destination - base` in the Sketch frame, with positive counter-clockwise rotation.

Scale accepts Base Point → Reference Point → destination. The reference distance must be non-zero and the runtime factor is:

```text
factor = |destination - base| / |reference - base|
```

Only finite factors strictly greater than zero are valid. Factors between 0 and 1 reduce geometry, factor 1 is a no-op, and negative scale is not represented by Scale.

Mirror accepts two distinct finite points defining an infinite axis and previews reflection of the complete frozen selection. Reflection preserves Line/Circle/Arc EntityIds and reverses Arc signed sweep orientation as required by the reflected directed arc.

All common-transform/COPY preview is recomputed from the interaction-start authored geometry snapshot; preview never compounds from a previous frame. Preview is runtime-only and never mutates `SketchModel`, revision, dirty state, identity allocation or Undo history.

Selected editable entities still expose semantic grips:

- Line: Start, Center, End;
- Circle: Center plus four quadrant radius grips;
- Arc: Center, Start, End and Arc/Mid.

Center grips perform the same mixed Line/Circle/Arc semantic translation as normal Move, using the grip's interaction-start point as the implicit base, and remain Move-only.

Line Start/End, Circle quadrant and Arc Start/End/Mid default to owner-only Reshape. During an active direct-manipulation session, viewport Space cycles those non-center grips between `Reshape` and `Move`. The active grip, interaction-start pivot, frozen semantic selection and current resolved pointer are preserved across the cycle. Reshape derives preview from the interaction-start owner geometry; Move derives translation from the interaction-start complete selection geometry. Switching mode therefore recomputes preview from frozen authored inputs rather than compounding a previous preview. Mode cycling is runtime-only and creates no authored mutation, revision, dirty state or history entry.

SK-07G adds one runtime-only **Grip Copy** modifier orthogonal to `DirectEditMode`. While direct manipulation is active, the tool-local token `C` submitted through the existing Sketch semantic CAD-input endpoint enables Copy without changing authored state, identity, revision or history. `C` is not a top-level command alias and the workspace-global input router remains domain-neutral.

With Grip Copy enabled, Move duplicates the complete frozen selection while Reshape duplicates only the entity that owns the active grip. Originals remain unchanged and selected/reference entities. Every accepted placement uses the existing semantic duplication command and SK-07C fresh-EntityId/high-water lifecycle. Repeated placements remain in the same direct-manipulation session and are always recomputed from the interaction-start source geometry and pivot. A successful placement clears the prior pointer candidate so a later Direct Distance requires a fresh direction. Space changing Reshape↔Move turns Grip Copy OFF; Esc/tool/history/context termination also ends the modifier with the owning manipulation session. Construction/Regular role remains part of the frozen owner geometry so exact no-change detection is role-correct.

Pointer and text-derived point values flow through the shared `ResolvedSketchInput` seam. A semantic `PointRequest` view is derived from the existing interaction stage rather than creating a second tool state machine. The request exposes an optional semantic base, one shared runtime pointer candidate and whether Direct Distance is legal at that stage.

The current Direct Distance resolver is deliberately minimal:

```text
direction = normalize(pointer_candidate - base)
resolved_point = base + direction * distance
```

The scalar must be finite and non-negative and the pointer candidate must define a non-zero direction. The resolver does not know Line, Move, Copy or primitive-specific reshape geometry; it only returns one resolved Sketch-local point. The active operation then consumes that point through the same preview/accept/commit path as pointer input.

Direct Distance is currently enabled only for:

- the second/next Line point after an accepted Line anchor;
- active grip Reshape;
- active grip Move, including a non-center grip after Space switches Reshape → Move;
- normal MOVE destination after Base Point;
- normal COPY placement after Base Point.

Rectangle PointRequests deliberately keep Direct Distance disabled in R9. A bare scalar at First Corner or Opposite Corner is rejected rather than guessed as width, height, diagonal, square side or a U/V coordinate. Rectangle precision/coordinate grammar remains for the later precision-input milestone.

Command submission is context-first. While a semantic PointRequest is active, the Part/Sketch input endpoint offers submitted text to that request before top-level command activation. During active direct manipulation that semantic layer additionally recognizes the bounded tool-local token `C` for Grip Copy before attempting bare-distance parsing. Outside active direct manipulation, `C` remains an unknown top-level Sketch command. The workspace-global text transport and live buffer do not know `PointRequest`, Grip Copy, Line, Move or Copy. The current Part/Sketch adapter accepts a bare finite non-negative scalar using `.` or the current UI-locale decimal separator and rejects grouping separators, units, coordinate tuples, polar syntax and exponent notation rather than guessing. Semantic Sketch code receives only typed/parsed semantic input and remains Qt/locale independent.

Printable text typed while the normal CAD viewport has focus now reaches the same global buffer as directly editing Command Line; no focus transfer is required. Real text editors retain their keyboard ownership. This changes only the input adapter path, not Direct Distance semantics or mutation authority.

After a Line segment, normal COPY placement or Grip Copy placement completes, stale pointer direction is not reused silently. The continuous Line anchor becomes the new base with no non-zero direction; repeated normal COPY and repeated Grip Copy clear their pointer candidate, so the pointer must establish a new direction before another numeric Direct Distance can resolve.

Accepted non-no-op edit transforms/reshape commit through the host semantic geometry-update command and Part transaction and preserve EntityIds. Each accepted normal COPY or Grip Copy placement instead executes the existing atomic semantic duplication command, allocates a fresh ID for each copied entity and creates one revision/Undo entry. Multiple repeated placements are independent Undo steps.

Esc cancels transient state and preserves the affected/source selection; already committed repeated copies remain. Undo/Redo cancels an active transient common transform/COPY/direct manipulation, including Grip Copy, before global history. PointRequest, pointer candidate and numeric resolution state are runtime-only and are cleared with their owning stage/se<!-- section-id: internal.shared-2d.measurement -->
## Read-only whole-entity measurement

R8A adds provider-neutral measurement in `measurement.hpp`. Measurement consumes the authoritative semantic `SketchModel` geometry by `EntityId`; it does not consume Viewer tessellation, presentation tokens, pixels or sampled display chords.

The current result is a value-semantic `EntityMeasurement` variant with one of:

- `LineMeasurement` — EntityId, Regular/Construction role, Length, DeltaU, DeltaV and directed angle from Sketch +U for Start → End;
- `CircleMeasurement` — EntityId, role, Radius, Diameter, Circumference and Area;
- `ArcMeasurement` — EntityId, role, Radius, canonical Start Angle, derived End Angle, signed Sweep Angle and positive Arc Length.

Line length uses `hypot(DeltaU, DeltaV)` and angle uses `atan2(DeltaV, DeltaU)`. Circle circumference/area use the semantic radius. Arc end is `start + signed sweep`; arc length is `radius * abs(sweep)`. No Viewer approximation participates in those values.

`measureEntity(model, id)` is read-only. Invalid/missing identity or a non-finite derived value returns no result rather than stale/partial data. It does not mutate `SketchModel`, allocate EntityIds, create history or introduce a Product tolerance.

Regular and Construction geometry use the same formulas. Construction remains excluded from material-region formation but is fully available for engineering inspection.

R8A deliberately measures whole entities only. No durable `SubElementId`, midpoint/quadrant identity, authored dimension, relation or solver state is introduced. The UI may present angular values in degrees, while the neutral semantic result remains radians. Linear/area results remain in the current Sketch coordinate scale; R8A does not define document-unit labels or unit conversion.

<!-- section-id: internal.shared-2d.regions -->
## Construction and region analysis

Every authored Line/Circle/Arc has an `EntityRole`. `Regular` geometry participates in region topology. `Construction` geometry remains authored, selectable and editable, but it is excluded from bounded-region formation and therefore cannot close or split a Profile region.

The neutral region layer operates on current authored/evaluated 2D geometry, never on Viewer tessellation or provider topology. It provides canonical Line/Circle/Arc pair relations, deterministic fragmentation at accepted intersections, bounded loop/holes construction, point-in-region picking, exact connected cell composition for transient Add/Subtract, and nested-island discovery.

Analysis returns bounded `RegionCandidate2D` values plus structured diagnostics. Open connected geometry with no bounded loop reports `open_boundary`; coincident/overlap ambiguity and invalid topology fail closed for the affected connected component while unrelated valid components remain usable.

There is no Product gap tolerance or auto-close policy. A small geometric gap remains a gap. Tangency and point-only contact do not silently merge disconnected material, and a point on a region boundary is never assigned arbitrarily to one adjacent candidate.

All derived region candidates, relation intersections, sampled interior points and analysis caches are runtime-only. Durable Profile identity is owned by Part through semantic RegionIntent references to source Sketch EntityIds and anchors.

ssion.

Numeric angles, scale factors, absolute/relative coordinates, polar coordinates, unit expressions and Dynamic Input are still not implemented. Direct Distance is the only numeric precision-input capability in the current interaction state.

<!-- section-id: internal.shared-2d.boundaries -->
## Deliberately not implemented yet

The current Part integration presents and edits authored Line/Circle/Arc through runtime adapters outside the Shared 2D target. Active authored geometry is mapped from Sketch U/V to the Part support frame, while the intrinsic Sketch Origin remains a runtime overlay.

The current normal Modify command set is Move, Copy, Rotate, positive uniform Scale and Mirror. COPY currently means translation duplication with repeated placements and fresh identities.

The product still does not implement:

- Copy modifier combined with Rotate/Scale/Mirror or other future edit modes beyond the implemented grip Reshape/Move paths;
- ordinary-Select RMB context;
- clipboard Copy/Paste or cross-Sketch/cross-Document duplication;
- intrinsic Origin snapping;
- snapping/Object Snap, tracking, Ortho/Polar/Grid Snap or geometric inference;
- numeric Rotate/Scale values, absolute/relative coordinate entry, polar syntax, unit expressions or Dynamic Input;
- authored dimensions, constraints or solver evaluation;
- a durable Rectangle primitive/group/center/constraint model or Polyline semantics;
- projected/reference geometry;
- planar-face Sketch support.

Those capabilities remain governed by later accepted Work Contracts.

<!-- section-id: internal.shared-2d.tests -->
## Verification

The current Sketch regression set covers Shared 2D dependency boundaries, mixed primitive semantics, Rectangle two-corner/decomposition state, atomic four/six-Line Rectangle creation, Creation Role and Construction-diagonal behavior, Profile exclusion of Construction geometry, read-only whole-entity measurement, Part hosting, schema-v6 persistence/history, provider-neutral presentation/input, selection, hover/grips, direct manipulation, common transforms, normal/Grip Copy identity behavior, Repeat Last Command, Space CycleEditMode and precision input.

Key registered tests include:

- `sk02a.shared_2d_core` / `sk02a.shared_2d_boundaries` — neutral model/dependency boundaries plus whole-entity measurement, curve relations, regions, Construction exclusion, open/overlap diagnostics, holes/islands, point picking and region composition;
- `sk06a.circle_arc_model_persistence` / `sk06a.circle_arc_interaction_state` — mixed Line/Circle/Arc authored and interaction semantics;
- `e1.arc_numerical_stability` — 3-Point Arc translation/scale conditioning, radial residual through all requested points, CW/CCW/long branch preservation, ±1e6 translation, 1e-200/1e200 scale, near-collinear acceptance and fail-closed exact/invalid extremes;
- `sk07a.transform_core`, `sk07b.transform_core`, `sk07b.common_transform_state`, `sk07b.transform_controller` — mixed transforms and atomic controller behavior;
- `sk07c.copy_command`, `sk07c.copy_interaction_state`, `sk07c.copy_controller` — fresh identity, repeated placement, Undo/Redo identity restoration and high-water persistence;
- `sk07d.repeat_last_command_controller` and `sk07e.space_cycle_edit_mode` — runtime command/grip grammar;
- `sk07f.precision_input_state` / `sk07f.precision_input_controller` — PointRequest and Direct Distance across Line, Move, Copy and grip paths;
- `wb02.cad_input_session`, `wb02.cad_input_boundaries`, `wb02.global_cad_input_ui` plus the Workbench Sketch-host regression — keyboard-first transport, focus arbitration, stale-context rejection and real Workbench integration.

Work-item completion uses the repository's exact-head Windows gate; current-state documentation does not preserve obsolete milestone gate counts.

