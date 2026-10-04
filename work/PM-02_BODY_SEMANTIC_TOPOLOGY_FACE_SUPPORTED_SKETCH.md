# PM-02 — Body Semantic Topology / Face-Supported Sketch

**Status:** ACTIVE — OWNER ACCEPTED 2026-10-04  
**Decision class:** D2 production Work Contract — Owner accepted 2026-10-04  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.8  
**Architecture authority:** ADR-0014 + ADR-0016  
**Entry gate:** PM-02P Body Semantic Topology Evidence COMPLETED — PASS; Owner accepted PM-02P D2 synthesis/recommendation on 2026-10-04  
**Evidence authority:** `work/PM-02P_TOPOLOGY_EVIDENCE_REPORT.md`, `work/PM-02P_REFERENCE_SURVIVAL_MATRIX.md`, `work/PM-02_PRODUCTION_ARCHITECTURE_RECOMMENDATION.md`  
**Production mutation:** AUTHORIZED ONLY WITHIN THIS ACTIVE CONTRACT

## 1. Goal

Deliver the first production semantic Body-topology consumer as a complete vertical slice:

```text
existing Body
-> select any resolved planar Body Face
-> runtime Face pick
-> semantic Face catalog record
-> semantic planar Surface support
-> Sketch with local U/V authored geometry
-> Profile
-> existing PM-01 Extrude Add/Cut
-> upstream edit
-> stage-aware semantic re-resolution
-> recompute or structured Missing/Ambiguous/Unsupported failure
-> Undo/Redo
-> Save/Close/Reopen
-> cold rebuild
```

PM-02 establishes one production reference foundation that later Datum, Revolve and Edge-consuming features may reuse.

Projection is explicitly independent and remains outside PM-02.

## 2. Activation rule

Owner explicitly accepted this exact Work Contract revision on 2026-10-04, including all four referenced UX/Viewer design inputs.

Activation is completed when `work/ACTIVE.yaml` points to this contract and the activation change passes the repository gate.

Once activated, production mutation is authorized only within this contract and its ordered checkpoints PM-02A through PM-02J.

Any scope expansion or D2 contradiction against ADR-0014, ADR-0016, PM-02P evidence or this contract is STOP and returns to Owner review.

## 3. Governing invariants

Implementation must preserve:

- design intent is authoritative;
- runtime topology is derived and disposable;
- one semantic concept has one owner;
- provider identity never becomes durable Part identity;
- topology and carrier geometry remain distinct:
  - Face != Surface;
  - Edge != Curve;
  - Vertex != Point;
- reference meaning is scoped to an explicit Body stage;
- singular resolution is fail-closed:
  - 0 candidates -> Missing;
  - 1 candidate -> Resolved;
  - >1 candidates -> Ambiguous;
  - undeclared meaning -> Unsupported;
- geometry similarity may diagnose but never independently produce Resolved;
- no stale last-good topology/frame is current modeling truth;
- Viewer selection is never mutation authority;
- commands revalidate current revision/session/evaluation/provider context before Finish;
- no universal dependency framework is introduced as a prerequisite.

## 4. Scope IN

PM-02 may implement only the bounded production functionality required by this contract.

### 4.1 Complete evaluated Body-stage topology catalog

Every successful Body Feature stage must publish disposable evaluated topology with one catalog record per unique current provider:

- Face;
- Edge;
- Vertex.

Required invariant at every successful stage:

```text
provider Faces    == catalog Faces
provider Edges    == catalog Edges
provider Vertices == catalog Vertices
```

Each record must have exactly one accounting class:

- Referenceable;
- KnownRepresentationArtifact;
- SemanticallyUnsupported;
- IntegrityFailure.

Silent omission is an integrity failure.

The catalog is derived evaluation state and is never serialized.

### 4.2 Explicit Body-stage semantics

PM-02 must introduce a provider-neutral stage concept equivalent in meaning to:

```text
BodyStageRef:
    EmptyBody
    AfterFeature(FeatureId)
```

Exact C++ naming may differ.

A semantic reference resolves only in its declared stage. Global final-Body search is forbidden.

### 4.3 Typed runtime topology tokens

The neutral Kernel/runtime boundary may expose transient typed tokens:

- RuntimeFaceToken;
- RuntimeEdgeToken;
- RuntimeVertexToken.

Tokens:

- are valid only for the current evaluation/provider generation;
- are never serialized;
- are never durable semantic identity;
- may be numerically reused only because authority also contains current generation context.

### 4.4 Runtime freshness authority

Any pick or transient topology evidence admitted to a mutating command must be validated against current context equivalent to:

```text
DocumentId
+ DocumentRevision
+ canonical DocumentSession generation
+ evaluation generation
+ provider generation
+ topology kind
```

A stale pick fails before authored mutation.

### 4.5 Production Face / Surface semantics

Production PM-02 must provide stage-scoped provider-neutral semantic Surface meaning for the current Extrude Add/Cut universe, including:

- ProfileCap;
- ExtentCap;
- NegativeCap;
- PositiveCap;
- Side generated from exact Profile boundary-use provenance;
- inherited/trimmed descendants;
- Cut-exposed tool Surfaces.

A strict bounded Face realization is allowed to become Ambiguous while its semantic Surface remains Resolved.

### 4.6 Surface selector meaning

For the activated Extrude Add/Cut universe, durable planar Surface support must encode meaning equivalent to:

```text
BodyStageRef
+ producer FeatureId
+ Surface role
+ exact source provenance when required
```

For side Surfaces, source provenance must retain the existing semantic boundary meaning required to distinguish source Sketch Entity / loop / use / hole role.

No provider handle, topology ordinal, geometry fingerprint, Viewer token or current Face index may appear in durable support identity.

### 4.7 Edge / Curve evaluated semantics

Every material Edge must be catalogued and given provider-neutral semantic meaning where defensible.

Evidence-backed meanings include:

- cap/side boundary;
- side/side boundary;
- inherited or uniquely trimmed material Edge;
- Boolean-created material intersection Edge.

Periodic seam topology is accounted as KnownRepresentationArtifact unless independent semantic design intent exists.

PM-02 does not persist a general durable EdgeReference merely to anticipate PM-05. It must, however, produce a provider-neutral semantic Edge/Curve catalog sufficient for:

- inspection;
- picking;
- status;
- future reuse;
- regression of complete accounting.

### 4.8 Accepted Edge branch/provenance rule

The Owner accepted on 2026-10-04 the PM-02P D2 finding:

> A singular Edge/Curve selector may use a bounded semantic branch/provenance discriminator only when the producing operation supplies defensible semantic meaning. If no such meaning exists, the selector remains Ambiguous.

Therefore:

- a pair of Surfaces alone is not always sufficient;
- provider branch index/order is forbidden;
- nearest/longest/shortest/XYZ sorting is forbidden;
- geometry similarity alone is forbidden;
- no arbitrary branch winner is allowed.

PM-02 may represent operation-owned branch provenance in evaluated semantic records when current Extrude Add/Cut semantics can prove it.

A durable serialized Edge selector remains outside PM-02 unless a concrete PM-02 user requires one and the contract is explicitly amended.

### 4.9 Vertex / Point evaluated semantics

Every current Vertex must be catalogued.

Provider-neutral Point/Vertex meaning may use accepted semantic relationships such as:

- cap + side + side carrier intersection;
- endpoint/intersection of semantic material Curves;
- explicit operation provenance.

XYZ is diagnostic only.

PM-02 does not persist a general durable VertexReference merely for hypothetical later consumers.

### 4.10 Carrier geometry classification

Production evaluated Surface classification must include at least:

- Plane;
- Cylinder;
- Cone;
- Sphere;
- Torus;
- Other.

Curve classification must include exact classes required by the active Extrude Add/Cut universe, including at least:

- Line;
- Circle;
- exact supported Arc/Circle-derived classes as required by current semantics;
- Other.

Classification corroborates meaning but is not identity.

### 4.11 Deterministic planar carrier frames

PM-02 must implement provider-neutral canonical O/U/V/N frames.

#### Extrude caps

For semantic cap offset `d`:

```text
O = source O + d * source N
U = source U
V = source V
N = source N
```

Semantic role determines the offset:

- OneSide Forward: ProfileCap / ExtentCap;
- OneSide Reverse: ExtentCap / ProfileCap;
- Midplane: NegativeCap / PositiveCap.

Canonical carrier orientation is independent of outward/material Face normal.

#### Planar side from authored Line

```text
O = authored Line start mapped through source support/Profile frame
U = normalized authored Line direction in world space
V = source support/Profile N
N = U x V
```

Trim/split preserves the carrier frame.

#### Cut-exposed planar Surface

Use the Cut tool's semantic Profile/boundary provenance and the same algebraic rules.

The following are forbidden as frame authority:

- raw provider UV;
- first current Edge;
- provider traversal order;
- current trim loop;
- Face centroid;
- bounding box;
- camera;
- tessellation.

### 4.12 Topology picking, visual styles and Feature inspection

PM-02 may add bounded provider-neutral Viewer/Part support required to:

- hover/select Body Face;
- hover/select Body Edge;
- hover/select Body Vertex;
- map current runtime pick token to current stage catalog record;
- expose semantic type/status/classification/provenance in inspection/Properties;
- present the accepted viewport View Style selector;
- visualize Current Feature Contribution from Document Tree hover/selection.

Picking is transient.

Only semantic command inputs may cross into persistent mutation.

The following UX/Viewer design inputs are normative for this active contract and were accepted by the Owner on or before 2026-10-04:

`work/PM-02_VIEW_STYLE_TREE_FEATURE_CONTRIBUTION_UX.md`;  
`work/PM-02_DIRECT_TOPOLOGY_SELECTION_UX.md`;  
`work/PM-02_BODY_PRESENTATION_VIEWER_API_DESIGN.md`;  
`work/PM-02_TOPOLOGY_PROPERTIES_INSPECTION_UX.md`.

#### Base View Style

PM-02 must expose one single-select View Style control in the viewport navigation/presentation HUD, grouped with HOME / ORTHO-PERSP / Navigation Cube presentation controls.

Required styles:

- Shaded;
- Shaded + Edges;
- Shaded + Hidden Edges.

View Style is presentation state:

- it does not increment DocumentRevision;
- it does not dirty the Part;
- it does not create CAD Undo/Redo history;
- it is not persisted in the Part file.

A UI/workspace preference may remember it outside the Part document.

Hidden-edge display is visual only. Enabling Shaded + Hidden Edges must not silently enable select-through of occluded topology.

#### Overlay independence

Base View Style, direct topology selection, tree Feature highlighting, command preview and diagnostic overlays are independent presentation layers.

Changing View Style must not change:

- semantic selection;
- topology/reference status;
- command targets;
- current Feature Contribution;
- authored CAD state.

Direct viewport Face/Edge/Vertex interaction uses the direct-topology selection overlay. Tree Feature interaction uses a distinct Feature-contribution overlay.

#### Current Feature Contribution

For Feature `F`, Current Feature Contribution is a **set-valued query** against the currently authoritative displayed Body stage.

It means the current semantic topology that still carries meaning produced directly by `F`.

Face contribution:

- current Face realizations whose semantic Surface carrier producer is `F`;
- if one prior Face realization split into several current Faces while retaining one Surface produced by `F`, all surviving current Faces are highlighted;
- this set-valued display is not a singular FaceReference resolution and therefore is not Ambiguous merely because there are several fragments;
- deleted semantic output is not ghosted as current contribution.

Edge/Vertex contribution:

- direct semantic Edge/Curve or Vertex/Point records whose producer provenance identifies `F`;
- boundary Edges/Vertices around highlighted contribution Faces may be drawn as a presentation envelope for readability without being reclassified as direct Feature-owned semantic topology.

A current topology item involving carriers from several Features must not be arbitrarily assigned to one Feature merely for coloring.

#### Tree hover and selection

Feature tree hover:

- shows temporary Current Feature Contribution;
- does not change primary selection/Properties authority;
- disappears on hover exit;
- creates no authored mutation.

Feature tree selection:

- shows persistent Current Feature Contribution;
- makes the Feature the tree/Properties selection;
- keeps the current Body in its current base View Style;
- does not automatically replace the current Body with a historical stage.

An active modeling command may suppress lower-priority tree hover so command target/preview cues remain unambiguous.

#### Cut contribution

For Cut, direct Current Feature Contribution includes current semantic topology introduced by the Cut, such as:

- Cut-exposed tool Surfaces;
- Boolean-created material intersection Edges with direct operation provenance;
- directly generated semantic Points/Vertices when defensible.

Upstream Surfaces merely trimmed by the Cut retain their original semantic producer and are not stolen by the Cut highlight.

#### Distinct presentation concepts

The following must remain distinct:

```text
Current Feature Contribution
!= Operation Scope / Delta
!= Historical Stage Preview
```

Current Feature Contribution is the default tree hover/selection meaning.

Operation Scope / Delta answers what volume the Feature added/removed at its own upstream stage. It may later be visualized as a translucent ghost and remains optional unless promoted by contract amendment.

Historical Stage Preview answers what the entire Body looked like immediately after the Feature. It must be an explicit presentation action if later productized and is not implied by ordinary tree selection.

#### One committed Body presentation authority

PM-02 must evolve the current triangle-only committed `SolidScene` into one atomic topology-aware committed Body presentation equivalent in meaning to:

```text
BodyScene
    generation
    shaded triangles
    Face presentation records
    Edge presentation records
    Vertex presentation records
```

The exact public type names are implementation detail.

Requirements:

- Body shading and Face/Edge/Vertex presentation come from the same current `RuntimeSolidHandle` / evaluation-provider generation;
- topology is never reconstructed from triangle adjacency;
- provider presentation records are keyed by current typed runtime Face/Edge/Vertex tokens;
- `PartViewportController` is the only owner of runtime-topology-token <-> `PresentationToken` bindings;
- Viewer receives only neutral presentation tokens and display geometry/flags;
- committed Body scene replacement is atomic from controller authority perspective;
- no independent public Face/Edge/Vertex scene setters may become separate committed Body authorities;
- the existing command-owned `SolidPreviewScene` remains separate and cannot supply durable Body reference identity;
- each installed Body scene carries a runtime-only generation stamp;
- Body hit/query results carry that generation and stale generations are rejected;
- direct selection may reuse existing `PresentationSelection`;
- Feature Contribution is a separate overlay over the same current Body tokens and must not create duplicate pick geometry;
- one current Edge presentation feeds visible-edge rendering, hidden-edge rendering and picking;
- support/carrier ghost overlays are derived presentation and do not become duplicate Body topology targets;
- a displayed `resolved_prefix_solid` after failure is diagnostic-only and cannot become successful-final-Body command authority.

The neutral Viewer boundary may expose concepts equivalent in meaning to:

```text
ViewStyle
BodyTopologyPresentationKind
BodyTopologyPickFilter
BodyTopologyPickCandidate
BodyTopologyPickQueryResult
BodyTopologyOverlay
```

but must not expose Part semantic IDs/references or OCCT handles.

#### Direct Body topology acquisition

Ordinary Body selection uses presentation hit priority:

```text
Vertex
  ↓
Edge
  ↓
Face
```

This priority is only a screen-space acquisition rule.

Required selection semantics:

- Vertex and Edge use bounded DPI-aware screen-space pick apertures;
- Face uses the front-visible projected Face region;
- cursor proximity may choose what visible item the user pointed at but never defines durable identity;
- hidden/occluded topology is not an ordinary pick candidate in any of the three View Styles;
- Shaded + Hidden Edges is visual only and does not enable select-through;
- all accepted hits are immediately mapped from PresentationToken to the current semantic topology catalog and revalidated there.

Active tools may narrow the Viewer kind filter without embedding Part semantics in the Viewer.

Create Sketch support uses Face-only acquisition. A non-planar Face must still be acquireable and then fail semantic admission as Unsupported; the picker must not silently fall through to a neighboring Edge/Face.

When several visible candidates remain under one pointer sample, PM-02 uses a runtime candidate stack rather than provider-first implicit choice.

In ordinary 3D topology targeting with viewport CAD focus:

- Tab selects the next preselection candidate;
- Shift+Tab selects the previous candidate;
- text input / Dynamic Input / semantic input contexts that already own Tab take precedence;
- pointer neighborhood, view, tool/filter or evaluation/provider generation changes reset the stack.

The currently visible preselected candidate is the user's acquisition target on click. Candidate stack order has no durable CAD meaning.

Representation artifacts such as a periodic seam remain completely catalogued and diagnosable but are excluded from ordinary material Edge preselection by default. Material topology whose durable referenceability is Unsupported remains visible/selectable for truthful inspection and diagnostics.

Feature Contribution, hidden-edge graphics and semantic support/carrier ghost overlays do not create duplicate selectable geometry. Normal picking always maps through the single current Body topology presentation.

#### Topology Properties / inspection

PM-02 should keep the existing Properties panel as the single product inspection surface and add a bounded adaptive topology context rather than dumping Face/Edge/Vertex nodes into Document Tree.

The detailed proposed behavior is in:

`work/PM-02_TOPOLOGY_PROPERTIES_INSPECTION_UX.md`.

Required contract semantics:

- hover/preselection and Tab/Shift+Tab candidate cycling do not churn Properties;
- explicit tree selection or committed viewport selection establishes the primary Properties subject;
- direct current topology presence is distinct from durable semantic referenceability;
- current selected topology may be Present while durable referenceability is Ambiguous or Unsupported;
- Face Properties expose Surface carrier/classification, producer/stage/provenance and standard Sketch-support capability;
- Edge Properties expose Curve classification, material/representation class, producer/provenance, adjacent Surface summary and durable referenceability;
- Vertex Properties expose Point/provenance, adjacency summary and durable referenceability;
- XYZ/area/length/radius are diagnostics only and are never identity;
- Body Properties expose current complete topology/accounting counts;
- Feature Properties expose Current Feature Contribution counts from the same semantic query used by tree highlighting;
- Sketch Properties expose authored support intent plus current Resolved/Missing/Ambiguous/Unsupported support state;
- no RuntimeFaceToken/RuntimeEdgeToken/RuntimeVertexToken, PresentationToken, TopoDS handle or provider traversal index is shown as product identity;
- deleted direct topology selection clears rather than geometry-rebinding;
- durable authored Sketch support may remain selected while its target reports Missing;
- diagnostic resolved-prefix topology is visibly non-authoritative and cannot offer normal mutating reference actions;
- Document Tree remains authored design/history and does not become a topology catalog dump.

A topology Properties action such as Create Sketch may only delegate to the same semantic command admission used elsewhere. Properties compatibility is never a bypass around command validation.

### 4.13 Sketch support types

Production PM-02 must replace the Origin-only support representation with a bounded semantic support model equivalent to:

```text
SketchSupport:
    BuiltinOriginPlane(role)
    BodyPlanarSurface(SurfaceReference)
```

No Datum support is introduced in PM-02.

Standard Sketch support admission:

- resolved Plane Surface -> admissible;
- resolved non-Plane Surface -> Unsupported;
- Missing -> Missing;
- Ambiguous -> Ambiguous;
- stale runtime selection -> rejected before semantic command execution.

### 4.14 Authored Sketch state versus evaluated frame

For semantically resolvable support, durable authored state owns:

- SketchId;
- SketchSupport;
- authored local U/V Sketch geometry;
- existing name/visibility/presentation intent;
- no independent absolute world-placement truth.

Evaluated state owns a result equivalent to:

```text
ResolvedSketchSupport:
    status
    O/U/V/N
    diagnostic
```

PM-02 does not expose authored support-relative offset/rotation unless explicitly added by contract amendment.

### 4.15 Schema v9 migration

PM-02 is authorized to introduce the next Part schema version for Sketch support.

Migration from current schema v8 must:

1. preserve DocumentId and Document properties;
2. preserve BodyId / FeatureId / Feature order;
3. preserve SketchId / EntityId and local Shared-2D geometry;
4. preserve ProfileId / RegionIntent;
5. preserve names, visibility and current authored presentation policy;
6. validate that the legacy Origin-plane support and legacy absolute `SketchPlacement` are coherent under current v8 invariants;
7. map legacy Origin support into the new semantic support representation;
8. stop persisting redundant absolute world `SketchPlacement` where the support frame is derivable;
9. fail closed on malformed legacy authored support/placement state;
10. never serialize evaluated topology catalogs, runtime tokens, provider history or evaluated carrier frames.

The exact serialized field names may be chosen D0/D1 inside this contract as long as the meaning above is unchanged.

### 4.16 Stage-aware Sketch/Profile evaluation

Evaluation order must become support-aware:

```text
evaluate upstream Body stage N
-> publish complete topology catalog(N)
-> resolve Sketch support declared at N
-> derive current Sketch frame
-> map local Sketch/Profile geometry into world
-> evaluate consuming Feature N+1
```

A Missing/Ambiguous/Unsupported support produces structured downstream failure/blocked state.

Stale last-good world Sketch geometry cannot feed a current Feature.

### 4.17 Create Sketch on Body Face

The product workflow must allow Sketch creation from a selected Body Face when:

- the current pick is fresh;
- the strict Face resolves to a semantic Surface;
- the Surface is Plane;
- dependency admission is legal.

The durable support is the semantic Surface, not the runtime Face token.

This must work for:

- Extrude caps;
- planar lateral Faces;
- Cut-exposed planar Faces.

It must not special-case only top/bottom caps.

### 4.18 Re-support existing Sketch

PM-02 may support changing an existing Sketch support between:

- Origin XY/XZ/YZ;
- resolved planar Body Surface.

Default behavior preserves authored local U/V geometry.

Re-support must:

- revalidate current source/target support;
- reject Missing/Ambiguous/Unsupported target;
- reject stale runtime selection;
- reject dependency cycle before mutation;
- preserve SketchId and EntityIds;
- produce one Undoable transaction.

No silent mirror/flip of local geometry is permitted.

### 4.19 Bounded dependency-cycle admission

For current single-Body ordered Feature history, PM-02 must use a bounded preflight:

1. identify the stage that produces the requested support;
2. identify Features consuming the Sketch through Sketch -> Profile -> Feature;
3. require support producer stage to be strictly upstream of each consumer;
4. reject self/forward dependency before transaction mutation.

No universal dependency graph is authorized.

### 4.20 Existing Extrude Add/Cut from face-supported Sketch

A Profile derived from a face-supported Sketch must be consumable by existing PM-01 Extrude Add/Cut without a second modeling implementation.

Required scenario:

```text
Sketch A on Origin
-> Extrude Add
-> Body
-> select arbitrary planar Body Face
-> Sketch B on semantic Surface
-> Profile B
-> Extrude Add or Cut
-> edit upstream A / first Extrude
-> re-resolve B support
-> move/recompute or structured failure
```

GUI, Command Line and non-GUI semantic callers must share the same command/domain meaning.

### 4.21 Failure and repair lifecycle

Production behavior must distinguish at least:

- Resolved;
- Missing;
- Ambiguous;
- Unsupported;
- CycleDependency;
- stale selection/context;
- geometric evaluation failure.

Diagnostics must identify:

- affected Sketch/Feature;
- support/reference kind;
- producer stage/Feature where relevant;
- semantic status;
- enough provenance for user repair without exposing provider identity as model truth.

No automatic geometry-similarity repair is permitted.

### 4.22 Undo / Redo / Save / Close / Reopen

PM-02-authored support changes must participate in existing transaction/history semantics.

Required:

- Create face-supported Sketch -> one authored transaction;
- Re-support -> one authored transaction;
- Cancel/rejected command -> no mutation;
- Undo/Redo restore semantic support intent and IDs;
- Save/Close/Reopen reconstruct semantic support and derived frames from authored state;
- cold rebuild does not require previous runtime topology tokens/provider handles.

## 5. Scope OUT

PM-02 does not authorize:

- Datum Plane / Axis / Point creation;
- Projection / Project Edge / automatic face-boundary capture;
- Reproject / Refresh projection;
- Revolve;
- Fillet;
- Chamfer;
- Through All / Up To Face Extrude;
- multi-body;
- arbitrary Feature reorder/insertion;
- non-planar Sketch mapping;
- persistent B-Rep;
- persistent OCCT/OCAF/provider identity;
- a general durable EdgeReference/VertexReference schema without a concrete PM-02 consumer;
- a global dependency graph;
- automatic geometry-similarity reference repair;
- general direct-face editing;
- Assembly/Drawing implementation.

## 6. Permitted repository areas after activation

After explicit Owner acceptance, bounded mutation may occur in:

```text
src/part/**
src/kernel/**
src/kernel_occt/**
src/application/**
src/viewer/**
src/ui/**
tests/**
cmake/**
CMakeLists.txt
scripts/verification-related files
work/**
docs/internal/**
docs/product/pl/**
docs/product/en/**
docs/browser/index.html
```

only where required by this contract.

Persistence-format changes remain Part-owned and must not leak provider identity.

Any new dependency direction outside the current architecture baseline is STOP.

## 7. Checkpoint sequence

### PM-02A — production evaluated topology catalog

Deliver:

- per-stage complete Face/Edge/Vertex catalog;
- typed runtime tokens;
- provider-neutral accounting classes;
- stage freshness;
- no persistence.

Gate:

- complete accounting at every current Add/Cut stage;
- no stale catalog published as final Body truth;
- semantic/core + kernel-native tests.

### PM-02B — Surface/Face production semantics

Deliver:

- production Surface semantic keys for current Extrude Add/Cut;
- strict Face current realization status;
- inherited/trimmed/split/delete/Cut-exposed behavior;
- exact planar Surface classification/frame.

Gate:

- survival matrix Face/Surface rows pass;
- strict Face split may be Ambiguous while Surface remains Resolved;
- zero false Resolved.

### PM-02C — Edge/Curve and Vertex/Point semantic catalog

Deliver:

- complete production Edge/Vertex accounting;
- material Edge semantics;
- seam representation-artifact handling;
- accepted bounded branch/provenance rule;
- Point/Vertex semantic relationships.

Gate:

- no provider-order/geometry heuristic;
- multi-branch pair remains Ambiguous without defensible branch provenance;
- exact same XYZ does not rebind Point identity.

### PM-02D — topology-aware Body presentation, Viewer pick, View Style and semantic inspection

Deliver:

- atomic topology-aware committed Body scene from one current runtime/evaluation generation;
- current Face/Edge/Vertex presentation records mapped to neutral PresentationTokens;
- Body-scene generation stamp and stale-generation rejection;
- fresh Face/Edge/Vertex hover/select mapping;
- runtime token -> current stage catalog;
- semantic Properties/inspection;
- stale selection rejection;
- ordinary Vertex -> Edge -> Face screen-space acquisition priority;
- active-tool Face/Edge/Vertex pick filters;
- runtime multiple-hit candidate stack with Tab / Shift+Tab cycling when no higher-priority input context owns Tab;
- viewport-HUD View Style selector;
- Shaded / Shaded + Edges / Shaded + Hidden Edges;
- visible/hidden edge rendering from the same current Edge presentation set;
- temporary Feature Contribution on tree hover;
- persistent Feature Contribution on tree selection;
- bounded topology overlay roles over the same current tokens;
- independent overlay roles for direct topology selection versus Feature/tree contribution.
- adaptive topology Properties/inspection context for Face/Surface, Edge/Curve and Vertex/Point;
- Body and Feature Properties topology/contribution summaries;

Gate:

- committed Body mesh and topology presentation come from the same current RuntimeSolid/evaluation-provider generation;
- triangle adjacency is never used to reconstruct Face/Edge semantic identity;
- one atomic committed Body scene is the only Body presentation authority after migration;
- Viewer token never serialized/authored;
- revision/session/evaluation/provider/body-scene generation replacement rejects stale pick;
- numeric PresentationToken reuse across generations does not preserve authority;
- View Style changes do not mutate/dirty/Undo the Part;
- hidden-edge display does not silently enable select-through;
- ordinary pick prioritizes visible Vertex then visible Edge then front-visible Face;
- Sketch-support acquisition filters to Face while non-planar Face remains acquireable and returns structured Unsupported;
- provider order cannot silently choose among equivalent multiple hits;
- candidate cycling is runtime-only and resets on stale scene/context;
- representation-artifact seam is excluded from ordinary material Edge pick but remains accounted/inspectable;
- one Edge presentation feeds visible-edge, hidden-edge and pick behavior;
- Feature/support overlays do not duplicate pick identity;
- Feature Contribution reuses current Body topology tokens rather than creating a second pick surface;
- command preview remains separate from committed Body topology identity;
- diagnostic resolved-prefix presentation cannot be consumed as successful final Body target authority;
- split Surface contribution highlights all surviving current Face realizations as a set-valued query;
- deleted outputs are not shown as current contribution;
- selecting a Feature does not implicitly switch to historical Body stage.
- hover/preselection/candidate cycling does not replace the Properties subject;
- selected current topology can distinguish Present from durable Referenceability;
- Properties never expose provider/runtime tokens as durable identity;
- Body Properties counts match the current complete topology catalog;
- Feature contribution counts match the same semantic query used for tree highlighting;

### PM-02E — Sketch support schema v9 + deterministic frame resolver

Deliver:

- new semantic SketchSupport persistence;
- v8->v9 migration;
- removal of redundant absolute world placement authority;
- Origin support parity;
- resolved planar Surface frame.

Gate:

- old valid v8 files migrate preserving IDs/geometry;
- malformed legacy support fails closed;
- Save/Reopen produces identical support meaning.

### PM-02F — stage-aware Sketch/Profile evaluation

Deliver:

- current support resolution from upstream stage;
- derived Sketch world frame;
- world Profile materialization from local geometry;
- structured Missing/Ambiguous/Unsupported failure.

Gate:

- upstream carrier move changes derived world geometry without authored local mutation;
- no stale frame feeds Feature evaluation.

### PM-02G — create/re-support Sketch on arbitrary planar Body Surface

Deliver:

- GUI + semantic command + Command Line parity;
- cap, lateral and Cut-exposed planar support;
- non-planar selection visible but Unsupported;
- re-support preserving local U/V geometry;
- cycle admission.

Gate:

- one Finish = one transaction;
- Cancel/rejected support = no mutation;
- SketchId/EntityIds preserved on re-support.

### PM-02H — existing Extrude Add/Cut from face-supported Sketch

Deliver the primary vertical workflow using existing PM-01 Extrude.

Gate:

- Add and Cut both work from face-backed Profile;
- upstream edits re-resolve support;
- valid support moves/recomputes;
- lost/ambiguous support fails structurally without stale downstream truth.

### PM-02I — lifecycle, repair, persistence and regression matrix

Close:

- Undo/Redo;
- Delete interactions;
- Save/Close/Reopen;
- cold rebuild;
- stale selection;
- split/delete/alias failure;
- same-geometry replacement trap;
- semantic repair/re-support;
- exact diagnostics.

Gate:

- PM-02 survival matrix passes as production tests;
- zero false Resolved;
- no ID/provenance corruption.

### PM-02J — documentation and Owner Windows acceptance

Deliver:

- current internal as-built docs;
- PL/EN product docs;
- regenerated Product Browser;
- final acceptance matrix;
- Windows Owner manual workflow.

Gate:

- required docs valid;
- final exact-head runtime gate;
- final docs/closure gate as appropriate;
- Owner manual PASS before PM-02 completion.

## 8. Acceptance requirements

PM-02 cannot complete without automated and manual evidence covering at least:

- v8 -> v9 migration preserving DocumentId/BodyId/FeatureId/SketchId/EntityId/ProfileId;
- canonical rectangular Add stage catalog: 6 Faces / 12 Edges / 8 Vertices accounted;
- complete accounting at every chained Add/Cut stage;
- all current material Faces selectable/inspectable;
- all current material Edges selectable/inspectable; KnownRepresentationArtifact edges remain fully accounted/inspectable but are excluded from ordinary material pick by default;
- all current material Vertices selectable/inspectable;
- selected Face Properties show Surface type, producer/stage/provenance, referenceability and Sketch-support capability;
- selected Edge Properties distinguish current presence from durable referenceability and identify representation artifacts truthfully;
- selected Vertex Properties expose Point/provenance while XYZ remains diagnostic only;
- Body Properties topology/accounting counts equal the current evaluated catalog;
- Feature Properties contribution counts equal the tree-highlight semantic contribution query;
- Sketch Properties expose authored support intent and current support state without authored-world-placement dual authority;
- hover/candidate cycling does not churn Properties;
- deleted direct topology selection clears rather than geometry-rebinding;
- diagnostic prefix topology is visibly non-authoritative in Properties;
- committed Body shading and Face/Edge/Vertex presentation are produced from one current RuntimeSolid/evaluation-provider generation;
- no Body Face/Edge identity is reconstructed from triangle adjacency;
- Body presentation installs atomically with one runtime-only scene generation;
- stale Body-scene generation cannot commit even if a PresentationToken numeric value is reused;
- one current Edge presentation backs visible-edge rendering, hidden-edge rendering and Edge picking;
- Feature Contribution overlays reuse current Body topology tokens and do not create duplicate selectable geometry;
- support/carrier ghost overlays remain non-selectable unless an explicit future tool declares them as target classes;
- command-owned solid preview remains separate from committed Body reference identity;
- diagnostic resolved-prefix Body presentation cannot be consumed as successful final Body reference authority;
- viewport View Style selector provides Shaded / Shaded + Edges / Shaded + Hidden Edges;
- View Style remains non-authored and independent of semantic selection;
- hidden-edge rendering does not silently enable occluded select-through;
- ordinary all-kind acquisition uses visible Vertex -> Edge -> Face priority;
- bounded DPI-aware Vertex/Edge apertures affect only presentation acquisition, never semantic identity;
- active-tool kind filters are honored without embedding Part semantic admissibility in Viewer;
- Sketch Face-only acquisition can select a non-planar Face and report Unsupported rather than silently falling through;
- multiple-hit candidate stack can be cycled explicitly without provider-order semantic authority;
- stale preselection/candidate stack cannot commit after scene/evaluation/provider replacement;
- Feature Contribution and support/carrier overlays are non-duplicating presentation layers, not second pick authorities;
- tree hover shows temporary Current Feature Contribution;
- tree selection shows persistent Current Feature Contribution without historical-stage substitution;
- split Feature Surface contribution highlights all surviving current Face realizations without misclassifying the set-valued display as Ambiguous;
- Cut contribution highlights direct Cut-created current topology without stealing upstream Surface ownership;
- planar cap Sketch support;
- planar lateral Sketch support;
- Cut-exposed planar Sketch support;
- non-planar Face selectable but standard Sketch support Unsupported;
- strict Face split with surviving Surface keeps Surface-backed Sketch valid;
- support deletion -> Missing and no stale current frame;
- support ambiguity -> Ambiguous and no arbitrary rebind;
- identical/near-identical replacement geometry does not steal support identity;
- upstream dimensional edit moves stable support and derived Sketch world geometry while local U/V is unchanged;
- no unexpected 180-degree flip/mirror;
- re-support preserves SketchId/EntityIds and local authored geometry;
- re-support to self/downstream stage rejects CycleDependency before mutation;
- stale revision/session/evaluation/provider pick cannot commit;
- existing Extrude Add from face-backed Profile;
- existing Extrude Cut from face-backed Profile;
- upstream edits recompute or produce structured downstream failure;
- Undo/Redo support lifecycle;
- Save/Close/Reopen + cold rebuild;
- no provider identity serialized;
- no geometry-similarity automatic repair;
- GUI / Command Line / semantic caller parity;
- semantic/core, kernel-native and desktop verification on accepted final candidate;
- supported Windows manual workflow.

## 9. Manual Windows acceptance themes

The final Owner workflow must exercise at minimum:

1. create Body from Origin Sketch + Extrude Add;
2. inspect/pick Body topology and verify ordinary near-corner -> Vertex, near-boundary -> Edge, Face-interior -> Face acquisition;
3. inspect Face/Edge/Vertex Properties and verify carrier, producer/stage/provenance and referenceability are semantic while geometry values are diagnostic;
4. create an overlapping/multiple-hit view and verify Tab / Shift+Tab cycles visible candidates without changing authored state or churning Properties;
5. switch Shaded / Shaded + Edges / Shaded + Hidden Edges from the viewport HUD and verify no Part dirty/Undo change;
6. verify hidden dashed edges remain non-selectable through the opaque Body;
7. hover/select multiple Extrude Features in the tree and verify Current Feature Contribution overlays;
8. verify a later trim/split shows all surviving current contribution fragments while deleted outputs are not ghosted as current truth;
9. create Sketch on a planar cap;
10. create Sketch on a planar lateral Face;
11. create Sketch on a Cut-exposed planar Face when available in the scenario;
12. attempt standard Sketch on a cylindrical Face and observe structured Unsupported without fallback to a nearby Edge/Face;
13. author Profile + Extrude Add/Cut from face-supported Sketch;
14. edit an upstream dimension so the support moves but survives;
15. exercise a support deletion/ambiguity and verify no stale geometry is modeled;
16. repair/re-support the Sketch;
17. Undo/Redo;
18. Save, close, reopen and verify cold reconstruction.

Exact UI labels may evolve D0/D1 inside the contract; semantic behavior may not.

## 10. Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: PM-02 changes durable Sketch support meaning, persistence schema, topology picking/inspection, error/repair behavior and the user workflow for creating Sketches on Body Faces.

Before completion:

- internal Part/persistence/viewer documentation must describe the as-built production boundary;
- PL/EN product docs must describe supported planar Body-face Sketch workflow and limits;
- Product Browser must be regenerated;
- planned-but-not-implemented later features must not be documented as current product capability.

## 11. STOP conditions

STOP and return to Owner review if implementation would require:

- changing ADR-0014 or ADR-0016 semantics;
- persistent TopoDS/OCAF/provider topology identity;
- Face[n]/Edge[n]/Vertex[n] or provider traversal identity;
- geometry similarity/proximity as automatic rebinding authority;
- arbitrary Edge branch selection;
- raw provider UV / first-edge / current trim fallback for canonical support frame;
- persisting evaluated topology catalog/runtime tokens/provider history;
- a schema meaning outside Section 4.15;
- a durable general Edge/Vertex reference system not justified by PM-02 consumers;
- a global dependency graph;
- multi-body;
- Projection as a prerequisite;
- Datum as a prerequisite;
- non-planar Sketch mapping;
- a new public dependency direction contrary to Architecture baseline;
- deriving production Face/Edge topology or semantic identity from committed triangle adjacency;
- maintaining independent committed Body mesh/topology scenes whose generations can diverge;
- exposing RuntimeFaceToken/RuntimeEdgeToken/RuntimeVertexToken or Part semantic references directly through the Viewer public API;
- allowing a diagnostic resolved-prefix scene to become normal successful-Body command target authority;
- a product case that cannot satisfy the accepted survival matrix without weakening fail-closed semantics.

## 12. Completion boundary

PM-02 completion authorizes only the delivered Body Semantic Topology / Face-Supported Sketch vertical slice.

PM-03 Datum and all later Part-v1 packages remain separately gated by their own accepted Work Contracts.

Projection remains separate and inactive.
