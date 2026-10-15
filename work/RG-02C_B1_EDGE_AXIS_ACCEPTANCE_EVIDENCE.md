# RG-02C/B1 — one straight Body Edge Axis: production implementation evidence ledger

**Status:** H1 product-native real OCCT FOCUSED **#3247 1/1 PASS** on source/test exact SHA `6081ffb05d6fe9a252757d08ce93c73c37af2e1f`; FAST/FULL/schema/native v19/GUI all PENDING. NOT a finished durable Axis, no Owner technical FINAL PASS or production protected-main merge.
**Date:** 2026-10-15.
**Active contract:** `work/RG-02C_B1_SINGLE_STRAIGHT_EDGE_AXIS_LIFECYCLE_D2_WORK_CONTRACT_PROPOSAL.md`, Owner delegated authority to implement; Work Features Axis UX Owner approved (`Edge` ONLY, existing Sketch Axis stays in Sketch, Edge Axis in Reference Geometry); contract PR #411 and ACTIVE PR #412 independently guarded-MERGED protected main `a9cec1d786bede4a4e6af624d5c1bb08eea7e795`.
**Allowed paths:** exact §0A of contract; this first increment modifies only pre-existing `src/part/include/simplesolid2/part/axis_evaluation.hpp`, `src/part/axis_evaluation.cpp`, `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` plus this ledger.

## First incremental H1 witness — what it does and does NOT do

- Product `resolveStraightMaterialEdgeAxisLine(document, source, evaluation, native_provider)` accepts only a strict, current-stage `MaterialEdgeReference`, invokes the existing production `resolveSelectedMaterialEdgeMid` from Q2 (not a duplicate Edge resolver), checks resolved catalog `CurveKind::line`, and returns the **derived infinitely unoriented** `AxisEvaluation` line + required stage, with typed missing/ambiguous/unsupported/native/stale diagnostic and **no payload on failure**.
- The already-registered real native `pm05c2b.part_edge_topology` adds only the new **PRODUCT** consumer assertions atop existing Q0/Q2/A0 fixture: positive strict line, geometric unoriented equivalence to A0, Arc rejection, different producer FeatureId and stale revision rejection, synthetic Q2 ambiguity refusal and full Part v18 cold reopen/new OCCT provider line recovery.
- Existing A0 test-only `a0StraightEdgeLine` is intentionally retained for independent cross-check and provenance. Product code uses the upstream Q2 resolver, not copies from A0 test.
- This H1 increment **does not yet** add new durable `PartAxis` source variant, native v19 file writer, native Axis Create/Edit command, viewer/Operations/Tree/CLI or downstream Revolve integration. These are *subsequent increments required* before B1 as a whole may claim production completion. Unresolved native B-Rep Edge topology actual reversal fixture remains a real **GAP**, not a successful test.

## Actual CI witnesses — record only after completion

| Gate | Exact SHA/run | Result |
| --- | --- | --- |
| Windows native FOCUSED `pm05c2b.part_edge_topology` | [#3247](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38776602273), SHA `6081ffb05d6fe9a252757d08ce93c73c37af2e1f` | **1/1 PASS — actual Windows MSVC + native OCCT** |
| Draft FAST (Core + Kernel) | PENDING | **NOT YET RUN** |
| Ready FULL (Core + native Kernel + Qt/OCCT Desktop) | PENDING | **NOT YET RUN** |
| DOC-01 internal + PL/EN + generated Browser | PENDING | **NOT YET RUN** |
| Owner GUI/Command Line manual whole lifecycle | PENDING | **NOT IMPLEMENTED / NOT RUN** |
| Owner technical FINAL PASS + guarded merge | PENDING | **NOT AUTHORIZED AS COMPLETED** |

No binary, codegen, provider-native identity, new CMake target or CI bypass is permitted. If a real test reveals a need to change excluded `src/kernel/**`/`src/kernel_occt/**` files, STOP and request a specific D2 amendment rather than widening silently.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: eventual complete Axis Edge feature changes Part semantics, native persistence, Workbench/Tree and interaction. These remain PENDING until actual end-to-end shipping and DOC-01 verification; this factual `work/**` ledger is not as-built product documentation.
