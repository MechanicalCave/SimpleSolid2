# RG-02A/G1 — Native Body VERTEX semantic-provenance investigation Work Contract (DRAFT)

**Status:** **OWNER-ACCEPTED BOUNDED G1 EVIDENCE CONTRACT TEXT — DEFERRED BEHIND PG-01E, NOT MERGED, NOT ACTIVATED. NO TEST OR PRODUCT MUTATION AUTHORIZED BY THIS DOCUMENT.**  
**Owner acceptance:** 2026-10-13 — explicit `akceptujekontynuuj` in response to the question accepting **the exact limited native RG-02A/G1 research scope in PR #334**. This acceptance covers ONLY this proposed evidence-only objective, the three allowed pre-existing test source paths, G1-F01/C01/A01/A02/S01/P01/L01 proof matrix and the listed hard exclusions. It is **not** Owner permission to merge PR #334 or #333, approve roadmap v1.33 sequencing, activate a Work Contract, execute G1 tests, adopt a persistent identity/schema or implement RG-02 UI.  
**Draft date:** 2026-10-13  
**Baseline for this proposal:** `main` `5b495c4bbed8754d7a1f01e87a30716429db786d` (Owner-authorized merge of G0 [PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332)). Revalidate all source APIs and exact `main` immediately before any future independent G1 activation.  
**D2 direction accepted:** Owner answered `akceptuję - kontymuuj` on 2026-10-13 **only to approve Option B as an evidence-first investigation direction and authorize preparing this separate draft**. That earlier narrow D2 decision did **not itself** approve this exact Work Contract, the unmerged architecture [Draft PR #333](https://github.com/MechanicalCave/SimpleSolid2/pull/333), any new persistent Vertex identity, stage sequencing, `ACTIVE.yaml` activation, implementation or UI. **The exact bounded Work Contract text was separately accepted afterward as documented in Owner acceptance above**; governance merge and activation remain pending.  
**Current active contract:** `work/ACTIVE.yaml` still names `work/RG-02A_G0_VERTEX_CHARACTERIZATION_WORK_CONTRACT_DRAFT.md` with roadmap **v1.32**. It does **not** authorize G1 execution.  
**Boundaries:** `governance/CONSTITUTION.md`, `governance/FOUNDATION.md`, `governance/ARCHITECTURE.md`, `work/PART_MODELING_V1_ROADMAP.md` v1.32, G0 native evidence on `main`, `work/RG-02A_VERTEX_SEMANTIC_IDENTITY_D2_PROPOSAL_DRAFT.md` on PR #333 (still unmerged). Older G0 file headers saying `UNRUN` or `NOT ACTIVE` are historical; only the independently approved G0 merge/CI and Owner decision determine the G0 evidence state.

## 1. Precisely bounded objective

**G1 is read-only semantic-provenance characterization of current real OCCT Body Vertices, not an extension of the Part implementation.** Determine whether the currently **Unsupported** or **Ambiguous** material Vertices can be separated by *existing independently authored semantic evidence*, before proposing any persistent discriminator. Preserve today's certified `FeaturePointAddress` resolution and its `BodyStageRef`. Do not implement even an experimental persisted `VertexId`, new `FeaturePointAddress` variant, feature behavior or UI.

The actual G0 quantitative baseline (Owner accepted **with limitations**, PR #332 merged) is:

| Real Part fixture | Current native vertices | Resolved | Ambiguous | Unsupported |
| --- | ---: | ---: | ---: | ---: |
| Rectangular Extrude | 8 | 8 | 0 | 0 |
| Extrude + interior Cut | 16 | 16 | 0 | 0 |
| Trihedral Fillet | 13 | 10 | 0 | 3 |
| Trihedral Chamfer | 13 | 10 | 0 | 3 |
| Revolve partial 90° | 6 | 6 | 0 | 0 |
| Revolve full 360° | 2 | 0 | 0 | 2 |
| Extruded analytic cylinder | 2 | 0 | 0 | 2 |
| Half-cylinder Boolean Cut | 6 | 0 | 4 | 2 |

On the half-cylinder Cut the four Ambiguous Vertices arise from **two distinct three-Surface semantic Point groups with multiple current native realizations**. On a removed-corner Cut, **two old lineage Point addresses are Missing** although all **12 current native Vertices are Resolved**. Those different inventories must not be conflated.

Source: [G0 native report on merged main](https://github.com/MechanicalCave/SimpleSolid2/blob/main/work/RG-02A_G0_NATIVE_VERTEX_EVIDENCE.md), [G0 matrix on main](https://github.com/MechanicalCave/SimpleSolid2/blob/main/work/RG-02A_VERTEX_NATIVE_G0_MATRIX_DRAFT.md), exact-head FAST [#38310478851](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38310478851) **93/93 PASS** and FULL [#38310694444](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38310694444) **199/199 PASS**. These are **G0** outcomes, not executed G1 cases.

## 2. Read-only evidence sources; EXACT permitted future mutation scope

Read-only inspection is allowed when preparing/reviewing G1 design: `src/part/include/simplesolid2/part/semantic_topology_reference.hpp`, `src/part/include/simplesolid2/part/feature_evaluation.hpp`, `src/part/feature_evaluation.cpp`, `src/kernel/include/simplesolid2/kernel/solid_modeling.hpp`, `src/kernel_occt/solid_modeling_kernel.cpp`, existing Part test fixtures, current G0 reports and governance.

**Only AFTER Owner-accepted bounded contract text is separately merged, any required roadmap sequencing amendment is separately accepted and merged, and a dedicated `ACTIVE.yaml` activation has separately merged, future G1 native test/evidence PRs may edit exactly:**

1. `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` — primary: native **Part** Fillet and Chamfer Unsupported classifications, owning-Feature/Surface/Curve provenance, Part-stage semantic topology, source MaterialEdgeReference (if already independently certifiable), positive historical stage cases.
2. `tests/kernel_native/pm04c2_production_revolve_test.cpp` — primary: real analytic half-cylinder Cut Ambiguous groups, ordinary cylinder and 90°/360° Revolve representation seams, native vs Part-stage counting, deterministic independent OCCT rebuilds.
3. `tests/kernel_native/pm02c_production_edge_curve_vertex_point_test.cpp` — **optional only when necessary** for raw OCCT/provider-neutral Vertex and adjacent Surface/Curve inventory observation not exposed by the first two *existing* test APIs. This test MUST NOT masquerade raw native incidence as resolved Part semantic provenance.

Only G1 documentation changes in `work/**`: this Owner-accepted contract once merged (if accepted), an eventual `work/RG-02A_G1_VERTEX_PROVENANCE_EVIDENCE.md` report with real SHAs/results, and a specifically Owner-authorized G1 Work Contract evidence/status appendix. Existing G0 evidence/matrix must remain historically intact; do not rewrite G0 conclusions to pretend they proved G1.

**Forbidden under this proposed test-only G1:** `src/**` (all production code, even logging or provider query helpers), any `CMakeLists.txt` or new test target, `adr/**`, `governance/**`, `docs/internal/**`, `docs/product/**`, `docs/browser/**`, CI/workflows/scripts, schemas, serializers/migrations, Part/Kernel APIs, persistent data model or `FeaturePointAddress` layout, application Commands, Viewer/UI/Tree, snap markers, Datum Plane/Axis operations, MID/CENTER, Mirror/Pattern. `work/ACTIVE.yaml` and roadmap are handled **only in independent, explicitly accepted governance PRs**, never inside a test/evidence branch. An unavailable provider detail or unexpressible fixture is `BLOCKED` and a STOP, never authorization to add a fourth test source or change `src/**`.

## 3. Native proof matrix (G1.1–G1.5; all UNRUN)

The observations must come from **real production OCCT geometry and actual Part-stage catalogs** at known after-Feature history stages. A synthetic `BodyStageTopologyCatalog` cannot establish a new discriminator. Runtime tokens and provider XYZ may be printed as diagnostics **only within an execution**, but are not durable identity or comparison keys.

| ID | Real fixture / investigation | Required report or native assertion | Pre-approval status |
| --- | --- | --- | --- |
| G1-F01 | Real trihedral Fillet, the **three Unsupported** current material Vertices | For EACH Vertex, count *raw native Face/Surface and incident Edge* carriers **separately** from resolved unique **semantic `FeatureSurfaceAddress` / `FeatureCurveAddress`** carriers. Report absent/aliased/unresolved/too-few/too-many independently; do not infer cardinality from the word Unsupported. | **UNRUN** |
| G1-C01 | Real trihedral Chamfer, the **three Unsupported** current material Vertices | Same independent cardinality, generated transition Surface source and authored source-Edge/Point ancestry; distinguish a seam artifact from material Vertex by existing `TopologyAccountingClass`. | **UNRUN** |
| G1-A01 | Half-cylinder Cut with **two Ambiguous semantic triple-Surface groups** and four native candidates | Build a report of already-resolved semantic *incident bounded material Edges/Curves* for each candidate. Determine **whether any candidate has an independently certified, provider-neutral unique branch witness**. If two remain indistinguishable, retain `Ambiguous` (valid negative finding), not an invented PASS of unique identity. | **UNRUN** |
| G1-A02 | Same half-cylinder before/after dimension edit, with independent OCCT rebuild | Only for candidates with a demonstrated independent branch witness: re-evaluate exact after-Feature stage and test uniqueness/stability without order, XYZ or runtime-token equality. If the existing test API cannot express edit or source witness, mark `BLOCKED`, request bounded amendment, do not add production code. | **UNRUN** |
| G1-S01 | Extruded cylinder, full 360° Revolve and partial 90° Revolve | Compare current Body native material Vertex accounting with provider seam/representation partition tokens and semantic Face/Curve carrier meaning. No promise that a raw seam is a durable referenceable physical corner. | **UNRUN** |
| G1-P01 | Positive semantic preservation (8-Vertex Extrude, 16-Vertex Cut), stage-scoped history before Chamfer, missing former Point on cut-away corner | Baseline certified triple-Surface `FeaturePointAddress` must stay uniquely `Resolved`; old lost Point stays `Missing`; no nearest-XYZ reattachment or replacement-Feature producer transfer. | **UNRUN** |
| G1-L01 | Cold **in-memory Part** rebuild and independent kernel for any candidate semantic source considered in G1-A01 | Compare only independently evidenced semantic addresses and current scope; no stale token reuse. Clearly separate this from **UNIMPLEMENTED** new RG-02 source persistence and untested interactive stale Viewer hit rejection (not approved UI test scope). | **UNRUN** |

**Per-case evidence format:** baseline `main` SHA; exact test source SHA and Windows run URL; Feature/stage and geometry; number of distinct **current native material Vertices**, provider raw Surface/Edge incidence, number of independently resolved **Part semantic Surface/Curve/Edge carriers**, accounting class, `ReferenceStatus`, source Feature provenance, actual ambiguity multiplicity, whether the proposed witness is **acyclic**, whether it survives an upstream edit and independent rebuild, and explicit outcome `CERTIFIED_UNIQUE_WITNESS` / `STILL_AMBIGUOUS` / `UNSUPPORTED` / `MISSING` / `BLOCKED` / `UNRUN`. `CERTIFIED_UNIQUE_WITNESS` is **test evidence only**, never authorization to author it in an actual Part/Datum document.

**Acyclicity rule:** a Vertex's proposed identifying semantic Edge is inadmissible if the Edge's own strict `MaterialEdgeReference` resolution already depends on that same Unsupported/Ambiguous Vertex without an independently certified prior source. Document the dependency path, not simply the existence of an incident Edge.

**Stop-on-integrity rule:** topology corruption, unaccounted native entities or silent aliasing is `RED`, not a success case; Unsupported/Ambiguous can remain evidence-backed negative results.

## 4. Execution sequence AFTER all governance gates (not now)

1. Verify exact `main` and the selected active G1 contract, accepted roadmap sequencing, existing test registration, and G0 merged baselines. Do not confuse PR Draft or Owner's approval of Option B *for drafting* with active evidence authority.
2. Open a **fresh test/evidence PR** from the main commit after actual G1 activation. Begin with G1-F01/C01 raw-vs-semantic carrier diagnosis; use only test-accessible existing APIs. Record RED/measurement SHA if a deliberate failing diagnostic is truly needed and label it intentional; remove such failure before GREEN, do not silently hide it.
3. Continue G1-A01/A02 and G1-S01 then regression G1-P01/L01; preserve negative findings as negative evidence. Report unavailable semantic carrier detail as `BLOCKED` if obtaining it would require new production API/code. No fake or inferred data.
4. **Windows FOCUSED** on every modified registered native target (e.g. `pm05c2b_part_edge_topology_test` / `pm05c2b.part_edge_topology`; `pm04c2_production_revolve_test` / `pm04c2.production_revolve`; optional `pm02c_production_edge_curve_vertex_point_test` / `pm02c.production_edge_curve_vertex_point`), using exact `SS2-Focus-Target`, `SS2-Focus-Test`, and `SS2-Focus-Mode: kernel` trailers under the CI classifier rules.
5. Run required Windows FAST and final exact-head Windows FULL, including closure/docs. Store SHA-to-claim-to-run links, and independently verify any non-green checkpoint. **No G1 PASS** is claimed in this Draft.
6. Present Owner a G1 evidence synthesis before proposing a technical Vertex selector, G2/API/persistence contract, or any product/UI changes. Require separate Owner acceptance of findings, separate PR merge authorization, and future distinct design governance.

## 5. Roadmap / G1 authority are INDEPENDENT pending Owner decisions

Part-v1 roadmap v1.32 authorizes completed G0 evidence followed by a separately gated **PG-01E review** and later PM-06. After an earlier permission to draft G1-before-PG-01E as v1.33, the Owner asked why the order had changed and explicitly answered **`akceptuje - kontynuu` (2026-10-13)** to the proposed **return to PG-01E NEXT and G1 LATER**. That later decision supersedes the v1.33 Variant A draft ([superseded PR #336](https://github.com/MechanicalCave/SimpleSolid2/pull/336)); the record is [Draft PR #335](https://github.com/MechanicalCave/SimpleSolid2/pull/335). **The exact G1 evidence contract text remains accepted, but deferred, unmerged and inactive.** No placement of G1 relative to PM-06 has been accepted; a fresh explicit Owner sequencing decision is required before any later G1 activation.

Required for any **later** G1 work: (a) **DONE: Owner accepted this bounded native evidence contract text**, (b) **NOW: PG-01E is the next separately reviewed package**; do not insert or activate G1 during PG-01E review, (c) obtain a new Owner D2 sequencing decision when G1's later place relative to remaining Part-v1 gates (including PM-06) is actually considered, (d) only then obtain any required roadmap governance, each independent PR #333/#334 merge permission and a **dedicated `work/ACTIVE.yaml` activation PR**, (e) **only after its separate approval and merge** mutate named G1 native tests on a fresh branch. A documentation-only merge never activates tests. No auto-activation or automatic merge.

**Explicitly excluded after any G1 evidence activation:** a production `FeaturePointAddress` variant, persistent VertexId, new source type or schema, MID/CENTER exact-kernel geometry, Datum Plane/Axis constructors, UI user interaction, PM-06, PG-01E, Mirror/Pattern implementation.

**Owner UX HARD STOP:** before **ANY** RG-02 UI/UX implementation (including VERTEX/MID/CENTER pick or snap markers, overlay feedback, Tree/Operations, preview, Edit/repair and Command Line), present a reviewable UX design to Owner, consult and obtain **separate explicit Owner approval**. Native G1 evidence and Option B investigation permission do not replace that gate.

## 6. Acceptance questions when reviewing THIS DRAFT (not approved)

1. **ACCEPTED 2026-10-13:** G1 native evidence goal, exact three-test-source maximum and no production changes (Owner `akceptujekontynuuj`), **text acceptance only; merge and activation remain blocked**.
2. **OWNER CHOSE PG-01E NEXT** (2026-10-13, `akceptuje - kontynuu`), superseding G1-first v1.33 draft. G1 is deferred; its final later placement relative to PM-06 has **not** been selected.
3. After reviewing G1 evidence, should Option A remain the fail-closed baseline if no non-circular unique semantic discriminator can be proved? (Suggested: **yes**, never select a speculative provider- or XYZ-based surrogate.)

**These are separate acceptance matters.** Both the Option B direction and the exact **bounded G1 contract text** are now Owner-accepted. **A PG-01E-first sequencing choice is now made, but future G1 placement relative to PM-06, each PR merge, G1 activation, technical D2 schema/design and UX remain pending**, and no G1 native test may run.

## Documentation Impact

Internal docs: not required
User/Product docs: not required
Reason: this is an Owner-accepted **contract text** for future native evidence only, not merged or activated, with no production behavior, source schema, API or UI mutation. A future Owner-authorized RG-02 feature requires full Internal/PL+EN Product documentation and generated Browser validation.
