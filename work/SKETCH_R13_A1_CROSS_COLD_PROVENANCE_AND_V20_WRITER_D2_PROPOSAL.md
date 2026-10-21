# R13-A1 — cross-cold PartEvaluation provenance and historical v20 writer evidence — D2 proposal

**Status:** PROPOSED / OWNER DECISIONS PENDING. Governance design only; **NOT an active contract amendment, NOT source/fixture authorization, NOT a merge authorization**.
**Prepared:** 2026-10-21.
**Foundation / authority:** Engineering Constitution 1.0; Foundation 1.0; Architecture 1.0; ADR-0012 (revision-bound Part transactions), ADR-0013 (guarded native Save), accepted R13-A1 active Work Contract `work/SKETCH_R13_A1_LOCAL_RELATIONS_D2_WORK_CONTRACT_PROPOSAL.md` including accepted D2-P21. `work/ACTIVE.yaml` remains untouched.
**Product evidence baseline:** Draft, unmerged [R13-A1 PR #506](https://github.com/MechanicalCave/SimpleSolid2/pull/506) at `f7eef9805bec267efed8cf4b1be9b9f55ca7c02e`. Normal Windows FAST **94/94** and original PlaneGCS native **2/2** passed on this head in [Actions #3941](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39500411178); exact-head FULL, DOCS, Owner/manual product acceptance and guarded product merge are **not** proven by this gate. This proposal does not change that status.

## 1. Exact diagnosis — neither overclaim nor dismissal

1. `PartDocumentStore::load()` invokes `PartDocument::restore(document_id, state)` without a revision. The default is `DocumentRevision{}` (zero). Two independent cold opens of the same file thus may yield the same durable `DocumentId` and numeric `DocumentRevision{0}`.
2. `PartEvaluation` records `source_revision` and optional `source_document_id`, but no independent Part runtime-incarnation identity. The exact-stage Q1 gate, `prepareStageLocalSolveProblem`, and other read-only Part query paths compare owner ID and/or numeric revision; those values alone cannot distinguish cold incarnation A from cold incarnation B.
3. The previously reported native Windows #3932 negative exposed a cross-cold/revision-collision test gap. A later bounded test correction did **not** fix the underlying identity predicate. This is a **possible stale evaluation acceptance**, **not proof** of an actually published stale B-Rep, provider token rebind, or corrupted persisted authored intent. No claim of such observed corruption is made.
4. The existing P21-03 test in `tests/sk02b_part_sketch_persistence_test.cpp` generates a labeled v20 container by removing the `constraints` and `next_constraint_id` fields from a **v21-writer-produced** Sketch JSON. It checks strict v20 read and explicit-Save upgrade, but is **not** an independent historical-v20-writer compatibility artifact.
5. Protected `main@6080e7774a44d6b8a50d32fedd085405bc81d038` still declares `PartDocumentStore::current_schema_version = 20` and therefore offers a pinned historical writer source candidate. Existing named `tests/fixtures/*.ss2part` must **not** be relabeled as writer-v20 evidence without inspecting their decoded descriptors and provenance. V16 fixtures are not substitutes.

## 2. D2-CP1 — proposed disposable runtime incarnation authority (independent YES/NO)

**Owner decision requested:** accept a nonpersistent, unique-on-create/restore **Part runtime incarnation identity** as an additional required authority witness for any caller-supplied `PartEvaluation`, without changing durable `DocumentId`, numeric `DocumentRevision`, feature IDs, native file schema, semantic geometry IDs, or kernel topology tokens.

### Proposed mechanism / invariants

- `PartDocument::create` and each independent `PartDocument::restore` manufacture a fresh opaque, immutable, process-local incarnation token. A **move of the same document** keeps that same token; constructing a new Part from bytes yields a different token even when file bytes, DocumentId and revision are identical. No persisted token or global project-wide cache.
- Prefer a reference-counted private token object, with identity by control-object/pointer equality while each `PartEvaluation` retains its token. This avoids pointer-address recycling as long as a stale evaluation survives; no random-number collision contract, timestamp authority, or resettable numeric counter. This is runtime provenance, **not a trust/security boundary against malicious code with access to internals**.
- An evaluation produced by the Part evaluator captures the exact owning Part incarnation together with the existing `source_document_id`, `source_revision`, and stage/Feature evidence. A default/fabricated evaluation missing the token is **invalid**, not implicitly current.
- One Part-owned fail-closed authority predicate requires **all**: same incarnation, same durable DocumentId, same numeric revision, and thereafter strict stage/Feature/topology checks appropriate to that consumer. Use it consistently at `prepareStageLocalSolveProblem`, `evaluateStageLocalSolve`, strict selected-edge/stage queries, PG-01 projection/projection-frame/effective-sketch, and Profile/Feature consumption where `PartEvaluation` enters. Do not simply patch one Sketch gate.
- A document mutation still invalidates old evaluations via revision; Undo/Redo, replaceState, cold reopen and DocumentSession restoration must never revive an evaluation from an earlier incarnation. Returning to geometrically identical authored state is not authority. A newly evaluated result from the new incarnation must work.
- Failed provenance admission calls **neither numeric solver nor provider** and publishes no profile/kernel input or current Body. Do not serialize or memoize the token; no global graph, no B-Rep cloning/revival and no new OCCT public identity API.
- Check the actual Part / DocumentSession restoration and move lifecycle before fixing implementation shape. If a valid in-session history operation reconstructs a new Part object, token rotation is allowed but requires new derived evaluation; never preserve authority across an independent restore for performance.

### Finite conditional source paths (new authority not active until separate Owner contract amendment)

| Exact repo-relative path | Exact permitted CP1 purpose |
|---|---|
| `src/part/include/simplesolid2/part/part_document.hpp` | **NEW** runtime incarnation member / narrow opaque accessor; no native schema fields |
| `src/part/part_document.cpp` | create/restore/move lifecycle and shared admission use |
| `src/part/include/simplesolid2/part/feature_evaluation.hpp` | **NEW** appended runtime-only optional incarnation evidence in `PartEvaluation`; preserve existing aggregate member order |
| `src/part/feature_evaluation.cpp` | capture at evaluation creation; replace unsafe authority checks at exact stage/read consumers |
| `src/part/profile_kernel_input.cpp` | enforce same owner at the evaluated Profile admission |
| `src/part/include/simplesolid2/part/effective_sketch_projection.hpp` | only if exact private gateway declaration is necessary; otherwise no edit |
| `src/part/part_document_store.cpp` | only if actual load/restore integration requires a store-local guard; no schema or checkpoint change |
| `tests/r13_a1_local_constraints_test.cpp` | registered cross-cold and Part/Sketch gate RED→GREEN assertions |
| `tests/sk02b_part_sketch_persistence_test.cpp` | cold-reopen provenance tests against exact real v21 native bytes |

No `src/application/**`, `src/core/**`, `kernel/**`, `third_party/**`, `ui/**`, new CMake paths, broad `tests/**`, or persistent file fields are granted by this proposal. If inspection proves another exact test/source path essential, **STOP and request separate finite D2 approval**.

### Minimum RED → GREEN matrix

1. Evaluate Part A, save and close; cold-load identical bytes into independent B, with identical `DocumentId` and both `DocumentRevision{0}`. Reuse A's `PartEvaluation` against B: **reject** before projection/provider/solver invocation; a new B evaluation **accepts**.
2. Repeat with independent restore of identical authored state and a live old `PartEvaluation`; reject despite equal stage FeatureIds, same line seeds and equal semantic topology labels.
3. Same incarnation but different revision: reject. Same incarnation plus exact revision, genuinely current stage: pass. A result missing token or belonging to a different DocumentId: reject.
4. Undo/Redo, any reconstruction/move and current-prefix/unavailable-Body cases preserve existing Stage/Feature repair semantics, no stale final Body publication, and all old/native A1/PG-01 regressions.
5. Negative test checks **backend/provider call count zero** when provenance fails; a test that only checks a later body status is insufficient.

**Acceptance is technical only after registered native CTest RED→GREEN + exact-head Windows appropriate FAST/FOCUSED/FULL and the original-PlaneGCS job as applicable.** CP1 is a D2 change of runtime evaluation authority; do not implement from this proposal alone.

## 3. D2-V20 — authentic historical writer fixture (independent YES/NO)

**Owner decision requested:** accept **one** exact immutable repository fixture path `tests/fixtures/r13a1_native_part_v20_writer_golden.ss2part` and one already allowlisted test file `tests/sk02b_part_sketch_persistence_test.cpp`, solely to prove native v20→v21 compatibility with bytes actually emitted by the historical v20 Part writer. The fixture path is **NEW and currently outside A1 allowlist**.

### Production of the evidence (must precede test code)

1. Freeze a real historical writer source SHA. Candidate is protected `main@6080e7774a44d6b8a50d32fedd085405bc81d038` with `current_schema_version = 20`; verify that the built executable actually links that writer and emits domain schema **20** before trusting it. No hand-authored package JSON, no backdating the v21 writer, no editing two v21 fields out of a v21 container. Existing older fixtures cannot satisfy this gate by filename alone.
2. Run the **actual old** `PartDocumentStore::createNew/save` via a bounded existing test harness or explicit one-shot historical writer harness using an Owner-approved single-checkout workflow; do not silently change the Owner's working `D:\SimpleSolid2` checkout or borrow CI's `D:\SS2Build`. An operator-generated independent file is acceptable; the creation method and build command must be recorded. Do not create a second Windows clone/worktree.
3. Fixture content: one deterministic, valid nonempty Part Sketch (prefer nondegenerate regular Line contour), with stable Sketch / Entity IDs and optional valid Profile/Extrude/Feature only if the v20 writer can produce them without UI or solver. Assert and record **actual** native descriptor and authored JSON keys, file length, SHA-256, exact historic writer SHA/target, environment, generation command, and expected semantic IDs. Geometry seeds must remain authored geometry. Never include user project data or proprietary documents.
4. Run the **old v20 reader** on exactly those bytes before freezing the fixture. Commit the immutable original output only after independently checking it is a valid native container, not an imitation assembled by a test helper.
5. Against v21: load the historical bytes *without modifying them*, assert domain schema 20, document and Sketch IDs, entities, all expected authored semantics, **no added constraints**, and initial Sketch constraint cursor 1; verify exact input-file hash unchanged by Load.
6. Explicit Save via normal existing checkpoint-guarded `PartDocumentStore::save` must produce schema 21 and required four-key Sketch models; cold close/reopen preserves all durable IDs and authored geometry. Check no automatically solved coordinates, runtime provider handles or spurious relations entered persistence. Keep existing synthetic P21-03, malformed v20 smuggling, and v21 strict rejection tests; the authentic fixture adds **independent** evidence.
7. Actual source path to open a fixture must be stable in CI (e.g. test's known source-tree fixture directory) and not require network access, an old toolchain during **current** CI, hidden runner files, or a new workflow/CMake path. If test harness cannot locate the single binary fixture without new source/runner paths, **STOP for another exact-path D2**, not an opportunistic CMake edit.

### Distinct evidence boundaries

- Passing synthetic P21-03 does **not** count as a historical writer-v20 artifact.
- Passing one genuine v20 fixture is one representative compatibility witness; it does **not** prove arbitrary future/malformed v20 inputs and does not replace parser fuzz/negative/syntactic tests.
- The `6080e777` commit SHA alone is **not** proof of the fixture's actual generating executable. Hash the output, record how it was produced and have a reviewer independently parse it.
- An unsupported or absent original historical executable is a recorded evidence **STOP**, not permission to forge a v20 file.

## 4. Governance, order, safety boundaries

1. This **single new Markdown document is a proposal**, with two **independent D2 yes/no decisions**: **CP1 runtime incarnation** (including two new Part headers and exact test scope), and **V20 original binary fixture path and test**. Owner may approve either, both, or neither; never treat general continuation as acceptance.
2. After explicit Owner acceptance, write a **separate one-file active-contract amendment** incorporating only accepted items; obtain its exact-head Windows governance CLOSURE, separate Owner guarded-squash merge permission, merge and protected-main readback. The proposal PR has no source authority even if reviewed or CLOSURE-green.
3. Only then make test-first product changes on Draft PR #506 or a separately Owner-approved successor, using the frozen allowlist. For V20: freeze and independently inspect actual old writer bytes **before** adding the golden-fixture test. For CP1: test RED, then minimal admission implementation, then GREEN.
4. Existing v21 P21-01–P21-10, Part/Sketch / OCCT / PlaneGCS tests and exact-head CI gates remain. No A2/A3, product UI, PM-06, RG-02, global solver, native schema successor beyond v21, vendor distribution/import, auto merge, or protected-main mutation. Documentation impact: **internal runtime provenance and v20 migration evidence require DOC-01 review**, and any exact docs paths need their own Owner scope before source completion. Generated Browser outputs are not manually edited.
5. No claims of technical PASS until an actual CI run on the exact product SHA and original native solver paths are checked. Governance-only CLOSURE is not product FULL or Owner product PASS.

## 5. Exact Owner decisions requested

**D2-CP1:** YES / NO — Approve ephemeral reference-counted Part runtime incarnation as an additional fail-closed `PartEvaluation` authority witness, including only the exact CP1 file list and lifecycle/tests stated above.

**D2-V20:** YES / NO — Approve the exact new historical binary fixture path `tests/fixtures/r13a1_native_part_v20_writer_golden.ss2part`, existing `tests/sk02b_part_sketch_persistence_test.cpp` and the independent real-writer provenance/migration test workflow stated above.

**Separate later decisions (NOT implied):** contract incorporation/activation, any extra file paths, DOC-01 exact files, Owner hands-on product acceptance, LGPL release distribution compliance and guarded product merge.
