# PG-01C — Project Edge UI, effective presentation and interaction: D2 design proposal

**Status:** PROPOSAL ONLY — OWNER D2 REVIEW REQUIRED; NOT ACCEPTED; no product implementation authority
**Date:** 2026-10-09
**Baseline:** main \`8d78320daca8a72a1431fbece49296c3bfb83aec\`, PG-01B Owner FINAL PASS and PR #301 squash merged
**Program:** Part Modeling v1 roadmap v1.30 — Projection before PM-06
**Accepted upstream:** Projection 00A, PG-01A, PG-01B; Foundation §7.2 associative same-Part, §7.3 future Assembly snapshot CORE
**Companion draft:** \`work/PROJECTION_01C_PROJECT_EDGE_UI_CONTRACT_DRAFT.md\`

## 1. Purpose / scope boundary

Deliver the **single user-visible Sketch Project Geometry / Rzutuj geometrię tool**, limited to **same-Part material Edges**, using PG-01B's exact source bindings and semantic Commands. Toolbar starts the tool; the **right Operations panel** owns source selection, role, current results/diagnostics, Finish and Cancel. No standalone dialogue or competing tool semantics.

No Face boundary/holes, no cross-document/Assembly projection, no changes to native schema v15, no new geometry class or approximate curves, no topology similarity matching, no hidden automatic membership reconciliation. PG-01D and PG-01E remain separately gated. No PM-06.

## 2. Verified current implementation hazards

1. \`src/ui/part_viewport_controller.cpp::buildSketchScene()\` currently renders \`hosted->model.state()\` (authored geometry). For a PG-01B linked entity this is **seed**, not its current projected curve. Using it for linked display/selection would misrepresent the CAD model after source edits.
2. \`buildProfileScene()\` currently calls \`PartDocument::evaluateProfile()\`. PG-01B deliberately fails closed on Profile intent referencing linked geometry there; without a **read-only effective-Sketch** presentation route, even a valid associated Profile can be invisible in Sketch editing.
3. Existing Sketch interaction/picking/snapping/edit paths must be checked for authored-seed reads. A UI tool must not merely draw correct linked geometry and then permit snaps, relation inputs, measurements, grips or edits against stale seed.
4. \`PartViewportController::selectedMaterialEdgeReferences()\`, \`setBodyTopologyEdgeDraftMode()\` and \`setBodyTopologyToolStage()\` already provide a typed same-Part material-Edge selection mechanism, with runtime generation and stage validation. Reuse that machinery instead of persisting Viewer tokens or introducing a parallel picker.
5. PG-01B already owns \`CreateProjectedSketchEdgesCommand\`, \`BreakProjectedEdgeLinkCommand\`, stable target EntityIds, atomic batch/Undo and exact \`evaluateEffectiveSketchProjection\`. UI is an adapter; no second command path or Part owner.

## 3. Proposed D2-A — one active runtime tool + source stage

- Only an open active Part \`DocumentSession\` in a valid Sketch edit context can start the tool. On activation, stage a **runtime-only** source set; explicitly show the selected upstream \`BodyStageRef\` and strict \`MaterialEdgeReference\` meaning. Existing stage-scoped Body picker can show the current legally available stage. It must not silently search the final Body or choose a different stage when the current one is not admissible.
- Users pick one or many strict material Edges; all must belong to the displayed same-Part source stage and fresh presentation generation. Duplicates are visibly rejected/deduplicated without creating extra entities. Invalid/representation-artifact/ambiguous Edge input fails visibly.
- The right panel shows source count/list, **Regular** (default) or **Construction**, explicit diagnostics, Finish and Cancel. Source selection is not a durable mutation. The tool may display a preview built exclusively from current exact curves.
- Before Finish re-resolve current DocumentRevision, SketchId, support Frame, stage, semantic Edge references and all batch projections through the already accepted PG-01B command. **All-or-nothing** for Edge batch. One successful Finish = one transaction, one Undo; zero valid items, rejection or Cancel = zero authored change/Undo.
- On Esc, document switch, Sketch Finish, provider loss or tool replacement: clear transient source set, preview, tool pick mode and its renderer tokens. No stale Finish after context change. Keyboard Command Line remains one workspace-global adapter as in ADR-0011; no speculative command grammar is added.

## 4. Proposed D2-B — one current effective Sketch for presentation and interaction

- The same revision-bound PG-01B effective Sketch model used by Profile/Extrude/Revolve is the **read-only source for displayed linked curves**, linked hover/selection, snapping and inspection where those operations are supported. It also drives currently valid Profile fill/picking. Never use authored seed as a substitute for unresolved linked geometry.
- The authoring SketchModel remains unchanged during UI reads. Stable SketchId/EntityId bind temporary presentation tokens to semantic identities. A broken linked target remains durable and inspectable via status/Properties; it is not falsely shown/snapped as a valid curve. Unaffected unlinked geometry and Profiles remain usable.
- All affected interactive edit paths either use current evaluated geometry safely or **reject linked-controlled edits** with a typed/visible reason. Linked grips do not imply free endpoint/center drag. Existing ordinary unlinked editing behavior must remain intact.
- Derivation failure is not repaired by replaying a previous scene. Presentation/provider failure uses existing degraded-state diagnostics/retry semantics without rolling back successful CAD commands.

## 5. Proposed D2-C — minimal neutral Viewer visual contract

A linked Sketch entity must be visibly distinguishable from an unlinked authored entity **independently of Regular/Construction role**, with a linked indicator and a typed status in right-side Properties. The current neutral \`viewer::SketchLinePresentation\` / \`SketchCurvePresentation\` carry construction role but no distinct link flag.

Preferred bounded option: add a presentation-only \`linked\` role/flag (or one equivalently narrow typed style) to those neutral Sketch presentation items; adapt Qt/OCCT rendering to a visibly distinct style while preserving geometry, picking and construction styling. This is a **public Viewer contract change (D2)** and needs explicit Owner approval; it must never become authored CAD identity. Do not add a generic styling framework or copy of Part semantics into Viewer.

Broken linked geometry may have a separate status indicator in Properties/tool diagnostics without inventing usable geometry. The actual visible color/style must remain consistent and discernible with selection, hover and Construction.

## 6. Proposed D2-D — Break Link and lifecycle in one UX

- When a linked target is selected, right-side Properties show \`Linked\`, source description, current \`Resolved/Broken\` reason and explicit **Break Link** and Delete.
- Break Link dispatches the existing PG-01B command against **current** resolution; it freezes the latest exact projected curve into authored geometry, retains EntityId/role/compatible references, removes the binding, and is one Undo. Broken links cannot be detached from stale seed; offer typed diagnostic and Delete/repair guidance instead.
- Save/Close/Reopen, Undo/Redo, upstream edits, source suppression and support changes rebuild display from semantic intent and current provider; they do not reuse previous viewer identity.
- Do not promise a general constraint-solver repair, Face membership Refresh or Part Feature edit changes in this package.

## 7. Proposed mandatory evidence and Owner gate

1. Native Windows Workbench: real OCCT material Edge pick in actual Sketch edit, source-stage display, single and multiple sources, Regular/Construction, exact staged preview, Finish and Cancel.
2. Verify one accepted batch/Undo, stable EntityIds across Undo/Redo/Save/Close/Reopen with fresh Kernel, associated Profile/Extrude/Revolve dependency path. A source change changes **displayed linked geometry** and valid Profile presentation, not only downstream solid.
3. Negative: stale DocumentRevision, changed pick generation/stage, missing/suppressed source, ambiguous/representation-only Edge, unsupported/degenerate exact curve, duplicate source, invalid Sketch frame, provider failure, interrupted tool; zero partial mutation and no false current geometry.
4. Selection/snapping/Properties/Break Link/Delete work on linked and unlinked geometry without stale-seed leaks; zoom/DPI and ordinary existing Sketch/Body tool interactions unaffected. Confirm provider failure/presentation recovery.
5. Internal as-built, bilingual PL/EN product documentation and regenerated offline Browser; Windows FOCUSED/FAST/one exact-head FULL with explicit failures recorded, then **Owner practical Windows PASS** before final merge.

**Known CI dependency:** [#302](https://github.com/MechanicalCave/SimpleSolid2/issues/302) identifies FAST CTest/aggregate mismatch (25 missing build dependencies). No CI/CMake remediation is authorized by PG-01C. If FAST remains untrustworthy, stop for a separate bounded Owner decision rather than relabeling RED as PASS.

## 8. Decision request / stop conditions

Owner approval is requested for **D2-A through D2-D** and the separate bounded PG-01C Work Contract. Explicitly confirm the small Viewer presentation-only contract extension, current effective Sketch as UI/read/interaction authority, and use of existing strict material-edge tool-stage picking.

STOP for Owner if a wider public Viewer/Kernel/Selection API, new durable identity or persistence schema, new solver/evaluator ownership, Face membership semantics, loosening strict source resolution, or unbounded Sketch interaction rewrite becomes necessary. Do not expand scope silently.

Approval of this proposal alone is not an implementation activation: the separately approved Work Contract must be installed in \`work/ACTIVE.yaml\` in a dedicated governance activation commit, validated before changing product source.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: this file is a design-only Owner D2 proposal; PG-01C implementation will require both internal and PL/EN Product docs.
