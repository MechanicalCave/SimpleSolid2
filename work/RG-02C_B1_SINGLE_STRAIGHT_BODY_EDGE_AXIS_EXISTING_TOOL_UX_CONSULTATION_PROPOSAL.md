# RG-02C/B1 — Axis along one straight Body Edge: existing-Workbench UX consultation (D2 PROPOSAL)

**Status:** **OWNER UX DIRECTION RECORDED FOR WORK FEATURES AXIS BUTTON, OPERATIONS EDGE CONSTRUCTOR, REFERENCE GEOMETRY TREE AND FULL LIFECYCLE (2026-10-15); remaining detailed flows remain PROPOSED FOR EXPLICIT OWNER UX SIGNOFF.** NOT AN EXECUTABLE WORK CONTRACT, NOT ACTIVE, NOT MERGED, NO UI MUTATION AUTHORITY.
**Date:** 2026-10-15.
**Protected-main baseline:** `38b0c8b274d47548647fc7c27e9a7884fd85ff20` (PR #408 A0 read-only evidence and separate Owner A0 technical FINAL PASS; explicit real-native-Edge topological orientation reversal fixture **GAP**, not retrospectively PASS).
**Authority:** Constitution + Foundation 1.0 FROZEN, Architecture/accepted ADRs, Part Modeling v1.33 §6/§20B, RG-02 parent Owner-accepted direction `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` §2.3(1), §3 mandatory prior UX Owner signoff, completed PM-04 Sketch-Line Axis, existing PM-03/S4B single Datum Plane UI, DOC-01.
**Owner constraints:** `UI musi byś spójne z aplikacją i istniejącym "datum"`; `nie powinnismy duplikować testów i istniejącej logiki`. **Latest Owner UX clarification (2026-10-15, accompanied by screenshots of Work Features and Datum Plane Operations):** `na pasku narzędziowym w kategorii "work features" nowy przycisk Axis. w operations analogicznie lista constructor i pozycja "edge" - pełny "cykl zycia" , w tree umieszczamy w reference geometry`. The visible button, exact **Edge** constructor label, full lifecycle, and canonical Reference Geometry Tree placement are now **Owner-directed design requirements**. Other detailed UX choices still require separate explicit Owner design approval; do not equate authorization to prepare documents with an executable contract.
**Target:** review the **FIRST** prospective Axis constructor along **one strict, straight material Body Edge at a declared Body history stage**, not all four later RG-02C constructors. This artifact is a design proposal only.

## 1. Grounded current UI versus Owner-requested UI — avoid pretending it already exists

- **Real current toolbar:** `src/ui/cad_workbench.cpp` creates `datum_plane_button_` (`datumPlaneToolButton`) and the `Work Features:` group label in Part's one-row toolbar. The screenshot supplied by Owner visibly shows **Datum Plane** there and **no Axis button**. `axis_button_` appears as a field in `cad_workbench.hpp` and in state-sync code, but **a corresponding Part toolbar QPushButton is not created in the inspected `cad_workbench.cpp` path**. The desired Axis toolbar button is therefore **new visible UI**, not an already delivered toolbar control.
- **Reusable current Axis machinery:** `axisOperationsWidget`, `axisSourceLabel`, `axisResultLabel`, `axisFinishButton`, `axisCancelButton`, `startAxisTool`, `startAxisEdit`, `stageAxisSource`, `refreshAxisEvaluation`, `syncAxisUi`, `finishAxis`, `cancelAxis`, `submitAxisCadInput` and `application::AxisDraft` already exist for authored **Sketch Line** sources. The Axis Finish/Cancel controls already use the **existing sticky Operations footer**. Reuse and extend them — do NOT add a second Axis runtime/draft/commit pipeline.
- **Real current Datum Plane precedent:** one toolbar `Datum Plane` entry in **Work Features**, a `Constructor` selector (`Offset`, `Edge + Point`, `3 Points`), source/result status, preview, operations footer, Edit, semantic commands, shared Tree selection/Show-Hide. Existing keyboard/Command Line and panel lifecycle are precedent, not a new framework to clone.
- **Real current Tree:** `Reference Geometry` currently owns **Datum Plane rows**. Existing authored **Sketch-Line Axis rows are currently children of their source Sketch**. `Origin` owns built-in X/Y/Z axes. Owner's new requested canonical Tree layout requires a **UI presentation migration** of authored Sketch-Line Axis rows to `Reference Geometry` alongside new Edge Axis rows, while leaving model identity and sketch-line source references untouched. One AxisId = one Tree row; never show it under both parents.
- The real right column is **Properties / Operations** with 40/60 split, independently scrolling or tabs on insufficient height; footer buttons remain fixed. Toolbars are single-row and context-safe. Preserve these behaviors for Axis.

**Evidence boundary:** Owner supplied real screenshots of (a) the Work Features toolbar with `Datum Plane` and (b) Datum Plane's `Operations` constructor dropdown, Offset and Reverse. Those screenshots are a visual UX reference, **not screenshots of an implemented Axis Edge constructor**.

## 2. Owner-directed narrow tool interaction; detailed semantics remain proposed for separate UX signoff

**Toolbar — Owner direction confirmed:** Add one **visible `Axis` button immediately alongside `Datum Plane` inside the existing `Work Features` category**, honoring the same toolbar grouping, focus, compact-window and active-tool checked/disabled grammar. This must be a new physical button wired to the **existing** `startAxisTool` / Edit/Finish/Cancel chain (currently no corresponding visible Part toolbar button), not a second independently implemented tool, a new `Datum Axis` command or a Sketch-toolbar duplicate. No other toolbar relocation.

**Operations — Owner direction confirmed:** The **same `axisOperationsWidget`** gains a `Constructor` dropdown analogous to Datum Plane, with precisely `Sketch Line` (existing source, default for compatibility) and **`Edge`** (new straight material Body Edge mode; exact Owner-requested displayed word, **not** `Body Edge` or `Edge + Point`). Switching constructor clears incompatible *transient* pick leases and preview, not existing Part or Undo history; Edit an existing Axis preloads its true constructor. Existing sketch-line designation/check/edit/CLI semantics must remain intact.

**One Edge source:** With `Constructor: Edge`, click **one** strictly `Resolved` native **straight material Body Edge** at one explicit `BodyStageRef`. Operations shows `Source / Edge` with a replace/select affordance, `Stage`, one status/diagnostic readout. Do not require a second point click, create a generic Datum Point, accept `MID/CENTER`, screen XYZ, tessellated visual straightness, runtime OCCT token, seam/closed edge, a nonmaterial edge or geometrically similar replacement. Reuse `MaterialEdgeReference`, pre-existing Q0/Q2 strict resolver and native exact provider queries, subject to a separately accepted production contract.

**Preview:** Before a strict Edge source, no preview and `Finish Axis` disabled. With source Resolved, preview one **finite line cue of an infinite unoriented Axis** in the viewport and highlight the selected Edge using **existing Axis/Datum presentation grammar**. No Axis-authored `Reverse` for `Edge`; the future Revolve consumer owns signed rotation. Hover alone never authors, and on Missing/Ambiguous/Unsupported/Blocked/Stale, preview disappears immediately (no last-good line).

**Complete lifecycle — Owner direction confirmed as scope requirement, proposed exact behavior:** `Create`, `Edit/Replace Edge`, `Finish`, `Cancel/Esc`, deletion, visibility `Show/Hide`, selection, inspection in Properties, Undo/Redo, Save/Close/Reopen and cold rebuild, stage/Feature dependency checking, source repair after upstream changes and downstream Revolve failure/recovery **must all be included** in any later bounded production contract and real tests. `Finish` revalidates source, `DocumentRevision` and dependency order through **existing Part commands/transactions** (one commit = one Undo); Cancel, hover and rejected invalid picks create **zero** authored mutation and Undo entries. Edit preserves `AxisId`, authored visibility and dependent `AxisReference` identity; no feature silently rebinds to the closest Edge. Delete follows current dependency-safe existing Axis semantics; dependents retain repairable authored intent, not a guessed replacement.

**Tree — Owner direction confirmed:** **All Part-owned authored Axis objects** (both Sketch Line and new Edge constructors) appear **once** under existing `Reference Geometry`, alongside Datum Planes. `Origin X/Y/Z` stay under `Origin`. For Sketch-Line Axis, preserve source `SketchId/EntityId` and native persistence but move the **Tree presentation row** from Sketch to Reference Geometry. Mixed group `Show/Hide` now needs consistent behavior covering Datum and authored Axis through existing transactions, ideally **one atomic Undo**; source Sketch visibility never suppresses Axis evaluation. Tree keyboard/context Edit/Delete/Show/Hide, selection synchronization, warnings/diagnostics, expansion and Properties must remain coherent. Never duplicate AxisId Tree rows.

**Stage and dependencies:** A strict Edge source persists an explicit semantic `BodyStageRef`, not the currently visible last Body shape; subsequent Features do not rebind it. UI shows a readable stage label, current status, repair action and dependency blockers. Missing source keeps the authored Axis selectable/editable, but prevents valid evaluation/preview and downstream consumption. Producer Feature replacement with equal geometry must fail closed.

**Revolve and history:** `AxisReference` to a real `AxisId` remains authoritative for dependent Revolve, regardless of `Sketch Line` vs `Edge` constructor, but **native v18 durable source variant, migration, stage floors, Revolve dependency rules and signed direction treatment are NOT yet specified or authorized by A0**. These need a separate narrow Owner D2 Work Contract before a product code change. The accepted A0 evidence GAP for native topological Edge orientation reversal is **not** proof of product behavior.

**CAD Command Line:** Reuse the current Axis input context and `SOURCE`, `FINISH`, `CANCEL` grammar; propose locally-scoped `EDGE` and `SKETCHLINE` mode tokens (exact proposed tokens, **not delivered yet**), consistent with the UI labels. No global `EDGE` shortcut, world XYZ text source, independent CAD CLI draft or ambiguous cross-tool routing. Retain owning Enter/Esc and focus semantics; final token names require distinct Owner UX signoff.

## 3. Reviewable wireframes — proposed UI, aligned with Owner screenshots

### W1 — Part Work Features toolbar and Axis Operations before source

```text
One-row Part toolbar:
  Sketch:       Work Features:            Create:             Modify:
  [Sketch]      [Datum Plane] [Axis]       [Extrude] [Revolve] [Fillet] [Chamfer]
                                   ^ NEW visible Axis button (existing controller reused)

Document Tree                     Viewport                Right-side Operations
  Origin                          [current Part Body]     Axis operation
    X Axis                                             Constructor  [Edge     v]
    Y Axis                                             Source / Edge  [Select...]
    Z Axis                                             Stage           [not set]
  Reference Geometry                                     Select one straight Edge
    Datum Plane 1                                       No preview; Finish disabled
    Axis 2  (future)                                    --------------------------
                                                        [Finish Axis — disabled] [Cancel]
```

Note: `Sketch Line` is the default Axis constructor for existing documents/selection; `Edge` is chosen explicitly. Property panel and existing responsive sticky footer remain unchanged. The future Axis tree row shown here is not committed at this stage.

### W2 — Constructor Edge, one strict current-stage Body Edge selected

```text
Document Tree                   3D Viewport              Operations > Axis
  Origin                        [selected material       Constructor  [Edge v]
  Reference Geometry             straight Edge]          Source Edge  [Extrude001 / Edge ...]
    Datum Plane 1               [finite Axis cue,        Stage        [after Extrude001]
                                 no direction arrow]     Status       [Resolved]
                                                         [Finish Axis] [Cancel]
```

One click stages one semantic straight Edge. The finite cue is a disposable preview of an infinite **unoriented** line. Hover alone, invalid source and Cancel never change Part data or Undo.

### W3 — After Finish; full Tree/Properties/Edit/repair lifecycle

```text
Document Tree                   Properties > Axis       Operations > Edit Axis
  Origin                        Axis 7                  Constructor  [Edge v]
    X / Y / Z Axis              AxisId [stable]         Edge         [Missing] [Replace Edge]
  Reference Geometry            Constructor Edge        Stage        [after Extrude001]
    Datum Plane 1               Source Body/Stage       Status       [Missing — repair]
    Axis 4 (Sketch Line)        Status Missing          [Finish disabled] [Cancel]
    Axis 7 (Edge) [Missing]     Visibility Shown
  Sketch 2
    ... (NO duplicate Axis 4)
```

A failed Axis remains authored/editable with one `AxisId`, no stale preview, and downstream failure diagnostics. Tree `Edit Axis`, `Delete Axis`, `Show/Hide`, Undo/Redo, native Save/Reopen, source repair and dependent Revolve are part of the required future vertical-slice lifecycle; this image does **not** claim implementation.

## 4. Complete Owner walk-through / future automated and manual acceptance

| ID | Interaction | Required acceptance behavior |
| --- | --- | --- |
| UX-01 toolbar | Part → `Work Features` → visible `Axis` adjacent to `Datum Plane` | One button/handler and original single-row/responsive group behavior; existing Sketch/Datum untouched |
| UX-02 current constructor | Axis → default `Sketch Line`; select Line | Existing source/draft/Finish/CLI unchanged; AxisId belongs to Part |
| UX-03 new constructor | Axis → `Constructor: Edge` | One Edge source slot; no second click or separate Datum Plane tool |
| UX-04 native hover/pick | Hover/click strictly eligible straight material Edge | Hover/marker consistent with Datum/Project Geometry; stage/source/status visible; finite, unoriented preview only after click |
| UX-05 invalid source | Curved Arc, seam, unsupported/ambiguous/missing/stale/wrong stage Edge | Typed error, no preview/Finish, no fallback, no mutation or Undo |
| UX-06 create/commit | `Finish Axis`, Enter with valid Edge | Exactly one revision-bound semantic Axis transaction, stable AxisId, Undo one step |
| UX-07 Cancel/undo/redo | Cancel/Esc during draft; Undo/Redo after Finish | Cancel 0 changes; Undo removes/reverts only that operation, Redo restores stable authored intent |
| UX-08 Tree canonical placement | New Edge Axis and existing Sketch-Line Axis, plus Datum | **Both** authored Axis classes once under `Reference Geometry`; none duplicated under Sketch; built-in Origin axes stay Origin |
| UX-09 visibility | Axis row and mixed `Reference Geometry` group Show/Hide | Durable selection-aware visibility, atomic mixed group operation, Undo/Redo and Save/Reopen |
| UX-10 Edit/re-source | Tree → `Edit Axis` → `Replace Edge` → Finish/Cancel | Preserve AxisId, visibility and consumer links; one Undo on Finish, none on Cancel |
| UX-11 deletion/repair | Delete Axis used by Revolve; Undo/delete upstream Feature | No silent feature retargeting, dependent Revolve blocked with diagnostic, explicit repair/Undo path |
| UX-12 independent reopen | Save/Close/Reopen (fresh OCCT provider) | Same AxisId/source stage and display state reconstructed, or typed fail closed; no native token persisted |
| UX-13 direction | Native Edge orientation representation varies; Revolve reverse | Geometry line still mathematically unoriented; Revolve owns direction. Native topological reversal fixture still an A0 GAP and separate production D2 risk |
| UX-14 Command Line | `AXIS`, `EDGE`/`SKETCHLINE`, `SOURCE`, `FINISH`, `CANCEL` in Axis context | Same GUI draft and commands, no global grammar diversion, valid focus/Enter/Esc |
| UX-15 responsive / conflicts | Narrow toolbar/panel, Edit Feature/Sketch collision | Existing Work Features group layout and Properties/Operations sticky footer; incompatible edit fails closed |

**No UX-01–15 manual integration tests have been performed.** These are future acceptance scenarios of an unimplemented UI, **not** proof or current product documentation.

## 5. Proposed Axis-scoped CAD Command Line, not yet approved as concrete tokens

Current `application::submitAxisCadInput` already accepts `SOURCE`, `FINISH`, `CANCEL` and owning blank Enter. Preserve these and the existing root `AXIS` tool. Owner selected a **UI constructor label `Edge`**, but has **not explicitly approved command tokens**. Propose only when Axis owns input:

- `EDGE` → choose constructor `Edge` (strict straight Body material Edge source).
- `SKETCHLINE` → choose current `Sketch Line` constructor.
- `SOURCE` → stage current certified selection in the matching mode; never parse arbitrary XYZ.
- `FINISH` or owning Enter → same semantic validation/transaction as UI; `CANCEL`/owning Esc → discard draft.

No global `EDGE` command, no second draft/parser subsystem, no automatic selection from nearest visual topology and **no `REVERSE` input for unoriented Edge Axis**. Proposed exact tokens/focus precedence require a separate explicit UX decision.

## 6. Product/architecture STOP conditions — no implementation permission

A0 **did not** define a durable Body-Edge `PartAxis::source` variant, production Edge Axis evaluation, full `AxisDraft` revision-bound model, data-schema migration, dependency floors for Revolve, or a public Viewer acquisition contract. The native B-Rep **topological orientation reversal fixture** remains an explicitly accepted *A0-only* GAP. A future production package must decide whether that is a STOP requiring additional test, not silently treat it as verified.

The visible Work Features Axis button is **new UI**, even though the Axis handlers already exist. Moving existing Sketch-Line Axis **Tree presentation** from Sketch to Reference Geometry is also a user-visible behavior change requiring targeted native Qt regressions (selection, source navigation, Edit, group Show/Hide, stable AxisId, no duplicate row and cold reopen); existing source Sketch ownership and persistence must not change just to make the tree look tidy.

Full-lifecycle delivery must prove one committed transaction/Undo, revert/Redo, Cancel, source repair, Show/Hide mixed group, delete downstream fail-closed, native Save/Close/Reopen with a fresh provider and real viewport hover/click/Command Line behavior. No weakening or duplication of PM-04/Q0/Q2/Datum tests.

If source persistence needs new versioned native schema, Part command/public API, exact provider-neutral reference or extra test paths, STOP for **one narrow independently Owner-approved RG-02C/B1 production Work Contract**, exact allowlist and DOC-01 product/internal PL+EN/Browser updates before any edit.

Out of scope: through two arbitrary Points, cylinder axis, two-plane intersection, new standalone Datum Point, generic VERTEX/MID/CENTER Axis, new independent Axis toolbar controller, modifications to Datum Plane constructor, G1, solid Mirror/Pattern, PM-06 or general UI frameworks.

## 7. Owner UX decisions: recorded direction versus still pending explicit signoff

### Directed by Owner using actual Workbench/Datum screenshots (2026-10-15)
- **Work Features:** physically visible new **Axis** toolbar button alongside **Datum Plane**, invoking existing Axis tool lifecycle, not a second tool.
- **Operations:** Datum-consistent **Constructor** dropdown for Axis with the exact new entry **Edge**; preserve legacy **Sketch Line** compatibility/default.
- **Tree:** authored Axis objects in existing **Reference Geometry** alongside Datum Plane; **one row per AxisId**. The current Sketch-Line Axis rows under Sketch require a bounded presentation migration, no duplicate row or durable model move; built-in axes remain under Origin.
- **Full lifecycle:** do not ship just "one Edge click": Create, Edit/Replace, Finish/Cancel, Delete, Show/Hide/group actions, Undo/Redo, Save/Close/Reopen/cold rebuild, stale/rejected reference repair and downstream consumer diagnostics must be testable with existing machinery.

### Detailed UX decisions to ratify before first UI implementation
1. Is `Sketch Line` as constructor **default** for legacy compatibility, with `Edge` as the second entry, acceptable? Should entering Axis with a preselected Body Edge choose `Edge` automatically or require explicit constructor change?
2. Should the **existing Reference Geometry mixed group Show/Hide** operate atomically on all Datum + authored Axis, while a source Sketch's own Show/Hide does not hide its authored Axis?
3. Is the proposed finite **unoriented** Axis preview with existing hover/selection cue, typed Stage and status, **without Axis Reverse** correct?
4. Are `Replace Edge`, one source slot, full repair diagnostics and existing Finish/Cancel/Enter/Esc semantics sufficient, including behavior for a formerly Sketch-Line Axis edited into Edge mode (and vice versa) if conversion is later allowed?
5. May the Axis-owned Command Line add local `EDGE` and `SKETCHLINE` tokens, while keeping `SOURCE`, `FINISH`, `CANCEL`, avoiding global `EDGE`?
6. Does the future **production contract** first need a genuine native topological Edge-reversal fixture, since A0 only accepted its GAP for read-only feasibility?

**Nothing in this design document is a new executable Work Contract.** The Owner's specific UI layout requests are recorded above, but broader details require a separately explicit Owner UX design signoff, distinct accepted narrow production D2 contract, governance merge and ACTIVE-only activation. No UI/Viewer/Tree/Operations/CLI/schema/command/test mutations before those independent gates.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This is a one-file proposed future UX consultation artifact under `work/**`, not an as-built user-facing feature or a change to current semantics. Any separately accepted and activated Axis vertical slice must update current internal docs, bilingual product docs and generated Browser under DOC-01.
