# Part Documents — As-built

<!-- doc-id: internal.part-documents -->
<!-- document-kind: internal -->

<!-- section-id: internal.part-documents.model -->
## Current model

The current `PartDocument` is persistent and hosts durable Part Sketch objects with embedded Shared 2D authored Line geometry. It still contains no Body, Feature or modeled solid B-Rep.

Its authored state consists of stable `DocumentId`, common Document Properties, persistent presentation state for the seven built-in Origin references and an ordered collection of Part-hosted Sketch records.

Each Part Sketch has stable `SketchId`, semantic support restricted to XY/XZ/YZ built-in Origin planes, explicit `SketchPlacement`, persistent visibility and one value-owned `sketch::SketchModel`. Part owns host support/placement/visibility/persistence semantics; Shared 2D owns the embedded entity identity and authored 2D geometry.

`DocumentRevision` is a technical monotonic counter for successful semantic mutations within the loaded lifecycle.

<!-- section-id: internal.part-documents.origin -->
## Built-in Document Origin

Every Part has seven deterministic semantic references that always exist: Origin Point, X/Y/Z Axis and XY/XZ/YZ Plane.

Their identity comes from their built-in role, not from random DocumentObjectId allocation.

Hiding an Origin reference changes only persistent presentation visibility. It does not delete the reference or change its identity.

The current default is Origin Point plus X/Y/Z axes visible and the three principal planes hidden.

<!-- section-id: internal.part-documents.mutation -->
## Command and transaction boundary

Persistent authored changes follow:

```text
Qt / caller
→ semantic DocumentSession command
→ history / revision validation
→ PartDocumentTransaction staged state
→ atomic domain commit
```

Current commands include common Document Properties, batch built-in reference visibility, `CreatePartSketchCommand`, `AddSketchLineCommand`, `EraseSketchEntityCommand` and atomic `EraseSketchEntitiesCommand`.

Sketch creation revalidates that support is XY/XZ/YZ Origin plane, derives its initial placement, allocates a stable SketchId and commits the new hosted Sketch as one transaction/Undo entry. Add Line targets `SketchId` plus Start/End and returns the allocated model-local `EntityId`. Single Erase targets `SketchId + EntityId`. Batch Erase targets one SketchId plus a non-empty EntityId set and validates the entire set before mutation; invalid, duplicate, stale or unknown identities reject the whole command with no partial erase.

Each successful Add or single Erase is one Part transaction, one history entry and one revision increment. `EraseSketchEntitiesCommand` validates the complete non-empty EntityId set before mutation; invalid, duplicate, stale or unknown targets fail without partial erase. A successful batch, whether one entity or many, is one Part transaction, one history entry and one revision increment. Undo restores every deleted entity with its original EntityId and geometry; Redo removes the same authored set again. A session-local cursor table keyed by SketchId preserves the maximum observed EntityId allocation cursor even when history temporarily rewinds geometry or removes/recreates the Sketch through Undo/Redo.

A multi-selection Show/Hide is one semantic command, one successful revision increment and one Undo entry.

A no-op creates neither a revision increment nor an Undo entry. A rejected/failed transaction leaves authored state and existing history unchanged.

Undo and Redo reapply authored states through PartDocumentTransaction and therefore count as new semantic mutations with increasing technical DocumentRevision.

<!-- section-id: internal.part-documents.session -->
## DocumentSession

`DocumentSession` is runtime-only and contains current physical path, loaded PartDocument, expected technical revision, Undo/Redo history and the save checkpoint.

`needsSave()` compares authored state with the saved authored checkpoint. It is not defined by numeric DocumentRevision equality, which allows Undo back to the saved semantic state to become clean even though DocumentRevision increased.

Closing and reopening creates fresh runtime history.

<!-- section-id: internal.part-documents.persistence -->
## Native Part persistence

The native extension is `.ss2part`.

Current Part domain schema version 3 stores authored identity/properties, a compact built-in Origin visibility mask and the hosted Sketch collection. Each Sketch record stores SketchId, Origin-plane support, explicit placement, visibility and an embedded Shared 2D model.

The schema-v3 model stores canonical decimal `next_entity_id` plus ordered Line records containing canonical model-local EntityId and finite Start/End U/V coordinates. Duplicate IDs, non-canonical IDs, IDs outside the cursor range and invalid Line geometry fail closed.

It does not serialize ProjectId, DocumentSession, Undo/Redo, active Sketch edit context, camera, active selection, Qt objects, Viewer objects or OCCT handles.

Part schema versions 1 and 2 remain readable. Schema v1 restores no Sketches; v2 restores its hosted Sketches with empty Shared 2D models. Opening either does not rewrite the file. A later successful Save publishes current schema v3.

Save uses shared staged atomic-file persistence. The save checkpoint changes only after a successful write/replace.

<!-- section-id: internal.part-documents.discovery -->
## Discovery and canonical sessions

ProjectSession owns a rebuildable Workspace index.

A DocumentId resolves only when exactly one valid native file in the Workspace declares it. Duplicate physical files with the same DocumentId form `IdentityConflict` and include all discovered relative paths.

When a resolved Part is already open, a second Open request returns the existing canonical DocumentSession rather than creating a second mutable session for the same DocumentId.

<!-- section-id: internal.part-documents.sketch-presentation -->
## Active Sketch presentation and spatial input

R3 does not change Part persistence or authored commands. It adds a runtime adapter around the current authoritative Part state.

While a Part Sketch is actively edited, `PartViewportController` rebuilds a neutral authored Sketch scene from the current embedded `SketchModel`, transforms Line endpoints from U/V to 3D through `SketchPlacement`, presents the intrinsic Origin as a non-authored overlay and keeps runtime presentation-token bindings back to `SketchId + EntityId`.

A separate preview scene is transient and independently replaceable/clearable. Neutral provider rays are intersected with the active Sketch frame above the provider to produce Sketch-local U/V. These operations do not change Part revision, dirty state, history or persistence.

If the active Sketch disappears through Undo/history, presentation fails closed and the runtime edit/input state is cleared.

<!-- section-id: internal.part-documents.current-limits -->
## Current limits

The Part model durably owns Shared 2D Line entities, and R3 presents the active Sketch's authored Lines plus intrinsic Origin in the shared 3D Viewport. Presentation tokens, preview and pointer input remain runtime-only and are not Part identity.

SK-04A now provides neutral Select/Line runtime semantics, transient EntityId selection and an atomic semantic multi-entity Delete command, but the current product UI still does not wire those capabilities into interactive Sketch Line drawing/editing, point/rectangle selection or Delete. Constraints, dimensions, solver, Datum/Construction Plane support, planar model-face support, Body, Feature, modeled solid geometry, persistent topology naming and Material model remain unavailable.

The Viewer is not a second model: no OCCT object or Viewer token is durable Part/Sketch identity, support or authored state.
