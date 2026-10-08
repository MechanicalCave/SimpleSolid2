# PM-05F R2 — Planar-miter Chamfer feasibility and lineage design

**Status:** PROPOSED / NOT ACCEPTED — 2026-10-08  
**Decision class:** D2; Owner approval required before production geometry implementation  
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

## Proposed bounded feasibility sequence (not authorized implementation)

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

**D2-A (requested, not granted):** authorize a **bounded evidence-first provider-local planar-miter feasibility spike** for the exact mixed three-straight-Edge equal-distance Part008 case. The spike may add isolated diagnostic/test code and measured provider experiments, but **not** production behavior or new public contracts until its geometric and lineage oracle passes and the Owner explicitly approves implementation.

**D2-B (separate later gate):** after that evidence, authorize a precisely scoped production fallback, with any internal history-adapter design and modeling-semantics compatibility decision recorded.

**D3-C (independent):** decide whether all valid two-Edge corner connectors must eventually be planar. Default remains **no change**.

Until those decisions, keep the triple regression RED, preserve fail-closed invalid-BRep rejection, do not merge PR #298, and do not activate PM-06.
