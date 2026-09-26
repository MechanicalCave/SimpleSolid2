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

The physical representation remains native Document container v1: one ZIP-compatible package with exactly two mandatory files and an optional derived namespace:

```text
Part001.ss2part
├── manifest.json
├── authored/
│   └── document.json
└── derived/
    └── ... optional disposable assets
```

Container v1 uses UTF-8 JSON. The common manifest contains `format`, `container_version`, `document_kind`, `document_id` and `domain_schema_version`. Shared persistence owns package/container safety; Part owns engineering meaning in `authored/document.json`.

The current Part domain writer is schema **v4**. It persists document properties, built-in Origin visibility and the ordered collection of Part-hosted Sketch records. Each Sketch record persists stable SketchId, built-in Origin-plane support, explicit SketchPlacement, visibility and one embedded Shared 2D model.

Schema-v4 Sketch models contain:

```text
next_entity_id
entities[]
```

`next_entity_id` is the canonical positive decimal shared EntityId high-water string. Each entity record has an explicit semantic `kind`:

- `line`: `id`, `start`, `end`;
- `circle`: `id`, `center`, `radius`;
- `arc`: `id`, `center`, `radius`, `start_angle`, `sweep_angle`.

Only canonical authored parameters are serialized. Tessellation, presentation tokens, grips, hover/selection, preview and creation-method metadata are not persisted.

Part schemas v1, v2 and v3 remain readable. V1 restores an empty Sketch collection. V2 restores host Sketch records with empty Shared 2D models and an initial cursor. V3 reads the previous `next_entity_id + lines[]` Line-only model. Opening an old schema does not rewrite the file; a later successful ordinary Save writes current schema v4.

DocumentId remains only in the common manifest. ProjectId is not embedded in the Document. `DocumentRevision` and Undo/Redo are runtime-only.

The former line-based `SS2PART` bootstrap format remains unsupported legacy test data and fails closed rather than being migrated.

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
- active Sketch edit context;
- Sketch support-pick tool state;
- Qt objects;
- Viewer provider objects and OCCT handles;
- evaluated B-Rep or tessellation.

Persistent user visibility of built-in Origin references is intentionally **not** in this runtime-only list; it is authored Part state.

The native package reserves `derived/*` for optional disposable assets. Unknown safe derived entries may be ignored. Deleting derived content must not destroy authored design intent. PERSIST-01 does not generate thumbnails or another derived asset.

Recent availability is also derived at runtime.

<!-- section-id: internal.persistence.identity-safety -->
## Identity and container safety

Part discovery reads the common manifest, then Part authored state.

A rename or move inside the Workspace does not change DocumentId. Duplicate native Part files declaring one DocumentId remain a fail-closed identity conflict; no automatic ID rewrite is performed.

Schema-v4 Sketch loading rejects malformed/non-canonical identity strings, duplicate EntityIds across any primitive kinds, EntityIds greater than or equal to `next_entity_id`, malformed/unknown `kind` values, non-finite geometry, exact-zero Lines, non-positive Circle/Arc radius, zero Arc sweep and Arc sweep whose magnitude reaches/exceeds a full turn. The same local EntityId value in two different SketchModels is valid because durable addressing is scoped by SketchId plus EntityId.

Schema-v3 validation remains intact for backward read compatibility.

Container v1 rejects unsafe or ambiguous input, including unsupported container versions, ZIP64, encrypted/unsupported entries, duplicate entry names, unsafe entry paths, missing mandatory entries, oversized content, invalid mandatory JSON and unsupported Part domain schemas.

A `.ss2part` whose manifest declares another Document kind fails closed. `document_id` uses Core's canonical DocumentId serialization; Persistence does not define a second identity representation.

<!-- section-id: internal.persistence.non-goals -->
## Current non-goals

The current persistence layer does not provide Project synchronization/semantic merge, cloud locking, Part Save As / Save Copy As UI, identity-conflict repair, Assembly/Drawing semantic persistence, Sketch constraints/dimensions/solver state, modeled solid geometry persistence, thumbnail generation, tile/icon browsing, or camera/selection persistence between application runs.

The architecture reserves the same native package mechanism for future Assembly and Drawing, but their semantic schemas are not implemented by PERSIST-01.

Implementation dependencies are vendored and pinned: miniz 3.1.2 for ZIP mechanics and nlohmann/json 3.12.0 for JSON. Normal builds do not download them, and their types do not cross CAD-domain public semantic APIs.
