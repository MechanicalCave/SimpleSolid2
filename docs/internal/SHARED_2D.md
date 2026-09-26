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
- the first authored primitive, `Line`;
- the value-semantic `SketchModel` plus validated state/restore transfer;
- host-neutral runtime `SketchInteractionState` for Select/Line tool semantics and transient EntityId selection.

Each persistent Part-hosted Sketch now embeds one `SketchModel` by value. That host integration does not reverse the dependency: `simplesolid2_sketch` still has no dependency on Part, Application/DocumentSession, Persistence, Viewer, Qt, OCCT or filesystem paths.

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
## Authored Line

A `Line` owns:

```text
EntityId
Start(U,V)
End(U,V)
```

Start and End are authored values owned by that Line.

Two Lines may have endpoints with exactly equal coordinates. Equal coordinates do not create shared Point identity or a persistent relation.

A Line whose Start and End are exactly equal is rejected. SK-02A deliberately applies no epsilon or near-zero length rejection: a finite non-zero Line remains valid regardless of how small its length is.

<!-- section-id: internal.shared-2d.model -->
## SketchModel operations

The current `SketchModel` provides the authored-entity lifecycle required by the implemented Line workflow:

```text
addLine(start, end)
findLine(EntityId)
updateLine(EntityId, start, end)
erase(EntityId)
entityCount()
entityIdCursor()
preserveEntityIdCursor(cursor)
state()
restore(state)
```

`addLine` validates finite coordinates and exact non-zero length before mutation. Invalid geometry is rejected with `std::invalid_argument`.

`findLine` returns no entity for an invalid, erased or otherwise unknown ID.

`updateLine` replaces the authored Start/End geometry of exactly one existing Line while preserving its `EntityId`. It fails closed for an invalid/missing ID, non-finite coordinates or exact zero length. Application-facing direct manipulation does not call this method directly on the live document: it stages semantic Line geometry updates through `DocumentSession`.

`erase` removes exactly the addressed Line and returns false for an invalid or unknown ID. Storage compaction does not alter the identities of remaining Lines.

<!-- section-id: internal.shared-2d.interaction -->
## Sketch interaction state

SK-04A adds one host-neutral runtime interaction authority inside Shared 2D. It is not authored Sketch state and is never persisted.

The default tool is `Select`. Activating `Line` enters `AwaitFirstPoint`; accepting the first finite point establishes a runtime anchor and enters `AwaitNextPoint`. A subsequent distinct finite point produces a `LineSegmentIntent`, but the state does not execute Part/Application commands itself.

The Line commit handoff is explicitly two-phase:

```text
runtime accepts candidate endpoint
→ pending LineSegmentIntent
→ host executes semantic AddSketchLineCommand
→ host acknowledges success/failure
```

Only successful acknowledgement advances the continuous-Line anchor. Failure preserves the previous anchor. Exact-zero candidate segments produce no request and introduce no epsilon policy.

Preview intent is transient and exists only while Line has an anchor and no unresolved commit request. Finish/Cancel return to Select and discard only uncommitted runtime state. Esc is hierarchical: AwaitNextPoint → AwaitFirstPoint → Select.

The same interaction state owns transient semantic Sketch selection as `EntityId` values plus optional primary identity. Ordinary point selection is additive: an unselected Line is added and becomes primary, while re-clicking an already selected Line changes only primary. Ctrl toggles membership. Window/Crossing selection adds semantic IDs, or toggles them with Ctrl, without allowing provider result order to choose primary. Blank LMB and Select-mode Esc clear selection.

SK-05A adds runtime Line hover and finite semantic grips without introducing a second interaction authority. Every selected editable Line exposes Start, Center and End grip roles. Grip references contain semantic `EntityId + HandleRole`; Viewer presentation tokens remain outside Shared 2D.

A `DirectManipulationSession` freezes the current semantic selection and interaction-start geometry. Start/End perform owner-only Reshape. Center performs Move for the complete frozen selection using the clicked Line midpoint as pivot. Pointer values are consumed through the shared `ResolvedSketchInput` seam; in SK-05A that resolver is intentionally identity for finite Sketch-local U/V so later snapping/precision work can extend one seam instead of replacing tool logic.

Direct-manipulation geometry is computed as transient preview data only. It never mutates `SketchModel`, revision, dirty state, identity allocation or Undo history. Esc cancels the session and preserves selection; a subsequent Esc in ordinary Select clears selection. Selection mutation is rejected while manipulation is active.

<!-- section-id: internal.shared-2d.boundaries -->
## Deliberately not implemented yet

The current Shared 2D / Part integration now has R3 runtime presentation/input adapters outside the Shared 2D target: active authored Lines can be presented, intrinsic Origin is a runtime overlay, and provider-neutral rays can be mapped to active Sketch U/V. Those runtime capabilities do not add authored state to `simplesolid2_sketch`.

The current product now wires the Shared 2D interaction state end-to-end through the Part Sketch edit context. Additive point/Window/Crossing selection, Ctrl-toggle, semantic primary, Line hover, Start/Center/End grips, bounded Line direct manipulation, continuous Line creation, transient preview, atomic multi-entity Delete, hierarchical Esc and history cancellation are runtime/application adapters around the same host-neutral state.

The current product still does not implement:

- intrinsic Origin snapping;
- Circle, Arc or construction geometry;
- snapping, inference, dimensions, constraints or solver evaluation;
- intersections, profiles/regions or projected/reference geometry;
- planar-face Sketch support.

Those capabilities are governed by the accepted Sketch roadmap and require later bounded Work Contracts.

<!-- section-id: internal.shared-2d.tests -->
## Verification

`sk02a.shared_2d_core` proves authored Line validation, identity, lookup/erase, non-reuse, equal-coordinate endpoint independence and value-copy isolation.

`sk02a.shared_2d_boundaries` guards the Shared 2D source tree against accidental dependencies on Part, Application, Persistence, Viewer, Qt and OCCT/provider tokens.

SK-02B adds `sk02b.part_sketch_model`, `sk02b.sketch_entity_lifecycle` and `sk02b.part_sketch_persistence` coverage for Part ownership, value-copy isolation, semantic Add/Erase commands, live Undo/Redo identity high-water, schema-v3 persistence, v1/v2 backward readability and Save→Close→Reopen identity/geometry preservation.

SK-03A adds `sk03a.viewer_sketch_contracts`, `sk03a.sketch_viewport_mapping`, `sk03a.part_viewport_controller` and `sk03a.viewer_native_input` coverage for neutral authored/preview presentation contracts, U/V↔3D and ray→U/V mapping, runtime token bindings, fail-closed active-Sketch lifecycle, routing/cursor state and real Qt/OCCT spatial input before/after orbit.

SK-04A adds `sk04a.sketch_interaction_state`, `sk04a.batch_delete` and `sk04a.line_commit_protocol` coverage for Select/Line runtime semantics, explicit request/acknowledgement, continuous anchor progression, exact-zero suppression, Finish/Cancel/Esc behavior, transient EntityId selection/reconciliation, atomic multi-entity Delete and one-segment-per-Undo integration with `DocumentSession`.

SK-04C adds `sk04c.part_sketch_interaction_controller` coverage for the bounded host coordinator: default Select, spatial-tool routing/cursor projection, continuous Line commits, runtime preview, point selection, Ctrl-toggle sampled at release, Crossing rectangle selection, atomic Delete, history reconciliation, exact-zero suppression and hierarchical Esc.

SK-05A adds `sk05a.direct_manipulation_state`, `sk05a.line_geometry_command` and `sk05a.part_sketch_direct_manipulation` coverage for additive/primary selection semantics, frozen direct-manipulation state, Line Reshape/Move preview, identity-preserving atomic batch geometry commits, stale-revision failure, exact no-op history behavior, Undo/Redo and the provider-neutral grip bridge. Native provider coverage also exercises screen-space grip hit testing separately from authored Line selection.

The repository Windows FULL gate builds the exact implementation head and runs the complete CTest suite.
