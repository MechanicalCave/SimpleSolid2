# SK-04A — Sketch Interaction State and Atomic Batch Delete

**Status:** PROPOSED  
**Owner acceptance:** pending  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.1  
**Roadmap milestone:** R4 — First complete continuous Line workflow  
**R4 delivery split:** SK-04A / SK-04B / SK-04C

## 1. Context

R3 completed the provider-neutral presentation and spatial-input boundary:

```text
authored active-Sketch scene
transient preview scene
PresentationToken ↔ SketchId + EntityId runtime binding
logical viewport point + Ray3
Ray3 → active Sketch U/V
primary routing mode
cursor mode
```

R4 must now introduce one real Sketch interaction authority rather than implementing Line/Select separately in Qt, CadWorkbench, Operations and Command Line.

The Owner accepted the R4 delivery direction on 2026-09-25:

```text
SK-04A
  host-neutral interaction state
  Line state machine
  semantic selection by EntityId
  atomic batch Delete command

SK-04B
  provider-neutral point/rectangle query
  selection rectangle overlay
  token → semantic selection bridge

SK-04C
  end-to-end Sketch toolbar / Select / continuous Line
  Operations + compact Command Line adapters
  Esc / Finish / Cancel / Undo-Redo integration
  rectangle selection + Delete UX
```

This contract proposes only SK-04A.

## 2. Goal

Implement the smallest host-neutral semantic runtime state needed by the later R4 UI and add one atomic semantic command for deleting multiple selected Sketch entities.

SK-04A must establish:

```text
one active Sketch interaction state
    Select
    Line / AwaitFirstPoint
    Line / AwaitNextPoint

semantic runtime selection
    0..N EntityId
    optional primary EntityId

Line commit protocol
    runtime request
    → host executes AddSketchLineCommand
    → success/failure acknowledgement
    → tool state advances only on success

atomic batch deletion
    EraseSketchEntitiesCommand
    → validate all
    → one transaction
    → one Undo entry
```

SK-04A does not expose the final UI workflow.

## 3. Architectural placement

### 3.1 Interaction state belongs to Shared 2D / Sketch Core

The interaction state is host-neutral and belongs in the neutral Sketch layer.

It may depend on existing Sketch types such as:

```text
Point2
EntityId
SketchModel
```

It must not depend on:

```text
PartDocument
DocumentSession
Qt
OCCT
Viewer
PresentationToken
SketchPlacement
filesystem
```

The state is scoped conceptually to one currently active Sketch. Host identity/lifecycle remains outside it.

### 3.2 One state authority, many future adapters

The state established here is the only semantic runtime authority for R4 Sketch interaction.

Future adapters may include:

```text
mouse / viewport
toolbar
Operations
Command Line
keyboard
```

Those adapters may invoke the state but must not duplicate Line or Select state machines.

## 4. Tool state

The minimum tool vocabulary is:

```text
Select
Line
```

Select is the default.

Line has two stable stages:

```text
AwaitFirstPoint
AwaitNextPoint
```

Conceptually:

```text
enter Sketch edit
    → Select

activate Line
    → Line / AwaitFirstPoint

accept first point P0
    → Line / AwaitNextPoint
    → anchor = P0

move pointer P
    → transient preview intent P0 → P
    → no authored mutation

accept next point P1
    → request authored segment P0 → P1
    → wait for host result

host success
    → anchor = P1
    → remain Line / AwaitNextPoint

host failure
    → anchor remains P0
    → remain Line / AwaitNextPoint
```

The interaction state never executes DocumentSession itself.

## 5. Line segment commit protocol

### 5.1 Two-phase semantic handoff

Accepting the second/subsequent Line point produces a host-facing semantic request equivalent to:

```text
Add Line:
    start = current anchor
    end   = accepted point
```

The state must not advance the continuous-Line anchor until the host confirms that the semantic command succeeded.

This prevents a failed/rejected transaction from making runtime state claim geometry that was never authored.

### 5.2 Exactly one outstanding segment request

The state must not produce a second authored Line request while an earlier one is unresolved.

The exact private representation is implementation detail, but observable behavior is:

```text
request pending
→ no second commit request
→ host resolves success or failure
→ state becomes ready again
```

R4 currently uses synchronous DocumentSession execution, but the semantic state must still make success/failure acknowledgement explicit.

### 5.3 Exact-zero segment

If accepted next point equals the current anchor exactly:

- no semantic Add request is produced;
- no authored mutation occurs;
- no Undo entry is created;
- no revision or dirty-state change occurs;
- the anchor remains unchanged;
- Line remains active.

No near-zero epsilon policy is introduced.

### 5.4 Preview

While Line is in `AwaitNextPoint`, a finite current pointer point may produce a transient preview from anchor to current point.

Preview:

- is runtime-only;
- is absent when no anchor exists;
- is absent for exact-zero anchor/current geometry;
- creates no authored entity;
- creates no EntityId;
- creates no Undo/dirty/revision effect.

SK-04A exposes semantic preview intent only. R3 already owns the provider-neutral preview presentation channel.

## 6. Finish, Cancel and Esc semantics

These semantics are frozen for R4.

### 6.1 Finish Line

Explicit Finish:

```text
any Line stage
→ clear pending runtime Line state/preview
→ Select
```

Already committed segments remain authored.

Finish creates no Undo entry by itself.

### 6.2 Cancel Line

Explicit Cancel:

```text
any Line stage
→ discard only uncommitted runtime Line state
→ Select
```

Cancel does not rollback previously committed segments.

Committed geometry is undone only by ordinary Document Undo.

### 6.3 Hierarchical Esc

The semantic state supports the ADR-0009 hierarchy:

```text
Line / AwaitNextPoint
Esc
→ cancel current anchor/pending stage
→ Line / AwaitFirstPoint

Line / AwaitFirstPoint
Esc
→ Select

Select
Esc
→ no tool-state change
```

Exact key-event plumbing belongs to SK-04C.

### 6.4 Undo/Redo policy for later UI integration

The accepted R4 policy is:

```text
Undo/Redo requested while Line active
→ cancel uncommitted Line runtime state
→ return Select
→ execute ordinary document Undo/Redo
```

SK-04A provides the state operation needed for this policy but does not wire Workbench buttons/shortcuts.

## 7. Undo granularity for continuous Line

Each successfully committed segment is exactly one existing `AddSketchLineCommand`.

Therefore:

```text
A-B
B-C
C-D

Undo #1 → remove C-D
Undo #2 → remove B-C
Undo #3 → remove A-B
```

There is no compound "whole Line-tool session" history entry in R4.

Finishing/cancelling the tool creates no history entry.

## 8. Semantic Sketch selection

### 8.1 Runtime identity

Selection stores semantic Sketch entity identity only:

```text
selected: 0..N EntityId
primary: optional EntityId
```

It never stores:

```text
PresentationToken
viewer object
OCCT handle
storage index
pointer
coordinates as identity
```

Selection is scoped to the active Sketch by the host.

### 8.2 Minimum operations

The interaction state must support semantic operations equivalent to:

```text
replace(EntityId)
toggle(EntityId)
clear()
replaceMany(EntityIds, optional primary)
reconcile/prune against currently existing entities
```

Selection must contain no duplicate EntityIds.

Primary, when present, must be a member of selected.

### 8.3 Point-selection grammar frozen for R4

Future SK-04B/C adapters use:

```text
Left click Line        → replace selection
Ctrl + Left click Line → toggle Line
Left click blank       → clear selection
```

A Line click selects the Line entity, not endpoint/midpoint sub-elements.

Grips remain R5.

### 8.4 Rectangle-selection grammar frozen for R4

Future SK-04B/C adapters use classical CAD direction semantics:

```text
drag Left → Right
Window
→ select entities fully contained

drag Right → Left
Crossing
→ select entities contained or intersecting
```

SK-04A does not implement provider rectangle hit testing or the rectangle overlay.

## 9. Atomic batch Delete command

### 9.1 New semantic command

Application/DocumentSession gains a batch command conceptually:

```text
EraseSketchEntitiesCommand
{
    SketchId
    EntityIds[]
}
```

The command is the semantic path for later Delete of a Sketch multi-selection.

### 9.2 Validation

The command fails closed if:

- SketchId does not resolve;
- target list is empty;
- any EntityId is invalid;
- EntityIds contain duplicates;
- any EntityId does not resolve in the target Sketch.

Validation must complete before any entity is erased.

No partial delete is legal.

### 9.3 Transaction/history semantics

A successful batch delete:

```text
1 user semantic command
→ 1 staged authored state
→ 1 Part transaction
→ 1 revision increment
→ 1 Undo entry
```

This remains true whether the batch contains one entity or many.

Failure produces:

```text
no authored mutation
no revision increment
no dirty-state change
no history change
```

Undo restores every deleted entity with its original EntityId and geometry.

Redo removes the same set again.

Existing model-local identity high-water rules remain unchanged.

### 9.4 Existing single-erase command

`EraseSketchEntityCommand` may remain as a compatibility/internal command.

Implementation may share validation/mutation mechanics with the batch command, but no existing semantic behavior may regress.

## 10. Selection after mutation

Selection is runtime-only and is not restored by CAD Undo/Redo.

Required semantic direction:

- after successful Delete, deleted EntityIds are removed from runtime selection;
- after Undo restores geometry, it is not automatically reselected;
- after any external authored-state refresh, stale selected EntityIds can be pruned explicitly against the current Sketch model;
- selection reconciliation creates no authored mutation.

SK-04A implements the neutral selection capability; SK-04C wires it to document refresh/history actions.

## 11. Compact Command Line boundary for R4

The accepted R4 Command Line scope is intentionally narrow.

R4 may display state/prompts such as:

```text
Command: SELECT
Command: LINE

LINE — Specify first point
LINE — Specify next point
```

R4 does not implement precision coordinate grammar such as:

```text
@25,10
25<45
#100,50
relative/polar modes
dynamic dimensions
repeat-command aliases
```

Those remain R7/later work.

SK-04A contains no UI widget or parser.

## 12. Deliberately OUT of SK-04A

```text
Qt toolbar/buttons
Operations widgets
Command Line widget
keyboard event plumbing
Esc key binding
mouse click/drag plumbing
Viewer point-pick query changes
rectangle hit testing
selection rectangle overlay
window/crossing provider implementation
presentation highlight mapping for Sketch selection
end-to-end Line creation from mouse
end-to-end Delete key action
dynamic input
numeric coordinate parsing
Object Snap
inference
grips
direct manipulation
constraints
dimensions
solver
Circle/Arc
Trim/Extend/Offset
profiles/regions
projected/reference geometry
planar-face Sketch support
```

No placeholders for these features are added.

## 13. Expected implementation surface

Expected production changes are bounded to:

```text
src/sketch/include/simplesolid2/sketch/**
src/sketch/**
src/application/include/simplesolid2/application/document_session.hpp
src/application/document_session.cpp
tests/**
docs/internal/SHARED_2D.md
docs/internal/PART_DOCUMENTS.md
docs/internal/BUILD_AND_TEST.md
work/**
```

No Viewer, Qt/OCCT, persistence-schema or Product documentation change is expected.

If implementation evidence requires Viewer/UI changes, stop and move that work to SK-04B/C rather than expanding this contract silently.

## 14. Acceptance tests

At minimum prove:

1. New interaction state defaults to Select.
2. Activating Line enters AwaitFirstPoint.
3. First finite point becomes anchor without authored request.
4. Line AwaitNextPoint exposes transient preview intent.
5. Exact-zero next point produces no authored request and preserves anchor.
6. Valid next point produces exactly one pending segment request.
7. A second request cannot be emitted while the first is unresolved.
8. Commit success advances anchor to committed endpoint and remains AwaitNextPoint.
9. Commit failure preserves the previous anchor and remains AwaitNextPoint.
10. Successive successful acknowledgements support continuous A-B, B-C, C-D intent.
11. Finish from either Line stage returns Select and clears pending/preview state.
12. Cancel returns Select without implying rollback of prior authored segments.
13. Esc from AwaitNextPoint returns AwaitFirstPoint.
14. Esc from AwaitFirstPoint returns Select.
15. Esc in Select is a no-op.
16. Runtime/history-cancel operation returns Line to Select with no authored semantics.
17. Selection replace stores exactly one EntityId as selected/primary.
18. Toggle adds an unselected EntityId.
19. Toggle removes a selected EntityId and maintains a valid primary.
20. Clear produces zero selection/no primary.
21. Replace-many rejects or normalizes duplicate input without allowing duplicate selected identities.
22. Selection reconciliation prunes stale IDs without authored mutation.
23. Interaction-state source remains independent of Part/Application/Viewer/Qt/OCCT.
24. Batch erase of one valid Line succeeds as one revision/history entry.
25. Batch erase of multiple valid Lines succeeds atomically as one revision/history entry.
26. Undo of batch erase restores all original EntityIds and geometry.
27. Redo erases the same entities again.
28. Empty batch fails with no state/history/revision/dirty change.
29. Duplicate EntityIds fail with no partial erase.
30. Invalid EntityId fails with no partial erase.
31. Unknown/stale EntityId fails with no partial erase.
32. Unknown SketchId fails with no mutation.
33. Same model-local EntityId value in another Sketch is unaffected.
34. Existing single EraseSketchEntity behavior remains PASS.
35. Existing R1/R2/R3 tests remain PASS.
36. Internal as-built documentation is current.
37. FULL exact-head gate passes on implementation.
38. Completion bookkeeping uses the appropriate CI-01 non-build tier.

## 15. Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: SK-04A establishes neutral interaction semantics and application command behavior but does not expose the final R4 user-facing workflow.

## 16. Roadmap impact

Roadmap milestone: R4 — First complete continuous Line workflow  
Roadmap version: 1.1  
Roadmap change: none.

This contract is the first of three bounded contracts planned to complete R4. R4 remains incomplete after SK-04A.

## 17. Completion

SK-04A completes only when:

- one host-neutral Select/Line interaction state exists;
- continuous-Line request/acknowledgement semantics are proven;
- runtime semantic selection stores EntityId rather than Viewer identity;
- atomic multi-entity Delete exists through DocumentSession;
- batch Delete has one-command/one-transaction/one-Undo semantics;
- no UI/provider behavior is hidden in Sketch Core;
- all acceptance tests and internal docs pass;
- implementation exact head passes FULL verification;
- completion bookkeeping passes the appropriate exact-head CI tier.

Completion of SK-04A does not activate SK-04B automatically. SK-04B requires a separate explicit Owner-accepted Work Contract.
