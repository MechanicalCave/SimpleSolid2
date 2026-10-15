# RG-02C/B1 — Axis from one straight Body Edge: Datum-consistent Work Features UX (Owner consultation proposal)

**Status:** **OWNER UX SIGNOFF ACCEPTED (2026-10-15) for the reviewed single-constructor Edge flows and the existing Sketch Axis separation**, in response to corrected PR #410 UX HEAD `6259c2c7932c5b6766b3b3dfb75b3e3ce1d656ab`. This D2 UX decision is NOT an executable Work Contract; PR #410 is UNMERGED / NOT ACTIVE / no UI mutation authority before separate technical contract acceptance, governance merges and `ACTIVE.yaml` activation.
**Date:** 2026-10-15.
**Baseline:** protected main `38b0c8b274d47548647fc7c27e9a7884fd85ff20`, Owner-final read-only native A0 evidence PR #408 squash MERGED; A0 true native topological Edge-orientation reversal fixture remains an explicit **GAP**, accepted for A0 only.
**Authority:** Constitution/Foundation 1.0 FROZEN/Architecture and accepted ADRs; Part Modeling roadmap v1.33 §6/§20B; Owner RG-02 reference geometry design `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` §2.3(1) and §3 mandatory Owner UX prior approval; existing PM-04 Sketch-Line Axis; Datum S4B tool; DOC-01.
**Owner principles:** new UI consistent with existing app and Datum Plane; **reuse existing code and tests, no parallel Axis implementation**. This is the **first** prospective Axis constructor, **one straight material Body Edge** only; later Axis constructors require independent decisions.

## Owner explicit UX approval — scope and limits, 2026-10-15

After being shown the revised proposal with **only `Edge` in the Constructor dropdown**, Sketch Line Axis created **inside Sketch** and preserved under its source Sketch, and Edge Axis under **Reference Geometry**, Owner responded **`akceptuję - kontynuuj`**. This is **separate, explicit Owner UX signoff** for the concrete corrected W1–W3 and bounded UX-01…UX-16 direction in this proposal, not merely the earlier permission to prepare the draft.

**Approved UX contract direction:** visible single Work Features `Axis` beside `Datum Plane`; one existing Workbench-style Operations dialog with `Constructor: Edge` as the **ONLY** current item; one strict straight material Body Edge; reuse existing native/Part semantic and viewport infrastructure; one finite unoriented line preview after resolution; no Axis-level Reverse; Finish/Cancel/Edit/Replace/Delete/visibility/Undo/Redo/Save/Close/Reopen and repair lifecycle; newly Edge-sourced Axis under Reference Geometry; existing authored Sketch Line Axis remains under Sketch; Origin axes stay under Origin. No extra Axis constructor has been approved.

**Not decided by this UX approval:** persistent new `PartAxis::source` representation, native schema/migration, dependency-stage cycles and Revolve consumer semantics, numeric policy for the A0 topological orientation-reversal evidence GAP, exact new public provider/API requirements, executable allowed paths, tests, documentation mutations, and whether an optional Axis-owned CLI alias `EDGE` is needed. Defaults in this document remain proposals where they touch architecture; Owner D2 separately decides before code. Concrete unreviewed interaction changes must return to the Owner instead of expanding this signoff.

**Independent STOP:** PR #410 requires its own exact-head governance checks and **separate Owner consent to guarded squash merge**; this UX acceptance does not itself merge PR #410 or #409, activate a technical Work Contract, or authorize any `src/**`, test, UI, command, schema or product documentation change. PM-06, G1 and later Axis constructors remain inactive.

## 1. Exact Owner UX direction and corrections — 2026-10-15

1. Owner illustrated the existing **Work Features → Datum Plane** button and its **Operations → Constructor** selector, and directed a **new visible `Axis` button next to Datum Plane under Work Features**. The right Operations panel is analogous to Datum Plane, but the new Axis tool's constructor dropdown is currently **ONE ITEM ONLY: `Edge`**. The combo is deliberately extensible for possible **future** Axis constructors; neither current Sketch Line Axis nor hypothetical point/cylinder/plane variants are entries.
2. Owner explicitly corrected the earlier proposal `Constructor: Sketch Line / Edge` as incorrect: **`Sketch Line` Axis is already created directly inside Sketch authoring**, not through the new Work Features Axis constructor selection. Do **not** expose `Sketch Line` in this combo; do not default to it, auto-switch to it, or add a Sketch-Line migration choice. Single `Edge` is selected by default because it is the **only** item.
3. The newest Owner screenshot shows `Axis001` nested beneath **Sketch 3**, highlighted by the added arrow. Existing Sketch-Line Axes **remain in their source Sketch subtree**. Owner's prior `w tree umieszczamy w reference geometry` applies to **newly created Edge-sourced Axes**, which belong beside Datum Planes under **Reference Geometry**. **No migration/duplication** of existing Sketch-Line Axis rows. Built-in X/Y/Z axes stay under Origin.
4. **Pełny cykl życia** applies to the new Edge Axis, not just picking one Edge: Create, Edit/reselect, Finish/Cancel, Delete, Show/Hide and group visibility, properties and selection, Undo/Redo, persistence Save/Close/Reopen/cold rebuild, dependency/diagnostic repair and Revolve consumer safety.
5. These statements are **design direction**, **not** proof that future UX exists or approval to modify code. Remaining preview, command tokens, signed-direction policy and exact technical/persistence design need further Owner UX/D2 decisions before production mutation.

## 2. Current SS2 application facts — preserve real behavior

- `src/ui/cad_workbench.cpp` currently physically instantiates the `datumPlaneToolButton` in a one-row **Work Features** toolbar. The screenshot displays that button and no `Axis` button. `axis_button_` is declared and referenced by Workbench state, but no physical Part toolbar Axis button was found in the inspected construction. **Adding a physical button is a new UI change**.
- The existing `axisOperationsWidget`, `axisSourceLabel`, `axisResultLabel`, `axisFinishButton`, `axisCancelButton`, `startAxisTool`, `startAxisEdit`, `refreshAxisEvaluation`, `syncAxisUi`, `finishAxis`, `cancelAxis`, `submitAxisCadInput` and application `AxisDraft` cover the previous Sketch Line Axis authoring/draft behavior. Reuse appropriate controls, commands and transactional lifecycle, **but do not make the new Work Features Axis tool create Sketch Line axes or clone a second Part-owned Axis machinery**.
- The current Sketch Line authoring exposes `sketchAxisDesignationCheck` and creates a Part-owned Axis from a selected/current Sketch Line. That workflow is the compatibility baseline; it stays in Sketch, and its `Axis001`-style Tree row remains under its source Sketch. Existing Sketch-Line Axis Edit/Delete and all referenced `AxisId` semantics must keep working.
- `application::submitAxisCadInput` currently supports `SOURCE`, `FINISH`, `CANCEL`, `ESC` and empty owning Enter for its current Axis draft. This existing parser is **not** proof that future Edge constructor tokens or exact picking dispatch exist.
- `Reference Geometry` currently contains authored Datum Plane rows. A **new** Edge-sourced Part Axis would appear there once implemented. `Origin` continues to contain builtin X/Y/Z Axis, and Sketch-line Axis remains nested below its Sketch. All rows resolve by semantic `AxisId`/DatumId rather than viewer/toplevel indices.
- The current right Properties/Operations pane uses the existing independently scrollable 40/60 split with tab fallback at insufficient height, and sticky Finish/Cancel footer. The Part toolbar is one-row, responsive, and tool conflicts fail closed. Copy its **UX grammar**, reuse its **code**, not an unrelated dialog.

## 3. Proposed single Work Features Axis tool — one constructor `Edge`

**Entry and Constructor:** Place one `Axis` button adjacent to `Datum Plane` under Work Features. It opens the existing right **Operations / Axis** ownership context with a `Constructor` combo containing **exactly one item `Edge`**, selected by default. The dropdown remains visible (as in Datum Plane) to establish the future multi-constructor UX affordance. No `Sketch Line` option, no conversion between Sketch-Line and Edge modes, no `SKETCHLINE` command token, no separate toolbar `Datum Axis` action. The unchanged Sketch authoring workflow remains the only current `Sketch Line` creation path.

**Source and precise selection:** The Edge constructor asks for exactly one **strictly resolved straight material Edge** from a declared current `BodyStageRef`. A single `Edge` source slot with `Select/Replace`, readable `Stage` and `Status` is sufficient. Hover and click must reuse already proven native material Edge acquisition, `MaterialEdgeReference`, Q0/Q2 strict resolution, exact analytic geometry and existing viewport highlight conventions; no second Point click, generic Datum Point, MID/CENTER, screen XYZ, provider handle, seam/closed Edge, arbitrary approximate line, or nearest-geometry rebind.

**Draft preview:** No source means `Finish Axis` disabled and no viewport Axis preview. Once strictly Resolved, render an ephemeral finite cue for the prospective **infinite unoriented Axis line**, with the source Edge highlighted according to the existing selection grammar. No Axis `Reverse` control for this Edge-sourced unoriented line; signed Revolve direction belongs to the consuming feature. Invalid/missing/stale/ambiguous/unsupported/blocked source immediately refuses Finish and clears preview without showing last-good geometry. Whether an actual native topological reversal fixture is a production precondition remains a separate D2 gate.

**Create/commit/cancel:** `Finish Axis` and owning Enter must recheck current Part document revision, strict Edge reference, stage/dependency order and future accepted persistence contract before committing **one** semantic Part transaction, issuing an authored `AxisId`. `Cancel`/owning Esc/hover/invalid pick create zero model mutations and zero Undo steps, including no consumed AxisId. The transient draft does not persist or become pickable geometry before Finish.

**Full lifetime / repair:** Tree/viewport `Edit Axis` reopens the existing Edge Axis with its source and explicit `Replace Edge`, keeping its `AxisId`, visibility and downstream `AxisReference`. Valid Edit is one Undo step; Cancel restores the prior object. `Delete Axis`, Show/Hide (individually and in mixed Datum+Edge Axis Reference Geometry group), Undo/Redo, Save/Close/Reopen, fresh-provider evaluation, missing/ambiguous/stale/unsupported source diagnostics and downstream Revolve block/repair must be validated. No guessed source, stale viewer line, accidental source migration or dependency bypass.

**Tree ownership/presentation:** **Reference Geometry = all Datums and only new Body-Edge Axes**, each exactly once under that group. **Sketches = existing Sketch-Line Axes**, each exactly once under its source Sketch, as the real screenshot shows. **Origin = built-in X/Y/Z**, never authored copies. Existing Sketch Axis creation and Edit remain intact; Reference Geometry group Show/Hide becomes a mixed Datum+Edge-Axis operation via existing commands/transactions, not a new group persistent flag or a second visibility system.

**CAD Command Line parity — design pending:** Work Features `Axis` must use the **same** source/status/draft semantic authority as the GUI, with existing `SOURCE`, `FINISH`, `CANCEL`, owning Enter/Esc behavior preserved where applicable. If an Axis-owned `EDGE` keyword is desired, its only possible meaning is selecting the already sole `Edge` constructor; **no need for a new keyword** just to choose the only mode. The exact root command and any Edge-specific alias need future Owner review, without overriding legacy Sketch Axis creation or global selection.

## 4. Reviewable UX wireframes — not screenshots of implemented behavior

### W1 — Work Features toolbar / Operations before selection

```text
Sketch:       Work Features:               Create:              Modify:
[Sketch]      [Datum Plane] [Axis]          [Extrude] [Revolve]   [Fillet] [Chamfer]
                            ^ NEW button, existing shared Workbench grammar

Document Tree                 Viewport                Operations — Axis
  Origin                      [current Body]          Constructor  [Edge   v]
    X / Y / Z Axis                                    Source Edge  [Select ...]
  Reference Geometry                                  Stage        [not set]
    Datum Plane 1                                     Status       Select a straight Edge
  Sketches                                            ---------------------------
    Sketch 3                                          [Finish Axis disabled] [Cancel]
      Axis001  (Sketch Line: unchanged)
```

The **only current entry** of the openable Constructor combo is `Edge`. The source Sketch-Line Axis row remains under Sketch 3.

### W2 — one strict Edge click, pending Finish

```text
Tree                          Viewport                    Operations — Axis
  Reference Geometry          [highlighted real]         Constructor  [Edge v]
    Datum Plane 1              [straight material Edge]    Edge         [Extrude001 / Edge …]
  Sketches                    [finite unoriented]         Stage        [after Extrude001]
    Sketch 3                   [Axis preview cue]          Status       Resolved
      Axis001                                             [Finish Axis] [Cancel]
```

One genuine current-stage Edge source; no independent Point/MID/CENTER, signed orientation checkbox or authored mutation until Finish.

### W3 — finished Edge Axis and repair, existing Sketch Axis unchanged

```text
Document Tree                  Properties — Axis         Operations — Edit Axis
  Origin                       Axis002                   Constructor [Edge v]
    X / Y / Z Axis             AxisId  [stable]          Edge        [Missing] [Replace]
  Reference Geometry           Source  [Body stage]      Stage       [after Extrude001]
    Datum Plane 1              Status  Missing           Status      Missing — repair
    Axis002 [Missing]          Visible Shown             [Finish disabled] [Cancel]
  Sketches
    Sketch 3
      Axis001 (Sketch Line, stays here)
```

No duplicate Axis rows, no migration of Sketch-Line Axis, no stale last-good preview. The right panel reuses existing Properties/Operations and lifecycle controls.

## 5. Future acceptance matrix — complete Edge Axis vertical slice, not yet executed

| ID | Requirement | Future native/Qt acceptance |
| --- | --- | --- |
| UX-01 | One Work Features `Axis` button beside `Datum Plane` | Same one-row toolbar/responsive/focus grammar; no second Axis command pipeline |
| UX-02 | `Constructor` combo with **only `Edge`** now | Default `Edge`, list stays visible and future-extensible; **no Sketch Line item** |
| UX-03 | Existing Sketch Line → Axis in Sketch | `sketchAxisDesignationCheck` remains working; no regressions in create/edit and source Sketch identity |
| UX-04 | Tree provenance | **Edge Axis in Reference Geometry; Sketch Line Axis under Sketch 3 (or its source Sketch); Origin X/Y/Z in Origin**, each once |
| UX-05 | Native straight Edge hover/click | Current-stage precise `MaterialEdgeReference`, exact line only, shared picker/marker behavior |
| UX-06 | Pre-Finish source/preview | No draft line or enabled Finish before strict Edge source; transient finite unoriented preview only when Resolved |
| UX-07 | Invalid/seam/curved/ambiguous/stale | Typed refusal, preview cleared, 0 authored mutation, never geometric guess/last-good fallback |
| UX-08 | Finish/Cancel/Enter/Esc | One revision-bound transaction and one Undo on valid Finish; Cancel/hover 0 Undo |
| UX-09 | Edit / Replace Edge | Same AxisId, visibility and dependent Revolve AxisReference, one Undo or zero if canceled |
| UX-10 | Delete and downstream repair | No silent reassignment of source; dependent Revolve diagnostic, Undo restore or explicit repair |
| UX-11 | Visibility in Tree | Individual Edge Axis/Datum and mixed Reference Geometry Show/Hide with one atomic transaction, Undo/Redo |
| UX-12 | Save/Close/Reopen/cold provider | Durable semantic Edge + Body stage, stable AxisId/visibility; resolved derived infinite line or typed refusal |
| UX-13 | GUI/CAD Command Line | Same Axis Edge draft and Finish/Cancel authority; **no `SKETCHLINE`/constructor-toggle token** |
| UX-14 | Existing behaviors unaffected | Sketch Axis nested row, Datum Plane constructors, Project Geometry and PM-04 Revolve intact |
| UX-15 | Responsive/edit context | Existing 40/60→tab panel/footer, same one-row toolbar, incompatible edit fails closed |
| UX-16 | Native direction semantics | Unoriented exact line; no provider orientation as authored sign; A0 actual native edge-reversal fixture **remains GAP** |

**No UX-01–16 tests were run for this unimplemented feature.** These are proposed acceptance criteria, not retroactive evidence from A0.

## 6. Product/architecture STOP boundaries and still-open Owner decisions

A0 only proved bounded read-only native **feasibility** for a test-only straight Edge line. It did **not** implement persistent `PartAxis::source` variant, native file migration/versioning, Part `AxisDraft` source semantics, dependency floors and cycle prevention for Revolve, downstream edit/recompute, viewport picker or UI/CLI. A0's genuine native topological orientation reversal fixture remains **GAP**, even though Owner accepted that limitation for A0 technical FINAL PASS.

**UX direction now unambiguous:** New Work Features Axis button; Operations Constructor = **Edge only** (future entries deferred); existing Sketch Line Axis **created in Sketch and stays under Sketch**; new Edge Axis under **Reference Geometry**; full lifecycle included, no duplicated code/test systems.

**UX now expressly Owner-approved for the reviewed bounded UI flows** (one `Edge` constructor, Work Features Axis button, same app/Datum look, source/preview, lifecycle, and Tree source separation). **Still require independent technical D2/implementation decisions:** (1) precise API and stage-aware durable source and schema versioning, (2) dependency/rebuild/consumer semantics, (3) A0 actual B-Rep topology reversal GAP as a production risk, (4) optional scoped CAD Command Line alias tokens, (5) exact allowed files/test/regression matrix and DOC-01 obligations. Do not infer new UX beyond W1–W3 and this one constructor.

**STOP:** The Owner's screenshot correction and UX direction are not an authorization to implement. Before a first UI/Viewer/Tree/Operations/CAD CLI mutation, obtain explicit approval of reviewable UI flows, then separately accept one narrow RG-02C production D2 Work Contract with exact allowlist, document versioning, FOCUSED/FAST/FULL/native/manual acceptance and DOC-01 internal plus bilingual product and generated Browser impacts; separately authorize governance merge and `ACTIVE.yaml` activation. No PM-06, G1, standalone Point, two-Point Axis, cylinder Axis, two-plane Axis, Mirror/Pattern or new Body is activated.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: this is a single `work/**` UX consultation design proposal, not a shipping product behavior change. Future Axis production slice requires separate DOC-01 updates for internal as-built, PL/EN product and generated Browser.
