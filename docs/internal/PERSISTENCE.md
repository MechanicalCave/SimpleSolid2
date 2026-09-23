# Persistence — As-built

<!-- doc-id: internal.persistence -->
<!-- document-kind: internal -->

<!-- section-id: internal.persistence.authority -->
## Filesystem authority

The Project Workspace filesystem is authoritative for physical Project content.

Current Project-level durable data is intentionally small. There is no hidden durable database that owns Project identity separately from the Workspace.

<!-- section-id: internal.persistence.project-metadata -->
## Project metadata

Project metadata is stored at:

```text
<Workspace>/.simplesolid/project.json
```

The current schema contains ProjectId, DisplayName, SchemaVersion and CreatedAt.

Metadata writes use a private atomic-write path. Initialization fails closed if the Workspace is invalid or already initialized.

Metadata loading has a size limit and validates schema and field meaning before a ProjectSession can be opened.

<!-- section-id: internal.persistence.recent -->
## Recent Projects catalog

Recent Projects is persisted outside the Project as application/user state.

The normal application selects its state root through Qt `QStandardPaths::AppLocalDataLocation` and stores:

```text
recent-projects-v1.txt
```

The catalog has a private versioned text format with a magic header and schema version. Each entry stores:

- ProjectId;
- DisplayName;
- absolute remembered Workspace path.

The catalog has a 256 KiB size guard and a logical maximum of 20 entries.

Catalog publication uses a temporary file and replacement backup so a failed replacement can restore the previous catalog.

<!-- section-id: internal.persistence.derived-state -->
## Derived state

Recent availability status is not persisted.

`Available`, `WorkspaceMissing`, `ProjectInvalid` and `IdentityMismatch` are recomputed from the current filesystem and Project metadata whenever the Hub refreshes.

ProjectSession is also runtime-only and is never serialized as Project design intent.

<!-- section-id: internal.persistence.identity-safety -->
## Identity safety

The store uses ProjectId as the logical Recent key.

Path normalization is used to compare locations, but path is never promoted to durable Project identity.

Relocation validates the expected ProjectId at the replacement Workspace before changing the remembered location.

A second presented location with the same ProjectId is an identity conflict unless the user explicitly performs relocation.

<!-- section-id: internal.persistence.non-goals -->
## Current non-goals

The current persistence layer does not provide:

- global filesystem scanning for Project copies;
- Project synchronization or merge;
- cloud locking;
- Duplicate / Save as New Project;
- CAD Document persistence;
- geometry or Viewer persistence.
