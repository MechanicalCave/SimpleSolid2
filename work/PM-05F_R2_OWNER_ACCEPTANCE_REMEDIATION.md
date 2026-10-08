# PM-05F R2 — Second Owner Acceptance Remediation

**Status:** ACCEPTED / ACTIVE — Owner 2026-10-08
**Parent Work Contract:** `work/PM-05_EDGE_FEATURES.md`
**Program:** `work/PART_MODELING_V1_ROADMAP.md` v1.29
**Baseline:** `main` at `b0f6e148c25027aa7f6cbf5a6a33250685d7591c`
**Previous automation:** Windows FULL #1744 PASS on `e1c358cf54fd051f15252ea198d116de0b6a9cdc`; PR #296 merged
**Owner acceptance attempt 2:** FAIL — 2026-10-08
**Decision boundary:** D0/D1 remediation inside accepted PM-05; D2/D3 discoveries STOP for Owner

## Purpose and Owner evidence

The Owner manually retested Fillet/Chamfer after #296 and reports material improvement but five remaining issues:

1. Preview colors the *entire candidate Body*, not the local Fillet/Chamfer effect, and does not show removed material orange versus added material blue as the accepted Extrude/Revolve presentation grammar requires.
2. A three-Edge Fillet at one corner of a simple cube can show a successful preview but leave the entire Body invisible after Finish.
3. Ordinary material Edges on a full or partial Revolve Body cannot be selected for Fillet/Chamfer.
4. Entering Edit for a previously committed Fillet/Chamfer succeeds, but existing selected Edges cannot be toggled off and further Edges cannot be added.
5. Some visible curved/silhouette Edges are difficult or impossible to pick precisely (Owner provided a ring-edge screenshot).

The Owner additionally supplied an independent static Edge-picker audit; its `tool_stage` contradiction was verified on current `main`. The Owner accepted this bounded R2 scope on 2026-10-08, then accepted use of existing efficient CI tiers for iteration, while deferring CI policy changes to a separate measured contract.

All findings must be closed explicitly; a code-local confirmation is not yet a runtime PASS.

## Finding matrix

| ID | Priority | Owner symptom | Static classification | Required disposition |
|---|---|---|---|---|
| R2-01 | P0 | Three adjacent cube Edges -> Fillet Finish -> invisible Body | Reproduction/root cause pending | Runtime kernel/evaluator/Viewer separation; no stale valid Body |
| R2-02 | P1 | Edit cannot add/remove Edge selections | CONFIRMED: `qt_occt_viewer_widget.cpp::queryBodyTopology` rejects `tool_stage` | Real Qt/OCCT pick -> controller -> semantic draft regression |
| R2-03 | P1 | Revolve material Edges cannot be selected | Owner CONFIRMED; root cause pending | Separate geometry hit, occlusion, catalog, strict authoring and provider outcome |
| R2-04 | P1 | Preview colors entire Body as Add | CONFIRMED: `CadWorkbench::refreshEdgeFeaturePreview` sends whole candidate mesh with additive tone | Local before/after difference or equally exact evidence-based presentation |
| R2-05 | P2 | Curved Edge picking imprecision | Mechanisms confirmed, causal attribution pending | Quantified deflection/projection/occlusion tests |
| R2-06 | P2 | Visibly indicated but semantically unsupported Edge feels unselectable | Filtering confirmed, UX impact pending | Explicit, safe diagnostic classification |

## Bounded delivery order and evidence

### R2-A — Body integrity first

Reproduce a simple cube with two adjacent and three trihedral Edges, varying radii. Trace draft preview, exact candidate result, commit, post-commit evaluation, topology completion and Viewer publication. Compare Feature status, current Body, resolved-prefix Body and display scene. Verify Undo/Redo and cold Save/Reopen. Fail closed; never convert a failed/blocked Body to stale last-good truth. Stop for Owner D2 if the fix needs altered durable Feature/evaluation semantics.

### R2-B — Edit tool-stage picking

The Viewer query currently returns an empty successful query for every scene other than `current_body`. Allow `tool_stage` *only as a transient query*; controller retains owning-document, stage, generation, and strict `MaterialEdgeReference` authoring validation. `diagnostic_prefix` stays rejected. Prove real Qt/OCCT cursor hover/select/toggle/deselect/add, both Fillet/Chamfer, Cancel zero mutation, Finish preserving FeatureId, stale scene/document rejection, Save/Reopen parity. Mock token injection alone is insufficient.

### R2-C — Revolve Edge support

Fixtures: partial Revolve with planar caps, complete 360-degree Revolve with periodic seam, circular material edges, curved side boundaries, Add/Cut where valid. Log actual Viewer candidates, visibility, catalog accounting, semantic Curve/Edge resolution and draft result. Admit ordinary uniquely resolved material Edges under accepted strict semantics; seams/representation partitions are not authorable. Do not invent nearest/XYZ/order identity. If a new durable semantic meaning is unavoidable, STOP D2.

### R2-D — Exact local preview

The normal Body stays neutral. Visualize only removed material in orange and added material in blue. Let `B0` be the immediate predecessor solid, `B1` the exact candidate: removed = `B0 \\ B1`; added = `B1 \\ B0`. This is derived runtime display, never durable geometry identity or a substitute for the accepted candidate. Characterize kernel accuracy/performance before authorizing new public Kernel/Viewer APIs. Do not add fuzzy/healing tolerance silently. Test both convex and concave scenarios, cancellation, stale preview, failure/degradation and Finish parity. Stop D2 if new public API or operation policy is required.

### R2-E — Curved/silhouette Edge selection

A visible cylindrical/revolved ring boundary is the Owner's reference case. The currently observed mechanisms are `GCPnts_QuasiUniformDeflection(curve, 0.25)`, integer `projectToScreen`, 8 logical-pixel Edge aperture and triangle-based `bodyPointFrontVisible`. First collect candidate counts and rejection points at multiple zoom/DPI/camera angles; distinguish polyline miss, occlusion, stale generation and semantic rejection. Only then choose minimal local rendering/picking corrections. No global visibility relaxation that admits rear occluded Edges.

### R2-F — diagnostics

Differentiate `not_hit`, `occluded`, `representation_artifact`, `unsupported`, `ambiguous`, and `stale_context` as appropriate in the existing UI/presentation contracts. A rejected candidate must have no durable mutation.

## Authority, limits and stop conditions

Preserve Foundation v1.0, accepted ADRs, exact semantic stage, Command -> Validation -> Transaction -> owning Part Document -> Evaluation, and strict authored intent with permissive *local result* topology. Never relax tangent input-contour membership, fail-closed missing/ambiguous/unsupported states, Undo/Redo, native-file reconstruction, schema or ID rules.

No direct main edits; no PM-06; no advanced Fillet/Chamfer variants; no generic native-OCCT picking migration, universal picker rewrite, new tolerance policy, provider-order/XYZ/nearest fallback or silent scope expansion. D0/D1 only inside this Owner-accepted bounded remediation; any D2/D3 finding requires Owner decision before implementation.

## Verification / merge / closure

Existing PM-05A–F tests remain mandatory. Add honest regression fixtures at the appropriate Core, kernel-native and native desktop boundaries. Real Qt/OCCT pick tests must exercise screen coordinates rather than fabricate accepted token selection. All unresolved findings remain explicit.

During development: Draft PR; FOCUSED for named targets/tests (kernel focus for kernel-native), FAST/SUBSYSTEM checkpoints. Changes to `tests/CMakeLists.txt` or verification scripts trigger CLEAN FULL by existing classifier; batch registry changes rather than repeatedly forcing a clean run. Make Ready only when runtime candidate is stable. Final exact-head Windows FULL, current internal/product docs and regenerated Browser, then repeat Owner manual Windows acceptance. Only explicit Owner PASS closes PM-05.

CI-05 classifier/build policy optimization is not part of R2 implementation; do not alter `.github/workflows/**` or `scripts/ci/**` without separately accepted authority.

## Documentation impact

Internal/Product documentation: update after actual behavior changes are verified. Product Browser is generated, never hand-edited. This file and associated `work/**` changes are governance/evidence only.

## Focused evidence checkpoint — 2026-10-08

- R2-B: test-only candidate a32568a Windows FOCUSED #1748 expected FAIL; corrected native Viewer query passed Windows FOCUSED #1750 (1/1) on b9ff051. Full Workbench Edit acceptance remains open.
- R2-A: box 40x30x20, Radius 2, three-edge Fillet Finish and current Body publication passed Windows FOCUSED #1751 (1/1). The Owner's disappearing-Body case remains open and not reproduced by this control case.
- R2-C: Windows kernel FOCUSED #1754 confirmed full-turn Revolve with 2 ordinary candidate Edges, 0 strict authorable Edges, 1 representation artifact. In current Part classifier only ordinary Extrude `side` is recognized as a side Surface; `revolve_side` lacks a valid same-producer analytic Curve rule. Any new durable Revolve Curve relation needs explicit Owner D2 approval; production semantics mutation is stopped pending that decision. No nearest/provider-order workaround.
- R2-D/E/F open. No new R2 exact-head FULL or Owner PASS.

## Owner D2 acceptance and current R2-C evidence — 2026-10-08

Owner accepted **one unified stage-scoped Body Surface/Curve/Point model** across generators, rather than a Revolve-only Edge identity system. Binding authority and negative cases are in `work/PM-05F_R2_UNIFIED_BODY_TOPOLOGY_D2.md`. The R2-C semantic D2 STOP has been lifted **only within that accepted bounded contract**. Any new durable Curve role/schema or point-identity extension still requires separate Owner D2 review.

The shared Part Curve classifier now admits analytically supported same-producer Revolve Surface pairs via existing canonical `cap_side` / `side_side` Surface-based provenance, while preserving Extrude-specific checks, explicit Body stages, strict cardinality, non-authorable seams and representation partitions. No persisted reference or schema was added.

- Before D2 implementation: kernel FOCUSED #1754 expected FAIL with 2 ordinary Revolve Edge candidates, 0 authorable.
- Kernel FOCUSED #1759 PASS (1/1): full-turn and partial Revolve authoring/resolution using existing shared semantic references.
- Kernel FOCUSED #1760 PASS (1/1): geometry-equivalent Extrude/Revolve cylinders, strict circular MaterialEdgeReferences, production Fillet and Chamfer for both generators, and repeated cold semantic evaluation.
- These tests prove the catalog and Kernel/Part behavior for their fixtures. Real native Viewer pointer precision and the Owner's full/partial Revolve interactive workflow remain separate open gates. Final exact-head FULL is not yet attempted.

Owner also accepted a **separate evidence-first runtime performance direction**, recorded in `work/PART_RUNTIME_PERFORMANCE_EVIDENCE_DIRECTION.md`. It does not activate caching, CI-05, PM-06, or performance implementation within this R2 PR.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: shared semantic interpretation and newly supported Revolve material Edge operations require current as-built documentation and bilingual user descriptions after complete runtime acceptance. Generated Browser must be synchronized, not manually edited.

## Additional R2-A/E focused evidence — 2026-10-08

- **R2-E:** Windows FOCUSED #1764 PASS, native `sk04b.viewer_native_selection_query` (1/1). Real Qt/OCCT screen-coordinate hover/click on a synthetic cylindrical Body with a curved top material ring: two camera scales and a rear bottom-ring occlusion negative control. The test uses a **densely sampled** ring display polyline; it does NOT establish that production 0.25 mm OCCT edge sampling is precise enough at high zoom/DPI. Owner screenshot/real Revolve picking remains OPEN. No tolerance was relaxed.
- **R2-A:** Windows kernel FOCUSED #1765 PASS, `pm05f.fillet_corner_diagnostics` (1/1). Additional cubes 10x10x10 and 20x20x20 and geometrically reasonable Fillet radii; any successful provider result must evaluate UpToDate with complete product topology. No provider-success/product-failure contradiction for this tested matrix. Owner's vanished Body after Finish remains OPEN because this test lacks the exact Owner geometry and native Viewer publication.
- **R2-D architecture finding:** current `ISolidModelingKernel` has presentation-only exact deltas for Extrude/Revolve, but not Fillet/Chamfer. `SolidPreviewScene` has a single `SolidPreviewTone` for the whole scene. True simultaneous before/after additive/subtractive patches require a bounded Kernel and atomic Viewer preview-interface extension. **D2 STOP pending Owner approval** of `work/PM-05F_R2_EDGE_PREVIEW_DELTA_D2_PROPOSAL.md`. Whole-Body blue overlay is not accepted as completion.

## R2-B Workbench selection lifecycle checkpoint — 2026-10-08

Windows FOCUSED #1767 PASS (1/1), `pm05e2.cad_workbench_edge_lifecycle` on `3c6c4c688065d978f18429a7dda52c4b3b256c11`. The real Workbench/controller with an instrumented viewport now verifies an Edit-stage restored Edge selection, cursor-intent toggle OFF and ON, adding and removing a second semantic Edge, restored exact preview, Cancel zero mutation, plus prior Edit/Finish stable FeatureId and Undo lifecycle. This test injects provider-neutral selection intentions; separate native Qt/OCCT widget FOCUSED #1750 proves screen-coordinate input at `tool_stage`. A single end-to-end native Qt/OCCT **Workbench** mouse test and repeat Owner workflow are still required before closing R2-02.

## R2 native CI reliability checkpoint — 2026-10-08

- Windows FAST #1766 PASS on the previous evidence candidate.
- Windows FAST #1768 FAIL: 92/93 PASS, native `sk04b.viewer_native_selection_query` failed an existing pre-ring Tab-cycle assertion (`cycle_forward == 1`), while the identical expanded native test passed FOCUSED #1764. Failure occurred **before** the R2 curved-ring fixture and is not evidence that curved-edge selection failed.
- Native `QtOcctViewerWidget::event()` handles Tab only while Body preselection is armed. The test previously drained global Qt/OS events after synthetic hover and focus, allowing queued real-cursor events to replace/clear the deterministic hover. Test corrected to establish focus and drain unrelated events **before** rearming the synchronous cursor hover, then dispatch Tab/Shift+Tab without an intervening global event pump. Both cycle assertions retained.
- Windows FOCUSED #1769 PASS (1/1) with the ordering correction. A new same-branch FAST is still required before claiming aggregate PASS. The next governance-only commit intentionally requests FAST; no workflow/classifier edits.

## Owner-approved R2-D exact local preview checkpoint — 2026-10-08

Owner explicitly accepted the bounded presentation-only Kernel and Viewer interface extension in `work/PM-05F_R2_EDGE_PREVIEW_DELTA_D2_PROPOSAL.md`. Earlier D2 STOP is lifted **only for that accepted contract**. No authored schema, topology meaning, modeling tolerance, generic cache or CI policy changed.

The exact successful upstream `B0` and candidate target-stage `B1` now feed provider-neutral `materialDifferencePreview`. OCCT computes zero-fuzzy exact B-Rep differences: `B0 - B1` (removed/orange) and `B1 - B0` (added/blue); a zero-sided difference is represented by an absent mesh, not fabricated material. The Viewer atomically installs both optional meshes in a generation-scoped `SolidPreviewScene`, while unchanged accepted Body retains its own neutral shading. This transient presentation is **never Finish or persisted CAD identity authority**. Unsupported/failing/no-effect preview never falls back to a fully recolored candidate Body.

Focused Windows evidence on separate exact candidates:
- **#1771 FULL PASS** before this specific preview change: 113/113 desktop and 57/57 kernel; real Qt/OCCT mouse → Workbench Fillet Edit integration `pm05f_r2.native_workbench_edit` passed. This is not the final R2-D exact-head FULL.
- **#1772 kernel FOCUSED PASS** provider-neutral exact-difference API build/base kernel tests.
- **#1774 desktop FOCUSED PASS** Fillet Edit Workbench preview/Cancel lifecycle after the dual-color implementation; #1773 was a temporary compile RED from the `FeatureEvaluation::id` vs `feature_id` typo, corrected before #1774.
- **#1775 kernel FOCUSED PASS** convex Fillet/Chamfer return true local removed material with no blue candidate-Body substitution, plus executable evaluation-time instrumentation.
- **#1776 native Qt/OCCT FOCUSED PASS** dual-color, added-only and removed-only atomic scenes, neutral Body isolation, stale generation rejection and explicit clearing.
- **#1777 kernel FOCUSED PASS** exact delta for complex four-Edge Line/Circle capsule opening, and exact no-effect on identical before/after solids.
- **#1778 expected diagnostic FAIL** demonstrated why a blind pocket's *opening* Boolean boundary is not the *concave floor*; its valid Fillet removed, not added, material.
- **#1779 kernel FOCUSED PASS** semantically targeted internal wall/floor `cap_side` Curve loop in a blind pocket; UpToDate Fillet generates actual added material, verified by `materialDifferencePreview` and without XYZ/provider-order selection.

Remaining R2-D verification: aggregate FAST, complete same-head FULL after other active R2 corrections, measured interactive p95/memory with realistic parts, real desktop visual Owner acceptance, canonical bilingual product docs and regenerated Browser. R2-A/E/F remain separate open findings. An operation may legitimately have only orange, only blue, or both deltas; no invented nonempty mesh is allowed.
