# SR-03 — Responsive Workbench Shell

**Status:** ACTIVE  
**Proposed:** 2026-10-02  
**Owner acceptance:** 2026-10-02  
**Decision class:** bounded D1 Workbench/UI composition under ADR-0006; STOP for D2 if DocumentKind ownership, public Viewer contracts, CAD input ownership or subsystem dependency direction must change  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0006, ADR-0010, ADR-0011  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.9  
**Baseline:** `main` at `e48b377b216c59d595d3f8f01a8ea5cd139d7b85` after completed SR-02  
**Milestone:** pre-Part stabilization — final package of SR-01 → SR-02 → SR-03 → readiness re-test

## 1. Goal

SR-03 removes the remaining concrete shell/layout blocker found by the post-R12 Sketcher profile-authoring readiness review.

The package makes the current Project Workspace and active Document Workbench behave as a compact, predictable CAD shell across ordinary desktop window sizes while preserving all existing CAD semantics and ownership.

The accepted Owner direction is:

- Project identity/path information must no longer consume three vertical text rows;
- the active Document's domain/tool surface must align in the same top row as Undo / Redo / Save / Close;
- left Document Tree and right Properties/Operations panels must start substantially narrower than the current shell, approximately matching the accepted Owner screenshot proportions;
- the central Editor/Viewport is the dominant stretch surface;
- the geometry of the shell is a common Document Workbench template for current Part and future Assembly/Drawing kinds, while each DocumentKind remains free to provide its own tools, properties and contextual operations;
- responsive behavior must remain controlled and usable at compact/narrow widths rather than merely squeezing all three columns.

SR-03 is a shell/layout stabilization package. It is not a visual redesign of the complete application and does not add CAD capabilities.

## 2. Preserved architecture and ownership

ADR-0006 remains authoritative:

- Project Workspace owns Project/document-launch surfaces;
- an active CAD Document owns its editing Workbench;
- Document editing actions such as Undo / Redo / Save / Close remain Document actions;
- the editor tool surface launches tools for the active Document kind/context;
- the right Operations surface remains contextual and is not a permanent tool catalog;
- future Part/Assembly/Drawing routing is based on DocumentKind and must not make the neutral Workspace pretend to be one CAD domain.

SR-03 may change only composition, sizing, responsive presentation and bounded shell plumbing needed to host those existing surfaces.

It must not move domain authority into UI widgets, introduce a second CAD input owner, change DocumentSession semantics, change semantic selection, or make provider-native state durable.

## 3. Frozen shell template

The active Document Workbench has one common geometric template:

```text
+-------------------------------------------------------------------+
| Document/domain tool surface                  Undo Redo Save Close |
+--------------+------------------------------------+---------------+
| Document     |                                    | Properties    |
| Tree         |          Editor / Viewport          | +             |
|              |                                    | Operations    |
+--------------+------------------------------------+---------------+
| Workbench status / existing lower surfaces as applicable          |
+-------------------------------------------------------------------+
```

This is a geometric/composition template, not a requirement that future Document kinds share the same concrete widgets or tool vocabulary.

For current Part:

- the left surface is the Part Document Tree;
- the center surface is the existing Editor/Qt-OCCT Viewport;
- the right surface hosts current Properties and contextual Operations;
- the top domain/tool slot hosts Part/Sketch tools according to the active context.

For future Assembly/Drawing, the same shell geometry may host kind-specific tree/content/tool/property/operation surfaces under separate future contracts.

SR-03 must not implement fake Assembly/Drawing support merely to prove this template.

## 4. Workspace information strip

The Project Workspace header currently uses separate vertical rows for Project, ProjectId and Workspace path.

SR-03 replaces that presentation with one compact information row.

The row must expose the same current information:

- Project display name;
- ProjectId;
- Workspace path.

Long values may elide visually when required by width, but the complete value must remain inspectable without changing semantic state, for example through a tooltip or equivalent read-only detail.

The shell must not use multi-line wrapping of the Workspace path as the ordinary default layout.

The existing Workspace / New Part… / Open… / Refresh / Close Project action ownership remains unchanged.

## 5. Unified active-document top row

The existing separate rows for Document actions and editor tools are replaced compositionally by one top row.

The row has two ownership-preserving zones:

- **left / expanding zone:** active Document/domain/context tool surface;
- **right / fixed zone:** existing Document actions Undo / Redo / Save / Close.

Visual alignment in one row does not merge their semantic ownership.

For current Part/Sketch behavior:

- normal Part context may expose Sketch and future Part tools in the left zone;
- Sketch Edit exposes the existing Sketcher tool family in that same zone;
- Profile/other contextual right-panel Operations remain contextual and do not migrate into the top tool catalog unless separately accepted by their owning contract.

The top row must not duplicate the same semantic action in multiple competing launch surfaces.

## 6. Default panel geometry

At a normal maximized desktop Workbench, the accepted visual target is approximately:

- left Document Tree: about **150 px**;
- right Properties/Operations panel: about **270 px**;
- center Editor/Viewport: all remaining width.

These are default layout targets, not durable semantic values and not a requirement to force exact pixels across DPI/font/theme environments.

Allowed D1 tuning may choose nearby values based on actual Qt size hints, DPI and platform metrics provided that manual Windows verification preserves the accepted visual proportions.

The central Editor/Viewport owns the majority of additional horizontal space as the window grows. Side panels must not expand proportionally merely because more width is available.

User splitter adjustment may remain available where it does not break the responsive invariants below.

## 7. Responsive behavior

SR-03 must define and implement three effective width regimes:

### Normal

There is enough width for:

- left Tree;
- center Editor/Viewport;
- right Properties/Operations;
- complete unified top row.

Default panel proportions follow Section 6 and the center surface receives the stretch surplus.

### Compact

When the window becomes narrower:

- the center Editor/Viewport remains the priority work surface;
- side panels may reduce only to usable bounded minima;
- top-row tools may compact according to existing context without changing command meaning;
- no control may overlap another shell region;
- native Viewer/navigation composition must remain valid.

### Narrow

When three usable columns can no longer coexist:

- the shell must use a controlled responsive presentation for one or both side surfaces rather than indefinitely shrinking the Viewer;
- hidden/collapsed side content must remain deliberately recoverable by the user;
- layout state changes must not change semantic selection, active Sketch/Profile context, current command, Document dirty state or history;
- automatic transitions must be stable around thresholds and must not visibly oscillate during ordinary resize.

The exact breakpoint pixels and the exact bounded mechanism used for compact/narrow presentation (for example controlled collapse, tabbed side surface, drawer-like host or an equivalent local Qt composition) are delegated D1 decisions.

The implementation must choose the smallest mechanism that satisfies the contract. It must not introduce a general docking/window framework without evidence that the bounded shell cannot satisfy the requirements.

## 8. Focus and CAD input invariants

Responsive reflow is presentation only.

It must preserve:

- workspace-global CAD input ownership from ADR-0011;
- text-editor local keyboard ownership;
- Ctrl+Z / Ctrl+Y behavior accepted by SR-01;
- Sketch keyboard-first tools and Dynamic Input;
- Command Line routing;
- semantic selection and primary selection;
- active manipulation/preview semantics;
- active Sketch/Profile edit context;
- Escape/Enter/Space behavior;
- no authored transaction merely because a panel reflows, collapses, restores or changes size.

A widget being temporarily non-visible due to responsive layout cannot become a second authority for its semantic content.

## 9. Viewer and Navigation Cube invariants

WB-01A/WB-01B and ADR-0010 remain authoritative.

Resize/reflow must preserve:

- the provider-surface Navigation Cube;
- projection control;
- native Qt/OCCT viewport composition;
- selection rectangle presentation;
- Pan/Orbit/Zoom;
- no stale native pixels or black/Workspace reveal;
- no Viewer minimum-width behavior imposed by obsolete QWidget overlays;
- camera runtime-only semantics.

If the responsive shell requires changing public `IDocumentViewport` ownership or moving shell responsibility into the Viewer provider, STOP for Owner D2 review.

## 10. Implementation order

The first implementation slice must establish mechanical shell geometry before styling polish.

Recommended sequence:

1. compact single-row Project information strip;
2. unify active Document tool/actions into one row while preserving separate ownership zones;
3. set normal default Tree/right-panel geometry and center stretch priority;
4. add measured compact/narrow responsive behavior;
5. add resize/focus/state regression coverage;
6. update internal and PL/EN Product documentation plus generated Browser;
7. exact-head Windows FULL;
8. Owner manual Windows verification;
9. work-only CLOSURE closeout.

No later step may silently broaden the scope into a general docking framework or future DocumentKind feature work.

## 11. Automated acceptance evidence

Automated coverage must include at least:

- normal shell geometry exposes left/center/right surfaces without overlap;
- initial side widths are within accepted D1 tolerance around the Section 6 targets;
- center Editor receives stretch surplus as total width grows;
- repeated width changes across normal/compact/narrow regimes do not crash;
- responsive transition does not mutate active Document revision/history;
- active Sketch context survives width transitions;
- selection state survives width transitions;
- text-editor focus remains text-editor-owned;
- active CAD viewport focus still routes CAD input correctly;
- Navigation Cube remains contained in the Editor/Viewport surface;
- native overlay/selection-box regressions remain PASS;
- switching Workspace ↔ active Document retains the correct shared shell state;
- multi-document tab switching does not leak one Document's contextual tool/property state into another;
- existing Part/Sketch/Profile interaction tests remain PASS.

Where pixel geometry is asserted, tests must tolerate normal DPI/font metric variation and test semantic containment/proportion rather than brittle screenshot-perfect coordinates.

## 12. Manual Windows verification

Owner manual verification on the exact runtime candidate must include:

- maximized/normal Workbench visual comparison with the accepted layout direction;
- Project information shown compactly in one row;
- Part/Sketch tool surface aligned with Undo / Redo / Save / Close;
- left Tree and right Properties/Operations start at the accepted narrow proportions;
- aggressive narrow ↔ wide resize;
- deliberate recovery/use of side panels in the narrow regime;
- Sketch Edit with Line/Circle/Arc plus Move/Copy smoke while resizing;
- Profile properties/operations smoke;
- multi-document tab switch;
- Command Line and text field keyboard ownership;
- Navigation Cube, Pan/Orbit/Zoom and Window/Crossing selection during/after resize;
- no stale Viewer pixels, overlap, inaccessible required controls or unexpected layout oscillation;
- Save → Close → Reopen regression.

Owner PASS is required for completion.

## 13. Deliberately out of scope

SR-03 does not authorize:

- AssemblyDocument or DrawingDocument implementation;
- new Part Feature Tree architecture;
- solid modeling / Extrude;
- new Sketch entities or tools;
- Split / Join;
- constraints, solver or authored dimensions;
- ordinary Select RMB context-menu design;
- public docking/plugin framework;
- persistence of arbitrary window geometry unless already owned by an accepted existing contract;
- new public Viewer APIs;
- changed CAD input semantics;
- changed persistence/schema/Document identity;
- redesign of Navigation Cube semantics;
- broad visual theme/icon redesign.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SR-03 materially changes the visible Workspace/Workbench composition, responsive behavior and location of existing Document tool/action surfaces while preserving their semantics.

Internal documentation must describe the common shell geometry, responsive regimes, ownership-preserving unified top row and focus/Viewer invariants.

Product documentation PL/EN must reflect the final visible Workspace header and active Document Workbench layout.

The deterministic Product Browser must be regenerated from canonical Markdown and pass freshness verification.

## 15. Delegated D1 tuning

After activation, implementation may tune without further Owner decision:

- exact nearby default Tree/right widths around the accepted ~150 / ~270 px direction;
- exact normal/compact/narrow breakpoint pixels;
- bounded minimum widths;
- exact compact/narrow Qt composition mechanism;
- button spacing, margins and elision width;
- local helper/widget extraction;
- responsive hysteresis needed to prevent threshold oscillation;
- test geometry tolerances;
- internal object names and test placement.

D1 tuning may not change semantic ownership, add new document kinds, hide required functionality without a deliberate recovery path or introduce a general new docking architecture.

## 16. Stop conditions

STOP for Owner review if implementation requires or attempts:

- changing ADR-0006 ownership of Project Workspace, Document Workbench, editor tools or contextual Operations;
- moving Undo/Redo/Save/Close semantic ownership merely to achieve visual alignment;
- new public DocumentKind/Workbench plugin architecture;
- public Viewer API/ownership change;
- new CAD input routing/ownership;
- persistence/schema or semantic state tied to panel geometry;
- a general docking/window manager;
- fake Assembly/Drawing functionality;
- SR-03-driven Part Feature Tree or solid-modeling work;
- changes to Sketch/Profile/selection/history semantics.

## 17. Activation and completion boundary

The Owner explicitly accepted the SR-03 design direction on 2026-10-02:

- compact one-row Workspace information;
- one aligned active-Document top row with domain/context tools left and Undo/Redo/Save/Close right;
- common geometric Workbench template across current/future Document kinds without implementing those future kinds;
- approximately 150 px left Tree and 270 px right Properties/Operations defaults at normal desktop width;
- center Editor/Viewport as the dominant stretch surface;
- controlled normal/compact/narrow responsive behavior with exact thresholds/mechanics delegated as D1.

The synchronized activation candidate may update `work/ACTIVE.yaml` and Roadmap v1.9 to SR-03 ACTIVE. Production layout mutation begins only after that work/governance candidate passes the repository gate.

Completion requires:

- required automated responsive/focus/Viewer regressions;
- required internal and PL/EN Product documentation;
- regenerated deterministic Product Browser;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- work-only CLOSURE closeout.

After SR-03 completion, production scope stops again. The next action is the explicit Sketcher profile-authoring readiness re-test. Part Feature Tree and solid modeling remain inactive behind their separate architecture gate.
