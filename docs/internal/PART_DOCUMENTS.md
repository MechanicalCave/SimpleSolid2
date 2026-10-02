# Part Documents — As-built

<!-- doc-id: internal.part-documents -->
<!-- document-kind: internal -->

<!-- section-id: internal.part-documents.model -->
## Current model

The current `PartDocument` is persistent and hosts durable Part Sketch objects with embedded Shared 2D Line/Circle/Arc geometry plus Part-owned Profile objects. It still contains no Body, Feature or modeled solid B-Rep.

Its authored state consists of stable `DocumentId`, common Document Properties, the durable Part display/input `LengthUnit`, persistent presentation state for the seven built-in Origin references, an ordered collection of Part-hosted Sketch records, a monotonic `ProfileId` cursor and an ordered collection of Profiles. Length geometry remains canonical in millimetres; changing the display/input unit changes interpretation and presentation only and never rescales existing geometry.

Each Part Sketch has stable `SketchId`, semantic support restricted to XY/XZ/YZ built-in Origin planes, explicit `SketchPlacement`, persistent visibility and one value-owned `sketch::SketchModel`. Shared 2D owns entity identity, Line/Circle/Arc geometry and Regular/Construction role; Part owns host support/placement/visibility/persistence.

Each Profile has stable `ProfileId`, source `SketchId`, authored name/visibility and durable `ProfileRegionIntent`. RegionIntent references source EntityIds and semantic endpoint/intersection anchors. It does not store Viewer tokens, OCCT topology, sampled fill geometry or derived runtime region indices.

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

Current commands cover common Document Properties, built-in reference visibility, Sketch creation, mixed Line/Circle/Arc creation/update/deletion, Regular/Construction role changes, common transforms/duplication and Profile create/edit/properties/delete. Durable mutation remains semantic-command driven; UI/Viewer presentation identity is never mutation authority.

Each `PartDocumentTransaction` captures the technical `DocumentRevision` from which its staged full-state snapshot was created. Commit is authorized only when that base revision still equals the owning `PartDocument` revision. A mismatch returns typed `stale_transaction` before validation, no-op comparison or authored mutation, so an older full-state transaction cannot overwrite a newer accepted mutation.

Part transactions are one-shot. The first commit attempt is terminal whether it succeeds, is a no-op, or fails as stale, invalid-state or revision-exhausted; a later commit returns `inactive_transaction`. Rollback is terminal and idempotent. A fresh no-op creates neither a revision increment nor a history entry.

The Part domain validates the complete staged `PartAuthoredState` at commit: hosted Sketch support/placement must be valid and match, SketchId/ProfileId values must be unique and below their cursors, every Profile must reference an existing source Sketch, and RegionIntent structure must be valid. Invalid full-state replacement returns typed `invalid_state` without changing authored state or revision. The same validator protects `PartDocument::restore`, which returns a structured validated reconstruction result rather than constructing an invalid live document.

`DocumentSession::verifyRevision()` remains a second command/history consistency guard; the owning Part transaction is the domain authority for stale-state rejection.

Undo and Redo reapply authored states through `PartDocumentTransaction` and therefore count as new semantic mutations with increasing technical `DocumentRevision`. Undo/Redo preserve accepted EntityId lineage/high-water rules and restore authored identity rather than Viewer/presentation identity.

<!-- section-id: internal.part-documents.session -->
## DocumentSession

`DocumentSession` is runtime-only and contains the current physical path, loaded `PartDocument`, expected technical revision, Undo/Redo history, the saved authored-state checkpoint and — for a native file opened/created through the Project runtime — a `PartFileCheckpoint`.

Undo/Redo still uses a runtime `std::vector` of two-snapshot history entries containing the authored state before and after each accepted command. Adding a new command no longer deep-copies all older history entries. The session prepares one pending history entry, reserves the required vector capacity and prepares the Sketch EntityId high-water map and ProfileId cursor before the Part transaction mutates the live document. History entries are non-copyable and no-throw movable, so vector relocation transfers ownership rather than copying prior authored snapshots.

A Redo suffix remains logically intact while a command is being prepared. It is destroyed only after a successful changed Part commit, then the already-prepared entry is appended inside reserved capacity and the prepared identity high-waters are published without copying older history. A rejected, failed or no-op command therefore preserves the current Undo/Redo branch. C1 intentionally leaves the `before/after` snapshot representation and history depth policy unchanged; deeper representation or budgeting remains subject to AUDIT-01 C2 measurement.

`needsSave()` compares authored state with the saved authored-state checkpoint. It is not defined by numeric `DocumentRevision` equality, which allows Undo back to the saved semantic state to become clean even though `DocumentRevision` increased.

The native-file checkpoint represents the exact file version established by load/create or the last successful Save. It contains the expected DocumentId, exact byte length, SHA-256 digest and platform file identity. Detached/test sessions may exist without a file checkpoint, but ordinary Save then fails closed rather than performing an unconditional overwrite.

Closing and reopening creates fresh runtime history.

<!-- section-id: internal.part-documents.kernel-evidence -->
## PM-00A transient Kernel evidence boundary

PM-00A Phase A0 adds a provider-neutral evaluation seam for architecture evidence without changing the durable Part schema.

A valid Part-owned Profile can be evaluated into transient `kernel::PlanarProfileInput` containing:

- a complete right-handed support frame `O/U/V/N`;
- exact Line/Circle/Arc source geometry;
- evaluated boundary-use trimming/orientation;
- outer/hole loop structure;
- semantic provenance back to the source Sketch EntityId plus loop/use position.

The neutral Kernel types contain no Qt, Viewer or OCCT/provider identity. The Part adapter depends only on the neutral Kernel surface. It does not depend on `kernel_occt`.

The bounded kernel-native OCCT evidence provider consumes the neutral input and returns neutral evidence such as B-Rep validity, topology counts and provenance-associated generated-edge counts. `TopoDS_*` handles stay private to the provider implementation and are not returned to Part or persisted.

A whole closed Circle is represented semantically as a whole closed curve; a technical provider/parameterization seam is not promoted to semantic authored meaning.

This seam is currently architecture evidence only. It does not add BodyId, FeatureId, Part Feature history, persistent topology references or a user-facing solid operation.

The PM-00A cold-model rebuild regression proves the intended lifecycle boundary:

```text
durable Part/Sketch/Profile state
→ evaluate neutral Profile input
→ build/validate OCCT evidence
→ Save
→ destroy DocumentSession + neutral input + provider/B-Rep runtime state
→ reopen native .ss2part
→ reevaluate neutral Profile input
→ rebuild/validate OCCT evidence
```

The rebuilt neutral input fingerprint and neutral provider evidence must match the pre-close result without previous-process provider handles or caches.

<!-- section-id: internal.part-documents.pm00a-e01-extrude -->
## PM-00A E01 transient Extrude-role evidence

PM-00A E01 extends the transient Kernel evidence surface with provider-neutral Extrude face roles:

- start cap;
- end cap;
- side generated from one exact Profile boundary-use provenance.

These roles are architecture evidence only. They are not persisted Body/Feature topology references and do not activate a product Extrude command.

For the OCCT evidence provider, side lineage is collected at the provider operation boundary. A `TopoDS_Edge` created before insertion into `BRepBuilderAPI_MakeWire` is not assumed to survive as the exact provider basis edge: `MakeWire` may copy/replace an edge while reconciling coincident vertices. The provider therefore retains the exact transient edge accepted by the completed wire builder and uses it when querying the local prism sweep.

That provider edge is runtime evidence only. The semantic meaning remains the Part/Profile boundary-use provenance. No OCCT handle, topology ordinal, geometry similarity or Viewer token is promoted to durable identity.

E01 exact source candidate `476258b74751a251c1c4fdf7dabaa03b3da63976` passed Windows FULL #1228, including cold replay after Save/Close/Reopen and zero false-Resolved outcomes for the accepted E01 matrix.

<!-- section-id: internal.part-documents.pm00a-e05-similarity -->
## PM-00A E05 similarity guardrail evidence

PM-00A E05 adds optional provider-neutral face geometry diagnostics to the transient Extrude evidence surface:

- surface kind;
- area;
- centroid;
- canonical surface axis.

These values exist only to construct and inspect deliberately similar topology during architecture evidence. They are not semantic selectors, are not persisted, and cannot independently produce `Resolved`.

E05 proves the following fail-closed behavior:

- two equal coplanar side faces with the same diagnostic surface class, area and axis remain separate semantic targets because their Profile boundary-use provenance differs;
- erasing one source EntityId and recreating the same geometric Line under a new EntityId does not transfer the old side identity; the old semantic target is `Missing`;
- a surviving semantic side remains `Resolved` through provenance even when another face is geometrically closer to the target's previous centroid;
- with semantic provenance intentionally omitted, multiple geometry-only candidates are `Ambiguous` and a unique geometry-only candidate is `Unsupported`; geometry similarity alone is never sufficient for `Resolved`.

The cold E05 replay reconstructs these outcomes after Save/Close/Reopen without previous-process OCCT handles, provider caches, Viewer tokens or topology ordinals.

The E01 regression was also tightened so semantic stability across an Extrude distance edit compares role/provenance/status/candidate cardinality rather than mutable geometry diagnostics.

E05 exact source candidate `7e0d6109cf927a2e8d85d2794b5718dcc2feab22` passed Windows FULL #1252 with zero false-Resolved outcomes.

This remains architecture evidence only. It does not add a persisted topology-reference schema, Body/Feature persistence, a Part Feature Tree or a product solid-modeling command.

<!-- section-id: internal.part-documents.pm00a-e03-e04-cardinality -->
## PM-00A E03/E04 split/merge cardinality evidence

PM-00A E03/E04 extends the transient Kernel evidence surface with neutral Boolean subshape-history observations:

- Modified count;
- Generated count;
- Deleted flag;
- unchanged-present flag;
- unique descendant count;
- shared-descendant count for bounded merge probes.

These values are runtime evidence only. They are not persisted reference selectors and do not give provider history authority to choose semantic identity.

The accepted fail-closed interpretation is:

```text
0 semantic candidates  -> Missing
1 semantic candidate   -> Resolved
>1 semantic candidates -> Ambiguous
undeclared aggregate   -> Unsupported
```

The evidence proves:

- a real OCCT Cut that splits one source edge into two descendants is Ambiguous for a singular reference;
- a completely removed source edge is Missing;
- independent semantic-role evidence may exclude technical candidates without using order, length or proximity;
- two prior face meanings that collapse onto one provider result remain Ambiguous without an independent semantic winner;
- Modified/Deleted provider-history asymmetry alone cannot select identity;
- independent producer/role meaning may preserve one target as Resolved while a genuinely removed target is Missing;
- aggregate split/merge meanings remain Unsupported unless a future contract explicitly declares such a reference type.

All E03/E04 provider probes are rebuilt after previous operation/history objects leave scope and must reproduce identical neutral history/cardinality evidence.

The synthetic fixture locates its known source subshapes by endpoint/centroid only to construct deterministic test inputs. Those lookup helpers are not topology resolution, not authored identity and not proposed persistent-reference semantics.

E03/E04 exact source candidate `30886e0d47a8f0bbfba5ab9048cf22fb9fa7be25` passed Kernel-focused #1274 and Windows FULL #1275 with zero false-Resolved outcomes.

This remains architecture evidence only. It does not add a durable topology-reference schema, Body/Feature persistence, a product Boolean operation, Part Feature Tree behavior or reference-repair UI.

<!-- section-id: internal.part-documents.pm00a-e02-multistage -->
## PM-00A E02 multi-stage lineage evidence

PM-00A E02 extends the transient Kernel evidence surface with explicit producer-stage reference observations across a bounded Extrude-stage prism, real OCCT Cut and real OCCT Fillet.

The evidence distinguishes:

- Extrude-output reference status;
- Cut-output reference status;
- downstream/Fillet-output reference status;
- provider Modified/Generated/Deleted/unchanged observations between stages;
- downstream operation outcome, including a separate geometric-failure state.

Producer/consumed stage is semantic context. A reference intended for an earlier stage is not resolved by searching only the final Body. When the same semantic meaning is uniquely present at more than one stage, omitting the intended stage is Ambiguous rather than permission to select a final-shape match.

For downstream Fillet input, candidate search is bounded by the already-resolved semantic Cut face. A first probe that searched the complete Cut result found multiple endpoint-compatible edges and failed. The accepted evidence does not choose a first, nearest or otherwise geometry-preferred candidate; it narrows the search by semantic context and then requires cardinality one.

Reference resolution and geometric feasibility remain separate. In the thin-geometry E02 scenario the Cut-stage reference remains Resolved and its Fillet input edge remains unique, while the unchanged Fillet radius fails geometrically. That geometric failure does not rewrite the input reference as Missing or Ambiguous.

The initial E02 “Extrude stage” is a deterministic one-solid prism fixture built for multi-stage topology evidence. It is not the production Extrude implementation. E01 remains the exact Profile-to-Extrude provenance evidence.

Known centroid/endpoint helpers are used only to construct deterministic provider fixtures. They are not authored identity, persistent selectors or automatic geometry-similarity resolution.

All E02 provider/B-Rep/history objects are transient. The evidence set is rebuilt after first-pass operation objects leave scope and must reproduce identical neutral outcomes.

E02 exact source candidate `953cdca42978916754f6ae6cc68d86352aad35ac` passed Kernel-focused #1285 and Windows FULL #1286 with zero false-Resolved outcomes.

This remains architecture evidence only. It does not add a durable topology-reference schema, Body/Feature persistence, Part Feature Tree behavior or product Extrude/Cut/Fillet commands.

<!-- section-id: internal.part-documents.pm00a-e07-cold-replay -->
## PM-00A E07 accumulated cold-rebuild parity

PM-00A E07 replays accepted topology-reference outcomes after previous runtime/provider state is destroyed.

Phase 1 covers E07-01 through E07-05. No new duplicate harness is added because the owning E01/E02/E03/E04/E05 regressions already perform the required cold boundary. Windows FULL #1286 runs those regressions together on one exact source candidate.

The accumulated kernel-native result on `953cdca42978916754f6ae6cc68d86352aad35ac` is 23/23 PASS, including:

- E01 authored Profile Save/Close/Reopen with stable Extrude cap/side semantic roles;
- E02 provider/B-Rep/history teardown followed by the same stage-scoped multi-stage outcomes;
- E03 split replay remaining Ambiguous;
- E04 merge/lost-distinction replay remaining Ambiguous/Missing according to the accepted source-row meaning;
- E05 authored reopen preserving Missing for a removed target despite a geometry-similar/identical decoy.

Cold rebuild therefore does not depend on previous-process OCCT handles, provider ordering or history caches, and it does not repair Missing/Ambiguous references by geometry similarity.

E07-06 is still pending because it depends on E06 full-Revolve / periodic-seam evidence. The whole E07 package is not considered completed until the revolved semantic side also reconstructs as Resolved while the provider seam remains non-semantic.

This is evidence synthesis only. It does not introduce a persistent topology-reference schema, Body/Feature persistence or product solid-modeling behavior.

<!-- section-id: internal.part-documents.persistence -->
## Native Part persistence

The native extension is `.ss2part`.

The current Part domain writer uses schema **v7**. It stores document properties, the durable display/input length unit, built-in Origin visibility, hosted Sketch records, canonical `next_profile_id` and authored Profiles.

Each Sketch stores stable SketchId, Origin-plane support, explicit placement, visibility and one embedded Shared 2D model. The model stores canonical `next_entity_id` plus mixed Line/Circle/Arc `entities[]`. Every entity stores its authored `regular` or `construction` role.

Each Profile stores canonical ProfileId, source SketchId, authored name/visibility and the semantic RegionIntent loop/anchor structure. Derived region indices, sampled presentation geometry, Viewer tokens and OCCT handles are not serialized.

Schemas v1–v6 remain readable. Older schemas have no length-unit field and therefore migrate in memory to **mm** without rescaling any geometry. Opening an older schema does not rewrite the file; a later successful ordinary Save publishes current schema v7 with the selected length unit.

ProjectId, DocumentSession, Undo/Redo, active Sketch/Profile tool context, Polar/Dynamic Input runtime configuration, request-local numeric locks, region-analysis cache, camera, active selection, Qt objects, Viewer objects and OCCT handles are not serialized as authored Part state.

Ordinary Save remains conditional on the session's native-file checkpoint. Save-conflict rules and whole-file atomic publication are unchanged.

<!-- section-id: internal.part-documents.discovery -->
## Discovery and canonical sessions

ProjectSession owns a rebuildable Workspace index.

A DocumentId resolves only when exactly one valid native file in the Workspace declares it. Duplicate physical files with the same DocumentId form `IdentityConflict` and include all discovered relative paths.

When a resolved Part is already open, a second Open request returns the existing canonical DocumentSession rather than creating a second mutable session for the same DocumentId.

<!-- section-id: internal.part-documents.profiles -->
## Profile semantics and lifecycle

Profile creation/editing is a Part operation over derived Shared-2D regions. The Profile tool caches region analysis for the current Sketch model state, performs hover/pick against that cache and keeps Add/Subtract composition runtime-only. Repeated pointer motion on unchanged geometry does not rebuild the full arrangement.

Nested/disconnected island analysis is part of that complete derived region truth and remains active for every Profile session. The runtime **Show Islands** option controls presentation/diagnostic visibility only; it cannot change RegionCandidate truth, Profile validity, point picking, RegionIntent or authored state. The former public Find All Regions action is no longer part of the normal Profile workflow; internal region enumeration remains derived analysis.

Finish executes one semantic Profile command. Create allocates one fresh ProfileId. Edit preserves the existing ProfileId and atomically replaces RegionIntent. Cancel, hover, diagnostic lookup and rejected drafts do not mutate authored state or consume identity.

Profiles are live references rather than geometry snapshots. `evaluateProfile` resolves durable RegionIntent against current source Sketch geometry. Missing source entities/intersections or ambiguous/unresolved topology make the Profile Invalid without rewriting intent; later source repair can return the same ProfileId to Valid.

Delete Profile removes only the Profile. Source Sketch geometry remains authored. Source Sketch removal cannot silently strand dependent Profiles. UI command targeting is runtime-only: while a Sketch is actively edited, its semantic selection owns Sketch Delete and stale Tree/Profile presentation cannot become mutation authority.

<!-- section-id: internal.part-documents.sketch-presentation -->
## Active Sketch presentation and spatial input

While a Part Sketch is actively edited, `PartViewportController` rebuilds a neutral authored Sketch scene from the current embedded `SketchModel`, maps Line/Circle/Arc geometry from Sketch U/V through `SketchPlacement` into 3D, presents the intrinsic Sketch Origin as a non-authored overlay and keeps runtime presentation-token bindings back to `SketchId + EntityId`.

A separate preview scene is transient and independently replaceable/clearable. Neutral provider rays are intersected with the active Sketch frame to produce Sketch-local U/V. Selection, hover, grips, preview, pointer candidates and Command Line input state remain runtime-only and do not change Part revision, dirty state, history or persistence until a semantic command commits.

Presentation tokens are ephemeral. Scene rebuild/history may allocate different tokens for the same authored EntityId; tests and runtime logic must therefore reacquire presentation bindings after a rebuild rather than treating Viewer tokens as durable identity.

If the active Sketch disappears through Undo/history or the editing context is replaced, presentation/input state fails closed and is cleared.

<!-- section-id: internal.part-documents.current-limits -->
## Current limits

The Part model durably owns hosted Shared 2D Line/Circle/Arc entities, a display/input length unit and Part-owned Profiles. The active Sketch editor supports semantic point/Window/Crossing selection, mixed Delete, Line/Circle/Arc/Rectangle creation, Regular/Construction role changes, Profile Create/Edit with Add/Subtract, Move/Copy/Rotate/Scale/Mirror, the full supported grip edit cycle, Grip Copy in Reshape/Move, Repeat Last Command and the R10 shared precision-input path.

Length input accepts mm/cm/m/in/ft with canonical millimetres, dimensional arithmetic and absolute/relative Cartesian or polar point syntax. Polar and Dynamic Input are application-session runtime aids; request-local numeric locks remain transient. Exact Rotate/Scale and supported grip Rotate/Scale/Mirror numeric input reuse the same semantic request pipeline.

Presentation tokens, preview, pointer input, Command Line/Dynamic Input live token, Polar capture, numeric locks, hover/grip state and camera remain runtime-only and are not Part/Sketch identity.

The product still does not implement authored constraints/dimensions/solver, OSNAP/tracking/inference, Grid Snap, Rotate/Scale/Mirror+Copy, ordinary-Select RMB context, clipboard/cross-Sketch Copy, Datum/Construction Plane support, planar model-face Sketch support, Body/Feature modeled solid geometry, persistent topology naming or Material.

The Viewer is not a second model: no OCCT object or Viewer token is durable Part/Sketch identity, support or authored state.

