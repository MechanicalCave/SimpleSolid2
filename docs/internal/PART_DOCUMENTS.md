# Part Documents — As-built

<!-- doc-id: internal.part-documents -->
<!-- document-kind: internal -->

<!-- section-id: internal.part-documents.model -->
## Current model

The current `PartDocument` is persistent and owns Part-hosted Sketches, Part-owned Profiles, Part-owned Sketch-Line Axes, Part-owned Datum Planes and exactly one durable Part Body. The Body has stable `BodyId`, a monotonic `FeatureId` cursor and an ordered collection of authored Features.

The authored state contains stable `DocumentId`, common Document Properties, display/input `LengthUnit`, persistent built-in Origin visibility, Sketches, Profiles, a monotonic `AxisId` cursor with authored Axis records, a monotonic `DatumId` cursor with ordered Offset Datum Plane records, `ModelingSemanticsVersion`, Body identity/cursor and ordered Features. Canonical geometric length remains millimetres; display/input unit changes do not rescale authored geometry.

Each Offset Datum Plane has stable `DatumId`, one semantic `PlaneReference`, one signed authored Offset and authored visibility. The current constructor is Offset only. A PlaneReference may name a built-in XY/XZ/YZ Origin plane, a planar Body `SurfaceReference` at an explicit `BodyStageRef`, or an earlier Datum Plane by `DatumId`. No world frame, provider object or Viewer token is authored as Datum identity.

Each Part Sketch has stable `SketchId`, persistent visibility and one value-owned Shared-2D `SketchModel`. Durable support may be a built-in XY/XZ/YZ Origin plane, one semantic planar Body `SurfaceReference`, or one Datum Plane by `DatumId`. For resolvable support, world-space placement is derived from the current support frame. Datum-backed Sketches persist only DatumId support; their world frame remains derived. Authored local U/V geometry, SketchId and EntityIds survive support changes.

Each Profile has stable `ProfileId`, source `SketchId`, authored name, durable `ProfileRegionIntent` and authored visibility policy: `automatic`, `force_shown` or `force_hidden`. RegionIntent references source EntityIds and semantic anchors; it does not store Viewer tokens, OCCT topology or sampled fill geometry.

Each authored Axis has stable `AxisId`, authored name, independent persistent visibility and one semantic source `SketchId + EntityId` that must identify a non-degenerate Line. A Line's Shared-2D geometry role (`Regular | Construction`) is orthogonal to its Part designation (`Axis | none`): both Line roles are admissible and changing role never creates, deletes or re-identifies an Axis. New Create/Re-source authoring enforces one exact source Line per authored Axis owner; pre-amendment duplicate-source records remain loadable without identity rewrite and are exposed as an explicit conflict until repaired. Axis evaluation derives an infinite world line from the current source-Sketch support frame and authored Line endpoint order. Origin X/Y/Z remain built-in semantic axes and never receive synthetic AxisId records.

The current durable solid Feature definitions are Extrude and Revolve. Extrude references one ProfileId and authors Add/Cut plus either OneSide distance with Forward/Reverse meaning or Midplane total distance. Revolve references one ProfileId and one `AxisReference` (built-in Origin X/Y/Z or authored AxisId), authors Add/Cut, and stores either OneSide Angle with Reverse or Midplane total Angle. The accepted Revolve range is `0 < Angle <= 2*pi` (360 degrees). A `PartFeature` also owns stable `FeatureId`, name and authored Suppressed state.

Evaluated solid geometry and Datum frames are derived. Ordered Part evaluation produces Body status `Empty`, `UpToDate` or `Unavailable`; Features report `UpToDate`, `Failed`, `Blocked` or `Suppressed`; Datum evaluation reports structured Resolved/Missing/Ambiguous/Unsupported/Blocked outcomes. A failed Datum has no stale last-good frame. Runtime B-Rep/provider handles are disposable and are rebuilt from authored state.

`DocumentRevision` remains a technical monotonic freshness counter for successful semantic mutations within the loaded lifecycle.

<!-- section-id: internal.part-documents.projection-bindings -->
## Persisted Sketch projection bindings (headless infrastructure)

Part owns `PartSketch::projection_bindings`: a canonical, unique and ordered mapping of existing Sketch `EntityId` targets to semantic `MaterialEdgeReference` sources (producer/BodyStage/curve/branch). The source is Part-local durable engineering intent. Linked authored Line/Circle/Arc parameters are **seed state only** and must not be mistaken for current evaluated geometry; provider tokens and B-Rep topology ordinals are absent from persisted bindings. On reconstruction, Part rejects duplicate or missing target EntityIds, invalid/unallocated source provenance and direct forward/cyclic Profile-consuming references. Missing *historical* yet allocated source Features can remain repairable intent.

The standalone `PartDocument::evaluateProfile` remains a safe authored-only query: it refuses a Profile directly referencing linked targets or their intersection anchors instead of interpreting persisted seed geometry as current truth. In contrast, ordered Feature evaluation materializes a disposable **effective Sketch** from the exact same-revision upstream Body stage and supplies it to both Profile region resolution and exact Extrude/Revolve input. Headless Project Edge creation, Break Link and draft-preview protection are internal PG-01B capabilities; the user-facing right-panel Project Geometry tool and Face-boundary capture are not shipped.

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
Qt / Command Line / caller
→ semantic DocumentSession command
→ current-context + revision validation
→ PartDocumentTransaction staged state
→ atomic Part-domain commit
→ derived evaluation / presentation refresh
```

Current commands cover Document Properties, Origin visibility, Sketch/Shared-2D mutations, Profile lifecycle, authored Axis Create/Edit/Delete/visibility and solid Feature lifecycle. The bounded Line+Axis path stages the new Line and its fresh Part Axis in one Part-authored state commit, so success creates one Undo entry and any Line/Axis validation or allocation failure leaves both objects absent. Existing-Line Axis OFF→ON and ON→OFF use the same semantic Create/Delete boundaries, while Edit/Re-source preserves AxisId. Extrude and Revolve Create/Edit commit through their revision-bound draft/evaluation paths; Feature Suppress/Unsuppress and Delete use shared semantic commands. UI rows, Viewer objects and preview handles are never mutation authority.

Each `PartDocumentTransaction` captures the `DocumentRevision` from which its staged full-state snapshot was created. Commit is authorized only when that base revision still equals the owning document revision. Stale state fails before authored mutation.

The Part domain validates the complete staged `PartAuthoredState`: Sketch/Profile identity and references, modeling-semantics version, Body/Feature identity cursors, Feature structural validity and legal Profile references must remain coherent. Invalid reconstruction fails closed.

Undo and Redo reapply authored snapshots through the same transaction boundary and therefore create new technical revisions while restoring durable IDs and authored Feature definitions. Preview, hover, rejected Finish and Cancel create no authored mutation or Undo entry.

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

<!-- section-id: internal.part-documents.pm00a-e06-revolve -->
## PM-00A E06 full-Revolve periodic-seam evidence

PM-00A E06 adds transient full-Revolve evidence without activating a product Revolve feature.

The bounded OCCT provider consumes a valid neutral Profile plus an evidence-only local 2D axis origin/direction and performs a full 360-degree revolve. The axis input is not a durable Axis/Datum definition.

For every Profile boundary use, the provider records the uniquely generated face when cardinality is one and keeps the original boundary-use provenance as the semantic meaning. Provider topology handles remain transient.

Full rotation produces real periodic/seam topology. Seam edges are detected as provider diagnostics, including count and total length, but they have no authored source record and therefore remain `Unsupported` as semantic references. A seam, face ordinal or traversal position cannot become durable Part identity.

The accepted E06 dimension edit changes Profile height and outer radius. Provider seam geometry changes, while the semantic outer revolved side remains `Resolved` with the same source provenance and candidate cardinality one.

Cold replay destroys the first provider sweep/B-Rep/TopoDS state and reconstructs the same legal Profile + evidence-axis input. The semantic revolved side reproduces as `Resolved`; the periodic seam remains non-semantic. This supplies E07-06.

E06 exact source candidate `44eda36ba6bdc19d8c252831937fb86f8afabde7` passed Kernel-focused #1305 and Windows FULL #1306 with zero false-Resolved outcomes.

This remains architecture evidence only. It does not add a durable topology-reference schema, Axis/Datum persistence, Body/Feature persistence, Part Feature Tree behavior or a product Revolve command.

<!-- section-id: internal.part-documents.pm00a-e07-cold-replay -->
## PM-00A E07 accumulated cold-rebuild parity

PM-00A E07 replays accepted topology-reference outcomes after previous runtime/provider state is destroyed.

The complete E07-01 through E07-06 matrix is now **COMPLETED — PASS**.

The accumulated kernel-native result on exact source candidate `44eda36ba6bdc19d8c252831937fb86f8afabde7` is **24/24 PASS** in Windows FULL #1306, including:

- E01 authored Profile Save/Close/Reopen with stable Extrude cap/side semantic roles;
- E02 provider/B-Rep/history teardown followed by the same stage-scoped multi-stage outcomes;
- E03 split replay remaining Ambiguous;
- E04 merge/lost-distinction replay remaining Ambiguous/Missing according to the accepted source-row meaning;
- E05 authored reopen preserving Missing for a removed target despite a geometry-similar/identical decoy;
- E06 full-Revolve replay preserving the semantic revolved side as Resolved while provider periodic seam topology remains Unsupported.

Cold rebuild therefore does not depend on previous-process OCCT handles, provider ordering or history caches. It also does not repair Missing/Ambiguous references by geometry similarity or promote provider periodic seams to semantic identity.

No duplicate E07 execution framework is introduced: the owning regressions remain the executable source of the cold boundary.

This is evidence synthesis only. It does not introduce a persistent topology-reference schema, Body/Feature persistence or product solid-modeling behavior.

<!-- section-id: internal.part-documents.persistence -->
## Native Part persistence

The native extension is `.ss2part`.

The current Part domain writer uses schema **v13**. In addition to document properties, length unit, built-in Origin visibility, Sketches, Profiles, modeling-semantics version 1, Body identity/cursor and ordered Features, it persists the AxisId high-water cursor with authored Sketch-Line Axes and the DatumId high-water cursor with ordered Offset Datum Plane records.

Each Datum record persists stable DatumId, semantic source, signed offset in millimetres and authored visibility. Origin sources store the built-in plane role; Body sources store the provider-neutral Body stage plus semantic Surface address/provenance; Datum-to-Datum sources store only the source DatumId. Derived O/U/V/N frames, runtime topology/provider tokens and Viewer presentation identity are never serialized.

Each Sketch stores stable SketchId, semantic support, visibility and one embedded Shared-2D model with canonical entity IDs and authored Regular/Construction role. Origin support stores its built-in plane role. Body support stores `BodyStageRef` plus semantic `SurfaceReference`. Datum support stores only `DatumId`. No independent authored world `SketchPlacement` is written for current semantic support.

Each Profile stores canonical ProfileId, source SketchId, name, visibility policy and semantic RegionIntent. Automatic/forced Profile presentation is authored policy; evaluated region geometry remains derived.

Each authored Axis record stores stable AxisId, name, source SketchId + Line EntityId and authored visibility. It stores no world origin/direction, provider edge or Viewer token. Revolve AxisReference stores either the built-in Origin-axis role or authored AxisId; an already-allocated but currently missing AxisId remains valid repairable authored intent.

The Body stores stable BodyId, `next_feature_id` and ordered Features. Extrude records preserve FeatureId, name, suppression, source ProfileId, Add/Cut operation and OneSide/Midplane distance parameters. Revolve records preserve FeatureId, name, suppression, source ProfileId, AxisReference, Add/Cut operation and OneSide/Midplane angle parameters with explicit radians. B-Rep, provider handles, runtime topology tokens, cached Feature/Datum/Axis evaluations, preview geometry and tessellation are not serialized.

Legacy schemas remain readable according to their migration rules. The v8→v9 migration validates legacy Origin support against legacy absolute Sketch placement before dropping redundant placement. Loading schema v9 introduces no synthetic Datum records and preserves existing Document/Body/Sketch identities. Schema v10 introduced durable Datum records; v11 added DatumId Sketch support; v12 introduced the AxisId cursor and authored Axis records without creating synthetic Origin axes; v13 adds Revolve Feature records. A later successful Save emits the current v13 schema. Malformed support, Axis/Revolve identity/reference state or dependency state fails closed through Part-owned reconstruction.

ProjectId, DocumentSession, Undo/Redo, active tools, preview, selection, camera, evaluated solid handles, Datum evaluation frames, provider state and Viewer tokens remain runtime-only.

Ordinary Save remains conditional on the session's native-file checkpoint. Save-conflict rules and whole-file atomic publication are unchanged. Current lifecycle coverage saves through `DocumentSession`, destroys the loaded session/provider state, reopens authored v13 data and rebuilds Datum-backed support, Profiles, authored Axes and ordered Extrude/Revolve semantics with a fresh provider/runtime generation.

<!-- section-id: internal.part-documents.discovery -->
## Discovery and canonical sessions

ProjectSession owns a rebuildable Workspace index.

A DocumentId resolves only when exactly one valid native file in the Workspace declares it. Duplicate physical files with the same DocumentId form `IdentityConflict` and include all discovered relative paths.

When a resolved Part is already open, a second Open request returns the existing canonical DocumentSession rather than creating a second mutable session for the same DocumentId.

<!-- section-id: internal.part-documents.profiles -->
## Profile semantics and lifecycle

Profile creation/editing remains a Part operation over exact derived Shared-2D regions. Finish executes one semantic Profile command: Create allocates a fresh ProfileId; Edit preserves ProfileId and atomically replaces RegionIntent. Cancel, hover and rejected drafts do not mutate authored state.

Profiles are live references rather than geometry snapshots. `evaluateProfile` resolves durable RegionIntent against current source Sketch geometry. Missing or ambiguous source meaning makes the Profile Invalid without rewriting intent; later source repair can return the same ProfileId to Valid.

Profile visibility is now an authored policy. `automatic` shows an otherwise valid Profile when it is not consumed by an active/non-suppressed Feature and hides it when it is consumed. `force_shown` and `force_hidden` override that presentation policy without changing modeling evaluation. Feature edit may transiently reveal its source Profile; that reveal is runtime-only.

Feature/Profile relationships do not change ownership. A Profile remains under its source Sketch. Feature Properties identify source Profile/Sketch, Profile Properties enumerate consuming Features, and navigation moves selection between related semantic objects.

Delete Profile removes only the Profile and leaves source Sketch geometry authored. A Feature that references a missing/unresolved Profile remains authored and evaluates Failed/Blocked according to its stage; the system does not silently rebind it to similar geometry.

<!-- section-id: internal.part-documents.sketch-presentation -->
## Active Sketch presentation and spatial input

While a Part Sketch is actively edited, `PartViewportController` rebuilds a neutral authored Sketch scene from the current embedded `SketchModel`, resolves the current support frame and maps Line/Circle/Arc geometry from local Sketch U/V into 3D. Origin-backed Sketches use the canonical built-in frame; Body-Surface-backed Sketches resolve their declared upstream stage and semantic planar Surface from the same-revision stage topology catalogs retained by the current evaluation. Later Features do not force edit presentation to use final-stage topology. The intrinsic Sketch Origin is presentation-only and runtime presentation-token bindings map back to `SketchId + EntityId`.

A separate preview scene is transient and independently replaceable/clearable. Neutral provider rays are intersected with the active Sketch frame to produce Sketch-local U/V. Selection, hover, grips, preview, pointer candidates and Command Line input state remain runtime-only and do not change Part revision, dirty state, history or persistence until a semantic command commits.

Presentation tokens are ephemeral. Scene rebuild/history may allocate different tokens for the same authored EntityId; tests and runtime logic must therefore reacquire presentation bindings after a rebuild rather than treating Viewer tokens as durable identity.

If the active Sketch disappears through Undo/history or the editing context is replaced, presentation/input state fails closed and is cleared.

<!-- section-id: internal.part-documents.accepted-part-feature-boundary -->
## As-built Part Feature and Extrude boundary

ADR-0014 and ADR-0015 define and are implemented by the current Part Feature/Extrude boundary.

Part v1 currently owns one durable Body with ordered Features. Extrude is a production Feature family with Add/Cut and OneSide/Midplane; Revolve is also production and is described in the Axis/Revolve section below. The first successful solid-producing Feature in an Empty Body must be Add; later Features may be Add or Cut. Every successful evaluated stage remains exactly one valid solid.

OneSide distance runs from the Profile support plane and may be Forward or Reverse. Midplane distance is the total symmetric length and does not author Reverse. Semantic cap/side meaning is stage/role/provenance based; provider topology order is not identity.

Already-authored Features retain identity and inputs when Failed, Blocked or Suppressed. Downstream evaluation never consumes stale last-good B-Rep as current truth. Suppress preserves the Feature and removes its contribution; Delete removes that authored Feature while leaving source Profile and remaining Features authored. Both are Undoable.

Extrude Create/Edit uses one runtime draft shared by GUI and Command Line. Valid parameter changes update derived preview; Finish revalidates document revision, draft generation, source Profile and successful evaluation before one authored transaction. Edit preserves FeatureId. Cancel or stale/rejected Finish commits nothing.

The Viewer receives provider-neutral derived committed Body and preview presentation. The current Body scene includes generation-scoped Face/Edge/Vertex presentation records derived from the same current `RuntimeSolid` evaluation generation; direct picking maps transient presentation/runtime tokens immediately back to the Part semantic topology catalog. No Viewer/provider token becomes durable Part identity.

<!-- section-id: internal.part-documents.axis-revolve -->
## As-built Axis and Revolve boundary

PM-04 implements Part-owned Sketch-Line Axis and the Revolve Feature family without changing Shared-2D ownership. An authored Axis is a Part object whose durable source is `SketchId + Line EntityId`; its evaluated origin/direction is derived from the current source-Sketch world frame and Line endpoint order. Axis source may be Regular or Construction. Visibility is authored independently from the source Sketch and never affects evaluation.

`AxisReference` has exactly two current variants: built-in Origin X/Y/Z or authored AxisId. Built-in axes remain deterministic Origin references with no synthetic AxisId. An authored Axis inherits the Body-stage dependency floor of its source Sketch support. Revolve must be downstream of every stage required by both its Profile and Axis source chains; cycle-causing or forward-stage state is rejected through the existing bounded Part validation rather than a global dependency graph.

Axis Edit may re-source the existing Axis to another admissible Line while preserving AxisId and authored visibility. Missing/invalid source Line leaves the Axis Missing/Unsupported; unavailable source-Sketch support leaves it Blocked. Delete Axis is allowed while referenced: the Axis object disappears, but each Revolve retains its authored AxisId reference and becomes repairably Missing. Undo restores the same AxisId. A later fresh Axis allocation never aliases the deleted identity.

Revolve consumes one Profile and one explicit AxisReference. It supports Add/Cut, OneSide/Midplane and `0 < Angle <= 360 degrees`; OneSide may author Reverse, while Midplane treats Angle as total symmetric sweep and has no Reverse state. No default Axis is inferred. The Axis must be coplanar with the Profile and material must remain on one half-plane; boundary contact/on-axis segments are legal while interior crossing is rejected.

Revolve uses the same single-Body ordered-stage rules as Extrude. First successful solid production must be Add; detached Add, no-effect Add/Cut, remove-all Cut and multi-solid results fail without committing. Edit preserves FeatureId and may change Profile, AxisReference, operation or extent parameters. Suppress/Delete/Undo/Redo use the common Feature lifecycle.

Failure never authorizes stale last-good modeling. Missing Profile, Missing Axis, AxisUnavailable, invalid coplanarity/crossing or upstream support failure keeps authored intent and publishes no current successful Body result for the failing stage. Explicit repair through Profile/source geometry edit, Axis re-source, Undo or Sketch support edit recomputes from current semantics.

Full 360-degree periodic provider seams remain representation artifacts unless separate semantic provenance makes an engineering boundary. Provider seam identity, traversal order, proximity and geometry similarity never become durable Edge identity.

Create/Edit Revolve uses one application-owned runtime draft for GUI, Operations and Command Line. Preview resolves the exact Profile/Axis input and publishes only the exact added/removed material as a transient presentation mesh; the complete candidate Body remains Finish authority. The source Profile and Axis may receive transient visibility/emphasis overrides for spatial clarity, but those overrides never author visibility. Finish revalidates current document/draft/evaluation and commits one transaction; Cancel/stale/invalid input commits nothing.

Schema v13 persists Revolve intent and cold rebuild reconstructs it with fresh provider tokens. Runtime solids, evaluated Axis lines, topology tokens and preview presentation remain disposable.

<!-- section-id: internal.part-documents.accepted-next-topology-boundary -->
## As-built semantic topology and face-supported Sketch boundary

PM-02 is implemented for the current Extrude Add/Cut universe. The Part evaluator publishes a complete disposable topology catalog for every successful Body Feature stage, with current Face/Edge/Vertex realization separated from semantic Surface/Curve/Point carrier meaning.

The current production flow is:

```text
Body Feature stage
-> complete evaluated Face / Edge / Vertex catalog
-> semantic Surface / Curve / Point carrier meaning
-> fresh runtime topology picking
-> planar semantic Surface Sketch support
-> derived current Sketch O/U/V/N frame
-> existing Profile / Extrude Add-Cut pipeline
```

The as-built invariants are:

- complete topology accounting at every successful Body stage;
- Face/Surface, Edge/Curve and Vertex/Point to remain distinct semantic levels;
- explicit producer-stage meaning rather than final-Body global search;
- runtime Face/Edge/Vertex tokens to remain transient and generation-scoped;
- no persistent OCCT/OCAF handles or topology ordinals;
- no automatic identity from geometry proximity/similarity;
- deterministic planar carrier frames from authored/producer provenance;
- a Surface-backed Sketch to preserve authored local U/V geometry while its derived world frame moves with the support;
- Missing/Ambiguous support to publish no stale current frame;
- bounded cycle rejection using ordered Feature history rather than a universal dependency graph.

For singular Edge/Curve meaning, Owner accepted the PM-02P finding that a Surface pair alone may be insufficient when several disconnected branches exist. A bounded semantic branch/provenance discriminator is permitted only when producer semantics can defend it; otherwise the singular meaning remains Ambiguous. Provider branch order, nearest/longest geometry and XYZ sorting are not valid identity.

The implementation remains bounded by `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`, ADR-0016 and ADR-0017. Planar Body Faces may host standard Sketches through semantic Surface support; non-planar Faces remain selectable/inspectable but standard Sketch support reports `Unsupported`. Re-support preserves SketchId, EntityIds and local U/V geometry, and cycle-causing downstream/self support is rejected before mutation. For Extrude Add only, provider Boolean lineage may prove that a newly-created planar tool Surface uniquely continues one inherited semantic Surface carrier. The inherited carrier keeps durable identity and expands over the current bounded Face fragments; visual coplanarity alone is never identity authority. An Edge separating bounded fragments of that same carrier is a runtime representation partition: fully accounted, non-referenceable and excluded from ordinary engineering Edge presentation/picking. Feature Contribution remains a separate runtime query, so the Add can truthfully contribute current fragments without stealing Surface ownership.

<!-- section-id: internal.part-documents.datum-reference-geometry -->
## Datum reference geometry and Datum-backed Sketch

The implemented construction-reference object is **Offset Datum Plane**. One semantic Datum Plane tool owns both GUI and Command Line input through the same revision-bound `DatumPlaneDraft`. A new draft starts at 10 mm. Offset is one signed authored Length; Reverse only negates that value. Preview, Cancel, rejected Finish and semantic no-op do not create CAD history.

Datum evaluation is Part-owned and provider-neutral. Origin-backed Datum frames use deterministic built-in frames. Body-backed Datums resolve the exact declared planar semantic Surface at its explicit Body stage. Datum-backed Datums recursively resolve earlier DatumId sources. Local Datum cycles and transitive Datum/Sketch/Feature Body-stage cycles fail closed without a global dependency graph.

A successful Create/Edit commits through `DocumentSession`; Edit preserves DatumId. Delete rejects atomically while another Datum or Sketch depends on the target. Individual Show/Hide persists the Datum's authored visibility. The Tree group `Reference Geometry`, directly below Origin, has no separate persisted visibility state: group Show/Hide bulk-updates child authored visibility in one command/Undo step.

The Viewer derives a finite translucent Datum patch/border from the current resolved frame. It may also derive a virtual intersection between that plane and the current Body. The intersection overlay has no independent presentation token or Edge/Curve identity: every rendered segment is owned by the Datum presentation and picking it selects the Datum Plane. It cannot become Projection or durable topology meaning.

A Datum Plane may host a standard Sketch. The Sketch persists only DatumId support and keeps local U/V geometry authored independently of the current world frame. Profile evaluation and existing Extrude Add/Cut consume the currently resolved Datum-backed Sketch through the same stage-aware evaluator. Missing, Ambiguous, Unsupported or Blocked Datum support publishes no stale frame and prevents stale downstream modeling. Repair is explicit through Datum source/offset edit or Change Sketch Support.

<!-- section-id: internal.part-documents.edge-features -->
## PM-05 Fillet / Chamfer and strict material Edge semantics

The current Body supports two edge-consuming ordered Features in addition to Extrude/Revolve:

- FilletFeature — one canonical explicit set of 1..N MaterialEdgeReference values plus one common positive Radius;
- ChamferFeature — one canonical explicit set of 1..N MaterialEdgeReference values plus one common positive Distance.

A MaterialEdgeReference is durable Part meaning, not a provider subshape handle. It stores the exact BodyStageRef, one FeatureCurveAddress and an EdgeBranchDiscriminator. The branch discriminator is either SingularAtAuthoredStage or BetweenSemanticPoints(first, second). BetweenSemanticPoints stores two defensible semantic Point addresses in canonical order and distinguishes bounded branches of one Curve family without using XYZ, length, nearest geometry, provider ordinals or Viewer tokens.

All references in one edge Feature consume exactly the same predecessor Body stage. The set is structurally deduplicated and canonical-sorted, so user click order has no authored meaning.

Authoring admission is strict. A picked runtime Edge must be fully accounted, material/referenceable, singularly resolvable in the exact current stage, and neither a periodic seam nor an ADR-0017 same-Surface representation partition. Multi-branch Curves are authorable only when semantic endpoint Points uniquely discriminate the bounded branch; otherwise authoring fails closed as Unsupported.

Resolution is always stage-scoped. SingularAtAuthoredStage resolves to exactly one current descendant, becomes Missing when none exists and Ambiguous after a semantic split. BetweenSemanticPoints uses semantic Point provenance rather than coordinates and likewise requires exactly one current branch. A later geometrically similar Edge never revives Missing intent. Merge/split cases do not choose first, nearest, longest or provider-order winners.

One Feature is one exact **authored** operation over the complete explicit runtime Edge set; that invariant does not prescribe a single internal OCCT `Build` invocation. In the normal Fillet/Chamfer path the OCCT adapter checks provider input-contour membership before Build: the provider-discovered contour must equal the explicitly resolved authored set. This guard prevents one tangent seed from unintentionally chamfering/blending an unselected tangent chain. Users may explicitly select a complete tangent chain, but PM-05 v1 has no implicit Tangent Chain authoring mode.

For a bounded three-line-Edge equal-distance Chamfer with two exact two-Edge material joints, when the normal combined OCCT Chamfer reports `invalid_brep`, the adapter may attempt an **OCCT-private planar mixed signed-delta fallback**. It checks each independent Edge against the same original stage and exact input-contour membership, builds individual Chamfer evidence, derives no-fuzzy positive/negative material deltas, then composes **one candidate Feature result**, not three persisted Features. The fallback requires both nonzero final added and removed material, a changed single valid solid, three individually accounted generated strip Surfaces, exactly two source-Vertex/incident-Edge corner Surfaces, unique upstream owner for every remaining Face, and complete current Edge/Vertex topology. OCCT `Modified/Generated` histories are chained only as transient provider evidence; no original or reconstructed provider handles, Face ordinals, operation order or geometric nearest-match identities are persisted. Cases outside the bounded supported geometry, missing history, or any conflicting claim return the original failure without partial commit. Existing valid one-/two-Edge and other ordinary Chamfer/Fillet results continue through the original OCCT path.

After that exact input set is accepted, the **result topology may change locally as needed to construct the requested corner**. Neighboring bounded Edges may be shortened or split, a shared Vertex may be replaced, adjacent Faces may split, and local transition/corner patches may appear. Those result entities are consequences of the requested operation, not extra authored inputs. A valid local transition boundary whose provider curve is outside the durable PM-05 line/circle vocabulary remains fully topology-accounted as non-authorable/semantically-unsupported rather than invalidating an otherwise valid Body.

Cross-producer Boolean intersection Curves are durable only when the pair of semantic Surface meanings plus provider analytic classification yields a strict line/circle relation. This admits common plane-cylinder circular Cut boundaries without XYZ, nearest-geometry or provider-order identity. Multiple realizations of the same semantic relation remain Ambiguous.

Successful output is topology-accounted rather than treated as an opaque B-Rep. Generated semantic Surface roles include Fillet surface, Chamfer surface and corner transition. Ordinary generated engineering boundaries use edge_feature_boundary Curve meaning and can become later strict material Edge inputs when semantic branch discrimination is defensible. Corner provenance is derived from producer FeatureId, semantic Point meaning and the canonical incident authored Edge set; provider Face/Edge order is never durable authority.

PM-05 v1 deliberately performs no generic post-operation healing, no global same-domain unification, no fuzzy escalation and no iterative tolerance widening. The normal valid provider result, or the fully certified bounded composed candidate described above, is accounted directly. Provider history is transient reconstruction evidence only. For an untouched Edge across a local Fillet/Chamfer, OCCT may omit explicit Edge-history output even though the exact same semantic Surface-pair boundary remains in the adjacent result stage. Only when provider Edge lineage is completely empty may Part recover that adjacent-stage continuation by the already-published semantic CurveRelation (same role, analytic CurveKind and exact canonical semantic Surface pair). Any non-empty provider lineage remains authoritative, and no geometry-nearest fallback is used.

Evaluation keeps reference resolution separate from geometry execution: Missing/Ambiguous/Unsupported input makes the Feature Blocked with failing input index and reference status; all inputs Resolved but impossible Radius/Distance or provider result makes it Failed; successful provider result plus complete semantic topology publication makes it UpToDate. Suppressed keeps authored FeatureId/inputs but contributes no current geometry. No stale last-good B-Rep becomes current Body truth or downstream input.

Create and Edit use the same revision-bound edge-feature draft/evaluation path. Edit preserves FeatureId, restores the complete Edge set and parameter, and presents/picks against the Feature's exact upstream stage rather than the final Body. Repair is explicit: unresolved intent remains authored until the user explicitly removes/replaces it. Cancel performs zero authored mutation; successful Finish/Edit is one semantic transaction and one Undo step.

PM-05F R2-D clarifies that **candidate evaluation** and **preview rendering** are separate authorities. The accepted candidate stage `B1` determines whether Finish is legal; its immediate upstream `B0` is available as the evaluated preceding Feature stage, also during Edit when downstream may be blocked. The provider computes two exact, fuzzy=0 B-Rep differences `B0 - B1` (removed) and `B1 - B0` (added). Both are disposable provider-neutral presentation meshes, and either may be absent. Neither mesh carries a durable Curve/Surface/Point identity or substitutes for the accepted candidate. If a preview Boolean fails, the Feature's result/Finish legality is not rewritten, and a whole-Body mesh is never silently substituted for a local delta. Native Body scene generation binds preview publication to the exact current/tool stage; stale scenes and Cancel clear transient color independently of authored state.

Save/Reopen persists only semantic intent in schema v15. Runtime topology tokens, OCCT handles, provider history and Viewer identity are rebuilt. Cold reconstruction is proven for both Fillet -> Chamfer and Chamfer -> Fillet. During proven continuation of the same planar semantic Surface, Part retains its existing canonical planar frame; provider U/V orientation is runtime geometry and is not allowed to replace semantic frame authority.

Part-local Projected Edge bindings are now authored as a sorted `PartSketch::projection_bindings` collection keyed by existing stable Sketch `EntityId`, each referencing one strict `MaterialEdgeReference` at a declared prior Body stage. Part schema v15 stores only this semantic link, never a resolved curve, OCCT handle or runtime Edge token. The pure `evaluateEffectiveSketchProjection` produces disposable same-revision geometry by strict source-stage resolution and PG-01A exact projection. Broken targets are omitted from the disposable geometry while their persisted bindings stay available for repair. For ordered Feature evaluation, Extrude and Revolve consume the same effective Profile region and exact kernel input geometry; the authored linked seed is not current modeling truth. Native OCCT evidence covers a linked Line driving a subsequent Cut with intentionally stale authored geometry; source suppression blocks current Body truth.

Headless `CreateProjectedSketchEdgesCommand` accepts a set of **strict semantic MaterialEdgeReference** sources and an independent Regular/Construction role, resolves each source at its exact evaluated stage, performs exact provider projection in the current Sketch frame and authors all target EntityIds plus bindings in **one transaction and one Undo**. An invalid, duplicate, unsupported or degenerate source rejects the whole Edge batch without partially authored entities or consumed IDs. The same pure resolver is used by linked effective Sketch evaluation. Native OCCT tests verify the supported two-Edge batch, failure at the second source, no partial mutation, stable Undo/Redo and cold v15 Save/Reopen with a fresh provider. This atomic Edge-only semantic command is **not** the future partial-Face capture rule in PG-01D.

Existing headless Sketch commands reject direct `UpdateSketchGeometry` of linked targets; Erase Entity/Entities removes targets and their bindings atomically. Trim/Extend/Extend Both reject attempts to use persisted linked seeds as current structural-edit geometry. Duplicate likewise rejects copying a linked authored seed as an unrelated editable curve; an explicit current-provider-derived structural edit must be designed before allowing that operation. `BreakProjectedEdgeLinkCommand` reevaluates current Part, freezes only the latest resolved Line/Circle/Arc in authored geometry, removes that one link while preserving EntityId/role and supports Undo/Redo. If source is suppressed/broken, Break Link rejects the request **without changing the binding or seed**. For Extrude and Revolve draft previews, the same disposable effective Sketch is passed into the provider geometry pipeline, not just the final Finish evaluation. These are **headless internal PG-01B semantics**; GUI picking, advanced provider-aware structural edits, exhaustive cold Feature-chain acceptance and final Owner sign-off remain outside current implementation.

<!-- section-id: internal.part-documents.current-limits -->
## Current limits

The current Part model supports persistent Sketches on Origin planes, planar Body Surfaces and Offset Datum Planes; Part-owned Offset Datum Plane reference geometry; Part-owned Sketch-Line Axes; Shared-2D authoring/precision/OSNAP/structural-edit workflows; live-reference Profiles; direct current Face/Edge/Vertex inspection; and one durable Body with ordered Extrude/Revolve/Fillet/Chamfer Features.

Solid modeling is intentionally bounded to Extrude Add/Cut with OneSide Forward/Reverse and Midplane; Revolve Add/Cut with OneSide/Midplane, explicit Origin/Authored AxisReference and `0 < Angle <= 360 degrees`; constant-radius Fillet; and equal-distance Chamfer over one explicit canonical set of strict material Edges. Axis and Feature Tree/Properties expose semantic identity/status and lifecycle actions. Suppress/Unsuppress, Delete, explicit Axis repair, semantic Sketch re-support, Datum edit/visibility and Reference Geometry bulk visibility participate in Undo/Redo. Save/Close/Reopen reconstructs schema-v15 semantic support, Datum/Axis references, strict MaterialEdgeReference intent and the ordered Body without persisted B-Rep, topology catalogs, provider history, derived Datum/Axis frames or runtime tokens.

Not yet implemented are Datum Axis, Datum Point, additional Datum Plane constructors, Body-Edge/Curve or other Axis constructors, non-planar standard Sketch mapping, the user-facing Project Geometry tool, some provider-aware structural-edit/constraint behaviors and automatic Face-boundary projection, variable-radius/full-round/face Fillet, asymmetric/distance-angle Chamfer, automatic tangent-chain authoring, other solid operations, arbitrary Feature reorder/insertion, multi-turn (>360-degree) Revolve, multi-body modeling, Material, Assembly and Drawing.

Authored constraints/dimensions/solver, Grid Snap, Rotate/Scale/Mirror+Copy, ordinary-Select RMB convergence and clipboard/cross-Sketch Copy also remain outside the current surface.

The Viewer is not a second model: OCCT objects, runtime solid/face tokens, Datum presentation/intersection objects, tessellation and Viewer presentation tokens are never durable Part identity or authored state.

