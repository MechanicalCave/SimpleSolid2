# B1 — Native Part Save Conflict Protection

**Status:** ACCEPTED — ACTIVE  
**Proposed:** 2026-09-27  
**Owner acceptance:** 2026-09-27  
**Decision class:** D2 persistence/save semantics + bounded D1 implementation  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package B1  
**Architecture:** ADR-0013 (ACCEPTED)  
**Baseline:** `main` at `069f913383a0c813f9a8e9a664aef8863360dc44` after completed B2

## 1. Goal

Prevent ordinary Save from silently overwriting a native Part file that changed, was replaced, disappeared or is concurrently being saved by another cooperating SS2 instance after this session's last load/save checkpoint.

B1 is about file-version authority at the persistence boundary.

It does not change Part authored mutation semantics, Undo/Redo representation or technical DocumentRevision rules.

## 2. Accepted architecture if activated

Implementation shall follow ADR-0013:

1. native load/create return a runtime `PartFileCheckpoint`;
2. checkpoint contains expected DocumentId, exact byte length, SHA-256 and platform file identity;
3. ProjectSession passes that exact checkpoint into DocumentSession;
4. ordinary Save requires that checkpoint;
5. Save acquires a per-target cooperative guard before checking/publishing;
6. target is compared against checkpoint by durable identity, file object and exact content;
7. missing/replaced/changed/busy conditions return typed save conflicts;
8. successful publication returns the new checkpoint;
9. DocumentSession updates saved authored state + file checkpoint only on success;
10. ordinary Save never recreates a missing target and has no force-overwrite bypass;
11. atomic temp-file reservation is hardened to true exclusive creation on supported Windows;
12. strict cross-process guarantee applies to cooperating SS2 writers only.

## 3. Scope IN

- Part native-file checkpoint type and checkpoint propagation;
- exact-byte SHA-256 fingerprinting;
- supported-Windows file-object identity capture;
- typed native Save conflicts;
- cooperative per-target save guard covering check -> publish;
- safe exclusive temporary-file reservation;
- create-new exclusivity regression;
- PartDocumentStore load/create/save checkpoint integration;
- ProjectSession create/open checkpoint propagation;
- DocumentSession checkpoint storage and conflict mapping;
- conflict-safe saved-state/dirty/history behavior;
- tests for sequential and controlled concurrent SS2 writers;
- internal docs and PL/EN Product docs;
- generated Product Browser;
- AUDIT-01/ACTIVE lifecycle records.

## 4. Scope OUT

- Force Overwrite;
- Save As / recovery UI;
- automatic conflict merge;
- file watching/live reload;
- B2 transaction changes;
- C1/C2 history redesign;
- cloud-sync-specific integration;
- filesystem journaling or full power-loss durability guarantees;
- distributed locks;
- arbitrary external-process compare-and-swap guarantees;
- persistent schema changes;
- Assembly/Drawing native save implementation.

## 5. File checkpoint contract

The session checkpoint represents one exact native file version.

Required fields/meaning:

```text
expected_document_id
exact_byte_length
sha256(exact_file_bytes)
platform_file_identity
```

Timestamp may be captured for diagnostics but is not authority.

Path text is not file identity.

The checkpoint is runtime-only.

## 6. Checkpoint creation

### 6.1 Load

Checkpoint and `PartDocument` must come from the same inspected native file version.

The implementation must not parse one version and then silently adopt a different version by re-reading later.

### 6.2 Create new

Successful create-new returns the checkpoint of the exact newly published target.

ProjectSession passes that checkpoint into the newly created DocumentSession.

### 6.3 Save

Successful conditional Save returns the checkpoint of the newly published target.

The session replaces its old checkpoint only after complete Save success.

## 7. Conditional ordinary Save

Save decision is fail-closed.

Under the cooperative guard:

1. confirm target still exists;
2. inspect/read target and capture current file identity;
3. parse enough native metadata to obtain current durable DocumentId;
4. compare DocumentId;
5. compare platform file identity;
6. compare exact byte length + SHA-256;
7. build replacement bytes;
8. publish atomically;
9. capture new target file identity;
10. return new checkpoint.

Conflict checks happen before publication.

## 8. Typed conflict semantics

Required Part-store conflict codes:

- `save_conflict_busy`;
- `save_conflict_target_missing`;
- `save_conflict_document_identity_changed`;
- `save_conflict_file_replaced`;
- `save_conflict_content_changed`.

Invalid/non-regular/unreadable target remains fail-closed and must never be treated as safe-to-overwrite.

Application diagnostics must preserve whether the failure is a Save conflict rather than generic I/O.

## 9. Cooperative save guard

Guard requirements:

- deterministic per canonical target;
- cross-process among SS2 instances;
- atomically acquired;
- released automatically on normal close;
- process termination must not create a permanent false ownership state on supported Windows;
- held from before target inspection until after successful new-checkpoint capture or failure cleanup;
- competing Save fails promptly with typed busy conflict;
- no infinite waiting/retry loop.

Implementation may use a sidecar/OS handle internally. It must not become durable model identity or native schema.

## 10. Ordinary Save side effects

On conflict/failure:

- no Part authored mutation;
- no DocumentRevision mutation;
- no Undo/Redo mutation;
- `saved_state_` unchanged;
- file checkpoint unchanged;
- target must remain untouched by this failed attempt;
- local dirty authored changes remain dirty.

On success:

- published bytes represent current document;
- saved authored checkpoint becomes current document state;
- file checkpoint becomes returned published checkpoint;
- history remains unchanged.

## 11. Missing/moved target

If the session target path no longer names the checkpointed file, ordinary Save fails.

A missing target is not recreated.

If the file was moved elsewhere, B1 does not search for it or retarget the session.

## 12. External writer guarantee

B1's strict no-lost-update guarantee is for cooperating SS2 writers that honor the same save guard.

For unrelated writers:

- modifications present before our checkpoint validation are detected;
- a race after our validation cannot be guaranteed impossible;
- docs must state this limitation accurately.

Do not call B1 a general filesystem CAS.

## 13. Atomic publication hardening

B1 must fix temporary reservation so the temp path is acquired with OS-level exclusive create semantics rather than exists-check + truncating open.

On Windows, create-new target publication must also remain fail-if-exists at publication time.

Atomic replacement, checkpoint conflict detection and crash durability remain distinct claims.

## 14. Required automated evidence

### B1.1 Sequential two-session lost-update prevention

- Session A and B open same checkpoint.
- A mutates + saves successfully.
- B mutates + ordinary Save.
- B returns typed conflict.
- A's bytes remain on disk.
- B remains dirty with its own in-memory changes/history intact.

### B1.2 Controlled concurrent cooperating Save

Force/coordinate two store Save attempts from same checkpoint.

Require:

- only one guard owner enters conditional publication;
- competitor returns `save_conflict_busy` or, after owner finishes, another typed stale-file conflict;
- both cannot publish from same checkpoint.

### B1.3 Content changed with misleading metadata

Modify current file content while preserving byte size and, where controllable, similar timestamp metadata.

Require SHA-256 mismatch conflict.

### B1.4 File replaced, same content/DocumentId

Replace path with a different file object carrying byte-identical contents.

Require `save_conflict_file_replaced`.

### B1.5 Document identity changed

Replace target with valid native Part having different DocumentId.

Require `save_conflict_document_identity_changed`.

### B1.6 Target missing

Delete/move target.

Ordinary Save returns `save_conflict_target_missing` and does not recreate old path.

### B1.7 Conflict side effects

For every conflict class verify:

- disk bytes unchanged by failed Save;
- Part state/revision unchanged;
- Undo/Redo depth unchanged;
- saved authored checkpoint unchanged;
- session file checkpoint unchanged;
- local dirty state remains correct.

### B1.8 Repeated normal Save

- successful Save returns a new checkpoint;
- immediate second Save without external change succeeds;
- save/reopen remains valid and clean.

### B1.9 Publication failure

Inject/produce publication failure after validation.

Require old checkpoint + saved authored state remain unchanged and local document remains dirty.

### B1.10 Create-new/temp reservation

Verify:

- competing create-new cannot both publish same target;
- temporary file reservation cannot be claimed by two cooperating writers;
- cleanup leaves no durable lock/temp artifact after success/failure on supported Windows.

### B1.11 Persistence regressions

All existing native container/schema/malformed-file tests remain green.

## 15. Expected implementation surface

Expected bounded files may include:

- `src/persistence/include/simplesolid2/persistence/atomic_file.hpp`;
- `src/persistence/atomic_file.cpp`;
- a small persistence SHA-256/file-snapshot helper if needed;
- `src/part/include/simplesolid2/part/part_document_store.hpp`;
- `src/part/part_document_store.cpp`;
- `src/application/include/simplesolid2/application/document_session.hpp`;
- `src/application/document_session.cpp`;
- `src/application/project_session.cpp`;
- persistence/store/session tests;
- internal + PL/EN Product docs;
- generated Browser;
- governance lifecycle files.

No CAD UI/editor/provider semantic code should require modification except existing generic diagnostic presentation if necessary.

## 16. Stop conditions

Stop for Owner review if implementation requires:

- changing native file schema;
- force-overwrite or Save As UX;
- automatic content merge;
- file watcher/reload architecture;
- OS-global/distributed lock service;
- claiming arbitrary-writer atomic CAS;
- broad Project discovery redesign;
- changing Part authored/history semantics;
- changing B2 transaction policy;
- introducing a third-party online/network dependency;
- weakening atomic publication tests.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: ordinary Save gains explicit conflict outcomes when the native file was changed, replaced, removed or is concurrently guarded by another SS2 instance.

## 17. Activation gate

This proposal does **not** activate B1.

Activation requires explicit Owner acceptance of:

- ADR-0013;
- this B1 Work Contract;
- exact-content + file-identity checkpoint;
- cooperative per-target guard;
- missing-target conflict/no implicit recreation;
- no force-overwrite in B1;
- strict guarantee limited to cooperating SS2 writers;
- Windows-exclusive temp reservation hardening.

Only after acceptance may `work/ACTIVE.yaml` switch from completed B2 to active B1.

## 18. Completion gate

B1 completes only after:

- all IN-scope behavior implemented;
- required sequential/concurrent conflict evidence passes;
- existing DocumentSession/ProjectSession/persistence regressions remain green;
- internal + PL/EN Product docs current;
- generated Browser current;
- exact-head Windows FULL passes;
- Owner manual Windows verification covers at least ordinary Save after external modification/removal and normal repeated Save;
- CLOSURE passes;
- merge to main.

After B1 completion, AUDIT-01 requires C1 as the next separately contracted package.
