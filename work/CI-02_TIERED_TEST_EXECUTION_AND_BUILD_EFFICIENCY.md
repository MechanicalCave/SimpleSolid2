# CI-02 — Tiered Test Execution and Build Efficiency

**Status:** ACCEPTED — ACTIVE  
**Owner acceptance:** 2026-09-26  
**Decision class:** D1 repository workflow / build implementation  
**Foundation:** 1.0 (`foundation-v1.0`)

## 1. Context

The current exact-head CI-01 gate correctly distinguishes FULL, DOCS and CLOSURE revisions, but every runtime-affecting implementation revision classified FULL still performs a complete native build and the complete CTest suite.

That policy is safe but does not scale with repository growth. The current CTest suite already includes a long native stress test and several UI executables that compile the same production UI translation units independently.

The Owner accepted a bounded optimization on 2026-09-26: preserve a mandatory complete regression before merge while making ordinary implementation iterations cheaper.

## 2. Goal

Introduce three test-execution levels without weakening final merge evidence:

- FAST — broad short-running regression for ordinary draft implementation iterations;
- SUBSYSTEM — explicit local/checkpoint regression for one or more named subsystems;
- FULL — complete CTest regression and clean exact-head merge evidence.

The existing CI-01 DOCS/CLOSURE exact-head model remains intact.

## 3. Required behavior

### 3.1 FAST

FAST is a stable CTest label-based suite.

It should include all deterministic tests that are appropriate for frequent execution and exclude tests intentionally classified slow/stress/full-only.

Draft PR runtime source changes may use FAST as their automatic Windows gate when the verification infrastructure itself is unchanged.

FAST is not sufficient merge evidence.

### 3.2 SUBSYSTEM

SUBSYSTEM selects tests by stable subsystem labels such as:

- core;
- application;
- persistence;
- part;
- sketch;
- viewer;
- ui;
- project.

Tests may carry multiple subsystem labels.

Subsystem selection is an explicit developer/checkpoint command. It is not a substitute for final FULL.

### 3.3 FULL

FULL remains the complete unfiltered CTest suite.

A PR that is ready for review and contains runtime-affecting changes must obtain an exact-head FULL result before merge. A later runtime-affecting commit invalidates that evidence and requires FULL again.

The existing trusted-FULL-ancestor + exact-head DOCS/CLOSURE suffix mechanism remains valid.

### 3.4 Draft/ready CI policy

For pull requests:

- work-only suffixes continue to use CLOSURE when authorized by CI-01 rules;
- docs/governance-only suffixes continue to use DOCS when authorized by CI-01 rules;
- draft PR source/runtime changes may use FAST;
- ready-for-review source/runtime changes require FULL;
- changes to verification infrastructure itself fail closed to FULL even while draft.

Verification infrastructure includes tests, CMake/build scripts, root dispatchers and the Windows gate workflow.

Non-PR/manual workflow execution remains FULL by default.

## 4. Canonical local commands

The root `ss2.ps1 test` command keeps FULL as its default for backwards compatibility.

The bounded interface must support commands equivalent to:

```powershell
.\ss2.ps1 test -Tier fast
.\ss2.ps1 test -Tier subsystem -Subsystem sketch
.\ss2.ps1 test -Tier subsystem -Subsystem sketch,viewer,ui
.\ss2.ps1 test -Tier full
```

The test command may also expose a bounded no-build option for CI so an immediately preceding successful Build step is not repeated.

Invalid tier/subsystem input fails closed.

## 5. Build efficiency

The implementation should remove avoidable duplicate work without changing CAD ownership:

- the CI Test step after an explicit successful Build must not invoke a second full build;
- production UI translation units currently compiled independently into many test executables should be centralized into a reusable production UI library where practical;
- tests should link the production UI library rather than compile duplicate copies of production UI sources;
- the application executable should use the same production UI library.

This is a build/link organization change only. It must not create a second UI semantic authority or alter runtime product behavior.

## 6. CTest labels

Labels are repository verification metadata, not product semantics.

Required stable label classes:

- `tier-fast`;
- `tier-full-only` for intentionally excluded FAST tests;
- `subsystem-<name>`;
- bounded descriptive labels such as `native` or `stress` where useful.

Every registered test must remain part of unfiltered FULL.

No test may be deleted, weakened or silently skipped in FULL to obtain performance.

## 7. CI-01 compatibility

CI-02 extends CI-01; it does not discard its exact-head guarantees.

The classifier remains fail closed.

A runtime source revision that only passed FAST cannot authorize a later DOCS/CLOSURE merge suffix. Only a successful exact-head `windows-msvc-full` job can become the trusted FULL ancestor used by CI-01.

The stable final `windows-msvc` summary check remains the selected-tier result.

## 8. Automated verification

At minimum prove:

1. root test dispatcher defaults to FULL;
2. FAST runs only tests labeled `tier-fast`;
3. SUBSYSTEM rejects missing/unknown subsystem values;
4. SUBSYSTEM can select one or multiple subsystem labels;
5. FULL remains unfiltered;
6. CI can run Test with no redundant build after a successful explicit Build;
7. draft runtime source diff classifies FAST;
8. ready runtime source diff classifies FULL;
9. draft verification-infrastructure diff classifies FULL;
10. DOCS/CLOSURE behavior remains unchanged;
11. only successful FULL evidence can authorize trusted-ancestor DOCS/CLOSURE classification;
12. production UI sources are compiled through one reusable production UI target rather than repeated directly in UI test executables;
13. all existing tests still pass in FULL;
14. exact-head Windows FULL passes for the CI-02 implementation candidate;
15. documentation verification and generated Product Browser freshness pass.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: CI-02 changes repository build/test/verification workflow and maintainer commands, not product behavior.

## 10. Expected implementation surface

Bounded changes may touch:

- `scripts/ss2-test.ps1`;
- `scripts/ci/ss2-pr-gate-classifier.ps1`;
- `ss2.ps1`;
- `.github/workflows/windows-pr-gate.yml`;
- `CMakeLists.txt`, `src/CMakeLists.txt`, `tests/CMakeLists.txt`;
- build/test-focused tests or self-tests;
- `docs/internal/BUILD_AND_TEST.md`;
- generated `docs/browser/index.html`;
- `work/**`.

No CAD/domain semantic implementation is authorized by CI-02.

## 11. Deliberately out of scope

- changing compiler/toolchain versions;
- distributed/remote build farms;
- compiler cache deployment such as sccache/ccache;
- test sharding across multiple Windows workers;
- speculative C++ dependency-graph analysis;
- weakening native/provider regression coverage;
- product/CAD behavior changes;
- R6+ Sketcher implementation.

## 12. Completion boundary

Completion requires:

- accepted tier semantics are implemented;
- FULL remains complete and mandatory before merge;
- CI-01 trusted-FULL-ancestor safety remains intact;
- duplicate Test-step build is removed;
- duplicated production UI compilation is materially reduced;
- exact-head Windows FULL passes;
- required internal documentation and Product Browser are current;
- closeout CLOSURE passes.

R6+ remains inactive and requires its own separate accepted Work Contract.
