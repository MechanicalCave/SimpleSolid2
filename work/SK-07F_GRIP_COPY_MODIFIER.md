# SK-07F — Grip Copy Modifier

**Status:** PROPOSED — OWNER REVIEW  
**Proposed:** 2026-09-27  
**Decision class:** D2 direct-manipulation duplication semantics + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009, ADR-0010  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.2  
**Milestone:** R7 — Common transforms, Copy and command grammar — sixth bounded slice

## 1. Goal

Add a bounded **Ctrl Copy modifier** to active grip direct manipulation.

The modifier applies only when the current semantic DirectEditMode is **Move**.

When active, commit duplicates the complete frozen selection at the current translated preview instead of editing the original entities.

This slice reuses:

- the SK-07E DirectManipulationSession;
- the existing provider-independent translation geometry;
- SK-07C fresh-identity duplication semantics;
- the existing atomic Part command/transaction path.

It does not add Reshape+Copy or Rotate/Scale/Mirror+Copy.

## 2. Modifier key and focus

The modifier key is **Ctrl**.

The semantic copy modifier is active only while:

- Sketch edit is active;
- one DirectManipulationSession is active;
- current DirectEditMode is Move;
- viewport CAD interaction owns the key/modifier state;
- Ctrl is currently held/requested.

Ctrl is a held modifier, not a latch.

Releasing Ctrl before commit returns the session to ordinary Move commit semantics.

Text-entry focus must not activate grip Copy.

Focus loss, session cancel/end, tool switch, Sketch switch or controller teardown must clear any remembered Ctrl-copy request so the modifier cannot become sticky.

## 3. Interaction with SK-07E Space CycleEditMode

Space and Ctrl are orthogonal runtime inputs.

For non-center grips:

```text
default: Reshape
Space → Move
Ctrl held while Move → Move + Copy
Space while Ctrl remains held → Reshape (Copy inactive)
Space again → Move (Copy effective again if Ctrl is still held)
```

For center grips:

```text
Move-only
Ctrl held → Move + Copy
```

The active grip, pivot, frozen selection and current resolved pointer remain unchanged.

Ctrl never creates a third DirectEditMode. Copy is a commit modifier over Move.

## 4. Preview semantics

Copy modifier does not create a second preview algorithm.

Move and Move+Copy display the same translated geometry:

```text
delta = current resolved point - interaction-start grip pivot
preview = translate(frozen interaction-start selection geometry, delta)
```

Only commit semantics differ.

Operations/status should make the effective mode observable, for example:

```text
Grip — Move
Grip — Move + Copy
```

Exact styling is D0/D1.

## 5. Commit semantics

### 5.1 Ordinary Move

With Ctrl not active, existing SK-07E behavior remains unchanged:

- existing selected entities are updated in place;
- EntityIds are preserved;
- one accepted non-no-op commit = one command/transaction/revision/Undo step.

### 5.2 Move + Copy

With Ctrl active at commit:

- source entities remain unchanged;
- each duplicated Line/Circle/Arc receives a fresh non-aliasing EntityId;
- duplicated geometry equals the current Move preview;
- source selection remains selected;
- created copies do not replace source selection;
- one commit is one atomic semantic duplication command/transaction/revision/Undo step;
- Undo removes that copy placement;
- Redo restores the same copied EntityIds.

The commit must reuse the existing semantic duplication/fresh-identity path from normal COPY rather than introducing a second allocator.

## 6. Zero displacement

If current pointer equals the interaction-start pivot, Move+Copy is a clean no-op.

It must create:

- no copied entities;
- no EntityIds;
- no DocumentRevision;
- no dirty-state change;
- no Undo entry.

The direct-manipulation session may remain active after the no-op commit attempt or finish according to the existing controller convention, but it must not create an invisible coincident copy.

If existing controller lifecycle cannot preserve clear UX here without architectural ambiguity, stop for Owner review.

## 7. Selection and identity

The source set is the frozen semantic selection captured when the grip session begins.

Mode changes and Ctrl press/release must not recapture selection.

For Move+Copy:

- every source entity remains present with its original EntityId;
- every created entity gets a fresh EntityId;
- copied IDs must not alias source IDs or any previously committed/undone IDs;
- session/document high-water identity behavior from SK-07C remains authoritative.

No provider token, Qt object or OCCT object may become durable copied identity.

## 8. Ctrl state and pointer events

The existing viewport `SpatialPointerModifiers::control` may be used as an adapter for pointer-derived modifier state.

Keyboard Ctrl press/release may also update the same semantic modifier while the viewport has CAD focus so the status/commit mode changes immediately even without pointer movement.

There must be one semantic Ctrl-copy state for the active DirectManipulationSession.

Do not implement separate pointer-copy and keyboard-copy authorities.

## 9. Commit inputs

Move+Copy commit is allowed through the same finalization surfaces as ordinary direct manipulation:

- LMB;
- Enter.

The effective Ctrl state at commit determines whether commit means Move or Move+Copy.

Examples:

- Ctrl held during preview, released before Enter → ordinary Move;
- Ctrl pressed immediately before Enter → Move+Copy;
- Ctrl held and LMB commit → Move+Copy.

Esc always cancels the complete transient session and creates no Move/Copy mutation.

## 10. History boundaries

Ctrl press/release and copy-mode preview are runtime-only.

They create no history.

An accepted Move+Copy commit is exactly one history entry.

Undo/Redo requested while transient manipulation is active first cancels that manipulation according to existing controller rules, then performs global history.

Redo restores the same copy IDs.

A later fresh Copy operation must allocate above preserved high-water and never reuse abandoned committed IDs.

## 11. Repeat Last Command and normal COPY

Grip Copy modifier does not become a command identity.

It must not replace SK-07D Last Repeatable Command.

Normal toolbar/Command-Line COPY behavior from SK-07C remains unchanged:

- explicit Base Point;
- repeated placements;
- one Undo per placement.

Grip Move+Copy is a one-shot direct-manipulation commit:

- one frozen grip session;
- one placement;
- session ends after accepted commit.

Repeated grip copy placement is out of scope.

## 12. Reshape behavior

Ctrl has no copy effect while current DirectEditMode is Reshape.

Reshape semantics remain exactly as completed in SK-07E:

- owner-only edit;
- existing EntityId preserved;
- Space may switch to Move.

This slice does not authorize:

- Reshape+Copy;
- duplicate-and-reshape owner;
- fresh-ID reshape variants.

A bounded status hint that Copy is available in Move only is permitted.

## 13. Automated acceptance coverage

At minimum verify:

1. center grip begins Move with Copy modifier inactive;
2. non-center grip begins Reshape with Copy modifier ineffective;
3. Space to Move enables Ctrl Copy semantics;
4. Ctrl press in Move changes effective commit to Copy;
5. Ctrl release returns effective commit to ordinary Move;
6. Space Move→Reshape disables effective Copy without losing held Ctrl intent;
7. Space Reshape→Move re-enables effective Copy if Ctrl remains held;
8. Ctrl changes no pivot, active grip, frozen selection or current pointer;
9. Ctrl changes no preview geometry relative to Move;
10. Ctrl press/release creates no revision/dirty/history change;
11. Move commit preserves existing IDs and edits source;
12. Move+Copy commit leaves source unchanged;
13. Move+Copy creates fresh IDs for mixed Line/Circle/Arc selection;
14. source selection remains selected after copy;
15. created copies do not replace selection;
16. zero-displacement Move+Copy creates nothing and consumes no IDs;
17. one Move+Copy commit is one atomic Undo step;
18. Undo removes copies only;
19. Redo restores the same copied IDs;
20. fresh copy after Undo does not reuse prior committed IDs;
21. Enter commit honors current Ctrl state;
22. LMB commit honors current Ctrl state;
23. Esc cancels without mutation regardless of Ctrl state;
24. text-entry Ctrl does not activate grip Copy;
25. focus/session end clears modifier state;
26. Repeat Last Command is not replaced by grip Copy;
27. normal repeated COPY remains unchanged;
28. SK-07E Space CycleEditMode regressions remain green;
29. final candidate passes exact-head Windows FULL;
30. documentation and Product Browser freshness pass.

## 14. Manual Windows verification

Final candidate requires Owner verification of:

- Line/Circle/Arc center grip Move + Ctrl creates a copy instead of moving source;
- non-center grip defaults to Reshape and Ctrl alone does not copy;
- Space to Move, then Ctrl, produces Move+Copy;
- while Ctrl is held, Space to Reshape disables Copy and Space back to Move restores it;
- preview geometry does not jump when Ctrl is pressed/released;
- releasing Ctrl before Enter/LMB performs ordinary Move;
- source remains unchanged after Move+Copy;
- source selection remains selected;
- zero displacement with Ctrl creates no coincident copy;
- Undo/Redo is one atomic copy step and Redo restores identities;
- Esc with Ctrl held cancels cleanly;
- Repeat Last Command remains unchanged;
- normal repeated COPY, Rotate/Scale/Mirror, Space CycleEditMode, Save/reopen and navigation do not regress.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-07F adds user-visible Ctrl modifier grammar and fresh-identity duplication through grip manipulation.

## 16. Expected implementation surface

Expected bounded production changes after Owner acceptance:

- `src/sketch/**` for one runtime copy-modifier request/effective state attached to the existing DirectManipulationSession;
- `src/ui/**` for Ctrl press/release and pointer-modifier adapters plus status projection;
- `src/application/**` only if a small adapter is required to invoke the already existing semantic duplication command;
- tests;
- internal/Product docs and generated Browser;
- `work/**`.

No persistence schema change is expected.

Stop for Owner review if implementation requires:

- a new identity allocator;
- a second semantic duplication command path;
- Reshape+Copy;
- Rotate/Scale/Mirror+Copy;
- repeated placement inside one grip session;
- recapturing selection after session start;
- a third DirectEditMode;
- provider identity in semantic state;
- a second interaction authority;
- numeric/Dynamic Input or snapping work.

## 17. Explicitly out of scope

SK-07F does not authorize:

- Reshape+Copy;
- Rotate+Copy;
- Scale+Copy;
- Mirror+Copy;
- repeated grip-copy placement;
- ordinary-Select RMB context;
- clipboard/cross-Sketch/cross-Document Copy;
- numeric distances/coordinates/angles/scale factors;
- Dynamic Input;
- snapping/Object Snap/tracking/inference;
- Ortho/Polar/Grid Snap;
- constraints/solver/authored dimensions;
- R8+ work.

## 18. Activation and completion boundary

SK-07F is a proposal only.

No production implementation is authorized until explicit Owner acceptance.

If accepted as written, completion requires:

- Ctrl is the held Copy modifier for active grip Move;
- Reshape remains non-copying;
- Ctrl and Space compose without changing active grip/pivot/frozen selection/current pointer;
- preview stays the existing Move translation preview;
- Move+Copy reuses SK-07C semantic duplication and fresh identity;
- zero-displacement Copy is history-free and consumes no IDs;
- accepted Move+Copy is one atomic history entry;
- source remains unchanged and selected;
- Repeat Last Command and normal repeated COPY do not regress;
- required internal and PL/EN Product documentation is current;
- final exact-head Windows FULL passes;
- Owner manual Windows verification passes;
- closeout CLOSURE passes;
- ordinary-Select RMB context and later work remain inactive until separately accepted.
