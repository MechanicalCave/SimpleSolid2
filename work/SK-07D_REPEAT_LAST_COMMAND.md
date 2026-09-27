# SK-07D — Repeat Last Command

**Status:** COMPLETED  
**Proposed:** 2026-09-27  
**Owner acceptance:** 2026-09-27  **Owner manual Windows verification:** PASS — 2026-09-27  
**Final exact-head Windows FULL:** #567 — `ef84954ddb22a4263d31c5a540fe7d96304e33e2` — PASS  

**Decision class:** D2 Sketch command-grammar semantics + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009, ADR-0010  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.2  
**Milestone:** R7 — Common transforms, Copy and command grammar — fourth bounded slice

## 1. Goal

Add bounded **Repeat Last Command** behavior for the active Part-hosted Sketch edit session.

In ordinary Select with viewport CAD focus and no active transient command/manipulation:

- Enter repeats the last repeatable Sketch command;
- Space repeats the last repeatable Sketch command.

This slice does not implement ordinary-Select RMB context, semantic Space CycleEditMode, grip Copy modifier or later precision-input grammar.

## 2. Preserved architecture

SK-07D preserves the completed SK-07A/SK-07B/SK-07C architecture:

- one `SketchInteractionState` remains the semantic runtime interaction authority;
- toolbar and Command Line are adapters to the same Sketch command activations;
- selection is transient semantic state keyed by EntityId;
- provider/Qt/OCCT identity never becomes command identity;
- durable CAD mutation remains Command → validation → transaction → Part Document;
- creation tools preserve pre-existing selection;
- Move/Copy/Rotate/Scale/Mirror retain their existing selection-first/command-first behavior;
- active transient commands retain their existing Enter/Space/Esc hierarchy;
- text-entry focus keeps text-entry semantics.

Repeat Last Command is runtime command-grammar state only. It is not authored CAD state and is not persisted.

## 3. Repeatable command identity

The repeatable Sketch command set proposed by SK-07D is:

```text
LINE
CIRCLE
ARC
MOVE
COPY
ROTATE
SCALE
MIRROR
```

The remembered value is only command identity.

It does not remember or replay:

- prior Base/Reference/axis/placement points;
- prior command selection snapshot;
- prior primary selection;
- pointer locations;
- transient preview;
- copied EntityIds;
- numeric values or dynamic-input fields;
- future snap/inference state.

When repeated, the command starts exactly as a fresh activation would start at that moment.

## 4. Commands that do not become the repeat target

The following do not replace the remembered repeatable command:

- SELECT;
- Delete Selection;
- Finish Sketch;
- Undo/Redo;
- Save;
- ordinary selection operations;
- grip-started direct manipulation;
- owner-only Reshape;
- unknown/failed Command Line input;
- Esc/cancel;
- the Repeat Last Command adapter itself.

This slice does not define later RMB context commands as repeatable.

## 5. Recording rule

A repeatable command becomes the remembered command only after its explicit activation succeeds.

Explicit activation means an accepted activation through the existing toolbar or Command Line adapter.

Examples:

- toolbar `Line` successfully activates → last repeatable command becomes LINE;
- Command Line `MOVE` successfully activates → last repeatable command becomes MOVE;
- an activation rejected because there is no valid active Sketch does not replace the remembered command;
- pressing Repeat Last Command to start MOVE again does not create a second distinct history concept; the remembered identity remains MOVE.

Grip-started manipulation does not update the remembered command.

## 6. Lifetime

The remembered command is scoped to one active Sketch edit runtime session.

It is initialized empty when Sketch edit begins and is cleared when that edit session ends, including:

- Finish Sketch;
- active Sketch loss;
- Document switch;
- controller/runtime teardown.

SK-07D does not persist the remembered command across Documents, Sketches, application restart or file reopen.

A later contract may broaden runtime scope if concrete UX evidence requires it.

## 7. Enter / Space precedence

Repeat Last Command is available only when all of the following are true:

- Sketch edit is active;
- current semantic tool is ordinary Select;
- there is no active direct manipulation;
- there is no active common-transform stage;
- viewport CAD interaction has focus;
- no text-entry control owns the key event.

Existing key semantics keep priority:

- Enter during active direct manipulation still commits that manipulation;
- Enter/Space during common-transform `Select objects` still completes object collection when valid;
- Enter during a final transform stage still commits the current valid preview;
- Space during text-entry focus remains a literal text-space;
- Enter in the Command Line continues to submit the Command Line;
- Esc keeps its existing cancellation/selection semantics.

Semantic Space CycleEditMode during active grip manipulation remains out of scope and reserved for a later contract.

## 8. Repeat behavior with current selection

Repeating a command reuses only the **current** persistent runtime selection through the command's already accepted entry grammar.

Examples:

- repeat MOVE with a non-empty current selection → selection-first MOVE;
- repeat MOVE with empty selection → command-first Select objects;
- repeat COPY with a non-empty current selection → selection-first COPY using a new Base Point and new placement session;
- repeat ROTATE/SCALE/MIRROR follows their current selection-first/command-first rules;
- repeat LINE/CIRCLE/ARC preserves the current selection exactly as ordinary creation-tool activation already does.

No selection snapshot from the prior invocation is restored.

## 9. No remembered command

If ordinary Select receives Enter/Space with no remembered repeatable command:

- no semantic command starts;
- selection is unchanged;
- authored state/revision/history is unchanged;
- no durable state changes.

A bounded status message such as `No repeatable Sketch command.` is permitted.

## 10. Runtime ownership

The remembered command identity must have one runtime semantic owner.

Preferred implementation direction:

- represent the remembered command in the existing Sketch interaction/controller runtime path using the existing `SketchTool` command identity where practical;
- toolbar and Command Line activations update the same remembered value through one activation path;
- viewport Enter/Space invokes one Repeat Last Command adapter.

Do not implement independent toolbar-last-command and Command-Line-last-command stores.

If implementation would require a second semantic interaction authority, stop for Owner review.

## 11. Automated acceptance coverage

At minimum verify:

1. initial Sketch edit has no repeatable command;
2. successful LINE activation records LINE;
3. successful CIRCLE/ARC activation records the correct identity;
4. successful MOVE/COPY/ROTATE/SCALE/MIRROR activation records the correct identity;
5. SELECT does not replace the last repeatable command;
6. Delete/Undo/Redo/selection/grip manipulation do not replace it;
7. Esc/cancel does not erase it;
8. repeat from ordinary Select activates the remembered command;
9. repeated transform uses current selection-first behavior when current selection is non-empty;
10. repeated transform uses command-first Select objects when current selection is empty;
11. repeated creation tool preserves current selection;
12. repeated COPY starts a fresh Base Point/placement session and does not reuse prior transient points;
13. Enter and Space both repeat in ordinary Select with viewport focus;
14. active direct-manipulation Enter retains commit precedence;
15. common-transform Enter/Space behavior retains precedence;
16. Command Line text focus does not use Space as repeat;
17. empty Command Line Enter remains Command Line behavior, not viewport Repeat Last Command;
18. no remembered command is a no-op;
19. remembered identity resets on end/begin Sketch edit;
20. remembered identity does not leak across active-Sketch/Document switch;
21. existing Move/Copy/Rotate/Scale/Mirror and creation regressions remain green;
22. final runtime candidate passes exact-head Windows FULL;
23. required documentation and generated Browser freshness pass.

## 12. Manual Windows verification

Final candidate requires Owner verification of:

- activate LINE, exit to Select, press Enter → LINE starts again;
- activate CIRCLE/ARC and repeat with Enter/Space;
- activate MOVE, finish/cancel back to Select, repeat with Enter/Space;
- repeat MOVE with non-empty selection uses selection-first;
- repeat MOVE with empty selection enters Select objects;
- repeat COPY starts a fresh Base Point/placement session rather than reusing previous placement;
- SELECT, ordinary selection and Delete do not overwrite the remembered command;
- Esc from a command leaves that command repeatable;
- Space in Command Line/text focus types text and does not repeat;
- active transform Enter/Space behavior is unchanged;
- Finish Sketch/re-enter Sketch clears the remembered command;
- no regressions in COPY repeated placement, Rotate/Scale/Mirror, grips/reshape, Undo/Redo, Save/reopen or navigation.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-07D introduces user-visible keyboard command grammar and a new runtime command-memory rule that maintainers must understand.

## 14. Expected implementation surface

Expected bounded production changes after Owner acceptance:

- `src/sketch/**` and/or `src/ui/**` for one remembered command identity and repeat adapter;
- `src/application/**` only if existing controller/runtime boundaries require a small adapter, with no durable command/persistence change;
- tests;
- affected internal/Product docs and generated Browser;
- `work/**`.

No persistence schema change is expected.

Stop for Owner review if implementation requires:

- persisted command history;
- cross-Document or cross-Sketch repeat state;
- changing selection semantics;
- changing existing transform geometry or identity behavior;
- ordinary-Select RMB context implementation;
- semantic Space CycleEditMode;
- grip Copy modifier;
- numeric parser/Dynamic Input;
- snapping/inference;
- a second semantic interaction authority.

## 15. Explicitly out of scope

SK-07D does not authorize:

- ordinary-Select RMB context menu;
- semantic Space CycleEditMode;
- grip Copy modifier;
- Rotate/Scale/Mirror+Copy modifier behavior;
- Reshape+Copy;
- clipboard/cross-Sketch/cross-Document Copy;
- numeric angles/distances/coordinates/scale factors;
- Dynamic Input;
- Object Snap/tracking/inference;
- Ortho/Polar/Grid Snap;
- constraints/solver/authored dimensions;
- R8+ work.

## 16. Activation and completion boundary

SK-07D was explicitly Owner-accepted on 2026-09-27 and is active within this bounded scope.

Completion requires:

- one remembered repeatable command identity for the active Sketch edit session;
- repeatable set limited to Line/Circle/Arc/Move/Copy/Rotate/Scale/Mirror;
- viewport Enter and Space repeat only from ordinary Select;
- existing active-command/text-focus key precedence remains unchanged;
- repeat starts a fresh command invocation using current selection and current runtime preferences only;
- no persistence or durable CAD semantics change;
- required internal and PL/EN Product documentation is current;
- final candidate passes exact-head Windows FULL;
- Owner manual Windows verification passes;
- closeout CLOSURE passes;
- remaining R7 and R8+ work stays inactive until separately accepted.

## 17. Completion evidence

SK-07D completed its bounded fourth R7 slice on 2026-09-27.

- One runtime-only last-repeatable-command identity is owned by the existing Part Sketch interaction controller for the active Sketch edit session.
- The repeatable set is limited to Line/Circle/Arc/Move/Copy/Rotate/Scale/Mirror.
- Viewport Enter/Space in ordinary Select repeats the remembered command through the existing activation paths.
- Repeat starts a fresh command invocation and uses current selection; prior points, previews, selection snapshots and COPY placement state are not replayed.
- Existing key precedence remains intact for active manipulation/transforms and text-entry focus.
- Select, ordinary selection changes, Delete, grip manipulation, Undo/Redo and Esc do not replace the remembered command.
- The remembered command is cleared when Sketch edit begins/ends and does not persist across Sketches, Documents or reopen.
- Required internal and PL/EN Product documentation is current and deterministic Product Browser freshness passed on the final candidate.
- Owner manual Windows verification passed the accepted SK-07D checklist on 2026-09-27.
- Final exact-head Windows FULL #567 passed on `ef84954ddb22a4263d31c5a540fe7d96304e33e2`: documentation verification, Build, FAST/SUBSYSTEM selector checks, 67/67 unfiltered CTest tests and stable `windows-msvc` all succeeded.
- Ordinary-Select RMB context, semantic Space CycleEditMode, grip Copy modifier, Rotate/Scale/Mirror+Copy, numeric/Dynamic Input, snapping/inference and R8+ remain outside this completed contract.

