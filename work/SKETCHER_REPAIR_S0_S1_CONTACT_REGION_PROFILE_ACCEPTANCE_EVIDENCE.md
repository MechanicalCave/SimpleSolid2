# Sketcher S0/S1 — B01/B02 tangent / arrangement / Profile repair — factual acceptance evidence

**Status:** IMPLEMENTATION IN PROGRESS — S0 real Windows RED CONFIRMED; S1 NOT YET TECHNICAL PASS.  
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

## 3. Remaining contractual acceptance gates

**Open, NOT PASS / NOT TESTED until actual evidence:** 24/24 F1/F2 exact geometry and area, F3 shared predicate and R12 circle Trim identity/Undo, Arc CW/CCW/seam, reversals/rotations/scales, finite cutter near-misses/secants and gap negatives, point-only separate material, connected Profile Add/Subtract, durable RegionIntent, real native OCCT Profile Face→Extrude and `.ss2part` cold rebuild, Windows FOCUSED/FAST and independent Ready FULL 25+57+117 (exact executed counts to be recorded), current PL/EN/internal docs and generated Browser DOC-01, a final evidence-only exact-HEAD Windows CLOSURE, Owner Windows manual results, distinct Owner technical FINAL PASS and separately authorized feature guarded merge. Nothing from H2's 199/199 test run is borrowed as S1 PASS.

**Prior governance failure unrelated to Sketcher numerical behavior:** first ACTIVE #456 [#3459](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38972422222) genuine RED for DOC-01 impact syntax, CLOSED UNMERGED; minimal canonical correction #457 Ready #3462 SUCCESS and guarded squash `090c58006420ec8f747a136f31f198fffdc33529`; clean successor #459 Ready #3463 SUCCESS, merged main `0eec634398f235d3be282f1a223aeaca41c3854c`. #458 prior stacked branch CLOSED UNMERGED because Git's merge-base compare would show more than one file despite identical contract text. No governance RED is concealed.

**S2 and S3 remain OUT OF SCOPE:** Circle/Grip snap (B03a/B04) and Esc/Copy/Mirror (B03b/B05/B06) require later separate contracts. No UI change or new schema/API/CMake/CI/solver is authorized here.
