# PG-01C — Owner Sketch 5: numerical endpoint diagnosis and bounded D2 STOP

**Status:** DIAGNOSTIC / OWNER DECISION REQUIRED; not a production repair or PG-01C acceptance.  
**Observed on:** 2026-10-09, Owner Windows re-test of PR #304 at `0f338a3`.  
**Scope authority:** `work/PROJECTION_01C_PROJECT_EDGE_UI_CONTRACT.md`, particularly §§3, 5 and 8.  
**Owner input:** a locally supplied native v15 Part. The file, its complete coordinates, names, document ID and payload are **not committed**. This report uses only a topology summary and rounded magnitudes.

## Confirmed observations

1. The Owner manually confirmed that an unsupported newly selected Edge now preserves the prior valid Project Geometry staging. This issue is **Owner PASS**, but PG-01C as a whole is not.
2. The supplied Part has a fifth Sketch with four **Regular linked Lines**. Their persisted seed endpoints visually enclose a quadrilateral, but exactly one join agrees bit-for-bit; the other three pairs have coordinate gaps of about `7.1e-15`, `7.9e-15` and `1.0e-14` millimetres. The four sources are material Edges from the post-Chamfer Body stage and the Sketch is offset-Datum supported. No completed Profile for this Sketch exists in the saved document.
3. The Owner cannot make the Profile on those Lines either before or after Break Link, but can create it after explicitly reconnecting their endpoints. **Break Link must not autoheal geometry**.
4. For a linked projected Circle, creating a Profile succeeds but the Tree later reports `[Invalid]`, recovering to `Valid` after Break Link. The saved diagnostic fixture does **not** contain this linked Circle configuration; its status cause is from source-code inspection, not a fixture reproduction.
5. UI Break Link currently refuses `selectedCount() != 1`. The singular semantic Command has one `EntityId`; a multi-select UI loop would commit partially and is unacceptable.
6. Git comparison `5d31da8..0f338a3` did not change the OCCT projection or Shared-2D region evaluator. Thus the previous implementation's reported successful Profile creation remains an **unreproduced behavioral difference**, not proven to have been caused by the staging fix.

## Technical localization

- `src/kernel_occt/solid_modeling_kernel.cpp::OcctSolidModelingKernel::projectEdgeToPlane` (Line branch) independently evaluates each Edge's BRep adaptor `FirstParameter/LastParameter` points and projects them to the same Frame3. Independently evaluated neighboring edge curves need not produce bitwise-identical endpoint coordinates.
- `src/sketch/region_analysis.cpp` uses strict `Point2 == Point2` for shared authored line endpoints; no general gap healing is approved. The documented micron-scale / roundoff-scale discrepancy is consistent with a missing region, but **the OCCT common-vertex identity of the three pairs has not been established**.
- The fifth Sketch's authored seed and effective current projection should be separately measured after cold evaluation, particularly after provider/Feature reconstruction. A current-source disagreement cannot be repaired by changing authored seed or by importing close points into durable topology.
- `tests/pm02f_stage_aware_profile_evaluation_test.cpp` now includes a **completely synthetic** one-ULP open/closed characterization. It intentionally keeps Shared-2D exact endpoint rules unchanged and never uses the Owner file in CI.

## Independent synthetic OCCT identity check (local, NOT the Owner Part)

A separate native OCCT 7.9.3 experiment built a four-sided deliberately skewed planar extrusion and chamfered its four upper perimeter edges. For each resulting analytic straight Edge, the experiment read **both** oriented endpoint vertices via `TopExp::FirstVertex/LastVertex(..., true)` and the corresponding edge-curve samples via `BRepAdaptor_Curve::Value(first/last)`. All pairwise samples belonging to exactly `TopoDS_Vertex::IsSame` groups were compared in 3D, without any closeness-based matching.

- In this independent synthetic model, **34 endpoint pairs** sharing the same exact `TopoDS_Vertex` had **different sampled curve endpoint coordinates**. The greatest pairwise distance was about `3.18e-14` model units (mm).
- `BRep_Tool::Pnt(shared_vertex)` is the one canonical point associated with each matching vertex, regardless of which incident Edge is inspected. The observed differences therefore can arise solely from separate analytic edge-parameter evaluations; they are not proof of an open 3D vertex wire.
- This is reproducible **mechanism evidence**, not source-topology certification of the Owner's saved Part, nor proof that directly substituting vertex coordinates is always modeling-semantics-compatible. The Part-specific common-vertex check, geometry validity and safety boundaries in D2-K remain mandatory.
- The experiment ran locally with Python bindings to OCCT rather than against the application's selected Kernel build. It does not constitute a Windows C++ regression PASS.

## Bounded follow-up decisions

**D2-K — provider source-vertex continuity proof and repair**, needs Owner authorization to add `src/kernel_occt/solid_modeling_kernel.cpp` (and related strictly necessary provider-owned headers/tests) to PG-01C permitted file scope. Required first: native OCCT evidence that adjacent projected Edges in a synthetic post-Chamfer/Datum fixture share exactly the **same TopoDS_Vertex** (including location) and that current Line end samples differ numerically. Only then consider projecting that *same topological vertex's authoritative point* for all incident supported linear Edges. Prevent unrelated nearby vertices from merging. Verify reversed edge orientation, geometric carrier compatibility, degenerate results, source changes, cold reopen, no persisted tokens and no schema change. Any change affecting modeling/topology semantics must be separately classified under ADR-0014 §7 and may require further Owner D2.

**D2-T — Tree status**, narrow `src/ui/part_document_tree_controller.cpp/.hpp` scope expansion for one read-only revision/provider-aware Profile evaluation source shared with Viewer/Properties. Do not modify PartDocument's authored-only fail-closed method.

**D2-P — Profile Create/Edit semantics**, separate bounded revision- and provider-aware semantic Command revalidation using the exact current effective Sketch on execution, without accepting stale saved seed or widening Feature dependencies. A new Application/Part API contract needs Owner approval.

**D2-B — batch Break Link**, a single atomic semantic Command for one or more selected linked EntityIds, one Undo with rollback when any current source fails. No sequence of single-command partial commits; unaffected Profile and entity identities persist.

## Next gates

1. Owner decides *each* D2 scope independently; `kontynuuj pracę` authorizes diagnosis/tests only, not implicit API or file-scope changes under the accepted Constitution.
2. New native procedural non-private post-Chamfer/Datum repro establishes actual shared `TopoDS_Vertex` evidence, then RED→GREEN with exact current geometry at every presentation/read gate.
3. Windows FOCUSED and exact-head FULL, generated docs only if behavior changed; Owner manual Sketch 5 and linked Circle/Break Link re-test.
4. No merge, no PG-01D, no PM-06 and no CI issue #302 work while these PG-01C acceptance blockers remain.

## Documentation impact

Internal docs: not required **for this diagnosis-only change**.  
User/Product docs: not required **for this diagnosis-only change**.  
Any approved production UX/semantic change requires the existing contract's canonical docs PL/EN and regenerated Browser obligations.
