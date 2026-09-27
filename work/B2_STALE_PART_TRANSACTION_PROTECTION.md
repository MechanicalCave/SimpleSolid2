# B2 — Stale Part Transaction Protection

**Status:** PROPOSED  
**Proposed:** 2026-09-27  
**Decision class:** D2 domain transaction semantics + bounded D1 implementation  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package B2  
**Architecture:** ADR-0012 (PROPOSED)  
**Baseline:** `main` at `2982310b1bc18fc007763c31f33155f836afbba7` after completed WB-02 / AUDIT-01 Package A

## 1. Goal

Prevent a stale `PartDocumentTransaction` from replacing newer accepted authored state.

The minimum failure scenario is:

```text
PartDocument revision N

T1 created at N
T2 created at N

T1 commits -> revision N+1
T2 commit attempt -> stale_transaction
document remains exactly at T1 result
```

B2 hardens the owning Part domain boundary. It does not rely solely on `DocumentSession::verifyRevision()`.

## 2. Why this is active in AUDIT-01

AUDIT-01 requires B2 immediately after WB-02 because the current transaction stages a full authored-state snapshot without binding that snapshot to the revision from which it was derived.

A stale whole-state commit is a silent lost-update risk and therefore a correctness problem, not an optimization.

## 3. Accepted architecture if activated

Implementation shall follow ADR-0012:

1. `PartDocumentTransaction` captures immutable `base_revision_` at construction.
2. `PartDocument` commit boundary receives the expected/base revision.
3. Freshness mismatch returns `PartCommitErrorCode::stale_transaction` before validation/no-op/state replacement.
4. Complete candidate state is validated at the Part domain boundary.
5. Invalid state returns `PartCommitErrorCode::invalid_state`.
6. Every commit attempt is terminal; later commit returns `inactive_transaction`.
7. Rollback is terminal/idempotent.
8. Unchecked Part authored-state reconstruction is replaced by a validated reconstruction result.
9. Existing application/history and persistence behavior is adapted to these typed results without introducing a second mutation/history path.

## 4. Scope IN

- Part-domain transaction base-revision capture;
- typed `stale_transaction` commit result;
- typed `invalid_state` commit result;
- one-shot transaction lifecycle;
- Part authored-state validator for current invariants;
- validated Part reconstruction API replacing unchecked authored-state restore;
- persistence adaptation to validated reconstruction;
- DocumentSession diagnostic propagation of new commit codes;
- domain/application/persistence tests required by this contract;
- internal as-built docs;
- AUDIT-01/ACTIVE lifecycle metadata after acceptance/completion.

## 5. Scope OUT

- B1 file/save conflict detection;
- filesystem checkpoints, hashes, mtime/size comparison or multi-process save locking;
- history representation/performance changes C1/C2;
- command grammar or UI changes;
- CAD Input changes;
- new Part features;
- Assembly/Drawing transaction implementation;
- generic cross-domain transaction framework;
- multi-threaded mutation guarantees;
- automatic merge/rebase of stale transactions;
- persistent schema changes;
- changes to `EngineeringRevision`.

## 6. Transaction lifecycle

### 6.1 Construction

Transaction captures:

```text
document pointer
base revision
staged authored state snapshot
active = true
```

### 6.2 Commit decision order

The owning domain commit path evaluates in this order:

1. transaction active/document valid;
2. current document revision equals base revision;
3. complete staged Part state is valid;
4. staged state equals current state -> successful no-op;
5. next technical revision is available;
6. atomically replace authored state and publish next revision.

No later step may execute after an earlier failure.

### 6.3 Terminal semantics

Before returning from the first `commit()` attempt, the transaction becomes inactive.

Therefore:

- success -> inactive;
- no-op -> inactive;
- stale -> inactive;
- invalid state -> inactive;
- revision exhausted -> inactive.

A subsequent commit returns `inactive_transaction`.

`rollback()` sets inactive and has no authored effect. Calling rollback again remains harmless.

## 7. Freshness semantics

### 7.1 Stale is based only on technical revision

Do not compare pointers, object identity, history cursor, dirty state or timestamps.

```text
base_revision != document.revision()
=> stale_transaction
```

### 7.2 Stale check occurs before no-op

A stale transaction never becomes authorized because its final state happens to compare equal to the current state.

### 7.3 No merge

B2 rejects stale state; it does not attempt to merge independently modified properties/sketches.

A future semantic merge would require its own contract.

## 8. Part authored-state validation

B2 shall introduce one domain validator used by commit and reconstruction.

Current minimum valid-state conditions:

- every `PartSketchSupport::valid()` is true;
- every `SketchPlacement::valid()` is true;
- `sketchPlacementMatchesSupport(...)` is true for each Sketch;
- no duplicate hosted `SketchId`;
- contained `SketchModel` instances remain trusted through their own construction/restore invariants.

The validator must be deterministic and provider-neutral.

It must not inspect Qt, Viewer, OCCT, presentation tokens or UI state.

## 9. Reconstruction boundary

Current unchecked:

```text
PartDocument::restore(id, state, revision)
```

must no longer return a live document for invalid Part authored state.

The implementation may define the exact result struct/name as D1, but it must provide:

- success with reconstructed `PartDocument`;
- typed/structured invalid-state failure;
- supplied technical revision preserved on success;
- no partial document on failure.

`PartDocumentStore::load()` maps reconstruction rejection to its existing malformed/invalid native Part diagnostic family.

No file schema or serialized field is added.

## 10. Application/history integration

`DocumentSession::verifyRevision()` remains unchanged in purpose.

`commitCommandState()` continues to:

- prepare history before durable commit;
- mutate history only after successful changed Part commit;
- leave history untouched on stale/invalid/exhausted/inactive failure;
- propagate `PartCommitErrorCode` through `DocumentSessionDiagnostic::commit_code`.

Undo/Redo remains ordinary authored-state mutation through the same transaction boundary.

B2 must not weaken existing `history_diverged`, revision cursor, fresh-ID or technical cursor behavior.

## 11. Required automated evidence

### B2.1 Domain overlap

Create T1 and T2 against revision N.

T1 changes property A and commits.

T2 changes property B from the old staged snapshot and attempts commit.

Require:

- T1 success/changed;
- revision N+1;
- T2 `stale_transaction`;
- T1 property remains;
- T2 old snapshot cannot erase T1;
- revision remains N+1.

### B2.2 Stale no-op

A transaction created at N is made stale by another accepted commit.

Even if stale staged state is later made/evaluates equal to current state, commit returns `stale_transaction`.

### B2.3 One-shot lifecycle

Cover second commit after:

- changed success;
- no-op success;
- stale failure;
- invalid-state failure;
- revision-exhaustion failure.

All second attempts return `inactive_transaction`.

### B2.4 Rollback

Rollback leaves state/revision unchanged.

Commit after rollback returns inactive.

Repeated rollback remains harmless.

### B2.5 No-op and revision increment

Current fresh no-op:

- success;
- changed=false;
- no revision increment.

Fresh changed commit:

- success;
- changed=true;
- exactly one revision increment.

### B2.6 Revision exhaustion

At max revision:

- fresh no-op remains successful/no-change;
- fresh changed commit returns `revision_exhausted`;
- authored state/revision unchanged;
- transaction becomes inactive.

### B2.7 Invalid candidate state

Use the public domain transaction API to stage a complete Part state violating a current invariant, e.g. invalid Sketch support/placement or duplicate SketchId.

Require:

- `invalid_state`;
- no document mutation;
- no revision change;
- transaction terminal.

### B2.8 Validated reconstruction

Require:

- valid state reconstructs with exact requested technical revision;
- invalid state is rejected;
- persistence load of valid existing fixtures remains green;
- malformed invalid native state remains fail-closed.

### B2.9 Application/history regression

Run existing DocumentSession coverage including:

- command commit;
- no-op;
- Undo/Redo;
- dirty checkpoint behavior;
- failed save behavior;
- Sketch create/add/update/erase/duplicate command paths.

No history entry may appear for a rejected Part commit.

## 12. Expected implementation surface

Expected bounded files may include:

- `src/part/include/simplesolid2/part/part_document.hpp`;
- `src/part/part_document.cpp`;
- `src/part/part_document_store.cpp`;
- `src/application/document_session.cpp` only if typed mapping requires adaptation;
- Part/DocumentSession/persistence tests;
- internal docs;
- work/ADR lifecycle files.

No UI, Viewer or provider file should require semantic changes.

## 13. Stop conditions

Stop for Owner review if implementation requires:

- changing persistent schema;
- defining file/save conflict semantics (B1);
- redesigning Undo/Redo representation (C);
- adding cross-domain generic transaction templates/framework;
- adding multi-thread locking/atomic concurrency claims;
- merging stale states automatically;
- changing stable identity rules;
- weakening PART-01 revision/no-op semantics;
- broad new Part authored invariants unrelated to current stored state;
- user-visible overwrite/conflict UX.

## 14. Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: B2 changes internal consistency/failure behavior and domain APIs, not normal product interaction. Reclassify if implementation surfaces a user-facing error/workflow.

## 15. Activation gate

This proposal does **not** activate B2.

Activation requires explicit Owner acceptance of:

- ADR-0012;
- this B2 Work Contract;
- optimistic base-revision policy;
- one-shot lifecycle;
- stale-before-no-op decision;
- current Part state-validation boundary;
- validated reconstruction boundary.

Only after acceptance may `work/ACTIVE.yaml` switch from completed WB-02 to active B2 and production implementation begin.

## 16. Completion gate

B2 completes only after:

- all IN-scope behavior implemented;
- required automated evidence passes;
- existing Part/Sketch/DocumentSession/persistence regressions remain green;
- internal docs current;
- exact-head Windows FULL passes;
- Owner manual testing is **not required by default** because B2 has no intended UI change, unless implementation exposes user-visible behavior;
- closeout CLOSURE passes;
- merge to main.

After B2 completion, AUDIT-01 requires B1 as the next separately contracted package.
