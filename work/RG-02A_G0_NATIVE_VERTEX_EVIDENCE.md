# RG-02A/G0 — Native VERTEX evidence report (in progress)

**Status:** PARTIAL / G0 NOT CLOSED; no Owner D2 synthesis yet, no FINAL FULL PASS claimed  
**Active authority:** `work/RG-02A_G0_VERTEX_CHARACTERIZATION_WORK_CONTRACT_DRAFT.md`, activated by [PR #331](https://github.com/MechanicalCave/SimpleSolid2/pull/331) (`351840b95b58bdbe7b17085f67b3ddd2cc2cf8c7`) under Part-v1 roadmap v1.32. The historical "NOT MERGED / NOT ACTIVATED" wording in the original G0 Work Contract heading predates these separately approved governance merges; this report does not amend its substantive contract scope.  
**Test source:** `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` (already registered and explicitly allowed). V03 Cut coverage was added in the same file.  
**Evidence source SHA:** `37fed9e523e5dee4113f1d7127f1962ff7a018ca`, Draft [PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332), [Windows FOCUSED run #38304772610](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38304772610), test `pm05c2b.part_edge_topology`.  
**Validation:** V01 exact source SHA `37fed9e523e5dee4113f1d7127f1962ff7a018ca` — Windows FOCUSED 1/1 PASS; **V03 native Add→Cut code SHA `5397db379690949f51a8daf7c831e85a8149a32e` — [Windows FOCUSED run #38306302187](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306302187) 1/1 PASS and aggregate `windows-msvc` PASS**. These are prior tested source commits; a subsequent evidence-document commit needs its own gate. FAST and FULL are not yet claimed PASS.

## 1. Proven V01 result — real OCCT and actual Part-stage topology

Native fixture: rectangular Sketch/Profile, Part Extrude Add 40×30×20 mm, evaluated by production `part::evaluatePart()` using production `kernel_occt::OcctSolidModelingKernel` at the specific after-Extrude Body stage.

**V01: PASS within the measured scope.** The native test asserts and passed:

- Exactly **8 current Body Vertex topology records**, **8 with `ReferenceStatus::resolved`**.
- Each Resolved Vertex has exactly one valid current `FeaturePointAddress` candidate, with **exactly three semantic Surface addresses**.
- Each such Point address is present in the **real current Body-stage `points` catalog** as `ReferenceStatus::resolved` with **exactly one current native Vertex token**, and that token is the same one as the examined native Vertex.
- Zero Ambiguous / Unsupported current native Vertices for this canonical fixture. No inferred XYZ-based identity, nearest-Vertex match, screen-space selection or cached prior-Body geometry.

**What this does not prove:** persisted Datum/Axis construction-point authoring, Save/Reopen/cold rebuild of a *new* point-source schema, or end-user viewport pickability. These features do not exist in G0 and have not been tested.

## 2. V03 — real Part Extrude Add → Cut semantic continuation (partial)

**V03: PASS for its tested assertions, but FULL classification table pending.** A new Part Sketch/Profile on Origin XY defines an interior 20×20 mm rectangle; a second Part Feature performs a 10 mm one-sided Cut into the existing 40×30×20 mm Extrude prism. Unlike a fabricated semantic catalog, production `part::evaluatePart()` evaluates **both actual stages using OCCT**. [Windows FOCUSED run #38306302187](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306302187) on exact source SHA `5397db379690949f51a8daf7c831e85a8149a32e` PASS (1/1).

Verified assertions:
- The actual Cut Body is `up_to_date`, with a valid two-Feature history and complete stage-specific topology.
- The resulting Cut and prior Extrude stages have different semantic `BodyStageRef` identities.
- Cut produces **more current native Vertices than the original eight**.
- **At least eight** Resolved current Cut Vertices retain exactly the semantic `FeaturePointAddress` identities present after original Extrude. **At least one** additional current Resolved Vertex has a new, different `FeaturePointAddress`.
- Every current Cut Vertex marked Resolved has exactly one valid three-Surface semantic Point witness with one-to-one same-stage runtime realization (as checked by the existing G0 characterization helper). No nearest-XYZ/provider-token authoring or reattachment.

**Open V03 evidence:** The exact number/status distribution of every new Cut Vertex and concrete non-three-Surface examples is **not yet captured** by the success-only CTest log; the helper prints counts but default CTest suppresses passing-test stdout. Native Face incidence versus *resolved semantic Surface* adjacency also requires its separately bounded provider/raw observation. No full V03 closure and no V10 claim yet.

## 3. V05/V06 partial execution — classification witness checks, exact distribution pending

The same passing FOCUSED test executes `characterizeNativeVertices` on real, successful Part Fillet and Chamfer stages (`V05-fillet`, `V06-chamfer`). At each stage it asserts a complete Part topology catalog, accounted Vertex statuses, and that **any resolved Vertex** has a one-to-one three-Surface `FeaturePointResolution` witness. These invariant checks passed.

**V05 and V06 status: PARTIAL / classification totals not yet published**. The helper prints `native/resolved/ambiguous/missing/unsupported` counters on stdout, but the default successful CTest transcript only shows 1/1 PASS and **does not expose those counter lines** in the fetched GitHub Actions job log. We therefore do **not** claim observed totals, a demonstrated non-three-Surface Vertex, or full V05/V06 characterization until the actual counters are captured with a suitable authorized test execution. Do not infer nonzero Unsupported from the source code alone.

## 4. Remaining VERTEX matrix (not executed or not demonstrated)

| Case | Evidence status | Outstanding proof |
| --- | --- | --- |
| V01 | **PASS (measured scope)** | Future lifecycle proof is V02/V11, not part of V01 |
| V02 | UNRUN | Upstream edit, current stage Vertex identity/XYZ recomputation |
| V03 | PARTIAL — tested assertions PASS | Exact status histogram and semantic-vs-native Surface adjacency required |
| V04 | UNRUN | Real Part Revolve circular rim, seam and supported/unsupported Vertex distinctions |
| V05 | PARTIAL | Publish actual Fillet Vertex status counts, relation cardinality and cases |
| V06 | PARTIAL | Publish actual Chamfer Vertex status counts, relation cardinality and cases |
| V07 | UNRUN | Missing removed-source identity and fail-closed status |
| V08 | UNRUN | Ambiguous one-to-many semantics fail closed |
| V09 | UNRUN | Pre-edge-feature stage vs final stage identity separation |
| V10 | **NOT DEMONSTRATED** | Concrete native ordinary-pickable but semantically Unsupported Vertex |
| V11 | UNRUN | Stale token/session/generation and cold source lifecycle |
| V12 | UNRUN | Same XYZ but wrong-provenance replacement remains Missing |

`MID`/`CENTER` E01–E08 are **not in this activated G0 execution scope**, except as future test planning. All E01–E08 remain UNRUN.

## 5. Immediate next G0 evidence steps

- Complete V03 native/semantic Surface-adjacency cross-check and publish the observed Cut status histogram without inferring values from successful default CTest output.
- Native **V04 Revolve** stage classifications.
- Obtain V05/V06 status distributions with a legitimate test output capture; not by synthetic B-Rep catalogs or asserted guesses.
- Complete focused lifecycle V02/V07–V12 as supported by bounded source paths; STOP for Owner amendment if a required fixture cannot be built without changing product code/CMake or creating a new test source.
- Final exact-head Windows FULL, Owner G0 evidence acceptance and separate production evidence merge approval only at genuine completion. No claims of full RG-02A or RG-02 implementation.

## 6. Independent UI STOP and documentation impact

**No RG-02 UI work.** Prior to any Body 3D `VERTEX/MID/CENTER` marker/pick implementation or Datum/Axis tool interaction, submit UX design for separate explicit Owner approval, regardless of these native findings.

Internal docs: not required  
User/Product docs: not required  
Reason: evidence-only test execution and results, with no user/product semantics changed. As-built docs and generated Browser update remain obligations of separately authorized future feature implementation.
