# PM-02B — Surface/Face Production Semantics Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Initial production runtime:** `4ae04fd28124d0d165941c10245c8ecdbc0cdc43` — Windows FULL #1399 PASS  
**Final runtime candidate:** `05434516fea0892aac2babae09da772043987f29` — Windows FULL #1400 PASS  
**Merged final runtime main:** `65d074577bc89e3507288118b17e99b12288a881`  
**Scope boundary:** PM-02B only; no Edge/Curve or Vertex/Point semantic promotion, Viewer topology picking, schema v9 or face-supported Sketch

## 1. Closure statement

PM-02B is COMPLETED — PASS.

The production model now distinguishes strict bounded Face identity from semantic Surface carrier identity for the accepted Extrude Add/Cut universe. A semantic Surface may remain Resolved across trim/split while strict Face realization changes independently.

The final checkpoint candidate is `05434516fea0892aac2babae09da772043987f29`, not the earlier #1399 candidate, because the final candidate also closes survival-matrix edit cases and removes stale current-frame publication from unresolved Surface states.

The next active checkpoint is PM-02C — Edge/Curve and Vertex/Point semantic catalog.

## 2. Delivered production boundary

Kernel/provider delivery:

- `RuntimeSurfaceToken` is a typed runtime-only carrier token and is never serialized;
- provider-neutral `SurfaceKind` classifies Plane, Cylinder, Cone, Sphere, Torus and Other;
- `NewSurfaceLineage` / `InheritedSurfaceLineage` are distinct from strict Face lineage;
- one semantic Surface may own one or more current Face realizations;
- Surface status and strict Face status are published separately;
- current Face realizations are mapped by typed runtime Face tokens;
- provider surface type corroborates the semantic generator type and cannot create identity;
- shared current Face descendants across incompatible Surface claims remain Ambiguous;
- Missing/Ambiguous/Unsupported Surface resolutions publish no current canonical frame;
- geometry equality never recreates a deleted semantic Surface.

Deterministic planar carrier frames:

- Extrude cap:
  `O = source O + semantic offset * source N`, with source `U/V/N` preserved;
- planar side from authored Line:
  `O = authored Line start in source frame`,
  `U = normalized authored Line direction`,
  `V = source support/Profile N`,
  `N = U x V`;
- OneSide Reverse and Midplane do not silently mirror the side frame;
- Boolean trim/split preserves carrier frame;
- Cut-exposed planar tool Surface uses tool semantic provenance/frame;
- provider UV, current trim loop, Face centroid, first Edge, traversal order, camera and tessellation are not frame authority.

Part delivery:

- `FeatureSurfaceAddress` is distinct from `FeatureFaceAddress`;
- `FeatureSurfaceResolution` publishes carrier status, strict-Face status, classification, current realizations and current frame only when Resolved;
- `BodyStageTopologyCatalog` contains Surface resolution records separately from Face records;
- each current supported Extrude Add/Cut Face maps to explicit semantic Surface candidate(s);
- a split Surface may map to several current Faces while remaining one Resolved semantic carrier;
- a current Face shared by incompatible Surface claims exposes multiple candidates rather than an arbitrary winner;
- inherited Surface kind/frame contradiction is a topology-integrity failure;
- final Surface references are published only for the current UpToDate Body; failed/unavailable Body cannot expose stale final Surface truth.

## 3. Survival-matrix result

The PM-02P Surface/Face matrix is covered in production semantics/tests for the current Extrude Add/Cut universe:

| Matrix event | Production result |
| --- | --- |
| ProfileCap extent edit | Resolved; semantic cap frame deterministic |
| ExtentCap extent edit | Resolved; origin follows semantic offset |
| Midplane Negative/Positive caps | Resolved with signed semantic offsets |
| planar Line side length edit | Resolved; authored Line direction remains frame provenance |
| Profile/support translation | Resolved; carrier frame translates semantically |
| legal loop traversal reversal | Resolved; no semantic frame flip |
| attached Add trim | strict Face singular/Resolved; Surface Resolved |
| Cut trim | strict Face singular/Resolved; Surface Resolved |
| one Face split into two | strict Face Ambiguous; Surface Resolved; carrier frame unchanged |
| complete deletion | Face/Surface Missing; no current frame |
| unrelated identical plane later appears | old Surface remains Missing |
| replacement from new provenance | new Surface Resolved with new runtime identity |
| two semantic claims collapse with no winner | both Surface/Face meanings Ambiguous; no current frame |
| independent survivor / removed claim | Resolved / Missing |
| Cut-exposed planar tool Surface | Resolved with tool provenance/frame |
| cylindrical Extrude side | Surface Resolved, Cylinder, no planar frame |
| cold provider reconstruction | semantic roles/status/kind/frame repeat without runtime-token authority |

## 4. Verification evidence

Final exact-head Windows FULL #1400 passed on `05434516fea0892aac2babae09da772043987f29`.

The FULL lane passed:

- complete desktop test graph build;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST and SUBSYSTEM selector verification;
- full desktop test execution;
- SR-02 latency benchmark evidence;
- CI-04 warm FULL parity evidence;
- CI-04 comparative timing evidence.

PM-02B-specific production regressions include:

- `pm02b.production_surface_lineage`;
- `pm01c.feature_evaluator`.

The final regression set verifies zero arbitrary Face/Surface winner selection, explicit split semantics, deleted-Surface no-frame behavior, edit/traversal stability, cold reconstruction and Part-level alias handling.

## 5. Frozen checkpoint invariants

PM-02C and all later checkpoints must preserve:

```text
Face != Surface

Surface split:
  strict Face -> Ambiguous
  Surface     -> Resolved

unresolved Surface:
  current canonical frame -> none

geometry equality/proximity:
  never establishes Resolved

provider/runtime token:
  never durable identity
```

PM-02C may consume Surface semantics as adjacency/provenance authority. It may not weaken or reinterpret them to simplify Edge/Vertex classification.

## 6. Next checkpoint

PM-02C is next.

It must promote the already-evidenced Edge/Curve and Vertex/Point semantics into the same production Body-stage catalog while preserving:

- complete Edge/Vertex accounting from PM-02A;
- Surface carrier authority from PM-02B;
- periodic seam as KnownRepresentationArtifact;
- material Edge semantic meaning from Surface adjacency and operation provenance;
- bounded branch/provenance discrimination only when semantically defensible;
- no provider-order/length/XYZ/nearest-geometry identity;
- Point/Vertex meaning from semantic relationships, never coordinate identity.

## Documentation impact

Internal docs: required — this completion record plus ACTIVE/roadmap/Work Contract checkpoint synchronization.  
User/Product docs: not required — PM-02B still introduces no user-facing topology picking or face-supported Sketch workflow.  
Product Browser: not required — no canonical product documentation changes in this checkpoint closure.
