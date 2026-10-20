# R13-A1 / D2-P21 — strict native Part v21 relation persistence — ADDENDUM PROPOSAL

**Status: OWNER-ACCEPTED EXACT D2-P21-1 / D2-P21-2 / D2-P21-3 (2026-10-20). GOVERNANCE PR DRAFT / NOT MERGED. NO IMPLEMENTATION OR MERGE AUTHORIZATION.**
**Date:** 2026-10-20. **Decision owner:** SimpleSolid2 Owner.
**Governance base:** protected `main@66d36af5a327a62a19c45869df150e223edcd818`, with `work/ACTIVE.yaml` pointing to `work/SKETCH_R13_A1_LOCAL_RELATIONS_D2_WORK_CONTRACT_PROPOSAL.md` (active R13-A1; retain the historical heading of that contract as history). **Product implementation evidence:** Draft PR #506 at `9e3fa46210032246a6ed9455764aebd6470e59e4`, **not merged**. Original PlaneGCS 0.8.0 native and OCCT tests 2/2 plus normal Windows FAST 94/94 PASS at this product HEAD, Windows PR gate #3902. Those PASS results are **not v21 persistence evidence**.

**Owner acceptance record:** Owner reviewed this exact substantive proposal at PR #511 head `4ae4a712cfe7080f40b353fa45d8e36b2488875a`, and responded **“akceptuję - kontynuuj pracę”**, accepting all three exact D2-P21 decisions. [Auditable PR comment](https://github.com/MechanicalCave/SimpleSolid2/pull/511#issuecomment-6295923290). This status-only update changes no accepted schema or test fields. The proposal remains governance-only and does not mutate the active Work Contract, its allowlist or `work/ACTIVE.yaml`; effective incorporation/activation and any product edit require their separate gates.

## 0. Existing authorities and concrete reason for v21

- SS1 `MechanicalCave/SimpleSolid` supplies inspected donor algorithms, numeric cases and UX references only. SS2 Sketch owns local semantic `ConstraintId` and authored seed geometry; `DocumentSession` owns atomic revisions/history; Part owns `.ss2part`, projection, Profile and Feature evaluation. No project-wide parametric graph.
- Native Part store header `src/part/include/simplesolid2/part/part_document_store.hpp` currently declares `PartDocumentStore::current_schema_version = 20`. The v20 writer `src/part/part_document_store.cpp` serializes each `sketches[].model` as EXACTLY `{"next_entity_id": "...", "entities": [...]}`, and the legacy parser requires exactly those two members. v20 cannot represent local relations.
- Draft PR #506 intentionally rejects Save of any Sketch with `constraintCount() != 0` as `PartStoreErrorCode::unsupported_schema`. It already implements six manual semantic kinds and their `SketchModelState` identity/cursor, but does not authorize or implement persistence.
- Accepted A1 scope is `Horizontal(Line)`, `Vertical(Line)`, `Coincident(Line endpoint, Line endpoint)`, `Parallel(Line, Line)`, `Perpendicular(Line, Line)` and `Fix(Line)`; `Fix` is an immutable-parameter mask in the solver, not an extra equation.
- A1.3 explicitly requires strict vNext, v20 migration, cold Save/Close/Reopen, stable IDs, revision-/stage-bound evaluation and durable downstream Part repair. v21 was previously a **candidate**; this proposal seeks the exact independent D2 decision.

## 1. Version and ownership — exact proposed D2-P21 disposition

1. **Approve native Part domain schema 21** in the existing `.ss2part` native container. Only on independent acceptance/activation may the writer and `current_schema_version` become 21; until then the v20 fail-closed Save guard MUST remain.
2. v21 changes **only** `sketches[i].model`: retain existing `next_entity_id` and `entities` fields with their v20 schema/semantics, and add **exactly** `next_constraint_id` and `constraints`. All existing top-level authored fields, `sketches[i]` fields (`id`, `support`, `visible`, `projected_edges`), Body/Profile/Datum/Axis records, container descriptor/document identity, checkpoint and atomic-write protocols retain their previous meanings and shapes.
3. No new top-level `constraints`, cross-Sketch links, global parameters, solver snapshot, extra Feature reference, UI annotation, dimension field, tolerance override, second document store or file extension.
4. Write **only v21** after activation; read all previously supported versions (in particular real v20) through their unchanged version-specific decoders. Upgrading a loaded v20 document happens only on its next explicitly requested Save/Save As; do not silently rewrite on Load. No v21-to-v20 downgrade/export is authorized.
5. Old v20 must remain **strictly two-key model** on read, rejecting smuggled `constraints` or `next_constraint_id` rather than silently dropping them. A v21 file requires exactly all four keys, including for an unconstrained Sketch. The version-specific parser must not weaken v1–v20 checks.
6. `modeling_semantics_version` stays unchanged. Do not conflate native file version 21 with geometric/feature semantics version.

## 2. Exact v21 Sketch model record

Only the `model` nested inside a normal v20-compatible Part Sketch record is shown below. This is a **schema illustration**, not a container payload or a benchmark assertion.

```json
{
  "next_entity_id": "4",
  "entities": [
    {"kind":"line","id":"1","role":"regular","start":[0.0,0.0],"end":[10.0,0.0]},
    {"kind":"line","id":"2","role":"regular","start":[10.1,0.2],"end":[10.3,7.0]},
    {"kind":"line","id":"3","role":"regular","start":[10.2,7.1],"end":[0.2,7.0]}
  ],
  "next_constraint_id": "4",
  "constraints": [
    {"id":"1","kind":"fix","first":{"entity_id":"1","element":"line"},"second":null},
    {"id":"2","kind":"coincident","first":{"entity_id":"1","element":"line_end"},"second":{"entity_id":"2","element":"line_start"}},
    {"id":"3","kind":"horizontal","first":{"entity_id":"3","element":"line"},"second":null}
  ]
}
```

**Exact grammar:**
- `next_entity_id`: unchanged v20 canonical typed decimal-string cursor, with unchanged entity rules.
- `entities`: unchanged v20 JSON Line/Circle/Arc records and `EntityRole`; their coordinates remain AUTHOR-EDITED SEEDS, never overwrite with numerically solved geometry.
- `next_constraint_id`: required JSON **string** containing canonical, base-10, unsigned, nonzero `ConstraintIdCursor`; restore exact monotonic cursor even if `constraints` is empty or previously deleted IDs create gaps. No silent reset to 1; no inferred cursor from the largest currently live ID.
- `constraints`: required JSON **array** (possibly empty) of exactly four-field objects `id`, `kind`, `first`, `second`, preserving their authored array order. Neither parser nor serializer may reorder to normalize a semantic pair.
- `id`: canonical decimal-string, nonzero typed Sketch-local `ConstraintId`. Unique within its containing Sketch, strictly smaller than its cursor, not globally unique across Sketches.
- `kind`: one and only one lower-case string from `horizontal`, `vertical`, `coincident`, `parallel`, `perpendicular`, `fix`. Unknown or future kinds require a future approved file schema.
- `first`: required exactly two-field object `{"entity_id": "<decimal EntityId>", "element": "<enum>"}`. `entity_id` must resolve to an existing **Line in the same Sketch**. Allowed element strings: `line`, `line_start`, `line_end`. `none` is not legal on disk.
- `second`: **JSON null** for the three unary kinds `horizontal`, `vertical`, `fix`; required exactly the same two-field object as `first` for the binary kinds `coincident`, `parallel`, `perpendicular`. No omission, empty object, default-constructed reference, empty string or synthetic `"none"` representation.
- Reject any additional/missing/unknown keys at the v21 model, relation or reference level, wrong JSON types, invalid decimal encodings (`0`, leading zero, sign, whitespace, overflow), wrong element/kind casing, duplicate IDs or structurally inconsistent references. The parser must not silently drop a relation.

**Semantic shape matrix:**

| `kind` | `first.element` | `second` | Rules |
| --- | --- | --- | --- |
| `horizontal` | `line` | `null` | Single existing local Line |
| `vertical` | `line` | `null` | Single existing local Line |
| `fix` | `line` | `null` | Same Line’s authored endpoints are the immutable numeric parameters; no separate persisted values |
| `coincident` | `line_start` or `line_end` | Endpoint reference object | Endpoints of two **distinct** local Lines; order of refs is a semantic presentation detail, not a new identity |
| `parallel` | `line` | Whole-Line reference object | Two distinct local Lines |
| `perpendicular` | `line` | Whole-Line reference object | Two distinct local Lines |

A relation may not refer to Circle, Arc, a non-existent Line, another Sketch, a generated OCCT edge, runtime selection token, ephemeral solved point, or a Profile/Feature as its local anchor. An EntityId matching a Line only in another Sketch remains missing in the owning Sketch.

## 3. Validation and exact-load failure semantics

**Three mandatory validation layers:**

1. **Container and version:** keep native container authentication/integrity, document kind/id, size and schema-range guards. Unsupported future version (`> 21`) -> `PartStoreErrorCode::unsupported_schema`. Failed read must not replace the current live document, dirty/history state or file checkpoint.
2. **Strict v21 decoding:** reject missing/extra keys, wrong type, invalid string IDs/cursor, duplicate constraint IDs, zero/noncanonical/out-of-range values, future enum kinds, bad reference shapes and nonlocal/wrong-entity-kind references. Map structural/semantic corruption to `PartStoreErrorCode::malformed_document` with a deterministic diagnostic; do **not** skip corrupt relations and load a supposedly healthy partial document.
3. **SS2 Sketch/Part authored invariants:** call existing `SketchModel::restore` on the complete candidate including constraints/cursor, then `PartDocument::restore`. Enforce `ConstraintId < next_constraint_id`, no duplicate semantics even for swapped binary pair, no direct Horizontal/Vertical or Parallel/Perpendicular contradiction already forbidden by the Sketch model, valid Fix/other relations on immutable authored Line seeds, valid existing `EntityId` and Part host/projection support invariants. No ID reallocation, auto-fix, auto-weld, cursor burn or partial publication.

**Structural corruption != numerical solver failure.** A structurally valid authored relation system may be underconstrained or become numerically nonconvergent because of current stage/projection; its document **must remain loadable** without a third-party DEV solver. The *derived* Profile/Feature is instead marked Blocked/Failed/Unavailable/repair-required and no stale last-good BRep is current. If a relation violates a **current core-authored invariant** such as incompatible Fix on known fixed seed, the file fails to load; do not defer that intrinsic data corruption to the backend.

Never call PlaneGCS or require its installation merely to parse/restore the document; the runtime solver is separately injected when actual stage evaluation is requested.

## 4. Lifecycle and transaction invariants

- **Add relation:** one `DocumentSession` command, revision guard and one Undo item; allocate new local `ConstraintId` only after all authoring validation succeeds. Save persists its exact `kind`/semantic endpoints and the monotonic next cursor.
- **Remove relation:** remove ONLY the selected relation; preserve seed geometry and Profile/Feature authored IDs. Save/Close/Reopen must not resurrect it; the cursor remains monotonic.
- **Delete local user geometry:** atomically cascade only local authored constraints whose endpoints/Lines actually refer to deleted Entities. Preserve unrelated local constraints and all durable downstream Part Profile/Feature authored records. Derived evaluation shows explicit repair-required/blocked status when profile integrity is lost, never stale BRep.
- **Projected PG-01 input:** keep existing projected binding and stage ownership; do not persist a second source-of-truth geometry snapshot in a relation, silently unlink projected edges, move fixed PG-01 source or bypass existing Delete restrictions.
- **Undo/Redo:** exact semantic `ConstraintId`, references, kind, cursor, source geometry, authoring order, profile/feature identities and revision history. No numeric solver candidate is a history item.
- **Cold Save/Close/Reopen:** saved document becomes a new session with same authored geometry, SketchId, ConstraintIds/cursors, all projections, Profiles, Features and document identity; derived solve must be recomputed only against **the reopened document's current revision/stage**. Reopening is not equivalent to replaying an in-memory state copy.
- **Save conflict/atomicity:** retain current native checkpoint preconditions, existing-file hash/file identity, temp+atomic replace and no partial write on conflict. Failed validation and failed Save must not falsely mark document clean or persist partial constraints.
- All previously supported unconstrained Part/Sketch/Feature work must remain semantically backward-readable. The writer should emit v21 with an empty `constraints` array and an explicit cursor even if a Sketch has no relations.

## 5. RED → GREEN test contract (actual registered SS2 CTest)

Tests use actual `PartDocumentStore`, real `DocumentSession`, existing `.ss2part` native container and, for geometry, native OCCT and caller-injected original pinned PlaneGCS where permitted. Do not substitute fabricated payload-only memory tests for cold file lifecycle.

**P21-01 — exact serializer/round trip:** native v21 JSON shape and exact 4-key Sketch model; six allowed kinds with semantic Line endpoints, local IDs, cursor and untouched authored seed coordinates. Include two Sketches with independently allocated local `ConstraintId "1"` to prove no global namespace collision.

**P21-02 — empty, deleted and gap cursors:** zero constraints with mandatory `[]` and `"1"` on a new Sketch; a deleted relation with a retained advanced cursor; non-consecutive historical IDs; new post-reopen relation must use the saved cursor and never recycle a retired ID.

**P21-03 — v20 and legacy migration:** load an **actual existing v20 fixture** and at least one supported legacy version; assert no authored relations, default local cursor 1, same Sketch/Profile/Feature/support/projection intent; explicit Save creates v21 and does not rewrite the original v20 on Load. Confirm a v20 payload containing forged v21 fields fails closed rather than downgrading silently. No unsupported v21-to-v20 Save.

**P21-04 — malformed record matrix (table-driven):** absent/extra keys at each level; wrong types; unknown kinds/elements; omitted versus null `second`; wrong first/second shape; `none` element; forbidden unary second; missing binary second; zero/leading-zero/sign/overflow IDs and cursor; cursor <= live ID; duplicate IDs; duplicate meaning including reversed binary pair; direct contradictions; inconsistent Fix; references to missing Line, Circle, Arc, deleted entity, second Sketch; nonfinite/corrupt seed geometry; future schema version. Every corrupt case must fail whole Load with correct error class and no partially published session.

**P21-05 — command/history:** Add and Erase relation, failed/stale commands do not mutate revision or consume IDs, one-command Delete cascade, Undo/Redo restoring original IDs and exact seed state, branch-after-Undo next-ID non-reuse; Save/Close/Reopen after representative accepted states.

**P21-06 — same document native Part:** cold v21 reopen with constrained Sketch and durable Profile/Extrude and Revolve. With accepted original solver injection, current-stage evaluation produces valid OCCT solids with exact seam and unchanged authored seed; without such injection original default Part evaluator stays fail-closed. No persisted solved/cached state.

**P21-07 — invalid downstream repair:** after Delete of constrained geometry, cold Save/Close/Reopen retains ProfileId and FeatureId, derived Feature remains Blocked/repair-required and no last-good BRep appears current; restore geometry/constraints through an explicitly valid command/history path and prove fresh-stage native reevaluation. Runtime must reject stale solver candidates from before Save/Reopen or Delete.

**P21-08 — projected PG-01 and stage:** stage-qualified fixed sources remain fixed, bindings unchanged through cold reopen, native evaluation correctly refuses missing/ambiguous/upstream-invalid stages and does not turn invalid sources into editable authored seeds.

**P21-09 — atomic failure and conflict:** invalid Save cannot create/overwrite a file or mark session clean; busy/missing/replaced/content-changed checkpoint classes and restored user document/history remain preserved. Byte corruption and over-limit container fail closed via existing native file integrity checks.

**P21-10 — regression and acceptance:** registered Windows Core, native Kernel/OCCT and Qt/Desktop FOCUSED → FAST → ready FULL on exact product HEAD. Maintain existing baseline and separately required original PlaneGCS native job; no claimed PASS until real results. DOC-01 internal + PL/EN + generated Browser paths need separately certified exact-path D2 approval; Owner manual Windows cold reopen/Sketch→Profile→Extrude and later Owner-reviewed UI walkthrough are independent product gates. No A1 FINAL on schema-green alone.

## 6. Compatibility, diagnostic and security consequences

- Readers: v1–v20 retain prior strict expected shapes; v21 is a new explicit decoder branch, not a permissive reinterpretation of old records. Future v22+ unsupported until separately approved.
- Persistence is **authored intent**, not a solver memoization format; original Line/Circle/Arc source seeds remain v20-like. Do not serialize DoF, residuals, conflict diagnostics, backend IDs, BRep, viewport selections, temporary command state, numeric solved candidate or tolerance settings.
- A v21 file contains no automatic constraints, Driving dimensions or Reference dimensions; A2/A3 remain distinct future schema and UI contracts.
- Existing pre-save file guard must reject unsupported or invalid states before atomic replacement; load errors carry deterministic contextual diagnostics where possible without leaking path-dependent internal state or silently repairing data.
- No vendor code, archive downloads, CMake/third_party additions, LGPL distribution clearance or publicly shipped default solver is implied by this persistence schema.

## 7. Implementation sequencing AFTER independent Owner approval (not authorized here)

1. Introduce strict native file-version v21 gate and exact-model V21 parser/serializer **only** in the authorized Part store paths; keep existing v20 parser and tests unchanged; make new RED tests first.
2. Validate exact `ConstraintId` cursor and kind/element semantic references through existing `SketchModel::restore` and Part restore; never call a native solver from the decoder.
3. Prove v20 migration, cold file lifecycle and negative fail-close matrix; then staged Profile/Extrude/Revolve/repair and projection regressions on registered SS2 native tests.
4. Only after implementation-specific paths, DOC-01 and acceptance gates are separately cleared: exact product HEAD Windows FULL/closure, relevant documentation/Browser and Owner manual PASS, independently assessed A1 technical FINAL and separate explicit product guarded-squash authorization.
5. No UI work until Owner's actual SS2 Sketcher walkthrough and separate exact scope/layout acceptance. A2/A3 remain separate contracts.

## 8. Proposed finite product path authority — ONE new header, others already conditional in A1

**New exact path whose addition specifically requires Owner D2 approval:**
- `src/part/include/simplesolid2/part/part_document_store.hpp` — change **only** `PartDocumentStore::current_schema_version` from 20 to 21 and any narrowly unavoidable store-local declaration, if specifically shown in a reviewed patch. This header is NOT in the currently effective A1 conditional allowlist. No approval of this draft = no mutation.

**Already listed in accepted R13-A1 contract; use minimally and only for named v21 scope:**
- `src/part/part_document_store.cpp` — version-specific strict serialize/parse and existing Save guard transition.
- `tests/sk02b_part_sketch_persistence_test.cpp` — native cold v20/v21 read/write, corrupt data, atomic/checkpoint regressions.
- `tests/r13_a1_local_constraints_test.cpp` — semantic Command/Undo/Redo, original solver and Part/OCCT lifecycle.
- `tests/sk02b_part_sketch_model_test.cpp` — Sketch/local identity if independent regression evidence requires it.
- `src/part/part_document.cpp` — **only if** native Part authored-state invariants demonstrably require adjustment; otherwise unchanged.
- `src/application/document_session.cpp` and `src/application/include/simplesolid2/application/document_session.hpp` — **only if** an actually reproduced cold-session integration defect requires a scoped repair, not a general API rewrite.
- Existing `src/sketch/sketch_model.cpp`, `src/sketch/include/simplesolid2/sketch/sketch_model.hpp` and `src/sketch/include/simplesolid2/sketch/local_constraints.hpp` — only proven narrow invariants/validation fixes; no new relation kind or public Kernel API.

**Not authorized by this draft:** new files under `src/**` or `tests/**`, tests registration outside the existing A1 allowlist, `.github/**`, `cmake/**`, `third_party/**`, `docs/**`, installer/release, UI `src/ui/**`, `work/ACTIVE.yaml`, work contract changes, new public Kernel API, unrelated Part/Solver refactor. Any extra actual dependency path, including DOC-01 Markdown and generated Browser, requires its **own finite D2 addendum** before mutation.

**Document governance scope NOW:** this one NEW proposal document at `work/SKETCH_R13_A1_V21_NATIVE_PART_SCHEMA_D2_ADDENDUM_PROPOSAL.md` on a separate governance Draft PR. No change to actual A1 contract or product code in this governance PR.

## 9. Exact Owner decision and stop rules

The following are **separate** decisions; none is silently accepted by approving a general direction:

- **D2-P21-1:** accept/amend/reject *exact native domain schema v21*, the mandatory four-key `model`, exact relation/ref tags and unary-null/binary-object rules in §§1–3, v20 unchanged read path and no solver-state persistence.
- **D2-P21-2:** accept/amend/reject the **single additional conditional product source file** `src/part/include/simplesolid2/part/part_document_store.hpp` as limited in §8. Other A1 conditional paths keep their original approved limits.
- **D2-P21-3:** accept/amend/reject exact RED→GREEN, cold Save/Close/Reopen, negative, migration, repair and Windows gates §§4–7. Documentation exact paths, UI layout, LGPL/source packaging and final product merge remain **independent approvals**.

**Current decision status: exact D2-P21 Owner ACCEPTED; governance Draft PR #511 remains unmerged and awaits its separate explicit guarded-squash merge permission.** This acceptance and any governance CI do NOT flip schema version, authorize v21 implementation, change `ACTIVE.yaml`, finish R13-A1, accept release of PlaneGCS, or authorize guarded squash of any PR. Incorporation of the reviewed and explicitly accepted D2 text into the active contract is itself a separate governance/readback step. Product PR #506 remains independent and Draft until its own completion and permission.
