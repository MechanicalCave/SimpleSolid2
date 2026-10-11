# UI-01 — Workbench context safety, adaptive right panels and universal document command surface — D2 PROPOSAL

**Status:** **OWNER ACCEPTED CONSOLIDATED D2 STATIC-VISUAL WORKBENCH DESIGN (2026-10-11, `akceptuję zestaw`)**, as refined in the decision register and [three-SVG visual gallery](UI-01_VISUAL_GALLERY_D2_OWNER_REVIEW.md). The Owner **explicitly accepted v1.31 UI-01-first roadmap sequencing** (`zatwierdzam`, 2026-10-11); it is prepared in dedicated [roadmap Draft PR #322](https://github.com/MechanicalCave/SimpleSolid2/pull/322), **not yet merged**. The [bounded UI-01 Work Contract in Draft PR #321](https://github.com/MechanicalCave/SimpleSolid2/pull/321) was **separately Owner-accepted** (`akceptuje -kontynuuj`, 2026-10-11) but is **NOT active**. D2 PR #320 remains **Draft, unmerged and not an implementation authority**. This PR no longer changes the roadmap file; only #322 owns the proposed v1.31 file diff.  
**Baseline:** `main` `2a91d0e1252255e943a367ab8a79bcd7cae5e258`, after Edge Picking R1 production PR #314 and acceptance closeout PR #319 merged.  
**Owner-approved sequencing v1.31 (not merged):** UI-01 P0/P1 stabilization **before** resuming/scoping PG-01E product-wide closeout and **before** PM-06. Revisit incomplete Project Geometry functional scope separately; do not claim PG-01E FINAL PASS merely from a favorable manual smoke test.  
**Governance:** Foundation 1.0 and accepted ADR-0006, ADR-0003, ADR-0010, ADR-0011, accepted SR-03 responsive-shell and SK-01A contextual-operations contracts; Part Modeling v1 roadmap 1.30 currently accepted. `work/ACTIVE.yaml` retains completed Edge Picking R1 only for history and authorizes **no new UI implementation**.

## 1. Owner's current native Windows witnesses

The Owner submitted two screenshots (2026-10-11) of the current Part Workbench and requested a roadmap change **before adding more Project Geometry capability**:

- **W-UI-01 — conflicting edit contexts (P0).** While editing a Body Feature (illustrated by Extrude Add/Midplane Edit), the Tree can open a Sketch/Profile for edit **without finishing/cancelling the Feature draft**; the UI simultaneously exposes Finish Edit and Finish Sketch, i.e. two mutable contexts. The captured status says "Profile edit context opened." The screenshot demonstrates conflicting UI edit states, **not yet a demonstrated committed corrupt B-Rep or lost Undo**.
- **W-UI-02 — compressed right side (P1).** Properties and Operations lack user-controllable height allocation or independently accessible overflow. With long feature forms and many tool options, both groups shrink until text/rows overlap or buttons become impractically small in compact windows.
- **W-UI-03 — horizontal toolbar scroll (P1).** Sketch/Part tools are appended as a long line with static text separators (e.g. Create, Profile, Modify). On window narrowing Qt shows a horizontal scrollbar occupying a second toolbar line; commands are hidden offscreen and the top/document action row becomes visibly asymmetrical. The requested future design is a **single reusable workbench command-surface policy**, applied to current Part and Sketch and reusable by future Assembly/Drawing *when those document kinds are separately delivered*.

The Owner also stated: current Project Geometry smoke testing reveals no major defect but **several important capabilities are still missing**. This is an **open, not-yet-enumerated capability inventory**; no PG-01E completion is inferred and no extra Project Geometry implementation is authorized in UI-01.

## 2. Source audit and already accepted constraints

**Static source observations, not a completed native RED test:**

1. `src/ui/cad_workbench.cpp::requestEditSketch()` rejects an already-active *different* Sketch but does **not** check `extrude_draft_`, `revolve_draft_`, `fillet_draft_` or `chamfer_draft_`. `requestEditProfile()` can enter Sketch edit through that function. Conversely `startExtrudeEdit()` and `startRevolveEdit()` already reject `active_sketch_id_`. This **asymmetry is a grounded P0 hypothesis** to reproduce with real Feature draft + Tree double-click/Edit Sketch/Edit Profile/commands before changing it. Also inspect ALL Feature edit entry paths, not only Extrude.
2. `src/ui/cad_workbench_shell.cpp` creates `documentToolsScroll` as a `QScrollArea` with `Qt::ScrollBarAsNeeded` horizontally and `QSizePolicy::Preferred` vertically. `cad_workbench.cpp` inserts many individual pushbuttons and headings by positions. This matches the reported scrollbar rather than providing adaptive grouping/overflow.
3. `src/ui/cad_workbench_shell.cpp` puts `Properties` and `Operations` into one right `QVBoxLayout` with stretch factors **0 and 1**, `right_panel_` size policy `Ignored`, and no independent vertical scrollers at the shell layer. Large content can be compressed; source audit cannot yet prove every clipping mechanism or all DPI variants.
4. **SR-03 is already accepted and completed.** Its responsive normal/compact/narrow Workbench template, center viewport priority, and future DocumentKind surface ownership must be **reused and restored**, not silently replaced with a contradictory global toolbar architecture. **ADR-0006** keeps domain launchers in the Document tool zone, contextual parameters in Operations, and Workspace/document actions in their respective owners.

## 3. D2 proposals for Owner decision

### D2-U1 — deterministic edit-context exclusivity (P0, FIRST)

- **One compatible, current editable context per Part Document:** an active mutable **Extrude/Revolve/Fillet/Chamfer Feature draft** must not coexist with entry into arbitrary Sketch/Profile edit or into a second unrelated Feature draft. Apply a shared, fail-closed **entry guard** at all entry points: Tree double-click/context menu, Properties button, toolbar, CAD Command Line, Profile navigation and any effective native callback.
- **Initial minimal safety choice:** while a Feature edit is active, reject opening Sketch/Profile edit with a concise status describing the active tool and requiring user Finish or Cancel **first**. No silent Finish, automatic draft Cancel, dirty-state mutation, history entry or implicit navigation.
- An explicitly intentional existing context combination (e.g. authored Axis source Line selection from within a Sketch edit) must be enumerated and preserved; do **not** broadly disallow valid Axis/Sketch or Profile-in-its-own-Sketch workflows. Formalize compatible vs incompatible context transitions with one auditable decision path; ensure G7 Edge Picking / PG-01C-D stage, stale revision and provider behavior remains intact.
- **RED then GREEN:** native Workbench real Part / Extrude Add Edit → attempt Sketch 2 and Profile002 through Tree and alternate entry routes; assert draft and FeatureId unchanged, Sketch edit not opened, no extra Finish Sketch, no revision/Undo/dirty changes. After explicit Finish or Cancel, Sketch edit starts normally. Repeat with Revolve, Fillet, Chamfer, other nested UI contexts and multi-Document switch; test accepted Axis source edit exceptions. Keep Undo/Redo/Save and explicit Finish semantics unchanged.

### D2-U2 — usable independently resizable Properties/Operations (P1)

- Replace implicit right-side `QVBoxLayout` height competition with a bounded, **user-adjustable vertical split** or equivalently explicit independent collapsible panels; give Properties and Operations **independent vertical scrolling**, stable non-zero effective minimum usability, and no horizontal form clipping at the accepted panel widths.
- **Finish/Cancel and blocking diagnostic text must remain reachable** even with long contents, potentially via a compact fixed action footer inside the owning Operations panel (not duplicate buttons with separate callbacks). User must be able to allocate more height to Operations without making Properties inaccessible, and vice versa.
- Retain existing right-panel width behavior and SR-03 narrow panel recovery, prioritize Viewport, preserve widget focus/local text input and currently active tool across resize, scrolling and document switches. No unrequested persistence of arbitrary splitter dimensions; decide if optional UI preference state needs an explicit follow-on contract.
- Verify high-content Feature, Profile, Project Geometry and deep Properties pages. Test vertical normal/compact/narrow windows including maximized, **1440x900, 1280x720, 1024x768 and a narrower supported size**; use DPI-aware layout invariants rather than hardcoded geometry. DPR 1.0/1.5/2.0 **only where actually available**.

### D2-U3 — reusable adaptive document command surface (P1)

- Preserve SR-03's **one-row ownership separation**: contextual **domain command slot** on the left; Document **Undo/Redo/Save/Close** fixed and always visible at right. Do not rebuild this as one Part-specific permanently growing button row.
- Introduce a bounded **presentation model for grouped actions**, contributed by the active DocumentKind and its context (Part standard, Sketch edit, active Feature/Project Geometry where applicable). Groups have stable action identities, ordering, enabled/checked state, tooltips/accessibility, shortcuts and shared callbacks; the right Operations panel remains for tool parameters and Finish/Cancel.
- For width reduction use **adaptive compact labels/icons plus a clearly named overflow dropdown** for lower-priority actions/groups, not a horizontal scrollbar. Essential context actions and at least a discoverable way to reach **every valid command** must remain visible/operable. A group title is semantic metadata/presentation, not a fixed-width spacer requiring more ribbon width.
- Reuse ordinary Qt Widgets/QAction/QToolButton/QMenu or an equivalent small compositional solution. No speculative QMainWindow global docking/ribbon/plugin framework, new shared CAD-input dispatcher, DocumentKind-specific domain implementation or public Viewer contract.
- Validate stable action identity and dispatch under resizes and moving between Sketch/Part context, menu keyboard accessibility, focus/shortcuts/Space/Esc/Ctrl+Z/Ctrl+Y, disabled action semantics, Viewport navigation and multi-tab switch. Commands moved into overflow execute exactly the same existing command and may not mutate the Part merely by reflow.

### D2-U4 — bounded common Workbench template and visual acceptance

- The **shell contract** may be reusable for future Assembly/Drawing, but **only Part and Sketch** receive current product implementation/tests. No fake Assembly/Drawing commands, editors or documents in UI-01.
- Provide a small before/after screenshot matrix: maximized and compact window, Feature active with blocked Sketch entry, long Properties/Operations, top bar expanded/overflow, and deliberate keyboard navigation. Selectable text must be readable, buttons fully clickable and neither native Viewer nor Navigation Cube obscured.
- Measure actual runtime sizes, visibility, widget rectangles and accessible actions in Qt native tests; screenshot visual quality requires independent Owner Windows manual acceptance. The supplied screenshots are witnesses, not complete machine-independent pixel acceptance specs.

## 4. Staged work and exit gates (only AFTER separate approved Work Contract and activation)

| Gate | Work | Minimum exit / STOP |
| --- | --- | --- |
| UI0 | Current source/Owner screenshot inventory, deterministic **native RED** for W-UI-01/02/03, compat-context table | If not reproducible, report exact setup and STOP rather than guessing |
| UI1 | P0 single-place edit entry guard + session-invariant regression | No conflicting Feature + Sketch/Profile modes, no silent authored mutation; native Windows FOCUSED |
| UI2 | P1 right panel vertical allocation + independently scrollable content | All contextual controls reachable with normal/compact/narrow layouts, stable focus/Viewport; native Qt layout tests |
| UI3 | P1 toolbar action grouping and adaptive overflow, shell-first reusable presentation only | No horizontal scrollbar, all available commands accessible, document actions fixed; focus/shortcut tests |
| UI4 | Existing docs/internal + PL/EN as-built updates; regenerate offline Browser via `.\ss2.ps1 docs`; FOCUSED/FAST when appropriate and **one immutable Windows FULL Core/Kernel/Desktop/docs + aggregate** | All tests green, no disabled tests; Owner screenshot/cross-context practical FINAL PASS and distinct merge permission |

## 5. Required regressions and ownership

**Primary likely modules (read/validate against the later Work Contract):** `src/ui/cad_workbench.cpp/.hpp` (Part edit guards and command contribution), `src/ui/cad_workbench_shell.cpp/.hpp` (pure Qt layout and overflow), existing Part Tree adapter for alternate entry routes only if a defect requires it; existing `tests/sk01_workbench_sketch_host_test.cpp`, `tests/pm05f_r2_native_workbench_edit_test.cpp`, shell resize/Document lifecycle tests and Qt-native UI integration tests. New targets/CMake machinery are not preauthorized.

**Preserve:** Project/Document ownership, Section 2 accepted SR-03 responsive states; native Viewer/provider boundary; stage-aware Part/Sketch/Project Geometry identity, transactional Finish/CANCEL, active draft lifetime; ModelingSemanticsVersion 2, Part schema v16, unrelated Engine/Kernel topology and B-Rep, existing R1 Edge Picking and CI issue #302.

**No code fixes in this D2 proposal.** Any edit-context authority change exceeding bounded UI orchestration, global CAD-input overhaul, persistence of splitter layout, Viewer/public provider API or different DocumentKind implementation is a separate explicit D2 STOP.

## 6. Re-sequencing APPROVED by Owner; awaiting separate roadmap PR merge

**Current roadmap on main remains v1.30**, while Owner explicitly approved v1.31 on 2026-10-11 (`zatwierdzam`). The formal **roadmap-only v1.31 amendment lives in [Draft PR #322](https://github.com/MechanicalCave/SimpleSolid2/pull/322)**, not in this D2 PR. Until a distinct merge approval and merge, v1.30 on main remains authoritative. The approved change is sequencing only and does not unfreeze completed Feature contracts:

`PG-01A–D + Edge Picking R1 completed` → **UI-01 (P0 first, P1 after)** → `PG-01E re-scoped capability gap audit and product closeout` → `PM-06 Part v1 completion`.

The current [PG-01E D2 proposal PR #311](https://github.com/MechanicalCave/SimpleSolid2/pull/311) stays **UNACCEPTED** until reviewed/updated; do not label it complete or automatically merge it. The Owner's unspecified missing Project Geometry capabilities must be **listed and categorized after UI stability**: required to close PG-01E under an amended accepted contract versus explicitly deferred next feature package. A positive existing smoke test is not PG-01E final signoff. PM-06 remains blocked.

**Formal order:** Owner accepted the consolidated D2 concept and roadmap v1.31 sequence; Owner separately accepted the bounded UI-01 Work Contract (PR #321). **Still required:** independent approval to merge governance/design PR #320, accepted-contract PR #321 and roadmap-only PR #322; coordinate dependency order without silent roadmap version downgrade; after accepted governance lands on main, a separate UI-01 `ACTIVE.yaml` activation PR, exact-head Windows checks and distinct Owner merge consent; only then open a fresh UI-01 production branch and begin native RED work. Complete native FULL, Owner UI-01 FINAL PASS and a **distinct merge approval** before declaring UI-01 completed. PG-01E then gets its own re-reviewed D2/Work Contract and activation.

## 7. Owner D2 decisions — individually resolved in accepted SVG set and roadmap v1.31

The original questions are preserved for historical review; **the D2 visual concept and v1.31 execution order have now been expressly approved by Owner**, with separate accepted bounded Work Contract in PR #321. Remaining gates are governance merges, activation and native product verification, not repeated D2 design approval.

1. Approve **UI-01 before PG-01E and PM-06**, with **P0 edit-context conflict first**?
2. Approve **reject Feature Draft → Sketch/Profile edit until Finish/Cancel**, while preserving specifically documented valid Axis-in-Sketch contexts, rather than automatic nested edit or draft termination?
3. Approve **independent Properties/Operations height management + vertical scrolling**, and a **context-aware grouped toolbar with overflow (never horizontal scrollbar)**, using a common shell template but not implementing Assembly/Drawing?
4. Agree that **PG-01E remains open**, with missing Project Geometry features inventoried for a later separately accepted scope?

The answers authorize preparation of a **Work Contract**, **not runtime mutation or merging this roadmap amendment without its separate approval**.

## Documentation impact

Internal docs: **not required for this proposal only** — records a D2 design decision request, not as-built implementation.  
User/Product docs: **not required for this proposal only** — no actual supported behavior changes.  
Generated Browser: **not required for this proposal only** — no canonical Product Markdown change.  
**Future UI-01 implementation:** both internal and PL/EN product docs **required**, deterministic offline Browser regeneration **required**, to reflect changed edit mode entry, right-panel resizing/scroll, and toolbar overflow.
