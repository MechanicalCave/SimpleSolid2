# RG-02B1 / P4-S4 / P0 — END → Semantic Point native proof evidence

**Status:** IMPLEMENTATION CANDIDATE — NOT VERIFIED / NOT OWNER-ACCEPTED.
**Scope:** strictly proof-only, no Part/schema/GUI/Viewer product change.
**Governance:** accepted P0 contract PR #390; separately authorized ACTIVE-only PR #391 guarded squash-MERGED to `main` as `c06bafbc29c693106b3c1ff765ee4adb1921c766`.
**Test:** existing registered `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp`, additive S4-P0 witness.
**First test change commit:** `7254b1f555f14bea4e52f2a1bc31da1478d13602`.

## Hypothesis and failure semantics

The canonical real OCCT Extrude box has twelve independently semantic material Edges with 24 distinct **Edge-local** END tuples. The P0 positive proposition is stronger: these tuples must collapse to **exactly eight**, each independently `Resolved`, stage-qualified `FeaturePointAddress` keys. Each strict Point must map to one current native Vertex and be reached by exactly three distinct incident Edge-local END selectors. Equality of XYZ, OCCT token, provider index or proximity NEVER selects or persists a semantic Point.

If any canonical current END is not unambiguously certified, this is **P0 FAIL / Owner D2 STOP**, not a reason to choose an incident Edge, allow a false Resolved, change provider API or persist XYZ.

## Additive native witness, not yet executed

`verifyS4P0NativeEndPointIdentity` checks each `SelectedMaterialEdgeEnd::terminal.certified_semantic_point` against the current stage's `FeaturePointResolution` (Resolved, one native Vertex), uniquely certified `BodyVertexTopologyRecord`, and transient incident material Edge. It checks 24→8, three END uses per key, and no false native-Vertex alias in the same current evaluation. The only persistent *candidate* meaning is `(BodyStageRef, FeaturePointAddress)`; there is no new authored datum, source, API or schema.

The witness runs on these real Part evaluations, reusing preexisting Q1 test fixtures:

| Test invocation | Checkpoint | Claimed outcome |
|---|---|---|
| `P0-01-pristine` | Box, 12 Edge / 24 END / 8 candidate Points | **PENDING NATIVE EXECUTION** |
| `P0-03-independent-OCCT` | Cold reconstructed Part + independent OCCT provider, same semantic key set | **PENDING NATIVE EXECUTION** |
| `P0-03-same-feature-height-edit` | Producer FeatureId retained, height changed, same semantic key set | **PENDING NATIVE EXECUTION** |

Geometry-only diagnostics distinguish three noncollinear distinct box corners, a duplicate source and a collinear diagnostic triple. These checks DO NOT create a Datum frame, authorize a tolerance or manufacture a new Point.

## Negative coverage retained from this same registered test

Existing Q1/Q2 cases are preserved, not weakened:

- Real whole-cylinder unsupported Points and half-cylinder Cut (4 Ambiguous, 2 Unsupported Points): Q1 END authoring fails closed without Edge-incidence promotion.
- Real partial Revolve END cases: only strictly resolved terminal Points may author; provider arc/line class is not point identity.
- Real Fillet/Chamfer generated vertices: unsupported/ambiguous Points cannot be promoted by a material Edge.
- Synthetic current real-stage catalog variants: Ambiguous, Missing and Unsupported Point statuses produce typed non-resolved results with no native observations.
- Wrong DocumentId, foreign Body, stale revision/stage and replaced FeatureId / equal-XYZ decoy are rejected by the existing Q1 tests.
- Independent-provider cold replay and source-preserving feature-height edit are already covered by earlier Q1/G0/E0 assertions; P0 adds a direct 24→8 semantic-key assertion.

These are existing checks in the test source; **no new CI PASS is claimed merely from their presence**.

## Execution gates

| Gate | Evidence |
|---|---|
| Windows native FOCUSED | PENDING |
| Windows FAST | PENDING |
| Windows Ready FULL (Core / Kernel / Desktop) | PENDING |
| Exact source/tests/evidence SHA and red history | PENDING |
| Owner P0 technical PASS/FAIL | PENDING |
| Separately authorized production squash | PENDING |

**STOP:** On a failing native Point uniqueness/provenance witness, preserve RED evidence and request Owner D2. Do not modify `src/**`, add APIs, add Part v18, implement `3 Points` GUI, change global hover, touch CMake/CI or mark P0 completed.

**DOC-01:** test/evidence-only impact; internal/product/browser documentation changes are not required by the accepted P0 contract.
