# RG-02A/G0 — Native VERTEX Boundary Characterization Work Contract (OWNER-ACCEPTED; PENDING ACTIVATION)

**Status:** **OWNER-ACCEPTED BOUNDED G0 SCOPE — NOT MERGED / NOT ACTIVATED.** No test or product mutation is authorized until formal governance activation.  
**Owner acceptance:** 2026-10-12 — explicit `akcepyuje - kontynuuj` in answer to the question approving precisely this limited RG-02A/G0 stage for formal activation. The approval covers the G0 evidence-only scope, not a roadmap revision, governance merges, tests, implementation of RG-02 features, or UI design.  
**Date:** 2026-10-12  
**Audited source baseline:** `main` `fa0f8074dc7bb0eb5e2c45ab0fa313644febf55c`; latest accepted governance baseline `main` `108420705aaee9182ca649401e73f1ca23309de2` (roadmap v1.32 merged via PR #330). Revalidate exact `main` immediately before formal activation and any tests.  
**Design input:** `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md`  
**Evidence plan:** `work/RG-02A_VERTEX_NATIVE_G0_MATRIX_DRAFT.md`  
**Governing authority:** `AGENTS.md`, `governance/CONSTITUTION.md`, `governance/FOUNDATION.md`, `governance/ARCHITECTURE.md`, accepted ADRs, `work/ACTIVE.yaml`, `work/PART_MODELING_V1_ROADMAP.md` v1.32, `governance/DOCUMENTATION.md`.  
**Decision class:** Owner-accepted bounded D2 evidence Work Contract text, pending formal activation; no further D2/D3 architecture or CAD product change authorized.

## 1. Goal and why G0 precedes implementation

Determine **which currently visible native Body Vertices are durably addressable using existing Part-v1 three-Surface `FeaturePointAddress` semantics**, across real Extrude/Cut/Revolve/Fillet/Chamfer geometry, selected Body history stages and loss/ambiguity. This is an **evidence-only characterization** of existing runtime behavior; it is **NOT** construction-point, Datum Plane, Axis, reference-schema or user-interface implementation.

The exact architectural boundary to measure: `pointRelationForObservation()` currently recognizes only **exactly three unique resolved semantic Surface sources**. `buildPointStage()` returns `semantically_unsupported` for a native Vertex without a certified relation; meanwhile `PartViewportController::bodyTopologyOrdinaryPickable()` permits some such geometry for ordinary inspection. Distinguish three separate claims in the report: **visible/pickable**, **Part semantic referenceable**, **durable selection capable of surviving rebuild**. None implies the next.

## 2. Only permitted source/test mutations AFTER formal activation

Test-only modifications, limited to the following **already registered** native test source files:

1. **Primary:** `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` — real `part::evaluatePart()` and actual OCCT Extrude/Fillet/Chamfer Body-stage topology, Vertex records and semantically resolved Point witnesses.
2. **Secondary if needed for Revolve:** `tests/kernel_native/pm04c2_production_revolve_test.cpp` — real OCCT Revolve geometry; only add bounded Vertex meaning observations where the actual Part-stage catalog is available through existing test APIs.
3. **Secondary if needed for isolated analytic/kernel evidence:** `tests/kernel_native/pm02c_production_edge_curve_vertex_point_test.cpp` — provider exact current topology/adjacent Surface observations from native Extrude/Cut; distinguish this runtime evidence from Part's semantic Point resolution.

**Documentation/evidence:** this Owner-accepted G0 contract after governance merge/activation, `work/RG-02A_VERTEX_NATIVE_G0_MATRIX_DRAFT.md` for case status, and a new dedicated `work/RG-02A_G0_NATIVE_VERTEX_EVIDENCE.md` as the **actual executed report** (must contain exact tested SHA and CI links or remain explicitly `UNRUN`). Any governance activation/roadmap edits occur in a separate specifically authorized governance change, not implicitly under the test-only implementation package.

**Never edit under G0:** `src/**`, `tests/CMakeLists.txt`, `tests/kernel_native/CMakeLists.txt`, CI/workflows, `adr/**`, `docs/internal/**`, `docs/product/**`, `docs/browser/**`, persistence schemas, serializations, core/kernel/Part APIs, application Commands, Viewer/UI, or current production Work Contracts. An attempted fourth test-source path is a **STOP for Owner amendment**, not an automatic allowed file.

## 3. Exact characterization cases and evidence required

The case names below are from the parent VERTEX matrix; a test should derive each observation from **real OCCT** and the resulting **real Part-stage catalog**, not from a hand-built fake semantic catalog.

| Gate | Case | Required observation |
| --- | --- | --- |
| G0.1 | V01 | Rectangular Extrude; enumerate every native Vertex; expect 8/8 resolved Point witnesses **only if** the native geometry provides unique trihedral meaning. Report actual counts and reasons. |
| G0.1 | V03 | Real Extrude + Cut: generated and inherited intersection Vertices, exact Surface-adjacency count and Part referenceability, including unsupported candidates. |
| G0.1 | V10 | Any **actual** native Vertex with different-than-three semantic Surface sources that remains ordinary Body-pickable; distinguish actual probe evidence from code-only inference. If no such example exists, record **NOT DEMONSTRATED**, not PASS. |
| G0.2 | V04 | Revolve native Vertex, including circular rim and periodic seam/degenerate candidate analysis; do not infer Vertex support from CurveKind or visible edge alone. |
| G0.2 | V05 | Fillet-created or modified corner/transition native Vertex classification. |
| G0.2 | V06 | Chamfer-created or modified corner/transition native Vertex classification. |
| G0.2 | V09 | Same earlier stage before the later edge feature and final stage afterward; prove no accidental use of the final-stage catalog as a source for the earlier stage, without pretending a new Datum/Point source already exists. |
| G0.3 | V02 | Upstream dimension edit: record whether existing three-Surface Point addresses survive and move as intended, where practical through existing test document operations. |
| G0.3 | V07/V08/V12 | Missing, ambiguous and semantic replacement checks against current Part rules; use existing tests as baseline, adding narrowly only when a real native counterpart is necessary for architectural decision. Do not manufacture a "resolved" outcome with geometry matching. |
| G0.3 | V11 | Existing stale evaluation/provider/session evidence; record the verified native regression or a separately identified nonnative freshness control; never claim a token freshness check is proof of a new persisted Datum source. |

**Per Vertex record:** user-visible stage/Feature chain, native Vertex count, adjacent *resolved semantic* Surface-address count (not merely native Face incidence), topology accounting class, `referenceability`, semantic Point candidates, resolved/ambiguous/missing distinction, source/descendant meaning where applicable, whether ordinary viewer picking admits it under existing policy, and whether it could currently be authored as a stable `FeaturePointAddress`. Token values may appear only in ephemeral diagnostics, never as persisted intent.

**Evidence-quality rule:** an existing, accepted unsupported case counts as a *measured limitation*, not as a failed attempt to implement a promised feature. An integrity failure, unaccounted topology, stale token acceptance or silent alias is a **RED/STOP**, not something to reinterpret or automatically repair.

## 4. Execution sequence and exact tests

1. **G0 baseline read-only:** confirm current `main`, active roadmap/contract state, matching native test registration, and historical accepted PM-02P/PM-02C/PM-05C2b tests. Do not copy code from old `PM-02P` **evidence-only** models and mistake it for production behavior.
2. **G0 native RED/measurement:** on a fresh evidence implementation branch from formally activated main (never from an unmerged draft), extend only the named existing test sources; capture actual bounded native cases, with counters and typed statuses. If a case cannot be prepared using allowed files/fixtures without production changes, mark `BLOCKED` and STOP for scope amendment rather than writing hidden scaffolding.
3. **FOCUSED:** use exact registered target/test pairs, e.g. `pm05c2b_part_edge_topology_test` / `pm05c2b.part_edge_topology`, `pm04c2_production_revolve_test` / `pm04c2.production_revolve`, `pm02c_production_edge_curve_vertex_point_test` / `pm02c.production_edge_curve_vertex_point`, with `SS2-Focus-Mode: kernel`. Focus each changed target; re-check current CI focused-commit trailer rules before execution. Do not rewrite CMake or fake PASS results.
4. **Broader verification:** native Windows FOCUSED for altered tests, FAST as required by current CI policy, and final exact-head Windows FULL including docs/aggregate before proposing a production evidence merge. Preserve and report every RED with exact tested SHA and logs.
5. **Evidence report:** publish a source-linked observed matrix showing `RESOLVED`, `MISSING`, `AMBIGUOUS`, `UNSUPPORTED`, `BLOCKED`, or `UNRUN` for each relevant case, and independently whether ordinary viewport picking would admit each class. No unexecuted statement may be labeled `PASS`.
6. **Owner D2 synthesis checkpoint:** explain the **minimal demonstrated gap** in existing `FeaturePointAddress`; recommend preserving three-Surface semantic Points where valid, with only justified bounded extra Vertex identity if needed. **STOP for Owner acceptance of any new semantic/persistence design**. No implementation continuation to MID/CENTER, Datum or Axis is implied.

**Mandatory separation of final gates:** exact-head native FULL evidence, Owner acceptance of G0 findings and next D2 decision, then *separate explicit merge approval*. Do not self-merge or auto-activate another RG-02 stage.

## 5. Hard exclusions and stop conditions

- No new `FeaturePointAddress` variant, no broad `VertexId`, no inferred edge-endpoint references, no persistent XYZ, no schema bump, no "best candidate" heuristics.
- No new `MID/CENTER` exact kernel API under G0: the E01–E08 matrix is a **later** technical stage after G0.
- No Geometry/Datum/Axis/Mirror features; no application draft/CommandLine, selection/hover, stage dropdown, snap markers, Tree UI or Viewer behavior.
- Existing `work/ACTIVE.yaml` still points to completed UI-01. The bounded G0 Work Contract text is now Owner-accepted, but G0 cannot start test mutations until that contract is merged under separate authorization and a dedicated activation on main has been approved and merged. Roadmap **v1.32** already includes the **RG-02A/G0 evidence-only gate before PG-01E**, by separate Owner D2 approval and roadmap-only PR #330 merge. This does **not** activate G0 or introduce broader RG-02 Datum/Axis product scope; future milestones/implementation require separate Owner decisions.
- **Independent RG-02 UI design gate:** before **any** future UI/UX or 3D marker/pick changes, provide Owner-reviewable interface design and obtain a **separate explicit Owner approval**, as mandated by section 3 of `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md`. G0 acceptance is not UI approval.

## Documentation impact

Internal docs: not required

User/Product docs: not required

Generated Browser: **not required** for G0 only.  
Reason: G0 is a native semantic-topology characterization; its authoritative result is the detailed `work/` evidence report. Any later implementation of durable references or user-facing construction tools requires separate internal docs, PL/EN product docs and Browser update.
