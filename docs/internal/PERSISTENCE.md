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

A native top-level Part is stored as `*.ss2part` in native Document container v1:

```text
Part001.ss2part
├── manifest.json
├── authored/
│   └── document.json
└── derived/
    └── ... optional disposable assets
```

The common manifest contains format/container metadata, Document kind/identity and Part domain schema version. Shared persistence owns package safety; Part owns engineering meaning in `authored/document.json`.

The current Part writer is schema **v11**. It persists document properties, display/input length unit, Origin visibility, hosted Sketches, Profiles, modeling-semantics version 1, one Body with ordered Features, the DatumId high-water cursor and ordered Offset Datum Plane records.

Each Datum Plane persists stable DatumId, one semantic source, one signed offset in millimetres and authored visibility. Source encoding is provider-neutral: an Origin source stores a built-in XY/XZ/YZ role; a Body source stores explicit `BodyStageRef` plus semantic planar `SurfaceReference`; a Datum source stores only the earlier source DatumId. Derived O/U/V/N frame, runtime provider/topology identity, Viewer token, plane patch and Body-intersection overlay are never authored payload.

Sketches persist stable SketchId, semantic support, visibility, canonical EntityIds, exact Line/Circle/Arc geometry and authored Regular/Construction role. Current support may be a built-in Origin plane, a provider-neutral planar Body Surface reference, or a Datum Plane by DatumId. The evaluated world frame is derived and is not persisted. Schema v11 does not write a separate world placement for Datum-backed Sketch support. Profiles persist ProfileId, source SketchId, name, semantic RegionIntent and visibility policy (`automatic`, `force_shown`, `force_hidden`).

The Body persists BodyId, FeatureId cursor and ordered Feature records. The current Extrude Feature record preserves stable FeatureId, name, suppression, source ProfileId, Add/Cut operation and OneSide/Midplane parameters. Evaluated B-Rep, OCCT handles, runtime topology tokens, Datum/Feature evaluation statuses, previews and tessellation are not authored payload.

Legacy schemas remain readable according to their existing migration rules. The v8→v9 migration validates legacy Origin support against legacy absolute Sketch placement and removes redundant placement on a later successful Save. Loading schema v9 preserves existing Document/Body/Sketch identity, creates no synthetic Datum records and starts the DatumId cursor at 1. Schema v10 introduced durable Datum records; schema v11 adds DatumId Sketch support. A later successful Save publishes current schema v11. Invalid IDs, malformed references, illegal Datum dependency/cycle state or invalid Sketch support fail closed through Part-domain reconstruction.

DocumentId remains in the common manifest. ProjectId, DocumentRevision, Undo/Redo, active tools, CAD input buffer, selection, camera, current Datum frames and all provider/runtime geometry remain non-persistent.

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

The following are derived/runtime and intentionally disposable:

- `ProjectSession`, `DocumentSession` and Undo/Redo history;
- `DocumentRevision`, runtime session/request/draft generations and file checkpoints;
- evaluated Shared-2D arrangements and Profile regions;
- Part Feature evaluation snapshots and UpToDate/Failed/Blocked/Suppressed diagnostics;
- Datum evaluation status, current O/U/V/N frames and transitive Body-stage dependency floors;
- runtime solid/B-Rep handles, Face/Edge/Vertex tokens, evaluated topology catalogs, canonical carrier frames and provider history;
- Extrude and Datum Plane preview presentation plus temporary source-Profile reveal;
- Viewer Datum plane patch/border, owner-bound Body-intersection overlay, presentation objects, tessellation, detection/picking tokens and camera state;
- active selection, hover, grips, Dynamic Input/Polar/OSNAP tracking state and CAD input buffer.

A clean reopen must recover authored semantic state without any of these objects. PM-03 lifecycle coverage saves through the guarded `DocumentSession` path, destroys the loaded session/provider state, reloads schema-v11 authored data and rebuilds Body-Surface/Datum references, Datum-backed Sketch/Profile support and ordered Add/Cut features with a deliberately different runtime token generation. Runtime token values may change; DatumId, semantic source, signed Offset, visibility, Sketch support and authored IDs must not.

<!-- section-id: internal.persistence.identity-safety -->
## Identity and container safety

Part discovery reads the common manifest, then Part authored state.

A rename or move inside the Workspace does not change DocumentId. Duplicate native Part files declaring one DocumentId remain a fail-closed identity conflict; no automatic ID rewrite is performed.

Schema-v6 loading rejects malformed/non-canonical identity strings, duplicate EntityIds across primitive kinds, EntityIds outside `next_entity_id`, malformed/unknown entity kind or role, invalid/non-finite geometry and invalid primitive parameters. The same local EntityId in two different SketchModels is valid because durable addressing is scoped by SketchId plus EntityId.

Profile loading fails closed on malformed/non-canonical ProfileId/cursor values, duplicate or out-of-range ProfileIds, missing/invalid source Sketch identity, malformed RegionIntent structure or invalid boundary-anchor encoding. Earlier schema validation remains intact for backward read compatibility.

After the schema-specific parser reconstructs `PartAuthoredState`, loading passes that state through the owning Part-domain validated reconstruction boundary. The same invariant check used by Part transaction commit therefore also guards native-file reconstruction: invalid semantic Sketch support, illegal support/consumer ordering, malformed legacy migration state or duplicate hosted Sketch identity cannot produce a live `PartDocument`. Domain reconstruction rejection maps to the existing `malformed_document` load failure family.

Container v1 rejects unsafe or ambiguous input, including unsupported container versions, ZIP64, encrypted/unsupported entries, duplicate entry names, unsafe entry paths, missing mandatory entries, oversized content, invalid mandatory JSON and unsupported Part domain schemas.

A `.ss2part` whose manifest declares another Document kind fails closed. `document_id` uses Core's canonical DocumentId serialization; Persistence does not define a second identity representation.

<!-- section-id: internal.persistence.non-goals -->
## Current non-goals

Current persistence does not store Undo/Redo history, evaluated B-Rep, Viewer state, runtime topology/provider identity, caches or active edit context.

The native format does not encode Datum-plane support, Projection, generic authored Edge/Vertex feature inputs, multi-body ownership, Assembly occurrence/constraint data or Drawing semantics. Planar Body-Surface Sketch support is encoded semantically in v9; evaluated topology, runtime repair candidates and provider identity remain derived.

Save remains whole-file conditional replacement rather than an in-place feature database or event log.

