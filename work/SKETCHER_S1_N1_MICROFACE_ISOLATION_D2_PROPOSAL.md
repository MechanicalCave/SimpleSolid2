# Sketcher S1/N1 — unresolved microface isolation (Owner D2-A) — EXACT SCOPE PROPOSAL

**Status:** **D2-A EXACT §§0–7 POLICY + 11 CONDITIONAL PATHS OWNER-ACCEPTED 2026-10-17; GOVERNANCE PR #470 AWAITING SEPARATE GUARDED SQUASH AUTHORIZATION / MERGE; NOT YET INTEGRATED IN MAIN; NOT ACTIVE; NO NEW MICROFACE PRODUCT IMPLEMENTATION AUTHORITY.**
**Decision class:** D2, narrowly bounded derived Shared-2D arrangement/Region policy; NOT an unrestricted numerical tolerance, global microfeature deletion, persistence change or product-wide healing mechanism.
**Governing priority:** \`AGENTS.md\` → Constitution → Foundation v1.0 FROZEN / Architecture → accepted ADR-0008/0009/0012 and 0014/0016/0017 as relevant → \`work/ACTIVE.yaml\` → \`work/SKETCH_ROADMAP.md\` v1.10 → active S0/S1 B01/B02 Work Contract and its already integrated F2/N1 D2 additions → \`governance/DOCUMENTATION.md\`.
**Historical parent:** \`work/SKETCHER_REPAIR_S0_S1_CONTACT_REGION_PROFILE_D2_WORK_CONTRACT_PROPOSAL.md\` (15 conditional paths); F2 two-path addendum accepted/integrated; \`work/SKETCHER_S1_REAL_GEOMETRY_NUMERICAL_ENVELOPE_D2_PROPOSAL.md\` accepted/integrated and N1 ACTIVE-only PR #467 guarded-squash merged main \`78bd9b6614494727302554e7886d80db3d9592d3\`.
**Owner's latest decision:** Owner explicitly replied **`zatwierdzam i akceptuję - dokoncz formalnosci i rozpocznij pracę`** on 2026-10-17 to a request to accept precisely the **§§0–7 scope and 11 conditional paths of PR #470**. This establishes **EXACT D2-A contract acceptance**. Acceptance does not itself satisfy the separately required guarded-squash permission, independent ACTIVE-only activation or technical implementation/final product merge gates.

## 0. Decision requested and purpose

Owner accepted EXACTLY the isolated policy in §§2–7 and the path/verification budget in §5 (2026-10-17). The goal is to keep independently valid larger Sketch Regions usable when the same connected arrangement includes a certified *non-representable microface*, without turning two authentic secant crossings into a tangent, welding real gaps, forging a Profile boundary or silently ignoring a material barrier.

A **microface** here is a *derived, mathematically bounded arrangement cycle with distinct source intersections* whose interior cannot be certified by the currently supported evaluated binary64 coordinate/witness representation. It is **not** any face whose area happens to be below a fixed threshold, a pixel threshold, the N1 contact band or the minimum user feature envelope. Small but representable regions remain valid candidates; a thin/topologically consequential or uncertain region never disappears merely because it is small.

The proposal is specifically motivated by the documented Owner 3-vs-4 real-world regression; it does not authorize a generalized exact algebraic geometry framework, higher-degree curves, generic constraints, Snap/Copy/Extend UI redesign or arbitrary OCCT small-edge tolerance.

## 1. Exact evidence — preserve all RED, no inferred PASS

**Owner file:** original \`szkic_test_01(2).ss2part\` (native authored scene; owner reports #3 zero Profiles after Extend Both while copied #4 exposes all expected Profiles). Source is authored binary64, not pixel geometry.

**Implementation stack:** Draft [#468](https://github.com/MechanicalCave/SimpleSolid2/pull/468) exact runtime/docs SHA \`9102854d927a31893c7c8ed5577e348a7e87bd05\`, Windows FULL [#3534](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39029769394) **199/199 PASS on that SHA only**; Owner separately reports the prior manual failure cases fixed, but introduced a **new unresolved #3 regression**, so no S1/N1 FINAL PASS.

**Independent new Draft [#469](https://github.com/MechanicalCave/SimpleSolid2/pull/469)** stacked on #468; HEAD at drafting \`36772d3dcf8898856103f99a2e004e04658f83a7\`, base restored to #468, not main and not merged:
- Windows FAST [#3535](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39035084464), *test-only* \`4b90d80865d2320b7e465c1a58afe4f3dfec2be0\`, intentional **RED 92/93**: Circle9–Line26 reported disjoint/0 when exact stored binary64 discriminant is positive with two distinct finite roots; Circle9–Line28 returned 2. Owner #3: **0 Region, 1 diagnostic**; #4: **2 Region, 0 diagnostics**.
- After first scoped predicate fix \`bb300841f357da7d7ef950d19937933218343573\`, Windows FAST [#3536](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39035350188), **RED 92/93**: both #3 lines now yield **2 true distinct secant contacts**, but #3 **still 0 Region / 1 diagnostic**.
- Instrumentation-only [#3537](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39035570873) proved component-loop rejection specifically at \`interiorPoint()\`. [#3539](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39035771407) identified a two-use Circle/Line sliver: **perimeter 1.9674449045939889e-6 mm**, nominally computed **area 4.7322193906518262e-13 mm²**. This naive area is numerically unreliable due to catastrophic cancellation; do NOT use it to label a valid microface or infer true material area. Candidate interior sagitta is at/below the representational ULP scale, subject to independent robust bounding proof.
- Temporary tracing (including \`std::cerr\` in production) was **removed** from latest #469 and source file restored byte-for-byte to candidate #3536. Any later SHA requires new exact-head validation; no claims of green product CI.
- The original #468/#460 and protected main were not changed or merged by these diagnostics.

**Root-cause split:** (i) fast negative discrimination in a near-cancellation case loses positive *exact represented* binary64 secant discriminant, although two finite roots are distinguishable; (ii) after recovery, a microscopic two-use cycle can fail interior-sample construction, causing \`buildComponentLoops\` to fail closed at entire-component scope, hiding independent ordinary regions. Both are real. A mere epsilon increase, blanket skip on \`interiorPoint\` failure or equating the two crossings is unacceptable.

## 2. Owner-accepted exact D2-A semantic contract (MUST prove, never infer)

1. **Real crossings remain real.** Exact-sign/filtered adaptive Line–Circle/Arc classification must preserve two distinct finite secant intersections, their canonical branches, original EntityIds and authored coordinates. Root approximations and finite \`[0,1]\` parameters require demonstrated stability and ordering. One synthetic N1 near-contact is allowed only by its previously accepted separate uniqueness/finite-error policy; it must not override certified real secants. An unresolved pair is explicitly rejected, not silently converted to one contact. Never expand tangent chains, merge positive gaps, or infer authored Tangent/Coincident.
2. **Topological graph retained in full.** A source-anchored microcycle stays present in the derived planar arrangement/face walk and cannot be reclassified as an open boundary, deleted edge or connected material bridge. Only its *promotion to a selectable/material RegionCandidate* can be withheld. No change to persisted Sketch/Profile identity or source geometry. It may remain a derived unresolved face with complete source provenance.
3. **Strict certification (no simple area/length cutoff).** A microcycle can be isolated **only** when deterministic, source-anchored evidence establishes that its interior has no safely representable/valid witness at the accepted evaluation precision, or all attempted interior samples are provably ambiguous *under bounded certified tests*. The proof must account for floating-point rounding, loop geometry, true secant separation, finite parameters and analytic boundary orientation. An unreliable near-zero signed area alone, lack of success from seven scanlines, face perimeter, N1 proximity threshold, zoom level or insertion order is **never** sufficient. If a verifiable geometric/error bound is not feasible with current internal representation, report **GAP/STOP** rather than claim certification.
4. **Local failure isolation only when independent.** Candidate surrounding macrocells must be enumerated and individually validated using original edges and strict adjacency, containment, positive-length shared material connectivity, orientation, loops/holes and independent source provenance. A microface cannot be suppressed if its existence can alter candidate material connectedness, hole meaning, picking ambiguity, parent/child containment, or a persisted Profile's intended boundary; these cases fail closed at the affected candidate/component. No silently fabricated hole or composite face. An unresolved microface remains explicitly unselectable and unconsumable for Profile/Extrude.
5. **Explicit diagnostics.** Use existing \`RegionAnalysis2D\` / \`RegionAnalysisDiagnostic2D\` and owning source EntityIds to represent unresolved topology; do not invent a new public enum/Query/UI API under this proposal. An existing \`invalid_topology\` diagnostic for implicated source(s), plus private/test diagnostics that prove microface classification, is permissible. The original \`complete()\` behavior is not silently weakened. Verify whether actual Region picking/Part consumers permit independently valid candidates when diagnostics also exist. If they currently require global \`complete()==true\` or diagnostics would mislead users, **STOP for a new explicit D2 consumer/API/UI amendment** instead of bypassing it.
6. **Durability and Kernel consistency.** Existing durable \`ProfileRegionIntent\` must replay uniquely after edit/Undo/Redo/Save/Close/Reopen and a clean OCCT Profile Face→Extrude when selected macro geometry is valid. Previously authored genuine tiny Profile or ambiguous/stale intent must not be rebound to a nearby macroface or silently dropped. If a microface cannot be represented as a native wire/solid, its use fails explicitly; no provider-specific fuzzy tolerances. No partial Part operations.
7. **No automatic topology simplification.** Zero-area edges, microscopic legitimate holes, narrow passages, externally tangent independent circles, two near roots, open arcs, seams, overlap/duplicate entities, construction edges and real gaps maintain existing accepted meaning. Performance remains bounded/deterministic; avoid escalating all ordinary precision-path cases to arbitrary exact math without a measured reason.

**Single point of failure:** precise certification that the microface itself is unrepresentable **while adjacent macrocells and durable downstream semantic boundaries remain independently valid**. If certification cannot distinguish these, stop — it is better to report one unresolved component than to invent a manufacturable Profile.

## 3. Candidate technical sequence, not mandatory algorithm

**A0 — RED/trace:** reproduce exact full Owner #3/#4 authored geometry and isolated minimum using existing registered tests; assert precise relation statuses, two distinct root parameters, region counts and negative controls. Preserve #3535/#3536/#3539 RED evidence unchanged.

**A1 — predicates:** retain/validate exact-sign fallback recovering truly positive discriminants only with two representably distinct finite roots. Preserve transformed SK07A historical regression and N1 one-unique-endpoint contact policy. Derive any precision bounds from source bit patterns and conditioning; no new universal fixed epsilon.

**A2 — cell graph:** carry immutable current semantic source and canonical branches through halfedges; for every candidate loop, separate face-walk structural failure from area/point witness failure. Develop a bounded geometric certification (e.g., interval/expansion envelope or equivalent) for a non-representable two-use near-secant lens. A *failed interiorPoint attempt alone* remains inconclusive. Stop for additional D2 if a public topology API or new numeric representation is needed.

**A3 — isolation:** mark certified unresolved microface locally without deleting its halfedges; continue face validation only when graph-consistent. Each valid macroregion must survive individual area/perimeter, point-in-loop, region picking and current Part/Profile consumption. No false material merge across skipped microface.

**A4 — lifecycle and docs:** independent Profile intent, Add/Subtract legal/illegal cases, native OCCT Face→Extrude + cold .ss2part reopening, no test weakening; update canonical PL/EN/internal docs and generate Browser via \`.\ss2.ps1 docs\`.

## 4. Required acceptance and explicit negatives

- **Owner #3** full 26-entity authored scene and isolated Circle9 / Line26 / Line28 with post-Extend Both common apex: recover exactly **2** certified finite crossings per side, keep original source parameters/canonical branches, and recover each independently valid macrocell as selectable/usable Region. Explicit unresolved microface diagnostic must identify affected original geometry. If existing API cannot simultaneously expose valid macro candidates and diagnostic, STOP for D2.
- **Owner #4** copied short-Line Circle59 fixture: retain its **2 existing Regions**, no extra synthetic face, no changes to source or Fill semantics; same results under creation permutations, reversal, transforms and cold reopening.
- **Microface negative:** cannot select/create/extrude microface; cannot turn two true crossings into one tangent; no bogus tiny Profile; no surrogate authored vertex.
- **Other geometry controls:** true gap \`1e-8 mm\` stays OPEN at stated 1 m /100 m envelopes; genuinely representable narrow region remains accessible; distinct near-root secants remain two; tiny but structurally consequential hole/passage blocks affected macroregion if uncertain; externally tangent whole Circle islands remain separate; overlaps and unsupported ambiguity fail closed.
- **Existing registered regression:** R12 one-cutter refusal and two-cutter Circle→Arc fresh-ID Trim, F1/F2 24 permutations and material composition, F3 exact oblique relation, partial/open Arc CW/CCW/seam, SK07A transformed dome, R11/OSNAP historical behavior (unchanged), UI cases 1–6, real native Owner #3/#4 manual pick.
- **Durable acceptance:** ProfileId stable through edit/Undo/Redo and persisted intent re-resolves unambiguously in fresh Part session; native OCCT Face and real Extrude plus .ss2part Save/Close/Reopen after change. Explicit INVALID/GAP for attempted unsupported microface consumption.
- **Performance/correctness:** deterministic results across entity order and direction, no uncontrolled expensive global fallback, no numerical-test oracle based on same faulty double area; report actual test SHA and target for every claimed PASS. Tests cannot be deleted, weakened or marked XFAIL to waive this defect.

## 5. Narrow prospective implementation allowlist (only AFTER all D2 gates)

The following is the **Owner-accepted conditional subset of already historically permitted paths**, with an explicit D2-A semantic role. Approval does not activate the contract for product implementation and does not authorize files outside these paths:

1. \`src/sketch/region_analysis.cpp\` — filtered exact root stability, private derived microface certification, independent face/Region isolation with existing public contracts.
2. \`tests/pm02f_stage_aware_profile_evaluation_test.cpp\` — exact Owner #3/#4 and independent macrocell/microface negative Region/Profile fixtures (existing registered test).
3. \`tests/r12_structural_edit_core_test.cpp\` — exact secant/tangent, Trim after Extend Both and negative invariants (existing registered test).
4. \`tests/sk02b_part_sketch_model_test.cpp\` — existing registered Profile valid/stale and point-contact material-composition negatives; **no new Part production path**.
5. \`tests/kernel_native/pm00a_profile_cold_rebuild_test.cpp\` — actual current OCCT Face→Extrude / .ss2part cold lifecycle, reject microface consumption.
6. \`docs/internal/SHARED_2D.md\` — as-built certified arrangement/error handling.
7. \`docs/internal/PART_DOCUMENTS.md\` — actual durable Profile/current failure meaning.
8. \`docs/product/pl/PARTS.md\` — Polish current supported/unsupported visible outcomes.
9. \`docs/product/en/PARTS.md\` — English peer current supported/unsupported outcomes.
10. \`docs/browser/index.html\` — **generated only**, never manually edited.
11. \`work/SKETCHER_REPAIR_S0_S1_CONTACT_REGION_PROFILE_ACCEPTANCE_EVIDENCE.md\` — factual acceptance ledger; only on its owning implementation branch, not fabricated main history.

**Not authorized:** \`src/part/**\` (including historically F2-approved \`profile.cpp\` for this *different* microface policy), public Sketch headers/API, Kernel OCCT production changes, native schema/persistence, \`src/ui/**\`, \`src/application/**\`, construction/OSNAP/Copy, constraints/solver, new tests/CMake targets, \`work/ACTIVE.yaml\` as an implementation edit, CI scripts, ROADMAP/ADR/Foundation. **Missing essential path, unsafe API/consumer contract, global-pick behavior or unresolved numerical semantics ⇒ STOP for exact supplementary Owner D2/D3**, not a stealth extension.

This governance-only acceptance record modifies **one new work/** file** on an independent branch. #469's prior diagnostic source edits are not retroactive product D2-A authority. No production microface changes before exact scope approval + integration + ACTIVE-only activation.

## 6. Gates and Owner decisions — in sequence

1. **SATISFIED 2026-10-17:** Owner explicitly accepted the complete exact §0–§7 text and its 11 conditional paths in PR #470 (`zatwierdzam i akceptuję - dokoncz formalnosci i rozpocznij pracę`). Earlier approval of the *direction only* remains distinct historical evidence. Scope is frozen for this governance gate; further changes require separate review.
2. Governance-only PR on a branch from verified protected main; exact-head Windows **CLOSURE** after final Owner decision text; Owner **separately authorizes guarded squash** to main; verify merge/main SHA. No direct main push.
3. **Separate one-file ACTIVE-only** amendment referencing the integrated D2-A document while preserving existing active Work Contract, Sketch Roadmap v1.10 and all historical \`next\` entries. Its own exact-head Windows governance CLOSURE, separate Owner authorization and verified protected-main squash are mandatory.
4. Only then, in a separate implementation candidate stacked on N1, revise source/test files within the exact approved conditional subset. Preserve initial RED commits and do NOT retroactively relabel those results as PASS.
5. Independent Windows FOCUSED as supported, Draft FAST, Ready FULL (Core+real native OCCT+Qt/Desktop), canonical DOC-01 PL/EN/generated Browser, evidence ledger; exact-HEAD Windows CLOSURE after evidence sync. Report every exact SHA and any GAP.
6. Owner manual Windows comparison on exact resulting product candidate: original cases 1–6, #3 post-Extend Both, #4 short-Line copy, profile pick and Profile→Extrude→Save/Reopen. **Owner manual acceptance**, **technical FINAL PASS**, disposition of any GAP and **separate guarded product-merge authorization** are independent gates. No merge into main from automatic green CI.

No automatic changes to S2/S3, RG-02, PM-06, Projection/Assembly/Drawing, Edge Feature D2-B or closed AUDIT-01. No revived unrelated work.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: D2-A, if later implemented, changes which already-existing mathematically valid but non-representable derived cells may be offered as a Region, the user-visible fail-closed diagnostics, and durable Profile/OCCT consumption behavior. Current-state internal and PL/EN Product docs need precise updates only with actual accepted product code; generated Browser must be regenerated from Markdown.

## 7. Decision log

2026-10-17 — Owner said \`akceptuję - kontynuuj pracę\` in response to **direction A**, meaning *certified local non-representable microface isolation with remaining valid macroregions, without merging two real intersections*. **D2-A direction accepted**.

2026-10-17 — After receiving Draft governance PR #470 and a specific request to accept its exact §§0–7 and 11 conditional allowlisted paths, Owner explicitly answered **`zatwierdzam i akceptuję - dokoncz formalnosci i rozpocznij pracę`**. **EXACT D2-A POLICY AND SCOPE ACCEPTED**. Independent final-head Windows CLOSURE, protected-main guarded-squash permission and integration, one-file ACTIVE-only gate, and implementation/manual/technical/merge decisions remain mandatory. This decision log is acceptance evidence, not code authorization.
