# C1 — History Copy Cost Removal

**Status:** COMPLETED  
**Proposed:** 2026-09-27  
**Owner acceptance:** 2026-09-27  
**Final exact-head Windows FULL:** #669 — `848fd01295bd9a93b882cca1d2df6cc362e8a265` — PASS  
**Manual UI verification:** not required  
**Decision class:** D1 local DocumentSession history commit preparation  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package C1  
**Architecture:** no new ADR required if implementation remains inside the private DocumentSession history mechanism described here  
**Baseline:** `main` at `de56a6d51ae830eae4b20ee5739e50aabe015c50` after completed B1

## 1. Goal

Remove the history-depth-dependent deep copy performed before every accepted DocumentSession command while preserving the current Undo/Redo model and strong consistency.

C1 is deliberately narrow.

It removes this current preparation pattern:

```cpp
std::vector<HistoryEntry> prepared = history_;
prepared.resize(cursor_);
prepared.push_back(HistoryEntry{document_.state(), after});
```

without replacing the history representation itself.

## 2. Confirmed baseline finding

Static audit of the C1 baseline confirms the AUDIT-01 finding.

`DocumentSession::commitCommandState()` currently copies the entire `history_` vector before every non-no-op command commit.

Each `HistoryEntry` owns:

```text
before : PartAuthoredState
after  : PartAuthoredState
```

and `PartAuthoredState` contains value-owned Document properties plus hosted Sketch models with value-owned entity collections.

Therefore copying `history_` deep-copies prior authored snapshots. The preparation cost grows with history depth even though an accepted new command logically needs only one new history entry plus possible Redo-branch truncation.

This finding is **CONFIRMED**.

C1 does not claim that all remaining authored-state copies are unnecessary. Command staging, Part transaction staging and the current two-snapshot HistoryEntry representation remain separate costs for C2 measurement.

## 3. Scope IN

- `DocumentSession::commitCommandState()` history preparation;
- private `HistoryEntry` move/copy constraints needed to make history relocation non-deep-copying;
- pre-allocation required before durable Part commit;
- Redo-branch truncation ordering;
- preparation of session-local Sketch EntityId high-water bookkeeping so post-commit bookkeeping does not allocate;
- focused DocumentSession history/branch/failure tests;
- regression coverage for current EntityId high-water behavior;
- current internal as-built documentation for DocumentSession history consistency;
- AUDIT-01/ACTIVE lifecycle records after Owner acceptance/completion.

## 4. Scope OUT

- changing `HistoryEntry{before, after}` snapshot representation;
- delta/event-sourced history;
- command replay history;
- compression/deduplication/copy-on-write state;
- persistent Undo/Redo;
- history depth or memory limits;
- history eviction policy;
- user preferences for Undo depth;
- multi-threaded DocumentSession mutation;
- changing Part transaction semantics from B2;
- changing B1 save/checkpoint behavior;
- changing command grammar, UI or Viewer behavior;
- C2 benchmark matrix or selecting a final long-term history representation.

Those questions remain C2 or later work.

## 5. Accepted local design if activated

C1 keeps:

```text
std::vector<HistoryEntry>
HistoryEntry = { before PartAuthoredState, after PartAuthoredState }
cursor_ = boundary between Undo and Redo
```

The command path remains:

```text
semantic command
→ prepare candidate authored state
→ prepare one history entry + non-throwing post-commit resources
→ PartDocumentTransaction
→ domain commit
→ publish history cursor/bookkeeping
```

No second history mechanism is introduced.

## 6. HistoryEntry copy prohibition

The private `HistoryEntry` must no longer be copyable as a vector element.

Required local property:

- copy construction deleted;
- copy assignment deleted;
- move construction supported and proven `noexcept`;
- move assignment supported and proven `noexcept`.

The implementation must compile-time verify the required no-throw move properties of the contained authored state/history entry.

Purpose:

1. the former full-history copy becomes mechanically impossible;
2. `std::vector` capacity growth relocates old entries by ownership move rather than deep copying their authored snapshots;
3. pre-reserving one additional slot can happen before durable mutation.

This is an internal implementation constraint, not a public API.

## 7. Command preparation order

For a non-no-op command, C1 requires this ordering.

### 7.1 Validate current command/history authority

Existing `verifyRevision()` and command-specific validation remain first.

No-op detection remains before history mutation.

### 7.2 Prepare exactly one pending history entry

Before the Part transaction mutates the live document, prepare:

```text
pending.before = current authored state
pending.after  = candidate authored state
```

Failure while creating this one entry leaves document, history, cursor and Redo unchanged.

C1 does not remove the snapshots inside the new entry.

### 7.3 Reserve history capacity before commit

Before durable mutation, ensure `history_` has capacity for `cursor_ + 1` entries.

Capacity growth may relocate existing entries only through their non-throwing move operations.

It must not deep-copy old `PartAuthoredState` snapshots.

Logical history size/cursor/Redo branch are not changed by this preparation.

### 7.4 Prepare Sketch EntityId cursor bookkeeping before commit

Current `absorbSketchEntityIdCursors()` may insert a new map node after a command commits, for example when a new Sketch is accepted.

C1 must not leave a potentially allocating bookkeeping step after durable document mutation.

Prepare the post-commit Sketch EntityId high-water map before the Part transaction. A bounded implementation may copy the current small cursor map and absorb the candidate state's cursors into that prepared map, then publish it by non-throwing swap after successful commit.

This metadata preparation is proportional to hosted Sketch cursor records, not history depth or authored geometry history.

Existing rules remain unchanged:

- Undo does not make committed EntityIds reusable;
- Redo restores the same EntityIds;
- a new branch after Undo allocates above the highest observed session-local cursor;
- Save/reopen persistence rules remain unchanged.

### 7.5 Commit Part transaction

Use the existing B2 transaction boundary unchanged.

If transaction creation, validation or commit fails:

- live document remains as defined by B2;
- history logical contents remain unchanged;
- cursor remains unchanged;
- Redo branch remains unchanged;
- prepared history/cursor resources are discarded.

### 7.6 Publish history only after changed success

Only after a successful changed Part commit:

1. destroy the obsolete Redo suffix `[cursor_, history_.end())`;
2. append the already-prepared pending entry into reserved capacity by non-throwing move;
3. advance `cursor_` to the new history end;
4. publish prepared EntityId cursor bookkeeping by non-throwing swap;
5. update `expected_revision_`.

There must be no allocating/deep-copying operation required to establish the matching Undo entry after the durable document changed.

## 8. Redo branch semantics

C1 preserves current branching semantics.

Given:

```text
A → B → C
        ↑ cursor after Undo to B
```

a rejected, failed or no-op command must preserve Redo to C.

A successful new command D creates:

```text
A → B → D
```

and only then discards C.

Redo is never destroyed merely because a command attempt began.

## 9. Failure/exception consistency

The required invariant is:

> A durable changed Part state must never exist without its matching new Undo entry because history preparation or bookkeeping allocation failed after commit.

Therefore all memory/resource preparation that can fail for the new history publication must occur before the owning Part transaction mutates the document.

C1 is not required to add a global allocator-failure framework.

The no-deep-copy/no-post-commit-allocation property should be proven by code ordering, deleted HistoryEntry copying, no-throw move/swap constraints and focused behavioral tests.

If implementation discovers another potentially throwing post-commit operation required for correctness, stop and move that preparation before commit rather than accepting a partial-success path.

## 10. Required automated evidence

### C1.1 Normal linear history

Execute several changed commands.

Require:

- one accepted changed command -> exactly one additional Undo entry;
- revision behavior unchanged;
- Undo/Redo states unchanged from baseline.

### C1.2 No-op

Execute a no-op while history exists.

Require:

- no new history entry;
- no cursor change;
- Redo branch, if present, remains intact;
- no revision change.

### C1.3 Branch after Undo

Create at least three history entries, Undo into the middle, then execute a changed command.

Require:

- old Redo suffix is discarded only after accepted change;
- retained Undo prefix is correct;
- new command contributes exactly one entry;
- Undo/Redo traverses the new branch and cannot resurrect the abandoned branch.

### C1.4 Rejected command with Redo available

With a non-empty Redo branch, issue a command rejected before commit.

Require document state, revision, Undo depth and Redo depth unchanged.

### C1.5 Transaction failure

Exercise an existing deterministic Part transaction failure such as changed commit at exhausted technical revision.

Require:

- no history entry added;
- document authored state/revision unchanged;
- cursor and history depths unchanged.

### C1.6 EntityId high-water regression

Run existing COPY/Undo/new-branch identity regressions.

Require:

- abandoned committed IDs are not reused;
- Redo restores prior IDs;
- new COPY after Undo allocates above preserved high-water;
- persistence behavior remains green.

### C1.7 Compile-time history relocation contract

Build must enforce that private history entries cannot be copied and can be relocated by no-throw move.

The old `std::vector<HistoryEntry> prepared = history_` pattern must therefore be impossible to reintroduce without a compile failure.

### C1.8 Full regression

Existing DocumentSession, Part transaction, persistence, Sketch command, Undo/Redo, B1 and UI regressions remain green.

## 11. Performance claim boundary

C1 may claim only:

> adding a new command no longer deep-copies all previous history entries.

C1 must not claim a final Undo performance/memory solution.

Remaining per-command whole-state costs are intentionally left visible for C2 measurement, including current authored-state snapshots and transaction staging.

No wall-clock threshold or synthetic benchmark pass/fail is introduced in C1. AUDIT-01 C2 owns the Release benchmark matrix and any deeper representation choice.

## 12. Expected implementation surface

Expected bounded files:

- `src/application/include/simplesolid2/application/document_session.hpp`;
- `src/application/document_session.cpp`;
- focused DocumentSession/COPY history tests;
- `docs/internal/PART_DOCUMENTS.md` or another existing internal current-state document;
- generated Product Browser only if canonical documentation changes included by the documentation generator;
- work lifecycle records.

No Part domain, persistence schema, Viewer, UI or Shared 2D semantic change is expected.

## 13. Stop conditions

Stop for Owner review if implementation requires:

- replacing the two-snapshot HistoryEntry representation;
- changing public DocumentSession command/history API;
- persistent history;
- a new generic command/event framework;
- changing Part transaction semantics;
- changing EntityId lifecycle rules;
- changing save/dirty semantics;
- adding history limits/eviction policy;
- introducing a third-party dependency;
- weakening Undo/Redo or identity tests;
- timing-based acceptance thresholds;
- any D2 public API, ownership, identity, persistence or dependency change.

If one of these becomes necessary, C1 must be amended rather than silently absorbing C2 or another architecture decision.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: C1 changes internal DocumentSession history preparation/consistency and performance characteristics while preserving the observable Undo/Redo workflow and product semantics.

## 14. Activation gate

This proposal does **not** activate C1.

Activation requires explicit Owner acceptance of this bounded Work Contract.

No new ADR is required for the proposed D1 implementation.

Only after Owner acceptance may `work/ACTIVE.yaml` switch from completed B1 to active C1 and production/test implementation begin.

## 15. Completion gate

C1 completes only after:

- the confirmed full-history-copy operation is removed;
- HistoryEntry copy is prohibited and no-throw relocation is enforced;
- potentially allocating history/cursor preparation occurs before durable Part mutation;
- Redo is truncated only after successful changed commit;
- all required C1 automated evidence passes;
- existing transaction/history/identity/persistence regressions remain green;
- required internal documentation is current;
- generated Product Browser is current when applicable;
- exact-head Windows FULL passes;
- Owner manual UI testing is not required by default because C1 has no intended user-visible behavior change;
- governance closeout/CLOSURE passes;
- merge to main.

After C1 completion, AUDIT-01 requires C2 as the next separately contracted package. C2 is not activated by C1.

## 16. Implementation checkpoint

**Runtime candidate:** `e88e7559493707252323f758dba3d8325f894a1d`  
**Windows FULL:** #667 — PASS

Implemented within the accepted C1 boundary:

- removed the `std::vector<HistoryEntry> prepared = history_` full-history copy;
- private `HistoryEntry` copy construction/assignment are deleted;
- compile-time assertions require no-throw move construction/assignment for `PartAuthoredState` and `HistoryEntry`;
- history capacity for the accepted branch is reserved before Part mutation;
- one pending `HistoryEntry` is fully prepared before the Part transaction;
- Sketch EntityId high-water bookkeeping is copied/prepared before the transaction and published after success by no-throw map swap;
- Redo suffix destruction happens only after successful changed commit;
- the pending entry is appended after commit without capacity growth;
- no-op, rejected command and failed Part transaction leave logical history/cursor unchanged;
- current two-snapshot history representation, save/dirty semantics, B2 transaction rules and B1 persistence behavior remain unchanged.

Focused DocumentSession evidence added for linear relocation/branching, no-op and rejected command while Redo exists, branch replacement traversal and revision-exhaustion transaction failure. Existing COPY EntityId high-water and full repository regressions remained green in #667.

This checkpoint is not completion. Required internal documentation and a final exact-head Windows FULL still precede closeout.

## 17. Final completion record

C1 is complete on exact-head candidate `848fd01295bd9a93b882cca1d2df6cc362e8a265`.

Final evidence:

- Windows FULL #669 checked out that exact SHA and passed documentation freshness, Build, selector checks and the full regression suite;
- DOCS #668 independently regenerated and verified the Product Browser after the internal as-built update;
- the former full `history_` vector copy is removed from `commitCommandState()`;
- `HistoryEntry` is non-copyable and no-throw movable, making deep-copy relocation of prior history mechanically unavailable;
- one pending entry, vector capacity and EntityId high-water bookkeeping are prepared before the Part transaction;
- after successful mutation the Redo suffix is erased, the prepared entry is moved into reserved capacity and cursor bookkeeping is swapped without allocation;
- no-op and rejected commands preserve Redo;
- changed branching after Undo replaces only the abandoned Redo suffix and traverses correctly through Undo/Redo;
- revision-exhaustion transaction failure preserves authored state, revision and history depths;
- existing COPY identity/high-water, Part transaction, persistence/save and UI regressions remain green.

C1 makes no claim that the current two-snapshot history representation is optimal. It removes the confirmed history-depth-dependent copy while intentionally leaving deeper representation, benchmark and history-budget decisions to C2.

Owner manual UI verification is not required because C1 has no intended user-visible behavior change.

C1 acceptance conditions are satisfied. This closeout commit now requires only CLOSURE and merge of PR #81.

After merge, AUDIT-01 requires a separate Owner-accepted C2 Work Contract before deeper history production mutation.
