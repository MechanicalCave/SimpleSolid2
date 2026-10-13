# PG-01E — Project Geometry product-wide completion: bounded Work Contract (OWNER REVIEW DRAFT)

**Status:** **DRAFT / NO TEST, PRODUCT, DOCS-AS-BUILT MUTATION OR ACTIVATION AUTHORITY**.  
**Date:** 2026-10-13  
**Current main:** `5b495c4bbed8754d7a1f01e87a30716429db786d`, accepted Part roadmap **v1.32**; `work/ACTIVE.yaml` still points to **completed RG-02A/G0 evidence**. PG-01E is the next work stage **for independent governance review**, but not yet active.  
**D2 direction:** Owner explicitly accepted E1–E6 for preparing this *exact bounded contract* on 2026-10-13 ("akceptuję - pozamykaj wszystkie formalności i zacznij prace !") in reply to an explicit scope-only question, now recorded in [D2 PR #311](https://github.com/MechanicalCave/SimpleSolid2/pull/311). This **does not itself accept this Work Contract**, approve merge of #311 or this draft PR, authorize a new `ACTIVE.yaml` merge, fix any product defect or grant PG-01E implementation privileges.  
**Required sources:** Constitution/Foundation v1.0, accepted Part roadmap v1.32, PG-01A–D as-built contracts and completion evidence, ADR-0014/0016/0017, existing current-stage Part source semantics and strict ModelingSemanticsVersion 2 policy. This contract freezes **no new feature or API**.

## 1. Objective and acceptance envelope

Finish **existing** Project Geometry across Parts (Edges, Planar Face, Face Boundary, linked Regular/Construction and explicit Break Link) by compiling rigorous requirement-to-proof and actual absence-of-proof inventory; recording reproducible *current* native positive/negative cross-feature lifecycle evidence; retaining the frozen **native Part schema v16 versus ModelingSemanticsVersion=2** rejection policy; measuring performance/resource trends; and checking PL/EN, internal, Browser and one practical Windows acceptance guide.

**An evidence/documentation-only finish is a valid result if no actual blocking defect is found.** When an actual failing scenario requires product, API, schema, UI, migration or other out-of-contract mutation, **STOP** with precise reproduction and a separate Owner D2/Work Contract amendment. Do not silently repair or expand.

**Preserve** current material Edge source identity, after-Feature stage, native provider freshness, exact supported Line/Circle/Arc only; certified Face Boundary exclusion of representation seams and geometric Unsupported/Degenerate per individual Edge, versus integrity failure that blocks the entire operation; no nearest XYZ, stale seed or provider token as lasting identity. Face pick/wire membership is a **transient acquisition gesture, not persisted dependency**. Cut/Fillet/Chamfer Refine policy is unchanged. No new Product Geometry mode or universal topological matching.

## 2. Strict future file-mutation allowlist (requires later activation)

**After** this exact contract is separately Owner-accepted and merged onto main, and a **new dedicated `work/ACTIVE.yaml` PG-01E activation PR** is separately accepted and merged, PG-01E test/evidence changes may mutate **only existing registered test sources**:

1. `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` — real OCCT exact edge projection, geometric Unsupported/Degenerate/provider-scope cases.
2. `tests/kernel_native/pm05e3_edge_feature_cold_rebuild_test.cpp` — real OCCT PG-01B linked Sketch/Profile/Feature, Break Link, dimension edit, suppress/source-loss and cold Save/Reopen with new provider.
3. `tests/pm05f_r2_native_workbench_edit_test.cpp` — existing Windows Qt/OCCT end-to-end Project Geometry Edges/Planar Face/Face Boundary, mixed selections, Finish, diagnostics, Undo/Redo, user-mode and resource observations.
4. `tests/part_document_store_test.cpp` — strict native v16 ModelingSemanticsVersion=2 persistence and typed modeling-v1/future rejection.
5. `tests/pm02p_e_core_lifecycle_test.cpp` — nonnative session/revision/provider-generation lifecycle regression (must never be misdescribed as a real Viewport hit).
6. `tests/pm04e2_revolve_lifecycle_cold_rebuild_test.cpp` — earlier Revolve source, dependent cross-stage semantics and cold authored identity, when test-accessible existing APIs suffice.

**Conditionally permitted canonical documentation paths, only for verified defects after activation:** `docs/internal/PART_DOCUMENTS.md`, `docs/internal/CAD_WORKBENCH_VIEWER.md`, `docs/product/pl/PARTS.md`, `docs/product/en/PARTS.md` and deterministic generated `docs/browser/index.html` **only through existing browser generator, exactly paired with a canonical documentation change**. A discovered false internal claim must be corrected with an evidence citation, not revised speculatively. Work evidence/reporting: `work/PROJECTION_01E_E0_CAPABILITY_GAP_INVENTORY.md`, `work/PROJECTION_01E_ACCEPTANCE_EVIDENCE.md`, `work/PROJECTION_01E_OWNER_WINDOWS_MANUAL_CHECKLIST.md` and explicit appendices to this future merged Work Contract (each other path requires a separate scope amendment).

**Strictly forbidden without an independent bounded Owner amendment:** all `src/**` production code, Kernel/Part/App/Viewer public APIs, schemas/serialization migration, `tests/**` outside six named sources, new tests/targets or `CMakeLists.txt`, workflows/CI/bench scripts, generated binaries, project files/fixtures not listed, `adr/**`, `governance/**`, Foundation, unrelated PM-06/PG-01E+ features, RG-02/G1 Vertex source/Datum Plane/Axis/MID/CENTER work, all `work/ACTIVE.yaml` mutation in test/evidence PR, and generic performance-budget changes. **Owner approval of D2-E1–E6 does not override this STOP.**

## 3. Owner-reviewable E0–E6 evidence matrix (not executed now)

| Gate | Real input and test/record authority | Mandatory outcome and fail-closed negative | State |
| --- | --- | --- | --- |
| E0 | Exact current main and merged PG-01A–D PRs/Owner final evidence, schema+source inventory, canonical PL/EN/Internal/Browser | Requirement → code/test → exact SHA+CI → Owner manual claim where available; distinguish prior PASS from PG-01E UNRUN and actual docs drift; record missing witness as OPEN | **PRELIMINARY READ-ONLY AUDIT BEGUN; NO NEW PG-01E TEST RUN** |
| E1-A | Newly authored modeling-v2 Part, actual earlier-stage Add/Cut and linked Sketch projected material Edge -> Regular Profile -> later Feature | Correct resolved current source, same EntityId/Profile/Feature IDs; no stale seed; same source stage; real separate provider after Save/Reopen | **UNRUN (PG-01E)** |
| E1-B | Native Revolve Add/Cut, Fillet/Chamfer and Face/Datum-supported Sketch with Edges+Planar Face+Face Boundary mixed source acquisition | Exact source/wire/Face and material-edge dedup only; supported/skipped counts; transitions/holes; identical manual+Face source added once; no automatic Face-link persistence | **UNRUN (PG-01E)** |
| E1-C | Edit source dimension, delete/suppress, link unresolved, explicit repair/Break Link, Undo/Redo, Delete and cold rebuild | Meaning preserved or typed Missing/Ambiguous/Unsupported; bad stage/revision/provider/wire/material identity blocks atomic Finish; do not bind first/nearest or cached seed; no partial authoring | **UNRUN (PG-01E)** |
| E2 | Native container v16 with modeling v2 and original immutable v16/modeling-v1 fixtures; older schemas and future versions where supported reader policy permits | v2 cold Save/Reopen OK; v1 typed `unsupported_modeling_semantics`; originals unchanged; no automatic migration, converter, resave-on-open or invented legacy compatibility | **UNRUN (PG-01E)** |
| E3 | Existing Windows native workbench test/resource-measurement facilities, repeated exact saved edit/reopen cycles | Machine/build/fixture/number of cycles/warmup and elapsed/resource methodology; compare repeated groups, report unavailable metrics explicitly; no PM-06 budget acceptance | **UNRUN (PG-01E)** |
| E4 | Canonical PL/EN Product, Internal docs and deterministic Browser, validated current as-built UI | Actual equality/false-claim audit, edit only verified mismatches in allowlisted docs; one reproducible Windows supported procedure and explicit unsupported/partial caveats | **E0 FOUND POTENTIAL DOC DRIFT; formal review unrun** |
| E5 | Existing six targets, Windows FOCUSED trailers for changed registered targets; relevant FAST and final exact-head Windows FULL including Core/native/Desktop and docs | Each positive case actual test PASS, negatives fail closed; no hidden CTest stdout invented; immutable source SHA-run table; CI failures investigated not waived; Owner practical Windows review separate | **UNRUN (PG-01E)** |
| E6 | Final integrated requirements matrix and documented exclusions | Owner FINAL PASS **explicitly granted** only after exact-head FULL, no unresolved P0; PR merge requires *another* express authorization | **PENDING** |

**Precise test/run crosswalk required:** do not re-label an earlier package's PASS as a newly executed PG-01E test. Source/test runners are already registered and may not be reconfigured under this contract. If an E1–E3 proof cannot be expressed without a forbidden extra path/API, stop and request a scoped amendment **before** implementing it. A negative fixture or blocked test is not silently accepted.

## 4. Existing-evidence-only E0 may be prepared before activation (no code mutation)

Allowed *pre-activation preparation* is inspection of existing `main` source/tests, reading accepted PG-01A–D records and documentation, and a **clearly labeled preliminary draft E0 inventory on an independent documentation-only PR**, without editing any accepted as-built docs, runtime tests or code. Such a draft is neither a final E0 PASS nor an activated PG-01E work product. Its final report path becomes authoritative only under a subsequently merged Work Contract and distinct activation.

Initial **verified audit lead:** `docs/internal/PART_DOCUMENTS.md` still states that user-facing Project Geometry and Face boundary acquisition are not present in sections describing earlier PG-01B semantics, while current `docs/product/pl/PARTS.md` and `docs/product/en/PARTS.md` describe the completed Edges / Planar Face / Face Boundary modes and [PG-01D merged acceptance](https://github.com/MechanicalCave/SimpleSolid2/blob/main/work/PROJECTION_01D_FINAL_ACCEPTANCE_AND_MERGE.md) confirms delivery. **Do not call this a product failure, assert all Internal docs stale, or edit as-built docs before activation.** E0 must log the exact line and scope of the contradiction, preserving historical context where relevant.

## 5. Governance, exact independent approvals and hard STOP

1. **DONE**: Owner accepted D2-E1–E6 *design direction* (2026-10-13). This does **not** accept this contract text.
2. **PENDING**: Owner explicitly review/accept **this exact bounded Work Contract** with the six source-test files, five documentation paths and E0–E6 proof matrix; any changes after acceptance require clear re-review.
3. **PENDING**: Owner **separate explicit merge permission for PR #311** (D2 decision record). Exact-head docs/closure checks must pass.
4. **PENDING**: Owner **separate explicit merge permission for this Work Contract PR**, after it is accepted and CI green. Contract acceptance is not merge authority.
5. **PENDING**: only after those merges (and accepted v1.32 roadmap stays on main), create a **dedicated `work/ACTIVE.yaml` governance PR** switching `active_work` from G0 to `work/PROJECTION_01E_PRODUCT_CLOSEOUT_WORK_CONTRACT.md`. Preserve `program_roadmap_version: "1.32"`. Get **separate Owner explicit approval to merge activation PR** after exact-head checks.
6. **ONLY AFTER activation merged**: start a fresh bounded PG-01E test/evidence PR, run FOCUSED, FAST and exact-head FULL. In case of production-code defect, halt, issue explicit failing witness and request separate remediation scope/Owner approval. Complete final Owner Windows manual acceptance and independent evidence PR merge authorization.

**No RG-02 UI code at any point without independent Owner UX consultation and prior explicit approval.** PG-01E does not bypass that standing STOP or activate deferred RG-02A/G1.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This contract-only governance change does not alter current product behavior or accepted as-built documentation; later activated PG-01E evidence work may correct only verified documentation defects within the existing conditional allowlist.
Browser: not required (no source Markdown changes now)
Later test/evidence work may correct **only proven** canonical docs inaccuracies within the accepted allowlist, with Browser regeneration when required.
