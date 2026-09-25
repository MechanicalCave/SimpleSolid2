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
- the first authored primitive, `Line`;
- the minimal value-semantic `SketchModel`.

It has no dependency on Part, Application/DocumentSession, Persistence, Viewer, Qt, OCCT or filesystem paths.

<!-- section-id: internal.shared-2d.coordinates -->
## Sketch-local coordinates

`Point2` stores `u` and `v` as finite real values in Sketch-local coordinates.

These values represent physical model-length coordinates in the host Sketch frame. Shared 2D does not store the Part/3D placement, camera transform, screen coordinates, snap tolerance or display units.

The host-specific mapping from local U/V into 3D remains outside Shared 2D.

<!-- section-id: internal.shared-2d.identity -->
## Entity identity

`EntityId` is an opaque stable identity scoped to one `SketchModel`.

The default-constructed value is invalid. Valid IDs are allocated by `SketchModel`; there is intentionally no public numeric accessor or persistence representation in SK-02A.

Current allocation is monotonic within the continuing model instance. Erasing an entity does not make its ID immediately available for a different entity.

Ordinary value-copy of `SketchModel` preserves existing EntityIds and allocator state while copying the authored storage by value. Mutating one copied state therefore does not alias another copied state.

This implementation detail is not a persistence contract. Future save/reopen and Undo/Redo integration must preserve the semantic identity rules from ADR-0009.

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

The current `SketchModel` provides the smallest authored-entity lifecycle required by SK-02A:

```text
addLine(start, end)
findLine(EntityId)
erase(EntityId)
entityCount()
```

`addLine` validates finite coordinates and exact non-zero length before mutation. Invalid geometry is rejected with `std::invalid_argument`.

`findLine` returns no entity for an invalid, erased or otherwise unknown ID.

`erase` removes exactly the addressed Line and returns false for an invalid or unknown ID. Storage compaction does not alter the identities of remaining Lines.

<!-- section-id: internal.shared-2d.boundaries -->
## Deliberately not implemented yet

The current Shared 2D core does not yet implement:

- Part-host embedding of authored entities or Part persistence schema changes;
- DocumentSession commands or host Undo/Redo;
- intrinsic Origin runtime presentation or snapping;
- Viewer presentation, pointer-to-plane input, Select or cursor behavior;
- grips/direct manipulation;
- Circle, Arc or construction geometry;
- snapping, inference, dimensions, constraints or solver evaluation;
- intersections, profiles/regions or projected/reference geometry;
- planar-face Sketch support.

Those capabilities are governed by the accepted Sketch roadmap and require later bounded Work Contracts.

<!-- section-id: internal.shared-2d.tests -->
## Verification

`sk02a.shared_2d_core` proves authored Line validation, identity, lookup/erase, non-reuse, equal-coordinate endpoint independence and value-copy isolation.

`sk02a.shared_2d_boundaries` guards the Shared 2D source tree against accidental dependencies on Part, Application, Persistence, Viewer, Qt and OCCT/provider tokens.

The repository Windows gate builds the exact PR head and runs the complete CTest suite.
