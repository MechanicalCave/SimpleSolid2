# Part Documents — As-built

<!-- doc-id: internal.part-documents -->
<!-- document-kind: internal -->

<!-- section-id: internal.part-documents.model -->
## Current model

The current `PartDocument` is persistent but still contains no Sketch, Body, Feature or modeled solid geometry.

Its authored state consists of stable `DocumentId`, common Document Properties and persistent presentation state for the seven built-in Origin references.

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

Current commands include common Document Properties and batch built-in reference visibility.

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

Current schema version 2 stores authored identity/properties plus a compact built-in Origin visibility mask. It does not serialize ProjectId, DocumentSession, Undo/Redo, camera, active selection, Qt objects, Viewer objects or OCCT handles.

Schema version 1 remains readable. Opening v1 does not rewrite the file. A later successful Save publishes the current v2 schema with deterministic default Origin visibility for the migrated state.

Save uses shared staged atomic-file persistence. The save checkpoint changes only after a successful write/replace.

<!-- section-id: internal.part-documents.discovery -->
## Discovery and canonical sessions

ProjectSession owns a rebuildable Workspace index.

A DocumentId resolves only when exactly one valid native file in the Workspace declares it. Duplicate physical files with the same DocumentId form `IdentityConflict` and include all discovered relative paths.

When a resolved Part is already open, a second Open request returns the existing canonical DocumentSession rather than creating a second mutable session for the same DocumentId.

<!-- section-id: internal.part-documents.current-limits -->
## Current limits

The shared CAD Workbench and Viewer can present the Part Origin and reference grid, but PartDocument still has no Sketch, Body, Feature, geometry evaluation, recompute graph, modeled topology, persistent topology naming or Material model.

The Viewer is not a second model: no OCCT object is durable Part identity or authored state.
