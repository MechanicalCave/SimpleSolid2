# UI-01 — Right Properties/Operations and top command bar — Owner UX REVIEW OPTIONS

**Status:** DRAFT FOR DETAILED DISCUSSION — NOT AN ACCEPTED LAYOUT OR IMPLEMENTATION CONTRACT.  
**Owner position (2026-10-11):** the UI-01 sequencing priority and P0 conflict goal are accepted in principle, **but the look AND behavior of the two right panels and top bar require detailed discussion and separate Owner approval before implementation**. See [PR #320](https://github.com/MechanicalCave/SimpleSolid2/pull/320#issuecomment-6112400422).  
**Current baseline:** accepted ADR-0006, SR-03 normal/compact/narrow template, Part/Sketch only today; future Assembly/Drawing may reuse shell presentation but receive no functionality from UI-01. Screenshots attached by Owner in discussion; no exact viewport pixels/DPI guaranteed.  
**Authority:** this file explores alternatives, not choices. No changes to `main`, `work/ACTIVE.yaml`, frozen Foundation/ADR, Part schema or runtime/UI source. Neither UI-01 Work Contract nor PG-01E is activated.

## 1. Problem and present implementation

- **R1 / right group height:** `src/ui/cad_workbench_shell.cpp` uses one right `QVBoxLayout`; Properties stretch 0 and Operations stretch 1, size policy `Ignored`. When complex Feature/Sketch forms increase in height, required controls become squeezed and hard to read or click.
- **R2 / vertical access:** no independent shell-level scroll for Properties and Operations; in long operation the lower Finish/Cancel can move out of accessible space.
- **T1 / command layout:** shell's `documentToolsScroll` is a horizontally scrollable `QScrollArea` (`ScrollBarAsNeeded`); `cad_workbench.cpp` inserts fixed QPushButtons/headings by integer position. Under resize a second row becomes a scrollbar rather than genuine responsive grouping. Document actions are already a distinct fixed right zone per SR-03.
- **P0 / conflicting mode:** `requestEditSketch()` lacks guards for active solid Feature drafts; `requestEditProfile()` can enter it indirectly. Existing Feature edit methods have some reverse guards. Reproduce with native Windows RED; preserve valid Axis/Sketch transitions rather than indiscriminately locking all nested operations.

## 2. Right panel variants — no choice yet

### R-A: vertical user splitter + independent content scrollers [recommended starting point]

Conceptual normal window:

```text
┌────────────── RIGHT PANEL (~existing default 270 px) ─────────────┐
│ Properties ▾                                                     │
│  Name, IDs, status, selected object ...                           │
│  (independent vertical scroll; all fields remain accessible)      │
├════════ user-draggable vertical splitter ════════════════════════┤
│ Operations ▾                       Editing: Extrude002            │
│  mode, distance, options, diagnostics ...                          │
│  (independent vertical scroll, no overlapping clipped controls)   │
├───────────────────────────────────────────────────────────────────┤
│ [ Finish Edit ]                          [ Cancel ]   (sticky)     │
└───────────────────────────────────────────────────────────────────┘
```

Behavior proposal: Drag splitter to assign vertical space within same right column. A panel may become compact but its **header and path to content** must remain discoverable; scrollbar lives inside that panel only. Context-specific Finish/Cancel for a currently active mutating tool lives in the *Operations* footer, not in a second command owner. Idle cases without active Finish/Cancel do not reserve a giant blank footer. Selection changes may update Properties without resetting the active Operations draft. Switching Documents rebinds contextual content without leaking another Document's edit state. No persistence of splitter position across application sessions **unless later specifically accepted**.

**Risks:** On a very short window, two minimum usable panes plus footer may not fit. Qt `QSplitter` by itself does not guarantee all content accessibility without scroll/fallback.

### R-B: mutually exclusive Properties | Operations tabs

One right column shows one full-height pane at a time, preserving the full vertical area. An active edit may automatically select Operations once on entry; selection may show an unobtrusive Properties status/badge without forcibly hiding active controls. Every content pane has its own vertical scroll. Main downside: cannot see geometry properties and tool parameters at the same time. Manual switching can become costly during inspection.

### R-C: collapsible accordion panels

Properties and Operations each have header and expand/collapse affordance; opening the other one may compress or auto-collapse the first. Simpler than drag handles but makes allocation less precise and can introduce jumps on changing selections. Must never auto-collapse the active operation so its required Finish/Cancel becomes hidden.

### Recommended trial: R-A with a short-height fallback subject to Owner choice

Prototype R-A at normal/compact; when height is insufficient for two meaningful panes, **either** R-B tabs temporarily **or** stacked collapsible headers with direct navigation. This fallback is **NOT authorized yet**. The breakpoint must come from measured effective DPI/font minimum content heights, not a single hard-coded desktop pixel number. Test 1440×900, 1280×720, 1024×768 and actual Owner-supported display/DPI where available. Keep left Tree and central Viewport widths governed by SR-03; do not enlarge app minimum width due to right forms.

**Detailed Owner decisions needed:**
- R1: simultaneous stacked panels with draggable divider, or tabbed panel only?
- R2: if stacked, prefer fallback tabs vs collapsible accordion when window is short?
- R3: should active-operation Finish/Cancel remain in sticky footer or scroll with form? Recommendation: sticky footer.
- R4: remember splitter proportion **per open Document for this session**, or reset to layout default on each Document? Neither creates CAD authored state; choose explicitly.
- R5: whether top Properties follows Tree selection during a running Feature draft or locks onto editing Feature (with inspect-other command). Must define to avoid accidental draft displacement.

## 3. Top toolbar variants — no choice yet

### T-A: one-row grouped contextual command strip with adaptive overflow [recommended starting point]

Preserve **fixed** Document actions on the right and a **context-specific** command region on the left.

```text
WIDE — Part:
[ Part ▾ ] [ Sketch ] [ Datum ] | [ Extrude ] [ Revolve ] [ Fillet ] [ Chamfer ] [ More ▾ ]  [ Undo Redo Save Close ]

WIDE — Sketch edit:
[ Sketch ▾ ] [ Select ] | [ Line Circle Arc Rectangle ] | [ Profile ] | [ Project Geometry Trim Extend ] | [ More ▾ ]  [ Undo Redo Save Close ]

COMPACT — Sketch edit:
[ Sketch ▾ ] [ Select ] [ Create ▾ ] [ Modify ▾ ] [ More ▾ ]                   [ Undo Redo Save Close ]
```

These are **concept sketches**; no choice of exact labels, icons or number of groups yet. Use stable action identities and one existing command callback independent of whether the action appears as a full button, compact button, drop-down or overflow menu. For normal width show clear labels/icons; shrink action representation or group into menus as needed. Never introduce a horizontal scrollbar or a second toolbar-height band under width pressure. Context indicator must not imply that Sketch and a solid Feature are editing simultaneously.

**Keyboard:** action menus usable with keyboard and accessibility names, existing Command Line commands continue working, Escape/Enter/Space and text-editor focus preserve accepted ownership. Menus/presentation changes cause no document mutation and no extra history entry. Undo/Redo/Save/Close stay visible within accepted min width; narrower layouts should use icon/tooltip variants without hiding the semantic action.

### T-B: navigation categories / ribbon-style contextual tabs

Top categories like `Part | Sketch | Inspect`, each opening a second surface with fewer commands. Pros: scales to very large command catalogs. Cons: consumes extra vertical viewport space, requires category navigation and risks confusing actual edit mode with navigation tab. A full docking/ribbon framework is **out of scope** until specifically justified and approved.

### T-C: compact command launcher + Search/Command Palette

Expose a few primary actions and a discoverable `Commands` dropdown/search for the rest. Pros: highly scalable; cons: hides commonly-used CAD commands behind multiple clicks. Search and shortcut persistence are new features requiring explicit design. A search palette is not authorized simply by designing T-A overflow.

**Detailed Owner decisions needed:**
- T1: prefer one-row adaptive groups (T-A) or ribbon-style categories (T-B)?
- T2: full text for frequent actions versus icon+tooltip at compact sizes?
- T3: which commands are pinned *per context*, which eligible for overflow, and should user customization be deferred?
- T4: is `More ▾` acceptable for overflow, or prefer named group menus (`Create ▾`, `Modify ▾`) at compact size?
- T5: active Feature/Sketch indicator in toolbar vs right Operations header (or both non-duplicated cues)?

## 4. Non-negotiable boundaries and acceptance plan

- **P0 FIRST:** no concurrent incompatible Feature Draft and Sketch/Profile edit. Fail closed with precise message. No auto-Finish, auto-Cancel, draft replacement, model mutation, new Undo/Redo item or state leak on denied entry. Existing valid Axis-in-Sketch workflows must remain available with explicit compatibility evidence.
- One shared document shell policy for Part and Sketch presentation; future Assembly/Drawing **only consumers after separate contracts**. Neutral Project Workspace stays separate. Right Operations stays contextual (ADR-0006) and not a duplicate permanent launcher.
- Existing responsive normal/compact/narrow geometry (SR-03), viewport and ViewCube containment, focus and shortcuts, editable CAD input, R1 Edge Picking, stage-aware selection, schema v16/ModelingSemanticsVersion 2 and native Document lifetime stay unchanged.
- Native Qt regressions should exercise long synthetic-but-real form content, repeated selection/context switches, Feature Edit → rejected Sketch entry → Finish/Cancel → allowed entry; layout min geometry, viewport widths, reflow; access to all operations/commands through menu and scroll; and no authored revision/dirty mutation due to reflow. Manual Owner comparisons use the original Windows screenshots and accepted alternate heights, plus real keyboard/mouse/resize.
- Require exact SHA Windows FOCUSED/FAST/FULL Core+Kernel+Desktop+docs+aggregate, then manual Owner UI-01 FINAL PASS and **distinct merge approval**. No test thinning, no UI pseudo-acceptance based on a sketch alone.

## 5. Review path — before ANY U2/U3 implementation

1. Owner selects the **right-panel interaction concept**, including short-height behavior and whether actions stay sticky. Record accepted R1–R5.
2. Owner selects the **toolbar information architecture**: T-A/T-B/T-C or amended, mandatory visible actions, compact behavior, command grouping. Record accepted T1–T5.
3. Produce owner-reviewed mockups/precise wireframes for normal/compact/narrow windows plus action matrix; **explicit Owner visual/behavioral signoff**. This may require another D2 revision cycle.
4. Only then write a bounded UI-01 Work Contract with sequence P0 → approved P1 variants, allowed modules/tests, doc impact, and independent gating; obtain separate Owner Work Contract approval and main governance activation.
5. **No product implementation from this discussion document alone.** PG-01E #311 remains unaccepted and open. PM-06 remains blocked.

## Documentation impact

Internal docs: **not required for this review-only proposal**.  
Product docs PL/EN: **not required for this review-only proposal**.  
Browser: **not required for this review-only proposal**.  
Later implemented behavior requires internal and PL/EN as-built updates plus deterministic generated Browser validation.
