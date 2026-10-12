# RG-02A — VERTEX native G0 evidence matrix (DRAFT)

**Status:** OWNER-ACCEPTED AND ACTIVATED G0 EVIDENCE PLAN — PARTIALLY EXECUTED; source-native tests have run; final evidence matrix / FULL / Owner D2 synthesis are NOT CLOSED  
**Date:** 2026-10-12  
**Audited source baseline:** `main` `fa0f8074dc7bb0eb5e2c45ab0fa313644febf55c`; roadmap v1.32 subsequently merged to `main` as `108420705aaee9182ca649401e73f1ca23309de2` (revalidate at G0 activation).  
**Design authority:** `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md`  
**Audit:** `work/RG-02A_REFERENCE_FOUNDATION_AUDIT_DRAFT.md`  
**Authorization:** Owner accepted bounded RG-02A/G0 evidence-only Work Contract scope on 2026-10-12, roadmap v1.32 was merged through PR #330, the contract was merged through PR #328, and the dedicated activation [PR #331](https://github.com/MechanicalCave/SimpleSolid2/pull/331) was Owner-approved and merged to `main` at `351840b95b58bdbe7b17085f67b3ddd2cc2cf8c7`. The sole active authority is the bounded test/evidence contract, **not** broader RG-02 production/UI work. No RG-02 UI changes before separate Owner UX consultation and explicit approval.

## 1. Hard evidence found in current implementation

- `src/part/feature_evaluation.cpp::pointRelationForObservation` returns a point relation **only if exactly three distinct resolved semantic Surface addresses** are adjacent to the native Vertex. A native Vertex with 2, 4+, unknown or unresolved adjacent semantic Surfaces does not currently become a `FeaturePointAddress` (the function returns `nullopt`).
- `src/part/feature_evaluation.cpp::buildPointStage` classifies such observations without a recognized relation as `TopologyAccountingClass::semantically_unsupported` / `ReferenceStatus::unsupported`. Resolved 3-Surface meanings remain distinguished from ambiguous aliases and missing inherited descendants.
- `src/part/feature_evaluation.cpp::semanticPointForRuntimeVertex` requires an exact one-to-one current resolved meaning and current runtime Vertex, rejecting unsuitable/ambiguous points. This helper is currently used as part of Edge Feature generated-surface provenance; it is not a general public Part authoring API for all 3D datum-point selections.
- `src/ui/part_viewport_controller.cpp::bodyTopologyOrdinaryPickable` admits ordinary nonartifact, nonintegrity-failed native Vertices without requiring `referenceability == resolved`. Thus **selectable/pickable != authorable as durable VERTEX**, by design.
- `tests/pm05b2_strict_edge_resolver_test.cpp` already checks strict Edge singular, split, point-pair branch and wrong-stage rejection, but mostly uses constructed semantic catalogs. `tests/pm02p_e_core_lifecycle_test.cpp` checks runtime token freshness, not the ability to persist every native Vertex.
- `work/PM-02P_REFERENCE_SURVIVAL_MATRIX.md` already mandates that XYZ, provider order, radius, nearest distance etc. cannot independently establish semantic identity.

Consequently a new general construction-point source **cannot** just wrap any `BodyVertexPresentationPoint`. Treating every visible Vertex as durably referenceable would be a false promise.

## 2. G0 native VERTEX classification matrix

For each case, record ALL of: Feature sequence, selected source Body stage, current native Vertex, provider adjacent Face/Surface inventory, canonical semantic Surface relations and cardinality, `BodyVertexTopologyRecord.accounting_class`, `referenceability`, `point_candidates` count, whether an exact one-to-one `FeaturePointResolution` exists, and whether the native vertex was ordinarily pickable. Retain a fixture recipe and deterministic native test trace without persisting any runtime token.

| Case | Native model / intended target | Question to prove | Current contract expectation (not a claimed runtime result) |
| --- | --- | --- | --- |
| V01 | Simple rectangular Extrude, corner shared by three planar side/cap carriers | Can existing `FeaturePointAddress` certify one real native prism Vertex? | Candidate should be Resolved **if** native/semantic adjacency is exactly three and one-to-one |
| V02 | Same Extrude, edit dimensions/offset upstream; inspect same authored Body stage | Does exact semantic Vertex remain identified while XYZ moves? | Preserve semantic identity when provenance and certified relation survive; no XYZ matching |
| V03 | Box then Cut; target new intersection/end Vertex | Are new Boolean-generated Vertices expressible as a unique 3-Surface Point? | Observe, do not assume all Cut Vertices are 3-Surface/referenceable |
| V04 | Revolve of a profile yielding cylindrical and planar caps; target a visible native Vertex adjacent to a circular boundary or seam | Are 2-Surface or seam-related Vertices pickable but semantically Unsupported? | Probe actual native inventory; no assumption that the full circle has a usable semantic endpoint |
| V05 | Fillet on an admissible Edge; target newly generated corner/transition Vertex | Does intersection have exactly three semantic Surfaces, more than three, or unresolved carriers? | Observe; do not use proximity to assign a surviving previous identity |
| V06 | Chamfer on an admissible Edge; target newly generated endpoint/corner Vertex | Is new Vertex definable with strict one-to-one provenance? | Observe 2/3/4+ Surface and alias cases, not just screen position |
| V07 | Upstream resolved 3-Surface Vertex subsequently removed by a Feature in the referenced stage | Does old authored semantic meaning become Missing rather than attach to nearest replacement? | Missing/no geometry fallback |
| V08 | Two native Vertices share an apparent spatial position or a Point relation has multiple current realizations | Is ambiguity reported rather than choosing first/nearest? | Ambiguous/fail-closed |
| V09 | Same semantic source at stage before Chamfer vs final stage after Chamfer | Can explicitly stage-bound identity and different realizations coexist without rebasing earlier-stage source? | Earlier-stage identity stays at its declared stage; final-stage result resolved separately |
| V10 | Native Vertex with non-three-Surface semantics but otherwise ordinary pickability | Is it visible/pickable yet unsupported as durable current `FeaturePointAddress`? | Yes at contract/API level; demonstrate concrete native example before concluding which end-user cases are affected |
| V11 | Rebuild, provider teardown, reopening session, numeric runtime Vertex token reuse | Does stale transient native evidence fail and semantic address remain authoritative? | Reject stale tokens; only reauthor from current stage |
| V12 | Remove/recreate same-looking Vertex at same XYZ but with unrelated semantic producer | Does prior source remain Missing? | Must never geometry-rebind |

**Important:** G0 is a *characterization* gate, not a directive to make all cases GREEN by modifying identity logic. Observed native outcomes determine the next D2 architecture choice. Mark status `UNRUN` until native execution; do not assert current runtime failures without evidence.

## 2A. Observed native G0 run status — 2026-10-12 (NOT FINAL)

The initial V01–V12 table above remains a *test design*; results and precise per-case limits live in [`work/RG-02A_G0_NATIVE_VERTEX_EVIDENCE.md`](RG-02A_G0_NATIVE_VERTEX_EVIDENCE.md) on [Draft PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332). No numerical status histogram can be inferred from the default passing CTest transcript; any stated counts/bounds are proved by explicit passing native assertions at the linked SHA.

| Case | Verified status | Observation / missing work |
| --- | --- | --- |
| V01 | PASS, bounded native prism | 8 of 8 Vertex Point addresses uniquely Resolved through three semantic Surfaces |
| V02 | PASS, bounded edit | All 8 addresses survive Extrude height 20 → 35; 4 Z moves +15, 4 stationary; no coordinate matching |
| V03 | PARTIAL, assertions PASS | Native Add→Cut retains ≥8 inherited Point addresses and creates ≥1 new; exact statuses and adjacency histogram not captured |
| V04 | PARTIAL, invariants PASS | 90° and 360° native Revolve provider/Part catalogs valid and all Resolved witnesses unique; missing exact 2/3/other distribution |
| V05 | PARTIAL, invariants PASS | Fillet real native catalog accounts all vertices; exact classified distribution missing |
| V06 | PARTIAL, invariants PASS | Chamfer real native catalog accounts all vertices; exact classified distribution missing |
| V07 | PASS, bounded removed corner | At least one old semantic Point becomes Missing with zero current native candidates |
| V08 | PASS, genuine ambiguous geometry | Half-cylinder Cut yields ≥1 three-Surface semantic Point with multiple current native Vertices, correctly Ambiguous |
| V09 | PASS, bounded stage identity | Pre-Chamfer stage retains all 8 original semantic Point addresses; later-stage identity is distinct |
| V10 | PASS for native and policy, not UI interaction | Real extruded cylinder exposes ≥1 native `semantically_unsupported` Vertex with no `FeaturePointAddress` but acceptable under existing ordinary accounting-class pick filter |
| V11 | PARTIAL, semantic cold in-memory rebuild PASS | Independent OCCT kernel rebuild reconstructs all 8 semantic addresses; stale UI selection generation and future RG-02 file persistence are NOT proved |
| V12 | PASS, bounded provenance replacement | All 8 same-XYZ regenerated Vertices have different semantic producer; old addresses not reused |

**Exact Windows focused evidence:** V01 [#38304772610](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38304772610), V03 [#38306302187](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306302187), V04 [#38306630003](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306630003), V02/V09 [#38307244650](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307244650), V07 [#38307452459](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307452459), V12 [#38307830593](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307830593), V10 [#38308507132](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308507132), V11 [#38308647893](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308647893), V08 [#38308817803](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308817803). These are individually verified FOCUSED checks for different source SHAs, **not** an exact-head FULL gate. Full G0 closeout and follow-on persistence/UI/D2 design decisions remain separately gated.

## 3. Separate edge-derived point matrix after VERTEX boundary is known

| Case | Source | Required future behavior |
| --- | --- | --- |
| E01 | Strictly resolved bounded straight material Edge | `MID` at half exact geometric length, not midpoint of display polyline |
| E02 | Strictly resolved bounded open circular arc | `MID` at half actual arc length along the bounded arc |
| E03 | Strictly resolved open circular arc | `CENTER` at analytic circle center; center need not lie on the Edge |
| E04 | Full closed circle | No edge `MID`; `CENTER` allowed if strict Edge authoring identity is independently certifiable |
| E05 | Edge Curve has multiple bounded realizations | `MID/CENTER` requires strict disambiguated bounded `MaterialEdgeReference` or typed failure |
| E06 | Periodic seam, representation partition or unsupported nonanalytic Edge | Reject as ordinary construction point source rather than misidentifying representation geometry |
| E07 | Later Chamfer shortens displayed final-stage Edge | Earlier-stage `MID` stays tied to original stage; final-stage selection, if separately valid, yields new midpoint |
| E08 | Stale Edge handle/generation/provider | Reject stale authority; never recompute from wrong stage or match by equal radius/position |

E01–E08 are accepted future product expectations, **not** already passing tests or permission to implement.

## 4. Proposed minimal G0 execution scope — pending contract activation

**Read-only baseline characterization first:** choose real native OCCT fixture generation in the existing C++ testing architecture; reuse existing tests/fixtures where feasible (`tests/pm05b2_strict_edge_resolver_test.cpp`, `tests/pm05c1_edge_semantic_kernel_test.cpp`, `tests/pm02p_e_core_lifecycle_test.cpp`, `tests/pm03b_datum_evaluation_test.cpp`) but do not claim those tests already prove the new VERTEX matrix. Fixture additions/target registration need an explicitly accepted allowed-path and test matrix. Do not edit existing tests outside an active bounded contract.

**Evidence checkpoint before changes:** publish per-case native results V01–V12 with actual stage, accounting status and native fixture conditions. If common user-visible Vertices fail durable referenceability, stop and ask Owner to approve an explicitly bounded semantic identity extension and its schema/API/lifecycle consequences. Do not widen `FeaturePointAddress` or store XYZ opportunistically.

**Only then design exact query contract:** Part-owned typed point intent (true semantic Vertex vs strict Edge-derived `MID`/`CENTER`); provider-neutral exact 3D Edge queries; stage/revision/token guards; schema upgrade and cold rebuild. An implementation contract must enumerate exact file paths, tests and acceptance gates.

## 5. Mandatory independent UI gate

All VERTEX/MID/CENTER Body snap feedback, pick eligibility/priorities, marker appearance, source selection and edit/repair workflows are **UI**. Before ANY such code, present the UX design and receive a separate explicit Owner approval as required by section 3 of the parent RG-02 Draft Work Contract. G0 native semantic characterization by itself gives no UI authorization.

## Documentation impact

Draft test matrix only, no as-built changes or execution claims. On approved implementation: Internal docs required; PL/EN product docs and Browser update required for released UX.
