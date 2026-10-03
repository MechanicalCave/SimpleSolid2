# PM-00A — E09 Stale Generation / Session Publication Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-03  
**Exact source candidate:** `018966aa143a006cdd39429a201755c54c3e7948`  
**Windows FULL:** #1313 — PASS  
**Core-only:** 19/19 PASS  
**Kernel-native aggregate:** 26/26 PASS  
**Desktop FULL:** 84/84 PASS  
**Regression:** `pm00a.e09_stale_publication`  
**Matrix:** `work/PM-00A_E01_E10_EVIDENCE_MATRIX.md` v1.0

## Result

E09-01 through E09-04 PASS.

Stale publications: **0**.

The evidence demonstrates that runtime result publication must be authorized by the context that produced the result rather than by geometric equality or durable DocumentId alone.

The bounded prototype uses:

```text
DocumentId
+ exact DocumentRevision
+ canonical DocumentSession generation
+ request generation / terminal request state
        ↓
publication authority
```

The prototype is deliberately local to the regression. It is architecture evidence for PM-00B, not a production API or persistent schema.

## Evidence summary

| Row | Expected | Actual | Result |
| --- | --- | --- | --- |
| E09-01 | result started at revision R is rejected after authored mutation to R+1 | old ticket returns stale_revision; no evidence is published | PASS |
| E09-02 | Session-A result is rejected after canonical Session B replaces it for the same durable DocumentId | real ProjectSession close/reopen preserves DocumentId but old ticket returns stale_session | PASS |
| E09-03 | geometrically identical old result remains stale | identical neutral ShapeEvidence at the same document revision cannot publish from the older request generation | PASS |
| E09-04 | cancelled/failed older completion cannot replace newer evidence | cancelled and failed requests remain terminal; newer published payload and request generation remain current | PASS |

## E09-01 — revision ownership

The probe begins one evidence request against the canonical `DocumentSession` at revision R.

Before that request publishes, an ordinary semantic `SetDocumentPropertiesCommand` commits through the existing command/transaction path and advances the Part Document to R+1.

The old ticket is then presented for publication.

Required and actual result:

```text
ticket.revision = R
canonical DocumentRevision = R+1
→ stale_revision
→ published evidence unchanged
```

A fresh request captured at R+1 publishes successfully.

This reuses the existing accepted `DocumentRevision` authority rather than inventing a parallel model revision.

## E09-02 — canonical session replacement

The test uses a real initialized Project Workspace and real `ProjectSession`.

Session A is clean and canonical for one durable Part Document. A result ticket is captured, then:

```text
ProjectSession
→ closeDocument(DocumentId)
→ no canonical open session
→ openDocument(same DocumentId)
→ new canonical Session B
```

The durable `DocumentId` is identical before and after reopen.

The publication prototype issues a new runtime session generation when the canonical session owner is rebound to Session B. The old Session-A ticket is rejected as `stale_session`.

This proves that durable Document identity alone is insufficient publication authority across runtime-session replacement.

The session generation is runtime evidence only. It is not persisted.

## E09-03 — geometric equality is not authority

Two evidence requests are started under the same canonical Session B and the same `DocumentRevision`.

Both use byte-for-byte equal neutral `kernel::ShapeEvidence` payloads.

The newer request publishes first. The older request then completes with the identical payload.

Required and actual result:

```text
same DocumentId
same DocumentRevision
same neutral geometry evidence
older request generation
→ stale_generation
→ newer publication remains current
```

No comparison of area, counts, topology, provider handles or other geometry diagnostics can revive publication authority.

## E09-04 — cancellation / failure are terminal

The prototype records request lifecycle independently from payload geometry.

For cancellation:

```text
request C begins
→ C cancelled
→ newer request N begins and publishes
→ late completion of C
→ cancelled / no replacement
```

For failure:

```text
request F begins
→ F failed
→ newer request N2 begins and publishes
→ late completion of F
→ failed / no replacement
```

The stored published payload and published request generation remain those of the newer accepted request.

A cancelled or failed request therefore cannot regain authority merely because asynchronous work later produces a value.

## Publication-authority prototype

The test-local gate captures four independent dimensions:

1. durable `DocumentId`;
2. exact `DocumentRevision`;
3. runtime canonical-session generation;
4. runtime request generation plus terminal request state.

These dimensions answer different stale cases:

- DocumentRevision rejects authored-state replacement;
- session generation rejects results from a superseded runtime owner even for the same DocumentId;
- request generation rejects older results within the same unchanged session/revision;
- terminal state prevents cancelled/failed requests from publishing.

The payload is deliberately not part of the authority comparison.

## Architecture implication

E09 provides evidence for the later PM-00B publication/evaluation contract:

- publication should be validated at the owning runtime boundary immediately before making evaluated evidence current;
- the durable DocumentId identifies which document the result concerns but does not identify the current runtime session;
- existing DocumentRevision should remain the authored-state freshness token;
- a non-durable session generation/lease is required to distinguish canonical runtime-session replacement;
- a non-durable request/evaluation generation is required to prevent out-of-order completion within one session/revision;
- cancellation/failure must remove publication authority permanently for that request;
- geometric equality cannot bypass any freshness check.

This evidence informs O-09 downstream failure/retry/history semantics and O-12 identity/snapshot/read-boundary foundations.

PM-00B still owns the production type names, owning class/service, exact threading model and publication API.

## Evidence limitations

E09 does not claim that SS2 already has a production asynchronous solid evaluator.

The probe intentionally answers the architecture question before that evaluator exists.

No provider-specific asynchronous scheduling, thread pool or UI event-loop behavior is required to establish the freshness rule. Later production integration must prove that its actual completion path performs the same authority checks.

No provider generated/modified/deleted observation is relevant to E09. No geometry similarity, topology ordinal or provider handle influences publication.

False-Resolved count is not applicable to this publication-only package; the corresponding integrity metric is stale publications, which is **0**.

## Verification

Windows FULL #1313 checked out exact SHA `018966aa143a006cdd39429a201755c54c3e7948`.

The E09 regression passed in:

```text
core-only                 19/19 PASS
kernel-native aggregate   26/26 PASS
desktop FULL              84/84 PASS
```

The regression is compiled and executed without Qt or OCCT. The kernel-native lane reruns the neutral core-only graph beside the existing provider regressions and also passes.

## Scope boundary

E09 introduced no:

- production async-evaluation manager;
- durable session/request generation;
- BodyId / FeatureId persistence;
- Part Feature Tree;
- user-facing solid operation;
- Qt/Viewer behavior;
- OCCT/provider identity;
- topology ordinal or geometry-similarity publication authority;
- schema migration.

**Next:** E10 tolerance/refine/healing matrix and O-11 evidence.
