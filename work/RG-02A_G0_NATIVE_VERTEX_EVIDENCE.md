# RG-02A/G0 — Native VERTEX evidence report (in progress)

**Status:** PARTIAL / G0 NOT CLOSED; no Owner D2 synthesis yet, no FINAL FULL PASS claimed  
**Active authority:** `work/RG-02A_G0_VERTEX_CHARACTERIZATION_WORK_CONTRACT_DRAFT.md`, activated by [PR #331](https://github.com/MechanicalCave/SimpleSolid2/pull/331) (`351840b95b58bdbe7b17085f67b3ddd2cc2cf8c7`) under Part-v1 roadmap v1.32. The historical "NOT MERGED / NOT ACTIVATED" wording in the original G0 Work Contract heading predates these separately approved governance merges; this report does not amend its substantive contract scope.  
**Test sources:** `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` (V01/V02/V03/V05/V06/V07/V09/V11/V12) and `tests/kernel_native/pm04c2_production_revolve_test.cpp` (V04/V08/V10), both existing native tests explicitly allowed by the G0 contract.  
**Evidence source SHA:** `37fed9e523e5dee4113f1d7127f1962ff7a018ca`, Draft [PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332), [Windows FOCUSED run #38304772610](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38304772610), test `pm05c2b.part_edge_topology`.  
**Validation:** V01 source SHA `37fed9e523e5dee4113f1d7127f1962ff7a018ca` — Windows FOCUSED 1/1 PASS; V03 Add→Cut source SHA `5397db379690949f51a8daf7c831e85a8149a32e` — [Windows FOCUSED #38306302187](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306302187) 1/1 PASS and aggregate PASS; **V04 partial/full Revolve source SHA `5052f998533a220242eb0eaece781a0fa58aba24` — [Windows FOCUSED #38306630003](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306630003) 2/2 PASS**. **V02/V09 source SHA `691d0f1379b289a1824430e7fdf1b42d5ab9f04d` — [Windows FOCUSED #38307244650](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307244650) 2/2 PASS; V07 source SHA `dfdcc7e276544cda6cc3a8f92eca881ab641002c` — [Windows FOCUSED #38307452459](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307452459) 2/2 PASS, aggregate `windows-msvc` PASS.** **V12 source SHA `958bf75561239b92b1905a71f4ec652e6a5d44a3` — [Windows FOCUSED #38307830593](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307830593) 2/2 PASS and aggregate `windows-msvc` PASS.** **V10 source SHA `bfd1d054d0a8e0c70280115749a7a3495948239b` — [Windows FOCUSED #38308507132](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308507132) 2/2 PASS; V11 source SHA `c256583b984554880e4f50f49b3dae251dd7dbc6` — [Windows FOCUSED #38308647893](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308647893) 2/2 PASS; V08 source SHA `5154227c9d1706fe93dbc91e6356031bb4cd8bd7` — [Windows FOCUSED #38308817803](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308817803) 2/2 PASS**, each with aggregate `windows-msvc` PASS. Each claim applies to the indicated exact code SHA; this subsequent report commit still requires its own head checks. FAST and FULL are not yet claimed PASS.

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

## 3. V04 — partial and full Revolve native/Part classification (partial)

**V04: PASS for invariant assertions; detailed classification table pending.** Existing real OCCT native test `pm04c2.production_revolve` evaluates **90° partial** and **360° full** Revolve both at provider-runtime geometry and as real Part-owned features on an `Origin X` axis. Test code added at `5052f998533a220242eb0eaece781a0fa58aba24`, [Windows FOCUSED run #38306630003](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306630003) PASS 2/2 (Revolve + preceding Part Edge test).

- Every actual provider-reported Vertex is included in a runtime inventory grouped by native adjacent-Surface cardinality (2 / 3 / other); no provider-ordinal/XYZ authority is persisted.
- Both Part stage catalogs are complete. Every actual Resolved Part Vertex has **exactly one** valid three-Surface `FeaturePointAddress` witness, and that witness resolves to **exactly one current native Vertex**.
- Missing / Ambiguous / Unsupported entries are accounted separately, never treated as falsely Resolved. The existing full-turn test also confirms that representation seam Edges are marked as nonauthorable artifacts, but a seam Edge alone is **not** proof of which Vertex classifications occur.

**Open V04 evidence:** exact native 2/3/other Surface cardinality histograms and Part `Resolved/Missing/Ambiguous/Unsupported` counts are printed by the test executable, but are not present in the default passing CTest job transcript. Do not claim their numerical values or ordinary viewport pickability, or treat every Revolve Vertex as a supported persistent point.

## 4. V02, V07 and V09 — native history identity, removal and stage separation

**V02: PASS for the tested upstream-edit fixture.** The same authored rectangular Extrude feature is reevaluated at 35 mm instead of 20 mm; the Sketch, Profile, Feature ID, and `BodyStageRef` identity are retained, without reauthoring source points. All eight original `FeaturePointAddress` values match eight Resolved current Vertex points in the new evaluation. Only *after matching by semantic address*, provider XYZ is examined as a diagnostic: **4 associated points moved by +15 mm Z and 4 remained at unchanged Z**. The test does **not** infer semantic identity from XYZ. Exact source SHA `691d0f1379b289a1824430e7fdf1b42d5ab9f04d`, [FOCUSED #38307244650](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307244650) PASS 2/2.

**V07: PASS for the tested removed-corner fixture.** An actual full-height 20 mm Cut removes the corner of the previous 40×30×20 mm Extrude. Both before-Cut and after-Cut stage catalogs are valid. At least one original Resolved semantic Point address remains listed at the later stage as **`ReferenceStatus::missing`**, with zero candidate native Vertices and no cached replacement; at least one other original address stays Resolved. The removed address is **not** reattached by nearest geometry. The test does not yet prove every possible broken-Vertex case. Exact source SHA `dfdcc7e276544cda6cc3a8f92eca881ab641002c`, [FOCUSED #38307452459](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307452459) PASS 2/2; aggregate `windows-msvc` PASS.

**V09: PASS for stage catalog separation.** In a real `Extrude → Chamfer` Feature history, the earlier and after-Chamfer catalogs have **different `BodyStageRef`** values. Every one of the original eight Resolved three-Surface Point addresses is still recognized in the **earlier stage's own catalog**, unaffected by the later stage's new/removed material boundary. This is not proof that a new persisted Datum point source has already been implemented. Source SHA `691d0f1379b289a1824430e7fdf1b42d5ab9f04d`, [FOCUSED #38307244650](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307244650) PASS 2/2.

**Limits:** The default successful CTest output suppresses printed status count histograms; no unavailable per-stage counts are invented. G0 is not proof of end-user 3D picking or of a new persistent RG-02 point-source schema.

## 5. V12 — same native geometry, different source Feature provenance

**V12: PASS for the tested replacement-producer fixture.** In the same Part document, recreate the exact same 40×30×20 mm prism using the same Profile, Sketch, dimensions and support, but assign a **new FeatureId** to the only Body Feature. After evaluating both versions using native OCCT and actual Part topology catalogs, the test checks all eight original Vertex positions have an equal-XYZ counterpart in the replacement solid. Nevertheless, **none of their old `FeaturePointAddress` values is present in the replacement catalog**, and the two `BodyStageRef` values differ. This comparison uses XYZ solely to demonstrate geometric equality, **not** as an identity-resolution input.

This proves an *existing* semantic Point reference cannot silently transfer to a recreated Feature merely because a visually indistinguishable Vertex happens to occupy the same coordinates. The test does **not** assert a persisted RG-02 construction-point reference or its typed runtime diagnostic (those are future implementation scope). Exact source SHA `958bf75561239b92b1905a71f4ec652e6a5d44a3`, [Windows FOCUSED #38307830593](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307830593) PASS 2/2 and aggregate `windows-msvc` PASS.

## 6. V08 — real analytic-cylinder ambiguity (positive native case)

**V08: PASS for a real ambiguous semantic Point.** A genuinely circular Extrude-cylinder is cut with a rectangular extruded tool through its axis, producing a half-cylinder with multiple rim Vertices on the same analytic cylinder/cap/cut-plane Surface triples. `part::evaluatePart()` on a real OCCT model produces at least **one `FeaturePointResolution` with `ReferenceStatus::ambiguous` and more than one current native Vertex**. Every such observed ambiguous Point has a valid three-Surface `FeaturePointAddress`, candidate count greater than one, and more than one current realization. There is at least one current native Vertex whose referenceability is `Ambiguous`. No first/nearest point is promoted to `Resolved` by the test. Exact SHA `5154227c9d1706fe93dbc91e6356031bb4cd8bd7`, [FOCUSED run #38308817803](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308817803) PASS 2/2.

**Architectural consequence:** a unique three-Surface set does not always identify a *single* bounded Vertex. Any future D2 generalization of VERTEX must preserve explicit ambiguity and require evidence-backed bounded discrimination. No such schema change is made by G0.

## 7. V10 — concrete ordinary-policy-eligible but unsupported native Vertex

**V10: PASS for a concrete ordinary-pick eligibility mismatch; actual UI interaction not executed.** A real analytic cylindrical solid created by extruding a circular Sketch has **one or more native Vertices with `ReferenceStatus::unsupported`**, `TopologyAccountingClass::semantically_unsupported`, and **zero point candidates**. Existing `PartViewportController::bodyTopologyOrdinaryPickable()` only excludes `integrity_failure` and `known_representation_artifact` for native Vertices (when the normal scene/generation guards admit a current selection); it does **not** check `referenceability == resolved`. These tested native records are therefore eligible under the existing ordinary Vertex category policy, while not durably authorable as `FeaturePointAddress`. Exact SHA `bfd1d054d0a8e0c70280115749a7a3495948239b`, [FOCUSED run #38308507132](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308507132) PASS 2/2.

**Boundary:** this is not a headless test executing `PartViewportController` picking or verifying visibility in a particular camera/viewer scene. No RG-02 marker or selection UI has been implemented. The status count is greater than zero, but no exact total is inferred from hidden CTest stdout.

## 8. V11 — fresh Part reconstruction in a separate OCCT provider (partial)

**V11: PASS for semantic cold in-memory reconstruction; stale-session token rejection remains unproven.** The same authored `PartDocument` state is restored and evaluated with an **independently instantiated `OcctSolidModelingKernel`**. All eight referenceable prism Vertices have a one-to-one Resolved `FeaturePointAddress` with the **same addresses** in the new Part-stage catalog. The current implementation's `RuntimeVertexToken` is a transient numeric token; the test makes **no equality assumption** across independent evaluations and does not persist or author numeric token identity. Exact SHA `c256583b984554880e4f50f49b3dae251dd7dbc6`, [FOCUSED run #38308647893](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308647893) PASS 2/2.

**Not proven:** stale old-session Vertex *selection* token rejection by a full UI/generation guard, cold file-based save/reopen of a future RG-02 point-source scheme, or a new typed source author's lifecycle (none exists in this G0 evidence-only scope). Explicitly preserve this as an open acceptance boundary, not full V11 closure.

## 9. V05/V06 partial execution — classification witness checks, exact distribution pending

The same passing FOCUSED test executes `characterizeNativeVertices` on real, successful Part Fillet and Chamfer stages (`V05-fillet`, `V06-chamfer`). At each stage it asserts a complete Part topology catalog, accounted Vertex statuses, and that **any resolved Vertex** has a one-to-one three-Surface `FeaturePointResolution` witness. These invariant checks passed.

**V05 and V06 status: PARTIAL / classification totals not yet published**. The helper prints `native/resolved/ambiguous/missing/unsupported` counters on stdout, but the default successful CTest transcript only shows 1/1 PASS and **does not expose those counter lines** in the fetched GitHub Actions job log. We therefore do **not** claim observed totals, a demonstrated non-three-Surface Vertex, or full V05/V06 characterization until the actual counters are captured with a suitable authorized test execution. Do not infer nonzero Unsupported from the source code alone.

## 10. Remaining VERTEX matrix (not executed or not demonstrated)

| Case | Evidence status | Outstanding proof |
| --- | --- | --- |
| V01 | **PASS (measured scope)** | Future lifecycle proof is V02/V11, not part of V01 |
| V02 | **PASS for tested upstream height edit** | 8/8 semantic addresses preserved, 4 moved +15 mm and 4 stationary; no new persistent point-source lifecycle |
| V03 | PARTIAL — tested assertions PASS | Exact status histogram and semantic-vs-native Surface adjacency required |
| V04 | PARTIAL — native/Part invariant assertions PASS | Exact native 2/3/other and Part status histograms, seam-vs-Vertex/pickability distinction required |
| V05 | PARTIAL | Publish actual Fillet Vertex status counts, relation cardinality and cases |
| V06 | PARTIAL | Publish actual Chamfer Vertex status counts, relation cardinality and cases |
| V07 | **PASS for tested full-height removed-corner Cut** | At least one original Point becomes Missing with no current native Vertex; broader deletion variants not covered |
| V08 | **PASS for real half-cylinder analytic Cut** | At least one three-Surface Point resolves to 2+ native Vertices, status Ambiguous; exact histogram outstanding |
| V09 | **PASS for tested Extrude→Chamfer stage separation** | Earlier stage has all 8 original Point identities; Datum/Axis source persistence not implemented |
| V10 | **PASS for native Unsupported and ordinary eligibility policy** | Real extruded cylinder has 1+ Unsupported native Vertices admitted by ordinary account-class policy; interactive picking not tested |
| V11 | PARTIAL — independent OCCT rebuild PASS | All 8 semantic Points reconstruct; stale runtime selection token rejection and new RG-02 file persistence not proved |
| V12 | **PASS for tested same-XYZ replacement Feature** | Old semantic Point addresses absent despite identical geometry; no persisted RG-02 source/detailed typed diagnostic tested |

`MID`/`CENTER` E01–E08 are **not in this activated G0 execution scope**, except as future test planning. All E01–E08 remain UNRUN.

## 11. Immediate next G0 evidence steps

- Complete V03 native/semantic Surface-adjacency cross-check and publish the observed Cut status histogram without inferring values from successful default CTest output.
- Complete the V04 provider native adjacency and Part status distribution from real execution evidence; do not infer a pickable Vertex from an OCCT seam Edge.
- Obtain V05/V06 status distributions with a legitimate test output capture; not by synthetic B-Rep catalogs or asserted guesses.
- Complete per-fixture exact native/semantic status histograms for V03–V06 and V08/V10; distinguish a proven one-to-many `Ambiguous` relation from native Unsupported. Assess whether stale-selection generation rejection can be characterized without UI or production changes (V11 partial only). The tested V02/V07/V08/V09/V10/V11/V12 fixtures do not prove universal Part Vertex coverage. STOP for Owner amendment if any required fixture would require changing code outside the three approved native tests.
- Final exact-head Windows FULL, Owner G0 evidence acceptance and separate production evidence merge approval only at genuine completion. No claims of full RG-02A or RG-02 implementation.

## 12. Independent UI STOP and documentation impact

**No RG-02 UI work.** Prior to any Body 3D `VERTEX/MID/CENTER` marker/pick implementation or Datum/Axis tool interaction, submit UX design for separate explicit Owner approval, regardless of these native findings.

Internal docs: not required  
User/Product docs: not required  
Reason: evidence-only test execution and results, with no user/product semantics changed. As-built docs and generated Browser update remain obligations of separately authorized future feature implementation.
