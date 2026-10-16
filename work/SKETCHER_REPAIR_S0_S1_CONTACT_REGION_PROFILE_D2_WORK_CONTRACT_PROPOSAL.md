# Sketcher Repair S0/S1 — Contact Predicates, Arrangement, Profile Integrity — D2 Work Contract PROPOSAL

**Status: PROPOSED EXECUTABLE CONTRACT; NOT OWNER-APPROVED EXACT D2; NOT ACTIVE; NO PRODUCT MUTATION AUTHORITY.** Prepared 2026-10-16 from protected main \`19d20eb3affb5656950604f8986233866c970012\` (H2 #453 merged). The Owner accepted the **S0–S3 repair program direction and S0/S1 as its first priority** in conversation on 2026-10-16, but had not yet reviewed/accepted *this exact bounded file allowlist and acceptance contract*. Distinguish these decisions.

**Governing authority:** AGENTS.md; governance/CONSTITUTION.md, FOUNDATION.md, ARCHITECTURE.md, DOCUMENTATION.md; accepted ADR-0008, ADR-0009, ADR-0011, ADR-0012, ADR-0014/-0016/-0017 as applicable; work/SKETCH_ROADMAP.md v1.9 (existing Sketcher semantics), work/F_REGION_PROFILE_SEMANTICS.md (accepted connected material Profile constraints), work/R12_TRIM_EXTEND_MUTUAL_EXTEND.md, and work/PART_MODELING_V1_ROADMAP.md v1.33. Current work/ACTIVE.yaml continues to reference **COMPLETED RG-02C/H2**; it does not authorize this Sketcher work.

## 0. Exactly one upcoming Owner D2 decision

Accept or amend this **bounded S0/S1 geometry/profile integrity contract**: §§1–7, the **15 explicit conditional file paths** in §5, and all test/CI and Owner signoff gates. The Owner's already accepted higher-level S0–S3 direction is recorded as scheduling guidance: S0 reproduce → S1 fix geometric/topological correctness → later independent S2 snap input (B03a/B04) → later independent S3 Esc/Copy/Mirror (B03b/B05/B06). There is **no** permission in S0/S1 to implement S2/S3.

**Governance sequencing STOP:** Following exact-text D2 approval, integrate the proposed contract and any separately reviewed priority/roadmap amendment into protected main via their own Windows exact-HEAD governance CLOSURE and **separately authorized guarded squash**. Amend accepted Part Modeling v1.33 RG-02 sole-current-focus and Sketch Roadmap v1.9 milestones **explicitly**, preserving history and PM-06 deferral, instead of silently overwriting them. Then propose a distinct **ACTIVE-only** switch to the integrated Sketcher S0/S1 contract and its governing roadmap; pass its exact-HEAD Windows CLOSURE and obtain separate Owner merge authorization. Only after verifying activated protected main may an independent implementation PR mutate §5 files. No direct push to main; no implied authority from this draft or from approval of the program direction.

## 1. Evidence, status and defect boundary

Audit supplied by Owner: **SS2_Sketcher_Repair_Guide_LLM(1).md**, dated 2026-10-07, authored against old main \`9547b986ccbd4dc7533c446c797eb5c4886ad0a6\`. It reports Linux GCC13 C++20 core probes, not a contemporary Windows/Qt/OCCT execution. On 2026-10-16, direct repository SHA comparisons prove **identical source blobs** between audit baseline and current main for:
- \`src/sketch/region_analysis.cpp\` (blob \`76aa2bbe2b1e82bad01bdad597dc70b15d256c67\`),
- \`src/sketch/structural_edit.cpp\` (\`a064fe0fd2957b8d8840de9c437bbf58cd92a18e\`),
- \`src/sketch/interaction_state.cpp\` (\`8503af284549cbb67a9025c33f987947fd5b18cf\`).

**B01 region/profile tangency — audit core reproduction, as-built mechanism corroborated:** \`analyzeRegions\` currently drops a contact with \`CurveContactKind::tangent\` when neither side is an endpoint (interior tangency); \`buildComponentLoops\` sorts outward half-edges by tangent_angle then edge index. The exact geometry's reported 24 permutations yield 12×0 regions and 12×1 region, never the expected 2. Failure is allocation-order dependent. Exact contribution of equal-tangent half-edge face-walk, seam/angle numeric error and connected-component construction remains **TRACE REQUIRED**, not independently established at current HEAD by execution.

**B02 exact oblique tangency/Trim — audit core reproduction, as-built mechanism corroborated:** current \`lineCircle\` normalizes by hypot and uses sharp \`perpendicular > radius\` / \`perpendicular == radius\` classification. The audit F3 witness computes 5.0000000000000009 for a mathematically exact R=5 tangent, giving disjoint/no_intersection. The same relation is shared by Trim, Snap Intersection and arrangement. This is not a blanket Trim ban: vertical F1 cutters already yield a ready Trim when two distinct contact points exist. Preserve one-cutter refusal.

**In S0/S1 scope ONLY:** B01 and B02; correctness of region enumeration, stage-appropriate durable Part Profile RegionIntent, Add/Subtract material composition and current Extrude consumption/cold rebuild. The other four reported issue families B03a/B03b/B04/B05/B06 **remain registered but outside this first contract**.

## 2. Required S0 reproductions, before any fix

Register failing **RED** assertions on the actual new implementation branch/HEAD. Preserve original audit numbers separately as historical, not new Windows PASS/RED. Each fixture consists of **Regular**, not Construction, entities:

- **F1:** Circle center (0,0), R=5; left Line (-5,-9)→(-5,0); right (5,-9)→(5,0); base (-5,-9)→(5,-9). Expected **two bounded arrangement cells**: disk \`25π = 78.53981633974483\` and lower pocket \`90 − 12.5π = 50.73009183012759\`; valid shared-edge composition outer area \`90 + 12.5π = 129.26990816987242\`. Assert invariant regions/boundaries for **all 24 creation orders**, not raw EntityId equality.
- **F2:** Same Circle and base, extend left/right Line upper ends to (-5,7), (5,7). Both tangent points are interior points on the Lines. Expected **two cells**, excess dangling Line portions do not open the lower pocket.
- **F2/Arc variant:** analytic Arc with C=(0,0), R=5, start -0.2 rad, sweep 6.0 rad; same base and both variants of tangent Lines. Verify only geometrically closed/legal sweep-bounded regions, with CW/CCW and seam variants and no false closure.
- **F3:** Circle C=(0,0), R=5, Line left (-13,-9)→(-4,3), right (13,-9)→(4,3), optional base for profile analysis, Circle Trim pick (0,-5). Each end is a tangent and both distinct contacts are classified; expected **Trim READY**, one Circle replaced by one Arc through one legal structural-edit transaction, with original Circle EntityId consumed and fresh Arc EntityId under frozen R12 policy.
- Add independent as-built trace output for relation classification (including exact source parameter and endpoint flags), outgoing half-edge local order, cycle walk, face area/orientation and region candidate diagnostics. Keep bounded test-only diagnostics; do not ship ad hoc always-on GUI logging.

If actual current execution differs from the audit, revise the evidence/diagnosis **before** a patch; do not hardcode the historical RED into final expectations.

## 3. Bounded S1 implementation semantics

1. **One common line–circle/arc relation:** fix the shared analytical predicate and finite parameter admission rather than adding Trim-only epsilon. F3 integer-vector witness: \`cross(q,d)^2 − R^2*dot(d,d) = 5625 − 5625 = 0\`. Choose an error-bounded stable/arithmetic-robust classification compatible with the supported coordinate domain, including dangerous overflow/underflow and near-tangent distinct secants; an unchanged result under scalar/rotation transforms must be justified by tests. Exact floating-point construction/round-trip policy must be documented; if a user-facing tolerance or uncertain-contact policy is required, **STOP for Owner D2 amendment**. No global epsilon, snap-aperture closure, input perturbation, viewer-point healing or near-contact merger.
2. **Semantic tangent topology:** preserve contact kind and endpoint status to arrangement. A tangency may create a graph vertex; it does **not** imply that two point-touching material interiors become one Part Profile. Do not silently change accepted F-region connectedness/holes rules. Support tangent at line endpoint and tangent interior, T-junction, coincident/overlap fail-closed existing behavior.
3. **Geometric half-edge order:** resolve equal/small first-derivative direction from local analytic side/curvature (or proven equivalent) independent of insertion index, reversed source Line/Arc orientation, CW/CCW, Circle seam and angle wrap. IDs may stabilize otherwise identical data, **never determine geometric side or face topology**. Ensure bridge cancellation, cycle orientation, signed areas, hole containment and open-boundary diagnostics are consistent.
4. **No authored mutation by analysis:** preserve Sketch entity geometry, roles, EntityIds and storage schema for region detection. Derived fragments/vertices remain disposable. Invalid/coincident/ambiguous/unsupported input fails closed.
5. **Current downstream semantics:** once exact two cells resolve, evaluate durable ProfileRegionIntent, Create/Edit/Add/Subtract under current F rules, legal composite region, dependent Extrude/profile input, Undo/Redo, Save/Close/Reopen and fresh native OCCT cold evaluation; do not replace intended valid/invalid semantics with nearest-profile rebinding. Identify any pre-existing saved Profile whose evaluation changes after the correction, document why, and preserve authored ProfileId.
6. **R12 structural identity frozen:** one distinct tangent cut must not trim Circle; two distinct legal Circle cuts can produce one Arc with fresh EntityId, original Circle removed; no Split, middle Arc removal that would require two result entities, universal topology genealogy or schema change.
7. **D1 internal algorithm freedom ONLY:** do not invent a new public Shared-2D API, Kernel provider contract, native schema version, durable topology reference, parametrization/constraint solver, auto Coincident/Tangent or independent UI/OSNAP correction under this S1 slice.

## 4. Acceptance matrix — individually evidenced

**Geometry and arrangement:**
- F1 exactly two cells and audited areas/perimeters within justified numeric error; 24/24 creation permutations; reversed Line direction, rotated/transformed and scale-varied fixtures, Circle seam, partial/open Arc CW/CCW, interior tangency F2.
- Real positive gaps stay gaps regardless of zoom or test scale; two close secant intersections remain two; exact tangent remains one; no arbitrary parameter clamp outside finite Line bounds/Arc sweep; open Line and isolated Circle do not invalidate unrelated disk.
- Externally tangent disks remain two separate material regions; Construction entities do not form boundaries; overlap/duplicates remain fail-closed. Region IDs may differ with creation order, but geometric cells and source provenance are equivalent.
- Trace comparators/face-walk evidence links a concrete source defect to the correction, not merely to new test assumptions.

**Trim and identity:** F3 left/right each one exact tangent; correct bounded cutter statuses and Circle-to-Arc outcome; one/duplicate cutter negative; picking cut contact returns typed ambiguous/not-applicable; near misses and two near secant crossings are distinct; original R12 single-tangent negative unchanged; rejected/no-op calls allocate no IDs or Undo. Valid Trim consumes original Circle ID and allocates fresh Arc ID with unchanged role; one undo/redo transaction and cold native persistence.

**Part consumer:** create actual durable Profile(s) for disk and lower cell; test valid Add/Subtract where material connection is legal, invalid point-only composition stays invalid; inspect source Sketch stage, stable ProfileId and stage-referenced intent. At least one corrected tangent fixture must flow through real current native OCCT Profile Face/Extrude evaluation and **Save/Close/Reopen from native .ss2part**, not only a mock WorkflowKernel. Record actual 3D result and invalid/error cases, including independent clean rebuild after edits; no old cached region reused.

**Regression:** all pre-existing fast/desktop/native cases remain green. No test deletion/weakened assertions to force PASS. Every claimed fixture and negative must identify the exact build SHA and executed Windows lane; label unexecuted or unsuitable native fixtures GAP.

## 5. Exact prospective implementation allowlist — 15 paths

Only **after separate contract integration + ACTIVE-only activation**; a path is permitted exclusively for the narrow role below, and is not mandatory to edit. Missing needed path ⇒ STOP and request exact D2 amendment **before** changing it.

1. \`src/sketch/region_analysis.cpp\` — shared analytical relation, tangent topology and half-edge arrangement/face-walk.
2. \`src/sketch/structural_edit.cpp\` — only legally necessary integration of the corrected shared relation and R12 Trim nonregression.
3. \`tests/pm02f_stage_aware_profile_evaluation_test.cpp\` — existing registered Core/Part region/Profile fixture: F1/F2, permutations and durable stage intent.
4. \`tests/r12_structural_edit_core_test.cpp\` — existing registered R12 core F3 exact tangent, bounded-cutter and negative tests.
5. \`tests/r12_structural_edit_document_session_test.cpp\` — command, fresh IDs and Undo/Redo if affected.
6. \`tests/r12_structural_edit_persistence_test.cpp\` — Circle→Arc identity, roles and cold reopen.
7. \`tests/pm02h_face_supported_extrude_workflow_test.cpp\` — Part command/Profile→Extrude integration with its **test WorkflowKernel**; not a substitute for native OCCT.
8. \`tests/pm02i_face_supported_lifecycle_survival_test.cpp\` — stage, dependent profile and native document lifecycle if required.
9. \`tests/kernel_native/pm00a_profile_cold_rebuild_test.cpp\` — EXISTING registered real native OCCT Profile Face, fresh-process/cold geometry input verification, corrected tangent input; no new native target.
10. \`docs/internal/SHARED_2D.md\` — current-state exact relation/arrangement and R12 semantics.
11. \`docs/internal/PART_DOCUMENTS.md\` — Profile/Feature evaluation and cold-rebuild as-built implications if they change.
12. \`docs/product/pl/PARTS.md\` — visible current Sketch/Profile/Trim supported outcomes/limits.
13. \`docs/product/en/PARTS.md\` — equivalent English Product documentation.
14. \`docs/browser/index.html\` — **generated ONLY** using the unchanged official \`.\ss2.ps1 docs\` generator.
15. **NEW** \`work/SKETCHER_REPAIR_S0_S1_CONTACT_REGION_PROFILE_ACCEPTANCE_EVIDENCE.md\` — factual native/desktop test IDs/SHA, RED/PASS/GAP, Owner outcome.

NO \`tests/CMakeLists.txt\`, \`tests/core_only/CMakeLists.txt\`, new CTest target, \`src/ui/**\`, \`src/application/**\`, \`src/part/**\`, Kernel/OCCT production source or API, \`src/sketch/**\` other than 1–2, new persistence schema/serializer, build scripts, broad CI fixes, Mirror/Copy/Esc, point/grip snapping, PM-06 or other roadmap packages. If the pre-existing native Profile fixture cannot cover a genuine OCCT Extrude end-to-end, report evidence **GAP** or request narrow additional allowlist, **never claim a mocked Extrude test is native OCCT**.

## 6. Required validation cadence, documentation and decisions

- Initial work branch: preserve immutable base-main SHA and RED diagnostics before S1. No upstream mutated user files; source-based Linux historical reproduction is **not** a current Windows run.
- Windows **FOCUSED** existing \`pm02f.stage_aware_profile_evaluation\`, \`r12.structural_edit_core\` and \`pm00a.profile_cold_rebuild\` (real OCCT lane; exact names confirmed by CTest), plus lifecycle witnesses under their registered CTest names. For separate native Core/Qt filters use existing project workflow syntax, not invented job flags.
- Independently run Draft Windows **FAST** (complete supported tier), Ready Windows **FULL** Core-only, OCCT kernel-native and Qt/OCCT Desktop, and official DOC-01 PL+EN/generated Browser freshness; record exact SHA/run IDs/test counts. A source mutation after PASS invalidates the affected prior gate.
- Final exact-HEAD Windows **CLOSURE** after factual evidence sync; separate Owner Windows **manual F1/F2/F3/Profile/Trim/Extrude** result with PASS/FAIL/GAP. Do not fabricate screenshots or a per-case Owner transcript.
- Final Owner technical PASS and any GAP disposition **separate from** merge authorization; protected main guarded squash only on verified exact HEAD, independently authorized by Owner; verify resulting main SHA.
- Evidence must expose remaining numerical accuracy limits, open native/UI witnesses and outcomes. No suppression of prior RED, and no broad “Sketcher repaired” claim for B03–B06.

## Documentation impact

Internal docs: **required** — analytical relation, arrangement/connectedness, deterministic diagnostics and Profile lifecycle semantics.
User/Product docs: **required** — Circle/Arc tangent Profile and Trim user-visible behavior and current limits; canonical Polish/English peers; generated Browser freshness.
Reason: S1 changes observable and reusable CAD geometry behavior, and may change derived evaluation of existing authored Profile references. Neither historical implementation log nor guessed future S2/S3 UX belongs in as-built docs.

## 7. Follow-on program boundary

S2: B03a Circle Radius/Diameter OSNAP and B04 Tangent/Perpendicular stationary-base grip, separate precise point/snap UX contract. S3: investigate B03b Esc with real Qt focus/buffer/locks and separately close B05 Rotate/Scale/Mirror Grip Copy and B06 normal Mirror+Copy, with independent CAD text ownership/one-key-C D2 if desired. Future S2/S3 contract(s) need their own narrower tests and Owner UI approval if interaction grammar changes. This work does not reopen R13 constraints or auto-solver.

**Current request for Owner:** review the 15 exact conditionally editable files, S0→S1 matrix and remaining D2 numerical/compatibility STOPs. Accepting the general S0–S3 program plan was not acceptance of this precise executable file budget; integration, activation, technical acceptance and guarded merge decisions remain independent.
