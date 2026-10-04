# PM-02 — Topology Properties / Inspection UX

**Status:** ACCEPTED UX DESIGN INPUT — 2026-10-04  
**Production PM-02:** PROPOSED / NOT ACTIVE  
**Applies to candidate:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Related design inputs:**  
- `work/PM-02_VIEW_STYLE_TREE_FEATURE_CONTRIBUTION_UX.md`  
- `work/PM-02_DIRECT_TOPOLOGY_SELECTION_UX.md`  
- `work/PM-02_BODY_PRESENTATION_VIEWER_API_DESIGN.md`  
**Architecture basis:** ADR-0014 + ADR-0016 + PM-02P evidence

## 1. Purpose

Define what the existing Properties panel displays when the primary inspection subject is:

- Body;
- Feature;
- current Body Face;
- current Body Edge;
- current Body Vertex;
- Sketch with semantic support.

The goal is to make semantic topology understandable without:

- adding every Face/Edge/Vertex to Document Tree;
- exposing provider runtime tokens/TopoDS handles;
- conflating current selectable topology with durable referenceability;
- turning Properties into a second modeling authority.

This is a UX/architecture input. It does not activate production PM-02.

## 2. One Properties panel, one primary inspection subject

The existing stacked Properties panel remains the single product Properties surface.

PM-02 may add a bounded topology inspection page/context.

The panel displays one **primary inspection subject** at a time.

Primary subject may come from:

- current tree selection;
- current direct viewport topology selection;
- current authored reference/support selection where existing UI already uses Properties.

Hover/preselection alone does not replace Properties.

Tree hover Feature Contribution does not replace Properties.

Candidate-stack cycling does not replace Properties until the user commits the visible preselection with click.

## 3. Selection-domain behavior

Ordinary explicit selection is treated as one primary inspection context.

Recommended PM-02 behavior:

- selecting a tree Feature makes that Feature the primary Properties subject and clears ordinary direct Body topology selection;
- selecting a current Body Face/Edge/Vertex makes that topology item the primary Properties subject and clears ordinary tree authored-item selection where required to keep one primary selection authority;
- transient tree hover and command preview do not clear the primary subject;
- active modeling command target acquisition may show target feedback in the Operations/status area and viewport without rewriting Properties on every hover.

This avoids simultaneous competing "primary" selections.

Presentation overlays may still transiently coexist where their semantics are distinct.

## 4. Direct topology: existence versus durable referenceability

A directly selected current topology object is present by definition in the installed current Body scene.

Properties must distinguish:

```text
Current topology presence
!= durable semantic referenceability
```

Examples:

- a current material Edge can be visibly selected while a singular durable Edge selector would be Ambiguous;
- a current material Edge can be inspectable while durable referenceability is Unsupported;
- a periodic seam can exist in the complete catalog while ordinary material selection excludes it.

Do not display every selected current topology item as automatically `Resolved`.

## 5. Common topology Properties fields

A single topology Properties page may adapt by selected kind.

Recommended common fields:

- **Topology** — Face / Edge / Vertex;
- **Current stage** — semantic Body stage in which this record is being inspected;
- **Current status** — Present / diagnostic prefix where applicable;
- **Accounting class** — Referenceable / Representation Artifact / Semantically Unsupported / Integrity Failure;
- **Referenceability** — Resolved / Ambiguous / Unsupported where meaningful;
- **Producer Feature** — human-readable Feature name plus stable FeatureId where useful;
- **Semantic role / provenance** — human-readable meaning;
- **Selection count** — when ordinary multi-selection contains more than one item.

Do not display:

- RuntimeFaceToken / RuntimeEdgeToken / RuntimeVertexToken;
- PresentationToken;
- provider traversal index;
- TopoDS/OCAF identity;
- geometry-similarity score as identity.

Internal diagnostics may log runtime tokens, but they are not product Properties.

## 6. Face / Surface Properties

When one current Face is primary, show at minimum:

- Topology: Face;
- Carrier: Surface;
- Surface type:
  - Plane;
  - Cylinder;
  - Cone;
  - Sphere;
  - Torus;
  - Other;
- Producer Feature;
- semantic Face/Surface role;
- current Body stage;
- accounting class;
- durable referenceability;
- Sketch support capability.

### 6.1 Sketch support capability

For standard planar Sketch:

- Plane + semantically admissible Resolved support -> Supported;
- non-Plane Surface -> Unsupported: non-planar standard Sketch support;
- semantic ambiguity -> Ambiguous;
- other unsupported semantic class -> Unsupported with reason.

The panel may expose a direct action such as **Create Sketch** only when the same semantic command admission would accept the selected Face.

Properties availability is not a bypass around command validation.

### 6.2 Geometry diagnostics

Derived diagnostics may be displayed, for example:

- current Face area;
- current bounded-loop count;
- orientation/normal diagnostics where useful.

They must be visually/semantically presented as geometry diagnostics, not identity.

For a planar Surface, canonical support-frame information may be available in an advanced/details section:

- origin O;
- U;
- V;
- N.

The frame comes from semantic carrier rules, never from current trim topology.

## 7. Edge / Curve Properties

When one current Edge is primary, show at minimum:

- Topology: Edge;
- Carrier: Curve where defensible;
- Curve type:
  - Line;
  - Circle;
  - exact supported Arc/Circle-derived class where applicable;
  - Other;
- Material/representation classification;
- Producer Feature where semantic provenance exists;
- semantic provenance/role;
- adjacent semantic Surface summary;
- current Body stage;
- accounting class;
- durable referenceability.

### 7.1 Ambiguous singular Edge meaning

If the selected current Edge exists but a stable singular semantic selector has multiple valid branches, Properties should explicitly distinguish:

```text
Current topology: Present
Referenceability: Ambiguous
Candidates: N
```

The user must not be misled into thinking the current visible Edge is already a stable durable reference merely because it can be clicked.

### 7.2 Representation artifact

If explicitly inspected in a diagnostic/inspection context, a periodic seam should read approximately:

```text
Topology: Edge
Classification: Representation Artifact
Carrier: provider representation seam
Referenceability: Unsupported
Ordinary material selection: No
```

Do not present it as a normal design Edge.

### 7.3 Geometry diagnostics

Derived geometry may include:

- current Edge length;
- radius for circular Curve;
- endpoints/parameter diagnostics where useful.

These remain diagnostics only.

## 8. Vertex / Point Properties

When one current Vertex is primary, show at minimum:

- Topology: Vertex;
- Carrier: Point where defensible;
- Producer Feature / semantic provenance;
- adjacent semantic Curve/Surface summary;
- current Body stage;
- accounting class;
- durable referenceability.

XYZ may be shown as current geometry diagnostics in document display units.

XYZ is never labeled or treated as semantic identity.

A moved semantic Point may retain identity while XYZ changes after recompute.

## 9. Body Properties extension

Current Body Properties already show:

- BodyId;
- status;
- ordered Feature count.

PM-02 should add current evaluated topology summary when Body is UpToDate:

- Faces;
- Edges;
- Vertices;
- Referenceable count by kind;
- Representation Artifact count;
- Semantically Unsupported count;
- Integrity Failure count.

If integrity-failure count is nonzero, Body/topology inspection must visibly report degraded/integrity state rather than quietly present a healthy catalog.

When Body is unavailable, do not display stale last-good topology counts as current successful truth.

## 10. Feature Properties extension

Current Feature Properties already expose authored/evaluation information.

PM-02 should add **Current Feature Contribution** summary:

- Current contribution Faces;
- Direct semantic Edges;
- Direct semantic Vertices;
- optional presentation-boundary Edge count;
- semantic outputs currently Missing/Ambiguous where meaningful;
- Feature stage.

This is a summary of the same Part semantic contribution query used for tree highlighting.

It is not computed from currently green pixels.

A split Surface can therefore report several current Face realizations without labeling the Feature Contribution itself Ambiguous.

## 11. Sketch Properties extension

For a Sketch, Properties should distinguish authored support intent from evaluated support state.

Recommended support fields:

- Support kind:
  - Origin Plane;
  - Body Surface;
- Support status:
  - Resolved;
  - Missing;
  - Ambiguous;
  - Unsupported;
- Support stage;
- Producer Feature for Body Surface support;
- Surface type;
- semantic role/provenance summary;
- current frame status.

Do not present the evaluated world O/U/V/N frame as independent authored placement.

If the support is Missing/Ambiguous/Unsupported:

- no stale frame is presented as current truth;
- repair/re-support action may be offered;
- downstream Feature failure/blocking is summarized where useful.

## 12. Multi-selection

PM-02 ordinary topology selection may contain multiple current items.

Properties remains primary-item oriented.

When more than one item is selected:

- show selection count;
- show the primary item's full details;
- optionally show a compact common-kind/common-class summary;
- do not pretend mixed selections share one carrier/referenceability result.

No generic multi-object property editor is required by PM-02.

## 13. Hover and candidate stack

Hover/preselection Properties updates are intentionally avoided.

Reason:

- Vertex/Edge candidate aperture can cause rapid candidate changes;
- Tab/Shift+Tab cycles runtime preselection;
- Properties should remain stable until explicit selection.

Transient candidate information belongs in viewport cue/status text.

On LMB commit, Properties switches to the selected current topology subject.

## 14. Active command interaction

During a command such as Create Sketch support acquisition:

- viewport shows the candidate/accepted target overlay;
- Operations/status displays command-specific instruction/diagnostic;
- Properties does not become command state authority.

After a Face is accepted as ordinary semantic selection or the command creates/edits the Sketch, Properties may transition to the resulting Face/Sketch subject according to the established selection result.

A command must not infer acceptance merely because Properties currently displays a compatible Face.

## 15. Missing and historical semantics

A current directly selected topology item cannot remain selected after its Body-scene generation disappears.

If recompute deletes the selected topology:

- clear the stale direct topology selection;
- do not preserve it via nearest geometry;
- Properties falls back to the next valid authored/tree/document context.

For authored references such as Sketch support, Missing remains visible in the owning object's Properties because the durable authored support intent still exists.

This is the key distinction:

```text
deleted current topology selection -> selection disappears
durable authored support whose target disappeared -> owner remains, support status = Missing
```

## 16. Diagnostic prefix scene

If the Viewer is showing a diagnostic `resolved_prefix_solid` because final Body evaluation failed:

- topology Properties must clearly mark the subject as **Diagnostic Prefix**;
- no "Create Sketch" or mutating topology-reference action is enabled from that subject;
- it must not be displayed as successful final Body current topology.

## 17. Identity presentation

Product UI should prefer human-readable semantic meaning:

- Feature name;
- FeatureId where useful;
- role such as Extent Cap / Side / Cut Surface;
- source Sketch/Profile/Entity label when available;
- Body stage.

Stable IDs may be exposed for traceability consistent with existing Properties practice.

Provider/runtime IDs must not be shown as durable identity.

## 18. Suggested topology Properties layout

A single adaptive page can remain compact:

```text
Topology
  Type: Face
  Stage: After Extrude002
  Status: Present
  Accounting: Referenceable

Semantic
  Carrier: Surface
  Surface Type: Plane
  Producer: Extrude001
  Role: Side
  Referenceability: Resolved

Capability
  Sketch Support: Supported

Geometry
  Area: ... mm²
  [advanced frame/details]
```

Edge/Vertex use the same section model with kind-specific fields.

Exact widget layout and labels are D0/D1 implementation detail as long as semantics remain clear.

## 19. Tree remains design history

PM-02 must not add a default tree dump such as:

```text
Body
  Faces
    Face001
    ...
  Edges
    ...
  Vertices
    ...
```

Document Tree remains authored design/history.

Current topology is inspected via:

- viewport selection;
- Properties;
- bounded future inspection tools.

## 20. PM-02 acceptance impact

Production PM-02 should verify at minimum:

- selecting current Face shows Face + Surface semantics without provider token leakage;
- planar versus cylindrical Face exposes correct Sketch support capability;
- selected current Edge can report Present while Referenceability is Ambiguous/Unsupported;
- representation-artifact seam is clearly distinguished from material design Edge;
- selected Vertex shows semantic Point/provenance while XYZ is diagnostic only;
- Body Properties counts match the complete current topology catalog;
- Feature Properties contribution counts match the same semantic query used by viewport tree highlight;
- Sketch Properties show authored support intent plus current support status without authored-world-placement dual authority;
- hover/candidate cycling does not churn Properties;
- deleted direct topology selection clears rather than geometry-rebinding;
- durable Sketch support can remain selected as owner while its support reports Missing;
- diagnostic prefix topology is visibly non-authoritative and cannot offer normal mutating reference actions;
- no default topology dump is added to Document Tree.
