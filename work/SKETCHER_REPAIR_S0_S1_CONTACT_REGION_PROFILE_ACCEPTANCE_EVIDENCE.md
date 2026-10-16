# Sketcher S0/S1 — B01/B02 tangent / arrangement / Profile repair — factual acceptance evidence

**Status:** IMPLEMENTATION IN PROGRESS — S0 real Windows RED CONFIRMED; S1 Shared-2D FAST #3471 93/93 PASS, but deliberate durable Part/F2 witness FAST #3472 RED; no technical FINAL PASS.  
**Contract:** `work/SKETCHER_REPAIR_S0_S1_CONTACT_REGION_PROFILE_D2_WORK_CONTRACT_PROPOSAL.md` (Owner D2 accepted, governance #454 merged).  
**Activation:** clean ACTIVE-only [#459](https://github.com/MechanicalCave/SimpleSolid2/pull/459), Windows exact-head #3463 SUCCESS, guarded squash `0eec634398f235d3be282f1a223aeaca41c3854c`. Sketch Roadmap v1.10 / Part Roadmap v1.34 Owner-accepted; H2 completed.  
**Implementation:** Draft [#460](https://github.com/MechanicalCave/SimpleSolid2/pull/460), based on that activated main. Owner authorized commencing S0/S1. No product source changed before the real source-baseline RED below.

## 1. Historical audit vs genuine current-head Windows RED

The 2026-10-07 audit originated on old source `9547b986ccbd4dc7533c446c797eb5c4886ad0a6`; historical Linux probes are background evidence, **not** current Windows execution. The S0 test-only implementation branch began at current post-activation main with **unmodified** product algorithms, adding:
- `tests/pm02f_stage_aware_profile_evaluation_test.cpp`: all 24 permutations F1/F2, exact area expectations and diagnostic counters; F3 Line/Circle relation witness added on later test-only commit.
- `tests/r12_structural_edit_core_test.cpp`: F3 exact oblique relation and Circle→fresh Arc R12 Trim assertions. Existing one-cutter-tangent negative preserved.

**Actual Windows FAST run [#3464](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38974425176)** on test-only SHA `6c63734bb7821818a4c38f88790c1f84b5a943c8`: **RED**, 92/93 PASS, sole failure newly added `pm02f.stage_aware_profile_evaluation`. Logged:
- F1 endpoint tangencies: **correct 0/24**, exactly **12/24 zero regions, 12/24 one region, 0/24 two regions**.
- F2 internal tangencies: **correct 0/24**, exactly **24/24 one region**, 0/24 two.
- This reproduces the independent audit's insertion-order dependence and interior-tangent omission. No existing FAST test was weakened or skipped to get this RED.

**Actual Windows FAST run [#3465](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38974691902)** on later **test-only** SHA `e75573939279ba842264d39261c979b2f6dcd065`: **RED**, same B01 counts and newly logged F3 classification:
- `SKETCH_S0_B02_F3_RELATION left=0 right=0 left_status=1 right_status=1`; `CurveRelationStatus::disjoint` (enum value 1) on both exact oblique tangent cutters. Both F3 contacts are **not found** on actual unchanged Windows baseline.
- FAST fails the expected new S0 assertion before the separate R12 F3 test can be counted as executed. The separate R12 R12 Circle→Arc witness is **NOT YET RUN** in a Windows FULL or independent FOCUSED lane on this test-only SHA. Do not extrapolate a Trim PASS or formal native F3 result from audit history.

## 2. First bounded S1 product change — awaiting real Windows positive evidence

After the two real RED baselines, the Draft #460 branch modified **only** the already-approved `src/sketch/region_analysis.cpp`:
- `lineCircle` now uses a binary power-of-two-scaled signed analytic discriminant before normalization; no global distance epsilon, endpoint clamping, auto-gap healing or Trim-specific predicate.
- tangent vertices include interior contacts; the private topology record retains contact kind.
- outgoing Line/Circle/Arc half-edges derive analytic radial tangents at common contact vertices and sort coincident tangent directions by signed curvature before stable index tie-break, instead of allowing allocation order to decide topological face side.
- The new geometry and negative invariance tests still need real verification; no claim of complete numerical robustness, arbitrary dynamic-range or near-tangent true-positive/negative accuracy on untested fixtures.
- Current product candidate SHA `af38006d9544e613e6e72787fc5021aff0aa852a`, initial Windows Draft FAST **#3468 pending at time of entry**. This entry is not a PASS record for it.

## 3. Subsequent actual S1 Windows results and bounded Part replay STOP

**Windows Draft FAST [#3469](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38975050730)** on first Shared-2D product algorithm + ledger HEAD `2f3bd9992b6e0cf256be40f7e320d8c55b9fbc6b`: **RED 92/93**, sole existing `sk02b.part_sketch_model` regression: attempting Add Area for two externally tangent complete disks must return `disconnected_result`, but did not. New S0 F1/F2/F3 fixture **did not fail**. This is an actual semantic regression, not a waived test.

**Windows Draft FAST [#3470](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38975296159)** after first material connectivity guard on HEAD `c771af5caaa624b33ffffc00f3d7c126c8c2b940`: **RED 92/93**, same old `sk02b.part_sketch_model` point-tangent Profile status regression. Retained true RED.

**Windows Draft FAST [#3471](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38975536644)** on HEAD `d6ae2dda886da9726c94b9c9cefb551baf8bddfa` after additional analytic point-contact circle island distinction: **SUCCESS 93/93**. This exact check includes newly registered B01 F1/F2 24-permutation two-cell positive assertions and B02 F3 exact oblique tangent predicate assertions, as well as all historical FAST tests, including independent externally tangent disk Add Area `disconnected_result`. A Draft FAST is not a Ready FULL or native OCCT proof.

**Windows Draft FAST [#3472](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38975757976)** on HEAD `233fd50617a2bdadcbc7c6198b1ec108233c3a50` after additionally registering **positive Part ProfileRegionIntent replay** assertions in an originally allowed test: **RED 92/93**, only `pm02f.stage_aware_profile_evaluation` failed at the **new F2 durable positive check**. Exact log:
- `SKETCH_S0_B01_ENDPOINT_24 correct=24 region0=0 region1=0 region2=24`
- `SKETCH_S0_B01_INTERIOR_24 correct=24 region0=0 region1=0 region2=24`
- `SKETCH_S0_B02_F3_RELATION left=1 right=1 left_status=2 right_status=2` (discrete)
- `SKETCH_S0_PROFILE_REPLAY_SUMMARY extended=0 resolved=2 expected=2` — **F1 Part durable replay 2/2 PASS**
- `SKETCH_S0_PROFILE_REPLAY extended=1 region_area=50.7301 status=4` — **F2 lower pocket unresolved_intent**, while `extended=1 resolved=1 expected=2`.

Independent as-built finding: `src/part/profile.cpp::resolveProfileRegionIntent` returns null for **every interior tangent** `CurveContactKind::tangent` with neither source endpoint. F2 uses such legal intersections. The accepted **15-path implementation allowlist forbids editing this Part source** or the existing `tests/sk02b_part_sketch_model_test.cpp`. **STOP at this new D2 boundary**: do not forge endpoint flags, relax source ownership, rewrite persistent schema or modify Part without separately accepted exact scope. A governance-only **Draft D2 addendum PR [#461](https://github.com/MechanicalCave/SimpleSolid2/pull/461)** proposes exactly those TWO additive paths, and no broader authorization. At the time of this ledger entry #461 is PROPOSED/UNMERGED/NOT ACTIVE for added paths.

**Current staged S1 positive and negative evidence remains incomplete:** R12 F3 structural Trim real Windows lane, Arc CW/CCW/seams, numeric scale/reversal invariance, native Profile Face and real OCCT Extrude/Cold Reopen, Docs internal/PL/EN/Browser, final Ready FULL/exact-head CLOSURE, Owner manual/technical FINAL PASS. #3472 RED must remain visible; there is no product merge authority.

## 4. Remaining contractual acceptance gates

**Open, NOT PASS / NOT TESTED until actual evidence:** 24/24 F1/F2 exact geometry and area, F3 shared predicate and R12 circle Trim identity/Undo, Arc CW/CCW/seam, reversals/rotations/scales, finite cutter near-misses/secants and gap negatives, point-only separate material, connected Profile Add/Subtract, durable RegionIntent, real native OCCT Profile Face→Extrude and `.ss2part` cold rebuild, Windows FOCUSED/FAST and independent Ready FULL 25+57+117 (exact executed counts to be recorded), current PL/EN/internal docs and generated Browser DOC-01, a final evidence-only exact-HEAD Windows CLOSURE, Owner Windows manual results, distinct Owner technical FINAL PASS and separately authorized feature guarded merge. Nothing from H2's 199/199 test run is borrowed as S1 PASS.

**Prior governance failure unrelated to Sketcher numerical behavior:** first ACTIVE #456 [#3459](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38972422222) genuine RED for DOC-01 impact syntax, CLOSED UNMERGED; minimal canonical correction #457 Ready #3462 SUCCESS and guarded squash `090c58006420ec8f747a136f31f198fffdc33529`; clean successor #459 Ready #3463 SUCCESS, merged main `0eec634398f235d3be282f1a223aeaca41c3854c`. #458 prior stacked branch CLOSED UNMERGED because Git's merge-base compare would show more than one file despite identical contract text. No governance RED is concealed.

**S2 and S3 remain OUT OF SCOPE:** Circle/Grip snap (B03a/B04) and Esc/Copy/Mirror (B03b/B05/B06) require later separate contracts. No UI change or new schema/API/CMake/CI/solver is authorized here.
