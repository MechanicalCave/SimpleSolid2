# SK-07A — Common Transform Core and MOVE

**Status:** PROPOSED — OWNER REVIEW  
**Proposed:** 2026-09-26  
**Decision class:** D2 interaction/transform architecture + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009, ADR-0010  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.2  
**Milestone:** R7 — Common transforms, Copy and command grammar — first bounded slice

## 1. Goal

Establish one provider-independent semantic transform path and ship normal Sketch `MOVE` for mixed Line/Circle/Arc selections.

SK-07A must prove that:

- grip-started Center Move and command-started Move use the same semantic translation core;
- the affected set is the frozen semantic selection snapshot;
- selection-first and command-first workflows share one interaction authority;
- normal Move has an explicit Base Point and destination point;
- preview, commit, cancel, transaction and history semantics remain identical regardless of entry adapter.

This is the architectural base for later Rotate/Scale/Mirror/Copy work. SK-07A activates only Move.

## 2. Preserved authority and invariants

The Constitution, Foundation 1.0, ADR-0008, ADR-0009 and Sketcher roadmap v1.2 remain authoritative.

In particular:

- authored Sketch geometry remains Shared 2D semantic state;
- Part owns hosted Sketch lifecycle and persistence;
- EntityId remains opaque, stable and SketchModel-local;
- provider tokens, Qt/OCCT handles and projected geometry never become CAD identity;
- one Sketch interaction authority owns selection and transient operation state;
- all durable mutation goes through semantic application command → validation → DocumentSession → Part transaction/history;
- all pointer-derived positions enter through the existing resolved-input seam;
- preview remains runtime-only;
- common transforms affect the complete frozen selection snapshot;
- entity-specific Reshape remains owner-only and is not redefined by this contract;
- no multiple-active-grip feature is introduced.

## 3. Shared semantic translation core

SK-07A introduces one provider-independent translation operation for canonical Line/Circle/Arc geometry.

For finite translation delta `D = (du,dv)`:

- Line: translate Start and End by D;
- Circle: translate Center by D; preserve Radius;
- Arc: translate Center by D; preserve Radius, StartAngle and SignedSweepAngle.

The transform:

- preserves every edited EntityId;
- accepts semantic authored geometry, not Viewer objects;
- validates finite input/output;
- is deterministic;
- is independent of camera orientation and Sketch support plane;
- does not allocate EntityIds;
- does not mutate the live SketchModel while calculating preview.

Exact C++ type names are D1. The implementation must place translation math in one reusable semantic layer so grip Center Move and normal `MOVE` cannot diverge into separate geometry algorithms.

SK-07A does not require speculative unused Rotate/Scale/Mirror APIs. It only requires that the architecture does not put transform math into toolbar, Command Line, Qt or Viewer adapters.

## 4. Affected set and frozen snapshot

For common Move, the affected set is the current semantic selection snapshot at operation start.

The snapshot:

- contains EntityIds only;
- may contain any mixture of Line, Circle and Arc;
- is frozen when the Move operation proceeds beyond object collection;
- cannot be modified during Base Point / destination preview;
- is revalidated against the current SketchModel before commit;
- fails the entire mutation if any required target is missing or invalid.

One accepted Move applies exactly one translation delta to every entity in the frozen set.

Single-entity Move is the same operation with a one-entity set.

## 5. Entry adapters

SK-07A supports three entry adapters into the same semantic Move operation:

1. existing Center-grip Move;
2. Sketch toolbar `Move`;
3. Sketch Command Line `MOVE`.

All three must converge on the same semantic transform/preview/commit path after their entry-specific setup.

### 5.1 Grip-started Move

Existing Line/Circle/Arc Center grips continue to:

- use the active grip's interaction-start position as the pivot/base point;
- immediately freeze the complete current selection;
- enter Move without asking for another Base Point;
- preserve the existing click-to-activate behavior;
- use LMB or Enter for commit;
- use Esc to cancel preview and preserve selection.

There remains no "change Base Point" operation inside a grip-started session.

Existing owner-only reshape behavior is unchanged.

### 5.2 Selection-first normal MOVE

If `MOVE` is activated while semantic selection is non-empty:

- the current selection is used immediately;
- there is no extra "Select objects complete" confirmation;
- the affected set is frozen;
- the operation enters **Specify Base Point**.

The Base Point may be any finite resolved Sketch-local point; it does not need to lie on selected geometry.

After Base Point acceptance the operation enters **Specify destination point**.

For destination point P and Base Point B:

```text
delta = P - B
```

Pointer movement previews the complete frozen set with that delta.

LMB or Enter accepts the current valid destination.

### 5.3 Command-first normal MOVE

If `MOVE` is activated with an empty semantic selection, it enters **Select objects**.

During this stage:

- ordinary entity pick adds and makes primary;
- re-picking an already selected entity preserves membership and makes it primary;
- Ctrl+pick toggles membership;
- Window/Crossing uses the existing semantic selection rules;
- Ctrl+rectangle toggles returned EntityIds;
- blank LMB is a no-op, not selection clear;
- grips are hidden/inactive;
- Enter, Space or RMB completes object collection only when the collected selection is non-empty;
- completing object collection freezes that semantic set and enters **Specify Base Point**;
- Esc cancels MOVE and returns to Select while preserving the objects collected so far as the normal selection set.

Object collection must reuse the existing semantic selection/query bridge; it must not create a second provider-owned command selection.

## 6. Base Point and destination lifecycle

Normal Move stages are:

```text
Select objects (command-first only)
→ Specify Base Point
→ Specify destination point / preview
→ commit or cancel
→ Select
```

Base Point acceptance:

- stores one finite resolved Sketch-local point;
- creates no authored mutation;
- creates no EntityId;
- does not dirty the Document;
- creates no history entry.

Destination preview:

- always derives from the interaction-start authored geometry and frozen selection;
- never compounds from the previous preview frame;
- applies exactly one delta `destination - base`;
- updates all affected Line/Circle/Arc entities coherently.

A zero translation is a valid no-op completion:

- no authored state changes;
- no DocumentRevision increment;
- no dirty-state change;
- no Undo entry;
- the command returns to Select with selection preserved.

## 7. Commit, validation and history

A non-zero accepted Move is one semantic operation:

- one command;
- one staged authored state;
- one Part transaction;
- one revision increment;
- one Undo entry.

Commit must validate:

- active Sketch identity;
- captured DocumentRevision;
- every frozen EntityId;
- canonical geometry after translation.

Any stale revision, missing target, invalid target or non-finite result fails the whole operation. Partial mutation is forbidden.

Edited entities preserve EntityId.

Undo/Redo:

- cancel any active transient Move first;
- then execute ordinary global history;
- reconcile surviving semantic selection afterward;
- never preserve a stale preview/session across the history boundary.

Save → Close → Reopen must preserve moved geometry and the same EntityIds through the existing schema v4. SK-07A does not change persistence schema.

## 8. Selection and post-command state

After successful Move, no-op Move or Esc from Base Point/destination stages:

- editor returns to Select;
- the affected semantic selection remains selected;
- primary remains semantic and deterministic;
- grips are regenerated from current geometry.

Esc behavior:

- in command-first Select objects: cancel command, preserve collected normal selection;
- in Specify Base Point: cancel command, preserve frozen selection;
- in destination preview: cancel preview/command, preserve frozen selection;
- subsequent Esc in ordinary Select follows the existing selection-clear grammar.

Explicitly switching to another Sketch tool cancels only the current uncommitted Move and preserves the semantic selection.

## 9. Keyboard and RMB scope

SK-07A implements only the keyboard/RMB behavior needed for MOVE object collection and existing commit/cancel grammar.

- Enter completes command-first Select objects when non-empty.
- Space completes command-first Select objects when non-empty.
- RMB completes command-first Select objects when non-empty.
- Enter commits a valid destination preview.
- Esc follows Section 8.
- Space inside text-entry focus remains text input and never invokes a CAD action.

Semantic `Space CycleEditMode` is not product-active in SK-07A because Move is the only common transform mode in this slice.

Ordinary-Select RMB context menu and Repeat Last Command remain later R7 work.

## 10. UI surface

Sketch edit gains a bounded `Move` command adapter.

Required product entry:

- Sketch toolbar/button activation;
- Command Line keyword `MOVE`.

Operations/status must expose the current finite stage:

- Select objects;
- Specify Base Point;
- Specify destination point.

The UI must not own transform geometry or a second selection set.

Cursor/pointer routing may reuse the current provider-neutral Sketch crosshair/pick-box modes. No new generic overlay framework is authorized.

## 11. Interaction-state architecture

One semantic runtime authority must represent normal Move stages alongside the existing Select/creation/direct-manipulation state.

The exact representation is D1, but it must satisfy:

- no parallel Qt-owned command state;
- no Viewer-owned CAD command state;
- no second semantic selection owner;
- grip Move and normal Move share transform calculation;
- command stage changes are explicit and testable;
- transient state can be cancelled deterministically on Esc, tool switch, Undo/Redo, document switch or active-Sketch loss.

If implementation reveals that current `SketchInteractionState` cannot represent this without splitting semantic authority, stop for Owner review rather than adding a second controller-owned state machine.

## 12. Automated verification

At minimum prove:

1. pure mixed Line/Circle/Arc translation preserves IDs and non-translated canonical parameters;
2. non-finite transform input/output fails closed;
3. grip Center Move and normal Move produce identical canonical geometry for the same frozen set/base/destination;
4. selection-first MOVE skips object collection and freezes the existing selection;
5. command-first MOVE enters Select objects only when starting from empty selection;
6. command-first point/Ctrl/Window/Crossing collection follows existing semantic selection rules;
7. blank LMB is a no-op in command object collection;
8. Enter/Space/RMB complete a non-empty command selection and do nothing when empty;
9. Base Point creates no authored/history mutation;
10. preview derives from the original snapshot and does not accumulate frame-to-frame;
11. mixed Move commit is atomic and creates exactly one revision/history entry;
12. zero-delta completion creates no revision/dirty/history change;
13. stale revision/missing target causes no partial mutation;
14. successful Move preserves selection/primary and EntityIds;
15. Esc behavior is correct in Select objects/Base Point/destination stages;
16. Undo/Redo cancels transient Move before global history;
17. Save → Close → Reopen preserves moved mixed geometry and EntityIds under schema v4;
18. existing Line/Circle/Arc creation, selection, grips and owner-only reshape remain green;
19. toolbar and Command Line `MOVE` route into the same semantic operation;
20. text-focus Space does not invoke CAD Move behavior;
21. native Viewer/navigation/grip regressions remain green;
22. FAST/SUBSYSTEM Sketch/UI checkpoints pass during implementation;
23. final runtime candidate passes exact-head Windows FULL;
24. internal and PL/EN product docs plus Product Browser freshness pass.

## 13. Manual Windows verification

Final candidate requires Owner verification of:

- select a mixed Line/Circle/Arc set, activate Move, choose arbitrary Base Point and destination, and see the whole set translate rigidly;
- start `MOVE` with no selection, collect objects by click and Window/Crossing, finish selection with Enter/Space/RMB, then move them;
- Center-grip Move gives the same geometry semantics as normal Move while using the grip as base;
- selection remains selected after commit and after Esc cancellation;
- no-op destination produces no visible/history mutation;
- LMB/Enter/Esc hierarchy is coherent;
- command-first blank LMB does not unexpectedly clear collected objects;
- grips disappear while command object collection/base/destination interaction requires it and return correctly in Select;
- text-entry Space remains text input;
- Undo/Redo, Save/Close/Reopen and mixed EntityId identity behave correctly;
- Pan/Orbit/Zoom/ViewCube remain stable with no stale provider pixels or hover leakage.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-07A adds a new user-visible MOVE command, explicit Base Point workflow, command-first object collection and a shared transform architecture.

## 15. Expected implementation surface

Expected bounded production changes:

- `src/sketch/**`;
- `src/application/**`;
- `src/ui/**`;
- `src/viewer/**` only if finite runtime state/presentation changes are necessary;
- `src/viewer_qt_occt/**` only for bounded cursor/presentation behavior if required;
- `tests/**`;
- affected internal/Product docs and generated Browser;
- `work/**`.

No Part schema migration is expected.

If implementation requires generic matrices in persistence, provider identity in semantics, numeric parser work, snapping/inference, a generic overlay system, or a second semantic selection/interaction authority, stop for a separate D2/D3 decision.

## 16. Out of scope

SK-07A does not authorize:

- Rotate;
- Scale;
- Mirror;
- Copy or repeated Copy;
- Copy modifier during grip manipulation;
- semantic Space CycleEditMode beyond the object-selection completion behavior explicitly listed here;
- ordinary-Select RMB context menu;
- Repeat Last Command;
- numeric coordinates, distances or angles;
- Dynamic Input;
- Object Snap / tracking / inference;
- Ortho/Polar/Grid Snap;
- constraints/solver/authored dimensions;
- Rectangle/Polyline or additional primitive breadth;
- planar-face Sketch support;
- region/profile work.

Those remain later R7+ slices.

## 17. Completion boundary

SK-07A becomes active only after explicit Owner acceptance.

Completion requires:

- one shared semantic translation core is used by grip Center Move and normal MOVE;
- selection-first and command-first MOVE are both implemented within this contract;
- no persistence schema change or out-of-scope transform mode is introduced;
- required internal and PL/EN product documentation is current;
- final runtime candidate passes exact-head Windows FULL;
- Owner manual Windows verification passes;
- closeout CLOSURE passes;
- remaining R7 work stays inactive until separately accepted.
