# Sketcher R13-G0 — Donor and local-constraint feasibility — D2 Work Contract PROPOSAL

**Status:** **DRAFT / EXACT OWNER D2 ACCEPTANCE PENDING / UNMERGED / NOT ACTIVE / NO PRODUCT OR DEPENDENCY MUTATION AUTHORITY.**
**Proposed:** 2026-10-18. **Design input:** Owner-approved R13 user/product D3 choices recorded in Draft PR #493 (separate design proposal, still unmerged); paired scheduling in Draft PR #494 (Sketch roadmap proposed v1.11 / Part proposed v1.35, still unaccepted exact text). **Baseline:** protected SS2 main `47bcd16f2b75b845b14fc5d9fecf5ffe8e553178`, S3 product #490 merged, S3 manual/Windows 199-test evidence; `work/ACTIVE.yaml` still references S3 / Sketch roadmap 1.10.
**Donor:** private `MechanicalCave/SimpleSolid`, `main`, `components/SimpleSolid_Sketcher` Constraint2D, sketch model/dimension interaction, numeric solver/PlaneGCS adapter, S5/S6 Parametric Loop/Extrude evidence. **No donor source, transitive header, binary or vendor blob may be copied or committed under this G0 contract.**

## 0. Authority, gates and priority

**Governance reconstruction:** `AGENTS.md` → Constitution/Foundation/Architecture → ADR-0008/0009/0011/0012 and Part/Projection ADRs → `work/ACTIVE.yaml` → *accepted* Sketch roadmap + accepted paired Part roadmap → **exact Owner-accepted** R13-G0 Work Contract → DOC-01. Shared 2D owns optional **local** solver-neutral authored constraints and evaluation; Part owns Sketch support/projection/profile/feature and native format; DocumentSession owns Commands/Undo/Redo/revisions; Qt/OCCT only presents runtime derived state. No universal parametric dependency graph, new standalone Point entity or change in upstream PG-01 semantics.

**Activation requires all:** (1) exact Owner acceptance of paired Sketch v1.11/Part v1.35 roadmap text; (2) separate guarded-squash merge of that roadmap PR after exact-head Windows governance CLOSURE; (3) exact Owner acceptance of **this G0 contract §§0–9 and all conditional paths**, **separate** explicit governance PR guarded merge permission, exact-head CLOSURE and verified merged main; (4) a dedicated one-file `work/ACTIVE.yaml` PR pointing `active_work` to this merged contract, synchronizing `program_roadmap_version` to accepted v1.11 while retaining ADR/Foundation/next-history; exact-head CLOSURE + distinct Owner authorized guarded squash; (5) only after readback from main may G0 be performed. No direct changes to protected main, no treating PR opening/D3 approval as execution approval.

**One active phase:** G0 is **read-only source/native feasibility and evidence**, not test-first A1 implementation. A1/A2/A3 each require their own bounded contract and switch. Deferred PM-06/RG-02 obligations remain untouched.

## 1. Objective and outputs

Find and document a feasible **SS2-native** architecture and legal/runtime dependency strategy for R13-A1, with actual bounded baseline measurements and explicit STOP findings. G0 must not itself add authored constraints/dimensions to the product. Deliver a separately reviewable technical decision dossier and draft R13-A1 exact D2 interfaces/schema/allowlist with authentic native tests planned as RED-before-production.

**Required artifact:** `work/SKETCH_R13_G0_FEASIBILITY_ACCEPTANCE_EVIDENCE.md` containing exact SS2 and donor commit SHAs, commands, test names, environment/config, quantitative measurements with sample sizes and uncertainties, compatibility table, rejection cases, provenance/licensing analysis and A1 readiness verdict. This ledger MUST distinguish **direct Windows SS2 evidence** from **historical donor test evidence** and from **design assumptions**. Merely citing donor M17 PASS cannot be labelled as SS2 native R13 PASS.

## 2. Donor mapping: what may be adapted after G0

| SS1 donor class/test | G0 fact to establish in SS2 | Ownership warning |
| --- | --- | --- |
| `platform2d/constraint2d/{identity,references,constraints,problem,solver}.hpp` | Map 5 accepted A1 relations to SS2 Sketch-owned `EntityId + semantic Start/End/Line` identities; candidate typed ConstraintId / conflict / DoF API, no alias or coordinate reattachment | No standalone Point entity or new parallel EntityId allocator |
| `solver_acceptance.hpp`, native solver tests | Determine distance/angle unit tolerances, finite residual acceptance, fixed/projected checks, diagnostic identity mapping, under/redundant/conflicting/no-convergence fixtures | Acceptance policy must be caller-owned and measured, not process-global epsilon |
| `src/solver/planegcs_sketch_solver.cpp`, persistent sessions | Verify version/pin/provenance, licensed distribution plan, native MSVC/Qt/OCCT isolation and deterministic rebuild/value-sync behavior; compare alternate isolated backends | No vendor or new CMake target under G0; no provider/solver type leakage into Foundation contracts |
| donor `SketchModel/SketchSession`, `dimension_state` | Identify reusable value/ID/placement/dual-track semantics while preserving SS2 DocumentSession/PartDocument authority | Do NOT import donor SketchSession history or donor JSON/schema |
| donor `SketchController` / `dimension_tool` / Qt canvas | Capture exact SS2-native 3D viewport labelling, dragging, inline text editing, Esc/Delete/selection risks for later A2/A3 tests | Donor's known numeric QLineEdit lives on toolbar; not proof of on-label SS2 UI |
| M17-S0/S5/S6 reports + tests | Enumerate donor Windows native evidence and parametric Sketch→Profile→Extrude/cold reopen cases; design SS2 replication/negative test matrix | Donor historical PASS is not a new SS2 RED/GREEN |

## 3. SS2 source/identity and dependency mapping

Read current exact `src/sketch/include/simplesolid2/sketch/sketch_model.hpp`, Sketch native EntityId/ElementRole headers, `src/part/include/simplesolid2/part/part_sketch.hpp`, `src/application/document_session.cpp`, native store (`src/part/part_document_store.cpp`), `src/part/include/simplesolid2/part/feature_evaluation.hpp`, Qt Sketch interaction/Workbench and registered SK-02B/SK-04C/PM-02F/Part persistence suites.

Required semantic decisions **as proposals, not implementation**: (a) constraint/dimension record identity+cursor ownership; (b) authored seed/evaluated geometry and current-revision caches; (c) stage-valid projected/fixed sources and support-frame mapping; (d) migration of strict `.ss2part` current schema to a proposed **next version only after verifying current writer version**; (e) one revision-bound command/transaction and Undo history; (f) S3 Esc/selection/native Qt text focus; (g) exposure of evaluated curves to Profile/Extrude and no stale final native Body; (h) same-ID Undo/new-ID duplication across R12 Trim/Extend, Mirror/Copy and approved geometry Delete cascade.

**Accepted first slice (Owner D3):** A1 geometric relations Coincident(2 Line endpoints), Horizontal(Line), Vertical(Line), Parallel(2 Lines), Perpendicular(2 Lines); A2 dimensions Line Length, Circle Radius, Circle Diameter (no duplicate independent R+D for one Circle); A3 durable read-only Reference dimensions, explicit typed fresh-ID conversion. Auto-Constraint OFF, all source-specific snap inputs remain transient. Arc dimensions, Tangent, Equal, Concentric, Point-on-curve, Angle, broad nonlinear parameter expressions deferred.

**Owner-accepted lifecycle:** 3D viewport dimension annotations are draggable *without detaching* semantic targets, direct Driving value edit on numeric label, Esc/Delete local keyboard priority and cold Save/Reopen; Delete Driving retains last accepted solved geometry; Delete selected user-owned geometry cascades only local referenced constraints/dimensions with impact confirmation, and invalid Part Profile/Extrude remains durable repair-required (B). Convert Driving/Reference uses new destination namespace ID and restores old typed ID on Undo.

## 4. G0 read-only verification matrix

All tests run on a clean exact recorded SS2 HEAD using **existing registered** test commands and workflow only; do not change test assertions or register new tests under G0.

| Gate | Evidence to collect (pass/fail/blocked with actual log) |
| --- | --- |
| G0-01 Foundation | Read authority, donor/legal status, detailed compatibility checklist with SS2 `Line/Circle/Arc + roles`, Part provenance and strict native-store ownership; record every stop condition |
| G0-02 Numeric design | Build a fixture/spec table of A1 5 relations, under-constrained legal and DoF, duplicate/redundant, opposing fixed endpoints, line singularity, angular/distance residual, NaN/Inf, stale revision, deterministic A1 rejection policy. Historical donor test coverage separately identified |
| G0-03 Baseline Windows | Run registered current SS2 Core + native Kernel + Qt/Desktop tests on exact HEAD through existing `.\ss2.ps1 test` (verify applicable valid Tier/Target/Test invocation from installed runner), record full inventories and counts; never import historical 199/199 as newly executed |
| G0-04 Part host | Run/inspect registered existing Sketch lifecycle (`sk02b`), stage-aware Profile/Extrude (`pm02f`), projection / cold store + Undo/Redo; record preexisting status and targeted evidence; no fabricated solver run |
| G0-05 Dependency/pin | Recompute SS1 PlaneGCS archive SHA-256 / vendor provenance, read native license, Eigen3/Boost requirements, test Windows compiler compatibility in **isolated external workspace only if legal/available**; document LGPL dynamic/static distribution options and owner legal signoff question, no network-fetch within SS2 build |
| G0-06 Performance | Record at least baseline empty/simple Part Sketch authoring and more complex 13-feature fixture **only if available and reproducible**, with measured sample count, median/p95 and cost split edit/solve/evaluation/UI where instrumentation already exists; do NOT invent threshold or regressions absent real data |
| G0-07 UI feasibility | Review actual Qt/OCCT viewport overlay and focus/hit architecture; diagram direct inline numeric label editor + Sketch-relative annotation drag + Cancel/Delete ownership; prove feasibility plan on real Qt test names, not donor toolbar mock |
| G0-08 D2 A1 candidate | Deliver draft A1 bounded exact paths, interface/record/serializer versioning options, solver choice risk, genuine Windows RED cases, Backward-reader/cold migration gates, DOC-01 internal/Product/browser and product Owner GUI flows |

Evidence not available because Windows environment, external archive or baseline fixture is missing must be marked **BLOCKED**, with retry requirements; do not assume PASS. G0 completion requires Owner and independent technical review of the feasibility ledger, not a code merge.

## 5. Exact G0 edit allowlist (2 paths; conditional only after ACTIVE)

1. `work/SKETCH_R13_G0_FEASIBILITY_D2_WORK_CONTRACT_PROPOSAL.md` — **governance-only contract text**: freeze on accepted merge; any later contract edit needs separately Owner-accepted D2 addendum and cannot be assumed under ACTIVE.
2. `work/SKETCH_R13_G0_FEASIBILITY_ACCEPTANCE_EVIDENCE.md` — **only mutable G0 artifact after activation**, factual technical ledger recording measurements, read-only source inspection and explicit test outputs / blockers.

**Forbidden during G0:** changes under `src/**`, `tests/**`, `include/**`, `CMakeLists.txt`, `cmake/**`, `.github/**`, `scripts/**`, `docs/internal/**`, `docs/product/**`, `docs/browser/**`, `third_party/**`, schema/data fixtures, `work/ACTIVE.yaml`, accepted roadmaps or donor code. No new solver dependency, test weakening, semantic Command, Qt surface or SDK API. A necessary instrumented test fixture/vendor sandbox inside SS2 requires a **new specific Owner D2 allowlist amendment before mutation**. Temporary untracked build outputs are not product commits. External licensed feasibility sandbox must remain isolated and documented.

## 6. G0 failure / STOP thresholds

STOP and ask Owner for D2/D3 amendment for: inability to resolve semantic endpoint role without invented Point entity, unsupported projected fixed-reference terms; non-reproducible solver/LLGPL provenance/distribution risk; non-deterministic under/over/constrained diagnostics; requirement to change Part persistence/Kernel/Qt API during G0; hidden numerical epsilon, mutated authored seed on read-only recompute; inconsistent Part-stage Profile/Extrude invalidation; no actual Windows evidence for claimed PASS; any source/test edit outside two files.

Any solver candidate must be evaluated against alternative or reference residual policy. Do not promise that PlaneGCS is selected until native build/ABI/license/distribution are reviewed and Owner explicitly accepts its dependency addition in an A1 D2 contract. Native PlaneGCS archive pin and hash in donor may serve as evidence **only after independent verification**.

## 7. G0 final output to Owner / A1 readiness

Ledger must include: clean exact SS2 Head/base, donor commit SHA and license/provenance, numeric policy candidate with units, full compatibility matrix, actual registered Windows test results with links, product baseline metrics or BLOCKED, explicit per-risk STOP/GO disposition, evaluation dependency boundary, design for Native 3D label/annotations and impact-confirmation UX, and one **DRAFT A1** exact D2 plan (typed records/schema/commands, 5 constraints, source-level conditional allowlist, real Windows RED-first tests, solver and migration alternatives). All source-visible R13 work remains unauthorized until new A1 contract + ACTIVE approval.

**G0 closure sequence:** factual evidence-only branch and CLOSURE or existing Work CI, independent technical validation of findings, Owner explicit G0 completion/approval, separately authorized ledger merge if needed; separately design, approve and merge A1 contract, independently switch ACTIVE and register Windows RED tests only then. No implicit R13-A1 code authorization from completing G0.

## 8. Documentation Impact — DOC-01

**Internal docs: not required**. **User/Product docs: not required.** **Reason:** G0 reads and evaluates current product and produces work-only feasibility evidence, with **no shipped behavior, identity, interface or persistence changes**. Actual A1/A2/A3 contracts must declare internal/product PL+EN and generated Browser impact **required** for their shipped semantics.

## 9. Owner decisions still required

1. Accept **exact** R13-G0 D2 §§0–9 and both allowable paths (with only evidence Markdown mutable after activation), rather than treating D3 product approval as code permission.
2. Separately accept/merge paired roadmap proposed v1.11/v1.35 first, then authorize **guarded squash** of this G0 governance-only contract after exact-HEAD Windows CLOSURE; third distinct Owner approval for one-file ACTIVE switch.
3. At G0 end choose backend/license/distribution, schema and A1 boundaries using real native evidence. Current R13 product D3 decisions are fixed, but SS1 PlaneGCS vs alternative numeric backend is **OPEN**.
