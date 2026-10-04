# PM-02C — Edge/Curve and Vertex/Point Semantic Catalog Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Runtime candidate:** `9135baf1d749f01eeb5dcae4f6ea7cffc856d8a0`  
**Windows gate:** FULL #1402 — PASS on the exact runtime candidate  
**Merged runtime main:** `b128f7445b2705ec9577afddcdbf7f9c81594aa0`  
**Scope boundary:** PM-02C only; no Viewer topology picking, BodyScene/ViewStyle implementation, topology Properties UI, schema v9 or face-supported Sketch

## 1. Closure statement

PM-02C is COMPLETED — PASS.

The production Body-stage semantic catalog now promotes the accepted current Extrude Add/Cut Edge/Curve and Vertex/Point relationships on top of completed PM-02A accounting and PM-02B Surface authority.

The implementation preserves the accepted distinction between bounded topology and semantic carrier geometry:

```text
Edge   != Curve
Vertex != Point
```

and keeps provider history, runtime tokens, geometry length and XYZ as transient evidence/diagnostics rather than durable identity.

The next active checkpoint is PM-02D — topology-aware Body presentation, Viewer picking, View Styles and semantic inspection.

## 2. Delivered production boundary

Kernel/provider delivery:

- one complete semantic observation is published for every current runtime Edge and Vertex;
- `CurveKind` classifies the exact supported current Curve families as Line, Circle or Other;
- current Edge observations publish adjacent semantic runtime Surface tokens;
- current Vertex observations publish adjacent semantic runtime Surface tokens and incident material Edge tokens;
- provider XYZ is finite diagnostic data only and never Point identity;
- periodic seam evidence is explicitly classified and not promoted to ordinary material design Edge;
- transient adjacent-stage Edge/Vertex realization lineage uses provider history only as evidence;
- inherited realization lineage is fail closed:
  - 0 current descendants -> Missing;
  - 1 -> Resolved;
  - >1 or incompatible alias -> Ambiguous;
- runtime Edge/Vertex observations and lineage are never serialized.

Part semantic catalog delivery:

- `FeatureCurveAddress` / `FeatureCurveResolution` are provider-neutral and distinct from bounded Edge realization;
- Curve meaning derives from canonical semantic Surface relationships:
  - cap/side;
  - side/side;
  - supported Boolean intersection;
- provider Curve class must agree with the semantic relation or evaluation fails closed;
- `FeaturePointAddress` / `FeaturePointResolution` use exactly three canonical semantic Surface relationships for the supported Point universe;
- Edge and Vertex catalog records distinguish accounting class from referenceability;
- material topology with unsupported semantic meaning remains explicitly SemanticallyUnsupported rather than disappearing;
- periodic seam is `KnownRepresentationArtifact` and ordinary durable material referenceability remains Unsupported;
- provider order, Edge length, nearest geometry and XYZ are not identity.

## 3. Edge/Curve lifecycle result

The production semantics close the accepted PM-02C Edge/Curve behaviors for the current Extrude Add/Cut universe:

| Event | Production result |
| --- | --- |
| pristine rectangular prism Edge | material Line Curve from semantic Surface relation |
| pristine cylindrical cap/side boundary | material Circle Curve |
| periodic cylinder seam | KnownRepresentationArtifact; not ordinary material Curve |
| inherited Edge unchanged | Curve/strict Edge Resolved |
| inherited Edge trim | Curve/strict Edge Resolved |
| inherited Edge split | Curve remains Resolved; strict Edge becomes Ambiguous with explicit descendants |
| inherited Edge removed | Missing |
| unique Boolean-created semantic Surface-pair intersection | supported Boolean-intersection Curve |
| one semantic Surface pair yields multiple disconnected branches with no defensible branch discriminator | Curve Ambiguous / strict Edge Ambiguous |
| provider Curve class contradicts semantic expectation | fail-closed topology integrity failure |

No provider traversal order, longest/nearest Edge or geometry similarity may turn an ambiguous branch set into Resolved.

## 4. Vertex/Point result

The supported production Point meaning is based on canonical semantic relationships rather than coordinate identity.

Current guarantees:

- the canonical rectangular prism publishes complete current Vertex semantic observations;
- current material Vertex records derive Point candidates from exactly three semantic Surface relationships;
- internal Cut preserves singular inherited outer Vertex lineage where provider history proves one descendant;
- moved Point/Vertex identity follows semantic lineage/relationships, not XYZ;
- exact same-XYZ replacement with different provenance cannot revive a deleted old Point identity;
- provider XYZ remains diagnostic only.

## 5. Verification evidence

Exact-head Windows FULL #1402 passed on `9135baf1d749f01eeb5dcae4f6ea7cffc856d8a0`.

The FULL lane passed:

- complete desktop test graph build;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST and SUBSYSTEM selector verification;
- full desktop test execution;
- SR-02 latency benchmark evidence;
- CI-04 warm FULL parity evidence;
- CI-04 comparative timing evidence.

PM-02C-specific production regressions include:

- `pm02c.production_edge_curve_vertex_point`;
- extended `pm01c.feature_evaluator`.

Production kernel-native coverage includes:

- rectangular prism: 12 material line Edges and 8 semantic Vertex observations;
- cylinder: representation seam accounting plus two material circular cap/side Edges;
- inherited Edge unchanged/trim/split/remove lifecycle;
- unique Boolean-created Surface-pair intersection;
- two disconnected branches for one Surface pair;
- inherited outer Vertex lineage through internal Cut.

## 6. Frozen checkpoint invariants

PM-02D and all later checkpoints must preserve:

```text
Face != Surface
Edge != Curve
Vertex != Point

periodic seam:
  accounted -> yes
  ordinary material semantic Edge -> no

multi-branch Surface pair without semantic discriminator:
  Curve -> Ambiguous

XYZ / length / proximity / provider order:
  never identity

runtime/provider history:
  evidence only
  never persisted
```

The bounded semantic branch/provenance discriminator accepted by the Owner remains permitted only when the producing operation provides defensible semantic meaning. PM-02C does not introduce arbitrary provider branch numbering.

## 7. Next checkpoint

PM-02D is next.

It may consume the completed Body-stage Face/Surface, Edge/Curve and Vertex/Point catalog to implement:

- one atomic topology-aware committed Body presentation;
- View Styles;
- fresh Face/Edge/Vertex picking;
- generation-scoped PresentationTokens;
- Current Feature Contribution overlays;
- topology Properties/inspection.

PM-02D must not reconstruct semantic topology from tessellation or make Viewer/provider tokens durable Part identity.

## Documentation impact

Internal docs: required — this completion record plus ACTIVE/roadmap/Work Contract checkpoint synchronization.  
User/Product docs: not required — PM-02C still introduces no user-facing topology picking or face-supported Sketch workflow.  
Product Browser: not required — no canonical product documentation changes in this checkpoint closure.
