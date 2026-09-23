# Part Documents — As-built

<!-- doc-id: internal.part-documents -->
<!-- document-kind: internal -->

<!-- section-id: internal.part-documents.model -->
## Current model

PART-01 implements a minimal persistent `PartDocument` without CAD features or geometry.

Its authored state currently consists of stable `DocumentId` and common Document Properties:

- Number;
- Title;
- Description;
- Engineering Revision.

`DocumentRevision` is a technical monotonic counter for successful semantic mutations within the loaded lifecycle.

<!-- section-id: internal.part-documents.mutation -->
## Command and transaction boundary

Persistent property changes follow:

```text
Qt / caller
→ SetDocumentPropertiesCommand
→ DocumentSession validation/history preparation
→ PartDocumentTransaction staged state
→ atomic domain commit
```

A no-op creates neither a revision increment nor an Undo entry.

A rejected/failed transaction leaves authored state and existing history unchanged.

Undo and Redo reapply authored states through PartDocumentTransaction and therefore count as new semantic mutations with increasing technical DocumentRevision.

<!-- section-id: internal.part-documents.session -->
## DocumentSession

`DocumentSession` is runtime-only and contains:

- current physical path;
- loaded PartDocument;
- expected technical revision;
- Undo/Redo history;
- save checkpoint represented by the last successfully saved authored state.

`needsSave()` compares authored state with the saved authored checkpoint. It is not defined by numeric DocumentRevision equality, which allows Undo back to the saved semantic state to become clean even though DocumentRevision increased.

Closing and reopening creates fresh runtime history.

<!-- section-id: internal.part-documents.persistence -->
## Native Part persistence

The native extension is `.ss2part`.

Schema version 1 stores only authored state and required identity/version metadata. It does not serialize ProjectId, DocumentSession, Undo/Redo, Qt, Viewer or OCCT data.

Save uses shared staged atomic-file persistence. The save checkpoint changes only after a successful write/replace.

<!-- section-id: internal.part-documents.discovery -->
## Discovery and canonical sessions

ProjectSession owns a rebuildable Workspace index.

A DocumentId resolves only when exactly one valid native file in the Workspace declares it. Duplicate physical files with the same DocumentId form `IdentityConflict` and include all discovered relative paths.

When a resolved Part is already open, a second Open request returns the existing canonical DocumentSession rather than creating a second mutable session for the same DocumentId.

<!-- section-id: internal.part-documents.current-limits -->
## Current limits

The current Part domain has no Sketch, Body, Feature, geometry evaluation, recompute graph, Viewer, topology selection, persistent naming, Material model, Assembly or Drawing.

There is no OCCT dependency in the implemented PART-01 authored-state lifecycle.
