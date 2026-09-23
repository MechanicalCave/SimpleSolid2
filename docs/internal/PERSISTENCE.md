# Persistence — As-built

<!-- doc-id: internal.persistence -->
<!-- document-kind: internal -->

<!-- section-id: internal.persistence.authority -->
## Filesystem authority

The Project Workspace filesystem is authoritative for physical Project content.

There is no hidden durable database owning Project or Part Document identity separately from the Workspace.

<!-- section-id: internal.persistence.project-metadata -->
## Project metadata

Project metadata is stored at:

```text
<Workspace>/.simplesolid/project.json
```

The current schema contains ProjectId, DisplayName, SchemaVersion and CreatedAt.

Metadata initialization is fail-closed and uses staged publication. Metadata loading has a size limit and validates schema and field meaning before a ProjectSession can be opened.

<!-- section-id: internal.persistence.part-document -->
## Native Part Documents

A native top-level Part is stored as `*.ss2part`.

The current private schema starts with the `SS2PART` signature. Schema version 2 persists native document kind, DocumentId, Number, Title, Description, Engineering Revision and the built-in Origin visibility mask.

The schema does not persist ProjectId. Workspace membership is physical/runtime context, not an owner ProjectId embedded in each Part.

`DocumentRevision` is an in-memory synchronization counter and is not serialized.

Schema version 1 remains readable. Loading v1 assigns deterministic built-in Origin visibility defaults in memory and does not rewrite the source file. A successful later Save writes current schema v2.

Unsupported future schemas, malformed fields, unsupported visibility bits, invalid DocumentIds and wrong declared document kind fail closed with structured diagnostics.

<!-- section-id: internal.persistence.atomic-save -->
## Atomic Part publication and Save

New Part creation writes staged bytes and publishes only if the target path does not already exist.

Save writes a temporary sibling file and replaces the target only after the complete serialized state has been written. On Windows, replacement uses an OS replace operation rather than delete-then-rename.

The DocumentSession advances its save checkpoint only after successful persistence. If replacement fails, the in-memory authored state remains dirty and the previous durable Part remains authoritative.

<!-- section-id: internal.persistence.recent -->
## Recent Projects catalog

Recent Projects is persisted outside the Project as application/user state.

The normal application selects its state root through Qt `QStandardPaths::AppLocalDataLocation` and stores `recent-projects-v1.txt`.

Each entry stores ProjectId, DisplayName and the remembered absolute Workspace path.

<!-- section-id: internal.persistence.derived-state -->
## Runtime-only and derived state

The following state is not serialized as Part authored state:

- ProjectSession and DocumentSession;
- Undo/Redo history and save-history cursor;
- Workspace discovery/conflict state;
- active Document tab;
- camera / projection / pan / orbit / zoom;
- selected set and primary selection;
- reference grid runtime presentation;
- Qt objects;
- Viewer provider objects and OCCT handles;
- evaluated B-Rep or tessellation.

Persistent user visibility of built-in Origin references is intentionally **not** in this runtime-only list; it is authored presentation state in schema v2.

Recent availability is also derived at runtime.

<!-- section-id: internal.persistence.identity-safety -->
## Identity safety

Project Recent state uses ProjectId as its logical key.

Part discovery uses embedded DocumentId as logical identity and path only as location. A rename or move inside the Workspace does not change identity.

If multiple native Part files declare one DocumentId, discovery records an identity conflict containing all relative paths and resolution/open by that DocumentId fails closed. No automatic ID rewrite is performed.

<!-- section-id: internal.persistence.non-goals -->
## Current non-goals

The current persistence layer does not provide Project synchronization/semantic merge, cloud locking, Part Save As / Save Copy As UI, identity-conflict repair, Assembly/Drawing persistence, modeled geometry persistence, or camera/selection persistence between application runs.
