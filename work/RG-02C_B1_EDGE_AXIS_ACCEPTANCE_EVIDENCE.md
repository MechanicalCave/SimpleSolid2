# RG-02C/B1 — one straight Body Edge Axis: production implementation evidence ledger

**Status:** H1 native product line **FOCUSED #3247 1/1 PASS**, H1 Draft FAST **#3248 SUCCESS** at `56b99cd33cdbe6b3d6d74aa6777e38fe1e3892b5`, H1 Ready FULL PENDING; **H2 native Part v19 durable strict Edge Axis source/model/parser and cold-OCCT provider tests are NOW IMPLEMENTED AS A CANDIDATE, FOCUSED PENDING**. Not a completed GUI/CLI/revolve lifecycle. No Owner technical FINAL PASS or guarded product merge.
**Date:** 2026-10-15.
**Active contract:** `work/RG-02C_B1_SINGLE_STRAIGHT_EDGE_AXIS_LIFECYCLE_D2_WORK_CONTRACT_PROPOSAL.md`, Owner delegated authority to implement; Work Features Axis UX Owner approved (`Edge` ONLY, existing Sketch Axis stays in Sketch, Edge Axis in Reference Geometry); contract PR #411 and ACTIVE PR #412 independently guarded-MERGED protected main `a9cec1d786bede4a4e6af624d5c1bb08eea7e795`.
**Allowed paths:** exact §0A of contract; this first increment modifies only pre-existing `src/part/include/simplesolid2/part/axis_evaluation.hpp`, `src/part/axis_evaluation.cpp`, `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` plus this ledger.

## First incremental H1 witness — what it does and does NOT do

- Product `resolveStraightMaterialEdgeAxisLine(document, source, evaluation, native_provider)` accepts only a strict, current-stage `MaterialEdgeReference`, invokes the existing production `resolveSelectedMaterialEdgeMid` from Q2 (not a duplicate Edge resolver), checks resolved catalog `CurveKind::line`, and returns the **derived infinitely unoriented** `AxisEvaluation` line + required stage, with typed missing/ambiguous/unsupported/native/stale diagnostic and **no payload on failure**.
- The already-registered real native `pm05c2b.part_edge_topology` adds only the new **PRODUCT** consumer assertions atop existing Q0/Q2/A0 fixture: positive strict line, geometric unoriented equivalence to A0, Arc rejection, different producer FeatureId and stale revision rejection, synthetic Q2 ambiguity refusal and full Part v18 cold reopen/new OCCT provider line recovery.
- Existing A0 test-only `a0StraightEdgeLine` is intentionally retained for independent cross-check and provenance. Product code uses the upstream Q2 resolver, not copies from A0 test.
- This H1 increment **does not yet** add new durable `PartAxis` source variant, native v19 file writer, native Axis Create/Edit command, viewer/Operations/Tree/CLI or downstream Revolve integration. These are *subsequent increments required* before B1 as a whole may claim production completion. Unresolved native B-Rep Edge topology actual reversal fixture remains a real **GAP**, not a successful test.

## Second incremental H2 candidate — typed PartAxis + native Part v19, not yet CI PASS

- `PartAxis` now holds a discriminated `PartAxisSource` with original `SketchLineAxisSource` OR `StraightMaterialEdgeAxisSource{MaterialEdgeReference}`; no invented SketchId, world XYZ, native token or cached line. Original PM-04 aggregate-style Sketch Line Axis construction retained by explicit compatibility constructor.
- `resolveAxisReference` supports an optional exact native Edge geometry provider for the new stage-qualified source, while existing Sketch Line and Origin evaluation behavior stays on its previous paths. Without a provider Edge Axis fails closed with typed diagnostic. Part history rejects forward/self dependencies for restored Edge Axis/Revolve consumption; deleted producer source remains repairable intent.
- The v19 native Part writer serializes new `kind=straight_body_edge` with ONLY the existing stage-semantic `materialEdgeReferenceJson` payload. The reader supports v12-v18 `kind=sketch_line` unchanged, plus strictly typed `straight_body_edge` only in v19, reusing existing `parseMaterialEdgeReferenceV14` parser and FeatureId cursor authority. Older native files must retain old Sketch meanings. Invalid/malformed new source is refused rather than guessed.
- The existing real native OCCT `pm05c2b.part_edge_topology` test now independently constructs one current-stage strict Body Edge `AxisId` in a restored Part, saves Part v19 to disk, fully loads into a new PartDocument and tests the `AxisReference` resolves from the reloaded semantic source using a fresh OCCT provider to the same infinite unoriented line; provider absent blocks without payload. The original A0 witness persists unmodified.
- **H2 is NOT YET VALIDATED.** Expect and diagnose real MSVC/native/legacy-fixture failures before recording any PASS. Previous v18 new-writer version assertions may require a small exact legacy fixture D2 scope amendment; do not weaken v9/v12/v14/v18 structural tests or simply ignore red tests.
- **Still PENDING:** runtime Create/Edit/Delete through Commands/AxisDraft (not just restored test record); downstream Revolve exact-provider semantic dependency; new Work Features `Axis` button with Constructor `Edge` only, Tree/Properties/CLI full lifecycle and DOC-01. Real B-Rep topological Edge orientation reversal remains explicit GAP; A0 acceptance cannot certify it.

## Actual CI witnesses — record only after completion

| Gate | Exact SHA/run | Result |
| --- | --- | --- |
| Windows native FOCUSED `pm05c2b.part_edge_topology` | [#3247](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38776602273), SHA `6081ffb05d6fe9a252757d08ce93c73c37af2e1f` | **1/1 PASS — actual Windows MSVC + native OCCT** |
| Draft FAST (Core + Kernel) | [H1 #3248](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38776932834), SHA `56b99cd33cdbe6b3d6d74aa6777e38fe1e3892b5` | **H1 SUCCESS; H2 PENDING** |
| Ready FULL (Core + native Kernel + Qt/OCCT Desktop) | PENDING | **NOT YET RUN** |
| DOC-01 internal + PL/EN + generated Browser | PENDING | **NOT YET RUN** |
| Owner GUI/Command Line manual whole lifecycle | PENDING | **NOT IMPLEMENTED / NOT RUN** |
| Owner technical FINAL PASS + guarded merge | PENDING | **NOT AUTHORIZED AS COMPLETED** |

No binary, codegen, provider-native identity, new CMake target or CI bypass is permitted. If a real test reveals a need to change excluded `src/kernel/**`/`src/kernel_occt/**` files, STOP and request a specific D2 amendment rather than widening silently.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: eventual complete Axis Edge feature changes Part semantics, native persistence, Workbench/Tree and interaction. These remain PENDING until actual end-to-end shipping and DOC-01 verification; this factual `work/**` ledger is not as-built product documentation.
