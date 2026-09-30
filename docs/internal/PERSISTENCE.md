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

The current Part domain writer is schema **v7**. It persists document properties, the durable display/input length unit, built-in Origin visibility, hosted Sketch records, `next_profile_id` and authored Profiles. Canonical geometric length values remain millimetres regardless of the selected display/input unit.

Sketch models store canonical `next_entity_id` plus mixed `entities[]`. Each Line/Circle/Arc record stores its semantic kind, canonical EntityId, canonical geometry and authored `regular`/`construction` role.

Each Profile record stores canonical ProfileId, source SketchId, authored name/visibility and semantic RegionIntent. RegionIntent persists source EntityIds and endpoint/intersection anchors; runtime region indices, evaluated parameters, cached arrangements, sampled fills and provider topology are not persisted.

Schemas v1–v6 remain readable. Files without the v7 length-unit field restore **mm** in memory and keep all existing geometry numerically unchanged. A later successful Save of an older loaded file publishes schema v7.

DocumentId remains only in the common manifest. ProjectId, DocumentRevision, Undo/Redo, active tool state, Polar/Dynamic Input application-session configuration, Dynamic Input field focus/locks, region-analysis cache and Viewer state are runtime-only.

The former line-based `SS2PART` bootstrap format remains unsupported legacy test data and fails closed rather than being migrated.

<!-- section-id: internal.persistence.atomic-save -->
## Atomic Document publication and Save

The complete native package is built before publication.

New Part creation reserves/publishes through create-new semantics and fails if the target already exists. Temporary publication files are reserved with exclusive-create semantics rather than exists-check + truncating open.

A native Part load reads one bounded file snapshot and derives both the parsed document and `PartFileCheckpoint` from that same snapshot. The checkpoint contains expected DocumentId, exact byte length, SHA-256 digest and platform file identity.

Ordinary Save is conditional. A deterministic per-target cooperative guard is held across:

```text
inspect current target
→ compare with session checkpoint
→ build/write temporary replacement
→ atomically publish replacement
→ capture checkpoint for the newly published target
```

Save fails closed when another cooperating SS2 Save owns the guard, the target is missing, the durable DocumentId changed, the underlying file object was replaced, or exact file bytes changed. Conflict leaves the target untouched by that Save attempt, leaves authored state/revision/history unchanged and leaves both session checkpoints unchanged.

Successful Save returns the checkpoint of the newly published file; only then does `DocumentSession` advance its saved authored-state checkpoint and native-file checkpoint.

Whole-file replacement remains the publication model; ZIP entries are not updated in place. Atomic replacement, stale-file conflict detection and crash/power-loss durability are separate guarantees.

The strict cross-process no-lost-update guarantee applies to cooperating SS2 writers that honor the guard. Changes by unrelated external writers that are visible before validation are detected, but the system does not claim OS-level compare-and-swap against an arbitrary process racing after validation.

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
- Polar/Dynamic Input application-session settings;
- Polar capture, Dynamic Input field focus and request-local numeric locks;
- Qt objects;
- Viewer provider objects and OCCT handles;
- cached Shared 2D region analysis and transient Profile drafts;
- evaluated B-Rep or tessellation.

Persistent user visibility of built-in Origin references is intentionally **not** in this runtime-only list; it is authored Part state.

The native package reserves `derived/*` for optional disposable assets. Unknown safe derived entries may be ignored. Deleting derived content must not destroy authored design intent. The current implementation does not generate thumbnails or another derived asset.

Recent availability is also derived at runtime.

<!-- section-id: internal.persistence.identity-safety -->
## Identity and container safety

Part discovery reads the common manifest, then Part authored state.

A rename or move inside the Workspace does not change DocumentId. Duplicate native Part files declaring one DocumentId remain a fail-closed identity conflict; no automatic ID rewrite is performed.

Schema-v6 loading rejects malformed/non-canonical identity strings, duplicate EntityIds across primitive kinds, EntityIds outside `next_entity_id`, malformed/unknown entity kind or role, invalid/non-finite geometry and invalid primitive parameters. The same local EntityId in two different SketchModels is valid because durable addressing is scoped by SketchId plus EntityId.

Profile loading fails closed on malformed/non-canonical ProfileId/cursor values, duplicate or out-of-range ProfileIds, missing/invalid source Sketch identity, malformed RegionIntent structure or invalid boundary-anchor encoding. Earlier schema validation remains intact for backward read compatibility.

After the schema-specific parser reconstructs `PartAuthoredState`, loading passes that state through the owning Part-domain validated reconstruction boundary. The same invariant check used by Part transaction commit therefore also guards native-file reconstruction: invalid Sketch support/placement relationships or duplicate hosted Sketch identity cannot produce a live `PartDocument`. Domain reconstruction rejection maps to the existing `malformed_document` load failure family. This validation adds no serialized field and does not change the native schema version.

Container v1 rejects unsafe or ambiguous input, including unsupported container versions, ZIP64, encrypted/unsupported entries, duplicate entry names, unsafe entry paths, missing mandatory entries, oversized content, invalid mandatory JSON and unsupported Part domain schemas.

A `.ss2part` whose manifest declares another Document kind fails closed. `document_id` uses Core's canonical DocumentId serialization; Persistence does not define a second identity representation.

<!-- section-id: internal.persistence.non-goals -->
## Current non-goals

The current persistence layer does not provide Project synchronization/semantic merge, cloud locking, Part Save As / Save Copy As UI, identity-conflict repair, Assembly/Drawing semantic persistence, Sketch constraints/dimensions/solver state, modeled solid geometry persistence, thumbnail generation, tile/icon browsing, or camera/selection persistence between application runs.

The architecture reserves the same native package mechanism for future Assembly and Drawing, but their semantic schemas are not implemented.

Implementation dependencies are vendored and pinned: miniz 3.1.2 for ZIP mechanics and nlohmann/json 3.12.0 for JSON. Normal builds do not download them, and their types do not cross CAD-domain public semantic APIs.
