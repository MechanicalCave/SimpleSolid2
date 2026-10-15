# RG-02B1 / P4-S4A — Strict v18 legacy-fixture regression test-only D2 amendment

**Status: OWNER D2 APPROVAL REQUIRED — NOT ACCEPTED / NOT ACTIVE / NOT EXECUTABLE.**
**Proposed:** 2026-10-15. This supersedes the earlier one-test draft scope on this same governance PR #396 after Windows FAST #3137 identified additional historical fixture witnesses.
**Base authority:** Owner D2-accepted `work/RG-02B1_D2_P4_S4A_THREE_END_POINTS_V18_HEADLESS_WORK_CONTRACT_DRAFT.md`, governance PR #393 and ACTIVE-only PR #394 independently guarded squash-MERGED, protected main `3ce618d0045bc6681f501b32db60cc85373cff08`. The original active S4A contract does NOT list any of the five tests below.
**Production source PR:** [#395](https://github.com/MechanicalCave/SimpleSolid2/pull/395) (Draft/unmerged, no Owner technical FINAL PASS). This D2 proposal changes no code.

## Evidence requiring an explicit addendum

1. Windows Ready FULL [#3123](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38704879754), exact `99ad7dc5fe65491f1b6a4d16e274ab3ccb90c3db`: **RED** Core **25/25 PASS**, kernel-native **56/57**, one genuine failure `pm02jr2.add_surface_continuation`. Current native writer now intentionally emits **v18**, but `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp:1521` still asserts **v17**. The same test synthesizes v15/v16 packages using a helper that strips the empty v17 EdgePoint collection but not the **new empty v18 ThreePoint** collection. A separate genuine historical schema-v16 assertion near line 6351 must stay 16. Desktop FULL not reached.
2. Windows Draft FAST [#3137](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38709767904), exact `73f71d0bd5f1dce3d53b0b28edbdd6a6eaea93bb`: **RED 88/93 passed, four explicit failed tests plus one Not Run**. Failures are immediate hardcoded native current-v17 expectations, *not* native modeling counterexamples:
   - `pm02e.sketch_support_schema_v9`, line 384 expects current writer v17;
   - `pm04a.axis_schema_v12`, line 135 expects current `PartDocumentStore::current_schema_version == 17`;
   - `pm04e1.revolve_persistence`, line 270 expects current schema 17;
   - `pm05b3.generated_surface_reference_v14`, line 478 expects current schema 17.
   - `pm04c1.revolve_semantic_kernel` was **Not Run**; it is **NOT** inferred PASS and NOT included in the new allowlist by precaution. Rerun unchanged in FAST/FULL. All four failing targets also contain genuine historical-schema downgrade fixture code requiring v18 empty-collection guarding, not only a numeric version change.
3. The same FAST run's official `ss2.ps1 docs`/Browser regeneration and documentation validator completed before the version assertion RED. Browser and bilingual docs are still subject to final exact-head Ready FULL.

Native S4A kernel-focused [#3133](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38708941986) **PASS 1/1**, and schema-focused [#3135](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38709549805) **PASS 1/1**, independently support the S4A feature; neither waive real FAST/FULL or the listed historical tests.

## Exact D2 ask — five files, test-only, no product/runtime scope

Add precisely these FIVE EXISTING REGISTERED TEST FILES to the S4A allowlist, for strictly versioned fixture migration and *additive* regression validation:

1. `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp`
2. `tests/pm02e_sketch_support_schema_v9_test.cpp`
3. `tests/pm04a_axis_schema_v12_test.cpp`
4. `tests/pm04e1_revolve_persistence_test.cpp`
5. `tests/pm05b3_generated_surface_reference_v14_test.cpp`

For each applicable authentic `PartDocumentStore::createNew` or new-current native package, change **current** writer-version expectations from 17 to **18**. For each legacy fixture converter targeting schemas v9/v12/v14/v15/v16/v17, first assert a `three_point_datum_planes` JSON array **exists and is EMPTY** before removing that v18-only key if the target version is <18. Continue to assert `edge_point_datum_planes` is EMPTY before removing it for target versions <17. A nonempty authored collection must make synthetic downgrade **FAIL**, never silently lose intent.

Keep **all genuine historical file schema assertions** and negative witnesses untouched (in particular real schema-v16 remains exactly v16); preserve ModelingSemanticsVersion v2 refusal of v1 and all Part/Sketch/Axis/Revolve/Surface/continuation identities, cold-replay and failure probes. No weakening/deleting existing tests, changing their registration, changing their actual historical fixtures to current formats, or adding synthetic sources that pretend old schema has fields introduced later.

**Disallowed:** all other test paths (including `pm04c1` until separately proven necessary), `src/**`, kernel/provider API, native Part parser/serializer, public APIs, GUI/Viewer, CMake/CI, ADR/roadmap/Foundations, v19, general Point authoring, S4B and PM-06. Other originally allowed S4A files continue under their existing contract, not this addendum.

## Finite acceptance / audit

- Each listed registered test passes under an exact current-v18 writer and its historical fixture remains readable/rejectable for the *right* reason; no dropped nonempty v18/v17 intent, missing mandatory fields masked, broadened current parser or hidden failure.
- Windows kernel FOCUSED of PM02JR2 plus relevant desktop/core FOCUSED or FAST, then genuine Ready FULL **Core/Kernel/Desktop** on exact integrated code/test/docs HEAD.
- Re-run `pm04c1.revolve_semantic_kernel` without editing it and report its actual result. Preserve all real RED and cancelled history.
- S4A native OCCT/semantic Point P0/Q1/Q2 and current new v18 headless tests remain green. DOC-01 internal and PL/EN Markdown plus officially regenerated Browser parity remains mandatory.
- Exact commits/CI URLs and Owner **S4A technical FINAL PASS/FAIL** in evidence. Protected-main production squash requires separate authorization; acceptance of this addendum alone does not waive it.

## Required governance before implementation

1. Owner D2 expressly accepts **exactly this five-file native/legacy test-only S4A amendment**.
2. Governance-only amendment PR #396 passes its exact-HEAD Windows closure and receives separately authorized guarded protected-main squash.
3. A distinct `work/ACTIVE.yaml`-only history amendment records extension of the current active S4A allowlist; after its own exact-head Ready closure and Owner-authorized guarded squash, and **only then**, production PR #395 may edit these five test sources.
4. FOCUSED, FAST and genuine Ready FULL with all affected tests PASS, Owner S4A technical FINAL PASS and separately authorized implementation squash. S4B UI/Command Line remains separately governed.
5. Any additional non-allowlisted failure is another concrete D2 STOP: do not infer authority from this proposal or silently adjust production semantics.

## Documentation impact

Internal: none for the **test-only** amendment itself. Product PL/EN: none. Existing S4A v18 DOC-01 as-built/browser obligations remain unchanged and independently tested.
