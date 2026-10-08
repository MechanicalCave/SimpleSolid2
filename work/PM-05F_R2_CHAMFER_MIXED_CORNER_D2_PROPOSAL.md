# PM-05F R2 — Mixed Chamfer Corner Closure (D2 proposal)

**Status:** PROPOSED / NOT ACCEPTED — 2026-10-08
**Authority:** PM-05 Edge Features / PM-05F R2 Owner remediation
**Decision:** D2 implementation boundary; Owner must explicitly approve before production geometry/policy changes
**Production mutation authorization:** D2-B APPROVED by Owner on 2026-10-08, strictly for the bounded provider-private fallback and provenance ledger described below; no public/persistent contract or product D3 extension authorized
**Owner decision:** D2-A feasibility accepted; D2-B bounded production spike accepted; D3 global corner appearance NOT accepted

## Owner evidence and exact bounded failure

Owner supplied Inventor screenshots of a comparable three-Edge Chamfer: straight, planar chamfer strips meet through a visibly mitered, piecewise-planar corner junction. This is a useful reference geometry, **not** an instruction to reproduce Inventor internals nor proof of equal surface dimensions.

The original SimpleSolid2 Part008 has two additive Extrudes and an upstream Body with three strictly authorable material Edges. The third Edge is **Extrude001 side/Entity 1 + side/Entity 4** as shown in the Owner's Properties pane, not sides 1+2. At Chamfer equal-distance **1 mm**, the existing selected two-Edge subset previews/commits; the third Edge alone succeeds; all three combined fail with `Chamfer rejected: Invalid B-Rep • Failed`. Live preview correctly clears and Finish is forbidden, rather than storing an invalid Body.

Strict native Windows FOCUSED #1834, #1835 reproduced this exact semantic triple on the persisted Part008 fixture, not using OCCT runtime Edge index or XYZ similarity. #1834 reports `solid_count=1`, `brep_valid=0`, `kernel.status=invalid_brep`. #1835: all **six** registration orders at **1 / 0.5 / 0.25 mm** fail identically. #1836: BRepCheck identifies **2/17 invalid Faces** and **2/17 invalid Wires**, with **0/78 invalid Edges**, **0/156 invalid Vertices** (whole Shell/Solid invalid). #1837: enumerated both incident OCCT faces for each selected Edge; `Add(d,d,E,F)` makes valid B-Reps for four of eight face masks. #1838: these valid B-Reps are **`no_effect`** under the existing strict finalization; they do not satisfy the requested edit and have no published transition surfaces. #1839 copied the upstream topology and then full geometry, exact mapped Edges: both remain invalid. #1841: `Add(E)` plus `SetDist(d,Contour(E),F)`, all eight masks, remains invalid. These are **negative investigations**, not a production fix.

The two-Edge case has **two planar `edge_transition` carriers** and **one genuinely nonplanar `corner_transition` carrier**, as measured by the provider (Windows #1834). The Owner questions this curved connector; the current accepted PM-05 contract does not require a planar connector for every two-Edge corner. Do not silently redefine that product behavior.

## Decision requested from Owner

Authorize a **bounded provider-local deterministic corner-construction remediation** for the **exact equal-distance material Chamfer on multiple explicitly authored line Edges**, including the mixed concave/convex network in Part008. The new fallback may build a **mitered piecewise-planar closed junction** analogous to the Owner's visual Inventor example when the existing OCCT `BRepFilletAPI_MakeChamfer` produces an invalid B-Rep. It must not be a generic healing mode or alter persisted Feature meaning.

Explicitly **leave the existing valid two-Edge nonplanar corner transition unchanged in the first remediation**. Changing all two-Edge corners to planar triangular transitions is a **separate Owner product-geometry choice (D3)**, not inferred from the reference screenshot. Owner may accept/reject that as a follow-up.

## Boundaries / failure conditions

- The complete strict semantic 1..N Edge set is one unordered authored Chamfer Feature against one upstream Body stage. No partial commit, no hidden tangent expansion, no sequential independent Feature reinterpretation.
- Only exact validated planar/linear corner geometry is in scope for the proposed fallback. Unsupported nonanalytic and mixed complex cases fail closed. All selected Edges must contribute at the common authored Distance.
- No new fuzzy tolerance, implicit distance reduction, tolerance inflation, blind `ShapeFix`, provider-order, runtime-edge-index, XYZ-nearest rebinding or persistence/schema expansion.
- The output must be a changed, single solid with `BRepCheck_Analyzer::IsValid()`, proper orientation/manifold/wire closure, nonzero local signed material delta, exact input membership and complete unambiguous provider-generated `edge_transition` and `corner_transition` lineage. Surface carriers cannot be asserted from mere triangle tessellation or painting.
- If the geometry cannot yield complete lineage with accepted APIs, or requires changing public Kernel contracts, topology ownership, persistent semantic representation, or tolerance policy, STOP for a narrower Owner D2 decision rather than weakening requirements.
- Existing successful chamfers (one, two, connected trihedral and other) must remain unchanged unless separately approved.

## Proposed execution and evidence

1. Preserve the **exact Part008 semantic three-Edge RED** as an active Windows FOCUSED regression. Classify which planar profiles and miter intersections from the Inventor-style reference correspond to selected Edge/Corner source entities. Do not reproduce a pretty but semantically unaccounted solid.
2. Implement and prove a local provider fallback only after Owner D2. Compare exact B-Rep geometry and topology against the native failure; assert proper one-solid shell, nonzero delta, two- and three-Edge corner continuity and explicit surface lineage, without arbitrary healing.
3. Prove deterministic results under all six input orders, several legal sizes and radius/distance variation, Edit/Finish/Cancel, Undo/Redo, cold Save/Reopen, as well as Shaded/Edges presentation.
4. Run FOCUSED → FAST → final exact-head FULL after Owner visual comparison and explicit Owner PASS. Continue PM-05F R2 in Draft PR #298, no PM-06 or main mutation until its separate gates.

## Current disposition

The current valid two-Edge nonplanar connector is a **product appearance question**, not automatically a modeling defect. The confirmed Part008 three-Edge `invalid_brep` is an **open acceptance blocker**. Keep the proposed fallback **UNAUTHORIZED** pending the Owner's explicit D2 decision.

## Owner D2-A feasibility outcome — 2026-10-08

Owner accepted the bounded **D2-A evidence-only provider experiment**, explicitly **not** a production fallback or public Kernel/history contract. Detailed test measurements and the temporary diagnostic source are recorded in [`work/PM-05F_R2_CHAMFER_MITER_FEASIBILITY_DESIGN.md`](PM-05F_R2_CHAMFER_MITER_FEASIBILITY_DESIGN.md).

- Native FOCUSED #1845–#1847 proved the exact three-edge Part008 set is a **two-junction chain**, with individual planar Chamfers valid but OCCT combined/pairwise combinations invalid under the observed distances.
- Native FOCUSED #1848–#1850 proved an isolated zero-fuzzy signed-delta CSG candidate produces a changed one-solid valid planar B-Rep at 1, 0.5 and 0.25 mm, with one matching support-plane Face per explicit source Edge and equal resulting material volume under the two tested precedence strategies.
- Native FOCUSED #1853–#1854 certify exact provider-history ownership for **12 inherited Face descendants plus 3 generated Edge-strip Faces** (15/21), without ownership conflicts, at all three distances and both CSG policies.
- Native FOCUSED #1855–#1857 identify **six additional Faces** via exact topology and OCCT tool histories. Two are adjacent to both Edge strips at their respective original shared-Vertex junctions, but their **single Boolean-tool owner flips** between equivalent add-wins/remove-wins constructions. That tool owner is not stable semantic identity. Four remaining split/closure Faces also need unambiguous classified Surface lineage.
- **F3 remains unresolved:** explicit, provider-private, construction-time source lineage for the two shared-Vertex/two-Edge corner transitions and all four split/closure Faces, plus all Face/Edge/Vertex semantics, positive-area trimmed participation, full input-order and lifecycle/persistence proofs. Do not infer Surface identity from planar coincidence or final Boolean tool order.

**Current decision:** D2-A remains restricted to bounded feasibility/lineage research; **D2-B production implementation remains unauthorized**. Do not use the valid throwaway B-Rep as a successful CAD Feature without accepted strict lineage evidence; do not weaken the red Owner three-Edge check.

**Specific pending Owner D2-B decision:** whether to authorize a constrained internal OCCT provenance ledger that assigns corner patches to their exact shared source Vertex and both incident authored Edges (independent of Boolean implementation order), while preserving unique ownership and fail-closed semantics for every additional Face. This would permit implementation research, **not** presume a successful product Feature. Exact evidence and D2-B conditions are recorded in the linked feasibility design. Until separately accepted, the production fallback remains prohibited.

## Refined D2-B decision requested after F3 diagnostics #1863–#1865 (2026-10-08)

**D2-A Owner-authorized feasibility evidence is recorded** in `work/PM-05F_R2_CHAMFER_MITER_FEASIBILITY_DESIGN.md`. It now classifies **all 21 candidate Faces geometrically**, but not yet all semantically:

- 12 exact upstream inherited Faces and 3 strictly selected Edge strip Faces already have unique OCCT history.
- 2 Faces have **one** unique same-domain inherited neighbor by exact shared result Edge, but extending ADR-0017 from **Extrude Add** to bounded Chamfer would require a **new explicit D2 decision**.
- The remaining two source joints each contain a **local triangular Face** (area `d²/2`) and a **quadrilateral Face** (area `d²`) for the tested three sizes. Exact selected-edge adjacency and source shared-Vertex pairing are stable under both Boolean precedence policies, but tool-delta Face last-writer provenance is not.

**Recommended Owner D2-B scope (NOT ACCEPTED):** allow implementation of a narrowly guarded OCCT-private provenance ledger for the specifically supported mixed linear planar equal-distance Chamfer network, with two limited semantic rules to be proven adversarially:

1. A new planar Face can continue an existing authored upstream Surface only if the Face has OCCT tool/operation provenance and exactly one provider-certified **current shared Edge and same-domain planar inherited Face**. This is **not** permission for arbitrary coplanar healing or for changing globally accepted ADR-0017 semantics.
2. A bounded local junction may produce **multiple current B-Rep Faces for one semantic corner Surface**, whose identity derives from **one exact source shared Vertex + both incident user-authored Edges**, with unique noncompeting construction/history evidence and no transient Boolean operation order or Face index in the durable address.

Both would remain behind the existing neutral Kernel/Part boundary, preserving the one-Feature authored selection, equality of its common distance, fail-closed unresolved claims, zero fuzzy/adaptive tolerance and no public API/persistence mutations by implication. A separate explicit STOP applies if production needs new durable topology roles, public contracts, modeling-semantics version changes, or a new product Chamfer corner policy.

**This paragraph records a recommendation, not Owner authorization**. Existing strict three-Edge RED remains, and no production candidate or acceptance was produced.

## Explicit Owner D2-B acceptance — 2026-10-08

After reviewing the exact Part008 F1/F2/F3 diagnostic evidence, the Owner explicitly accepted **D2-B** in the conversation ("akceptuję — kontynuujmy"). This is authorization to *attempt* a bounded production implementation of a provider-private planar equal-distance Chamfer fallback for geometrically supported mixed linear multi-Edge corners, not automatic acceptance of any implementation or of a new persistent modeling schema.

Implementation gates are mandatory: original 3-Edge material Edges only; default OCCT valid result stays preferred; no-fuzzy exact per-Edge signed deltas; one valid nontrivially changed solid; all authored Edge strips represented at true local offset; exactly one invariant semantic Surface owner for each final Face, including the two independently sourced planar continuation Faces and local corner split Faces; complete current-stage Face/Edge/Vertex accounting with unambiguous provenance; six Edge permutations, distances 1/0.5/0.25 mm; faithful edit/replay/undo/save and current topology no false Resolved; existing good 1-/2-/3-edge cases unmodified. If any gate fails, fall back to original fail-closed error, never silently accept partial geometry.

**Not authorized:** blanket same-domain healing, geometry/proximity rebinding, provider Face-index identity, implicit Edge expansion, tolerance escalation, changed schema/neutral public API, global planar-only 2-Edge product policy, merging PR #298, or activating PM-06. Owner manual Windows and exact-head FULL remain final acceptance gates.
