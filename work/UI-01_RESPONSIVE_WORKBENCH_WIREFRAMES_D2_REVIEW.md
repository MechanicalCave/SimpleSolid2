# UI-01 — Responsive Workbench wireframes / interaction matrix — OWNER D2 REVIEW

**Status:** **Owner accepted the COMPACT Sketch Edit concept and the VERY-SHORT-HEIGHT Feature Edit concept** individually (2026-10-11); normal-height design elements, including the initial 40/60 Properties/Operations split, also have recorded Owner acceptance. **Final consolidated visual/behavioral D2 signoff is STILL PENDING.** These are design wireframes, not pixel-accurate native Qt screenshots; no UI runtime authority, Work Contract, ACTIVE activation or merge permission.
**Basis:** Owner's recorded UI-01 D2 decisions in `UI-01_RIGHT_PANELS_AND_TOP_TOOLBAR_DESIGN_REVIEW_OPTIONS.md`; v1.30 accepted roadmap and `main` remain unchanged.
**Important:** Text wireframes depict conceptual states and component hierarchy, **not pixel-accurate Qt screenshots, exact icons, fixed breakpoints or new CAD features**. The 3 scenarios are defined by actual space available at current DPI/font metrics; named example sizes do NOT become hardcoded width/height thresholds.

## 1. Normal viewport, generous width and height — Part Feature Edit

```text
+--------------------------------------------------------------------------+
| DOCUMENT controls: [Undo] [Redo] [Save] [Close]    (separate fixed zone)   |
| TOOL STRIP: [pin Sketch | Sketch 2D] [Work Features | Datum Plane]         |
|             [pin Create | Extrude Revolve] [Modify | Fillet Chamfer]       |
|             (ONE visual row only; schematic wrapping here is notation)    |
+---------------+------------------------------------+---------------------+
| Tree          | MODE: Feature Edit · Extrude002     | PROPERTIES          |
|  Body         | [3D viewport / ViewCube]           | selected: Edge X    |
|  Sketch 2     |                                    | values of Edge X    |
|  Extrude002   |                                    | independent scroll  |
|               |                                    +---------------------+
|               |                                    |  ↕ draggable split  |
|               |                                    +---------------------+
|               |                                    | OPERATIONS          |
|               |                                    | Feature Edit:       |
|               |                                    | Extrude002          |
|               |                                    | [parameters]        |
|               |                                    | [scrollable...]     |
|               |                                    |---------------------|
|               |                                    | [Finish Edit][Cancel]|
+---------------+------------------------------------+---------------------+
```

- The top-left mode cue is **above**, not obstructing the 3D viewport. It and Operations header have the same true live Feature Draft identity.
- Changing Tree/viewport inspection selection changes **Properties only**; Operations continues to edit Extrude002. Attempting incompatible Sketch Edit is refused fail-closed, leaving draft and cues untouched.
- `Extrude` is a **creation** action. Its button is NOT automatically lit just because Extrude002 is under **editing**. Hover and keyboard focus have their own distinct visual states.
- **Owner-approved first-use right-column split: 40% Properties / 60% Operations** of usable splitter height at normal window heights, not an enforced ratio during dragging. On resizing, keep meaningful pane minimums and reserve the non-scrolling Operations footer; when height does not suffice, use the accepted tabs fallback. Per-open-Document user adjustments survive tab fallback and document switching during the session, not app relaunch. Resizing does not change authored CAD state.
- Vertical scrollbar gutter is permanently reserved for each Properties and Operations pane, regardless of overflow, to avoid form-width jumps. Scrollbar thumb discreet but accessible. Each form scrolls independently; the contextually valid Operations action footer stays visible while Operations is displayed.

**Normal-height default:** 40/60 Properties/Operations is the Owner-approved initial proportion. Exact Qt separator position derives from effective splitter content geometry and must not obscure headers or Finish/Cancel at supported DPI/font sizes.

## 2. Compact width, still sufficient height — Sketch Edit

```text
+------------------------------------------------------------------+
| DOCUMENT controls [Undo][Redo][Save][Close] (reserved zone)       |
| TOOL STRIP [Select] [pin Create: icons/labels as fit]             |
|            [pin References: Project Geometry] [Modify v]         |
|            [Transform v] [Inspect v]   [More v only if needed]    |
+------------+--------------------------------+--------------------+
| Tree       | MODE: Sketch Edit · Sketch 2    | PROPERTIES         |
|            | [3D / sketch viewport]         | selected object    |
|            | Line tool currently ENGAGED    | scroll             |
|            |                                +--------------------+
|            |                                | ↕ draggable split  |
|            |                                +--------------------+
|            |                                | OPERATIONS         |
|            |                                | Sketch Edit        |
|            |                                | current valid      |
|            |                                | controls/diagnosis |
|            |                                |--------------------|
|            |                                | [Finish Sketch]    |
+------------+--------------------------------+--------------------+
```

- Text sketch uses **Line engaged** as an interaction example, not a requirement to add a new Sketch mode. `Line` appears active only while it actually owns the currently running tool.
- `Finish Sketch` is shown, but **no fictitious `Cancel Sketch` command** is introduced. Display only contextually real existing actions in the fixed Operations action zone.
- Responsive priority: first collapse **unpinned** groups to named drop-downs; next switch buttons within **pinned** groups to accessible icon-only tool buttons; only then collapse pinned groups to named menus. No horizontal scrollbar, extra toolbar row, spontaneous group reorder, or permission to enlarge application minimum dimensions.
- **Owner accepted this compact-width Sketch Edit arrangement (2026-10-11)** as the intended D2 behavior: Tree, a usable Sketch viewport with persistent true edit-mode cue, and independently scrolling Properties (above) + Operations (below) remain visible side by side. Right-column initial ratio is 40/60 subject to pane minimums. Adapt the contextual tool strip first, preserving the one-row layout and separate Document actions. Width alone does NOT justify Properties/Operations tabs; only insufficient height can trigger them. No fake Cancel Sketch command. Exact native Windows sizing, labels, focus and overall mockup signoff remain pending.

## 3. Very short available height (regardless of width) — active Feature Edit

```text
+----------------------------------------------------------------+
| DOCUMENT controls [Undo][Redo][Save][Close]                     |
| TOOL STRIP [pin Sketch v] [pin Create v] [More v] (if needed)    |
+----------+--------------------------------+--------------------+
| Tree     | MODE: Feature Edit · Extrude002 | [Properties][*Ops] |
|          | [3D viewport / ViewCube]       | OPERATIONS         |
|          |                                | Extrude002         |
|          |                                | [scrolling form]   |
|          |                                |--------------------|
|          |                                | [Finish Edit]      |
|          |                                | [Cancel]           |
+----------+--------------------------------+--------------------+
```

- Switch the right-hand resizable split to **Properties / Operations tabs only if measured usable height cannot hold both panes plus the active Operations footer**; prefer Operations on entering edit. Properties remains one click away, without cancelling or finishing the draft.
- Returning to sufficient height restores both panels and the **same per-open-Document splitter ratio** as before the temporary tab fallback, until the application session ends. Do not persist splitter ratio to CAD files or across app restarts.
- In the Properties tab the hidden Operations footer is NOT duplicated, but switching back to Operations always reveals valid Finish/Cancel. Define a reachable path back even when tab titles are truncated.
- **Owner approval for this VERY SHORT-height concept (2026-10-11):** the Owner accepted this exact scenario with Operations initially selected during Feature Edit, Properties accessible without ending the draft, and the existing context-appropriate Finish/Cancel pinned at the bottom of visible Operations. This is a specific layout/behavior acceptance; actual Qt pane geometry, footer min-height/accessibility and final consolidated visual approval remain OPEN.
- `More > [named group] > [actual command]` is the final overflow level for complete groups that do not fit. Command identity/disabled state/focus is preserved. Pinned status remains passive outside Customize mode.

## 4. Group customization (all three layouts)

- Dedicated `Customize toolbar` mode only: whole-group drag handle, explicit pin/unpin button, keyboard Move Left/Move Right, and Reset to Defaults. Normal mode shows a small passive pin icon beside an inline pinned group title.
- Stable action and group identities; independently remembered **Part** and **Sketch** group order/pin settings between app launches in non-CAD user preferences. Default pins: Part Sketch + Create; Sketch Create + References.
- Accepted group content:
  - Part: `Sketch: Sketch 2D`, `Work Features: Datum Plane`, `Create: Extrude/Revolve`, `Modify: Fillet/Chamfer`.
  - Sketch: `Select` directly; `Create: Line/Circle/Arc/Rectangle`, `Profile: Profile`, `References: Project Geometry`, `Modify: Trim/Extend/Extend Both`, `Transform: Move/Copy/Rotate/Scale/Mirror`, `Inspect: Measure`.
- **Owner-accepted default order (2026-10-11):** Part: `Sketch → Work Features → Create → Modify`. Sketch Edit: `Select → Create → Profile → References → Modify → Transform → Inspect`, where `Select` is a direct base command, not a reorderable group. Reset to Defaults restores both those orders and previously approved default pins. Users may reorder whole groups per context thereafter; independent non-CAD preferences persist between app launches. Actual icon artwork, exact naming/localization, fine spacing, and popup behavior remain to review. No permission to reorder individual buttons or implement future Assembly/Drawing commands.

## 4A. Owner-approved compact-density reference for visual mockups (2026-10-11)

- Owner accepted the **compact desktop CAD** baseline over oversized/spacious toolbar buttons: approximately **18–20 logical px icons** and **32–36 logical px button heights at 100% Windows scaling**, small but legible in-group gaps, and a single-row inline title + direct-button arrangement. Preserve the accepted group taxonomy, priority, customization and no horizontal scrollbar.
- These are **approximate drawing proportions only**, not implementation constants: final sizes, DPI-safe hit regions, tooltip/focus affordances and text legibility require Windows screenshot/Qt testing at 100%, 150%, 200% scaling and current font metrics. Do not interpret this acceptance as final pixel-by-pixel UI approval.
- Tool buttons show neutral technical pictograms with labels while space allows; on compact width first collapse unpinned groups, then show pinned actions as icon+tooltip, finally collapse pinned groups to named menus; `Więcej -> group -> tool` is last-resort overflow.

## 5. Behavioral state checklist / proposed native test witnesses

| Witness | Expected observation |
| --- | --- |
| Sketch active Line tool | Line button has a restrained **engaged** highlight; mode cue says Sketch Edit; hover elsewhere does not erase tool ownership. |
| Edit existing Extrude002 | Operations/mode cue say Feature Edit · Extrude002; **Part Extrude creation button is not automatically active**. |
| Hover on idle button | Hover-only surface distinct from engaged tool and focus. |
| Keyboard Tab focus on action | Visible independent focus ring and accessible name even if the action isn't engaged. |
| Ineligible command | Disabled treatment not confused with active/hover; action never launches from disabled state. |
| Inspect other Tree item during Feature Edit | Properties follows selection; Operations and edit state unchanged. |
| Attempt Sketch/Profile Edit inside incompatible Feature draft | Clear rejection; same draft, same two cues, no authored revision/Undo/dirty mutation. |
| Toolbar narrows, then widens | Document controls and valid commands reachable; stable single row; no scrollbars, no unexpected edit change or group order reset. |
| Right-hand form overflows | Gutter already reserved; fields don't jump in width; footer accessible; no model mutation just from scroll/resize. |
| Window becomes too short | Tabs appear and Operations selected for active edit; Properties reachable; returning to tall restores prior per-Document split. |
| Switch between Part and Sketch | Only applicable real tool groups/actions shown; distinct persisted group arrangement, no simultaneous edit modes. |
| Customize or Reset | Changes GUI preferences only; Undo/redo, DocumentRevision, authored geometry and dirty flag untouched. |

## 5A. Consolidated static SVG visual mockups — ready for Owner's final D2 review

The three detailed visual renditions were prepared together under [`UI-01_VISUAL_GALLERY_D2_OWNER_REVIEW.md`](UI-01_VISUAL_GALLERY_D2_OWNER_REVIEW.md), including (1) normal-height Part Feature Edit with 40/60 panels, (2) width-compact Sketch Edit with two stacked right panels, and (3) height-short Part Feature Edit with Operations-selected tabs. **Each scenario's design behavior has individual Owner acceptance; the final combined visual appearance, actual icon assets, exact Windows/Qt geometry and interactive QA do not.** SVGs are intentionally static proposals, not screenshots of executable Qt software, so an additional explicit final visual/behavioral approval is required.

## 6. Remaining visual decisions — NOT APPROVED by this wireframe

1. Inspect normal/compact/narrow **native Windows screenshots at real DPI/font scaling** with representative long Operations content and the original Owner witnesses; tune actual containment thresholds from minimum effective widget geometry and hysteresis.
2. Final visual samples for button icon silhouettes, label abbreviations and tooltip wording; hover vs engaged vs focus vs disabled and pinned indicator emphasis in both light and dark themes where applicable.
3. **Default group order now Owner-accepted**; still review visual spacing of independent fixed Document commands, and popup style/focus behavior for named group and two-level More menus.
4. Operations header and fixed footer size / form-field focus at short height; check viewport cue collision with ViewCube and minimum viewport.
5. After Owner **explicit visual/behavioral signoff**, draft separately bounded UI-01 Work Contract, seek independent Owner acceptance and main governance activation. Only then can staged P0 -> P1 product implementation begin with exact-SHA Windows test gates and separate Owner final pass/merge approval.

**Scope guard:** PR #320 remains governance/D2 design-only. PG-01E PR #311 remains unaccepted; no PM-06 or future feature scope is activated by these wireframes.
