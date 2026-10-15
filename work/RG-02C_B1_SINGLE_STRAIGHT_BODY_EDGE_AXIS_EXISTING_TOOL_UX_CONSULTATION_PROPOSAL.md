# RG-02C/B1 — Axis along one straight Body Edge: existing-Workbench UX consultation (D2 PROPOSAL)

**Status:** **UX D2 PROPOSED FOR OWNER REVIEW — NOT ACCEPTED, NOT AN EXECUTABLE WORK CONTRACT, NOT ACTIVE, NOT MERGED, NO UI MUTATION AUTHORITY.**
**Date:** 2026-10-15.
**Protected-main baseline:** `38b0c8b274d47548647fc7c27e9a7884fd85ff20` (PR #408 A0 read-only evidence and separate Owner A0 technical FINAL PASS; explicit real-native-Edge topological orientation reversal fixture **GAP**, not retrospectively PASS).
**Authority:** Constitution + Foundation 1.0 FROZEN, Architecture/accepted ADRs, Part Modeling v1.33 §6/§20B, RG-02 parent Owner-accepted direction `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` §2.3(1), §3 mandatory prior UX Owner signoff, completed PM-04 Sketch-Line Axis, existing PM-03/S4B single Datum Plane UI, DOC-01.
**Owner constraints:** `UI musi byś spójne z aplikacją i istniejącym "datum"`; `nie powinnismy duplikować testów i istniejącej logiki`. They are mandatory design constraints, **NOT** approval of these particular screens, tokens or source model.
**Target:** review the **FIRST** prospective Axis constructor along **one strict, straight material Body Edge at a declared Body history stage**, not all four later RG-02C constructors. This artifact is a design proposal only.

## 1. Grounded existing application behavior — do not displace it

- There is **one existing Axis tool** (Workbench `axis_button_`, `axisOperationsWidget`, `axisSourceLabel`, `axisResultLabel`, `axisFinishButton`, `axisCancelButton`). `startAxisTool`, `startAxisEdit`, `stageAxisSource`, `refreshAxisEvaluation`, `syncAxisUi`, `finishAxis`, `cancelAxis` and `submitAxisCadInput` already operate through a Part-owned semantic draft/transaction. Current Axis sources are **Sketch Line only**, default, and existing authored Sketch-Line Axis Tree rows live **under their source Sketch**. Origin X/Y/Z remain in `Origin`.
- There is **one existing Datum Plane tool**, under Work Features, with constructor selector **Offset / Edge + Point / 3 Points**; shared Operations source/status, Preview, Finish/Cancel, Edit, global contextual CAD Command Line, Reference Geometry Tree, Show/Hide and atomic Undo. S4B already supports strictly certified END points; do not create a second Axis/Datum picker, new global Point type or separate draft layer.
- The existing **Reference Geometry** tree group currently contains authored **Datum Planes** only, while existing Sketch-Line Axes appear under Sketches. The placement of future Body-Edge Axes in this group is therefore a **new UX decision**, not an existing behavior to assert as fact.
- On the right, independently scrollable **Properties** and **Operations** use 40/60 default; at insufficient height they become Properties/Operations tabs. Existing action buttons remain in the sticky operation footer where applicable. The Part toolbar stays one row and preserves contextual editing safety.

## 2. Proposed narrow single-tool interaction — OWNER DECISIONS REQUIRED

**Entry / mode:** Keep the existing single **Axis** Work Features command and `axisOperationsWidget`; add an Axis **Constructor** row with **Sketch Line (default, existing)** and **Body Edge (proposed)**. Do not add `Datum Axis` as a new parallel toolbar button, do not change the already-supported Sketch Line behavior, and do not make Body Edge a variation of `Datum Plane`. Constructor switches discard only transient draft input and selection leases after explicit user feedback, not authored objects, loaded documents or editing history.

**Eligibility:** For `Body Edge`, the user selects one **strictly resolved straight material Edge in the current declared Body stage**. Use the existing real native topological selection/hover affordance and strict `MaterialEdgeReference` source pipeline proven by A0/Q0/Q2. Do **not** infer from a visually straight tessellation, hover XYZ, native runtime token, arbitrary Vertex/MID/CENTER marker, seam/closed Edge, nearest line or final Body geometry after stage change. A single source slot named **Edge** is enough; a second point click is **not** required.

**Stage:** The selected authored source identifies the **Body stage used when accepted**; subsequent feature additions do not auto-rebind that source to final Body. Operations displays a compact **Stage** readout (e.g. `Body stage — after Extrude001`) and **Edge / Resolved | Missing | Ambiguous | Unsupported | Blocked**. Stage label is a user-visible description, **not** durable source identity. If the current stage is unavailable or forward-dependent, refuse Finish, with a meaningful diagnostic.

**Draft and preview:** Before one strict source has been acquired, `Finish Axis` is disabled and there is no ghost Axis. Once the test-only A0 semantics have a separately accepted production evaluation contract, show a *finite viewport cue for the infinite unoriented line* with subdued highlight of its source Edge, using the existing Axis presentation/selection conventions. No positive-arrow indicator or `Reverse` checkbox for this unoriented Body-Edge constructor — the **consuming Revolve** owns signed direction. Camera orientation/native Edge orientation must not change authored Axis meaning. Invalid source immediately removes the preview, never uses a last-good line.

**Create/edit:** `Finish Axis` (or owning-context Enter) revalidates document revision/source/stage, issues one future **Owner-approved** Part semantic transaction, then displays the resulting stable `AxisId`; `Cancel` (or owning-context Esc according to existing focus grammar) leaves 0 authored changes and 0 Undo steps. `Edit Axis` preloads existing source/mode, permits **Replace Edge** in the same slot, preserves `AxisId`, visibility and dependent `AxisReference` references, and commits one Undo step. If source is Missing/Blocked/Ambiguous, keep authored Axis selectable/editable with no stale viewport line; expose its source and typed diagnostic. Conflicting Sketch/Profile/Feature edits refuse the transition rather than implicitly finish/cancel another operation.

**Revolve:** Existing Origin X/Y/Z and Sketch-Line Axis selection/consumption remain untouched. A future Body-Edge Axis is still referenced by authored **AxisId**, not a copy of its native geometry; source stage/dependency/forward-cycle validation remains Part-owned and **requires a later D2 source/persistence contract**. Viewer cues are derived; visibility never controls evaluation.

## 3. Reviewable wireframes — proposals, not as-built screenshots

### W1 — Create Axis, Body Edge constructor, source pending

```text
Part / Work Features                         3D Viewport                 Right column
[Sketch] [Axis] [Datum Plane] [...]          [ current Body ]           Properties
                                                                        No Axis committed
                                                                        ----------------------
                                                                        Operations > Axis
                                                                        Constructor
                                                                        ( Sketch Line | Body Edge* )
                                                                        Source Edge    [Select...]
                                                                        Stage          [not set]
                                                                        Status         Select one
                                                                                       straight Edge
                                                                        [Finish Axis disabled]
                                                                        [Cancel]
```

`*` Proposed addition. The existing `Sketch Line` path is unchanged; no second Axis tool.

### W2 — Source resolved / live draft (one Body Edge click)

```text
Tree                      3D Viewport                    Operations > Axis
Origin                    [solid with one highlighted    Constructor   Body Edge
  X Axis / Y Axis / Z ...   straight physical Edge]       Edge          Extrude001 / Edge …
Reference Geometry         [finite Axis line cue]         Stage         after Extrude001
  Datum Plane …            (unoriented; no +arrow)        Status        Resolved
                                                          [Finish Axis] [Cancel]
```

Preview stays transient. It does not create an AxisId, does not become a pickable authored object, does not change Undo/dirty/save and vanishes on source failure, Cancel or Finish.

### W3 — Existing Axis selected for Edit/repair

```text
Document Tree                    Properties / Axis            Operations / Edit Axis
Reference Geometry               Axis 7                        Constructor   Body Edge
  Datum Plane 3                  AxisId       [stable ID]       Edge          [Missing]
  Axis 7 [Missing]  <selected>    Source       Body stage…      [Replace Edge]
Sketch 2                         Status       Missing          Diagnostics   Source unavailable
  Axis 4 [Sketch Line]           Visibility   Shown            [Finish disabled] [Cancel]
```

**Placement shown is a PROPOSAL**: new Body-Edge Axes as siblings of Datum Planes under **Reference Geometry**; preserve Sketch-Line Axes under their source Sketch; do not duplicate or migrate existing Tree rows. The group Show/Hide behavior would have to include these new children in **one existing semantic visibility transaction**, without acquiring an independent group persistent flag. This placement and group-visibility consequence need explicit Owner approval and a later contract/test scope.

## 4. Explicit guided scenarios for Owner walk-through

| Scenario | Gesture | Expected screen / safety |
| --- | --- | --- |
| U01 existing Axis compatibility | Start Axis with a selected Sketch Line | `Sketch Line` default; original Source/Finish, Sketch nested Axis, CAD input preserved |
| U02 Body-Edge entry | Start Axis in Part context → `Body Edge` | Single source slot, no external Datum Plane tool/modal |
| U03 picking | Hover then click one *strictly supported* straight material Edge | Existing hover feedback, visible Stage/Resolved and finite **unoriented** preview; hover alone does not commit |
| U04 unsupported | Click curved Arc, seam/closed or non-strict/ambiguous Edge | Typed explanatory diagnostics; no preview; Finish disabled; no fallback/migration |
| U05 stale stage | Upstream edit invalidates source / switch feature stage | No last-good ghost; display authored source and repair path, do not select nearest Edge |
| U06 Edit | Tree → Edit Axis → Replace Edge → Finish | Stable AxisId, stable Revolve consumer, one transaction/Undo; Cancel leaves unchanged |
| U07 Tree | Select Show/Hide on proposed Reference Geometry group | Datum+Body-Edge Axis visibility changes as one authored transaction; Sketch-Line Axis stays with Sketch |
| U08 CLI | Enter Axis contextual input, change constructor, acquire viewer/tree source, Finish/Cancel | Exactly same draft, semantic validation and commit; no raw 3D coordinates |
| U09 keyboard/collisions | Esc/Enter while Axis owns input; attempt conflicting Sketch edit | Existing focus/priority semantics, no stealth Finish/Cancel and no unintended Undo |
| U10 narrow-window | Collapse/restore Tree or right Panel; low viewport height | Existing single-row toolbar, Operations footer and 40/60→tab behavior retained |
| U11 upstream change / cold reload | Save, close, reopen and rebuild on a new provider | Source stage/AxisId retained, evaluated line reconstituted or typed blocked; no cached B-Rep token |

**No U01–U11 manual tests have been run for this unimplemented UX**. This is a consultation checklist, NOT execution evidence.

## 5. Scoped global CAD Command Line grammar for Owner decision

The **existing** `AXIS` tool and `SOURCE`/`FINISH`/`CANCEL` semantics are the compatibility baseline; **do not claim the new constructor tokens exist today**. Propose, only within an Axis-owned context, `BODYEDGE` (switch constructor to Body Edge) and `SKETCHLINE` (switch back to legacy); `SOURCE` requests an eligible source selection in viewport or Tree, not XYZ parser arguments; `FINISH` revalidates exactly one Part transaction; `CANCEL` abandons the draft. No global `EDGE` verb, no new standalone `DATUMAXIS` command, no `REVERSE` for the unoriented Body Edge line. Preserve current input priority, whitespace/case policy and Esc/Enter route; exact tokens are **proposed**, subject to Owner review and future native/Qt integration evidence.

## 6. Product/architecture STOP conditions — explicitly not yet solved

A0 **did not** implement or approve a persistent direct Body-Edge `PartAxis::source` variant, revision-bound Axis draft integration, `AxisId` lifecycle for that source, native Part schema migration, fail-closed dependency floor, Revolve consumption of a Body-stage source or new viewport/tree acquisition APIs. Native B-Rep **topological orientation reversal** remains an A0 evidence GAP accepted *for A0 only*; do not silently claim it verified for a durable production Axis.

If any proposed interaction needs public API, new serializable selector, a provider-neutral native geometry contract, multi-stage dependency/cycle semantics or third-party presenter changes, **STOP for one separate narrow Owner D2 production Work Contract with exact allowed paths, data format, tests and DOC-01 obligations**. Do not duplicate the Q0/Q2 resolver, Stage Semantic Reference system, Axis draft/transaction path, Datum Plane picker, global CAD command dispatcher, Tree row identity or test infrastructure. Exact native/Qt manual mouse-pick testing and lifecycle must remain in registered tests with existing resource locks.

Excluded: through two Points, cylindrical surfaces, intersection of planes, new standalone Datum Point, generic VERTEX/MID/CENTER for Axis, reverse direction as Axis authored input, changes to Datum constructors, PM-06, Mirror/Pattern, new UI toolbar, new Part Body or G1. Future proposals need separate D2 and Owner UX decisions.

## 7. Owner UX consultation decisions — do NOT infer consent from permission to prepare this PR

1. **Single existing Axis tool / constructor** — accept `Sketch Line` default + `Body Edge` second option within existing Operations?
2. **Tree placement and group visibility** — accept Body-Edge Axes as children of existing Reference Geometry (while Sketch-Line Axes stay under Sketch), including group Show/Hide of mixed Datum/Body Axis?
3. **Source + preview** — accept one `Edge` slot with visible Body stage, existing hover/pick grammar, finite **unoriented** line, no `Reverse` in Body Edge mode?
4. **Edit/diagnostics** — accept Replace Edge, preserve AxisId, Missing/Unsupported/Ambiguous/Blocked visible and repairable, no stale preview?
5. **Command Line** — accept proposed Axis-scoped `BODYEDGE` / `SKETCHLINE` tokens, retaining SOURCE/FINISH/CANCEL and existing ownership of Enter/Esc?
6. **Scope/constraints** — require any additional native-topological reversal evidence *for future production*, given A0 only accepted that exact evidence GAP for its own limited feasibility stage?

The Owner may accept, amend or reject each item. Approval of review preparation, this proposed `work/**` artifact, D2 native A0 evidence, or a governance squash of this proposal is **not** prior Owner UX design approval. Only a subsequent **explicit UX decision** identifying these screens/flows and limitations may satisfy RG-02's mandatory UI STOP, and **still does not activate executable product code**.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This is a one-file proposed future UX consultation artifact under `work/**`, not an as-built user-facing feature or a change to current semantics. Any separately accepted and activated Axis vertical slice must update current internal docs, bilingual product docs and generated Browser under DOC-01.
