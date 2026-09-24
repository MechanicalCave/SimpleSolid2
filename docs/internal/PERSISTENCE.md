# Persistence — As-built

<!-- doc-id: internal.persistence -->
<!-- document-kind: internal -->

<!-- section-id: internal.persistence.authority -->
## Filesystem authority

The Project Workspace filesystem is authoritative for physical Project content.

There is no hidden durable database owning Project or CAD Document identity separately from the Workspace.

A top-level CAD Document is one portable user-visible native file. Path is location; embedded DocumentId is logical identity.

<!-- section-id: internal.persistence.project-metadata -->
## Project metadata

Project metadata is stored at:

```text
<Workspace>/.simplesolid/project.json
```

The current schema contains ProjectId, DisplayName, SchemaVersion and CreatedAt.

Metadata initialization is fail-closed and uses staged publication. Metadata loading has a size limit and validates schema and field meaning before a ProjectSession can be opened.

<!-- section-id: internal.persistence.part-document -->
## Native Document container and Part payload

A native top-level Part is stored as `*.ss2part`.

The physical representation is native Document container v1: one ZIP-compatible package with exactly two mandatory files and an optional derived namespace:

```text
Part001.ss2part
├── manifest.json
├── authored/
│   └── document.json
└── derived/
    └── ... optional disposable assets
```

Container v1 uses UTF-8 JSON for `manifest.json` and `authored/document.json`.

The common manifest contains exactly:

```text
format
container_version
document_kind
document_id
domain_schema_version
```

The current format identifier is `simplesolid.native-document`; container version is 1.

Shared persistence owns ZIP recognition, bounded parsing, common manifest recognition and container safety. It does not interpret Part engineering meaning.

The Part domain owns `authored/document.json`. Current Part domain schema version 1 persists:

- Number;
- Title;
- Description;
- Engineering Revision;
- built-in Origin visibility.

DocumentId is stored only in the common manifest. ProjectId is not embedded in the Document. Workspace membership remains physical/runtime context.

`DocumentRevision` is an in-memory synchronization counter and is not serialized.

The former line-based `SS2PART` bootstrap format is not a supported legacy product format. PERSIST-01 intentionally resets persistence before meaningful user data exists; old bootstrap files fail closed instead of being migrated.

Future accepted domain-schema changes remain explicitly versioned and require their own compatibility/migration decision.

<!-- section-id: internal.persistence.atomic-save -->
## Atomic Document publication and Save

The complete native package is built before publication.

New Part creation writes staged bytes and publishes only if the target path does not already exist.

Save writes a temporary sibling file and replaces the target only after the complete package has been produced. On Windows, replacement uses an OS replace operation rather than delete-then-rename.

The DocumentSession advances its save checkpoint only after successful persistence. If package construction or replacement fails, the in-memory authored state remains dirty and the previous durable Part remains authoritative.

The initial implementation uses whole-file replacement; it does not update ZIP entries in place.

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

Persistent user visibility of built-in Origin references is intentionally **not** in this runtime-only list; it is authored Part state.

The native package reserves `derived/*` for optional disposable assets. Unknown safe derived entries may be ignored. Deleting derived content must not destroy authored design intent. PERSIST-01 does not generate thumbnails or another derived asset.

Recent availability is also derived at runtime.

<!-- section-id: internal.persistence.identity-safety -->
## Identity and container safety

Part discovery reads the common manifest, then Part authored state.

A rename or move inside the Workspace does not change DocumentId.

If multiple native Part files declare one DocumentId, discovery records an identity conflict containing all relative paths and resolution/open by that DocumentId fails closed. No automatic ID rewrite is performed.

Container v1 rejects unsafe or ambiguous input, including unsupported container versions, ZIP64, encrypted/unsupported entries, duplicate entry names, unsafe entry paths, missing mandatory entries, oversized content, invalid mandatory JSON and unsupported Part domain schemas.

A `.ss2part` whose manifest declares another Document kind fails closed.

`document_id` uses Core's canonical DocumentId serialization and is parsed by Core. Persistence does not define a second identity representation.

<!-- section-id: internal.persistence.non-goals -->
## Current non-goals

The current persistence layer does not provide Project synchronization/semantic merge, cloud locking, Part Save As / Save Copy As UI, identity-conflict repair, Assembly/Drawing semantic persistence, Sketch persistence, modeled geometry persistence, thumbnail generation, tile/icon browsing, or camera/selection persistence between application runs.

The architecture reserves the same native package mechanism for future Assembly and Drawing, but their semantic schemas are not implemented by PERSIST-01.

Implementation dependencies are vendored and pinned: miniz 3.1.2 for ZIP mechanics and nlohmann/json 3.12.0 for JSON. Normal builds do not download them, and their types do not cross CAD-domain public semantic APIs.
