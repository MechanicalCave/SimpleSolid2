# PM-02 — Direct Body Topology Selection UX

**Status:** ACCEPTED UX DESIGN INPUT — 2026-10-04  
**Production PM-02:** PROPOSED / NOT ACTIVE  
**Applies to candidate:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Related UX:** `work/PM-02_VIEW_STYLE_TREE_FEATURE_CONTRIBUTION_UX.md`  
**Architecture basis:** ADR-0014 + ADR-0016 + PM-02P reference-survival evidence

## 1. Purpose

Define direct viewport acquisition of current Body:

- Face;
- Edge;
- Vertex;

including:

- cursor hit priority;
- screen-space aperture;
- occlusion;
- active-tool filters;
- multiple-hit cycling;
- representation artifacts;
- interaction with View Style and Feature overlays;
- semantic revalidation after a presentation hit.

This is an interaction/presentation design input. It does not activate production PM-02.

## 2. Fundamental boundary

Direct selection is a two-step process:

```text
screen-space acquisition
    ↓ PresentationToken + topology kind
semantic mapping/revalidation
    ↓ current BodyStageTopologyCatalog record
tool admission / runtime selection
```

Screen-space distance answers only:

> what visible thing did the user point at?

It never answers:

> what is the durable CAD identity of that thing?

Therefore pick aperture, depth and cursor proximity are legal presentation/acquisition evidence but are never semantic reference authority and may never repair Missing/Ambiguous references.

## 3. Directly acquired topology kinds

Normal Body topology acquisition works on bounded current topology:

- Vertex;
- Edge;
- Face.

Surface / Curve / Point are not separate default click targets.

After acquisition, Part maps:

```text
Face   -> semantic Face record   -> Surface carrier where defensible
Edge   -> semantic Edge record   -> Curve carrier where defensible
Vertex -> semantic Vertex record -> Point carrier where defensible
```

Carrier visualization appears only when the active command/inspection context needs it.

## 4. Ordinary Select priority

When ordinary Body Select allows all three topology kinds, the default acquisition priority is:

```text
Vertex
  ↓
Edge
  ↓
Face
```

This is a presentation hit-priority rule, not ownership or semantic identity.

### 4.1 Vertex zone

A visible Vertex inside the current screen-space Vertex aperture is preferred over Edge/Face hits under the same pointer.

The provider should display a point/halo preselection cue so the user can see that Vertex is the current candidate before clicking.

### 4.2 Edge zone

If no Vertex wins and a visible material Edge lies inside the current screen-space Edge aperture, Edge is preferred over Face.

The Edge itself is highlighted; the Face behind it does not need to disappear.

### 4.3 Face zone

If neither Vertex nor Edge acquires the pointer, the frontmost visible Face under the pointer becomes the ordinary candidate.

This yields the expected behavior:

```text
near corner  -> Vertex
near boundary -> Edge
face interior -> Face
```

## 5. Pick aperture

Vertex and Edge acquisition use bounded logical-pixel apertures converted by the provider for current DPI.

Exact pixel values are provider/presentation tuning, not CAD semantics.

Recommended initial tuning range:

- Vertex: approximately 8–10 logical px;
- Edge: approximately 6–8 logical px.

The aperture must remain small enough that a user can deliberately move from Vertex -> Edge -> Face without camera-dependent surprises.

Face acquisition uses the visible projected Face region rather than a geometry-proximity tolerance.

Changing DPI, zoom, camera or aperture may change which visible item is easiest to point at. It may not change semantic identity once the token is mapped.

## 6. Occlusion and visibility

Normal PM-02 topology selection is **front-visible selection**.

A topology item hidden behind the current opaque Body is not an ordinary pick candidate.

This remains true in:

- Shaded;
- Shaded + Edges;
- Shaded + Hidden Edges.

Hidden-edge rendering is visual only.

```text
visible hidden-line cue
!= selectable-through-body target
```

A future explicit Select Through / X-Ray selection mode is a separate product decision and is outside PM-02.

## 7. Active-tool selection filters

The active semantic context may narrow which topology kinds participate in acquisition.

Conceptually:

```text
TopologyPickFilter
    Face
    Edge
    Vertex
```

### 7.1 Ordinary Select / Inspect

Default:

- Face enabled;
- Edge enabled;
- Vertex enabled.

### 7.2 Create Sketch support

Filter:

- Face enabled;
- Edge disabled;
- Vertex disabled.

All visible current Faces remain acquireable, including non-planar Faces.

After Face acquisition, semantic admission decides:

- resolved Plane Surface -> accepted;
- resolved non-Plane Surface -> Unsupported;
- Missing -> Missing;
- Ambiguous -> Ambiguous;
- stale context -> rejected.

The picker must not silently fall through from an unsupported/non-planar Face to a nearby Edge or another Face.

This preserves the user's actual gesture and gives a truthful diagnostic.

### 7.3 Future tools

Later tools may request other filters, for example:

- Fillet -> Edge;
- Datum Point consumer -> Vertex/Point;
- mixed reference tool -> declared bounded set.

Those later product tools remain separately gated.

## 8. Semantic capability is not a Viewer filter

Viewer kind filtering answers only Face / Edge / Vertex.

Part semantic admission separately answers questions such as:

- material versus representation artifact;
- Plane versus Cylinder;
- Referenceable versus Unsupported;
- current stage validity;
- cycle legality.

The Viewer must not embed Part-specific rules such as "Sketch supports Planes".

## 9. Multiple-hit candidate stack

One pointer sample may produce more than one valid visible candidate.

Examples:

- several Edges project onto nearly the same screen line;
- multiple Vertices project to the same/similar pixel;
- an Edge/Vertex lies exactly at a Face boundary;
- several same-kind candidates remain after kind priority.

The interaction maintains a runtime-only candidate stack.

### 9.1 Initial preselection

The initial candidate follows:

1. active kind filter;
2. front-visible/occlusion rule;
3. ordinary kind priority where several kinds are enabled;
4. minimum screen-space distance within Vertex/Edge kind;
5. current visible depth where required.

Provider iteration order must not silently define CAD meaning.

### 9.2 Exact/near acquisition ties

If several same-kind candidates remain effectively indistinguishable by the declared presentation ranking, the system must not pretend that one is semantically "the" target merely because the provider returned it first.

The UI exposes a candidate stack.

Only the currently preselected candidate receives the strong hover cue; Status/Diagnostics may report that multiple targets are available.

### 9.3 Candidate cycling

In ordinary 3D topology targeting with viewport CAD focus:

- Tab -> next candidate;
- Shift+Tab -> previous candidate.

Cycling changes only runtime preselection.

Tab/Shift+Tab topology cycling has lower precedence than:

- focused text editor;
- active Dynamic Input field navigation;
- semantic point/value request that already owns Tab;
- modal/popup keyboard ownership.

A pointer move beyond the current hit neighborhood resets the cycle to the newly ranked stack.

The stack also resets on:

- camera/view change;
- View Style change only if it changes the available visible candidate set;
- topology filter/tool change;
- document/session/evaluation/provider generation change;
- Body scene replacement.

### 9.4 Acquisition authority

When the user clicks, the currently visible preselected candidate is the acquisition target.

The user-visible preselection gesture is the authority for choosing among a cycled stack; the internal stack order has no durable semantic meaning.

## 10. Ordinary topology selection grammar

Outside a single-target command, Body topology selection should follow the established CAD selection grammar where practical:

```text
LMB target        -> replace/select target
Ctrl + LMB target -> toggle target membership
LMB blank         -> clear ordinary Body topology selection
Ctrl + LMB blank  -> preserve selection
Esc               -> clear ordinary selection when no higher-priority command context owns Esc
```

PM-02 does not require rectangle/window selection of solid subshapes.

Single-target modeling commands own their own target request and may ignore ordinary multi-selection membership.

## 11. Hover and selected visuals

The direct topology overlay remains independent from base View Style.

Recommended cue shapes:

### Face

- bounded Face tint/fill;
- visible boundary emphasis as presentation envelope where useful.

### Edge

- stronger line/outline;
- no semantic Curve extension unless active inspection/reference context requests it.

### Vertex

- bounded screen-space marker/halo;
- no XYZ label by default.

Direct topology hover/selection uses the direct-selection visual role, distinct from green Feature Contribution.

## 12. Representation artifacts

Complete topology accounting does not mean every provider artifact is an ordinary design-selection target.

A catalog record classified as `KnownRepresentationArtifact`, such as a periodic cylinder seam without independent design intent:

- remains accounted;
- remains diagnosable/inspectable;
- is excluded from ordinary material Edge preselection by default;
- is not silently treated as a durable semantic Curve.

Explicit topology-inspection/debug presentation may reveal such artifacts.

Material topology with `SemanticallyUnsupported` referenceability is different: it remains normal visible geometry and may still be directly selected/inspected so the user can receive the structured Unsupported status.

## 13. Interaction with View Style

### Shaded

Face/Edge/Vertex acquisition still works from the current Body topology presentation even when ordinary visible edges are not permanently drawn.

Preselection draws the acquired Edge/Vertex cue transiently.

### Shaded + Edges

Visible material edges are drawn continuously and use the same current topology presentation records as selection.

### Shaded + Hidden Edges

Visible material edges remain selectable normally.

Dashed/occluded edge pass is non-selectable.

No second "hidden-edge topology" is created.

## 14. Interaction with Feature Contribution

Feature Contribution overlay is presentation-only and does not become a duplicate pick surface.

The pick pipeline continues to query the underlying current Body topology presentation.

Therefore:

```text
green Feature highlight
!= new selectable geometry
```

A Face/Edge/Vertex may be visually emphasized by Feature Contribution and still be acquired through the same one topology token/catalog mapping as normal.

## 15. Interaction with semantic support overlays

A support-plane, carrier-Curve extension or semantic Point cue is non-selectable unless the active tool explicitly declares that overlay as a target class.

For PM-02 Create Sketch:

- user selects the bounded Body Face;
- Part maps it to semantic planar Surface;
- support-plane overlay is feedback after semantic admission;
- the ghost support plane does not become an independent second Face target.

This avoids duplicate or recursive selection authorities.

## 16. Selection freshness

Every direct Body topology preselection/selection is runtime-only and generation-scoped.

Before a modeling command consumes the target, Part revalidates:

- DocumentId;
- DocumentRevision;
- canonical DocumentSession generation;
- evaluation generation;
- provider generation;
- topology kind;
- current semantic catalog record.

A stale PresentationToken cannot commit merely because a later scene reused a numeric token.

## 17. Failure behavior

If a previously preselected token disappears before click/commit:

- clear the stale preselection;
- do not map by geometry similarity;
- reacquire from the current pointer sample if the interaction is still valid.

If semantic mapping after a valid click reports:

- Missing;
- Ambiguous;
- Unsupported;
- stale context;

the tool reports that status and does not silently choose a neighboring candidate.

## 18. Viewer/API direction

The neutral Viewer boundary should remain presentation-oriented.

A production API may introduce concepts equivalent to:

```text
TopologyPresentationKind
    Face
    Edge
    Vertex

TopologyPickFilter
    allowed kinds

TopologyPickCandidate
    PresentationToken
    kind
    presentation hit metrics

TopologyPickQueryResult
    current candidate stack
```

Exact type names are implementation detail.

The Viewer never returns:

- FeatureId;
- SurfaceReference;
- CurveReference;
- PointReference;
- OCCT TopoDS handles.

PartViewportController immediately maps PresentationToken to the current semantic topology record.

## 19. PM-02 acceptance impact

PM-02 should verify at minimum:

- Vertex wins near a visible corner under the ordinary all-kind filter;
- Edge wins near a visible material boundary when no Vertex is in aperture;
- Face wins in visible Face interior;
- Sketch-support filter admits only Faces but still allows non-planar Face acquisition followed by structured Unsupported;
- hidden dashed edges are not selectable through the Body;
- representation-artifact seam is excluded from ordinary material Edge acquisition;
- material but SemanticallyUnsupported topology remains directly inspectable;
- multiple-hit candidate stack can be cycled without provider-order semantic authority;
- cycling is runtime-only and resets on scene/generation change;
- stale preselection cannot commit;
- Feature Contribution overlay does not duplicate selection identity;
- support-plane overlay does not become a second Face target;
- selection behavior is consistent in all three accepted View Styles.
