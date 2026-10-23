# SS2 R13-A2 — PlaneGCS production distribution / native solver D2 decision proposal

**Status:** OWNER-SELECTED L-DLL FEASIBILITY DIRECTION (2026-10-23); EXACT ABI / FILE SCOPE / LICENSE / PRODUCTION IMPLEMENTATION STILL PROPOSED, NOT APPROVED
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

**Decision 1 (D2 feasibility): OWNER ACCEPTED L-DLL EXPLORATION ONLY (2026-10-23).** In direct response to the L-DLL first feasibility request, Owner answered `akceptuje - kontynuuj`. Record the selection of read-only/native research and ABI design, **not** authorization to add C++ source, change CI, vendor the library or distribute an application. L-STATIC and L-ALT are alternatives for a later separate decision, not presently selected.

**Decision 2 (D2 source scope, deferred):** after seeing exact proposed CMake/adapter/test/CI path list and generated ABI/install model, accept or reject the bounded implementation amendment; no implicit `third_party/**`, SDK/API or workflow blanket permission.

**Decision 3 (D3/legal distribution, deferred):** independently accept the actual commercial/open distribution strategy, library source/patches, relevant notices, replacement/relink and installed-consumer proof **before** production A2 release. Neither decision 1 nor native 2/2 DEV PASS satisfies this gate.

STOP for new path/API without separate Owner approval, license conflict, absent original bytes, misidentified fixed input, stale Part stage, nonconverged/redundant/ambiguous result, incomplete installed-consumer test or failure to revalidate at command commit. A2.0 remains Draft; A2.1–A2.3 and R13-A3 not activated or accepted by this proposal. PM-06 / further RG-02 deferred.


## 6. L-DLL read-only ABI feasibility design — prepared after bounded Owner research approval, NOT frozen

### 6.1 Exact boundary and a failure risk

At pinned upstream `spookylukey/planegcs@5a60d5328bedd85f1e4aef81b9b57f5c4410ae52`, `src/wrapper.h` exposes C++ `SketchSolver` with STL containers/exception-capable methods. Upstream `CMakeLists.txt` builds `pybind11_add_module(_planegcs)`; this Python module is **not** a stable native C ABI or a drop-in user-replaceable SS2 DLL. A CMake STATIC→SHARED change, direct C++ mangled export or `LoadLibrary` of `_planegcs.pyd` without a specifically reviewed native interface is **insufficient**.

**Candidate seam:**

```text
PartCommand/revision+incarnation+stage validation
    → SS2 sketch::ILocalSketchSolver (existing neutral C++ seam)
    → privately owned SS2 DLL client/admission adapter
    → explicit versioned C-compatible ABI (no Qt/OCCT/C++ objects)
    → source-backed PlaneGCS 0.8.0 DLL (original solver + thin ABI bridge)
    → caller-owned, fully checked primitive candidate/diagnostics
    → SS2 independent semantic+numeric residual review
    → guarded Part transaction only on total acceptance
```

**Single point of failure:** a genuinely replaceable, ABI-compatible DLL whose source/distribution/relink pathway and installed consumer pass on real Windows. If not achieved, reject L-DLL as production route; do not mark DEV static original-native tests as a release substitute.

### 6.2 Candidate ABI policy for later D2 freeze — no public SS2 CAD-domain contract yet

Propose **one explicitly versioned, fixed-width C ABI**, callable under a documented Windows calling convention from the SS2-private adapter. This section is *acceptance criteria*, not declarations presently permitted for production.

| Concern | Proposed bounded invariant |
| --- | --- |
| Entry/export | Exactly one documented factory/version negotiation symbol with explicit ABI major, minor, size and capabilities; no relying on C++ mangled `SketchSolver` symbols or Python module |
| Request | Plain fixed-width integer tags and finite `double` values for one disposable solver invocation, with explicit sizes/counts, supported Line endpoints, equation semantic roles and fixed flags; **no durable DocumentId/SketchId/ConstraintId** crossing as CAD identity: map invocation-local indexes/tags only inside SS2 |
| Constraints | Only A2.0's already accepted five Line equations and Fix-mask; reject unknown discriminants, reserved fields, inconsistent count, duplicate map, unsupported projected/linked source. A2.1 Circle and numeric kinds require a **later** separate ABI capability/version decision after v22 freeze |
| Result | Explicit enum status/convergence, DoF, native diagnostics tagged by invocation-local equation index, full candidate coordinate array with expected count; distinguish unsupported, conflict, redundant, not-converged and ABI failure; no partial success or invention of missing diagnostics |
| Ownership | Caller supplies memory buffers/capacities, DLL reports required capacities without unbounded allocation or pointer retention; if a per-solve opaque native handle is needed, creation/destruction must be paired in the same loaded module and covered by leak/error tests; **never free across module CRT boundaries** |
| Exceptions/threads | No exception, STL, callback, Qt, OCCT, external file path or raw pointer lifetime crossing the ABI; catch native exceptions at DLL boundary and return typed failure; specify non-reentrancy or independent fresh session per solve; do not silently cache per-document native state |
| Version failure | Incompatible major, undersized struct, unsupported operation, absent export, mismatched bitness or corrupt output **fails closed**. No fallback to scripted/echo DEV or unsolved A1 command in shipped UI |
| Loader | Resolve only a verified application-controlled absolute installation path with safe Windows DLL dependency-search flags, never CWD/PATH-first or implicitly another system DLL. Pin main product's expected ABI and record actual loaded module identity for troubleshooting; security/signature policy requires distinct review |
| Part integrity | SS2 validates exact DocumentId, runtime incarnation, revision, active Sketch stage and fixed-source lease both sides of backend invocation; rechecks finite nondegenerate geometry, exact endpoint Coincident closure, independent residual tolerance, semantic IDs and no new authored mutation before final commit. Runtime solver pointers/tags are not persisted |

A compatibility contract for a replaceable library must distinguish ABI compatibility from numerical/semantic acceptance. A replacement may solve differently while still returning a valid byte layout; only the strict host-side residual, fixed-input, convergence and complete diagnostic tests can authorize candidate publication. If replacement lacks an explicit supported algorithm/version capability, fail closed and show a typed actionable diagnostic.

### 6.3 Windows evidence matrix, before any shipping claim

| Phase | Required executable evidence | Failure oracle |
| --- | --- | --- |
| E0 source attestation | Independent immutable `planegcs-0.8.0.tar.gz` digest `09e77e6e3a27fa77d3b5a69065ea4c82ae6ed35753246a2e518b1a6005fea162` plus exact consumed-source SHA manifest, Eigen/Boost pin and MSVC x64 build | Hash/version/file/headers missing or unapproved vendored copy → STOP |
| E1 DLL prototype | Isolated, untracked external Windows MSVC x64 build with C symbol export inspection, handshake and pure C consumer; two builds of equivalent source with independent dll/lib link inputs | C++ mangling, missing/incorrect exports, CRT-cross ownership or invalid ABI version → STOP |
| E2 actual replacement | Clean installed-consumer directory using no developer build tree, verified library identity and **user-replaced rebuilt compatible DLL**, cold process restart, identical expected semantic outcomes | Hidden load from dev PATH/working directory, package requires developer machine, relink impossible → STOP |
| E3 adversarial/native | Failures for missing DLL, wrong bitness, missing dependent DLL, wrong ABI, truncated buffers, duplicate tags, nonfinite values, conflicting/redundant equation, nonconverged partial candidate, stale lease during callback, unsupported projected source | Any resulting Part mutation, Undo/ID consumption, stale publish, accepted partial solution or silent mock fallback → STOP |
| E4 Part integration | Original installed backend drives six manual A2.0 relation gestures including Fix Current Position, select/delete and current-stage Profile→OCCT Extrude/Revolve; Save→Close→new-process Reopen, Undo/Redo, failed-body repair, genuine UI owner-selection and camera input | No real user-facing 3D viewport pathway or stale/ambiguous glyph selection → A2.0 not complete |
| E5 licensing/release | Owner/legal signoff on LGPL-2.1-or-later notices/full license, corresponding source and patches, attribution, relink/replacement instructions/rights and binary dependencies for each distributed build; test clean customer-like replacement, not merely document it | Absent approval/proven installed mechanism → NO RELEASE |
| E6 CI | Registered exact-head Windows native/DLL and installed-consumer job; applicable FOCUSED/FAST/Ready FULL + DOC-01 checks and independent Owner manual/technical FINAL | `SKIPPED` is not `PASS`; old PR #506-only native CI remains historic, not A2 proof |

### 6.4 Path impact inventory — CANDIDATES ONLY, next explicit Owner D2 path approval required

**Already named in accepted A2 §4 conditional maximum** (not automatically applicable to DLL):
- `src/CMakeLists.txt`: separate externally supplied original-source build target, build-tree vs install-tree and safe OFF default, with no automatic downloads
- `src/sketch/include/simplesolid2/sketch/local_solver.hpp`, `src/sketch/local_solver.cpp`: **only if required** to preserve the existing SS2-neutral solver acceptance API; no public CAD identity changes
- `tests/CMakeLists.txt`, `tests/core_only/CMakeLists.txt`, `tests/kernel_native/CMakeLists.txt`: separately allowed targets/test registration as appropriate
- `.github/workflows/windows-pr-gate.yml`: specifically Owner-reviewed extension of PR-#506-only original-native lane, not an authorization to edit CI now
- Existing `tests/r13_a1_local_constraints_test.cpp` and existing Sketch-domain tests: only new positive and negative native evidence, never weakened assertions

**Likely new exact-path candidates, explicitly NOT AUTHORISED BY THIS ACCEPTANCE:**
- `src/sketch/include/simplesolid2/sketch/planegcs_dll_abi.h` — candidate private frozen C ABI declarations only, no Qt, OCCT or public domain identity
- `src/sketch/planegcs_dll_backend.cpp` — candidate private host adapter from existing neutral `ILocalSketchSolver`
- `src/sketch/planegcs_dll_exports.cpp` — candidate source-backed DLL implementation of C ABI built only when verified original source is explicitly provided
- `tests/r13_a2_planegcs_dll_abi_test.cpp` — candidate host handshake/error/diagnostic/replacement native test
- Any **installer/packaging/installed-consumer** target, release notice and license/source/relink materials: **exact paths unknown**, therefore BLOCKED, not implicitly in scope

The accepted R13-A2 §4 list does not authorize these new paths. The future D2 addendum must either accept a fully enumerated minimal manifest with those exact paths or change the experiment design; an unknown install/relink path is a hard STOP. Do not edit `third_party/**`, `work/ACTIVE.yaml`, stored Part schema, product CMake/CI or release packaging on the basis of this design proposal.

### 6.5 Bounded acceptance and next decision

At the currently approved **L-DLL feasibility-only** scope, the legitimate completion is: an inspectable exact ABI design, source and dependency pin strategy, reproducible isolated external Windows experiment instructions and explicit gap report. It is **not** a shipping library or final A2.0.

The **next Owner D2 decision** must approve a reviewed concrete ABI shape, a finite exact-path diff budget and Windows experiment runner/installed-consumer matrix (including naming installer/relink paths) *before* any tracked implementation. A separate later D3/legal release decision remains mandatory. Reject blanket static fallback, extension of unapproved numeric/3D APIs, silent vendor installation and global parametric graph.


## 7. Exact experimental C ABI candidate v1 — reviewable D2 addendum, **NOT OWNER-FROZEN**

This is a **new concrete design proposal** within the previously Owner-selected *feasibility research* direction, not a claim that the Owner has approved these exact declarations or files. It defines E1: an *optional, externally source-backed Windows x64 development DLL experiment*, not an installer, shipping backend, or v22/numeric A2.1 implementation. The original SS2 R13-A2 work-contract §4 allowlist and Foundation remain unchanged. A separately explicit Owner D2 acceptance of this exact ABI and path list is mandatory before tracked C++/CMake/CI changes.

### 7.1 Standalone C11/C++20 candidate header (complete v1 declarations)

The proposed exact new path is src/sketch/include/simplesolid2/sketch/planegcs_dll_abi.h, a *private experiment* header, not a public SS2 domain API. On Windows x64 use the normal 8-byte struct alignment with default MSVC packing and __cdecl. Library definitions export these exact undecorated ASCII symbols with extern "C" and __declspec(dllexport); host discovers exports by absolute-path GetProcAddress, not implicit imports.

```c
#ifndef SS2_PG_DLL_ABI_V1_H
#define SS2_PG_DLL_ABI_V1_H
#include <stdint.h>
#if defined(_WIN32)
#define SS2_PG_CALL __cdecl
#else
#define SS2_PG_CALL
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define SS2_PG_ABI_MAJOR 1u
#define SS2_PG_ABI_MINOR 0u
#define SS2_PG_FIXED 1u
#define SS2_PG_UNUSED_INDEX UINT32_MAX

/* Tags refer to zero-based per-call positions, never durable CAD IDs. */
typedef struct ss2_pg_line_v1 {
    uint32_t tag;
    uint32_t flags;
    double u0, v0, u1, v1;
} ss2_pg_line_v1;

/* kind 1 Horizontal, 2 Vertical, 3 Coincident,
        4 Parallel, 5 Perpendicular.
   point fields: 0 whole Line, 1 Start, 2 End.
   The unused second line is SS2_PG_UNUSED_INDEX. */
typedef struct ss2_pg_equation_v1 {
    uint32_t tag;
    uint32_t kind;
    uint32_t line_a, point_a, line_b, point_b;
    uint32_t reserved0, reserved1;
} ss2_pg_equation_v1;

/* kind 1 conflicting, 2 redundant; tag is an equation index. */
typedef struct ss2_pg_diagnostic_v1 {
    uint32_t equation_tag, kind;
} ss2_pg_diagnostic_v1;

typedef struct ss2_pg_request_v1 {
    uint32_t bytes, abi_major, abi_minor, flags;
    uint32_t line_count, equation_count, reserved0, reserved1;
    const ss2_pg_line_v1 *lines;
    const ss2_pg_equation_v1 *equations;
} ss2_pg_request_v1;

typedef struct ss2_pg_response_v1 {
    uint32_t bytes, status, line_capacity, diagnostic_capacity;
    uint32_t line_count, diagnostic_count, reserved0, reserved1;
    int32_t dof; /* -1 unknown, otherwise nonnegative. */
    uint32_t reserved2;
    ss2_pg_line_v1 *lines;
    ss2_pg_diagnostic_v1 *diagnostics;
} ss2_pg_response_v1;

typedef struct ss2_pg_abi_v1 {
    uint32_t bytes, abi_major, abi_minor, capabilities;
    uint32_t sizeof_line, sizeof_equation, sizeof_diagnostic, sizeof_request;
    uint32_t sizeof_response, max_lines, max_equations, max_diagnostics;
} ss2_pg_abi_v1;

/* Transport return 0: valid invocation; solve status in response.status:
   0 converged, 1 conflict, 2 redundant, 3 nonconverged,
   4 unsupported, 5 backend failed.
   Nonzero transport codes: 1 incompatible ABI,
   2 invalid input, 3 insufficient capacity, 4 internal failure. */
int32_t SS2_PG_CALL ss2_pg_query_abi_v1(ss2_pg_abi_v1 *abi);
int32_t SS2_PG_CALL ss2_pg_solve_v1(
    const ss2_pg_request_v1 *request, ss2_pg_response_v1 *response);
#ifdef __cplusplus
}
#endif
#endif
```

**Proposed x64 size and offset checks, under default packing**:

| Type | sizeof (bytes) | Critical pointer offset |
| --- | ---: | ---: |
| ss2_pg_line_v1 | 40 | — |
| ss2_pg_equation_v1 | 32 | — |
| ss2_pg_diagnostic_v1 | 8 | — |
| ss2_pg_request_v1 | 48 | lines at 32 |
| ss2_pg_response_v1 | 56 | lines at 40 |
| ss2_pg_abi_v1 | 48 | — |

**Limited local syntax/layout check completed:** an untracked standalone header with the same structure/member layouts and function prototypes as this candidate compiled as C11 and C++20 with GCC on a Linux development environment and passed size/offset static assertions. Its source comments differ from this Markdown listing: **no byte-identical compilation of the embedded text or Windows MSVC ABI proof is claimed**. This is **not** a build of SS2, an original-native PlaneGCS DLL, installation proof, or release license approval. The proposed header is included for D2 review only; no tracked C/C++ source was created.

### 7.2 Required implementation interpretation (not negotiable without D2 revision)

1. **Handshake:** caller initializes abi.bytes=48, all other fields zero; query checks input size then returns major=1, minor=0, the exact five structure sizes above and finite capacities. Candidate fixed limits: max_lines=4096; max_equations=8192; max_diagnostics=8192. Capabilities bit 0 = all five Line relation equations, bit 1 = Fix mask. Unknown capability/reserved bits fail closed. Any mismatching major/size, unsupported minor or unknown capability is rejected. No silent compatibility guess.
2. **Semantic binding:** host sorts the current-stage SS2 semantic Line inventory deterministically, assigns invocation-local line.tag equal to its array index, equation.tag equal to its array index, and retains ephemeral lookups inside the caller. No original EntityId/ConstraintId, SketchId, DocumentId, revision, source UUID or Part feature identity crosses the DLL. A single request describes the *entire* current-stage constraint inventory, not just the new equation; missing fixed/projected source is unsupported, not guessed.
3. **Validation:** line.flags may contain **only** SS2_PG_FIXED. Every Line's endpoints and all input values must be finite; segments must be positive/nondegenerate. H/V each use one Line, whole-Line element=0, second line=UINT32_MAX, second point=0; parallel/perpendicular use two distinct Line indexes, whole-Line element=0; Coincident uses two valid Start/End endpoint indexes 1/2 (possibly on separate Lines). Any duplicate equation tag, invalid/reserved field, out-of-range element/tag or count over limit fails before a native solve.
4. **Solution ownership:** call is synchronous; all request pointers are borrowed only for the call. Caller supplies result arrays and explicit capacities; DLL never retains/frees caller memory. If output capacity is insufficient, return transport 3, set required counts only in response, write **no** result array elements, and reject any claim of partial convergence. For a valid converged reply, result.line_count equals request.line_count; each returned line tag/flags and every original semantic entity mapping must match the request. The DLL owns/free its native internal allocations and catches every C++ exception before crossing ABI.
5. **Statuses and diagnostics:** transport 0 does not mean a valid CAD solve. For solve status 0 the host independently runs evaluateLocalSolve/reviewLocalSolveCandidate including residuals, Coincident closure, fixed endpoints and exact stage/Part lease; conflict/redundancy/nonconvergence/unsupported/failure yield **no commit**. Each diagnostic refers to an existing equation.tag and uses kind 1/2 only; duplicate, out-of-range or contradictory diagnostics fail closed, with conflict higher priority than redundancy. Unknown DoF is -1, never fabricated 0. No silent mock/echo fallback on missing DLL.
6. **Loader and replacement:** approved application-owned *absolute path*, verified same-bitness exports and safe Win32 dependency-search flags; do not use CWD/PATH-first search. A clean customer-like process starts with pinned original DLL, then a separately rebuilt ABI-compatible replacement after cold restart, with actual module path/ABI diagnostic reporting. Runtime hard pinning to a single DLL digest would defeat the stated replacement objective, so separate distributed-source provenance attestation from runtime ABI/semantic validation and any later independent code-signing policy.
7. **Part atomicity:** after every native solve and before Transaction commit, recheck DocumentId, ephemeral Part incarnation, revision, Sketch support/stage, projected/fixed source and exact accepted semantics. No UI event, C++ wrapper, DLL pointer, native token or solver output becomes durable intent or bypasses Command. Preserve rejected candidate zero ID/Undo/revision consumption.

**Critical design question for Owner D2 freeze:** is a plain C ABI with an invocation-local Line/Equation array and two exports acceptable as the experimental boundary, or must the prototype use a different process boundary? No answer is implied by the earlier acceptance of L-DLL feasibility in general.

### 7.3 Finite proposed E1 implementation path budget — must be separately approved *as a set*

| Path | Status | Only authorized purpose **if separately Owner-frozen** |
| --- | --- | --- |
| src/CMakeLists.txt | Existing A2 §4 conditional | Add distinct OFF-by-default original-PlaneGCS DLL E1 target, pinned offline source and export compilation; no shipping installer |
| src/sketch/include/simplesolid2/sketch/planegcs_dll_abi.h | **NEW / NOT APPROVED** | Exact private v1 C ABI header shown above |
| src/sketch/planegcs_dll_exports.cpp | **NEW / NOT APPROVED** | Export two functions; adapt original source-backed SketchSolver, no crossing STL/Qt |
| src/sketch/planegcs_dll_backend.cpp | **NEW / NOT APPROVED** | Host-private loader and ILocalSketchSolver adapter, fail-closed native result mapping |
| tests/r13_a2_planegcs_dll_abi_test.cpp | **NEW / NOT APPROVED** | Registered MSVC native ABI handshake, real Line solver and malformed-loader/diagnostics test source; test-only negative DLL builds may reuse this file |
| tests/core_only/CMakeLists.txt | Existing A2 §4 conditional | Register no-Qt, no-OCCT MSVC native ABI/backend test only |
| .github/workflows/windows-pr-gate.yml | Existing A2 §4 **separate CI approval** | Exact PR-head E1 Windows source-backed native lane, no arbitrary number == 506 classifier restriction for new A2 test; never weaken existing A1 job |

**Exactly seven tracked paths for optional E1, no more.** The three new backend/interface sources and new test source are **not currently authorized** by the accepted A2 §4 conditional maximum. If a later E1 implementation needs tests/CMakeLists.txt, tests/kernel_native/CMakeLists.txt, install/packaging, public include, external vendoring, original wrapper changes, license notices or scripts beyond this list, **STOP** for a new exact-path D2 addendum. E1 is a development-only native ABI feasibility build and **cannot close** the installed-consumer replacement/license requirements E2/E5 or R13-A2.0 user-facing product gate.

### 7.4 RED → GREEN Windows matrix for any separately approved E1 implementation

- **RED-first registered native:** missing export; wrong major; wrong size/packing; truncated result and null buffer; incompatible capability; unsupported kind; invalid arrays/tags; duplicate diagnostics; nonfinite/degenerate Line; two independent fixed Line cases; conflicts, redundancy and nonconvergence; zero model mutation and no fallback in each failure. Negative library may be emitted from a test target built from the same exact test source under a test-only define.
- **GREEN native:** exact original immutable 0.8.0 source, MSVC x64 DLL with two inspected exported symbols; exact Line Horizontal/Vertical/Coincident(semantic Start/End)/Parallel/Perpendicular/Fix-masked cases and residual/DoF diagnostics; independent user-rebuilt ABI-compatible DLL in a clean **test directory** (not falsely claimed as installed product), cold restart and replacement load.
- **CI:** fail a required E1 native job if source/dependencies absent rather than silently report a skipped PASS. Separate real Core, kernel OCCT, Qt Desktop tiers remain required for A2 product readiness; E1 native proof does not substitute them.
- **Verification still open after E1:** actual installed product, DLL replacement permissions for users, LGPL-2.1-or-later notices/full license/source/patch/relink and signed/reproducible release artifacts, original six relation gestures in real 3D viewport, strict v22 and Driving dimensions, DOC-01 PL/EN+internal+generated Browser, independent Technical FINAL and Owner manual PASS.

**Proposed Owner decision for E1 (not yet granted):** accept the exact ABI v1 declarations in §7.1–7.2 and the seven paths in §7.3 strictly for offline-sourced, OFF-by-default Windows experimental DLL build/native tests; approve separately justified workflow extension for the one exact native test lane. **Exclude** shipping, installer, legal distribution signoff, v22 and native annotation API until their own D2/D3 freezes. Acceptance of this specific proposal should be recorded in its own Owner review and *not* inferred from “akceptuje - kontynuuj” against the earlier, less specific draft.


## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This is an unaccepted governance/design proposal only; no current implementation or user workflow changes. A future approved production backend needs its own as-built and release documentation impact and DOC-01 verification.
