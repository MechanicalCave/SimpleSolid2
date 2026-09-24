# ADR-0005 — Part-hosted Sketch identity, support and edit context

**Status:** ACCEPTED  
**Date:** 2026-09-24  
**Owner acceptance:** 2026-09-24  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** ADR-0002, ADR-0003, ADR-0004

## Context

SS2 now has a common CAD Workbench, provider-neutral 3D Viewport, stable built-in Origin references and accepted native Part persistence.

The first Sketch implementation must establish the host/lifecycle boundary without simultaneously introducing 2D entities, constraints, solver behavior, Body/Feature modeling, topology naming or Datum architecture.

The Owner explicitly chose a small-contract strategy so mistakes in Sketch identity, placement or host integration can be corrected before a large Sketch subsystem depends on them.

## Decision

### 1. Sketch is an embedded authored object, not a Document

A Part-hosted Sketch is durable authored state inside one PartDocument.

It has stable `SketchId` independent of Tree order, display position and runtime editor state.

Sketch is not a top-level Document and does not own a standalone native file.

### 2. Host owns support, placement and persistence

Part owns for each hosted Sketch:

- SketchId;
- support reference;
- SketchPlacement;
- persistent visibility;
- embedding in Part persistence;
- Commands/Transactions and Undo/Redo integration;
- Tree placement and 3D scene/edit integration.

The reusable future Sketch Core owns 2D authored geometry/constraints, not these Part-host responsibilities.

### 3. SK-01 supports only built-in Origin planes

The only creatable supports in SK-01 are:

```text
BuiltinReference::XYPlane
BuiltinReference::XZPlane
BuiltinReference::YZPlane
```

Axes and Origin Point are invalid Sketch supports.

The architecture must leave room for later support kinds, including Datum/Construction Plane and a semantically stable planar model face, but SK-01 does not implement or guess those references.

In particular, no OCCT face handle, topology ordinal, Viewer token or Tree row may become durable Sketch support identity.

### 4. SketchPlacement is explicit authored Part state

A Sketch has an explicit 3D placement/frame represented by origin plus local U/V axes.

For SK-01 the placement is deterministically initialized from the selected built-in Origin plane and must be coherent with that support.

Malformed persisted support/placement combinations fail closed.

Future contracts must explicitly decide dependency/associativity behavior for movable Datum or model-face supports rather than inheriting it accidentally from SK-01.

### 5. Sketch creation is a semantic command

The user starts the `Sketch` tool in the active Part Workbench, then selects a valid Origin plane.

Persistent creation follows the ordinary path:

```text
Sketch tool
→ semantic support selection
→ CreatePartSketchCommand
→ validation/revalidation
→ Part transaction
→ PartDocument
→ persistence/presentation refresh
```

Creation is one Undo/Redo entry. Undo removes the newly created Sketch; Redo restores the same SketchId and authored state.

### 6. Edit context remains in the common 3D Viewport

Successful creation enters a runtime Sketch edit context in the same Workbench and same Document Viewport.

No separate Sketch application/window/canvas is opened.

On entry:

- the reference grid uses the Sketch frame;
- the camera automatically aligns normal to the Sketch plane;
- the view is fitted to a useful working extent.

This camera alignment is convenience only. The user may immediately Pan/Zoom/Orbit/use ViewCube.

Camera navigation never mutates SketchPlacement, DocumentRevision or needsSave.

### 7. Finish Sketch is runtime lifecycle

`Finish Sketch` exits the active Sketch edit context.

It does not delete the Sketch or change its authored geometry/placement merely because editing ends.

The active editor context itself is runtime-only and is not persisted.

An empty SK-01 Sketch is still a durable semantic object even though it has no authored 2D entities to render yet.

### 8. Persistence versioning

SK-01 advances the Part domain schema from v1 to v2.

Part schema v1 remains readable and restores an empty Sketch collection. Opening v1 does not rewrite the source file.

A later successful Save writes current schema v2.

Schema v2 persists SketchId, support, placement and visibility. It does not persist editor state, camera, grid runtime state or provider objects.

### 9. Multi-host direction

Future Part, Assembly and Drawing hosts may reuse the same Sketch Core.

The host contract is shared in principle, but presentation need not be identical:

- Part/Assembly may edit a Sketch inside the common 3D Viewport;
- Drawing may later host a true DrawingSketch in its own 2D page/editor context.

Shared Core must not depend on PartDocument, AssemblyDocument, DrawingDocument, Qt, OCCT or filesystem paths.

## Explicitly deferred

SK-01 does not define or implement:

- Line/Arc/Circle entities;
- entity IDs;
- constraints or dimensions;
- constraint solver;
- snapping/inference;
- profile/loop semantics;
- Sketch-to-Feature consumption;
- Body/Feature/Extrude;
- Datum/Construction Plane object model;
- planar model-face support;
- persistent topology naming;
- external references/projected geometry;
- Assembly or Drawing implementation;
- Sketch delete/rename/reorder UX;
- detailed Sketch Properties UI.

## Consequences

SK-01 proves the smallest durable host/lifecycle seam needed before 2D authoring begins.

A later SK-02 can add the first shared 2D entity model without redesigning how a Sketch belongs to a Document or enters/leaves edit mode.
