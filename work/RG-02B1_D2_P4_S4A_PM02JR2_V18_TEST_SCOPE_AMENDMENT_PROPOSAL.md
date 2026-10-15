# RG-02B1 / P4-S4A — v18 legacy PM-02JR2 native test-only D2 amendment proposal

**Status:** OWNER D2 APPROVAL REQUIRED — not accepted, not active, no executable authority.
**Date:** 2026-10-15.
**Base active Work Contract:** `work/RG-02B1_D2_P4_S4A_THREE_END_POINTS_V18_HEADLESS_WORK_CONTRACT_DRAFT.md`; Owner D2 accepted PR #393 and activated through ACTIVE-only PR #394, main `3ce618d0045bc6681f501b32db60cc85373cff08`.
**Implementation PR:** [#395](https://github.com/MechanicalCave/SimpleSolid2/pull/395), no production merge/Owner FINAL PASS.
**Normative:** Constitution, Foundation, Architecture, accepted ADR-0012–0017, P4-S4A headless v18 contract, DOC-01. P0 / Q1 native geometry tests remain intact.

## Exact blocker / evidence

Real Windows Ready FULL [#3123](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38704879754) on PR #395 SHA `99ad7dc5fe65491f1b6a4d16e274ab3ccb90c3db` completed **RED 198/199**: Core **25/25**, kernel-native **56/57** (one failure), Desktop not reached. The only kernel failing test is `pm02jr2.add_surface_continuation`. Its actual assertion at line **1521** of `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp` expects `domain_schema_version == 17` after a real `PartDocumentStore::createNew` write; S4A intentionally made the current native writer v18. The same native test's `rewrite_package` helper at lines 1532–1546 already refuses to downgrade nonempty **v17** `edge_point_datum_planes`, but it does **not** strip or refuse nonempty new **v18** `three_point_datum_planes` from its intentionally constructed historical v15 fixture.

The test file is **not** in S4A's exact executable allowlist; do not silently modify it under current authority. Its separate genuine historical v16 fixture assertion at line ~6351 and all Geometry/continuation/Point-specific witnesses must remain unchanged.

## Single D2 change requested

**Add precisely ONE existing registered test path**, only to align its version expectations and synthetic legacy fixture construction:

`tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp`

Allowed edits, bounded to the existing v18/v17 versioning fixture assertions:

1. For a genuinely **current new Part store output**, expect `domain_schema_version == 18`, not 17.
2. Before repackaging a **current v18** authored payload as historical v15 or v16, demand that `three_point_datum_planes` exists and is an **EMPTY** JSON array. If it contains authored 3-Point intent, **FAIL** fixture construction rather than dropping data.
3. Only after asserting emptiness, remove the **v18-only** collection; retain the existing `version < 17` guarded check that `edge_point_datum_planes` is empty before removing it when targeting v15/v16. Keep all authentic old-schema semantics and the deliberate nested semantic-provenance negative tests.
4. Retain the actual v16-file `==16` assertions. Do not rewrite or relax native Part loader, provider, feature evaluation, CMake/CI or historical Q1/Q2/P0 tests.

**No new test executable, no source code outside this one path, no schema/public API changes, no product/UI/Viewer, no broadened S4A or S4B scope.** Other S4A allowlisted files remain separately governed by original contract.

## Acceptance

- The targeted `pm02jr2.add_surface_continuation` native test PASS, retaining all existing positive/negative checks.
- Windows actual FOCUSED, FAST where applicable, and Ready FULL Core/Kernel/Desktop PASS on the exact amended code/tree (include S4A #395), with no ignored/cancelled RED passed off as success.
- Existing v15/v16 historical fixture semantics remain valid and cannot silently drop a nonempty v18 3-Point collection.
- Evidence ledger names former Windows FULL #3123 RED, precise cause and retest result. This is versioned fixture maintenance, not architectural evolution.

## Governance gates

1. Owner D2 accepts **this exact one-path additive S4A test-only allowlist amendment**.
2. Governance-only document PR passes exact-head Windows closure and obtains separate protected-main guarded squash authorization.
3. Separate `work/ACTIVE.yaml`-only history amendment references the accepted test-only scope after its own CI and guarded squash.
4. Only after both protected-main approvals can implementation PR #395 add this file and rerun real native/ready FULL. Any further non-allowlisted failure remains STOP for a new bounded Owner decision.
5. Existing S4A technical FINAL PASS, protected-main production squash, and subsequent S4B GUI/Command Line contract remain **separate** gates. Do not infer success from this proposal.

## Documentation impact

Internal docs: none for this test-only amendment. User/Product docs: none. DOC-01 as-built for S4A v18 remains in original work contract and is not waived here.
