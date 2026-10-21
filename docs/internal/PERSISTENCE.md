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

The R13-A1 headless/native Part writer uses schema **v21** and retains independent **ModelingSemanticsVersion 2** (`modeling_semantics_version=2`), not modeling semantics v21 or a modeling-v1 migration. It persists document properties, length unit, Origin visibility, hosted Sketches and Profiles, the Sketch-local authored relation records and ID cursors, Project Geometry's strict semantic Edge links, ordered Body/Feature and Axis/Datum intent, and their durable identity cursors. The Document container remains v1; actual native schema and modeling policy are separate version axes.

Each Datum Plane persists stable DatumId, one semantic source, one signed offset in millimetres and authored visibility. Source encoding is provider-neutral: an Origin source stores a built-in XY/XZ/YZ role; a Body source stores explicit `BodyStageRef` plus semantic planar `SurfaceReference`; a Datum source stores only the earlier source DatumId. Derived O/U/V/N frame, runtime provider/topology identity, Viewer token, plane patch and Body-intersection overlay are never authored payload.

Sketches persist stable SketchId, semantic support, visibility, canonical EntityIds, exact authored Line/Circle/Arc seed geometry and Regular/Construction role. The strict v21 `sketches[].model` object has **exactly four keys**: `next_entity_id`, `entities`, `next_constraint_id`, `constraints`; the original v20 model has **exactly two**: `next_entity_id`, `entities`. Ordered local constraint records contain stable canonical `ConstraintId`, typed `EntityId` and `line`/`line_start`/`line_end` references, with six supported explicit Line-only kinds (`horizontal`, `vertical`, `coincident`, `parallel`, `perpendicular`, `fix`). The next-constraint cursor is monotonic even across deletion/Undo branches. Malformed/foreign/cross-Sketch/non-Line targets and invalid kinds fail the complete file load, not silently drop relations. Current Sketch support may be an Origin plane, a provider-neutral planar Body Surface reference or a Datum Plane by DatumId; the evaluated world frame is never serialized. Profiles persist ProfileId, source SketchId, name, semantic RegionIntent and visibility policy (`automatic`, `force_shown`, `force_hidden`).

Each authored Axis persists stable AxisId, name, visibility and one typed semantic source: Sketch Line, strict straight material Body Edge or strict cylindrical Body Face. The evaluated infinite world line is derived and not stored. Origin X/Y/Z remain built-in references, never synthetic Axis records.

The Body persists BodyId, FeatureId cursor and ordered Feature records. Extrude records preserve stable FeatureId, name, suppression, source ProfileId, Add/Cut operation and OneSide/Midplane distance parameters. Revolve records preserve stable FeatureId, name, suppression, source ProfileId, provider-neutral AxisReference, Add/Cut operation and OneSide/Midplane angle parameters; authored angles are stored explicitly in radians.

Fillet and Chamfer records preserve stable FeatureId, name, suppression, a canonical non-empty set of strict MaterialEdgeReference values from one consumed Body stage, and one common positive Radius/Distance in canonical millimetres. Each MaterialEdgeReference stores only BodyStageRef + FeatureCurveAddress + EdgeBranchDiscriminator; the discriminator is SingularAtAuthoredStage or a canonical BetweenSemanticPoints pair. No TopoDS_Edge, runtime Edge token, Viewer token, provider ordinal, XYZ/length/nearest-match identity or provider-history object is serialized.

Evaluated B-Rep, OCCT handles, runtime topology tokens, Axis/Datum/Feature evaluation statuses, previews, provider contour/history and tessellation are not authored payload.

Legacy native schemas remain readable only under their exact schema-specific structural rules and supported modeling policy. The v8→v9 migration validates Origin support against legacy absolute placement; v10 introduced Datum records, v11 Datum-supported Sketches, v12 authored Axes, v13 Revolve, v14 Fillet/Chamfer and strict MaterialEdgeReference, and v15 the canonical `projected_edges` semantic link records. Subsequent v16–v20 versions added bounded material Edge, Datum and Axis source forms while retaining their historical reader contracts. In particular, **v20 requires the exact two-key Sketch model**, with no invented constraints/cursor on disk. Loading v20 does **not** rewrite it; only an explicit successful checkpoint-protected Save writes current **v21** with the exact four-key Sketch model, then an independent cold Reopen must retain authored IDs, cursors, Profile/Feature IDs and supported relations. A numerically contradictory but structurally valid authored constraint system may load; its derived Profile/Feature must be blocked/repair-required rather than presented as an up-to-date B-Rep. Invalid IDs, malformed or extra schema fields, broken references, noncanonical relation records, illegal dependencies or invalid support fail closed through Part reconstruction. Files declaring modeling semantics v1 are rejected as `unsupported_modeling_semantics` rather than automatically upgraded to v2.

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
- ephemeral reference-counted Part-incarnation proof and `PartEvaluation` owner context; solver backend objects, solved coordinates, DoF/residuals and constraint diagnostics;
- evaluated Shared-2D arrangements and Profile regions;
- Part Feature evaluation snapshots and UpToDate/Failed/Blocked/Suppressed diagnostics;
- Datum evaluation status, current O/U/V/N frames, Axis evaluation status/current world lines and transitive Body-stage dependency floors;
- runtime solid/B-Rep handles, Face/Edge/Vertex tokens, evaluated topology catalogs, canonical carrier frames and provider history;
- Extrude/Revolve/Fillet/Chamfer and Datum Plane preview presentation plus temporary source-Profile reveal and transient Revolve source-Axis emphasis;
- Viewer Datum plane patch/border, owner-bound Body-intersection overlay, presentation objects, tessellation, detection/picking tokens and camera state;
- active selection, hover, grips, Dynamic Input/Polar/OSNAP tracking state and CAD input buffer.

A clean reopen recovers authored semantic state without any runtime object. Native R13-A1 cold tests load original v20 two-key files, explicitly Save v21 through the guarded `DocumentSession`/store checkpoint, then reopen through a distinct Part incarnation with stable Sketch/Entity/Constraint/Profile/Feature identity and ID gaps; stage-solved geometry is recomputed from sources, not restored from cached coordinates. The original Owner-supplied v20 regression fixture is publicly permitted, but its exact historical writer executable identity is **DEFERRED/UNPROVEN** under one-blob §5H; a valid v20 ZIP and native cold migration PASS must not be called byte-specific executable provenance. The lifecycle also rebuilds Body-Surface/Datum references, Datum-backed Sketch/Profile support, authored Axes, material Edges and ordered Extrude/Revolve/Fillet/Chamfer with a fresh runtime/provider generation. Part-owned admission checks **DocumentId + DocumentRevision + distinct ephemeral Part incarnation** before a derived `PartEvaluation` can be consumed; identical cold DocumentId/revision does not make a foreign evaluation valid. Provider reentry requires rechecking current ownership before geometry publication.

<!-- section-id: internal.persistence.identity-safety -->
## Identity and container safety

Part discovery reads the common manifest, then Part authored state.

A rename or move inside the Workspace does not change DocumentId. Duplicate native Part files declaring one DocumentId remain a fail-closed identity conflict; no automatic ID rewrite is performed.

Schema-v6 loading rejects malformed/non-canonical identity strings, duplicate EntityIds across primitive kinds, EntityIds outside `next_entity_id`, malformed/unknown entity kind or role, invalid/non-finite geometry and invalid primitive parameters. The same local EntityId in two different SketchModels is valid because durable addressing is scoped by SketchId plus EntityId.

Profile loading fails closed on malformed/non-canonical ProfileId/cursor values, duplicate or out-of-range ProfileIds, missing/invalid source Sketch identity, malformed RegionIntent structure or invalid boundary-anchor encoding. Axis loading validates canonical AxisId/high-water, unique IDs, valid source Sketch/Line identity and authored visibility. Schema-v13 Revolve loading validates ProfileId, AxisReference, operation and extent; a missing but previously allocated AxisId may remain repairable authored intent, while never-allocated IDs fail closed. Schema-v14 Fillet/Chamfer loading validates FeatureId/high-water, parameter, canonical unique MaterialEdgeReference set, shared consumed stage and semantic branch discriminator; Missing/Ambiguous/Unsupported runtime resolution remains repairable authored intent rather than a load-time identity rewrite. Schema-v15 projected Edge binding loading validates target EntityId membership, canonical increasing order, uniqueness and structured semantic source addresses; malformed/cyclic target-source relationships fail reconstruction, while valid historical allocated but no-longer-available source provenance remains repairable. Earlier schema validation remains intact for backward read compatibility.

After the schema-specific parser reconstructs `PartAuthoredState`, loading passes that state through the owning Part-domain validated reconstruction boundary. The same invariant check used by Part transaction commit therefore also guards native-file reconstruction: invalid semantic Sketch support, illegal support/consumer ordering, malformed legacy migration state or duplicate hosted Sketch identity cannot produce a live `PartDocument`. Domain reconstruction rejection maps to the existing `malformed_document` load failure family.

Container v1 rejects unsafe or ambiguous input, including unsupported container versions, ZIP64, encrypted/unsupported entries, duplicate entry names, unsafe entry paths, missing mandatory entries, oversized content, invalid mandatory JSON and unsupported Part domain schemas.

A `.ss2part` whose manifest declares another Document kind fails closed. `document_id` uses Core's canonical DocumentId serialization; Persistence does not define a second identity representation.

<!-- section-id: internal.persistence.non-goals -->
## Current non-goals

Current persistence does not store Undo/Redo history, evaluated B-Rep, Viewer state, runtime topology/provider identity, caches or active edit context.

The native format does not encode Datum Axis/Point, evaluated projection curves or Project Geometry UI state, Body-Edge/Curve Axis constructors, generic authored Edge/Vertex feature inputs, multi-body ownership, Assembly occurrence/constraint data or Drawing semantics. Planar Body-Surface Sketch support is encoded semantically in v9, Datum support in v11, authored Sketch-Line Axis in v12, Revolve in v13 and strict Fillet/Chamfer material-Edge intent in v14, and Part-local Sketch projection source bindings in v15; evaluated topology, runtime repair candidates, provider contour/history and provider identity remain derived.

Save remains whole-file conditional replacement rather than an in-place feature database or event log.

