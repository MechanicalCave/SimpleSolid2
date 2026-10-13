# RG-02B1/E0 — strict Edge + Point native feasibility gate (Owner-accepted evidence-only Work Contract)

**Status:** **OWNER-ACCEPTED EXACT FOUR-TEST SCOPE; PR #346 MERGED; PENDING SEPARATE ACTIVE ACTIVATION.** No E0 test/product mutation authority before an independently Owner-approved and merged `ACTIVE.yaml` activation.  
**Decision class:** Owner-accepted bounded D2 evidence-only Work Contract; no approval of a new semantic Point type, kernel API, persisted Datum variant or UI.  
**Prepared:** 2026-10-13; read-only source audit against `main` `b7da33653b8bc6214fd5774a34498838decb19cd`.  
**Program:** accepted and merged Part Modeling roadmap v1.33 §6/9F; independently Owner-approved post-merge metadata correction PR #344 MERGED as `20756f4da4c50f29e46f48cfc9fbf837bb7fa110` (`ACTIVE.yaml.program_roadmap_version` is `1.33`).  
**Activation boundary:** Before an independently approved activation PR is merged, `work/ACTIVE.yaml.active_work` references completed PG-01E **for history only**. Once activated by that dedicated merge, this contract authorizes ONLY the evidence/test scope in §2; it never authorizes product, UI, schema or Kernel API changes.  
**Design input:** `work/RG-02B1_DATUM_EDGE_POINT_WORK_CONTRACT_DRAFT.md` in Draft PR #345 (Owner-accepted one-constructor interaction direction, not executable or fully UX-approved). Preserve independent G0 findings, accepted ADR-0014/0016/0017 and Foundation v1.0.  
**Repository authority:** `AGENTS.md` → Constitution → Foundation/Architecture → accepted ADRs → ACTIVE → accepted Part roadmap → active contract → code/tests; documentation rule `governance/DOCUMENTATION.md`.

## 1. Goal: answer feasibility before designing the Datum schema

Characterize **strict current Part-stage material Edge identity, semantically certified END and independent Point candidates, exact bounded Line/open Arc geometry capability, and explicit fail-closed limits** from real OCCT and accepted semantic catalogs. Distinguish:

1. **Viewer hit/visible point** (transient gesture);
2. **Part semantic Point status** (Resolved/Missing/Ambiguous/Unsupported, at an exact Body stage);
3. **strict material Edge-to-Point membership**, not just shared XYZ;
4. **availability of an existing provider-neutral 3D metric-END/MID/tangent query**;
5. **future persistent Datum usability**, which is *NOT* proved by E0 alone.

The **Owner-accepted end-user interaction stays unchanged**: one existing Datum Plane tool, one proposed `Edge + Point` constructor, sources in either order, MID same marker as END, unique semantic source-Edge END/MID automatically preferred among independently valid overlapping candidates, and straight-Edge + independent off-line Point meaning. No extra mode/picker/dropdown and no automatic G1/Axis/PM-06 scope.

**E0 is an evidence gate, NOT a new production capability.** Successful characterization may establish a genuine Unsupported/Blocked limitation requiring D2; a green test does not imply new Datum behavior.

## 2. Exact file-mutation allowlist — ONLY after independent Owner acceptance, governance merge AND ACTIVE activation

Permit **at most these four already registered existing test source files**, with narrow test-only code and fixtures local to those files:

| Path | Existing registered CTest name | Allowed observation |
| --- | --- | --- |
| `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` | `pm05c2b.part_edge_topology` | Real evaluated Extrude/Add/Cut/Fillet/Chamfer topology and stage; strict material Edge authoring/resolution, native Vertex catalog, current incident material Edges, certified semantic Points, shared corners, source loss and re-evaluation |
| `tests/kernel_native/pm04c2_production_revolve_test.cpp` | `pm04c2.production_revolve` | Existing analytic circular/Revolve cases, full/partial circle/periodic seam status, G0 half-cylinder Ambiguous and Revolve Unsupported negatives |
| `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` | `pm02p.e_kernel_lifecycle` | Existing real native Line/Circle/Arc edge evaluation and exact bounded **2D projection** controls; characterize 3D metric midpoint/tangent *availability and evidence gap* without claiming projection is a 3D Datum query |
| `tests/pm05b2_strict_edge_resolver_test.cpp` | `pm05b2.strict_edge_resolver` | Existing pure semantic fail-closed tests: ambiguous Point cannot be made Resolved through Edge incidence; strict multi-branch and stage ambiguity, not a simulated OCCT hit |

**Only additional reporting files allowed following actual activation:** this specific Work Contract for append-only observed evidence, and `work/RG-02B1_E0_NATIVE_EDGE_POINT_PROOF_EVIDENCE.md` for a finite actual measured source/test/CI table (new work report, not an implementation file). Report MUST say `UNRUN` until verified.

**No other writable paths:** no `src/**`, other `tests/**`, new target/fixture file, `CMakeLists.txt`, CI/workflows, build scripts, `adr/**`, `governance/**`, `work/ACTIVE.yaml`, roadmap, current product/as-built docs or generated Browser. **No provider-private helper API added to production** under a test-only exception. Existing tests and checks remain enabled and unweakened.

**Source/header/link boundary:** if a requested native analytic 3D query requires adding OCCT include paths, libraries, new registered targets or changing production APIs, record the exact existing capability gap and **STOP for separate Owner D2/amendment**; do not edit CMake or sneak a provider implementation into a test. Geometry reconstructed from authored profile numbers or 2D projected arcs may be corroborating math only; it is **not proof of querying the current 3D strict material Edge**.

## 3. Finite E0 matrix: current evidence versus still-unproved Datum behavior

| Gate | Test location | E0 evidence that is legal to claim | Not established by this gate |
| --- | --- | --- | --- |
| E0-01 — canonical straight END | `pm05c2b.part_edge_topology` | On real 40×30×20 Extrude, determine one strict, stage-current material Edge and its two actual current native endpoint Vertex meanings. For any Resolved Vertex, require unique exact `FeaturePointAddress` and matching one-to-one catalog resolution; report native incident material Edge relation and counts. | Does **not** create durable `Edge + END` selector or prove all possible ENDs. |
| E0-02 — shared-corner and ambiguity | `pm05c2b.part_edge_topology` plus `pm05b2.strict_edge_resolver` | At a real trihedral corner, verify stage-current strict incidence for two or more Edges; distinct already-Resolved Point addresses may be compared only by exact meaning. Preserve the existing regression: one intrinsically Ambiguous Point stays Ambiguous regardless of incidence. Report `NOT DEMONSTRATED` if fixture lacks two *distinct* independently certified Point meanings at one display location. | Does **not** simulate Viewer click order or resolve two candidates solely by XYZ. |
| E0-03 — line MID feasibility | `pm05c2b.part_edge_topology` / `pm02p.e_kernel_lifecycle` | Characterize a strict bounded straight material Edge and current exact Line classification; explicitly inventory whether its 3D half-length position and tangent can be obtained through a *current* provider-neutral API. If unavailable, report `API GAP / BLOCKED` rather than generating an authored MID. | No new metric query, persisted MID role or frame orientation decision. |
| E0-04 — open Arc END/MID feasibility | `pm02p.e_kernel_lifecycle` and relevant `pm04c2.production_revolve` | Use existing real circular-segment OCCT cases (including positive/negative sweep) to prove exact supported bounded Arc in current tests; distinguish analytic half arc-length/tangent formula from missing stage-bound 3D native query. Explicitly classify full circle MID/representation seam as Unsupported. | Exact current 2D projected `Arc2` is **not** enough to certify a durable 3D MID/END reference. |
| E0-05 — independently certified off-line Point | `pm05c2b.part_edge_topology` | Select a resolved strict straight Edge and a *different*, independently Resolved semantic Point from the **same Body stage**; inventory incidence, distinct semantic source, and collinear vs noncollinear geometric feasibility using accepted current observations. | No persistence of XYZ or through-Edge-and-Point plane; no UI acquisition equivalence test yet. |
| E0-06 — nonauthorable negative topology | `pm04c2.production_revolve` and `pm05c2b.part_edge_topology` | Preserve G0 V04/V08/V10/V05/V06 counted Unsupported/Ambiguous cases. Verify no invalid Vertex is marked authorable because an adjacent strict Edge is valid. Fail on unexplained accounting/integrity errors. | No wholesale deferred G1 semantic Vertex extension. |
| E0-07 — earlier/later stage and cold provider | `pm05c2b.part_edge_topology`; optional already allowed `pm04c2.production_revolve` | Verify identical declared earlier Body stage survives a later Chamfer, semantic identities after upstream edit, known source-loss failures, and available fresh-kernel evaluation of existing semantic catalog only. If the existing test path cannot establish true cold source meaning, report `NOT DEMONSTRATED / BLOCKED` rather than silently switching stage. | No newly persisted Datum, Datum Save/Reopen, UI Edit/Finish, or session-retained authority proof. |

**Test assertions must be objective.** Count actual native topology and statuses, preserve one-to-one semantic Point evidence, enforce source-stage equality and accepted strict Edge resolver outcomes. If an unsupported result is genuinely observed, it is a valid **negative architectural finding**, not permission to relax a test. For every gap distinguish `SUPPORTED`, `UNSUPPORTED`, `AMBIGUOUS`, `MISSING`, `BLOCKED` and `UNRUN`, with exact test SHA/run/log evidence. Never report full E0 feasibility PASS from skipped or limited tests.

## 4. Execution sequence after formal activation — new E0 native tests UNRUN

1. **Read-only exact baseline:** refresh main HEAD, ACTIVE, accepted v1.33 roadmap, parent PR #345 status, and the four existing registered tests; preserve historical G0 proof without relabeling its runs as new E0 execution.
2. **Native Part identity FIRST:** stage-correct END/incident Edge at the canonical rectangle and shared corner; real difficult G0 negative topologies; strict semantic Point ambiguity guard. If the required END cannot be certified, stop before proposing a Datum schema.
3. **Native analytic feasibility SECOND:** line and open arc cases; record exact current provider-neutral query gap, not a fake derived End/Mid implementation.
4. **Stage/cold controls THIRD** only where already supported. No custom test-only persistent synthetic Datum schema to create a misleading Save/Reopen success.
5. **Windows FOCUSED** on each changed registered native test, with current CI-required exact `SS2-Focus-Target`, `SS2-Focus-Test`, `SS2-Focus-Mode: kernel` trailers and current dispatcher rules; the semantic `pm05b2.strict_edge_resolver` participates in FAST. Verify actual selected target/test, not just aggregate job green. Recheck the current CI configuration before issuing any focused run.
6. **Windows FAST** after source stabilization, then **one final Ready-for-Review exact-HEAD FULL** with Core/native/Desktop/docs and aggregate gates. Preserve RED logs and investigate rather than waive issue #302 or weaken an assertion. An evidence-only implementation PR remains Draft until these and independent Owner evidence review.
7. **E0 evidence synthesis** distinguishes successful test assertions from **product capability not yet demonstrated**. Owner then separately decides the D2 next step; CI PASS does not constitute Owner acceptance or permission to merge.

**No current E0 native tests have been run.** Previous G0, PG-01E, or PR #345 Windows governance passes do not transfer to new E0 assertions.

## 5. Hard STOP and Owner decision requested

**Single point of failure:** a uniquely Resolved material Edge does not imply a uniquely Resolved model END Point. The acceptance of `Point → Edge` overlap filtering applies **only** to already independently certifiable semantic candidates. No incident-Edge trick may upgrade intrinsically Ambiguous `FeaturePointAddress`.

**STOP for Owner D2**, with bounded evidence, if actual END needs a new provenance-backed Edge-local endpoint selector, if current 3D MID/tangent requires public Kernel API, if a construction-point source needs new durable Part variant or versioned schema, if orientation/Reverse policy is undecided, or if an exact E0 test requires a fifth source path or CI/CMake change. Those are separate decisions, not implementation discretion under E0.

**UI HARD STOP independently remains:** before new Datum/Viewer/Tree/Operations/Command Line behavior, review the complete UX details in parent Draft §1B and obtain explicit Owner approval. Previously accepted one-tool/two-pick-order principle is preserved, not equivalent to full UI sign-off. No implementation of Axis, G1, three-point/angled planes, Mirror/Pattern, PM-06, or CI issue #302 under this E0 scope.

**Approval chain — each step independent:**
1. **COMPLETED 2026-10-13:** Owner explicitly accepted **this exact four-path evidence-only contract** in response to an exact-scope approval request (not from generic `kontynuuj`).
2. **COMPLETED 2026-10-13:** [PR #346](https://github.com/MechanicalCave/SimpleSolid2/pull/346) Windows governance #2817/#2818 PASS; Owner separately authorized exact-head squash merge, MERGED on main as `ef7721afc40486d576986611bb236e9396f94da8`. Independent roadmap metadata [PR #344](https://github.com/MechanicalCave/SimpleSolid2/pull/344) Owner-authorized squash MERGED as `20756f4da4c50f29e46f48cfc9fbf837bb7fa110`. Parent UX/design PR #345 remains Draft OPEN, unmerged.
3. **PENDING:** a distinct governance-only activation PR updates `work/ACTIVE.yaml.active_work` to this accepted E0 contract, preserving roadmap v1.33 and every other scope boundary. Obtain exact-head CI, then independent Owner approval of this activation PR and separate PR-specific merge authorization. No E0 test changes before that merge.
4. **Only after activation merge**, open a fresh test-only implementation PR from current main. Final evidence acceptance and separate PR-specific merge permission are later gates.
5. Any required D2 schema/API/UX mutation is a **new contract**, not an E0 completion amendment by implication.

## Documentation impact

Internal docs: not required  
User/Product docs: not required  
Generated Browser: not required  
**Reason:** this accepted evidence-only Work Contract and proposed activation introduce no product behavior. The later separately activated test-only gate changes only the four allowed tests and specified `work/` evidence. A future shipped Datum Edge + Point feature requires current-state Internal documentation, paired PL/EN Product documentation and deterministic `./ss2.ps1 docs` Browser regeneration.

**Next narrow Owner decision:** separately accept this dedicated E0 activation proposal after reviewing its two-file governance-only diff; then separately authorize exact-head merge after Windows closure. The prior four-test scope acceptance and contract merge do **not** themselves approve activation, product schema/API, full UI UX or future E0 implementation PR merge.
