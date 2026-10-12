# RG-02A/G0 — Native VERTEX evidence report (in progress)

**Status:** PARTIAL / G0 NOT CLOSED; no Owner D2 synthesis yet, no FINAL FULL PASS claimed  
**Active authority:** `work/RG-02A_G0_VERTEX_CHARACTERIZATION_WORK_CONTRACT_DRAFT.md`, activated by [PR #331](https://github.com/MechanicalCave/SimpleSolid2/pull/331) (`351840b95b58bdbe7b17085f67b3ddd2cc2cf8c7`) under Part-v1 roadmap v1.32. The historical "NOT MERGED / NOT ACTIVATED" wording in the original G0 Work Contract heading predates these separately approved governance merges; this report does not amend its substantive contract scope.  
**Test source:** `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` (already registered and explicitly allowed).  
**Evidence source SHA:** `37fed9e523e5dee4113f1d7127f1962ff7a018ca`, Draft [PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332), [Windows FOCUSED run #38304772610](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38304772610), test `pm05c2b.part_edge_topology`.  
**Validation:** Windows FOCUSED 1/1 PASS, aggregate `windows-msvc` PASS at the above exact SHA; FAST and FULL were skipped and **are not claimed PASS**.

## 1. Proven V01 result — real OCCT and actual Part-stage topology

Native fixture: rectangular Sketch/Profile, Part Extrude Add 40×30×20 mm, evaluated by production `part::evaluatePart()` using production `kernel_occt::OcctSolidModelingKernel` at the specific after-Extrude Body stage.

**V01: PASS within the measured scope.** The native test asserts and passed:

- Exactly **8 current Body Vertex topology records**, **8 with `ReferenceStatus::resolved`**.
- Each Resolved Vertex has exactly one valid current `FeaturePointAddress` candidate, with **exactly three semantic Surface addresses**.
- Each such Point address is present in the **real current Body-stage `points` catalog** as `ReferenceStatus::resolved` with **exactly one current native Vertex token**, and that token is the same one as the examined native Vertex.
- Zero Ambiguous / Unsupported current native Vertices for this canonical fixture. No inferred XYZ-based identity, nearest-Vertex match, screen-space selection or cached prior-Body geometry.

**What this does not prove:** persisted Datum/Axis construction-point authoring, Save/Reopen/cold rebuild of a *new* point-source schema, or end-user viewport pickability. These features do not exist in G0 and have not been tested.

## 2. V05/V06 partial execution — classification witness checks, exact distribution pending

The same passing FOCUSED test executes `characterizeNativeVertices` on real, successful Part Fillet and Chamfer stages (`V05-fillet`, `V06-chamfer`). At each stage it asserts a complete Part topology catalog, accounted Vertex statuses, and that **any resolved Vertex** has a one-to-one three-Surface `FeaturePointResolution` witness. These invariant checks passed.

**V05 and V06 status: PARTIAL / classification totals not yet published**. The helper prints `native/resolved/ambiguous/missing/unsupported` counters on stdout, but the default successful CTest transcript only shows 1/1 PASS and **does not expose those counter lines** in the fetched GitHub Actions job log. We therefore do **not** claim observed totals, a demonstrated non-three-Surface Vertex, or full V05/V06 characterization until the actual counters are captured with a suitable authorized test execution. Do not infer nonzero Unsupported from the source code alone.

## 3. Remaining VERTEX matrix (not executed or not demonstrated)

| Case | Evidence status | Outstanding proof |
| --- | --- | --- |
| V01 | **PASS (measured scope)** | Future lifecycle proof is V02/V11, not part of V01 |
| V02 | UNRUN | Upstream edit, current stage Vertex identity/XYZ recomputation |
| V03 | UNRUN | Real Part Cut generated/inherited native Vertex classification and Surface cardinality |
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

## 4. Immediate next G0 evidence steps

- Native **V03 Cut** and provider topology cross-check using only already-authorized test sources, separating raw native adjacency from Part semantic referenceability.
- Native **V04 Revolve** stage classifications.
- Obtain V05/V06 status distributions with a legitimate test output capture; not by synthetic B-Rep catalogs or asserted guesses.
- Complete focused lifecycle V02/V07–V12 as supported by bounded source paths; STOP for Owner amendment if a required fixture cannot be built without changing product code/CMake or creating a new test source.
- Final exact-head Windows FULL, Owner G0 evidence acceptance and separate production evidence merge approval only at genuine completion. No claims of full RG-02A or RG-02 implementation.

## 5. Independent UI STOP and documentation impact

**No RG-02 UI work.** Prior to any Body 3D `VERTEX/MID/CENTER` marker/pick implementation or Datum/Axis tool interaction, submit UX design for separate explicit Owner approval, regardless of these native findings.

Internal docs: not required  
User/Product docs: not required  
Reason: evidence-only test execution and results, with no user/product semantics changed. As-built docs and generated Browser update remain obligations of separately authorized future feature implementation.
