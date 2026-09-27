# SK-07E — Space CycleEditMode

**Status:** COMPLETED  
**Proposed:** 2026-09-27  
**Owner acceptance:** 2026-09-27  **Owner manual Windows verification:** PASS — 2026-09-27  
**Final exact-head Windows FULL:** #587 — `af8734bffd5e31c49844c63a718881fd21f44cf1` — PASS  

**Decision class:** D2 direct-manipulation interaction semantics + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009, ADR-0010  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.2  
**Milestone:** R7 — Common transforms, Copy and command grammar — fifth bounded slice

## 1. Goal

Implement the roadmap's semantic **Space CycleEditMode** behavior during an active grip-started DirectManipulationSession.

SK-07E is deliberately limited to the two direct-edit modes already present in the semantic interaction model:

```text
Reshape
Move
```

During active direct manipulation with viewport CAD focus, Space cycles among the modes supported by the active grip.

This slice does not add Copy as a direct-manipulation modifier and does not implement ordinary-Select RMB context.

## 2. Preserved authority and architecture

SK-07E preserves the completed Sketch interaction architecture:

- one `SketchInteractionState` remains the semantic interaction authority;
- HandleRole remains separate from EditMode;
- grip/provider/Qt/OCCT identity remains runtime-only;
- one active grip is allowed;
- the selection snapshot is frozen for the whole direct-manipulation session;
- grip-started manipulation uses the clicked grip's interaction-start location as pivot/reference;
- preview is runtime-only;
- LMB/Enter commit and Esc cancel keep their existing semantics;
- authored edits flow through the existing semantic geometry-update command and Part transaction path;
- editing preserves existing EntityIds;
- Repeat Last Command from SK-07D remains a separate ordinary-Select grammar and must not compete with active manipulation Space.

No new authored state, persistence schema or second interaction owner is introduced.

## 3. Supported mode set by grip role

The active grip determines which edit modes are meaningful.

### 3.1 Two-mode grips

These grips support both modes:

```text
Line Start       Reshape ↔ Move
Line End         Reshape ↔ Move

Circle quadrant  Reshape ↔ Move
  +U / +V / -U / -V

Arc Start        Reshape ↔ Move
Arc End          Reshape ↔ Move
Arc Mid          Reshape ↔ Move
```

Their existing default remains **Reshape**.

### 3.2 Move-only grips

These grips support only Move:

```text
Line Center
Circle Center
Arc Center
```

Their existing default remains **Move**.

Space on a Move-only center grip is a semantic no-op:

- mode stays Move;
- current preview stays Move;
- no revision/dirty/history change occurs.

A bounded status message such as `Only Move is available for this grip.` is permitted.

## 4. Reshape semantics

Reshape remains owner-only.

For a non-center grip:

- only the owning primitive is geometrically reshaped;
- other selected entities are unchanged;
- the active grip role retains its existing primitive-specific semantics;
- the owner EntityId is preserved.

Existing behavior remains authoritative:

- Line Start/End moves the corresponding endpoint;
- Circle quadrant changes radius from the unchanged center;
- Arc Start/End changes the directed arc endpoint semantics;
- Arc Mid changes radius from the unchanged center.

Cycling back to Reshape must not reinterpret another entity as owner.

## 5. Move semantics

When a two-mode grip is switched to Move:

- the affected set becomes the complete frozen semantic selection snapshot captured when the grip session began;
- the active grip's interaction-start location remains the pivot;
- translation delta is `current resolved point - pivot`;
- the same translation applies to every frozen selected Line/Circle/Arc;
- all existing EntityIds are preserved;
- commit is one atomic semantic update and one Undo entry.

This is the same provider-independent translation geometry semantics already used by center-grip Move and normal MOVE.

No separate grip-specific translation algorithm is authorized.

## 6. Mode cycling

During an active DirectManipulationSession:

- Space cycles to the next supported DirectEditMode for the active grip;
- for current two-mode grips the cycle is `Reshape → Move → Reshape → ...`;
- for current center grips there is only Move, so Space does not change mode.

Cycling:

- does not end the session;
- does not change active grip;
- does not change pivot;
- does not change the frozen selection snapshot;
- does not change primary selection;
- creates no authored mutation;
- creates no DocumentRevision;
- creates no dirty-state change;
- creates no Undo/Redo entry.

The mode is runtime-only and is never persisted.

## 7. Preview after cycling

All preview must remain derived from interaction-start authored geometry.

When Space changes mode:

- the current resolved pointer position is retained;
- preview is immediately recomputed using the new mode;
- preview must not be transformed from the previous mode's preview;
- repeated cycling must not accumulate numeric drift.

Example:

```text
Line endpoint grip starts at P0
pointer currently at P1

Reshape preview:
  owner endpoint → P1

Space → Move:
  frozen selection translates by P1 - P0

Space → Reshape:
  owner endpoint → P1 again
```

If the retained current pointer is invalid for the newly selected mode, that mode remains active but has no committable geometry until a later valid pointer position is resolved.

Mode cycling itself must never commit the previous preview.

## 8. Selection and owner semantics

The selection snapshot is frozen at direct-manipulation start and remains frozen across every mode cycle.

For a two-mode grip:

- Reshape affects the active grip owner only;
- Move affects the complete frozen selection snapshot.

The active owner must itself remain part of the frozen snapshot.

No entity may enter or leave the affected snapshot because of mode cycling.

Provider hit-test order must never redefine owner or selection.

## 9. Keyboard/focus precedence

Space semantics are stage-dependent.

### Viewport CAD focus

When direct manipulation is active:

```text
Space  → CycleEditMode
Enter  → Commit
Esc    → Cancel
LMB    → Commit
```

This active-manipulation Space behavior has precedence over SK-07D Repeat Last Command.

When no direct manipulation is active and ordinary Select is active, the completed SK-07D behavior remains:

```text
Space / Enter → Repeat Last Command
```

Existing common-transform Space semantics also remain unchanged:

- Space during transform `Select objects` completes object collection when valid.

### Text-entry focus

Space in Command Line or another text-entry control remains a literal text space and must not cycle EditMode.

No global application-level Space interception is authorized.

## 10. Commit and cancel

LMB or Enter commits the geometry of the **currently active mode**.

Commit uses the existing direct-manipulation command/transaction path.

A valid non-no-op commit:

- changes authored geometry once;
- increments revision once;
- creates one Undo entry;
- preserves existing EntityIds;
- returns to ordinary Select with the existing selection behavior.

A geometry-identical result remains the existing history-free no-op.

Esc:

- cancels the complete active DirectManipulationSession regardless of how many mode cycles occurred;
- clears preview;
- preserves selection;
- creates no authored mutation/history.

A subsequent Esc in ordinary Select keeps the existing selection-clearing behavior.

## 11. Undo/Redo and stale state

Existing history behavior remains:

- Undo/Redo requested during active manipulation cancels transient manipulation first;
- then ordinary global history runs;
- mode cycles themselves never appear in history.

Stale revision, missing target or invalid geometry must fail closed with no partial mutation.

Switching EditMode must not refresh the frozen source from a newer authored revision.

If a stale condition makes the session invalid, commit fails the complete operation and interaction returns safely according to the existing controller policy.

## 12. UI/status presentation

Operations/status presentation should expose the active semantic mode while manipulation is active so manual verification is unambiguous.

A bounded presentation such as:

```text
Grip — Reshape
Grip — Move
```

is sufficient.

Exact visual styling is D0/D1.

No new generic HUD/overlay framework is authorized.

## 13. Automated acceptance coverage

At minimum verify:

1. Line Start/End begin in Reshape;
2. Circle quadrant grips begin in Reshape;
3. Arc Start/End/Mid begin in Reshape;
4. Line/Circle/Arc center grips begin in Move;
5. Space on Line endpoint cycles Reshape → Move → Reshape;
6. Space on Circle quadrant cycles Reshape → Move → Reshape;
7. Space on Arc Start/End/Mid cycles Reshape → Move → Reshape;
8. Space on center grips leaves mode at Move;
9. cycling preserves active grip identity;
10. cycling preserves frozen selection membership and primary;
11. cycling preserves the interaction-start pivot;
12. Move after cycling translates the complete frozen mixed selection;
13. Reshape after cycling affects only the active owner;
14. preview after every cycle is recomputed from interaction-start authored geometry;
15. cycling repeatedly does not compound preview;
16. invalid retained pointer for the new mode yields no committable preview but no mutation;
17. LMB commit uses the currently active mode;
18. Enter commit uses the currently active mode;
19. Esc after any number of cycles cancels all transient preview and preserves selection;
20. mode cycling creates no revision/dirty/history change;
21. Undo/Redo boundary behavior remains unchanged;
22. Space during direct manipulation does not invoke Repeat Last Command;
23. ordinary-Select Space still invokes Repeat Last Command;
24. Space in transform Select objects keeps its completed semantics;
25. Space in text-entry focus remains text input;
26. Move/Copy/Rotate/Scale/Mirror and SK-07D regressions remain green;
27. final runtime candidate passes exact-head Windows FULL;
28. required documentation and generated Browser freshness pass.

## 14. Manual Windows verification

Final candidate requires Owner verification of:

- Line endpoint grip defaults to Reshape;
- while dragging/previewing endpoint, Space switches to Move of the complete selected set;
- another Space switches back to owner-only Reshape using the same pointer position;
- Circle quadrant and Arc Start/End/Mid behave analogously;
- Line/Circle/Arc center grips stay Move when Space is pressed;
- selected mixed geometry stays frozen through cycling;
- no preview jump caused by compounding from a previous preview;
- LMB and Enter commit whichever mode is currently shown;
- Esc cancels regardless of mode and preserves selection;
- Undo/Redo after commit remains one-step atomic;
- ordinary-Select Space still repeats the last command;
- transform object-selection Space remains unchanged;
- Command Line/text-field Space remains text;
- no regressions in COPY repeated placement, Rotate/Scale/Mirror, Repeat Last Command, Save/reopen or navigation.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-07E changes user-visible grip keyboard grammar and makes DirectEditMode switching observable during direct manipulation.

## 16. Expected implementation surface

Expected bounded production changes after Owner acceptance:

- `src/sketch/**` for semantic supported-mode cycling in the existing DirectManipulationSession;
- `src/ui/**` for viewport Space adapter/status projection;
- tests;
- affected internal/Product docs and generated Browser;
- `work/**`.

No application command or persistence schema change is expected.

Stop for Owner review if implementation requires:

- a third DirectEditMode;
- Copy modifier semantics;
- changing HandleRole identity;
- recapturing a mutable selection instead of preserving the frozen session snapshot;
- changing normal Move/Copy/Rotate/Scale/Mirror semantics;
- ordinary-Select RMB context;
- numeric parser/Dynamic Input;
- snapping/inference;
- provider identity in semantic interaction state;
- a second semantic interaction authority;
- a generic overlay framework.

## 17. Explicitly out of scope

SK-07E does not authorize:

- Copy modifier during grip manipulation;
- Rotate+Copy / Scale+Copy / Mirror+Copy;
- ordinary-Select RMB context;
- new context menus;
- third or configurable DirectEditMode;
- changing Base Point during a grip session;
- multiple active grips;
- numeric distances/coordinates/angles/scale factors;
- Dynamic Input;
- snapping/Object Snap/tracking/inference;
- Ortho/Polar/Grid Snap;
- constraints/solver/authored dimensions;
- R8+ work.

## 18. Activation and completion boundary

SK-07E was explicitly Owner-accepted on 2026-09-27 and is active within this bounded scope.

Completion requires:

- Space cycles Reshape/Move only on grips supporting both modes;
- center grips remain Move-only;
- the same active grip/pivot/frozen selection/current pointer survive cycling;
- preview always derives from interaction-start authored geometry;
- currently active mode owns LMB/Enter commit;
- active-manipulation Space has precedence over Repeat Last Command;
- text-focus and common-transform Space semantics do not regress;
- no Copy modifier or RMB context enters this slice;
- required internal and PL/EN Product documentation is current;
- final candidate passes exact-head Windows FULL;
- Owner manual Windows verification passes;
- closeout CLOSURE passes;
- remaining R7 and R8+ work stays inactive until separately accepted.

## 19. Completion evidence

SK-07E completed its bounded fifth R7 slice on 2026-09-27.

- Space cycles the existing semantic `DirectEditMode` values Reshape/Move during active grip manipulation on supported non-center grips.
- Line Start/End, Circle quadrant and Arc Start/End/Mid default to Reshape and cycle `Reshape ↔ Move`; Line/Circle/Arc center grips remain Move-only.
- Mode cycling preserves the same active grip, interaction-start pivot, frozen semantic selection and current resolved pointer.
- Reshape preview derives from interaction-start owner geometry; Move preview derives from interaction-start complete frozen-selection geometry, so repeated cycling never compounds preview.
- Mode cycling itself creates no authored mutation, revision, dirty-state change or history entry.
- LMB/Enter commit the currently active mode through the existing direct-manipulation command/Part transaction path; Esc cancels the transient session and preserves selection.
- Active-manipulation Space has precedence over SK-07D Repeat Last Command, while common-transform and text-entry Space semantics remain unchanged.
- Required internal and PL/EN Product documentation is current and deterministic Product Browser freshness passed on the final candidate.
- Owner manual Windows verification passed the accepted SK-07E checklist on 2026-09-27.
- Final exact-head Windows FULL #587 passed on `af8734bffd5e31c49844c63a718881fd21f44cf1`: documentation verification, Build, FAST/SUBSYSTEM selector checks, 68/68 unfiltered CTest tests and stable `windows-msvc` all succeeded.
- Grip Copy modifier, Rotate/Scale/Mirror+Copy, ordinary-Select RMB context, numeric/Dynamic Input, snapping/inference and R8+ remain outside this completed contract.

