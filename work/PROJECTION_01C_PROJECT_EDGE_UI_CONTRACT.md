# PG-01C — One Project Geometry tool: Edge authoring & linked Sketch UX Work Contract 

**Status:** OWNER D2-A–D2-E + WORK CONTRACT ACCEPTED 2026-10-09 — authorized only after dedicated ACTIVE activation commit
**Prepared:** 2026-10-09
**Baseline:** main `8d78320daca8a72a1431fbece49296c3bfb83aec` after Owner PG-01B FINAL PASS and [PR #301](https://github.com/MechanicalCave/SimpleSolid2/pull/301) merge
**Program:** `work/PART_MODELING_V1_ROADMAP.md` v1.30, Projection before PM-06
**Prerequisites:** Projection 00A, PG-01A, PG-01B completed; same-Part associativity D2 approved, Assembly §7.3 snapshot unchanged
**D2 authority:** `work/PROJECTION_01C_PROJECT_EDGE_UI_D2_PROPOSAL.md` D2-A/B/C/D/E explicitly approved by Owner 2026-10-09 (`zatwierdzam PG-01C - kontynuuj pracę`), following Owner requirement for full Workbench parity
**Activation rule:** `work/ACTIVE.yaml` must point to THIS accepted contract in a distinct activation commit before any production code mutation. Owner accepts this bounded contract, not PG-01D/PG-01E/PM-06.
**Known independent CI defect:** [#302](https://github.com/MechanicalCave/SimpleSolid2/issues/302)

## 1. Goal

Ship **one** real Part-hosted Sketch **Project Geometry / Rzutuj geometrię** user tool for existing exact, strict, same-Part material **Edges**, building on the PG-01B headless Commands/Part binding/effective Sketch. All working controls and diagnostics are in the contextual **right-hand Operations panel**. Project Geometry launcher remains in existing Sketch toolbar.

The workflow must be usable end-to-end: pick one/many Edges, select Regular/Construction, stage exact derived preview, Finish once or Cancel, see colored/identified linked geometry in Sketch and Properties, Break Link / Delete, Undo/Redo, Save/Close/Reopen and rebuild of dependent Profile/Features from current source.

## 2. In scope

- One runtime tool state in current Sketch edit and active Part session. Reuse current Body tool-stage Edge pick selection and its strict `MaterialEdgeReference` translation, same-revision/stage/generation guards and duplicate detection.
- Right Operations tool controls: clear active selection, source stage/context, source list/count, role Regular (default) / Construction, live typed validation, Finish, Cancel; no authored mutation before Finish.
- Semantic Finish through existing `CreateProjectedSketchEdgesCommand` with exact current revision and one atomic batch/one Undo, no UI/Viewer direct Part mutation. `BreakProjectedEdgeLinkCommand` and existing Delete/role commands are the only durable actions for linked targets.
- Read-only effective Sketch integration for existing linked entities in 3D Sketch scene, selection, supported snap/inspection, valid Profile fill/selection and right-side Properties; no authored-seed fallback for unresolved source. Use PG-01B `evaluateEffectiveSketchProjection` and same-revision Part evaluation.
- Narrow neutral Viewer linked visual styling / provider adapter **as approved by Owner D2-C**. Display Regular/Construction orthogonally to linked state.
- Tool/editor lifecycle on Esc/Finish/Cancel/Undo/Redo/document change/Sketch exit/provider failure; stale visual or pending pick state must be rejected/cleared.
- Focused Qt/OCCT native regressions, exact-CAD semantic tests and docs.

## 3. Explicitly out of scope

- **PG-01D:** planar Face pick, loop traversal, holes, partial per-edge Unsupported skips, source Face identity policy, Face boundary membership refresh.
- **PG-01E:** aggregate Project Geometry program closeout and additional product-wide performance milestones beyond mandatory PG-01C lifecycle.
- Future Assembly-context projection, cross-Part links, Drawing, auto-bind/rebind, similarity/proximity resolution, automated closure/gap healing, spline/ellipse approximations, unrestricted constraint solver/Sketch structural editor redesign.
- New native schema (current v15), new MaterialEdgeReference semantics, edits to Foundation/accepted ADRs without Owner decision, PM-06.
- CI build/test registry repair [#302], workflow/classifier policy changes or test weakening.

## 4. Architecture, identity and safety

```text
Qt Workbench / right Operations panel / Viewer pick transport
  -> stage only current runtime semantic Edge candidates
  -> exact Part same-revision source + support-frame resolution
  -> PG-01B DocumentSession CreateProjectedSketchEdgesCommand
  -> Validation -> Transaction -> owning PartDocument
  -> pure effective Sketch -> Profile/Feature evaluation
  -> neutral Viewer scenes / Properties / diagnostics
```

- Part owns the only durable `EntityId -> MaterialEdgeReference` mapping and role. Viewer runtime generation/token, Qt widget/Tree identity and currently sampled curve are **never persisted as source identity**.
- One tool state; no duplicate pipeline hidden in Qt or command-line transport. Any keyboard action uses the existing global CAD Input semantic routing policy.
- Source must be a strict material Edge at an explicitly displayed and current legal Body stage, from the **same Part**; stage ordering and cycles are checked by PG-01B. No selection from arbitrary final Body when declared stage differs.
- On every redraw and user-facing query, linked curves come only from current resolved effective Sketch; missing/ambiguous/unsupported/broken links do not present/snaps/commit stale authored seed. Unaffected entities and regions remain usable.
- Properties report linked source, role and typed current status; Break Link succeeds only on latest resolved geometry, keeps EntityId and role; broken Break Link is mutation-free failure.
- Finish revalidates exact current source and document context; no partial batch commits. Transient preview failures do not dirty the Document. No tool stage's Viewer tokens outlive their current generation.
- Presentation setter failures are surfaced/recoverable without undoing a successfully committed Part command.

## 4.1 Mandatory native tool interaction and Command Line parity

**Owner D2-E approved 2026-10-09.** "Full tool" means parity with accepted Workbench patterns, not only happy-path buttons. Integrate Project Geometry with the existing Sketch toolbar (launcher), contextual **right Operations panel** (options/diagnostics), global Command Line (tool activation and control), native 3D Viewport (strict stage-scoped Edge picking), Workbench Status, Sketch Select fallback, and existing Undo/Redo/Save/shortcut/focus arbitration. There must be **one** tool state machine and one semantic transaction path shared by all adapters.

**Exact minimum tool-local grammar:** top-level `PROJECT` (optionally `PROJECTGEOMETRY`, subject to no keyword collision); in active tool `REGULAR`, `CONSTRUCTION`, `REMOVE` primary staged source, `CLEAR` all staged sources, `FINISH`, `CANCEL`. Global Command Line token parsing is **context-first** as in ADR-0011; command activation and each option must update the same right panel state. No direct durable source ID typing or speculative global CAD command grammar. Expose contextual prompt and rejected-token diagnostic; rejected input leaves tool/Document unchanged and empties submitted buffer.

**Esc/Enter/Cancel contract:**
- First Esc with a non-empty Command Line buffer clears **only that buffer**, not the Project Geometry staging area; next Esc reaches tool cancellation. With empty buffer, cancel the innermost unfinished source pick/request/preview stage first, then exit the non-Select tool on a further Esc; absent nested stage, exit immediately. Never leave pending runtime Edge capture or an undefined tool.
- Explicit right-panel Cancel and `CANCEL` exit immediately and restore Select; do not save partial selections, dirty the Document or create Undo. Full cancellation must also drop stale preview, hover and generation-bound picks.
- Right-panel Finish, `FINISH`, and **empty-buffer Enter from the CAD viewport** invoke the same commit path while Project Geometry owns input; invalid/empty Finish leaves session active with a corrective diagnostic and zero mutation; successful Finish returns to Select with one Undo entry.
- Preserve accepted Space/Repeat Last Command behavior when Project Geometry is **not** the active owner. Project Geometry must not turn Space into global Finish or allow an empty-buffer Enter to repeat an unrelated Sketch command.
- Typing from the CAD viewport must not force Qt focus into Command Line; a focused editable widget, text composition/IME, modal editor, and platform/application shortcut (Ctrl+S/Ctrl+Z etc.) outrank CAD capture. No input to inactive/hidden Document.
- Toolbar start, Command Line start, tool switch, Sketch Finish, document switch/close, Workspace navigation, Undo/Redo, provider loss and revision change must synchronize/deactivate/revalidate staged picks and input generations. No stale Finish, cross-document token consumption, ghost preview or accidental persistent mutation.

**User-visible UI consistency:** normal existing toolbar icon placement/grouping, contextual Operations layout, disabled/enabled Finish states, role defaults/indicators, selection/hover style, cursor, prompts, current stage and source count, diagnostic rendering and return to Select must match adjacent Sketch and Part Fillet/Chamfer tools. No modal-only fallback, detached second command console or hidden context; the tool remains discoverable and consistent across PL/EN docs.

## 5. Permitted bounded file scope AFTER formal activation

- `src/ui/cad_workbench.cpp/.hpp`: one tool and right panel, command dispatch, status and lifecycle.
- `src/ui/part_viewport_controller.cpp/.hpp`: reuse stage-scoped material Edge picking; read-only current effective Sketch / linked scene and Profile scene projection, transient binding and diagnostics.
- `src/ui/part_sketch_interaction_controller.cpp/.hpp`: only necessary linked-aware read/interaction/selection/typed edit protection; no generalized new tool architecture.
- `src/viewer/**`: only approved D2-C narrow, provider-neutral transient linked-style surface and Qt/OCCT presentation/pick parity. Stop before public contract expansion beyond D2-C.
- `src/application/**` and `src/part/**`: **only** bounded read-only reuse/factor of existing PG-01B current effective projection and typed diagnostics if demonstrably necessary; any new persistence/identity/Feature semantic mutation needs Owner D2 STOP.
- `tests/**`: bounded existing Qt/OCCT Workbench/Sketch/Part regressions. New test/CMake registration only when intrinsically justified, separately reviewed for [#302](https://github.com/MechanicalCave/SimpleSolid2/issues/302) consistency.
- `docs/internal/**`, `docs/product/pl/**`, `docs/product/en/**`, canonical `docs/browser/index.html` **via `.\ss2.ps1 docs` only**, `work/**` evidence and acceptance.

No edits to `main` except through approved PR merge.

## 6. Planned bounded execution stages

**C0 — baseline/proofs:** audit existing neighboring Workbench tool UX, Command Line routing/precedence, Esc hierarchy, key event focus arbitration and all current authored-seed reads in Sketch rendering, Profile rendering, selection, OSNAP/measurement, Properties and interaction; add RED focused evidence proving displayed link/visible Profile differs from current effective geometry on an upstream source edit. No speculative Qt refactor.

**C1 — single tool state:** toolbar **and Command Line PROJECT** activation within Sketch edit, right-side role/options/status, stage-scoped material Edge collection, dedup, one semantic input bridge, `REGULAR/CONSTRUCTION/REMOVE/CLEAR/FINISH/CANCEL`, hierarchical Esc, eligible Enter, focus priority and context cleanup. Native real Body Edge picking; typed rejected-pick status. Reuse existing Workbench grammar conventions and do not introduce a second tool dispatcher.

**C2 — effective current presentation:** consume PG-01B derived Sketch model across Sketch scene, eligible Profile scene, selection/snapping/Properties; use the narrow D2-C linked visual flag. Unresolved source is not drawn as a valid linked curve. Preserve semantic EntityId/role and unlinked behavior.

**C3 — lifecycle:** stable preview and one Finish from panel, global Command Line or eligible viewport Enter, one Undo; Break Link and Delete via headless semantic Commands, Undo/Redo, upstream source edit/suppress/missing recovery, Save/Close/Reopen, current support frame and provider loss. No last-good preview or stale token command authority.

**C4 — evidence/docs/closeout:** focused native UI tests including Command Line and Esc/focus parity matrix, FAST, final exact-head Windows FULL and Owner practical Windows walkthrough; internal current-as-built and bilingual Product docs parity, Browser regeneration and `ss2 verify` pass. Retain all RED-to-GREEN evidence and independent CI #302.

Each stage is strictly within this Work Contract after Owner D2 + contract approval and dedicated `ACTIVE.yaml` activation. FOCUSED success alone does not close PG-01C.

## 7. Mandatory acceptance cases

1. Start from a real Part with upstream Extrude/other supported Feature; enter later Sketch; launch Project Geometry once; pick two distinct **material** Edges on currently declared Body stage; chosen role reflected in the right panel and preview; Finish creates two linked EntityIds, **one** Undo entry.
2. Undo removes both; Redo restores their identities, source bindings and role. Cancel, zero-selection Finish, duplicate source, rejected source, and stale context produce zero mutation and zero Undo entries.
3. Regular linked geometry may participate in valid Profile/Extrude/Revolve; Construction linked geometry stays reference-only. Toggling role is an explicit semantic command, not a link detach.
4. Editing upstream dimensions changes projected **Sketch display**, eligible derived Profile fill, snap/selection queries and dependent solid, with no authored seed mutation. The same test uses deliberately different persisted seed values to prove no stale-source shortcut.
5. Clicking a linked entity shows distinct linked indication, source, role and current status; attempts to drag/trim/extend/duplicate linked geometry fail closed where not provider-aware, without harming normal unlinked edits.
6. Break Link keeps latest resolved curve, original EntityId and role; Undo restores association, Redo freezes again. Broken linked source cannot Break Link from old seed. Explicit Delete removes entity+binding atomically.
7. Material Edge selection rejects representation partition/unsupported/nonmaterial, stale generation and stage mismatch; preview provider failure, Edit Context switch, Sketch exit and document close clear picks/overlays without changing authored model.
8. True native Windows Save/Close/Reopen with new OCCT provider and upstream edited source reprojections maintain exact semantic source. GUI is stable under source suppression/unresolved geometry and restoration.
9. Existing Sketch tools, selecting normal Body Edges outside this tool, cursor routing, zoom/DPI and Command Line are regression-green. Failed Viewer update surfaces degraded state and recovers from current authoritative model.
10. **Full native command/input acceptance matrix (mandatory):** launch by toolbar and typed `PROJECT` arrives at identical state; `REGULAR/CONSTRUCTION/REMOVE/CLEAR` both reflect in Operations; finish via button, `FINISH`, and eligible empty-buffer viewport Enter creates the same atomic transaction; Cancel button, `CANCEL`, and hierarchical Esc revert to Select without authorship change. Text-buffer first-Esc clearing must not erase staged Edges. Invalid Command Line text shows a diagnostic, clears only submitted token and preserves tool. Test with focus in QLineEdit/property field, IME input, global Ctrl+S/Ctrl+Z, Sketch Space/Repeat Last Command, and active vs inactive document. Verify that tool switch, Sketch exit, Undo/Redo, close, stale provider generation or failed Finish leaves no ghost selection/pick capture and that all three presentations of status agree.
11. Docs PL/EN parity, generated Browser freshness, explicit exact-head Windows FULL and Owner manual Windows PASS before merge. If FAST is red or cannot certify source binaries due to [#302](https://github.com/MechanicalCave/SimpleSolid2/issues/302), record failed evidence and require an explicit Owner maintenance/exception decision.

## 8. Known risks / STOP (D2 or D3)

- **Single point of failure:** some UI read path still consumes stored seed instead of effective Sketch after source change; **cannot ship** an apparently valid linked curve/profile in this state.
- Existing Body Edge pick/Sketch input routing cannot coexist without expanding public Viewer/Selection protocols beyond D2-C.
- New persisted identity/reference, schema change or changed Profile/Feature semantics would be necessary.
- Missing/ambiguous source gets replaced by similarity/proximity fallback, stale provider token or old displayed geometry.
- Fix expands into general Sketch solver/constraints, Face capture, Drawing/Assembly, CI infrastructure or global performance architecture.

STOP, prepare bounded amendment, request Owner D2/D3 rather than silently expanding.

## 9. Owner acceptance / activation gate

**Owner final D2 and contract acceptance: 2026-10-09 — `zatwierdzam PG-01C - kontynuuj pracę`.** This approves design decisions D2-A/B/C/D/E, including bounded transient Viewer style, existing stage-scoped picker, effective Sketch as the read/presentation authority, and complete Esc/Enter/Cancel/Command Line interaction parity. It authorizes this Work Contract, not a blanket application rewrite or skipped tests.

A distinct governance activation commit must update `work/ACTIVE.yaml -> active_work` to THIS contract and pass the appropriate gate **before** source changes on a feature branch. PG-01D/E and PM-06 remain gated; final PG-01C product completion and merge require exact-head Windows evidence and separate Owner practical PASS.

## 10. Owner-approved bounded amendment — 2026-10-09 (D2-K/T/P/B)

**Decision:** Owner explicitly approved all four bounded extensions in response to the Sketch 5 / linked Circle / multi-selection findings: `Zatwierdzam - mamy wyniki - kontynuuj`. This is permission to implement and verify, **not** final PG-01C PASS or authorization to merge.

1. **D2-K — exact source topology:** expand this contract to `src/kernel_occt/solid_modeling_kernel.cpp` and strictly necessary related native tests. For a supported OCCT Line Edge, use its **actual oriented TopoDS_Vertex** endpoint coordinates to ensure genuinely shared source vertices project identically. No proximity-based vertex identification, global Sketch tolerance relaxation, broad OCCT healing, new persisted geometry/reference identity, or change to Circle/Arc support policy. A native after-Chamfer exact-region regression is required. Missing/degenerate source fails closed.
2. **D2-T — Document Tree:** expand to `src/ui/part_document_tree_controller.cpp/.hpp`. Display linked Profile validity from the same revision-bound effective geometry used by Viewer and Properties. Never treat saved linked geometry as a current fallback.
3. **D2-P — Profile Create/Edit:** expand to `src/application/document_session.cpp`, `src/application/include/simplesolid2/application/document_session.hpp`, and narrow Sketch interaction call sites. Semantic Command execution must revalidate against a current same-Part source provider and document revision, reject provider loss or unresolved links without durable mutation, retain stable ProfileId/RegionIntent/EntityIds and existing v15 schema. Existing authored-only commands must not become a seed-based linked bypass.
4. **D2-B — atomic batch Break Link:** expand the existing semantic command and Sketch Select UI to support multiple selected linked EntityIds, validate all against the same current provider/revision and commit one all-or-nothing mutation/Undo. Preserve all current geometry, roles and existing Profile identity. Duplicate, mixed unlinked and missing sources reject the entire set; no loop of partial single-target commits.

**Required verification:** focused RED→GREEN native OCCT Chamfer/region, linked Line/Circle and mixed Profile status, Create/Edit current provider and loss, atomic multi-Break Link/Undo/Redo/Save/Reopen, exact-head Windows FULL and separate Owner practical re-test of `Part006.ss2part` Sketch 5. No private CAD fixture is added to the repository. PL/EN canonical docs and deterministic Browser remain mandatory.

**Unchanged STOP:** PG-01D Face, PG-01E, PM-06, independent CI issue #302, new CAD identity, persisted schema changes, broad modeling tolerance policy and automatic endpoint healing are not authorized. Existing ADR-0014 modeling-semantic compatibility rules remain in effect.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: new linked Sketch presentation and Project Geometry/Break Link end-user command workflows, diagnostics and interaction/lifecycle behavior. Provide canonical bilingual PL/EN docs, regenerate Browser, and run `ss2 verify`.
