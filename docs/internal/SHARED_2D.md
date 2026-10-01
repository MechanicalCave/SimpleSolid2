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

Line keeps the continuous Start/Next-point grammar. Circle uses **Center → Size**. Size has one runtime Diameter/Radius selector that defaults to Diameter for each Sketch edit; pointer placement still uses the cursor as a circumference point, while typed Diameter/Radius values must be finite and strictly positive. Arc uses **Start → End → Arc Point / Radius**. Start and End define the chord; a third pointer/point token preserves the exact 3-point construction, while typed Radius must satisfy `R >= chord/2` and creates the pointer-side minor/semicircle solution. Major-arc creation remains available through the third Arc Point path. These creation selectors/locks are runtime-only; committed Circle/Arc geometry keeps the canonical center/radius and center/radius/start-angle/signed-sweep representations.

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

Pointer and text-derived point values flow through the shared `ResolvedSketchInput` seam. A semantic `PointRequest` view is derived from the existing interaction stage rather than creating a second tool state machine. R10 expands that same request path instead of adding per-tool parsers: point requests may accept absolute Cartesian `U;V`, relative Cartesian `@dU;dV`, relative polar `@Distance<Angle`, and Direct Distance where the active stage permits it. Shared quantity parsing resolves Length to canonical millimetres, Angle to radians and Scalar as dimensionless finite values before Sketch semantics consume them.

Dynamic Input is an adapter to that same request and the one workspace CAD buffer. Unbased point fields are `U → V`; based point fields are `Distance → Angle → dU → dV`. Request-local locks constrain the existing resolver only and never become authored dimensions or constraints. Enter may combine explicit locks with the remaining pointer/Polar degrees of freedom; a complete explicit point token outranks assistance, and incompatible lock families fail closed.

Direct Distance remains finite and non-negative and uses the current resolved free direction. It is available for the second/next Line point, Arc End, normal Move/Copy destinations and supported grip Reshape/Move requests. Rectangle uses its explicit positive `Width;Height` size grammar instead of guessing a generic scalar; Circle Size uses Diameter/Radius; Rotate uses signed Angle; Scale uses positive Factor; grip Mirror uses Axis Angle.

Command submission remains context-first. While a semantic request is active, the Part/Sketch input endpoint offers submitted text to that request before top-level command activation. During active direct manipulation the same semantic layer additionally recognizes the bounded tool-local token `C` for Grip Copy. Outside active direct manipulation, `C` remains an unknown top-level Sketch command. The workspace-global text transport and live buffer remain domain-neutral; semantic code receives typed/parsed values and stays independent of Qt/locale presentation.

Printable text typed while the normal CAD viewport has focus reaches the same global buffer as directly editing Command Line; no focus transfer is required. Real text editors retain their keyboard ownership. Polar is runtime directional assistance, not authored geometry: captured direction may supply a free direction, while explicit point/Angle locks keep their accepted priority.

After an accepted Line segment, normal COPY placement or Grip Copy placement, stale pointer candidates and request-local locks are cleared as required by the owning request lifecycle. Repeated placements therefore cannot silently reuse an old direction.

Accepted non-no-op edit transforms/reshape commit through the host semantic geometry-update command and Part transaction and preserve EntityIds. Each accepted normal COPY or Grip Copy placement instead executes the existing atomic semantic duplication command, allocates a fresh ID for each copied entity and creates one revision/Undo entry. Multiple repeated placements are independent Undo steps.

Esc cancels transient state and preserves the affected/source selection; already committed repeated copies remain. Undo/Redo cancels an active transient common transform/COPY/direct manipulation, including Grip Copy, before global history. PointRequest, pointer candidate and numeric resolution state are runtime-only and are cleared with their owning stage/session.

<!-- section-id: internal.shared-2d.structural-editing -->
## Structural Trim and Extend

Shared 2D owns the provider-neutral structural-edit evaluator for the current authored `Line`, `Arc` and `Circle` vocabulary. The evaluator consumes semantic `EntityId` values, Sketch-local U/V geometry and explicit finite boundary identities; it has no dependency on Viewer/provider tokens, screen pixels, OSNAP aperture or Part persistence.

The R12 UI adapter supports two equivalent runtime acquisition paths without changing semantic command meaning. Tool-first Trim/Extend enters boundary-selection state, toggles supported finite entities into runtime selection, and snapshots that set only after **Enter** or tool-local **RMB** confirmation; this selection phase creates no authored mutation, revision or Undo entry. If valid geometry is already selected when Trim/Extend starts, that preselection seeds the boundary set immediately and the adapter enters target editing directly. Ordinary Select RMB context remains outside R12.

`Trim` removes exactly one connected target fragment and is accepted only when one authored result remains. Terminal Line and Arc trims update the same primitive and preserve `EntityId` plus Regular/Construction role. A middle Line/Arc trim that would leave two authored pieces is not an R12 operation and fails unchanged. Circle Trim requires at least two distinct exact finite-boundary cut locations. Removing the picked local Circle span authors the connected complement as one Arc, retires the Circle identity and allocates one fresh monotonic non-reused Arc `EntityId` with the source role.

Standard `Extend` supports Line and Arc targets only. The chosen endpoint continues to the nearest exact positive intersection with explicit **finite authored** Line/Arc/Circle boundary geometry. A hit that exists only on a boundary's virtual continuation is ineligible. Line extension preserves the Line identity and opposite endpoint; Arc extension preserves identity, center/radius, role and signed orientation. Circle is not an Extend target.

`Extend Both` is a separate two-Line operation. It may use the two infinite supporting lines only to compute one unique virtual intersection, and is applicable only when both finite Lines need extension to that point. Parallel, coincident, already-containing and one-sided cases fail closed. Both Line endpoint mutations commit atomically and preserve both identities and independent roles.

Coincident/overlapping ambiguity, non-finite geometry, stale identity/revision, unsupported output cardinality and equal-nearest ambiguity fail without authored mutation. The UI's hover target, endpoint choice, preview and boundary selection are runtime-only. Trim previews the fragment to remove; Extend previews the added continuation; Extend Both previews both continuations. These visuals are not selectable CAD identity.

The application/Part path remains authoritative for durable mutation:

```text
UI / Command Line
→ semantic structural request
→ Shared-2D evaluation
→ revision-bound Part command/transaction
→ PartDocument commit
→ derived Profile evaluation
→ presentation
```

A successful single Trim or Extend is one history entry. Extend Both updates both Lines in one transaction and one history entry. Undo/Redo restores the exact committed old/new identities, including the original Circle versus its fresh replacement Arc.

Profile intent is never rebound automatically. Same-kind edits can keep a Profile valid when all referenced identities/anchors still resolve. Circle→Arc replacement leaves the existing ProfileId and RegionIntent unchanged; if that intent referenced the retired Circle, derived evaluation becomes invalid/missing-source. Undo can restore the old Circle and make the unchanged intent valid again.

No structural operation history, picked fragment, preview, virtual guide or genealogy is persisted. Existing Sketch persistence stores only the resulting ordinary geometry, roles, EntityIds and `next_entity_id` high-water; Save/Reopen therefore preserves a Circle→Arc replacement identity without introducing a new authored schema.

<!-- section-id: internal.shared-2d.measurement -->
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

Common Move/Rotate/positive-Scale/Mirror transforms preserve already-existing Line↔Arc endpoint topology without introducing a persistent constraint or Product gap tolerance. The transform snapshot carries runtime-only provenance for source endpoint contacts that are proven by the source geometry, and the transformed explicit Line endpoint is canonicalized to the transformed evaluated Arc endpoint. Region relation analysis gives that exact evaluated endpoint contact precedence over an alternate floating-point image of the same intersection root; any de-duplication bound is dimensionless parameter-space conditioning after exact contact is already established, never a world-space rule for creating contact. Previously disjoint geometry is not welded, so a real authored gap remains open through the same common transforms.

<!-- section-id: internal.shared-2d.boundaries -->
## Deliberately not implemented yet

The current Part integration presents and edits authored Line/Circle/Arc through runtime adapters outside the Shared 2D target. Active authored geometry is mapped from Sketch U/V to the Part support frame, while the intrinsic Sketch Origin remains a runtime overlay.

The current normal Modify command set is Move, Copy, Rotate, positive uniform Scale and Mirror. COPY currently means translation duplication with repeated placements and fresh identities.

There is deliberately no separate Ortho mode in the current Sketcher; Polar is the single directional-attraction mechanism and a 90° Polar step provides orthogonal-only attraction.

The product still does not implement:

- Copy modifier combined with Rotate/Scale/Mirror or other future edit modes beyond the implemented grip Reshape/Move paths;
- ordinary-Select RMB context;
- clipboard Copy/Paste or cross-Sketch/cross-Document duplication;
- intrinsic Origin snapping;
- Object Snap, Object Snap Tracking, Grid Snap or geometric inference;
- authored dimensions, constraints or solver evaluation;
- a durable Rectangle primitive/group/center/constraint model or Polyline semantics;
- projected/reference geometry;
- planar-face Sketch support.

Those capabilities remain governed by later accepted Work Contracts.

<!-- section-id: internal.shared-2d.tests -->
## Verification

The current Sketch regression set covers Shared 2D dependency boundaries, mixed primitive semantics, Rectangle two-corner/decomposition state, atomic four/six-Line Rectangle creation, Creation Role and Construction-diagonal behavior, Profile exclusion of Construction geometry, read-only whole-entity measurement, Part hosting, schema-v6 persistence/history, provider-neutral presentation/input, selection, hover/grips, direct manipulation, common transforms, normal/Grip Copy identity behavior, Repeat Last Command, Space CycleEditMode, R10 quantities/point grammar/Polar/Dynamic Input and Profile topology invariance across common transforms.

Key registered tests include:

- `sk02a.shared_2d_core` / `sk02a.shared_2d_boundaries` — neutral model/dependency boundaries plus whole-entity measurement, curve relations, regions, Construction exclusion, open/overlap diagnostics, holes/islands, point picking and region composition;
- `sk06a.circle_arc_model_persistence` / `sk06a.circle_arc_interaction_state` — mixed Line/Circle/Arc authored and interaction semantics;
- `e1.arc_numerical_stability` — 3-Point Arc translation/scale conditioning, radial residual through all requested points, CW/CCW/long branch preservation, ±1e6 translation, 1e-200/1e200 scale, near-collinear acceptance and fail-closed exact/invalid extremes;
- `sk07a.transform_core`, `sk07b.transform_core`, `sk07b.common_transform_state`, `sk07b.transform_controller` — mixed transforms and atomic controller behavior, including closed Line+Arc region invariance through Move/Rotate/positive Scale/Mirror/repeated transforms, preservation of both endpoints in the Arc+diameter case, and proof that a real gap remains open;
- `sk07c.copy_command`, `sk07c.copy_interaction_state`, `sk07c.copy_controller` — fresh identity, repeated placement, Undo/Redo identity restoration and high-water persistence;
- `sk07d.repeat_last_command_controller` and `sk07e.space_cycle_edit_mode` — runtime command/grip grammar;
- `r10.quantity_input`, `sk07f.precision_input_state` / `sk07f.precision_input_controller` — shared quantity grammar plus PointRequest capabilities, coordinate/polar/direct-distance resolution and request-local precision state across creation, transform and grip paths;
- `sk02b.part_sketch_model` — durable Profile RegionIntent resolution, including a mixed Line+Arc Profile that remains valid after a common source transform while retaining source EntityIds/anchors;
- `wb02.cad_input_session`, `wb02.cad_input_boundaries`, `wb02.global_cad_input_ui` plus the Workbench Sketch-host regression — keyboard-first transport, focus arbitration, stale-context rejection and real Workbench integration.

Work-item completion uses the repository's exact-head Windows gate; current-state documentation does not preserve obsolete milestone gate counts.
