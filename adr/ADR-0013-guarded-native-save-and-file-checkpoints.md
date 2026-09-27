# ADR-0013 — Guarded native-file Save with durable file checkpoints

**Status:** PROPOSED  
**Proposed:** 2026-09-27  
**Decision class:** D2 persistence consistency / ordinary Save conflict semantics  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package B1  
**Related:** ADR-0004, ADR-0012, PERSIST-01, PART-01

## Context

AUDIT-01 Package B1 requires ordinary Save to stop silently replacing a native Part file that changed after the document was loaded or last successfully saved.

Today `DocumentSession::save()` calls `PartDocumentStore::save(path, document)`, and the store performs an atomic replacement of the target path. Atomic replacement protects publication shape, but it does not prove that the file still represents the version originally opened by this session.

A simple pre-Save timestamp/size check is insufficient. A simple content hash read immediately before rename is also insufficient to claim atomic compare-and-swap against arbitrary writers: another process may modify the target after our check and before our publish.

Two cooperating SS2 instances must not silently overwrite one another. At the same time, B1 must be explicit that unrelated processes which ignore the SS2 cooperative guard remain outside the strict cross-process guarantee.

## Decision

### 1. Open documents carry a durable file checkpoint

A successful native Part load or create-new publication returns a `PartFileCheckpoint` representing the exact published/opened file version.

The checkpoint contains at least:

- expected durable `DocumentId`;
- exact file byte length;
- SHA-256 digest of the exact native file bytes;
- platform file identity captured from the opened target file.

On supported Windows, file identity uses a stable file-object identity from the opened handle (volume identity plus file ID), not path text, timestamp or byte size.

The checkpoint is runtime-only and is not serialized into the native document.

### 2. Checkpoint ownership flows into DocumentSession

`ProjectSession::createPart()` passes the checkpoint returned by successful create-new publication into the new `DocumentSession`.

`ProjectSession::openDocument()` passes the checkpoint returned by the exact load snapshot into the new `DocumentSession`.

`DocumentSession` must not reconstruct/adopt a checkpoint by casually re-reading the path after load/create. The checkpoint must correspond to the same publication/snapshot from which the in-memory document was established.

### 3. Ordinary Save is conditional

Ordinary Save requires the current session checkpoint.

Save does not mean "replace whatever is currently at this path".

Under a cooperative save guard, the store inspects the current target and compares it with the checkpoint.

Conflict precedence is:

1. target missing -> `target_missing`;
2. current target cannot be inspected as the expected regular native file -> fail closed;
3. current durable `DocumentId` differs -> `document_identity_changed`;
4. platform file identity differs -> `file_replaced`;
5. exact byte length or SHA-256 differs -> `content_changed`;
6. otherwise conditional publication may proceed.

A byte-for-byte identical replacement is still a replacement conflict when platform file identity changed.

### 4. Save conflict is typed

B1 introduces typed Part-store conflict outcomes distinct from ordinary I/O failure.

Required conflict codes:

- `save_conflict_busy`;
- `save_conflict_target_missing`;
- `save_conflict_document_identity_changed`;
- `save_conflict_file_replaced`;
- `save_conflict_content_changed`.

The application layer must preserve enough typed information to distinguish a save conflict from a generic persistence failure.

B1 may add a dedicated `DocumentSessionErrorCode::save_conflict` if that is the smallest clear application contract.

### 5. Cooperating SS2 instances serialize check -> publish

B1 introduces a deterministic per-target cooperative save guard.

The guard must be acquired atomically and held across:

```text
inspect current target
-> compare checkpoint
-> build/write temporary replacement
-> publish replacement
-> capture checkpoint for newly published target
```

Two cooperating SS2 processes targeting the same canonical native file must not both pass the same checkpoint check and then both publish.

If one SS2 instance already owns the guard, another ordinary Save fails closed with `save_conflict_busy`; it does not wait indefinitely and does not overwrite.

On supported Windows the guard should use an OS-backed exclusive create/open primitive with automatic cleanup on handle close/process termination where feasible.

The exact internal guard filename/handle type is D1 implementation detail and must not become durable CAD identity.

### 6. Successful Save produces the new checkpoint

After successful publication, the store captures the file identity of the newly published target and returns a checkpoint representing that published version.

The checkpoint content digest is computed from the exact bytes published.

`DocumentSession` updates both:

- its saved authored-state checkpoint;
- its file checkpoint;

only after the store reports successful publication and a valid new checkpoint.

### 7. Conflict has no authored/history side effects

Any B1 conflict:

- leaves the target file unchanged by this Save attempt;
- leaves the in-memory Part authored state unchanged;
- leaves technical `DocumentRevision` unchanged;
- leaves Undo/Redo unchanged;
- leaves the existing file checkpoint unchanged;
- leaves `saved_state_` unchanged.

Therefore `needsSave()` retains its current semantic definition based on authored-state comparison with the last successfully saved authored state.

If there are no local authored changes but the external file changed, explicitly invoking Save still detects and reports the file conflict.

### 8. Missing target is not recreated by ordinary Save

If the file was deleted or moved away after load/save, ordinary Save returns `save_conflict_target_missing`.

It does not silently recreate the old path.

A future explicit Save As / Recover / Force Overwrite workflow requires a separate contract.

### 9. Force overwrite is not part of B1

B1 does not provide an override button/API that bypasses checkpoint validation.

If force overwrite is introduced later it must be a separate explicit user operation with its own UI/product contract.

### 10. Atomic publication and conflict detection remain separate guarantees

Existing atomic replacement remains responsible for publishing one complete file rather than exposing a partial target.

B1 checkpoint/guard logic is responsible for cooperative stale-file conflict detection.

Neither one implies full crash/power-loss durability.

B1 does not claim fsync-equivalent persistence of every storage-controller cache or directory metadata unless separately proven and contracted.

### 11. Safe temporary-file reservation is hardened

The current "check random temp name, then open/truncate" sequence is not a sufficient exclusive reservation contract.

On supported Windows, temporary-file creation for atomic publication must use an OS primitive with create-new/exclusive semantics so two writers cannot accidentally claim the same temporary path.

Create-new publication must likewise fail if the target appeared before publication.

### 12. Strict concurrency guarantee is limited to cooperating SS2 writers

B1 guarantees that cooperating SS2 instances using the same guarded save protocol cannot silently overwrite one another from the same stale checkpoint.

For an unrelated external process, editor, cloud-sync client or tool that ignores the SS2 guard:

- changes visible before checkpoint validation are detected;
- changes that race after our validation cannot be claimed as atomically prevented;
- B1 must not describe a pre-publish digest comparison as OS-level compare-and-swap against arbitrary writers.

This limitation must be documented in internal/product documentation.

## Rejected alternatives

### Timestamp + size checkpoint

Rejected. Same timestamp/size does not prove identical content or file identity.

### Content hash only

Rejected. A file can be replaced byte-for-byte; B1 must also detect replacement of the underlying file object.

### DocumentId only

Rejected. In-place content changes can preserve DocumentId.

### Pre-rename hash without cooperative guard

Rejected as the SS2-to-SS2 guarantee because two SS2 instances could both validate the same old version before either publishes.

### Unconditional retry after conflict

Rejected. Retrying without explicit user decision could turn a detected conflict back into silent overwrite.

### Ordinary Save recreates a missing target

Rejected. Missing/moved target is a conflict, not implicit Save As.

## Consequences

Positive:

- ordinary Save becomes stale-file aware;
- two cooperating SS2 instances cannot silently overwrite one another from the same checkpoint;
- exact content change, durable DocumentId replacement and file-object replacement are distinguishable;
- missing target fails closed;
- current dirty/history semantics remain intact;
- checkpoint is updated only from successful publication.

Costs:

- load/create/save results carry a file checkpoint;
- DocumentSession stores one runtime checkpoint;
- persistence needs SHA-256, platform file identity and cooperative guard support;
- Windows atomic temp reservation must be hardened;
- Save can now fail with a user-visible conflict diagnostic.

## Verification

Required automated evidence includes:

- two sessions load the same checkpoint; first Save succeeds; second Save conflicts and cannot overwrite first;
- controlled concurrent cooperating Save: one guard owner; competing Save returns busy/conflict and does not publish;
- same-size/same-metadata content modification is detected by SHA-256;
- same DocumentId + byte-identical replacement is detected by file identity;
- different DocumentId replacement is reported distinctly;
- missing/moved target is a conflict and ordinary Save does not recreate it;
- conflict preserves disk bytes, authored state, revision, dirty checkpoint and Undo/Redo;
- successful Save updates checkpoint and a second unchanged Save succeeds;
- successful Save/reopen produces a checkpoint matching the published file;
- publication failure leaves old checkpoint and dirty state unchanged;
- create-new remains exclusive;
- temporary publication file reservation is exclusive on supported Windows;
- existing persistence malformed-file and schema regressions remain green.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: ordinary Save gains visible conflict behavior when another process/session changed, replaced or removed the native Part file.
