# PM-05F R2 — Planar-miter Chamfer feasibility and lineage design

**Status:** D2-A FEASIBILITY APPROVED by Owner — 2026-10-08; D2-B production implementation NOT APPROVED  
**Decision class:** D2-A experiment authorized; separate Owner D2-B required before production geometry, history-adapter, or policy implementation  
**Parent proposal:** `work/PM-05F_R2_CHAMFER_MIXED_CORNER_D2_PROPOSAL.md`  
**Active authority:** `work/PM-05_EDGE_FEATURES.md`, PM-05F R2  
**PR:** #298 / `pm-05f-r2-owner-remediation`  
**Nature of this document:** design/evidence only; no code, modeling-semantics, persistence, or public API authorization

## Verified reproduction and limits of current evidence

The sanitized Part008 fixture has two authored Extrude Add Features. Equal-distance Chamfer at 1 mm on two selected strict material Edges succeeds, and the independently selected third Edge succeeds. The exact three-Edge semantic set (third: Extrude001 side Entity1 + side Entity4) produces one solid with `invalid_brep`. All six edge orders at 1 / 0.5 / 0.25 mm fail. BRepCheck finds two invalid Face/Wire boundaries and no invalid Edge/Vertex. Explicit-face Add variants that returned valid B-Reps were `no_effect`; symmetric `Add(E) -> SetDist(d, Contour(E), Face)` variants failed.

Owner-provided Inventor imagery is an **output-shape reference**, not algorithmic authority. It demonstrates a plausible piecewise-planar mitered junction but does not establish SS2's exact material offsets, edge participation, or semantic provenance.

The existing **successful two-Edge** Part008 Chamfer has two planar `edge_transition` carriers and one nonplanar `corner_transition` carrier. Changing that valid corner is **not** within the proposed first remediation. A global planar-only corner policy is a separate Owner D3 choice.

## Critical integration finding from actual OCCT adapter

In `src/kernel_occt/solid_modeling_kernel.cpp`:

1. `OcctSolidModelingKernel::edgeFeature` registers the strict runtime-selected Edges in one `BRepFilletAPI_MakeChamfer` operation.
2. `finishEdgeFeature` first checks input-contour membership, builds once, requires one valid changed solid, then publishes the result.
3. Publication is **history-dependent**. It uses operation descendant/Modified/Generated evidence for inherited Surface lineage, generated `edge_transition` ownership by exact source Edge, generated `corner_transition` ownership by source shared Vertex and incident authored Edges, and complete Face/Edge/Vertex accounting.
4. A stand-alone `TopoDS_Solid` produced by local sewing/Boolean construction cannot be substituted into this path without equivalent exact *provider-local provenance evidence*. Visual resemblance, geometric proximity, tessellation, and provider traversal indices cannot supply that evidence.

**Single point of failure:** finding/building a valid closed miter is insufficient if each inherited/generated Face cannot be accounted for with unique and defensible source lineage. A fallback must fail closed in that case.

## Owner-authorized D2-A feasibility sequence (no production implementation)

### F0 — Preserve precise semantic oracle

Keep the existing strict Part008 native test RED until a genuinely successful result is produced. Identify all three selected material Edges by existing stage-scoped `MaterialEdgeReference`, not runtime token or coordinates. Record their incident semantic Surfaces, common semantic Vertices, orientation, and exact kernel geometry. Preserve negative evidence from #1834–#1842.

### F1 — Classify geometric admissibility before constructing anything

A fallback candidate is admissible only if:

- the operation is **equal-distance Chamfer** and the complete explicit selection consists of supported straight material Edges;
- every contributing adjacent Face has an exact planar carrier, with uniquely defined orientations and local material-side classification;
- the selected network's connected corner(s) and trim bounds are unambiguous;
- the common authored Distance is strictly positive and the local geometry admits finite, nondegenerate intersections.

Generate offset/chamfer support planes from each selected Edge's **oriented pair of adjacent source planes**, using the same equal-distance meaning for each Edge. Derive junction candidates through analytic plane/line intersections; reject coincident, parallel, singular or out-of-domain intersections. No proximity search, first-face selection or implicit tolerance widening.

For the mixed concave/convex Part008 case, a simple single convex clipping half-space is **not assumed sufficient**. Any local solid reconstruction must respect both possible added and removed material. This remains a feasibility question, not a claimed algorithmic success.

### F2 — One coupled planar-boundary construction

A feasible fallback must construct all selected chamfer strips and the shared miter closure **as one coupled geometric operation**. Sequentially applying independent one-Edge Chamfer Features and reinterpreting the authored set is forbidden.

Candidate mechanism for investigation: analytic trim/intersection of the local planar carrier arrangement, with shared exact curve/vertex construction and an oriented closed shell. Outside the bounded modified corner neighbourhood, preserve the original material boundary. Use OCCT construction/sewing/Boolean mechanics only if they can produce a valid one-solid B-Rep without adaptive fuzzy/healing. This paragraph specifies a **research hypothesis**, not an approved choice of OCCT builder.

Required geometric postconditions:

- exactly one closed, consistently oriented manifold solid;
- `BRepCheck_Analyzer::IsValid()` true; no invalid Face, Wire, Edge, or Vertex;
- every explicitly authored Edge contributes its requested chamfer at the original common distance;
- nonzero exact signed material delta against the upstream Body (detect and reject `no_effect`);
- no unintended remote geometry edit, implicit omitted Edge, tangent-chain expansion or partial success.

### F3 — Provider-local provenance contract

Before any production publication, a feasibility prototype must demonstrate an explicit internal mapping:

- inherited upstream Face -> zero/one/many *known constructed* descendant Faces;
- each new strip Face -> **exactly one** authored source Edge;
- each new corner/miter Face (if present) -> the uniquely identified source shared Vertex **and explicit incident selected Edges**;
- all new topology -> complete current Face/Edge/Vertex inventory; no conflicting Surface ownership;
- no untracked new material Edge or fabricated semantic carrier.

A provider-private construction/history adapter may be proposed to feed the existing topology/lineage finalizer. Such an adapter is **not yet proven compatible** with the current operation-template `Generated/Modified` expectations. It must be kept behind Kernel API and may not invent persistent topology identity. If this cannot be expressed without a public Kernel contract or changed semantic reference policy, STOP and request a more specific D2 approval.

### F4 — Verify as one authored Feature

Only after F1–F3 succeed together should an accepted implementation be considered. Preview and Finish must use the same successful candidate and revalidate current stage/revision. Existing successful one/two/trihedral Chamfer paths remain unchanged. No persistence or modeling-semantics-version changes are presumed; any behavior-impacting compatibility decision must be explicitly reviewed under ADR-0014.

## Evidence gates for any future Owner-authorized implementation

| Gate | Minimum falsifiable evidence |
|---|---|
| Exact Part008 geometry | Three authored Edges at d=1 mm: one valid changed solid, complete lineage |
| Input-order determinism | Six permutations produce the same accepted semantic result; no order-selected owner |
| Size sensitivity | d=0.5 and 0.25 mm, plus explicit invalid/degenerate distance outcomes |
| Surface semantics | Each constructed strip/corner Face uniquely classified; zero false Resolved |
| Material correctness | Exact before/after signed deltas; no-effect and partial operation rejected |
| Negative topology | Coincident unrelated Faces, ambiguous split/alias, unsupported curves fail closed |
| Regression isolation | Prior valid 1-/2-/3-Edge Fillet/Chamfer, Revolve/Extrude and strict selection unchanged |
| Lifecycle | Create/Edit, Preview/Finish/Cancel, Undo/Redo, Save/Close/Reopen and cold rebuild |
| Presentation | Neutral committed Body, correct local added/removed preview, Shaded/Edges visibility |
| Release | FOCUSED -> FAST -> exact-head FULL -> explicit Owner Windows PASS; Draft PR until then |

No temporary diagnostic toggles or kernel instrumentation should remain in the final production candidate.

## Owner decision surface

**D2-A (Owner approved 2026-10-08):** authorize a **bounded evidence-first provider-local planar-miter feasibility spike** for the exact mixed three-straight-Edge equal-distance Part008 case. The spike may add isolated diagnostic/test code and measured provider experiments, but **not** production behavior or new public contracts until its geometric and lineage oracle passes and the Owner explicitly approves implementation.

**D2-B (separate later gate):** after that evidence, authorize a precisely scoped production fallback, with any internal history-adapter design and modeling-semantics compatibility decision recorded.

**D3-C (independent):** decide whether all valid two-Edge corner connectors must eventually be planar. Default remains **no change**.

Until D2-B is separately approved, keep the triple regression RED, preserve fail-closed invalid-BRep rejection, do not merge PR #298, and do not activate PM-06.

## D2-A spike checkpoint — 2026-10-08

The Owner explicitly approved **D2-A only** in the conversation after the initial proposal was committed. This is authorization for diagnostic and test-only feasibility work; it does not accept the parent production-fallback D2 proposal.

Commit `8dda62284d00bc7c08a09a41dd7a13ce8e36aa01` adds an **opt-in, failure-isolated** private `probeChamferPlanarMiter` inside the OCCT adapter, activated only under the existing native regression's `SS2_PM05F_R2_CHAMFER_TRIAGE` test switch. It measures, from the *actual* selected OCCT Edges and upstream Body:

- line-vs-curve source, two semantic adjacent Surface carriers, two incident Faces and planar eligibility;
- validity and generated strip Face kind for each independent one-Edge equal-distance Chamfer;
- exact shared-Vertex degree of the three input Edges, never proximity-based adjacency;
- intersection conditioning of single-Edge generated support planes at common two-/three-Edge junctions, and the finite candidate intersection position when defined.

The probe is **read-only** with respect to authored inputs and the production result. It must not authorize a geometry fallback from diagnostic results. Exceptions in diagnostic work are caught so they do not change regular Chamfer status. Instrumentation is temporary and must be removed from any final accepted production candidate.

**Evidence status:** the original three-Edge `CHECK(kernel_three.ok())` is deliberately unchanged, so a correctly executing focused regression may still finish RED. Observe `PM05F_R2_MITER_*` diagnostic lines in Windows job logs before inferring F1 geometry. Do not claim F1/F2/F3 feasibility or B-Rep repair without those results. The 2026-10-08 decision does not constitute an Owner product PASS.

## D2-A F1/F2 actual Windows measurements — 2026-10-08

**Test boundary:** exact `tests/fixtures/pm05f_r2_part008_sanitized.ss2part` and the unchanged semantic three-Edge `CHECK(kernel_three.ok())` in `pm05f_r2.native_workbench_edit`. These FOCUSED runs are **RED by the original production check** after the read-only probe executes. A diagnostic-only candidate is not returned as a production Body.

### F1 — exact source topology and local strip supports

Windows FOCUSED [#1845](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37824803422) on `13c67c23`:

- All three selected OCCT Edges are straight Lines, each bounded by two planar source Faces and two strict semantic Surface carriers.
- Each single-Edge Chamfer at 1 mm builds a valid B-Rep and generates **exactly one planar strip Face**.
- Source graph is **not trihedral**: junction `(-5,-15,0)` connects Edges `0+1`; junction `(-5,5,0)` connects Edges `1+2`; no Vertex has selected degree three.
- Strip-support plane normals at both degree-two junctions have `|n0 × n1|² = 0.75`: a nonparallel analytical intersection.

Windows FOCUSED [#1846](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37825445549) on `c3246d72`:

- At d=1 mm the two nearest-to-junction points on the analytical miter intersection lines are `(-5,-14,1)` and `(-5,4,1)`, each 1.41421 mm from its source Vertex.
- The first pair `0+1` and disconnected pair `0+2` yield valid changed B-Reps; pair `1+2` yields invalid B-Rep even without selecting the third Edge.

Windows FOCUSED [#1847](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37825703125) on `6c8c5ea2`:

| Pair | d=1 mm | d=0.5 mm | d=0.25 mm |
|---|---|---|---|
| `0+1` | Valid/changed | Invalid (2 Faces) | Invalid (2 Faces) |
| `0+2` | Valid/changed | Valid/changed | Valid/changed |
| `1+2` | Invalid (2 Faces) | Invalid (2 Faces) | Invalid (2 Faces) |

The two registration orders of **each pair** agree at every distance. Therefore neither selecting a different OCCT `Add(edge)` order nor reducing distance fixes this case. Note the nonmonotonic `0+1` behavior: success at 1 mm but invalid at smaller distances. The full triple remains invalid in the existing original test.

### F2 — exact signed-delta Boolean construction feasibility

Windows FOCUSED [#1848](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37825991972) on `ae516d5f`:

- For each independent valid one-Edge Chamfer, `B0 - S_i` and `S_i - B0` use OCCT Boolean Cut with `SetFuzzyValue(0.0)`.
- Edge 0 and Edge 2 each **remove** material only; Edge 1 **adds** material only. This explains why a global subtractive planar clipping construction is not sufficient.
- Experimental composition of those exact deltas — both add-first/remove-last and remove-first/add-last precedence — yields **one changed solid**, valid `BRepCheck_Analyzer`, with **21/21 planar Faces** at d=1 mm. This shape is *throwaway diagnostic evidence*, not a Fillet/Chamfer production output.

Windows FOCUSED [#1849](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37826307095) on `43bf8b2f`:

- Each CSG candidate contains one planar Face sharing the supporting plane with **each** of the three independent Chamfer strip Faces (`strip_support_counts=1,1,1`).
- Exact zero-fuzzy Boolean Cut in *both directions* between the two CSG candidates reports no positive-volume difference. This establishes **material-volume equivalence**, **not** identical Face partitioning or topological identity.

Windows FOCUSED [#1850](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37826561327) on `53288b7a`:

| Distance | CSG remove-wins | CSG add-wins | Support faces for Edge 0/1/2 | Between-policy material difference |
|---|---|---|---|---|
| 1 mm | 1 valid changed solid, 21/21 planar Faces | same | 1 / 1 / 1 | none in either direction |
| 0.5 mm | 1 valid changed solid, 21/21 planar Faces | same | 1 / 1 / 1 | none in either direction |
| 0.25 mm | 1 valid changed solid, 21/21 planar Faces | same | 1 / 1 / 1 | none in either direction |

**Interpretation:** For this bounded fixture and three values, an exact no-fuzzy provider-local Boolean construction is **geometrically feasible** despite invalid direct OCCT Chamfer results. The diagnostic evidence does not establish a general Chamfer algorithm, a stable canonical Face decomposition, exact local trimming/size coverage at every strip boundary, proper edge/corner Surface ownership, or Edit/Persistence lifecycle parity.

### F3 — remaining blocking lineage proof (D2-A research, not D2-B permission)

The generated candidate contains 21 planar Faces, compared with the original upstream and independently generated strip Faces. Before it can become any product result, the experiment must demonstrate:

1. **Inherited Face descendants:** compose exact OCCT Boolean `Modified/Generated/IsDeleted` histories from each upstream Face through all signed-delta and final CSG steps, without geometric-nearest/source-order guessing.
2. **Generated strips:** trace the `Generated(source Edge)` Face from each independent Chamfer through the Boolean delta and candidate history, verify positive-area participation, and uniquely classify the final trimmed descendants as the corresponding `edge_transition`.
3. **Junctions:** classify any residual generated Face at `(-5,-15,0)` and `(-5,5,0)` against the unique shared source Vertex and its incident selected Edges, or prove that no separate Face patch exists; zero ambiguous ownership.
4. **Completeness:** every final Face, Edge and Vertex must be accounted for under strict stage-scoped runtime topology; do not infer identity solely from common planar support or visual similarity.
5. **Geometric controls:** positive material delta, proper shell/orientation, exact authored distance on all three trimmed regions, no remote unrequested modifications, size/ordering independence, unsupported/ambiguous inputs fail closed.

A private provenance composition adapter is a **candidate design requiring its own evidence**, not an accepted API change. If exact lineage is not recoverable through these internal OCCT histories, STOP for a more specific Owner D2 decision.

### Disposition after F2

**F1 qualified for the exact fixture; F2 geometric feasibility demonstrated for d=1/0.5/0.25 mm; F3 unproven and blocking.** The current production three-Edge test deliberately remains RED at `kernel_three.ok()`. No fallback is activated, no public Kernel API or persistent semantics changed, PR #298 remains Draft, and PM-06 remains gated. Only Owner-authorized D2-A isolated diagnostic/research code was added; all `PM05F_R2_MITER_*` probes must be removed before a final accepted production candidate.

## D2-A F3 private-lineage feasibility findings — 2026-10-08

**F3 testing remains diagnostic-only.** All tests below complete the opt-in OCCT probe and then remain **RED on unchanged original `CHECK(kernel_three.ok())`**, because the successful experimental CSG Body is never returned to production.

### Preserved upstream and edge-generated provenance

- [Windows FOCUSED #1853](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37827059028), `055b1eec`: at each distance and both CSG precedence orders, all **12/12 upstream source Faces** produce uniquely traceable `Modified` descendants. **12/21 result Faces** are exactly inherited once; **0 ambiguous**; **9 newly created Faces** require generated-surface claims.
- [Windows FOCUSED #1854](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37827374266), `4e430535`: the independent OCCT `Generated(source Edge)` strip Face for each authored Edge survives through exact signed delta and final Boolean histories. Exactly **three** final Faces have exclusive Edge strip provenance (`1,1,1`), with **no overlap** with inherited Faces and **no competing strip Edge owners**. Thus **15/21** final Faces have provider-history-backed, source-unique claims.

### Six new Faces and local corner evidence

- [Windows FOCUSED #1855](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37827665014), `676ea815`: of the remaining six Faces, four touch exactly one tracked Edge strip and one inherited upstream carrier, while two touch **both** explicitly selected strips at their source shared-Vertex junctions. Stable adjacency masks are `011` at the `1+2` junction and `110` at the `0+1` junction. All six also share a plane with at least one upstream Face, but **planar coincidence is not provenance**.
- [Windows FOCUSED #1856](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37827955223), `7e458816`: expanding source-upstream history from `Modified` to `Modified + Generated` yields **zero** certifiable upstream Face owners for each of those six, across both policies and three sizes. The existing inherited-ownership mechanism cannot assign them even with `Generated` included.

### The decisive Boolean tool-history limitation

[Windows FOCUSED #1857](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37828279696), `ee86608e`:

- All six Faces have exact OCCT **tool-delta Face history** under the experimental CSG sequence; none needs nearest-geometry matching just to identify a contributing Boolean tool.
- Four Faces have stable single-Edge tool contributor masks across both Boolean precedence orders: `001`, `010`, `010`, `100`. These are **not automatically valid inherited Surface or edge_transition claims**. They are new split/closure Faces that still require their own exact semantic classification.
- Critically, the **two corner-connected Faces change apparent tool-owner under equivalent Boolean construction orders**:

| Authored junction | Exact result-strip adjacency | `remove_wins` tool owner | `add_wins` tool owner |
|---|---|---|---|
| Edges `1+2` (near `(-5,5,0)`) | `011` | Edge `2` (`001`) | Edge `1` (`010`) |
| Edges `0+1` (near `(-5,-15,0)`) | `110` | Edge `0` (`100`) | Edge `1` (`010`) |

This exact pattern repeats at d=1, 0.5 and 0.25 mm. All six tool-history masks are supported by OCCT's exact algorithmic history, **but the corner owner selected by that history depends on a transient implementation order**; the two candidates have equivalent material volume and the same explicit authored Edges.

**Therefore neither `Generated(source upstream Face)`, plane coincidence, nor the last modifying Boolean tool is sufficient to assert a stable semantic owner for these corner Faces.** Simply choosing a fixed Boolean order would mask that contradiction, not solve the identity problem.

### D2-A disposition and explicit D2-B decision request

**D2-A proved:**

1. The exact authored set is supported by straight linear Edges and planar incident Faces.
2. Exact zero-fuzzy CSG of individually certified signed material deltas produces a valid changed single-solid all-planar B-Rep at 1 / 0.5 / 0.25 mm.
3. The two tested material-composition policies are volume-equivalent at these distances.
4. Exact provider histories certify 12 inherited Face claims and 3 Edge strip Face claims, with stable geometric correspondence for each source strip.
5. Two remaining corner patches are uniquely distinguishable by the explicit **pair of incident authored Edges** at two distinct source Vertices, while their *Boolean tool* provenance is order-dependent. Four additional new planar Faces also need explicit semantic classification.

**D2-A did NOT prove:** unique provider-independent Source Surface/Corner Surface ownership for all 21 Faces, complete Edge/Vertex current-topology semantic parity, exact local trim dimensional coverage of all three strips, replay/Undo/Edit/Save/Reopen, all six input orders for the experimental CSG candidate, or generalization outside Part008. The production validation remains RED and correct.

**D2-B — Owner decision requested, NOT approved:** authorize a narrowly scoped **OCCT-provider-private provenance ledger** for a deterministic analytic planar signed-delta equal-distance Chamfer fallback, only where the default provider reports an invalid B-Rep and all source Edges and incident Faces are strictly supported. The ledger must carry, through construction, explicit source identifiers for:

- each upstream source Surface and its split descendants;
- each independently generated Edge strip with exactly one authored source Edge;
- each **corner** with its exact shared source Vertex and **both** incident selected Edges, independent of Boolean last-writer/order;
- every additional split/closure Face and its exactly one derived semantic carrier, supported by exact construction or OCCT history, never by planar coincidence/proximity alone.

This is **not** permission to fabricate identity, assign ambiguous Faces by provider enumeration, or declare the six remaining Faces fully resolved. A candidate internal history adapter would still need a dedicated falsifiable native regression proving every one of the 21 Faces has exactly one valid Surface owner, no contradictory upstream/strip/corner claims, and complete downstream Edge/Vertex semantics. Any need for public Kernel interfaces, persistent/schema changes, tolerance relaxation, or altered product corner behavior is a **new D2/D3 STOP**.

**Until explicit D2-B acceptance: no production geometric fallback, no publication of experimental solids, no merge of Draft PR #298, and no PM-06.** Remove temporary diagnostics from the eventual production implementation candidate; do not change the original RED regression.

## Owner Inventor visual reference and F3 tool-history caution — 2026-10-08

**Owner evidence:** Two Inventor screenshots were supplied in conversation on 2026-10-08 showing an analogous **equal-distance Chamfer, 1 mm**, with a long horizontal lower strip, upright side strip(s), and a zoomed junction where the vertical strip meets the lower strip next to another sloped/extruded material boundary. Selected Edges appear blue/dashed and chamfer candidate regions are tinted. Inventor's `Setback` UI controls are visible, but these pictures do **not** establish which corner mode was selected, Inventor's internal construction method, or equivalence of the authored CAD geometry.

**Visual oracle only, bounded by semantic and geometric checks:**

- The junction is **not** visually a single continuously extended chamfer strip: multiple planar-looking bounded patches and short miter/transition Edges are visible, including a small right-lower corner triangle.
- The image supports testing whether the experimental closed Part008 CSG output has explicit nonoverlapping corner/side patches and no gap near the *two* already confirmed source shared Vertices, `(-5,-15,0)` and `(-5,5,0)`.
- It does **not** show or prove that all six unclaimed Faces are corner patches, that the CSG candidates satisfy every local setback distance, or that any Face has a persistent CAD owner.
- This remains a shape/appearance reference for eventual **manual Owner comparison**. Do not introduce a `Setback` Chamfer variant or change previously accepted two-Edge connector geometry merely because the Inventor dialog displays these controls; those would be product D3 changes.

**Verified subsequent F3 evidence:**

- Windows FOCUSED [#1853](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37827059028), d=1/0.5/0.25, both Boolean precedence policies: 12 of 21 final planar Faces have **exactly one** inherited upstream Face owner through chained OCCT `Modified` history; 9 have zero; none ambiguous.
- Windows FOCUSED [#1854](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37827374266): exactly three of those nine have unique certified descendants from their corresponding independent `Generated(source Edge)` strip Faces; thus **12 inherited + 3 strip + 6 still unclaimed**. No inherited/strip overlaps or cross-strip owner conflicts.
- Windows FOCUSED [#1855](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37827665014): exact shared-result-Edge adjacency masks of six unclaimed Faces are `001`, `010`, `010`, `100`, `011`, `110`. The two faces touching two strips are *corner patch candidates*, not automatically certified corner owners. Four face fragments touching one strip also remain unclaimed.
- Windows FOCUSED [#1856](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37827955223): broadening upstream Face descendants to `Modified+Generated` does **not** certify any of those six (`inherited_expanded_owners=0` for each). Geometric coplanarity with an upstream Face must not be elevated to lineage.
- Windows FOCUSED [#1857](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37828279696): each unclaimed Face is traceable through at least one exact final Boolean **tool input Face history**, but the source-delta mask for the two dual-strip-adjacent corner candidates **depends on add/remove precedence**. Example at d=1 mm: under `remove_wins`, candidate `011` traces to delta input `001` and candidate `110` to `100`; under `add_wins` these two trace to `010`. Since both candidates were earlier proven *equivalent as material volumes*, the tool-input Face source is **not an invariant authored corner owner**. Source-Edge `Generated` strip history remains unique, but residual corner partition authority is **still missing**.

**Implication for next F3 spike:** Seek a deterministic geometric/topological *corner certificate*, expressed in terms of the two actual shared authored Vertices and their exactly incident selected Edges, together with an OCCT-backed provenance for every resulting Face. A transient tool-delta index or construction order may be retained only as evidence, never as persistent or semantic identity. Treat residual single-strip-adjacent Faces as separate candidate inherited fragments or local corner-adjacent pieces until their specific lineage is proven. F3 remains BLOCKED; Owner D2-B production fallback is **not** authorized.

All reported diagnostic FOCUSED CI runs continue to fail the unchanged original production `CHECK(kernel_three.ok())` after producing the diagnostic output. The screenshot files are conversation references, not copied into the repository.
