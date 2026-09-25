# SK-04C — End-to-end Select, Continuous Line and Delete Workflow

**Status:** ACCEPTED  
**Owner acceptance:** 2026-09-25  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.1  
**Roadmap milestone:** R4 — First complete continuous Line workflow  
**R4 delivery split:** SK-04A / SK-04B / SK-04C

## 1. Context

R3 completed provider-neutral active-Sketch presentation and spatial pointer transport.

SK-04A completed the host-neutral runtime semantic state:

```text
SketchInteractionState
  Select
  Line / AwaitFirstPoint
  Line / AwaitNextPoint

semantic transient selection
  EntityId[]
  optional primary EntityId

Line request/acknowledgement
atomic EraseSketchEntitiesCommand
```

SK-04B completed the provider-neutral view-space selection bridge:

```text
logical screen point
  → current authored Sketch presentation query
  → PresentationToken
  → SketchId + EntityId

logical screen rectangle
  → Window/Crossing presentation query
  → PresentationToken[]
  → EntityId[]

runtime selection-box overlay
independent pointer routing and cursor mode
```

R4 is not complete until those layers are wired into one actual user-facing workflow in the shared Part Sketch edit context.

## 2. Goal

Deliver the first complete interactive authored Line lifecycle in the existing 3D Document Viewport:

```text
enter Sketch
→ Select is active
→ activate Line
→ click first point
→ rubber-band preview
→ click successive points
→ authored segments commit continuously
→ finish/cancel Line returns Select
→ point/rectangle select authored Lines
→ Delete selected Lines atomically
→ Undo/Redo behaves predictably
→ Finish Sketch leaves runtime edit context
```

SK-04C must also introduce the compact Sketch Command Line and contextual Operations adapters to the same single interaction authority.

Completion of SK-04C completes roadmap milestone R4.

## 3. One interaction authority

### 3.1 Ownership

There must be exactly one live `sketch::SketchInteractionState` for the active Sketch edit context.

The preferred bounded implementation is a Part/UI interaction coordinator owned by `CadWorkbench` (for example `PartSketchInteractionController` or equivalent) that composes:

```text
SketchInteractionState
DocumentSession
active SketchId
PartViewportController
runtime pointer-drag state
runtime tool/prompt callbacks
```

Exact private class/file naming is implementation detail.

The coordinator must not own authored Sketch geometry. Authored mutation remains only through existing semantic DocumentSession commands.

### 3.2 No duplicate state machines

The following are adapters only:

- toolbar Select/Line controls;
- viewport mouse/pointer input;
- Operations controls;
- compact Command Line;
- Esc/Delete keyboard actions.

None may maintain an independent Line stage or semantic selection set.

## 4. Active Sketch lifecycle

### 4.1 Enter edit

Entering an existing or newly created Sketch:

```text
interaction state = Select
semantic Sketch selection = empty
preview = empty
selection box = empty
primary routing = spatial_tool_input
cursor = select_pick_box
```

The existing camera alignment/Fit behavior for initial Sketch entry remains unchanged. Pan/Zoom/Orbit/ViewCube remain global runtime navigation.

### 4.2 Leave edit

Finishing the Sketch, switching/closing Document, or fail-closed loss of the active Sketch:

- clears uncommitted Line state;
- clears semantic Sketch selection;
- clears preview and selection-box overlay;
- removes Sketch highlight projection;
- detaches runtime pointer handling for that edit context;
- restores ordinary non-edit viewport routing/cursor through the existing host lifecycle.

No authored geometry is rolled back merely because the edit context ends.

## 5. Tool surface

While Sketch edit is active, the editor tool strip exposes at least:

```text
Select
Line
```

Select is the default and visually active tool.

The existing Part-level `Sketch` launcher remains for creating a Sketch when not editing one.

Activating Select:

- cancels only uncommitted Line runtime state;
- clears Line preview;
- configures `spatial_tool_input + select_pick_box`;
- preserves already committed geometry;
- does not create history.

Activating Line:

- calls the single `SketchInteractionState::activateLine()`;
- clears transient Sketch entity selection for R4;
- clears projected selection highlight;
- configures `spatial_tool_input + create_edit_crosshair`;
- enters AwaitFirstPoint.

### 5.1 Contextual editor surfaces

The Editor Tools surface and Operations panel follow the active semantic editing context.

Outside Sketch edit, the Workbench presents the tool and Operations surfaces appropriate to the Part modeling context.

Entering Sketch edit switches those surfaces to the Sketch editing context:

- Editor Tools above the Viewport expose Sketch tools such as Select and Line;
- Operations on the right exposes contextual Sketch/tool state and actions;
- the compact Command Line is part of the central editor surface directly below the Viewport;
- the global Workbench Status remains a separate channel below the main shell;
- visual active/checked tool state is derived from the single Sketch interaction authority and is not independent UI state.

Leaving Sketch edit restores the Part modeling Editor Tools and Operations surfaces.

Conceptually:

```text
Part modeling context
→ enter Sketch edit
→ Sketch Editor Tools + Sketch Operations + Sketch Command Line
→ finish/lose Sketch edit context
→ Part modeling Editor Tools + Part Operations
```

The same restoration applies to fail-closed edit termination caused by Document switch/close, history removal of the active Sketch, or other loss of valid edit context.

This bounded Part ↔ Sketch switching is not authorization for a universal mode/framework abstraction.

### 5.2 Tool hierarchy

While Sketch edit is active:

- Select and Line are mutually exclusive tool presentations;
- their checked/active state is projected from `SketchInteractionState`;
- tool-specific actions appear in Operations above the whole-context `Finish Sketch` action;
- `Finish Line` / `Cancel Line` act only on the current Line tool;
- `Finish Sketch` exits the whole Sketch edit context.

Select and Line are not presented as active Part-modeling tools outside Sketch edit.

## 6. Provider-neutral modifier transport

SK-04C may extend neutral spatial pointer input with the smallest runtime modifier data required by R4 point selection.

Required semantic input:

```text
Control pressed?  yes/no
```

Qt modifier types must not cross the Viewer boundary.

The exact neutral representation may be a boolean or small neutral modifier struct. Do not introduce a universal keyboard framework.

Existing aggregate callers/tests must remain straightforward to migrate.

Shift+MMB Orbit remains navigation behavior and is not reinterpreted as Sketch selection modifier state.

## 7. Select pointer state

Screen-space click/drag state belongs above the provider and outside `SketchInteractionState`.

Conceptually the active Select adapter maintains only:

```text
optional primary-press anchor in logical viewport coordinates
whether rectangle drag is active
current Window/Crossing rule
```

This is transient runtime UI state, not authored or persisted state.

## 8. Point selection

### 8.1 Click recognition

A primary press begins a possible Select interaction.

If release occurs without establishing a rectangle drag, SK-04C performs the SK-04B point query at the release position.

The Control modifier used for point-selection semantics is sampled from the release event that resolves the semantic click. Press-time Control state does not become a separate selection authority.

Point query failure:

- leaves semantic selection unchanged;
- creates no authored mutation;
- reports a runtime status/diagnostic rather than silently acting as blank-space clear.

### 8.2 Selection grammar

The accepted R4 point grammar is:

```text
Left click Line        → replace selection with that EntityId
Ctrl + Left click Line → toggle that EntityId
Left click blank       → clear Sketch entity selection
Ctrl + Left click blank
                       → keep current Sketch selection unchanged
```

A click selects the Line entity, not endpoints/midpoint/grips.

After every successful semantic selection change, the host projects the current EntityIds back to current PresentationTokens through SK-04B for visual highlight.

No PresentationToken is stored in `SketchInteractionState`.

## 9. Rectangle selection

### 9.1 Drag threshold

R4 uses a small logical-screen drag threshold so ordinary click jitter does not create a box.

The implementation shall use a fixed threshold in logical viewport pixels, independent from DPR, pick tolerance, snap tolerance and geometric tolerance.

The exact constant is an interaction detail but must be explicit in code and covered by deterministic tests.

A rectangle drag starts only once both box dimensions are non-zero and the pointer movement exceeds the threshold. Otherwise release remains a point-selection attempt.

### 9.2 Direction semantics

During an active rectangle drag:

```text
current.x >= anchor.x
→ Window

current.x < anchor.x
→ Crossing
```

The selection-box overlay updates dynamically with the same rule.

On release:

- clear overlay in all outcomes;
- normalize the rectangle;
- call the SK-04B rectangle query with the explicit current rule;
- provider failure leaves semantic selection unchanged;
- successful query replaces the semantic Sketch selection with the returned EntityIds;
- rectangle selection has no primary EntityId in R4, because provider result order must not become semantic primary ordering.

R4 does not add Ctrl-additive/subtractive rectangle selection. That refinement remains available to R5.

## 10. Line pointer workflow

### 10.1 Commit phase

Line uses primary-press events to accept points.

Primary-release has no separate Line commit meaning in R4.

For a valid mapped Sketch-local point:

```text
Line / AwaitFirstPoint
primary press P0
→ acceptLinePoint(P0)
→ AwaitNextPoint, anchor=P0

Line / AwaitNextPoint
move P
→ previewLine(P)
→ R3 transient preview Panchor→P

primary press P1
→ acceptLinePoint(P1)
→ if segment_requested:
     execute AddSketchLineCommand(active SketchId, start, end)
     acknowledge success/failure to SketchInteractionState
```

### 10.2 Successful segment

On successful `AddSketchLineCommand`:

- resolve the pending request as committed;
- refresh authored presentation from DocumentSession;
- the anchor advances to the committed endpoint;
- remain Line/AwaitNextPoint;
- clear stale preview until the next pointer move;
- one segment remains exactly one transaction/revision/Undo entry.

This supports continuous:

```text
A-B
B-C
C-D
...
```

without reactivating Line.

### 10.3 Failed segment

On command rejection/failure:

- resolve pending request as failed;
- previous anchor remains;
- authored presentation/history remain unchanged;
- Line stays active in AwaitNextPoint;
- show the existing application diagnostic/status;
- no silent runtime advancement.

### 10.4 Exact-zero candidate

The SK-04A exact-zero rule remains authoritative:

- no Add command;
- no Undo/dirty/revision change;
- same anchor;
- Line remains active.

No snap/near-zero epsilon is introduced.

## 11. Finish, Cancel and Esc

### 11.1 Operations actions

While Line is active, Operations exposes contextual actions equivalent to:

```text
Finish Line
Cancel Line
```

Both return to Select and clear uncommitted preview/runtime Line state.

Neither rolls back already committed segments.

`Finish Line` and `Cancel Line` differ only in user intent/status wording in R4; both use the accepted SK-04A no-rollback semantics.

### 11.2 Esc hierarchy

When viewport Sketch interaction has focus:

```text
Line / AwaitNextPoint
Esc
→ clear pending anchor/preview
→ Line / AwaitFirstPoint

Line / AwaitFirstPoint
Esc
→ Select

Select
Esc
→ no CAD/tool-state change
```

Esc does not finish the whole Sketch in R4.

The existing explicit `Finish Sketch` action exits Sketch edit.

### 11.3 Finish Sketch while Line active

`Finish Sketch` first discards only uncommitted Line runtime state, then exits the edit context.

Committed segments remain authored.

## 12. Delete

### 12.1 Availability

Delete is a Select-mode operation.

It may be exposed through:

- keyboard Delete while viewport Sketch Select has interaction focus;
- contextual Operations `Delete Selection`.

Delete is not triggered from text-entry focus.

### 12.2 Semantic command

For non-empty semantic Sketch selection:

```text
selected EntityIds
→ EraseSketchEntitiesCommand(active SketchId, IDs)
→ one atomic DocumentSession transaction
→ one Undo entry
```

On success:

- clear deleted EntityIds from SketchInteractionState selection;
- refresh authored presentation;
- clear/update presentation highlight;
- no automatic selection restoration.

On failure:

- authored state remains unchanged;
- runtime selection remains unchanged;
- show diagnostic.

## 13. Undo / Redo

The accepted R4 history policy is frozen:

```text
Undo or Redo requested while Line active
→ cancelForHistory()
→ clear preview
→ Select + pick-box
→ execute ordinary DocumentSession Undo/Redo
```

After every Undo/Redo while Sketch edit remains valid:

- refresh authoritative authored presentation;
- reconcile transient EntityId selection against current SketchModel;
- prune stale selected IDs;
- re-project remaining selection to current presentation tokens.

Undo restoring previously deleted geometry does not automatically reselect it.

If history removes the active Sketch itself, the existing fail-closed edit-context reconciliation closes Sketch edit.

## 14. Compact Command Line

SK-04C introduces a compact Command Line associated with the Editor Surface.

It is an adapter to the same interaction coordinator, not another command state.

### 14.1 Minimum visible state

It displays the current runtime command/prompt, for example:

```text
Command: SELECT

Command: LINE
LINE — Specify first point

Command: LINE
LINE — Specify next point
```

### 14.2 Minimum input

R4 accepts exact command-name activation, case-insensitively:

```text
SELECT
LINE
```

Submitting those names invokes the same Select/Line activation methods as the toolbar.

Unknown/non-empty input:

- produces a runtime status such as `Unknown Sketch command`;
- creates no authored mutation.

Empty submission is a no-op.

### 14.3 Focus behavior

Submitting a recognized `SELECT` or `LINE` command transfers interaction focus back to the Viewport so pointer interaction can continue immediately.

While the Command Line text field owns focus:

- Delete does not invoke CAD Delete;
- Esc first cancels/clears the text-entry interaction and does not mutate Sketch tool state;
- Qt text-editing behavior remains local to the text control.

Command Line prompt/state is distinct from the global Workbench Status channel. Prompt text describes the current command/input request; Status reports operation outcomes, failures and application messages.

### 14.4 Explicitly not R4

The Command Line does not parse:

```text
coordinates
@relative values
polar syntax
length/angle values
dynamic dimensions
repeat-last-command
history/autocomplete
aliases
RMB command menus
```

Precision input remains R7.

## 15. Operations adapter

Operations reflects the one interaction state.

Minimum behavior:

### Select

Show contextual selection state such as:

```text
Select — N Lines selected
```

Expose `Delete Selection` only/enabled when appropriate.

### Line / AwaitFirstPoint

Show:

```text
Line — Specify first point
Finish Line
Cancel Line
```

### Line / AwaitNextPoint

Show:

```text
Line — Specify next point
Finish Line
Cancel Line
```

The existing `Finish Sketch` action remains available for the whole edit context.

Operations never stores its own Line stage.

## 16. Focus and keyboard safety

R4 keyboard actions must not steal ordinary editing keys from text controls.

At minimum:

- Delete CAD action applies only to viewport/Sketch interaction focus, not Command Line or document property text editing;
- Esc tool action applies to the active Sketch interaction context and must not unexpectedly mutate text fields;
- normal QWidget/system cursor remains over UI controls;
- viewport cursor follows the active Select/Line mode.

Exact shortcut implementation may use Qt shortcuts/event filtering internally, but Qt key enums do not become domain semantics.

Right mouse button remains reserved/no-op in SK-04C. This contract does not introduce context menus, repeat-last-command, Finish/Cancel shortcuts or other RMB command grammar.

## 17. Navigation coexistence

Throughout Select and Line:

- MMB drag = Pan;
- Shift+MMB drag = Orbit;
- wheel = Zoom;
- ViewCube remains available.

Navigation:

- does not cancel the active tool;
- does not mutate authored state;
- does not create Undo/dirty/revision effects;
- keeps subsequent point/rectangle mapping based on the updated camera.

An active selection-box drag must not reinterpret MMB navigation as CAD selection input.

## 18. Presentation and refresh invariants

Authored presentation is always rebuilt from authoritative Part/Sketch state.

Runtime channels remain separate:

```text
authored Sketch scene
Line preview scene
selection-box overlay
PresentationSelection highlight
cursor/routing
```

No runtime channel owns authored CAD identity.

After each authored mutation/history refresh, semantic EntityIds are authoritative and current PresentationTokens are reacquired through SK-04B.

## 19. User-facing completeness of R4

R4 completion requires the first primitive to satisfy:

```text
create
→ present
→ select
→ delete
```

Specifically the user can:

- enter a Part Sketch;
- activate Line;
- draw multiple continuous Line segments by clicking;
- see rubber-band preview;
- finish/cancel Line back to Select;
- point-select a Line;
- Ctrl-toggle Lines;
- Window/Crossing rectangle-select Lines;
- delete selected Lines;
- Undo/Redo authored segment creation/deletion;
- finish Sketch without losing committed geometry.

No snap, constraint or dimension support is required for R4 completeness.

## 20. Deliberately OUT of SK-04C

```text
Object Snap
inference
auto-constraints
constraints/solver
dimensions
dynamic input
numeric coordinate/value entry
Line grips
direct manipulation
hover sub-element selection
endpoint/midpoint semantic selection
selection cycling
Ctrl-additive rectangle selection
selection filters
Circle/Arc
Move/Trim/Extend/Offset
construction geometry
profiles/regions
projected/reference geometry
planar-face Sketch support
inactive Sketch display
Body/Feature/Extrude architecture
```

No placeholders for those capabilities are added.

## 21. Expected implementation surface

Expected production changes are bounded to:

```text
src/sketch/** only if a narrowly required existing interaction API adjustment is unavoidable
src/viewer/include/simplesolid2/viewer/spatial_pointer.hpp
src/viewer_qt_occt/** for neutral Control modifier transport only
src/ui/cad_workbench.*
src/ui/cad_workbench_shell.* if needed for compact Command Line placement
src/ui/part_viewport_controller.* only for existing bridge use/refinement
src/ui/<bounded Sketch interaction coordinator>.* if used
tests/**
docs/internal/CAD_WORKBENCH_VIEWER.md
docs/internal/SHARED_2D.md
docs/internal/PART_DOCUMENTS.md
docs/internal/BUILD_AND_TEST.md
docs/product/en/PARTS.md
docs/product/pl/PARTS.md
work/**
```

No Part persistence schema or new authored entity type is expected.

If implementation evidence requires snapping, numeric coordinate grammar, grips, constraint semantics, new geometry primitives or persistence changes, stop rather than expanding R4 silently.

## 22. Acceptance tests

At minimum prove:

1. Entering Sketch edit creates/resets one interaction state in Select.
2. Select configures spatial primary routing plus pick-box cursor.
3. Line activation uses the same interaction state and configures crosshair.
4. Line activation clears transient Sketch entity selection/highlight.
5. Toolbar Select and Command Line `SELECT` reach the same state transition.
6. Toolbar Line and Command Line `LINE` reach the same state transition.
7. Unknown Command Line input creates no authored mutation/history.
8. First Line primary press accepts anchor without authored mutation.
9. Line move after first point creates runtime preview only.
10. Exact-zero second point creates no Add command/history.
11. Valid second point creates exactly one `AddSketchLineCommand`.
12. Successful commit advances anchor and keeps Line active.
13. Failed commit preserves previous anchor and history.
14. Three continuous clicks after anchor can author A-B, B-C, C-D with three Undo entries.
15. Finish Line returns Select and preserves committed segments.
16. Cancel Line returns Select and preserves committed segments.
17. Esc AwaitNextPoint → AwaitFirstPoint and clears preview.
18. Esc AwaitFirstPoint → Select.
19. Esc Select is a no-op.
20. Finish Sketch while Line active discards only uncommitted runtime state.
21. Point click Line replaces selection by EntityId.
22. Ctrl+point click Line toggles EntityId membership.
23. Blank click clears selection.
24. Ctrl+blank click preserves selection.
25. Point-query failure preserves selection.
26. Below-threshold pointer jitter resolves as point selection, not rectangle.
27. Rectangle drag displays the runtime selection-box overlay.
28. Left→Right drag uses Window rule.
29. Right→Left drag uses Crossing rule.
30. Rectangle release clears overlay.
31. Rectangle query replaces selection with returned EntityIds and no primary.
32. Rectangle-query failure preserves prior selection and clears overlay.
33. Provider token order is not converted into semantic primary selection.
34. Selection highlight is rebuilt from current tokens after authored presentation refresh.
35. Stale runtime tokens never enter SketchInteractionState.
36. Delete with empty selection creates no command/history.
37. Delete non-empty selection executes one atomic `EraseSketchEntitiesCommand`.
38. Multi-Line Delete is one revision/history entry.
39. Successful Delete clears deleted semantic selection/highlight.
40. Failed Delete preserves semantic selection and authored state.
41. Undo during Line cancels uncommitted Line runtime state, returns Select, then performs one ordinary Undo.
42. Redo during Line follows the same cancellation-before-history policy.
43. Undo/Redo reconciles stale selected EntityIds against the authoritative model.
44. Undo restoring deleted Lines does not automatically reselect them.
45. Undo that removes the active Sketch closes edit fail-closed.
46. MMB Pan works while Select is active.
47. Shift+MMB Orbit works while Select is active.
48. wheel Zoom works while Select is active.
49. MMB/Shift+MMB/wheel remain functional while Line is active.
50. Navigation creates no Part revision/dirty/Undo effect.
51. Line point mapping remains correct after orbit.
52. Rectangle selection remains based on current projected view after orbit.
53. Delete shortcut does not fire while Command Line/text editing owns focus.
54. UI controls use ordinary system cursor rather than viewport tool cursor.
55. Document switch/close clears Sketch interaction state, preview, overlay and selection safely.
56. Save persists committed Lines only; pending runtime Line state remains non-persistent.
57. Existing built-in Origin/reference selection behavior remains PASS outside the Sketch Select path.
58. Existing R1/R2/R3/SK-04A/SK-04B regressions remain PASS.
59. Product docs describe the actual interactive Line/Select/Delete workflow without claiming snap/constraints.
60. FULL exact-head gate passes on implementation.
61. Completion bookkeeping uses the appropriate CI-01 non-build tier.
62. Final merged tree equals the exact verified closeout tree.

## Documentation Impact

Internal docs: required  
User/Product docs: required  
Reason: SK-04C exposes the first complete user-facing Sketch Line authoring/selection/deletion workflow and completes R4.

## 24. Roadmap impact

Roadmap milestone: R4 — First complete continuous Line workflow  
Roadmap version: 1.1  
Roadmap change: none to milestone intent.

Successful SK-04C completion changes R4 status to completed and makes R5 the next roadmap milestone. It does not activate R5 automatically.

## 25. Completion

SK-04C completes only when:

- one runtime interaction authority drives toolbar, viewport, Operations and Command Line;
- continuous Line creation works end to end through semantic commands;
- point and Window/Crossing rectangle selection operate on semantic EntityIds;
- atomic Delete is wired to selected authored Lines;
- Undo/Redo and edit-context lifecycle follow the accepted runtime cancellation/reconciliation policy;
- Product and internal docs are current;
- implementation exact head passes FULL verification;
- closeout exact head passes the appropriate non-build CI tier;
- verified closeout tree equals merged tree.

Completion of SK-04C completes R4 but does not activate R5. Any R5 direct-manipulation/grip work requires a separate explicit Owner-accepted Work Contract.
