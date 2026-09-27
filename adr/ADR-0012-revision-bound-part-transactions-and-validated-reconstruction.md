# ADR-0012 — Revision-bound Part transactions and validated authored-state reconstruction

**Status:** PROPOSED  
**Proposed:** 2026-09-27  
**Decision class:** D2 domain transaction safety / shared Part mutation semantics  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package B2  
**Related:** PART-01, ADR-0004, ADR-0008, ADR-0009

## Context

AUDIT-01 Package B2 identified a domain consistency hole in the current Part transaction API.

`PartDocumentTransaction` snapshots the full `PartAuthoredState` when it is created, but it does not capture the `PartDocument::revision()` that owns that snapshot. Two transactions may therefore start from the same old state:

```text
revision N
  -> T1 snapshots state at N
  -> T2 snapshots state at N
  -> T1 commits state A -> revision N+1
  -> T2 commits stale state B based on N
```

Without an explicit freshness check, the T2 commit can replace the whole authored state and silently erase effects accepted through T1.

The application layer already uses `DocumentRevision` to detect command/history divergence, but the domain transaction itself must be safe even when used directly. Domain safety cannot depend on every caller remembering an application-level pre-check.

A second related boundary is currently too weak: `PartDocumentTransaction::replaceState()` accepts a complete staged authored state and `PartDocument::restore()` reconstructs one directly. B2 must not leave a public route that can bypass the Part authored-state invariants while hardening transaction freshness.

## Decision

### 1. Transactions use optimistic base-revision binding

Every `PartDocumentTransaction` captures both:

- the current authored state;
- the current `DocumentRevision`;

at construction time.

That captured revision is the transaction's immutable **base revision**.

Commit authority is:

```text
transaction.baseRevision == document.currentRevision
```

If the revisions differ, commit fails with the typed domain error:

```text
PartCommitErrorCode::stale_transaction
```

and the document is unchanged.

The freshness comparison occurs in the owning `PartDocument` commit boundary before any authored-state replacement or revision increment.

### 2. Freshness precedes no-op equivalence

A stale transaction is rejected even if its staged state happens to equal the current document state.

This makes transaction authority deterministic and fail-closed: an object created from an obsolete revision never receives commit authority merely because later state happens to compare equal.

### 3. Part transactions are one-shot

A transaction has exactly one terminal action:

- `commit()`; or
- `rollback()`.

Any `commit()` attempt consumes the transaction regardless of result:

- changed success;
- no-op success;
- stale-transaction rejection;
- invalid-state rejection;
- revision-exhaustion rejection.

A later `commit()` on that same transaction returns:

```text
PartCommitErrorCode::inactive_transaction
```

and cannot mutate the document.

`rollback()` is idempotent and leaves the transaction inactive.

This prevents retrying a staged/moved state after the world that created it has changed.

### 4. Final authored-state validation belongs to the Part domain commit boundary

`PartDocument` validates the complete candidate `PartAuthoredState` before accepting it.

For the current Part model, B2 defines the minimum authored-state invariants as:

- every hosted Sketch support is valid;
- every Sketch placement is finite/valid;
- every Sketch placement matches its declared support;
- hosted `SketchId` values are unique within the Part;
- each contained `SketchModel` is assumed valid by its own construction/restore invariant and is not re-parsed by Part.

Invalid candidate state fails with:

```text
PartCommitErrorCode::invalid_state
```

with no authored mutation and no revision increment.

`replaceState()` remains a staging adapter only. It never bypasses commit validation.

### 5. Reconstruction is validated rather than unchecked

The current unchecked `PartDocument::restore(...)` API is replaced by a validated reconstruction result.

The reconstructed authored state must pass the same Part authored-state validation used by transaction commit before a `PartDocument` is returned.

Persistence already validates/parses serialized Sketch models before constructing `PartAuthoredState`; B2 adds the owning Part-domain validation boundary rather than relying solely on parser discipline.

A reconstruction failure is mapped by `PartDocumentStore` to its existing malformed/invalid-document diagnostic family. B2 does not introduce a new persistent schema.

### 6. Revision semantics remain unchanged otherwise

Preserve PART-01 rules:

- accepted changed commit increments technical `DocumentRevision` exactly once;
- successful no-op does not increment revision;
- revision exhaustion rejects a changed commit;
- Undo/Redo continues to restore authored state through the normal transaction boundary and increments technical revision monotonically;
- `EngineeringRevision` remains unrelated.

### 7. DocumentSession remains a second consistency guard, not the transaction authority

`DocumentSession::verifyRevision()` remains useful for command/history diagnostics.

The Part transaction must still independently reject stale commits.

If a Part transaction fails with `stale_transaction`, the application layer reports its existing `transaction_failure` with the typed `commit_code`. B2 does not create a second history mechanism.

### 8. No thread-safety claim

B2 solves logical overlapping transaction freshness.

It does **not** establish concurrent multi-thread mutation of one `PartDocument`, locking, atomics, process synchronization or file-level compare-and-swap. Those require separate contracts.

## Rejected alternatives

### Exclusive single active transaction stored by PartDocument

Rejected for B2.

It would prevent useful independent staging and introduce active-transaction ownership/lifetime state into the document. Optimistic base-revision binding solves the stale overwrite directly with less shared runtime state.

### Rely only on DocumentSession::verifyRevision()

Rejected.

The domain transaction remains publicly usable and must be safe without application-layer discipline.

### Merge stale full-state snapshots

Rejected.

There is no accepted field/entity-level merge grammar for arbitrary Part authored state. Guessing a merge would create silent semantic conflicts.

### Accept stale no-op commits

Rejected.

Freshness is transaction authority, not merely a final-state comparison.

### Retry a failed transaction object

Rejected.

One-shot lifecycle is simpler, fail-closed and prevents reuse of staged state after stale/invalid/exhausted outcomes.

## Consequences

Positive:

- an old full-state snapshot cannot overwrite a newer accepted Part mutation;
- stale failure is typed and testable;
- direct domain callers and application callers receive the same safety;
- transaction lifecycle becomes deterministic;
- complete state replacement cannot bypass Part invariants;
- persistence reconstruction receives the same final Part invariant check.

Costs:

- `PartDocumentTransaction` stores one additional revision value;
- commit API gains typed error cases;
- reconstruction call sites must handle validation result;
- tests that intentionally construct max-revision documents must use the validated reconstruction result.

## Verification

Required tests include:

- T1/T2 overlap from the same base revision: T1 succeeds, T2 returns `stale_transaction`, T1 effects remain;
- stale rejection preserves state and revision exactly;
- stale no-op is still rejected;
- changed success increments revision once;
- current-revision no-op succeeds without revision increment;
- rollback leaves document unchanged and later commit is inactive;
- repeated commit after success is inactive;
- repeated commit after no-op is inactive;
- repeated commit after stale failure is inactive;
- repeated commit after revision exhaustion is inactive;
- invalid complete authored state returns `invalid_state` without mutation;
- validated reconstruction rejects invalid Part state;
- validated reconstruction accepts valid state at supplied technical revision;
- revision exhaustion behavior remains unchanged;
- DocumentSession command/Undo/Redo regressions remain green;
- persistence load/save/reopen regressions remain green.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: B2 changes internal domain consistency/error semantics without changing ordinary user-visible CAD workflow. If an implementation exposes a new user-facing diagnostic, Product docs must be reclassified before completion.
