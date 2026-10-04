# PM-02A — Production Body-Stage Topology Catalog Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Runtime candidate:** `58368a6ada5f6c30e03cb845682dbf5408ecb763`  
**Windows gate:** FULL #1397 — PASS on the exact runtime candidate  
**Merged runtime main:** `b95f72105b8242eedce9cd79dd8bf910f3ccb715`  
**Scope boundary:** PM-02A only; no Surface/Curve/Point semantic promotion, Viewer topology picking, schema v9 or face-supported Sketch

## 1. Closure statement

PM-02A is COMPLETED — PASS.

The production runtime now has a provider-neutral, disposable Body-stage topology catalog foundation for every successful current Extrude Add/Cut stage. The checkpoint closes complete Face/Edge/Vertex accounting and typed runtime topology tokens without promoting provider identity into durable Part identity.

This closure does not claim completion of PM-02. The next active checkpoint is PM-02B — Surface/Face production semantics.

## 2. Delivered production boundary

Kernel/provider delivery:

- `RuntimeFaceToken`, `RuntimeEdgeToken` and `RuntimeVertexToken` are typed runtime-only tokens;
- `SolidModelingResult` publishes complete current Face/Edge/Vertex inventory plus unique current counts;
- the OCCT provider enumerates unique current subshapes with `TopExp::MapShapes`;
- existing PM-01 semantic Face lineage remains a semantic subset rather than becoming raw topology identity;
- Resolved semantic Face tokens are reused by the complete Face inventory for the same stage;
- current topology tokens are never serialized.

Part delivery:

- `BodyStageRef` distinguishes stage meaning equivalent to `EmptyBody` / `AfterFeature(FeatureId)`;
- `BodyStageTopologyCatalog` owns provider-neutral stage accounting;
- every successful Feature evaluation retains the same-revision runtime solid and topology catalog for that exact stage;
- final `current_topology` is published only for an UpToDate final Body;
- a same-revision `resolved_prefix_topology` may exist only as diagnostic/presentation prefix after downstream failure and is not final Body truth;
- Face accounting is Referenceable only when exactly one existing Resolved semantic Face claim owns the current runtime Face token;
- topology without defended semantic promotion is explicitly SemanticallyUnsupported rather than silently omitted;
- inventory count/token mismatch or contradictory Resolved Face claims fails closed as `topology_integrity_failure` before the candidate solid becomes Body truth.

## 3. Verification evidence

The exact runtime candidate passed Windows FULL #1397, including:

- complete desktop test graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST and SUBSYSTEM selector verification;
- full test execution;
- existing SR-02 and CI-04 evidence lanes.

PM-02A-specific regression evidence:

- `pm01c.feature_evaluator` verifies per-stage Part catalogs, final-vs-prefix topology publication and corrupted-inventory fail-closed admission;
- `pm02a.production_topology_catalog` verifies production OCCT complete inventory;
- canonical rectangular Extrude inventory is exactly 6 Faces / 12 Edges / 8 Vertices;
- every published typed runtime token is valid and unique within its topology kind;
- every Resolved semantic Face lineage token occurs exactly once in the complete current Face inventory;
- chained Add and Cut results publish complete current inventory.

## 4. Invariant result

PM-02A closes the following production invariant for the current Extrude Add/Cut universe:

```text
provider unique Faces    == catalog Faces
provider unique Edges    == catalog Edges
provider unique Vertices == catalog Vertices
silent omission          == 0
runtime topology persisted == 0
```

Semantic promotion remains deliberately incomplete at this checkpoint:

- Surface/Face carrier semantics belong to PM-02B;
- Edge/Curve and Vertex/Point semantic promotion belongs to PM-02C;
- Viewer presentation tokens and stale-pick generation authority belong to PM-02D.

## 5. Next checkpoint

PM-02B is next and may build only on this completed accounting foundation.

PM-02B must add production Surface semantic keys for the current Extrude Add/Cut universe, strict Face realization status, inherited/trimmed/split/delete/Cut-exposed behavior and exact planar Surface classification/frame while preserving zero false Resolved.

## Documentation impact

Internal docs: required — this completion record plus ACTIVE/roadmap/Work Contract checkpoint status synchronization.  
User/Product docs: not required — PM-02A introduces no user-facing topology-selection or face-supported Sketch behavior.  
Product Browser: not required — no canonical product documentation changes in this checkpoint closure.
