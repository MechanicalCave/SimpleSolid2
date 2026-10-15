# RG-02B1 / P4-S4 / P0 — END → Semantic Point native proof evidence

**Status:** P0 NATIVE AUTOMATED PASS + OWNER TECHNICAL FINAL PASS (2026-10-15); OWNER AUTHORIZED GUARDED EXACT-HEAD SQUASH. Actual merge/production closeout must still be independently verified.
**Scope:** strictly proof-only, no Part/schema/GUI/Viewer product change.
**Governance:** accepted P0 contract PR #390; separately authorized ACTIVE-only PR #391 guarded squash-MERGED to `main` as `c06bafbc29c693106b3c1ff765ee4adb1921c766`.
**Test:** existing registered `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp`, additive S4-P0 witness.
**First test change commit:** `7254b1f555f14bea4e52f2a1bc31da1478d13602`.

## Hypothesis and failure semantics

The canonical real OCCT Extrude box has twelve independently semantic material Edges with 24 distinct **Edge-local** END tuples. The P0 positive proposition is stronger: these tuples must collapse to **exactly eight**, each independently `Resolved`, stage-qualified `FeaturePointAddress` keys. Each strict Point must map to one current native Vertex and be reached by exactly three distinct incident Edge-local END selectors. Equality of XYZ, OCCT token, provider index or proximity NEVER selects or persists a semantic Point.

If any canonical current END is not unambiguously certified, this is **P0 FAIL / Owner D2 STOP**, not a reason to choose an incident Edge, allow a false Resolved, change provider API or persist XYZ.

## Additive native witness — executed on real Windows OCCT

`verifyS4P0NativeEndPointIdentity` checks each `SelectedMaterialEdgeEnd::terminal.certified_semantic_point` against the current stage's `FeaturePointResolution` (Resolved, one native Vertex), uniquely certified `BodyVertexTopologyRecord`, and transient incident material Edge. It checks 24→8, three END uses per key, and no false native-Vertex alias in the same current evaluation. The only persistent *candidate* meaning is `(BodyStageRef, FeaturePointAddress)`; there is no new authored datum, source, API or schema.

The witness runs on these real Part evaluations, reusing preexisting Q1 test fixtures:

| Test invocation | Checkpoint | Claimed outcome |
|---|---|---|
| `P0-01-pristine` | Box, 12 Edge / 24 END / 8 strictly Resolved Points | **PASS** in #3094 and #3095 |
| `P0-03-independent-OCCT` | Cold reconstructed Part + independent OCCT provider, same semantic key set | **PASS** in #3094 and #3095 |
| `P0-03-same-feature-height-edit` | Producer FeatureId retained, height changed, same semantic key set | **PASS** in #3094 and #3095 |

Geometry-only diagnostics distinguish three noncollinear distinct box corners, a duplicate source and a collinear diagnostic triple. These checks DO NOT create a Datum frame, authorize a tolerance or manufacture a new Point.

## Negative coverage retained from this same registered test

Existing Q1/Q2 cases are preserved, not weakened:

- Real whole-cylinder unsupported Points and half-cylinder Cut (4 Ambiguous, 2 Unsupported Points): Q1 END authoring fails closed without Edge-incidence promotion.
- Real partial Revolve END cases: only strictly resolved terminal Points may author; provider arc/line class is not point identity.
- Real Fillet/Chamfer generated vertices: unsupported/ambiguous Points cannot be promoted by a material Edge.
- Synthetic current real-stage catalog variants: Ambiguous, Missing and Unsupported Point statuses produce typed non-resolved results with no native observations.
- Wrong DocumentId, foreign Body, stale revision/stage and replaced FeatureId / equal-XYZ decoy are rejected by the existing Q1 tests.
- Independent-provider cold replay and source-preserving feature-height edit are already covered by earlier Q1/G0/E0 assertions; P0 adds a direct 24→8 semantic-key assertion.

These are existing checks in the same registered native test, whose entire executable completed PASS in both #3094 and #3095; no separate individual failure-case transcript is invented.

## Execution gates — exact Windows evidence

| Gate | Evidence |
|---|---|
| Windows native FOCUSED | [#3094](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38695881121) **PASS 1/1**; exact head `a8e46fe80b43ed158b7b4e24fdf25955380a5aac`; real `pm05c2b.part_edge_topology` Windows OCCT 0.31 s. |
| Windows FAST | [#3093](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38695707741) completed SUCCESS on evidence HEAD `669c99ca80dfc246371b1008b8094c0946fcf0bf`; FOCUSED/FULL skipped. This is NOT native proof. |
| Windows Ready FULL (Core / Kernel / Desktop) | [#3095](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38696069495) **PASS 199/199** at the SAME exact `a8e46fe80b43ed158b7b4e24fdf25955380a5aac`; Core **25/25**, Kernel-native **57/57** (including `pm05c2b.part_edge_topology`), Desktop **117/117**, Windows aggregate **SUCCESS**. |
| Exact source/tests/evidence SHA and red history | Source/test SHA `7254b1f555f14bea4e52f2a1bc31da1478d13602`; pre-closeout tested HEAD `a8e46fe80b43ed158b7b4e24fdf25955380a5aac`. First draft Windows FAST #3093 PASS applies to predecessor evidence SHA `669c99ca80dfc246371b1008b8094c0946fcf0bf`; no known P0 RED run. Final ledger-only closure HEAD will be the SHA of this Owner-acceptance ledger sync; apply exact-head Windows closure before protected-main merge. |
| Owner P0 technical PASS/FAIL | **PASS** — Owner explicitly accepted this contract and granted all formal permissions needed to complete it on 2026-10-15; recorded in [PR #392 acceptance comment](https://github.com/MechanicalCave/SimpleSolid2/pull/392#issuecomment-6186828989). This is aggregate technical acceptance, not a fabricated per-fixture Owner transcript. |
| Separately authorized production squash | **OWNER AUTHORIZED** with exact-head guard and all applicable green CI. **Not yet executed** as of this evidence entry; verify GitHub merger state separately. |

**Interpretation:** P0 proves the declared **canonical box END family**, cold independent-provider re-evaluation, source-preserving dimension edit, strict trihedral semantic deduplication, and the preexisting typed negative fixture guards. It **does not prove universal Point support** for all material edges (real partial Revolve, half-cylinder, generated Fillet/Chamfer include explicitly limited families), a persisted v18 source, a shipping 3-Points Datum UI or manual GUI acceptance. Each unsupported family must stay excluded/fail-closed until a separately approved D2 contract establishes otherwise.

**STOP:** On a failing native Point uniqueness/provenance witness, preserve RED evidence and request Owner D2. Do not modify `src/**`, add APIs, add Part v18, implement `3 Points` GUI, change global hover, touch CMake/CI or mark P0 completed.

**DOC-01:** test/evidence-only impact; internal/product/browser documentation changes are not required by the accepted P0 contract.
