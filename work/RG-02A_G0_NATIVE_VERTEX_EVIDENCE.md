# RG-02A/G0 — Native VERTEX evidence report (in progress)

**Status:** PARTIAL / G0 NOT CLOSED; no Owner D2 synthesis yet, no FINAL FULL PASS claimed  
**Active authority:** `work/RG-02A_G0_VERTEX_CHARACTERIZATION_WORK_CONTRACT_DRAFT.md`, activated by [PR #331](https://github.com/MechanicalCave/SimpleSolid2/pull/331) (`351840b95b58bdbe7b17085f67b3ddd2cc2cf8c7`) under Part-v1 roadmap v1.32. The historical "NOT MERGED / NOT ACTIVATED" wording in the original G0 Work Contract heading predates these separately approved governance merges; this report does not amend its substantive contract scope.  
**Test sources:** `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` (V01/V02/V03/V05/V06/V07/V09/V11/V12) and `tests/kernel_native/pm04c2_production_revolve_test.cpp` (V04/V08/V10), both existing native tests explicitly allowed by the G0 contract.  
**Evidence source SHA:** `37fed9e523e5dee4113f1d7127f1962ff7a018ca`, Draft [PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332), [Windows FOCUSED run #38304772610](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38304772610), test `pm05c2b.part_edge_topology`.  
**Validation:** V01 source SHA `37fed9e523e5dee4113f1d7127f1962ff7a018ca` — Windows FOCUSED 1/1 PASS; V03 Add→Cut source SHA `5397db379690949f51a8daf7c831e85a8149a32e` — [Windows FOCUSED #38306302187](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306302187) 1/1 PASS and aggregate PASS; **V04 partial/full Revolve source SHA `5052f998533a220242eb0eaece781a0fa58aba24` — [Windows FOCUSED #38306630003](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306630003) 2/2 PASS**. **V02/V09 source SHA `691d0f1379b289a1824430e7fdf1b42d5ab9f04d` — [Windows FOCUSED #38307244650](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307244650) 2/2 PASS; V07 source SHA `dfdcc7e276544cda6cc3a8f92eca881ab641002c` — [Windows FOCUSED #38307452459](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307452459) 2/2 PASS, aggregate `windows-msvc` PASS.** **V12 source SHA `958bf75561239b92b1905a71f4ec652e6a5d44a3` — [Windows FOCUSED #38307830593](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307830593) 2/2 PASS and aggregate `windows-msvc` PASS.** **V10 source SHA `bfd1d054d0a8e0c70280115749a7a3495948239b` — [Windows FOCUSED #38308507132](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308507132) 2/2 PASS; V11 source SHA `c256583b984554880e4f50f49b3dae251dd7dbc6` — [Windows FOCUSED #38308647893](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308647893) 2/2 PASS; V08 source SHA `5154227c9d1706fe93dbc91e6356031bb4cd8bd7` — [Windows FOCUSED #38308817803](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308817803) 2/2 PASS**, each with aggregate `windows-msvc` PASS. **Histogram diagnostic RED:** [run #38309695097](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38309695097), source SHA `6c0ab31e9e51d266e00812f6bae0849cd524fc4f` — deliberately forced CTest failure *after all substantive checks* to reveal success-suppressed fixture counters. **Numeric assertion GREEN:** [run #38310021876](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38310021876), source SHA `98107ed3bada2c121ea2055bf8d090830dbf48af` — Windows FOCUSED **2/2 PASS**, `windows-msvc` aggregate PASS. Diagnostic failures were removed and replaced by exact measured-value assertions in both permitted native tests. Both SHAs are separate from this later report edit; FAST and FULL are not yet claimed PASS.

## 1. Proven V01 result — real OCCT and actual Part-stage topology

Native fixture: rectangular Sketch/Profile, Part Extrude Add 40×30×20 mm, evaluated by production `part::evaluatePart()` using production `kernel_occt::OcctSolidModelingKernel` at the specific after-Extrude Body stage.

**V01: PASS within the measured scope.** The native test asserts and passed:

- Exactly **8 current Body Vertex topology records**, **8 with `ReferenceStatus::resolved`**.
- Each Resolved Vertex has exactly one valid current `FeaturePointAddress` candidate, with **exactly three semantic Surface addresses**.
- Each such Point address is present in the **real current Body-stage `points` catalog** as `ReferenceStatus::resolved` with **exactly one current native Vertex token**, and that token is the same one as the examined native Vertex.
- Zero Ambiguous / Unsupported current native Vertices for this canonical fixture. No inferred XYZ-based identity, nearest-Vertex match, screen-space selection or cached prior-Body geometry.

**What this does not prove:** persisted Datum/Axis construction-point authoring, Save/Reopen/cold rebuild of a *new* point-source schema, or end-user viewport pickability. These features do not exist in G0 and have not been tested.

## 2. V03 — real Part Extrude Add → Cut semantic continuation (partial)

**V03: PASS for actual Part-stage Vertex status histogram and semantic inheritance; raw provider adjacency still requires separate evidence.** A new Part Sketch/Profile on Origin XY defines an interior 20×20 mm rectangle; a second Part Feature performs a 10 mm one-sided Cut into the existing 40×30×20 mm Extrude prism. Unlike a fabricated semantic catalog, production `part::evaluatePart()` evaluates **both actual stages using OCCT**. [Windows FOCUSED run #38306302187](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306302187) on exact source SHA `5397db379690949f51a8daf7c831e85a8149a32e` PASS (1/1).

Verified assertions:
- The actual Cut Body is `up_to_date`, with a valid two-Feature history and complete stage-specific topology.
- The resulting Cut and prior Extrude stages have different semantic `BodyStageRef` identities.
- Cut produces **more current native Vertices than the original eight**.
- **At least eight** Resolved current Cut Vertices retain exactly the semantic `FeaturePointAddress` identities present after original Extrude. **At least one** additional current Resolved Vertex has a new, different `FeaturePointAddress`.
- Every current Cut Vertex marked Resolved has exactly one valid three-Surface semantic Point witness with one-to-one same-stage runtime realization (as checked by the existing G0 characterization helper). No nearest-XYZ/provider-token authoring or reattachment.

**Measured V03:** 16 current Cut-stage native Vertices: **16 Resolved, 0 Ambiguous, 0 Missing, 0 Unsupported**, with 16 unique three-Surface Point witnesses. **Exactly 8 inherited** earlier semantic Point addresses and **8 new** resolved Point addresses. The separate full-height removed-corner V07 Cut has **12 current Resolved Vertices**, but **2 older Point addresses become Missing**, with **6 old addresses retained**; this is not a contradiction—current Vertex records and lineage Point records are distinct inventories. Outstanding V03 provider Face/native Surface incidence cross-check is not claimed by the Part-stage inventory.

## 3. V04 — partial and full Revolve measured native/Part classification

**V04: PASS for the measured provider adjacency and Part-stage histogram assertions; interactive pick UX remains outside G0.** Existing real OCCT native test `pm04c2.production_revolve` evaluates **90° partial** and **360° full** Revolve both at provider-runtime geometry and as real Part-owned features on an `Origin X` axis. Test code added at `5052f998533a220242eb0eaece781a0fa58aba24`, [Windows FOCUSED run #38306630003](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38306630003) PASS 2/2 (Revolve + preceding Part Edge test).

- Every actual provider-reported Vertex is included in a runtime inventory grouped by native adjacent-Surface cardinality (2 / 3 / other); no provider-ordinal/XYZ authority is persisted.
- Both Part stage catalogs are complete. Every actual Resolved Part Vertex has **exactly one** valid three-Surface `FeaturePointAddress` witness, and that witness resolves to **exactly one current native Vertex**.
- Missing / Ambiguous / Unsupported entries are accounted separately, never treated as falsely Resolved. The existing full-turn test also confirms that representation seam Edges are marked as nonauthorable artifacts, but a seam Edge alone is **not** proof of which Vertex classifications occur.

**Measured V04:** partial Revolve 90° provider reports **8 Vertices, all with 3 native adjacent Surfaces**; the resulting Part stage has **6 native Vertices, 6 Resolved**. Full Revolve 360° provider reports **4 Vertices, all with 2 native adjacent Surfaces**; the Part stage has **2 native Vertices, both Unsupported**. Different native provider and Part-stage counts reflect their respective inventories; do not equate seam artifacts or raw provider cardinality with durable Part Point identity. These numbers are backed by measured assertions, not by screen-space pick behavior.

## 4. V02, V07 and V09 — native history identity, removal and stage separation

**V02: PASS for the tested upstream-edit fixture.** The same authored rectangular Extrude feature is reevaluated at 35 mm instead of 20 mm; the Sketch, Profile, Feature ID, and `BodyStageRef` identity are retained, without reauthoring source points. All eight original `FeaturePointAddress` values match eight Resolved current Vertex points in the new evaluation. Only *after matching by semantic address*, provider XYZ is examined as a diagnostic: **4 associated points moved by +15 mm Z and 4 remained at unchanged Z**. The test does **not** infer semantic identity from XYZ. Exact source SHA `691d0f1379b289a1824430e7fdf1b42d5ab9f04d`, [FOCUSED #38307244650](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307244650) PASS 2/2.

**V07: PASS for the tested removed-corner fixture.** An actual full-height 20 mm Cut removes the corner of the previous 40×30×20 mm Extrude. Both before-Cut and after-Cut stage catalogs are valid. Exactly **2 original Resolved semantic Point addresses** remain listed at the later stage as **`ReferenceStatus::missing`**, with zero candidate native Vertices and no cached replacement; **6 other original addresses stay Resolved**. The removed address is **not** reattached by nearest geometry. The test does not yet prove every possible broken-Vertex case. Exact source SHA `dfdcc7e276544cda6cc3a8f92eca881ab641002c`, [FOCUSED #38307452459](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307452459) PASS 2/2; aggregate `windows-msvc` PASS.

**V09: PASS for stage catalog separation.** In a real `Extrude → Chamfer` Feature history, the earlier and after-Chamfer catalogs have **different `BodyStageRef`** values. Every one of the original eight Resolved three-Surface Point addresses is still recognized in the **earlier stage's own catalog**, unaffected by the later stage's new/removed material boundary. This is not proof that a new persisted Datum point source has already been implemented. Source SHA `691d0f1379b289a1824430e7fdf1b42d5ab9f04d`, [FOCUSED #38307244650](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307244650) PASS 2/2.

**Limits:** The controlled diagnostic RED published the real per-stage counters, and following GREEN fixed them as exact assertions. No inference about native Surface cardinality for every unsupported Fillet/Chamfer Vertex is claimed solely from this Part-level status inventory. G0 does not prove end-user 3D picking or any future persisted RG-02 point-source schema.

## 5. V12 — same native geometry, different source Feature provenance

**V12: PASS for the tested replacement-producer fixture.** In the same Part document, recreate the exact same 40×30×20 mm prism using the same Profile, Sketch, dimensions and support, but assign a **new FeatureId** to the only Body Feature. After evaluating both versions using native OCCT and actual Part topology catalogs, the test checks all eight original Vertex positions have an equal-XYZ counterpart in the replacement solid. Nevertheless, **none of their old `FeaturePointAddress` values is present in the replacement catalog**, and the two `BodyStageRef` values differ. This comparison uses XYZ solely to demonstrate geometric equality, **not** as an identity-resolution input.

This proves an *existing* semantic Point reference cannot silently transfer to a recreated Feature merely because a visually indistinguishable Vertex happens to occupy the same coordinates. The test does **not** assert a persisted RG-02 construction-point reference or its typed runtime diagnostic (those are future implementation scope). Exact source SHA `958bf75561239b92b1905a71f4ec652e6a5d44a3`, [Windows FOCUSED #38307830593](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38307830593) PASS 2/2 and aggregate `windows-msvc` PASS.

## 6. V08 — real analytic-cylinder ambiguity (positive native case)

**V08: PASS for a real ambiguous semantic Point.** A genuinely circular Extrude-cylinder is cut with a rectangular extruded tool through its axis, producing a half-cylinder with multiple rim Vertices on the same analytic cylinder/cap/cut-plane Surface triples. `part::evaluatePart()` on a real OCCT model produces at least **one `FeaturePointResolution` with `ReferenceStatus::ambiguous` and more than one current native Vertex**. Every such observed ambiguous Point has a valid three-Surface `FeaturePointAddress`, candidate count greater than one, and more than one current realization. **Exactly 2 distinct Ambiguous three-Surface Point groups cover 4 current native Vertices**, while **2 additional current native Vertices are Unsupported** (6 native Vertices total, none Resolved or Missing). No first/nearest point is promoted to `Resolved` by the test. Exact SHA `5154227c9d1706fe93dbc91e6356031bb4cd8bd7`, [FOCUSED run #38308817803](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308817803) PASS 2/2.

**Architectural consequence:** a unique three-Surface set does not always identify a *single* bounded Vertex. Any future D2 generalization of VERTEX must preserve explicit ambiguity and require evidence-backed bounded discrimination. No such schema change is made by G0.

## 7. V10 — concrete ordinary-policy-eligible but unsupported native Vertex

**V10: PASS for a concrete ordinary-pick eligibility mismatch; actual UI interaction not executed.** A real analytic cylindrical solid created by extruding a circular Sketch has **one or more native Vertices with `ReferenceStatus::unsupported`**, `TopologyAccountingClass::semantically_unsupported`, and **zero point candidates**. Existing `PartViewportController::bodyTopologyOrdinaryPickable()` only excludes `integrity_failure` and `known_representation_artifact` for native Vertices (when the normal scene/generation guards admit a current selection); it does **not** check `referenceability == resolved`. These tested native records are therefore eligible under the existing ordinary Vertex category policy, while not durably authorable as `FeaturePointAddress`. Exact SHA `bfd1d054d0a8e0c70280115749a7a3495948239b`, [FOCUSED run #38308507132](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308507132) PASS 2/2.

**Boundary:** this is not a headless test executing `PartViewportController` picking or verifying visibility in a particular camera/viewer scene. No RG-02 marker or selection UI has been implemented. The measured number is **2 native Unsupported Vertices**, both matching the existing ordinary accounting-class pick-eligibility policy; no actual interactive hit/pick is claimed.

## 8. V11 — fresh Part reconstruction in a separate OCCT provider (partial)

**V11: PASS for semantic cold in-memory reconstruction; stale-session token rejection remains unproven.** The same authored `PartDocument` state is restored and evaluated with an **independently instantiated `OcctSolidModelingKernel`**. All eight referenceable prism Vertices have a one-to-one Resolved `FeaturePointAddress` with the **same addresses** in the new Part-stage catalog. The current implementation's `RuntimeVertexToken` is a transient numeric token; the test makes **no equality assumption** across independent evaluations and does not persist or author numeric token identity. Exact SHA `c256583b984554880e4f50f49b3dae251dd7dbc6`, [FOCUSED run #38308647893](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38308647893) PASS 2/2.

**Not proven:** stale old-session Vertex *selection* token rejection by a full UI/generation guard, cold file-based save/reopen of a future RG-02 point-source scheme, or a new typed source author's lifecycle (none exists in this G0 evidence-only scope). Explicitly preserve this as an open acceptance boundary, not full V11 closure.

## 9. V05/V06 measured native Part status histograms

The same passing FOCUSED test executes `characterizeNativeVertices` on real, successful Part Fillet and Chamfer stages (`V05-fillet`, `V06-chamfer`). At each stage it asserts a complete Part topology catalog, accounted Vertex statuses, and that **any resolved Vertex** has a one-to-one three-Surface `FeaturePointResolution` witness. These invariant checks passed.

**V05 Fillet:** 13 current native Vertices = **10 Resolved, 3 Unsupported, 0 Ambiguous, 0 Missing**. **V06 Chamfer:** identical histogram, **13 = 10 Resolved + 3 Unsupported**. Every one of the 10 Resolved vertices in each has a unique current three-Surface Point witness. The three Unsupported native Vertex records have no currently resolvable semantic Point relation; the evidence does not yet establish whether that is caused by 2 / 4+ semantic Surface sources or an unresolved analytic provenance. Exact cardinality of their native or semantic Surface adjacency is **NOT YET MEASURED**. These are **real OCCT Fillet and Chamfer Feature results**, not fabricated topology fixtures.

## 9A. Exact measured status distributions, distinct inventories and native CI provenance

The following values were observed at **source SHA `6c0ab31e9e51d266e00812f6bae0849cd524fc4f`** using an intentional **diagnostic RED** to expose the stdout otherwise suppressed by successful CTest in [run #38309695097](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38309695097). Both native executables returned failure *after* substantive assertions solely for measurement. The artificial fail returns were subsequently removed, and the exact observed counts became **positive native assertions** at **source SHA `98107ed3bada2c121ea2055bf8d090830dbf48af`**, [GREEN FOCUSED run #38310021876](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38310021876): 2/2 native tests PASS; aggregate `windows-msvc` PASS.

| Actual Part-stage fixture | Native vertices | Resolved | Ambiguous | Missing (current vertex) | Unsupported | Unique Resolved Point witnesses |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| V01 rectangular Extrude | 8 | 8 | 0 | 0 | 0 | 8 |
| V02 edited Extrude, taller | 8 | 8 | 0 | 0 | 0 | 8 |
| V11 independent-kernel rebuild | 8 | 8 | 0 | 0 | 0 | 8 |
| V12 rebuilt geometry, new FeatureId | 8 | 8 | 0 | 0 | 0 | 8, **new producer identities** |
| V03 Extrude + interior Cut | 16 | 16 | 0 | 0 | 0 | 16 |
| V07 cut away a corner | 12 | 12 | 0 | 0 | 0 | 12 |
| V05 trihedral Fillet | 13 | 10 | 0 | 0 | 3 | 10 |
| V06 trihedral Chamfer | 13 | 10 | 0 | 0 | 3 | 10 |
| V04 Revolve 90° | 6 | 6 | 0 | 0 | 0 | 6 |
| V04 Revolve 360° | 2 | 0 | 0 | 0 | 2 | 0 |
| V10 Extrude cylinder | 2 | 0 | 0 | 0 | 2 | 0 |
| V08 half-cylinder Cut | 6 | 0 | 4 | 0 | 2 | 0 |

**Provider-level raw Surface adjacency (a different inventory):**

| Raw OCCT Revolve case | Provider Vertex tokens | 2 adjacent Surfaces | 3 adjacent Surfaces | Other count |
| --- | ---: | ---: | ---: | ---: |
| Partial 90° Revolve | 8 | 0 | 8 | 0 |
| Full 360° Revolve | 4 | 4 | 0 | 0 |

**Semantic lineage is not current-native Vertex count:** for V07 **2 old `FeaturePointResolution` entries are Missing (zero descendants), 6 old entries remain Resolved** even though all **12 current** Vertex records are Resolved. For V08, **2 distinct three-Surface Point addresses are Ambiguous**, represented by **4 Ambiguous current Vertices**. No nearest-XYZ choice, no cached prior-geometry fallback. Native adjacency counts and distinct *semantic* Surface addresses must never be conflated.

These bounded fixture outcomes establish the actual **non-universal three-Surface identity** of real `VERTEX`: native Unsupported at both Fillet/Chamfer corners and closed cylinders, and non-unique triple-Surface meaning on a cut analytic cylinder. The exact additional semantic discriminator (if any) is a **future Owner D2 decision**, not implemented or preapproved here.

## 10. Remaining VERTEX matrix (not executed or not demonstrated)

| Case | Evidence status | Outstanding proof |
| --- | --- | --- |
| V01 | **PASS (measured scope)** | Future lifecycle proof is V02/V11, not part of V01 |
| V02 | **PASS for tested upstream height edit** | 8/8 semantic addresses preserved, 4 moved +15 mm and 4 stationary; no new persistent point-source lifecycle |
| V03 | **PASS Part histogram 16/16 Resolved** | 8 inherited + 8 new semantic Points; raw provider Surface adjacency not separately measured |
| V04 | **PASS measured native/Part histograms** | 90°: provider 8 with 3 adjacent Surfaces, Part 6 Resolved; 360°: provider 4 with 2 adjacent Surfaces, Part 2 Unsupported; UI pick not tested |
| V05 | **PASS Part histogram** | Fillet 13 = 10 Resolved + 3 Unsupported; unsupported Vertex adjacent semantic Surface cardinality not established |
| V06 | **PASS Part histogram** | Chamfer 13 = 10 Resolved + 3 Unsupported; unsupported Vertex adjacent semantic Surface cardinality not established |
| V07 | **PASS for tested full-height removed-corner Cut** | At least one original Point becomes Missing with no current native Vertex; broader deletion variants not covered |
| V08 | **PASS measured half-cylinder Cut histogram** | 6 native = 4 Ambiguous + 2 Unsupported; exactly two Ambiguous three-Surface groups, each with 2+ native realizations |
| V09 | **PASS for tested Extrude→Chamfer stage separation** | Earlier stage has all 8 original Point identities; Datum/Axis source persistence not implemented |
| V10 | **PASS measured native Unsupported/policy** | Real cylinder has exactly 2 native Unsupported Vertices; ordinary eligible by accounting class, actual picking UI untested |
| V11 | PARTIAL — independent OCCT rebuild PASS | All 8 semantic Points reconstruct; stale runtime selection token rejection and new RG-02 file persistence not proved |
| V12 | **PASS for tested same-XYZ replacement Feature** | Old semantic Point addresses absent despite identical geometry; no persisted RG-02 source/detailed typed diagnostic tested |

`MID`/`CENTER` E01–E08 are **not in this activated G0 execution scope**, except as future test planning. All E01–E08 remain UNRUN.

## 11. Immediate next G0 evidence steps

- Any additional V03 provider raw Surface incidence characterization must be separately demonstrated with a contract-allowed native fixture; its **Part** 16/16 Resolved classification and 8+8 lineage are now measured.
- Preserve measured V04 provider/Part distinction: 90° provider 8 three-Surface Vertices vs Part 6 Resolved; 360° provider 4 two-Surface Vertices vs Part 2 Unsupported. Do not infer actual hit/pick behavior from seam-edge presence.
- Further characterize why the **3 Unsupported** native Vertices in each real Fillet/Chamfer fixture lack current semantic Point relations (native/semantic Surface adjacency or unresolved provenance) only through allowed test paths.
- Part-level and V04 native-provider status histograms are now evidenced. Remaining scope is diagnosing unsupported Fillet/Chamfer Surface cardinality, assessing the separately controlled stale-selection V11 generation evidence, and completing exact-head FAST/FULL. The tested finite native fixtures do not prove every possible Part Vertex. STOP for Owner amendment if a necessary test requires a fourth file, production code or UI implementation.
- Final exact-head Windows FULL, Owner G0 evidence acceptance and separate production evidence merge approval only at genuine completion. No claims of full RG-02A or RG-02 implementation.

## 12. Independent UI STOP and documentation impact

**No RG-02 UI work.** Prior to any Body 3D `VERTEX/MID/CENTER` marker/pick implementation or Datum/Axis tool interaction, submit UX design for separate explicit Owner approval, regardless of these native findings.

Internal docs: not required  
User/Product docs: not required  
Reason: evidence-only test execution and results, with no user/product semantics changed. As-built docs and generated Browser update remain obligations of separately authorized future feature implementation.
