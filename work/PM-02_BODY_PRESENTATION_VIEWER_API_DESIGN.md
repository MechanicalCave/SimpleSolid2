# PM-02 — Body Presentation / Viewer API Design

**Status:** ACCEPTED TECHNICAL DESIGN INPUT — 2026-10-04  
**Production PM-02:** ACTIVE  
**Applies to active contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Related UX:**  
- `work/PM-02_VIEW_STYLE_TREE_FEATURE_CONTRIBUTION_UX.md`  
- `work/PM-02_DIRECT_TOPOLOGY_SELECTION_UX.md`  
**Architecture basis:** ADR-0014 + ADR-0016 + PM-02P evidence

## 1. Purpose

Define the provider-neutral presentation boundary required by PM-02 so that:

- committed Body shading;
- visible/hidden material edges;
- Face/Edge/Vertex picking;
- direct topology hover/selection;
- tree Feature Contribution;
- support/diagnostic overlays;

all project the **same current Body topology snapshot** rather than independently reconstructing topology from tessellation, Viewer state or provider traversal.

This accepted technical design is normative within the active PM-02 Work Contract.

## 2. Core rule: one topology, one presentation snapshot

The committed Body must have one authoritative runtime presentation snapshot per installed generation.

Conceptually:

```text
RuntimeSolidHandle
    ↓ same evaluation/provider generation
Kernel/provider Body presentation extraction
    ↓
Part semantic BodyStageTopologyCatalog
    ↓
PartViewportController semantic <-> presentation bindings
    ↓
atomic BodyScene
    ↓
IDocumentViewport / Qt-OCCT
```

The Viewer must not derive CAD topology from the committed triangle mesh.

Forbidden:

```text
triangle adjacency
-> guessed Face/Edge topology
-> selection/reference identity
```

Tessellation is presentation only.

## 3. Existing PM-01 boundary

Current PM-01 committed Body presentation is:

```text
kernel::SolidPresentationMesh
    triangles[]
        ↓
viewer::SolidScene
    triangles[]
        ↓
IDocumentViewport::setSolidScene(...)
```

This is sufficient for shaded display but insufficient for PM-02 because it does not preserve per-Face/Edge/Vertex presentation identity.

PM-02 may evolve the committed Body path into an atomic topology-aware scene.

Transient `SolidPreviewScene` for Extrude command preview remains a separate command-preview concept.

## 4. Kernel/provider presentation extraction

PM-02 should introduce a provider-neutral result equivalent in meaning to:

```text
BodyPresentationResult
    status
    mesh
    faces[]
    edges[]
    vertices[]
```

Exact names/layout are implementation detail.

The result is runtime-only and never serialized.

### 4.1 Shared runtime source

The Body presentation payload must be extracted from the same current `RuntimeSolidHandle` and provider generation that produced the current evaluated Body/topology evidence.

Do not rebuild presentation from an unrelated provider object or an older cached shape.

### 4.2 Face presentation mapping

The provider already tessellates Face-by-Face.

PM-02 presentation should preserve that grouping using an equivalent of:

```text
FacePresentationRange
    RuntimeFaceToken
    first_triangle
    triangle_count
```

or another equally explicit mapping.

Requirements:

- every rendered current Face can be mapped to its current runtime Face token;
- Face grouping is not reconstructed later from triangle adjacency;
- triangle order/range is presentation data only;
- runtime Face token is never persisted.

### 4.3 Edge presentation mapping

For each current provider Edge that participates in the complete topology catalog, presentation extraction may provide an equivalent of:

```text
EdgePresentationPath
    RuntimeEdgeToken
    polyline_points[]
```

The polyline is display geometry only.

Provider-private sampling may depend on:

- exact Line;
- exact Circle/Arc;
- other provider Curve class;
- view-independent tessellation tolerance chosen for presentation.

The sampled points do not define semantic Curve identity.

### 4.4 Vertex presentation mapping

For each current provider Vertex:

```text
VertexPresentationPoint
    RuntimeVertexToken
    Point3
```

The point is display/hit-test geometry only.

XYZ never becomes semantic Point identity.

## 5. Typed runtime tokens

The Kernel/provider boundary may expose typed runtime tokens:

- RuntimeFaceToken;
- RuntimeEdgeToken;
- RuntimeVertexToken.

These tokens:

- identify one current provider realization inside the current runtime authority generation;
- are valid only while that current runtime generation is authoritative;
- are not serialized;
- are not Viewer PresentationTokens;
- may be numerically reused in a later generation without preserving identity.

The provider must not collapse all topology kinds into one untyped numeric namespace merely for convenience.

## 6. Complete topology versus ordinary display

Kernel/provider presentation extraction may include every current catalogued Face/Edge/Vertex.

Whether an item is normally:

- rendered;
- pickable;
- hidden;
- diagnostic-only;

is not determined from provider topology kind alone.

Part/controller policy uses semantic accounting such as:

- material topology;
- KnownRepresentationArtifact;
- SemanticallyUnsupported;
- IntegrityFailure.

Examples:

- periodic seam Edge may exist in the snapshot but be excluded from ordinary material-edge display/pick;
- a material Edge whose durable referenceability is Unsupported remains displayable/pickable for truthful inspection.

Complete accounting does not imply visual clutter.

## 7. Viewer committed Body scene

The neutral Viewer target should evolve from `SolidScene` into an atomic scene equivalent in meaning to:

```text
BodyScene
    generation
    triangles[]
    faces[]
    edges[]
    vertices[]
```

Exact names are implementation detail.

### 7.1 PresentationToken ownership

Viewer records contain only `PresentationToken` plus neutral display geometry/flags.

They do not contain:

- RuntimeFaceToken;
- RuntimeEdgeToken;
- RuntimeVertexToken;
- FeatureId;
- SurfaceReference;
- CurveReference;
- PointReference;
- TopoDS handles.

`PartViewportController` owns the current runtime-token <-> PresentationToken binding.

### 7.2 Atomic installation

Committed Body scene replacement must be atomic from the controller's authority perspective.

Do not expose independent public setters such as:

```text
setBodyMesh(...)
setFaceScene(...)
setEdgeScene(...)
setVertexScene(...)
```

whose generations may diverge.

The target contract is one operation equivalent to:

```text
setBodyScene(scene)
```

where mesh, topology presentation and generation belong to one snapshot.

### 7.3 Transition from current SolidScene

PM-02 implementation may temporarily retain compatibility helpers while migrating tests/providers.

By completion of the topology-aware Viewer checkpoint, there must be only one committed Body presentation authority.

The old triangle-only `SolidScene` must not remain an independently mutable second committed Body scene beside `BodyScene`.

Transient command preview remains separate.

## 8. Body presentation generation

Each installed committed Body scene carries a nonzero runtime-only presentation generation.

Conceptually:

```text
BodyPresentationGeneration
```

It is owned by the controller/application presentation context, not persisted CAD state.

Every Body hit/query result carries the generation from which it was produced.

A hit is current only when:

```text
hit.generation == installed BodyScene.generation
```

and the higher-level document/session/evaluation/provider freshness checks also pass.

Scene replacement invalidates:

- direct Body hover;
- Body candidate stack;
- transient Body topology selection bindings whose semantic meaning has not been re-resolved;
- stale overlay token sets.

Numeric PresentationToken reuse in another generation does not preserve authority.

## 9. Semantic-to-presentation binding

After Part evaluation, the controller builds bindings equivalent in meaning to:

```text
RuntimeFaceToken   <-> PresentationToken
RuntimeEdgeToken   <-> PresentationToken
RuntimeVertexToken <-> PresentationToken
```

Bindings are generation-scoped runtime state.

The controller may also associate each PresentationToken with the corresponding current semantic catalog record for immediate lookup.

The Viewer does not own this mapping.

## 10. Body topology query API

The Viewer boundary should expose a query equivalent in meaning to:

```text
queryBodyTopology(
    ViewportPoint2 point,
    BodyTopologyPickFilter filter)
        -> BodyTopologyPickQueryResult
```

The exact API shape is implementation detail.

### 10.1 Viewer query result

A candidate contains only presentation facts equivalent to:

```text
BodyTopologyPickCandidate
    PresentationToken
    BodyTopologyPresentationKind
    screen_distance
    depth
```

The Viewer may return enough neutral hit information for controller ranking.

It must not return semantic Part identity.

### 10.2 Policy ownership

The Viewer/provider detects neutral geometric hits.

`PartViewportController` owns selection policy:

- active Face/Edge/Vertex filter;
- Vertex -> Edge -> Face ordinary priority;
- aperture acceptance;
- multiple-hit stack/cycling;
- command-context precedence;
- semantic mapping/revalidation.

Provider iteration order is not selection policy.

## 11. PresentationSelection reuse

Existing `PresentationSelection` remains the neutral selected-token presentation channel for direct ordinary selection.

PM-02 should reuse it for Body topology PresentationTokens rather than creating a separate Viewer-only Body-selection authority.

Semantic ownership remains outside Viewer.

## 12. Feature Contribution overlay

Tree Feature Contribution is not ordinary `PresentationSelection`.

Selecting a Feature in Document Tree does not mean the user selected every highlighted Face/Edge/Vertex.

PM-02 should expose a bounded neutral overlay concept equivalent in meaning to:

```text
BodyTopologyOverlay
    role
    face_tokens[]
    edge_tokens[]
    vertex_tokens[]
    boundary_edge_tokens[]
    boundary_vertex_tokens[]
```

Required PM-02 roles should remain bounded to actual product needs, such as:

- FeatureContribution;
- FeatureBoundary;
- direct hover/target if not already covered by selection presentation;
- support/target cue;
- AmbiguousCandidate / diagnostic cue where required.

Do not introduce a generic universal graphics-layer framework.

## 13. Feature Contribution source of truth

Feature Contribution is computed by Part/evaluation semantics, not by Viewer color grouping or provider history alone.

Flow:

```text
FeatureId
    ↓
Part current semantic contribution query
    ↓
current semantic Face/Edge/Vertex records
    ↓
current runtime-token bindings
    ↓
PresentationToken overlay sets
    ↓
Viewer
```

Viewer never infers contribution by comparing colors, geometry or triangle ownership.

## 14. View Style

The neutral Viewer API should expose one presentation state equivalent to:

```text
enum class ViewStyle {
    shaded,
    shaded_with_edges,
    shaded_with_hidden_edges,
};
```

and operations equivalent to:

```text
viewStyle()
setViewStyle(...)
setViewStyleActionHandler(...)
```

Exact naming may differ.

View Style is not Part-authored state.

## 15. One Edge presentation, multiple projections

The same current material Edge presentation records must back:

- visible-edge drawing;
- hidden-edge drawing;
- direct Edge hit testing;
- direct Edge selection highlight;
- Feature Contribution/Feature boundary overlay where applicable.

Do not build separate independent Edge geometry for each presentation feature.

This enforces:

```text
one current Edge presentation
-> many visual projections
```

## 16. Hidden-edge rendering

The neutral contract defines semantics only:

- hidden edges use the same current Edge presentation records;
- hidden-edge rendering does not create another topology set;
- hidden-edge pass is non-selectable through the opaque Body;
- representation artifacts remain governed by ordinary-display policy.

Qt/OCCT may choose provider-private rendering techniques such as multi-pass depth treatment.

No specific OCCT primitive or line algorithm becomes public architecture.

## 17. Navigation/presentation HUD

The View Style selector belongs to the same provider-surface HUD family as existing:

- HOME;
- ORTHO/PERSP;
- Navigation Cube.

Qt/OCCT may render/hit-test the control inside the Viewer surface.

However, the selected View Style must be observable/settable through the neutral Viewer API so:

- mock Viewer tests can verify it;
- controller/application runtime state remains explicit;
- a later workspace preference can be implemented without hidden provider-only authority.

## 18. Carrier/support overlays are not Body topology

Semantic Surface/Curve/Point visualization remains separate from the bounded Body topology scene.

Examples:

- planar support plane;
- Curve extension;
- semantic Point cue.

These are neutral derived overlays produced after semantic resolution.

They are non-selectable unless an active future tool explicitly declares them as target classes.

For PM-02 Create Sketch:

```text
user picks bounded Body Face
-> Part resolves semantic planar Surface
-> support-plane overlay appears
```

The support plane does not become a duplicate Face target.

## 19. Command preview remains separate

`SolidPreviewScene` remains command-owned transient presentation for Extrude Add/Cut preview.

It must not share committed Body topology tokens or become a source of Body reference identity.

Command preview and committed Body scene may be visible together, but their roles and token domains remain distinct.

## 20. Prefix/diagnostic Body presentation

If current evaluation fails but exposes `resolved_prefix_solid`, PM-02 may show a diagnostic prefix Body scene.

Such a scene must be explicitly marked non-authoritative for modeling-target acquisition.

The UI may support diagnostic inspection, but commands must not consume prefix Face/Edge/Vertex as if they belonged to a successful final Body.

No stale last-good final Body scene may remain selectable as current truth after a failing evaluation.

## 21. Scene installation transaction

Recommended controller sequence:

```text
PartEvaluation N
-> obtain presentation payload from current RuntimeSolidHandle
-> build semantic <-> presentation bindings
-> allocate BodyScene generation G
-> build complete BodyScene G
-> viewport.setBodyScene(G)
-> if success:
       install controller bindings G
       invalidate old hover/candidate stacks
       restore/re-resolve allowed semantic selection
   if failure:
       authored CAD remains authoritative
       report presentation degradation
       old tokens are not accepted as current modeling authority
```

Exact mutation ordering inside controller/provider may differ as long as no stale presentation token becomes command authority.

## 22. Failure and validation rules

`BodyScene::valid()` or equivalent should verify at minimum:

- nonzero generation when non-empty;
- all referenced PresentationTokens are valid;
- Face triangle ranges are in bounds and non-overlapping/consistent with chosen representation;
- Edge polylines contain valid finite points;
- Vertex positions are finite;
- no duplicate token exists across incompatible Body topology records;
- topology kind for each token is unambiguous.

Semantic completeness remains Part/domain responsibility, not Viewer validation.

## 23. PM-02 checkpoint impact

### PM-02A

Must establish the complete runtime topology/token source from which presentation can be built.

### PM-02D

Must deliver the topology-aware Viewer boundary:

- atomic committed Body scene;
- generation-scoped PresentationTokens;
- Face/Edge/Vertex query;
- direct selection;
- View Style;
- Feature Contribution overlays.

### PM-02I

Must regression-test scene replacement/freshness across:

- upstream recompute;
- split/delete;
- failed evaluation/prefix presentation;
- session/evaluation/provider replacement;
- Save/Reopen cold rebuild.

## 24. Acceptance requirements

The production implementation must prove at minimum:

- committed Body triangles and topology presentation come from one current RuntimeSolid/evaluation generation;
- no Face/Edge selection identity is reconstructed from triangle adjacency;
- every ordinary displayed/pickable material topology record maps through one current PresentationToken;
- scene installation is atomic from controller authority perspective;
- stale generation candidates cannot commit;
- numeric token reuse across generations does not preserve authority;
- one Edge presentation feeds visible-edge, hidden-edge and picking behavior;
- hidden-edge drawing remains non-selectable through the Body;
- Feature Contribution reuses current topology tokens rather than creating duplicate pick geometry;
- support-plane/carrier overlays do not create duplicate Body topology targets;
- command preview remains separate from committed Body reference identity;
- representation artifacts remain accountably present without polluting ordinary material display/pick;
- prefix diagnostic scene cannot be consumed as successful final Body reference authority;
- current mock Viewer tests can verify View Style and Body-scene/query semantics without OCCT types.
