# UI-01 — Safe edit context and responsive Part/Sketch Workbench — BOUNDED WORK CONTRACT DRAFT

**Status: DRAFT FOR SEPARATE OWNER WORK-CONTRACT APPROVAL — NOT ACCEPTED, NOT ACTIVE, NO CODE AUTHORITY.**
**Prepared:** 2026-10-11. **Base:** `main` `2a91d0e1252255e943a367ab8a79bcd7cae5e258` after Edge Picking R1 acceptance and merge.
**Design source:** [UI-01 governance/D2 Draft PR #320](https://github.com/MechanicalCave/SimpleSolid2/pull/320), including:
- `work/UI-01_WORKBENCH_CONTEXT_RESPONSIVE_TOOL_SURFACES_D2_PROPOSAL.md`;
- `work/UI-01_RIGHT_PANELS_AND_TOP_TOOLBAR_DESIGN_REVIEW_OPTIONS.md`;
- `work/UI-01_RESPONSIVE_WORKBENCH_WIREFRAMES_D2_REVIEW.md`;
- `work/UI-01_VISUAL_GALLERY_D2_OWNER_REVIEW.md` and three accompanying static SVGs.

**Owner D2 design status:** The Owner approved the three-mockup set as a whole on 2026-10-11 by explicitly answering `akceptuję zestaw`. This signs off their *conceptual visual and behavioral direction*, **NOT** as-built Windows/Qt behavior, exact DPI breakpoints, icons, native screenshot equivalence or this Work Contract. Earlier individual Owner D2 decisions remain recorded in PR #320.

**Current frozen governance:** `work/ACTIVE.yaml` on `main` retains completed Edge Picking R1 for traceability only and authorizes **no** new UI-01 production work. `work/PART_MODELING_V1_ROADMAP.md` v1.30 remains the accepted roadmap. UI-01 re-sequencing before incomplete PG-01E [PR #311](https://github.com/MechanicalCave/SimpleSolid2/pull/311) and PM-06 must be made an explicit accepted governance amendment where required; **this draft does not silently change the roadmap or activate UI-01**.

## 1. Goal, users and completed-state witness

Deliver three linked outcomes in this strict sequence:

1. **P0 safety:** prevent any incompatible concurrent Feature Edit draft and Sketch/Profile Edit. Refuse an attempted mode transition precisely and without changing the in-progress feature, sketch, authored model, Undo/Redo, dirty flag or live UI state.
2. **P1 right column:** independently scrollable Properties and Operations, user-adjustable height allocation (initial **40% / 60%**), fixed existing operation-completion footer, and short-height Properties/Operations tabs which do not mutate edit state.
3. **P1 one-row contextual toolbar:** as many labelled real Part/Sketch tools as fit, retained fixed Document actions, user preference for whole-group ordering/pinning, named responsive collapse and last-resort two-level `Więcej → grupa → narzędzie` overflow.

**Completion evidence:** actual native Windows UI-01 workflows, exact-head Windows FULL tests, owner-reviewed before/after Windows screenshots at ordinary, compact and very short heights, explicitly separate Owner practical **UI-01 FINAL PASS** and **distinct permission to merge**. The accepted D2 static SVGs serve as composition reference, not executable evidence.

## 2. Architectural invariants (must not change)

- Retain Foundation v1.0, accepted ADR-0003/0005/0006/0010/0011/0012, existing SR-03 responsive shell, stage-aware Part/Sketch edit authority and Document lifecycle. The right Operations panel owns only the live contextual operation; never create a second mutable draft through Tree/Viewport switching.
- Workbench remains a **Part and Sketch product**. No Assembly/Drawing command set, Ribbon, new command palette, global CAD-input dispatcher, dock framework or new public Viewer/provider API is introduced by this package. Generic compositional code may be reusable later only.
- No changes to exact semantic Commands/Transactions, immutable Body topology identity, R1 Edge Picking behavior, Part Document file persistence/schema **v16**, `ModelingSemanticsVersion 2`, B-Rep kernel semantics, native save protocol or existing authorized feature scope.
- Active draft/finish/abort semantics remain **as-built**. In particular, fixed footer placement never fabricates `Cancel Sketch` where the current product has no such command. `Axis-in-Sketch` valid workflows are explicitly preserved.
- Resize, tab switching, scrollbar display, group reordering/pinning and menu navigation are **non-authoring UI actions**; they do not create Undo entries, dirty Documents, revise geometry, auto-Finish/Cancel a tool or silently change selected edit target.

## 3. Frozen D2 presentation and behavior for this contract

**Right panels:** Properties above Operations, independent vertical scrollbars with stable reserved gutter. Initial effective usable height `Properties 40% / Operations 60%`; divider is drag-adjustable. User-customized proportions are retained per open Document **only within current app session**, across Document switches and temporary tabs, and reset to default on next launch. During active Feature Edit, Properties follows semantic Tree/Viewport inspection selection; Operations stays bound to the active Feature draft. Mode/target cue appears **above viewport near top-left** and repeats subtly in Operations header; both reflect actual context rather than Tree selection or a denied edit entry. A non-scrolling Operations footer shows only existing appropriate Finish/Cancel controls, Finish subtly stronger.

**Short window:** if *measured effective height* is inadequate for two usable panes and fixed action footer, switch to `Properties | Operations` tabs with Operations selected on entering an active edit, Properties reachable in one click. Restore two-pane layout and the user's session ratio when height permits; DPI/font thresholds have hysteresis. **Window width alone must not trigger those tabs.**

**Toolbar:** exactly one contextual visual row; fixed Undo/Redo/Save/Close Document controls in a separate zone, without horizontal scrollbar or second line. Inline group names with subtle separators and compact neutral CAD-specific icons + labels. Approximate reference at Windows 100%: 18–20 **logical px** icon artwork and 32–36 logical-px button heights, subject to real DPI, font, adequate hit targets and accessibility proof. Small pin marker beside a pinned group title is **passive in normal CAD use**; only the explicit `Dostosuj pasek` mode has interactive pin/unpin, intentional whole-group drag handles, keyboard Move Left/Right and Reset Defaults.

**Default command groups/order (only EXISTING commands):**
- Part: `Sketch [Sketch 2D]` (pinned), `Work Features [Datum Plane]`, `Create [Extrude, Revolve]` (pinned), `Modify [Fillet, Chamfer]`.
- Sketch: direct base action `Select`, then `Create [Line, Circle, Arc, Rectangle]` (pinned), `Profile [Profile]`, `References [Project Geometry]` (pinned), `Modify [Trim, Extend, Extend Both]`, `Transform [Move, Copy, Rotate, Scale, Mirror]`, `Inspect [Measure]`.
- `Sketch 2D` is a proposed **display label for the existing Sketch action**, not a new command. Owner-approved group order and default pins are initial/Reset Defaults only; whole-group order/pin changes persist between app restarts separately for Part vs Sketch in **non-CAD application preferences**, never in Part files; `Select` remains outside group reordering. Individual action rearrangement is deferred.

**Overflow priority:** 1) collapse **unpinned** groups to named dropdowns; 2) switch buttons in still expanded **pinned** groups from icon+text to icon-only with tooltips and accessible names; 3) collapse pinned groups to named menus if needed; 4) put otherwise unrepresentable groups under `Więcej → nazwana grupa → prawdziwe narzędzie`. Keep valid commands reachable even at narrow widths, preserve group order and QAction identity, enabled/checked/focus/shortcut semantics. Preserve fixed Document controls and Viewport usability throughout.

**Tool state:** only an actually engaged current Sketch tool (e.g. Line while drawing) receives subtle active highlight. Existing `Extrude002` Feature Edit never alone marks the `Extrude` **creation button** engaged. Hover, keyboard focus ring, disabled and engaged state must remain distinct; no color-only semantics.

## 4. Bounded permissible implementation locations — AFTER explicit activation ONLY

Candidate existing modules, **subject to native source audit at gate G0**:
- `src/ui/cad_workbench.cpp`, `src/ui/cad_workbench.hpp`: P0 edit-entry compatibility/guards; context and existing command contributions; no new CAD Commands.
- `src/ui/cad_workbench_shell.cpp`, `src/ui/cad_workbench_shell.hpp`: P1 Qt splitter, scrollbars, footer, tabs, responsive toolbar group/menu presentation, keyboard customization.
- A local existing Tree adapter or Document-context presentation interface only if a deterministic RED proves it is the alternate entry route; **STOP for Owner scope amendment before touching new paths**. 
- Focused tests: `tests/sk01_workbench_sketch_host_test.cpp`, `tests/pm05f_r2_native_workbench_edit_test.cpp`, `tests/cad_workbench_test.cpp`, `tests/workbench_native_stress_test.cpp`, `tests/workbench_document_dialog_integration_test.cpp`, `tests/view_cube_responsive_test.cpp`, existing Qt boundary checks `tests/verify_workbench_shell_boundaries.cmake`, and narrowly justified additional fixtures in those targets. Adding a new target or changing CMake test registration requires separate Owner scope authorization.

**Production as-built docs** after approval: existing `docs/internal/CAD_WORKBENCH_VIEWER.md`, related internal lifecycle/UI docs as actually impacted, and paired `docs/product/pl/PARTS.md` and `docs/product/en/PARTS.md` (plus other *existing* product files only if demonstrably affected). Regenerate `docs/browser/index.html` strictly via `.\ss2.ps1 docs`; never hand-edit the generated Browser. Stable `doc-id` and section-id parity are mandatory. No new persisted CAD settings.

## 5. Strict staged RED → GREEN workflow and STOP rules

| Gate | Allowed stage | Evidence to move forward |
| --- | --- | --- |
| **G0 — baseline/reproduction** | Fresh production branch from separately UI-01-activated main. Inventory current Qt/Tree edit-entry call paths and Owner Windows witnesses. Reproduce P0 original Feature draft → Sketch/Profile entry conflict with **native deterministic RED** (plus valid Axis-in-Sketch negative control); characterize long-form panel and narrow-top-bar layout in native assertions. | Reproducible actual failure and all as-built callback semantics known. If original P0 fails to reproduce, STOP for Owner review, no speculative bypass. |
| **G1 — P0 first** | Introduce one authoritative **fail-closed** incompatibility gate for all Sketch/Profile edit entry routes while a solid Feature Draft is live. Reject with useful message; no Finish/Cancel, status split-brain, authored mutation, revision/Undo drift, draft replacement or shortcut bypass. | Native Qt GREEN cases for Extrude, Revolve, Fillet, Chamfer draft paths, Tree double-click, selection then Edit, context switching, accepted Axis-in-Sketch and explicit Finish/Cancel → valid Sketch entry. **No P1 until P0 FOCUSED pass.** |
| **G2 — right-hand panels** | Native split, stable gutters and independent scrolling; fixed existing Finish/Cancel footer; true mode cue synchronized to Operations; per-document session ratio 40/60; short-height tabs with hysteresis. | Native Qt tests for long real forms, 40/60 initial ratio, user drag/restoration, form overflow, panel footer, selection during Feature Edit, document switches, 100/150/200% DPI where feasible and no geometry/Undo mutation. |
| **G3 — adaptive toolbar** | Existing Part/Sketch actions grouped, named, prioritized; explicit whole-group customization and non-CAD per-context preference; context-specific presentation; no new commands. | Native Qt tests for wide/compact/narrow availability, icon/menu action identity, pin/collapse priority, two-level More reachability, hover/focus/disabled/engaged correctness, reset/defaults and restart persistence, shortcut/Esc/Ctrl+Z/Ctrl+Y behavior. All valid commands reachable without horizontal scroll or a second row. |
| **G4 — user docs and exact-head tests** | As-built doc parity, offline Browser regeneration; FOCUSED → FAST → FULL exact-head Windows Core+Kernel+Desktop+docs+aggregate. | All required native suites PASS with immutable SHAs and no disabled/weakening tests, no unsupported CI #302 workaround. Complete screenshot/Owner manual workflows. |
| **G5 — Owner acceptance** | Owner compares actual running Windows/Qt application with accepted D2 SVG concepts. | Separate explicit **UI-01 PRODUCT FINAL PASS** after practical test; only after PASS request **distinct merge permission** and proceed on approved exact production HEAD. No auto-merge. |

**STOP conditions (any gate):** new feature/Project Geometry scope, changing schema/B-Rep/Viewer public API, accidental double-edit, change in authored revision from resize, insufficient Viewport/Navigation Cube containment, inaccessible context-specific finish control, translation losing command identity, invalid keyboard/focus routing, large unreviewed command registration changes, or missing exact-head tests. Any such change needs distinct D2/contract amendment before work.

## 6. Mandatory acceptance scenarios and architecture evidence

1. Starting an incompatible Sketch/Profile edit while an Extrude/Revolve/Fillet/Chamfer draft is live gives a clear rejection message; draft/selection/cue/buttons remain single-owner and unchanged. Repeated attempts and Undo/Redo are safe.
2. Legitimate Sketch editing and Axis-in-Sketch remain usable; after explicit Feature Finish/Cancel, the same Sketch entry is allowed.
3. Selecting another Tree/Viewport entity while editing a Feature updates Properties for inspection only; Operations stays with original draft. Switching Document or closing it follows existing lifecycle without ghost drafts.
4. At long Properties and Operations forms both have independent vertical scroll and reserved gutters. Finish/Cancel footer remains accessible while form content scrolls; no fictitious Cancel Sketch. Accessible focus/keyboard navigation remains intact.
5. Normal windows start 40/60; dragging changes ratio; short-height tabs pick Operations during edit; per-document ratio survives tabs and document switch until application exit, resets after next run; no persisted CAD metadata.
6. At width pressure only contextual group representation compresses via the approved priority, maintaining separate Document actions and a usable Viewport; no extra toolbar row/scrollbar; all valid commands accessible via named menus and two-level More when necessary.
7. Create-mode Line's engaged state versus existing Extrude002 edit cue, hover, keyboard focus and disabled visual states are semantically distinct.
8. Pin marker inert outside Customize; only whole groups reorder in Customize; keyboard alternative and reset work; Part/Sketch preferences persist independently across app launches; no Part dirty/Undo effect.
9. Windows native layout includes maximized and representative 1440×900, 1280×720, 1024×768, narrower supported size, and 100%/150%/200% DPI where actually available. Tested minimums are measured against actual widget geometry and text metrics, not frozen by the conceptual SVG.
10. Existing Edge Picking R1, Project Geometry, ViewCube, selecting, Sketch/Profile operations, close/reopen and Parts remain regression-clean; all Core, Kernel, Desktop and docs gates remain trustworthy.

## 7. Exclusions and external gates

- PG-01E [PR #311](https://github.com/MechanicalCave/SimpleSolid2/pull/311) remains unaccepted/incomplete; no inference of Project Geometry capability completion or modifications of that contract.
- PM-06 remains blocked. Future Assembly/Drawing are outside this Contract; reusable UI shell must not synthesize a toolbar full of unimplemented commands.
- Independent CI issue [#302](https://github.com/MechanicalCave/SimpleSolid2/issues/302) cannot be fixed, bypassed or used to waive tests under UI-01.
- No feature addition, Ribbon redesign, individual tool-button drag customization, new search palette, persisted CAD preferences, source-level domain/kernel changes or unapproved native implementation dependencies.
- The roadmap re-sequencing and D2 proposal in PR #320 must be reconciled with accepted roadmap v1.30 **before a formal activation**; this DRAFT is not itself a roadmap amendment.

## 8. Separate Owner acceptance and activation gates

1. **D2 package:** PR #320 now has Owner acceptance of the three static SVGs as a set. It remains Draft and unmerged until independent approval to merge the D2/governance-only proposal after its exact-head Windows checks. Mere D2 design acceptance is **not** merge consent.
2. **This Work Contract:** Owner must separately review and explicitly accept *this bounded text*, including P0/P1 order and allowed file/test set. Draft PR review/CI cannot substitute for that approval.
3. **Formal governance activation:** after agreed sequencing/roadmap authority is recorded and this contract is accepted/landed on main through a separately approved governance merge, create a **dedicated activation PR** changing `work/ACTIVE.yaml -> active_work` to the accepted UI-01 contract path. Its exact-head Windows governance closure and distinct Owner merge approval are required before any product branch.
4. **Production:** open fresh implementation branch *only from activated main*. The production package follows G0–G5, requires real native RED/GREEN, exact-head Windows FULL, Owner manual product FINAL PASS and separate merge authorization. Never treat this proposal or its SVGs as permission to edit production.

## Documentation impact

Internal docs: **required** for P0 edit-context authority, panel/tool-surface ownership, transient preferences and runtime lifecycle.
User/Product docs: **required** in canonical PL/EN Markdown for changed UI workflows, fail-closed messaging, responsive panels and toolbar customization/overflow.
Generated Browser: **required** via deterministic offline `.\ss2.ps1 docs` and `ss2 verify` before product completion.
Reason: P0 changes user-visible edit entry behavior and error feedback; P1 changes Workbench layout and command discovery. **This draft review PR requires no as-built doc changes yet.**
