# PM-02 — Body Semantic Topology / Face-Supported Sketch

**Status:** PROPOSED — AWAITING EXPLICIT OWNER ACCEPTANCE  
**Decision class:** D2 production Work Contract candidate; no production mutation authorized until accepted  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.7  
**Architecture authority:** ADR-0014 + ADR-0016  
**Entry gate:** PM-02P Body Semantic Topology Evidence COMPLETED — PASS; Owner accepted PM-02P D2 synthesis/recommendation on 2026-10-04  
**Evidence authority:** `work/PM-02P_TOPOLOGY_EVIDENCE_REPORT.md`, `work/PM-02P_REFERENCE_SURVIVAL_MATRIX.md`, `work/PM-02_PRODUCTION_ARCHITECTURE_RECOMMENDATION.md`  
**Production mutation:** NOT AUTHORIZED UNTIL THIS CONTRACT IS EXPLICITLY ACCEPTED

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

This document is only a Work Contract candidate.

Production PM-02 becomes ACTIVE only when all of the following are true:

1. the Owner explicitly accepts this exact contract or an amended revision;
2. `work/ACTIVE.yaml` is changed to point to this contract;
3. the repository closure/docs gate for that activation change passes;
4. no unresolved D2 contradiction exists against ADR-0014, ADR-0016 or the accepted PM-02P evidence.

Until then, no production schema, topology catalog, Viewer topology picking or face-supported Sketch mutation is authorized.

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

The accepted detailed UX semantics are recorded in:

`work/PM-02_VIEW_STYLE_TREE_FEATURE_CONTRIBUTION_UX.md`.

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

### PM-02D — Viewer pick, View Style and semantic topology inspection

Deliver:

- fresh Face/Edge/Vertex hover/select mapping;
- runtime token -> current stage catalog;
- semantic Properties/inspection;
- stale selection rejection;
- viewport-HUD View Style selector;
- Shaded / Shaded + Edges / Shaded + Hidden Edges;
- temporary Feature Contribution on tree hover;
- persistent Feature Contribution on tree selection;
- independent overlay roles for direct topology selection versus Feature/tree contribution.

Gate:

- Viewer token never serialized/authored;
- revision/session/evaluation/provider replacement rejects stale pick;
- View Style changes do not mutate/dirty/Undo the Part;
- hidden-edge display does not silently enable select-through;
- split Surface contribution highlights all surviving current Face realizations as a set-valued query;
- deleted outputs are not shown as current contribution;
- selecting a Feature does not implicitly switch to historical Body stage.

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
- all current Faces selectable/inspectable;
- all current Edges selectable/inspectable;
- all current Vertices selectable/inspectable;
- viewport View Style selector provides Shaded / Shaded + Edges / Shaded + Hidden Edges;
- View Style remains non-authored and independent of semantic selection;
- hidden-edge rendering does not silently enable occluded select-through;
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
2. inspect/pick all topology kinds on the Body;
3. switch Shaded / Shaded + Edges / Shaded + Hidden Edges from the viewport HUD and verify no Part dirty/Undo change;
4. hover/select multiple Extrude Features in the tree and verify Current Feature Contribution overlays;
5. verify a later trim/split shows all surviving current contribution fragments while deleted outputs are not ghosted as current truth;
6. create Sketch on a planar cap;
7. create Sketch on a planar lateral Face;
8. create Sketch on a Cut-exposed planar Face when available in the scenario;
9. attempt standard Sketch on a cylindrical Face and observe structured Unsupported;
10. author Profile + Extrude Add/Cut from face-supported Sketch;
11. edit an upstream dimension so the support moves but survives;
12. exercise a support deletion/ambiguity and verify no stale geometry is modeled;
13. repair/re-support the Sketch;
14. Undo/Redo;
15. Save, close, reopen and verify cold reconstruction.

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
- a product case that cannot satisfy the accepted survival matrix without weakening fail-closed semantics.

## 12. Completion boundary

PM-02 completion authorizes only the delivered Body Semantic Topology / Face-Supported Sketch vertical slice.

PM-03 Datum and all later Part-v1 packages remain separately gated by their own accepted Work Contracts.

Projection remains separate and inactive.
