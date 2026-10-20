# Sketcher R13-A1 — Local relations and native Sketch evaluation — D2 Work Contract PROPOSAL

**Status:** DRAFT FOR EXACT OWNER D2 ACCEPTANCE; **NOT ACTIVE**, NOT a product authorization. Prepared 2026-10-19 after Owner approved a functional-first SS1-donor adaptation and explicitly requested implementation progress. This records that direction without falsely asserting independent exact-path/ACTIVE authorization. No source/vendor/schema/UI change is authorized by this draft alone.

**Baseline:** SS2 protected `main@c0aa91f5abdbc2ba9a997bc53b99cebe873d335f`; ACTIVE currently `work/SKETCH_R13_G0_FEASIBILITY_D2_WORK_CONTRACT_PROPOSAL.md`. R13 G0 ledger in Draft PR #498; Windows existing-product FULL #3710 on product baseline `21205a3b879bde80650eef2ef857a858eaa50f21` (Core 25/25, Kernel 57/57, Qt/Desktop 117/117). The performance D2 relief in merged #499 is **DEFERRED-BY-OWNER-D2, not PASS**. Accepted Sketch v1.11 and Part Modeling v1.35 roadmaps schedule R13-A1 next; their old headers can contain superseded merge statuses.

## 0. Purpose, authority, activation

Implement the five Owner-approved **explicit manual** local geometric relations by adapting proven SS1 ideas, not inventing an unrelated solver: **Coincident(Line Start/End, Line Start/End), Horizontal(Line), Vertical(Line), Parallel(Line, Line), Perpendicular(Line, Line)**. Auto-Constraint OFF. Source `MechanicalCave/SimpleSolid@b6c010b1af4a03ab130b09f25b6e61b1d9f421ec` is an Owner-owned, tested functional prototype: its historical Windows M17 solver + parametric Sketch→Part lifecycle counts as **donor evidence**, not a new SS2 run. Prefer translating donor acceptance fixtures and solver-neutral contracts over re-proving donor algorithms. SS2 Foundation 1.0 / ADR-0008, 0009, 0012, Part/Projection ADRs and active governance own integration semantics.

**Activation is separate from this proposal:** first independent technical/Owner disposition of remaining G0 evidence and separate permission to merge #498 if needed; Owner exact acceptance of THIS D2 contract and exact conditional path allowlist; one governance-only contract PR passing its own exact-head Windows CLOSURE and separate Owner guarded-squash authorization; then a **distinct one-file `work/ACTIVE.yaml` PR**, separately CLOSURE-verified, explicitly Owner-authorized, merged and read back from protected main. Product RED/production changes start only after this switch. No implicit approval of contract text, solver distribution, merge or ACTIVE from a general “kontynuuj”. PM-06/RG-02 stay deferred, not cancelled.

## 1. Delivery strategy — small, real vertical slices

**A1.1 semantic RED → GREEN:** first `Horizontal(Line)` with Sketch-local typed monotonic `ConstraintId`, semantic references, authored-only persistent intent, duplicate/missing/stale rejection, derived evaluated Sketch snapshot separate from authored seeds; a real `DocumentSession` revision-bound Create/Remove Command with atomic Part transaction and Undo/Redo. This first green must use the actual SS2 native build, not the external smoke surrogate. The prior separate `local_constraints.hpp` spike is design input only (its ad hoc horizontal evaluator and vertical-seed rejection are **not accepted solver semantics**).

**A1.2 donor numeric solver adoption:** bring the remaining four relations through the **SS2-owned solver-neutral adapter**, translating the donor PlaneGCS acceptance policy and diagnostics; verify only material integration risks in actual Windows VS2022 x64 (compile/link, fixed/projected entities, conflict/redundancy, finite residual/DoF, installed consumer). Do not write a second general solver or add a global dependency graph. Do not install new vendor code until a pinned-rights/relink/distribution plan is explicitly Owner accepted. If that dependency decision blocks A1.2, record the STOP without pretending A1.1 is the five-relation implementation.

**A1.3 Part and file lifecycle:** new future strict native Part store version (**candidate v21, not effective before exact approval**), v20 read migration, cold Save/Close/Reopen, same-revision Part stage/projection → constrained evaluated Sketch → Profile/Extrude/Revolve; local Delete cascade, repair-required downstream Features preserved, Undo/Redo exact identities; native 3D Sketch/Part tests. No persisted solved state.

An A1.1 milestone is not A1 FINAL. Finish all five relations plus persistence/Part integration and Windows acceptance before A1 technical FINAL or product merge. Owner UI review is required **before** implementation of any A1 relation controls/Qt UI; A2 dimensions and A3 Reference/Driving are separately contracted.

## 2. Nonnegotiable minimal semantics

1. **Ownership:** Shared 2D owns authored local `ConstraintId`, kind, semantic `EntityId + Start/End/Line` reference, optional user intent; Part retains Sketch support/PG-01 projection/Profile/Feature/native file ownership; DocumentSession owns transaction, revisions, history and dirty state. Constraint-only changes participate in equality/state Save/Undo.
2. **Geometry:** authored Line/Circle/Arc coordinates remain seeds; solve runs on disposable revision-and-stage-bound geometry. Projected PG-01 targets are fixed source references; do not mutate them or break links through ordinary Delete. Reject stale stage/revision, singular or nonfinite results; no general tolerance-welding. A numerically converged Coincident must feed exactly matching evaluated endpoints to the Profile resolver, or explicitly fail.
3. **Identity/lifecycle:** create unique Sketch-local IDs without burning cursors on failed commands; restored Undo resurrects exact semantic IDs; Redo deterministic; removing user geometry cascades only dependent Sketch local relations with impact confirmation when applicable. Do not delete dependent Part Profile/Feature records; set derived repair-required when evaluation invalidates.
4. **Evaluation:** explicit user relations only; Snaps, inference, dynamic input and Grip never persist constraints. Underconstrained is legal, contradictory/redundant/fixed-incompatible/ambiguous constraints reject atomically with structured diagnostics. Respect caller-owned length/angle units; accept no nonfinite solution or global arbitrary epsilon.
5. **Release license:** SS1 donor ownership removes needless donor-prototype feasibility repetition; third-party PlaneGCS LGPL is a **distinct** dependency. Actual archive checksum and required source/notice/replace-or-relink pathway must be proven before vendoring/distribution as appropriate. External verification is permitted; no runtime network fetch as part of SS2 builds.

## 3. Finite candidate path allowlist — applies ONLY AFTER contract acceptance + ACTIVE

All paths below are **conditional**, for the purposes stated, with no generic directory wildcard authority. Owner acceptance freezes a finite list; adding further paths requires explicit D2 addendum. Use the smallest subset required by each slice.

**Sketch model + neutral solver:**
- `src/sketch/include/simplesolid2/sketch/sketch_model.hpp`
- `src/sketch/sketch_model.cpp`
- `src/sketch/include/simplesolid2/sketch/local_constraints.hpp` (new; exact name)
- `src/sketch/local_constraints.cpp` (new, only if implementation requires)
- `src/sketch/include/simplesolid2/sketch/local_solver.hpp` (new neutral adapter contract, only in A1.2)
- `src/sketch/local_solver.cpp` (new neutral adaptation, only in A1.2)

**Part/store/stage evaluation:**
- `src/part/part_document_store.cpp`
- `src/part/part_document.cpp`
- `src/part/feature_evaluation.cpp`
- `src/part/profile_kernel_input.cpp`
- `src/part/include/simplesolid2/part/effective_sketch_projection.hpp`
- `src/part/include/simplesolid2/part/part_sketch.hpp`

**Commands and existing Sketch-facing integration:**
- `src/application/document_session.cpp`
- `src/application/include/simplesolid2/application/document_session.hpp`
- `src/ui/part_sketch_interaction_controller.cpp` **only AFTER Owner real SS2 UI walkthrough**
- `src/ui/part_viewport_controller.cpp` **only AFTER Owner real SS2 UI walkthrough**

**Registered test candidates:**
- `tests/sk02b_part_sketch_model_test.cpp`
- `tests/sk02b_part_sketch_persistence_test.cpp`
- `tests/r12_structural_edit_document_session_test.cpp`
- `tests/pm02f_stage_aware_profile_evaluation_test.cpp`
- `tests/sk03a_part_viewport_controller_test.cpp` **UI-gated**
- `tests/sk04c_part_sketch_interaction_controller_test.cpp` **UI-gated**
- `tests/r13_a1_local_constraints_test.cpp` (new native registered test)
- `src/CMakeLists.txt` (register new SS2-owned Sketch source only)
- `tests/core_only/CMakeLists.txt` (register the first real RED semantic test in Core)
- `tests/CMakeLists.txt` (desktop integration test registration only when applicable)
- `tests/kernel_native/CMakeLists.txt` (native Part/OCCT integration test registration only when applicable)

**As-built documentation and DOC-01 (exact existing file paths to be certified before activation):** omitted from this initial executable allowlist intentionally; if internal ownership/native schema or product UI changes require DOC-01 updates, accept a finite D2 addendum listing the confirmed existing Markdown paths and generated Browser before production completion. The absence is **not** permission to skip mandatory docs.

**Not included / STOP:** any other `src/**`, other `tests/**`, `third_party/**`, `cmake/**`, `.github/**`, CMake vendoring/FetchContent, general `docs/**`, `work/ACTIVE.yaml`, runtime autoconstraint UI, dimensions A2/A3, unrelated PM/RG, new public Kernel API, arbitrary solution-cache persistence. If dependency build needs an additional path, explicitly request exact D2 allowlist extension; do not silently add it.

## 4. Compact acceptance — no benchmark inflation

- **RED before GREEN** on actual registered C++20 SS2 tests; Horizontal semantic identity, command revision, dirty/equality, Undo/Redo; five relations and fixed/projection/conflict rejection; v20 backward read + candidate successor strict round trip/corrupt record rejection; cold Part Profile/Feature repair and exact evaluated vertex closure. Tests stay small, representative and source-tied.
- Real **Windows FOCUSED / FAST / Ready FULL** (Core, Kernel-native, Qt/Desktop when relevant) at recorded exact PR product HEAD, plus relevant DOC-01 verify/browser and existing released regressions. Never call donor M17 a new SS2 PASS.
- Manual Owner inspection of user-visible 3D constraint controls after Owner UI walkthrough; native Save/Reopen and representative Sketch→Profile/Extrude. Product technical FINAL and explicit guarded squash decision remain distinct.
- **Performance:** functional-first; no p50/p95 benchmark or 13-feature stress test as a prerequisite. Still fail-closed on nontermination, numerical corruption, excessive obvious hangs and stale publication. Future profiling can be contracted after functional implementation.

## 5. Decision points to freeze before first SS2 product edit

The precise D2 acceptance of this draft approves: the five relation kinds, typed Sketch-local identity, separate authored/evaluated state, one SS2 DocumentSession history, stage-aware Part semantics, **candidate** v21 native format only when the exact schema disposition is accepted in A1 execution, and conditional paths above. The initial native RED/Horizontal slice can proceed **after formal ACTIVE**, without importing the PlaneGCS binary; approval of G0 feasibility evidence does not constitute distribution sign-off.

The independent Owner must still decide: (a) PlaneGCS 0.8.0 DLL/STATIC licensed packaging or approved alternative before third-party import; (b) actual schema successor version/field ownership before writing it; (c) A1 UI layout after the requested joint UX review. These are finite, precisely deferred decisions, not permission for uncontrolled architecture changes.

## 5A. Owner-accepted Fix(Line) and DEV B1a amendment — precise two-item D2

**Approval and activation distinction (2026-10-20):** following Owner review of the specific two-part D2-A / D2-B1a proposal in [governance PR #507](https://github.com/MechanicalCave/SimpleSolid2/pull/507), and the external Windows MSVC real PlaneGCS→SS2 bridge proof `SS2_B1A_REAL_GATEWAY_PASS=32` at exact product Draft [PR #506](https://github.com/MechanicalCave/SimpleSolid2/pull/506) head `d7e5491723e983b2b1505214a15fdde6e80abed2`, Owner explicitly answered **“akceptuje - kontynuuj pracę”** to the individually named D2-A and finite D2-B1a decisions. These **two design/scope decisions are Owner accepted**, but this appendix becomes a **normative amendment only after this separate one-file governance PR passes its exact-head Windows CLOSURE and is merged to protected main under separately granted guarded-squash authorization**. The existing `work/ACTIVE.yaml` already points at this contract: no second workspace/checkout and no speculative ACTIVE pointer transition. Historical pre-acceptance descriptions elsewhere in this document remain as chronological baseline; this appendix controls only the two narrow accepted additions after activation.

**D2-A — sixth authored manual `Fix(Line)`:**

1. Append `ConstraintKind::fix` after the original five ordinal kinds, preserving existing serialized identity, typed Sketch-local monotonic `ConstraintId` and canonical Line identity. Both authored endpoints of the Line are grounded at their original Sketch-local coordinates. One fixed **parameter mask**, **not** a sixth numeric constraint equation.
2. `Fix` joins real PG-01 projected fixed Line inputs as immutable solver parameters, but PG-01 projections are **not** automatically authored Fix relations. Validate `Fix` references, duplicate/contradictory intent and fixed incompatibilities atomically before solving. Preserve IDs/cursors/Undo on rejection.
3. Updating, Trim, Extend or ExtendBoth on a fixed Line must reject changes; using it as a geometrical reference is allowed. Erasing `Fix` ungrounds that Line only. Deleting the Line cascades its owned Sketch relations but **never silently deletes Part Profile/Feature records**. Part and `DocumentSession` revisions, atomic Undo/Redo, stale-stage guards, authored/evaluated distinction and exact Coincident closure remain mandatory.
4. No persistence schema change, Qt controls or A2/A3 dimensions are approved by D2-A. Strict native store v20 must continue fail-closed on unsupported constrained Sketch until an independently accepted next schema and cold round-trip tests.

**D2-B1a — developer-only actual PlaneGCS backend, conditional exact file extension:**

Use existing neutral `sketch::ILocalSketchSolver` and `evaluateLocalSolve` from PR #506, with original pinned upstream PlaneGCS 0.8.0 five native C++ translation units from externally provisioned source at SHA-256 `09e77e6e3a27fa77d3b5a69065ea4c82ae6ed35753246a2e518b1a6005fea162`. Add no vendor copies, second CMake project in SS2, auto-fetch, Python/pybind11 runtime, installed application or DLL deployment. The **only additional/existing file edit locations and purposes** permitted after this amendment is operative and the below rights gate is satisfied are:

| Exact repo-relative file path | Narrow approved B1a purpose |
| --- | --- |
| `src/sketch/include/simplesolid2/sketch/planegcs_dev_solver.hpp` | **NEW**, one DEV-only SS2 `ILocalSketchSolver` subclass declaration; no new public Kernel/Part API |
| `src/sketch/planegcs_dev_solver.cpp` | **NEW**, real PlaneGCS 0.8.0 native equation/fixed-point bridge; translate semantic `EntityId`/`ConstraintId`, diagnostics and disposable candidate, never publish authored model changes |
| `src/CMakeLists.txt` | Extend its existing A1 permission **only** for OFF-by-default `SS2_R13_PLANEGCS_DEV` opt-in native source/core adapter build with explicit `SS2_PLANEGCS_080_SOURCE_DIR`; verify exact local prerequisites and fail closed when missing; no network |
| `tests/r13_a1_local_constraints_test.cpp` | Extend existing actual native SS2 test with RED→GREEN real backend fixtures, independently asserting fixed/source preservation, semantic residuals, Coincident exact closure, conflicts/redundancy/DoF, and upstream raw Success false-positive refusal |
| `tests/core_only/CMakeLists.txt` | Only opt-in test graph registration/link; no default test count/name change and no duplicate test for identical assertions |
| `tests/kernel_native/CMakeLists.txt` | **DEFERRED B1a2 AFTER CORE PASS**, existing Part-stage + true OCCT Profile/Extrude/Revolve route through real adapter, respecting revision/stale publication and existing registered kernel target |

**Operational isolation:**

- Existing single working checkout `D:\SimpleSolid2`; **no** `git worktree add`, second clone or forced reconfiguration. Existing `D:\SimpleSolid2\build\dev` remains production developer CMake cache. `D:\SS2Build` is CI-owned and must not be borrowed or cleaned.
- Original PlaneGCS archive and already installed `D:\SimpleSolid2\.simplesolid-env` Eigen/Boost may be reused; disposable DEV build/sources/output restricted to existing Git-ignored `D:\SimpleSolid2\build\b1a`. No heavy dependency installation on C:. Always verify current working-tree cleanliness and restore exact original detached HEAD after external test. This operational evidence is not authority to mutate protected main.
- The Owner-executed original external MSVC 19.38 harness yielded 32/32 named PASS assertions for five geometric kinds, Fix mask, fixed projected references, exact Coincident and unchanged authored Sketch. This is **external DEV native API feasibility**, not a registered SS2 CTest/Part/OCCT/UI/schema/release PASS and not an imported product solver.
- **Mandatory independent source/IP gate before tracked adapter/CMake integration:** examine upstream LGPL-2.1-or-later core rights, actual absent full license text from original 0.8.0 sdist, FreeCAD copyright/provenance and non-SPDX `src/wrapper.h` / shim files that the bridge necessarily includes. Archive-byte/hash inventory B0-A PASS does **not** settle B0-B rights. Record reviewer and narrow legal-compliance disposition before tracking any vendor-aware code. Only then run real registered native MSVC RED→GREEN with caller-owned tolerances, fixed guards, residual rejection and exact semantic IDs.
- Existing five original A1 source, Part/DocumentSession and tests remain conditional only for their already-accepted purposes, with no broader dependency path privilege. **STOP outside these six additional/existing paths:** `third_party/**`, `cmake/**`, root `CMakeLists.txt`, `.github/**`, `docs/**`, `work/ACTIVE.yaml`, Qt UI, schema, Python packaging, installer, consumer DLL replacement, automatic online dependency fetch or new release obligations. Any additional file needs its own exact Owner D2.

**Next explicit gates:** once this amendment has passed Windows governance CLOSURE and separately authorized merge/readback, pass narrow B0-B developer source-rights review, then register product MSVC Core test/adapter RED→GREEN; B1a2 Kernel/OCCT only after Core PASS, Owner Sketch manual UI walkthrough only after its separate exact UI path acceptance, native cold Save/Reopen only after its separate schema contract. A2 Driving dimensions and A3 Reference/Driving remain separate bounded phases. A native DEV pass never proves installer/LGPL distribution compliance and never permits merging Draft PR #506 without independent Owner product technical acceptance and guarded merge authorization.

## 5B. Owner-accepted D2-C — source-backed original PlaneGCS Windows PR CI (2026-10-20)

**Approval:** Owner explicitly answered “akceptuję - kontynuuj pracę” to the specific single-path D2-C proposal presented in the conversation and in [governance PR #507](https://github.com/MechanicalCave/SimpleSolid2/pull/507). This accepts only the conditional edit permission below. It is **not** a protected-main merge authorization, **not** an independent third-party rights/legal signoff, **not** approval to copy original source/vendor or alter any product UI/schema, and **not** evidence that original PlaneGCS is already running inside PR CI. This addition becomes operative **only after** this one-file contract PR passes its exact-head Windows CLOSURE and receives a **separate Owner guarded-squash merge decision**.

**One and only one newly permitted file:**

| Exact repo-relative path | Conditional permitted purpose |
| --- | --- |
| `.github/workflows/windows-pr-gate.yml` | Add a named, source-backed Windows/MSVC **original PlaneGCS 0.8.0** native Core CTest job, and gate relevant PRs on its non-skipped result, but **only after** B0-B independent reviewer approval and green actual D2-B1a DEV adapter + native Core test |

No changes to other `.github/**`, `scripts/**`, `cmake/**`, `third_party/**`, root `CMakeLists.txt`, `work/ACTIVE.yaml`, Qt, strict persistence schema, installer or release. Previously frozen D2-B1a six source/test paths retain their own exact purposes and dependencies.

**Future implementation contract (when all prerequisites are satisfied):**

1. Preserve current self-hosted Windows labels, exact PR-head checkout and existing FAST/FOCUSED/FULL/CLOSURE classifier. Preserve **all** `D:\SS2Build` persistent Core/desktop/kernel caches and the owner's `D:\SimpleSolid2\build\dev` and `build\b1a` independently. Build in one disposable ignored subdirectory of the CI runner's *existing single checkout* on D:. No second clone/worktree, C: heavyweight installation, cleanup of other build trees, network fetch or CMake FetchContent.
2. Verify the **runner-specific**, already-provisioned original 0.8.0 source TAR SHA-256 `09e77e6e3a27fa77d3b5a69065ea4c82ae6ed35753246a2e518b1a6005fea162`, original upstream pin `5a60d5328bedd85f1e4aef81b9b57f5c4410ae52`, five native C++ translation units, original wrapper/shims, Eigen3 and Boost headers. The human owner's D:-drive inventory is **not** proof of CI runner provisioning; obtain read-only inventory before wiring paths. Wrong/missing source, include path or commit in explicitly enabled native mode must **FAIL** rather than SKIP, mock, or download.
3. Opt into `SS2_R13_PLANEGCS_DEV=ON` plus explicit `SS2_PLANEGCS_080_SOURCE_DIR`; compile/link actual original PlaneGCS and SS2-owned `ILocalSketchSolver` adapter with `simplesolid2_sketch`. Give the real native CTest a **distinct name** from scripted `r13a1.local_horizontal` and scripted-backend `r13a1.occt_profile_closure`.
4. Real numerical fixtures must cover six authored kinds (five geometric, `Fix(Line)` as fixed mask), seven-equation fixed-base rectangle, immutable PG-01 source geometry, exact Coincident closure, semantic IDs and diagnostics, DoF/conflict/redundancy classification, independent residual acceptance, and original upstream false-Success/degenerate-candidate rejection. Only **after native Core PASS** proceed to D2-B1a2 OCCT/Profile/Extrude/Revolve native caller evidence.
5. Add the extra job to the required aggregate `windows-msvc` only for PRs where it is in scope; if the job is expected, absence/skipping/false-green must fail the gate. Ordinary unrelated PRs must continue through current modes. Never label scripted backend PASS, Windows FAST or governance CLOSURE as original PlaneGCS native PASS. Record exact PR SHA, source SHA, runner D: paths, actual test log and reviewer disposition.

**STOP until prerequisites:** (a) independent named B0-B rights/provenance reviewer records a written narrow PASS for pinned exact originals, including FreeCAD lineage, `wrapper.h`, shims, full LGPL 2.1 licensing terms and existing Eigen/Boost dependencies; (b) actual tracked DEV adapter + Core native CTest is green under effective D2-B1a; (c) CI D:-resident original source/dependency paths are confirmed. Only then implement the single workflow edit. Release LGPL replaceability/distribution, v21 file format and UI remain independent decisions. **Original PlaneGCS PR-native gate today: NOT RUN.**

## 5C. Owner-accepted D2-P21 — native Part v21 relation persistence (2026-10-20; independent governance incorporation PENDING)

**Owner decision (exact scope):** After reviewing [the dedicated one-file v21 schema proposal in Draft PR #511](https://github.com/MechanicalCave/SimpleSolid2/pull/511), at exact proposal HEAD `4ae4a712cfe7080f40b353fa45d8e36b2488875a`, the Owner expressly answered **“akceptuję - kontynuuj pracę”**. The [PR #511 Owner decision record](https://github.com/MechanicalCave/SimpleSolid2/pull/511#issuecomment-6295923290) accepts **all three exact D2-P21-1 / D2-P21-2 / D2-P21-3 decisions**, without approving any merge or production mutation. The accepted proposal document is `work/SKETCH_R13_A1_V21_NATIVE_PART_SCHEMA_D2_ADDENDUM_PROPOSAL.md` (single-file governance PR #511, content blob `95e620537e4b9cb25c0c2d675a070bbdc99b8723`). The following is an **incorporation proposal**; it becomes normative only after both document and contract amendment are **independently guarded-squash merged and verified** into protected main, with their own exact-head CI CLOSURE and separate explicit Owner merger permissions, in that order. This Draft section is **not** itself an implementation authorization.

**D2-P21-1 — exact v21 file-contract choice, accepted:** Native Part `.ss2part` schema successor is **21**, not a vague candidate. Keep all v20 outer document/Sketch/Body/Profile/Datum/Axis/PG-01 projection and native container/checkpoint formats unchanged. Within each `sketches[].model`, v21 requires exactly four keys: `next_entity_id`, `entities`, `next_constraint_id`, `constraints`. Old v20 remains *exactly* the two-key model on read. Preserve authored Line/Circle/Arc seed geometry, Sketch-local monotonic `ConstraintIdCursor` (also after deletion, with empty arrays and gaps), ordered local constraints and six exact kinds `horizontal`, `vertical`, `coincident`, `parallel`, `perpendicular`, `fix`. Each relation has exact `id`, `kind`, `first`, `second`: `first` and binary `second` use only `{"entity_id":"<local Line EntityId>","element":"line|line_start|line_end"}`; all unary kinds require literal `"second":null`. Binary line/end reference shape must match its kind. All IDs are canonical nonzero decimal strings strictly below their saved per-Sketch cursors; wrong/missing/extra fields, broken/wrong-kind/cross-Sketch references, duplicates (including swapped binary pair), unsupported kinds and contradictions that fail core authored-state invariants **reject the entire file**, not drop a relation. No saved solved coordinates, DoF, residuals, backend object IDs, BRep, numeric tolerance, annotation or dimensions. A structurally valid but numerically unsatisfied system remains loadable; only its derived Profile/Feature is blocked/repair-required with no last-good BRep reported current. No solver is required merely to load the document. New writer v21 only after the complete schema is ready; previously supported versions, especially v20, retain strict version-scoped parsing and explicit-Save migration.

**D2-P21-2 — one exact additional conditional source path, accepted:**

| Additional exact repo-relative path | Strict permitted purpose **only after effective D2-P21 activation** |
| --- | --- |
| `src/part/include/simplesolid2/part/part_document_store.hpp` | Update `PartDocumentStore::current_schema_version` **20 → 21** and only strictly necessary store-local declarations reviewed in the implementation diff |

Every already allowlisted A1 path retains its existing conditional permission and purpose. In particular `src/part/part_document_store.cpp` handles the strict version-specific encoder/decoder and rejection guard; `tests/sk02b_part_sketch_persistence_test.cpp`, `tests/r13_a1_local_constraints_test.cpp` (and other specifically approved existing A1 test paths only when needed) witness v20-v21 cold lifecycle. This **does not** grant generic `src/**`/`tests/**`, docs paths, new CMake, workflow, vendor or Qt privileges. New paths require their own finite D2.

**D2-P21-3 — exact lifecycle and acceptance, accepted:** The reviewed document's test scenarios **P21-01–P21-10** are obligatory: six-kind strict JSON/round-trip; empty and retired ID cursor; real v20 and older backward read/migration; table-driven malformed/native-container fail-close; Add/Erase/geometry-Delete cascade with identity-preserving Undo/Redo and fork; real cold Save/Close/Reopen and stable Document/Sketch/Profile/Feature/projection identifiers; native stage-bound Sketch→Profile/Extrude/Revolve with **original pinned PlaneGCS** where independently licensed/dev-gated, no stale candidate; invalid downstream repair-required with no current BRep; unchanged PG-01 fixed source policy; atomic and checkpoint/conflict behavior; registered RED→GREEN Windows focused/FAST/ready FULL + original solver-native job and independently approved DOC-01 PL/EN/internal/Browser + Owner manual PASS. A schema test PASS is **not** A1 technical FINAL or released product permission.

**STOP / dependency order:** (1) governance PR #511 stays Draft/unmerged until independent exact-head CLOSURE and separate Owner guarded-squash merge permission, then source-readback on protected main; (2) this separate **one-file active-contract incorporation** must pass its own exact-head CLOSURE and receive a distinct Owner guarded-squash permission, then source-readback; (3) only after both effective approvals may the separately gated v21 RED-first product implementation begin in Draft PR #506 or an Owner-approved successor branch, limited to the frozen paths. No `work/ACTIVE.yaml` edit is needed since it already points to this A1 contract; no automatic merge, `main` update, native schema bump, v21 product edit, Qt UI, A2/A3 dimensions, new vendor distribution or LGPL release clearance follows from this proposal. A1 Qt controls remain subject to the requested real SS2 UI walkthrough before any UI change.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: meaningful persistent CAD intent and a new user-facing capability. Identify and separately approve **exact** documentation paths before documentation mutation; no G0 documentation files are repurposed.

## 7. Handoff / status

This PR is **governance/design only**. No R13-A1 tracked product source/test/schema/vendor/UI modifications, no `ACTIVE.yaml` edit, no protected-main merge, and no assumption of exact contract acceptance. Next: independently accept/freeze this finite contract, close G0 per its actual process, guarded-merge contract, then an **independently authorized** one-file ACTIVE transition. Build the donor-based A1.1 native RED then GREEN only after verified activation.
