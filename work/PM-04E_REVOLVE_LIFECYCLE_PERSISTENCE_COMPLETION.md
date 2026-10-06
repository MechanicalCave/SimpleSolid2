# PM-04E — Revolve Integrated Lifecycle / Repair / Persistence Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Checkpoint:** PM-04E — integrated lifecycle / repair / persistence  
**Date:** 2026-10-06

## 1. Runtime evidence

### PM-04E1 — native persistence / schema v13

- exact candidate: `dad92ce54fb0cf69ae96b7210d5f49bfce3f9402`;
- Windows FULL #1560: PASS;
- final `windows-msvc` aggregate: PASS;
- merged main: `a559f7b2d6a667e4d4b34cbfd7af6f2d93cd65e1` (#270).

E1 delivered:

- native Part schema v13;
- durable `RevolveFeature` persistence beside existing Extrude Features;
- provider-neutral `ProfileId + AxisReference + Add/Cut + extent + angle + OneSide Reverse`;
- explicit persisted angular unit `angle_rad`;
- built-in Origin X/Y/Z AxisReference persistence without synthetic AxisId;
- authored AxisReference validation against AxisId high-water;
- deleted-but-previously-allocated AxisId remains legal repairable Missing intent;
- never-allocated AxisId fails closed;
- schemas v1-v12 remain readable;
- a Revolve record mislabeled as legacy v12 fails closed;
- Save/Close/Reopen reconstructs durable Revolve meaning;
- malformed/out-of-contract persisted Revolve state is rejected.

Exact-head FULL evidence included:

- core-only: 25/25 PASS;
- kernel-native Release: 47/47 PASS;
- complete desktop CTest: 103/103 PASS;
- dedicated `pm04e1.revolve_persistence`: PASS.

### PM-04E2 — integrated lifecycle / repair / cold rebuild

- exact candidate: `bdf87a29b5cdc27c608023af863e32c44dc55ef1`;
- Windows FULL #1562: PASS;
- final `windows-msvc` aggregate: PASS;
- merged main: `e9a1056da4ec3639458aa1cdf4faacfc1621fcc9` (#271).

E2 proved through existing semantic commands, with no new production API:

- true cold rebuild from durable authored state using a fresh provider/runtime generation;
- authored Axis source geometry edit recomputes downstream Revolve;
- source Line deletion leaves Axis intent and blocks Revolve as AxisUnavailable;
- explicit Axis re-source repairs the same AxisId;
- Undo/Redo toggles exact broken/repaired Axis intent;
- Delete referenced Axis leaves the Revolve's durable AxisId reference and produces MissingAxis;
- Undo restores the same AxisId and repairs the consumer;
- Delete Profile leaves durable MissingProfile intent and Undo restores the same ProfileId;
- generic Feature Suppress/Unsuppress applies to Revolve;
- generic Feature Delete/Undo/Redo preserves/restores exact FeatureId;
- Save/Reopen after repair preserves authored IDs;
- broken dependency states issue zero modeling-kernel calls, publish no current Body solid/topology and never reuse stale last-good geometry.

Exact-head FULL evidence included:

- core-only: 25/25 PASS;
- kernel-native Release: 47/47 PASS;
- complete desktop CTest: 104/104 PASS;
- dedicated `pm04e2.revolve_lifecycle_cold_rebuild`: PASS.

The initial E2 candidate failed only because the test fixture default-constructed durable ID types that intentionally have no default constructor. The production implementation was not weakened; the fixture was corrected to construct the ID bundle atomically, after which the exact candidate above passed FULL.

### PM-04E3 — Profile source and Sketch support edits

- exact candidate: `b7dcd55ab9c04fdf208a534f6e09de7ffac0e00f`;
- Windows FULL #1563: PASS;
- final `windows-msvc` aggregate: PASS;
- merged main: `1a7e1228a91b23f89bf13f6bb5c07c8abe9afae3` (#272).

E3 extended the integrated lifecycle evidence, again with no production behavior change:

- source Profile geometry edit recomputes the current Revolve input;
- ProfileId, AxisId and FeatureId remain stable;
- re-support of the common Profile/Axis source Sketch through the existing Part support command recomputes both current world meanings together;
- SketchId, EntityIds and local U/V geometry remain stable across re-support;
- Revolve recomputes from the new Profile/Axis world meaning;
- Undo/Redo of re-support restores exact prior/current semantic Revolve kernel inputs;
- Save/Reopen after Profile/support edits preserves IDs and support intent;
- a fresh provider generation reproduces the same semantic Revolve input after reopen.

Exact-head FULL evidence included:

- core-only: 25/25 PASS;
- kernel-native Release: 47/47 PASS;
- complete desktop CTest: 104/104 PASS;
- expanded `pm04e2.revolve_lifecycle_cold_rebuild`: PASS.

## 2. Reused accepted lifecycle evidence

PM-04E does not create duplicate dependency machinery.

The remaining contract gates are already covered by accepted lower-layer evidence and are consumed unchanged by Revolve:

- PM-03E Datum-backed Sketch lifecycle proves support-chain edits, upstream Datum changes, loss of required Body support, fail-closed behavior, no stale world-frame reuse and cold semantic reconstruction;
- PM-02I proves semantic Surface-backed Sketch support loss/ambiguity, exact re-support repair and no geometry-similarity rebinding;
- PM-04C proves the bounded Axis/Sketch/Body-stage cycle rule rejects cyclic Revolve authored state before mutation.

E3 adds the missing Revolve-specific bridge: a Profile and authored Axis sourced from the same Sketch remain semantically coherent through source geometry and support edits while durable IDs do not change.

## 3. PM-04E contract matrix

The PM-04E deliverables are closed:

- upstream Axis edit — E2 PASS;
- upstream Profile edit — E3 PASS;
- upstream Sketch support edit — E3 PASS;
- Missing/Blocked repair — E2 plus PM-02I/PM-03E PASS;
- Suppress/Delete — E2 PASS;
- Undo/Redo — E2/E3 PASS;
- stage-cycle rejection — PM-04C PASS;
- Save/Close/Reopen — E1/E2/E3 PASS;
- true cold rebuild with fresh runtime/provider tokens — E1/E2/E3 PASS.

The PM-04E gates are also closed:

- no stale last-good modeling — explicit zero-kernel-call/unavailable-Body evidence in E2 plus accepted support-chain evidence;
- all authored IDs stable/non-aliasing — E1/E2/E3 PASS;
- broken intent remains repairable exactly as contracted — E2/E3 PASS;
- persistence reconstructs semantic meaning without provider continuity — E1/E2/E3 PASS.

## 4. Architecture boundary

PM-04E does **not** add or authorize:

- Datum Axis;
- Body Edge/Curve as AxisReference;
- provider or Viewer identity in durable state;
- geometry-similarity/proximity rebinding;
- persisted provider periodic seam identity;
- a global dependency graph;
- multi-Body semantics;
- Projection as a prerequisite;
- skew/non-coplanar general sweep semantics;
- multi-turn Revolve;
- a new numerical tolerance policy.

No PM-04 STOP condition was triggered.

## 5. Result

PM-04E is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-04F — documentation / Product Browser / Owner Windows acceptance** under the unchanged Owner-accepted PM-04 Work Contract.
