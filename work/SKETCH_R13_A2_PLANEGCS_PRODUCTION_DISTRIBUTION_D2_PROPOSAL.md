# SS2 R13-A2 — PlaneGCS production distribution / native solver D2 decision proposal

**Status:** PROPOSED / OWNER DECISION PENDING / NOT AN ACTIVE IMPLEMENTATION CONTRACT
**Date:** 2026-10-23 (Europe/Warsaw)
**Decision class:** D2 production backend, third-party distribution, provider-neutral integration and CI scope; possible D3 commercial licensing/redistribution disposition remains Owner-owned
**Authorities:** `AGENTS.md`, frozen Constitution/Foundation/Architecture, ADR-0008/0009/0012, `work/ACTIVE.yaml`, `work/SKETCH_ROADMAP.md` v1.11, active `work/SKETCH_R13_A2_RELATIONS_DRIVING_DIMENSIONS_D2_WORK_CONTRACT_PROPOSAL.md` §§0, 2.2, 3, 4, 6, 6B
**Scope of this PR:** ONE `work/**` proposal only. NO `ACTIVE.yaml`, `src/**`, `tests/**`, CI, vendor/dependency, persistence, production API, UI or product documentation changes.

## 1. Proven facts, separated by acceptance boundary

| Evidence | Verified outcome | What it does NOT prove |
| --- | --- | --- |
| R13-A1 original pinned PlaneGCS 0.8.0 native Windows | **PASS**: [PR #506 original-native + docs run #39620240812](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39620240812), real CTest `r13a1.planegcs_native` and `r13a1.planegcs_occt_native` **2/2**, Windows MSVC source-backed Core + OCCT. Separate exact-head FULL [#39626326569](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39626326569) **26/26 Core, 59/59 Kernel, 118/118 Qt Desktop** on final R13-A1 product+DOC-01 commit `bfc135bc775e9b1d45b3ea489d3fb721399b0fc1`. | No redistributable product, licensed installer, tested replacement/relink workflow, Circle/Driving equations, production A2 GUI, A2 strict v22. Native original tests were **not** run in the manually dispatched FULL workflow; two separate same-SHA workflows establish separate evidence. |
| R13-A2.0 DEV/native Qt-controller product branch | Exact feature-branch `dc24b03c701a06a44bbc9de2150717f350335e70`; manual [FULL #39940051026](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/39940051026) **26/26, 59/59, 118/118**. Native PR-head repair proposal is [Draft #528](https://github.com/MechanicalCave/SimpleSolid2/pull/528), same exact source commit, no feature changes. | Uses explicitly borrowed/injected DEV solver for GUI relation acceptance; no generally available production PlaneGCS. Manual branch FULL does not constitute standalone original-PlaneGCS native, installed consumer or product Owner FINAL. |
| Existing optional CMake backend | `src/CMakeLists.txt` declares `SS2_R13_PLANEGCS_DEV=OFF` by default, requires separately provisioned original source, exact file SHA-256 manifest and explicit Eigen/Boost headers. It builds `ss2_r13_planegcs_dev` as a **STATIC** target for test use only. | No production linkage, distributable licensing bundle, DLL ABI, installation layout or replaceability proof. The header `planegcs_dev_solver.hpp` says DEV-only and no Part publication. |
| Existing original-native PR CI | `.github/workflows/windows-pr-gate.yml` job `windows-planegcs-native-core` currently runs only if `github.event_name == 'pull_request' && github.event.pull_request.number == 506`; a native original-source runner path is already evidenced on that historical PR. | New A2 PR #528 cannot receive this native job without a **separate Owner-approved exact D2 CI gate change**. A SKIPPED lane is never PASS. |

Pinned 0.8.0 candidate archive: `planegcs-0.8.0.tar.gz`, SHA-256 `09e77e6e3a27fa77d3b5a69065ea4c82ae6ed35753246a2e518b1a6005fea162`; corresponding upstream commit `5a60d5328bedd85f1e4aef81b9b57f5c4410ae52`. Already inspected source metadata and native DEV tests are not grounds to relax byte/manifest verification for a future distributable artifact.

Upstream identifies the extracted FreeCAD-derived solver as **LGPL-2.1-or-later**. The applicable license's §6 requires specific notice/source and linking or relinking provisions according to how the application and LGPL library are distributed. The external [GNU license text](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html) and [GNU FAQ](https://www.gnu.org/licenses/gpl-faq.en.html#LGPLStaticVsDynamic) explain the principal shared-library versus static-object/relink distinctions. This proposal is **not** a legal opinion or an assertion that a proposed build layout already complies.

## 2. Owner decision requested — choose a technical/licensing feasibility route

**L-DLL — recommended first, narrowly scoped D2 feasibility experiment, NOT predetermined production approval.** Keep the SS2 solver-neutral `ILocalSketchSolver` and all Part-stage authoring gates. Investigate an actually independent, interface-compatible, user-replaceable original PlaneGCS 0.8.0 **shared-library mechanism** in the Windows installed SS2 application. Before deciding this is viable, freeze the wrapper ABI ownership, dependency/linking and distribution; build the external original with verified source/dependencies, demonstrate application startup and actual user replacement with a compatible rebuilt DLL, and independently review LGPL source/notice terms. A DEV `STATIC` test target is not a DLL feasibility result. If an appropriate narrow ABI requires a new source path, install target, workflow or public contract, STOP for an exact Owner D2 file/interface amendment before implementation.

**L-STATIC — alternative requiring its own documented legal/relink design.** Keeping upstream original implementation objects in a statically linked target is not automatically forbidden by LGPL, but this path requires a fully documented §6-compliant redistribution/relink method, corresponding library source/patches, appropriate notices/license and complete customer-supplied object/relink materials or other properly reviewed license-compliant path. An existing DEV static native CTest cannot authorize product static distribution.

**L-ALT — separately justified alternative solver.** Select only with independent license/technical acceptance, original five relation parity, fixed/projection source behavior, strict residual/DoF/redundancy, real Windows native test and identical downstream Part/Kernel lifecycle. A new dependency is D2; no automatic authority under R13-A2.

**No-choice is a STOP:** no preferred route may silently become product runtime, and a feasibility experiment must not be described as commercially/redistributably accepted.

## 3. Mandatory proof before production A2.0 / A2.1 acceptance

1. **Source / provenance:** obtain original immutable 0.8.0 distribution bytes in a legal external build environment, validate exact archive digest and consumed file manifest; pin supported Eigen/Boost and MSVC toolchain. No network auto-fetch or uncontrolled vendor copy in SS2.
2. **Distribution / installed consumer:** Owner-reviewed LGPL-2.1-or-later notice, license text, correspondence/source offer as applicable, attributable source, replacement or relinking proof appropriate to chosen route, repeatable clean installed x64 Windows consumer and a deliberately replaced compatible PlaneGCS library if DLL is selected. Legal review is independent of numerical test PASS.
3. **Solver acceptance:** exact source-stage Line inputs; Horizontal, Vertical, exact endpoint Coincident, Parallel, Perpendicular and Fix parameter mask; converged candidate, bounded residuals, DoF/conflict/redundancy, unchanged fixed inputs, zero guessed references or implicit snap constraints; no authoring if source is stale/unsupported, solver reports conflict or output violates independent SS2 verification.
4. **Transactional Part lifecycle:** actual installed product invokes an approved provider through Command → Validation → Transaction → PartDocument → Evaluation and revalidates DocumentId, Part runtime incarnation, revision and Sketch stage after every callback; Undo/Redo, Save→Close→new-process Reopen, Profile→native OCCT Extrude/Revolve, rejected and failed candidate paths. No last-good BRep presented as current.
5. **Native CI:** exact-HEAD FOCUSED/FAST/Ready FULL Core/Kernel/Qt, product DOC-01 PL/EN+internal+generated Browser, a separately justified A2 original-native runner lane and installed-consumer proof; maintain historical #506 results without falsely crediting skipped A2 jobs. No relaxing tests or special-casing unsupported source.
6. **A2.1 separation:** Circle center/radius inputs and Line Length/Circle Radius/Circle Diameter equations, strict v22 schema and 3D annotation/focus require their *own* accepted D2 details; do not treat first solver distribution feasibility approval as schema/GUI authorization.

## 4. File and decision-scope boundary

For a **future exact Owner-approved experiment**, some existing R13-A2 §4 paths may be relevant: `src/CMakeLists.txt`, `src/sketch/include/simplesolid2/sketch/local_solver.hpp`, `src/sketch/local_solver.cpp`, `tests/CMakeLists.txt` and named local-solver/native tests. Existing `.github/workflows/windows-pr-gate.yml` is only conditionally listed in the contract for separately justified CI changes. No new adapter/install/build/documentation path is authorized by this proposal; any required exact addition must be enumerated, reviewed and accepted via an **independent D2 addendum**. Existing DEV adapter is not to be renamed 'production' without qualifying evidence.

**Proposal-only result:** If Owner selects L-DLL, authorize a bounded **read-only/native evidence and concrete ABI/install/relink design** phase first, not automatic production code. A separate exact file allowlist, approval and validation plan precede implementation. The original static DEV native adapter remains optional, OFF by default and excluded from shipped claims.

## 5. Owner review questions / STOP conditions

**Decision 1 (D2 feasibility):** approve **L-DLL technical exploration only**, or request L-STATIC evidence design, or select L-ALT. No release permission implied.

**Decision 2 (D2 source scope, deferred):** after seeing exact proposed CMake/adapter/test/CI path list and generated ABI/install model, accept or reject the bounded implementation amendment; no implicit `third_party/**`, SDK/API or workflow blanket permission.

**Decision 3 (D3/legal distribution, deferred):** independently accept the actual commercial/open distribution strategy, library source/patches, relevant notices, replacement/relink and installed-consumer proof **before** production A2 release. Neither decision 1 nor native 2/2 DEV PASS satisfies this gate.

STOP for new path/API without separate Owner approval, license conflict, absent original bytes, misidentified fixed input, stale Part stage, nonconverged/redundant/ambiguous result, incomplete installed-consumer test or failure to revalidate at command commit. A2.0 remains Draft; A2.1–A2.3 and R13-A3 not activated or accepted by this proposal. PM-06 / further RG-02 deferred.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This is an unaccepted governance/design proposal only; no current implementation or user workflow changes. A future approved production backend needs its own as-built and release documentation impact and DOC-01 verification.
