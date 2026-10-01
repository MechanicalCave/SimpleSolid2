# CI-04 — Build Graph & Warm Verification

**Status:** COMPLETED  
**Owner proposal acceptance:** 2026-09-30  
**Owner activation:** 2026-10-01  
**Decision class:** D1 repository workflow / build implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** CI-01, CI-02, CI-03, R11 Object Snap / Tracking / Inference proposal

## 1. Context

CI-01 through CI-03 preserve strong exact-head verification and provide DOCS/CLOSURE, FAST/SUBSYSTEM/FULL and draft-only FOCUSED execution. The remaining cost is now dominated by build-graph shape and repeated cold/native work:

- the normal `ss2 build` path currently builds the default CMake graph rather than only the product;
- desktop tests are part of the configured graph and ordinary product iteration can therefore compile more test executables than required;
- FAST filters CTest execution but the preceding build is still broader than the FAST test set;
- self-hosted Windows jobs use clean exact-SHA checkouts, while build directories currently live inside the checkout and cannot provide durable incremental reuse across jobs;
- CTest execution is mostly serial even where tests are independent.

R11 is expected to add substantial semantic and integration regression coverage. CI-04 is intended to improve iteration cost before R11 production implementation without weakening verification evidence.

## 2. Goal

Create a predictable build/test graph in which:

```text
product iteration
  -> build only SimpleSolid2 + dependency closure

focused verification
  -> build only requested test target(s) + dependency closure

FAST/SUBSYSTEM
  -> build only the selected test graph

FULL
  -> build every registered desktop test

self-hosted CI
  -> reuse a valid external build tree when safe
  -> retain a clean exact-SHA source checkout
```

CI-04 is infrastructure-only. It does not activate R11 and does not authorize any CAD/product semantic mutation.

## 3. Evidence invariants

CI-04 MUST preserve:

- exact-SHA verification;
- the fail-closed PR classifier;
- FOCUSED as iteration evidence only;
- FAST as iteration evidence only;
- FULL as mandatory runtime merge evidence;
- DOCS/CLOSURE exact-head behavior;
- trusted-FULL-ancestor semantics;
- `-NoBuild` only after an explicit successful build of the required target set;
- the stable final `windows-msvc` summary check.

FOCUSED and FAST MUST NOT become trusted merge evidence.

## 4. Product build semantics

The canonical command:

```powershell
.\ss2.ps1 build
```

MUST build the `simplesolid2` product target and its dependency closure rather than relying on the raw default CMake ALL graph.

`ss2 run` MUST perform:

```text
configure
-> incremental build simplesolid2
-> run SimpleSolid2.exe
```

and MUST NOT require the ordinary test executable graph to be built first.

A raw/diagnostic ALL build may remain available explicitly, but it is not the normal product-development path.

## 5. Test graph separation

The normal test directory/targets MUST be excluded from the default product ALL build while remaining directly buildable.

CI-04 MUST provide aggregate build targets corresponding at minimum to:

- FAST;
- FULL;
- subsystem-core;
- subsystem-application;
- subsystem-persistence;
- subsystem-part;
- subsystem-sketch;
- subsystem-viewer;
- subsystem-ui;
- subsystem-project.

A directly requested test target MUST remain buildable with its normal dependency closure.

The existing core-only configuration remains a separate graph using:

```text
SS2_BUILD_DESKTOP=OFF
```

and MUST continue to prove that Qt/OpenCASCADE are not discovered or required.

## 6. Single test metadata registry

Tier/subsystem execution metadata and aggregate build membership MUST derive from one authoritative registration source.

For each registered test the metadata model MUST identify at least:

- CMake target;
- CTest test name;
- tier membership;
- subsystem membership;
- core-only eligibility where applicable;
- resource/serialization requirement where applicable.

The same registration MUST drive both:

```text
CTest labels
and
aggregate CMake build targets
```

Independent manually maintained tier-build and tier-test lists that can silently diverge are not acceptable.

Existing tests may be migrated incrementally if required; new R11 tests SHOULD use the unified registration model from creation.

## 7. Registry verification

Verification infrastructure MUST fail closed when it detects inconsistencies including:

- registered CTest without a corresponding build target;
- registered build test target without its expected CTest;
- FAST label without FAST aggregate membership;
- subsystem label without matching subsystem aggregate membership;
- registered test missing from FULL aggregate;
- unknown or malformed metadata;
- an ordinary registered desktop test unintentionally omitted from complete FULL execution.

## 8. FOCUSED

The accepted CI-03 command remains:

```powershell
.\ss2.ps1 check -Target <cmake-target>[,<target>...] -Test <ctest-name>[,<test>...]
```

FOCUSED MUST continue to:

1. configure the selected development build tree;
2. build only the explicitly requested target set plus dependency closure;
3. verify that every requested CTest name exists;
4. run only the exact requested tests.

FOCUSED MUST NOT require FAST or FULL to have been built first.

## 9. FAST

FAST MUST perform:

```text
configure
-> build FAST aggregate
-> run tier-fast
```

FULL-only/native-stress test executables MUST NOT be built only because FAST was requested, unless they are genuine dependencies of a required production target.

## 10. SUBSYSTEM

SUBSYSTEM MUST build the union of aggregate targets corresponding to the selected subsystem labels and then execute the matching CTest selection.

For example:

```text
sketch,viewer,ui
```

must not automatically build Project/Persistence/FULL-only native stress test targets unless required by the actual dependency graph.

## 11. FULL

FULL remains complete desktop regression evidence:

```text
configure
-> build every registered desktop test target
-> run the complete unfiltered desktop CTest suite
```

FULL may use a valid warm build tree.

## 12. CLEAN FULL

CLEAN FULL is not a weaker or stronger evidence tier. It is FULL with an intentionally empty build tree before configure.

Conceptually:

```text
FULL + clean_build=true
```

CLEAN FULL MUST be required for changes that can affect correctness of the build/verification graph, including relevant changes to:

- CMake build/test infrastructure;
- test registry/aggregation;
- configure/build/test/check scripts;
- compiler/generator assumptions;
- Qt/OCCT dependency discovery;
- external build-tree management;
- fingerprint/invalidation logic;
- relevant GitHub Actions build workflow.

Ordinary `.cpp`, `.hpp` and product regression `*_test.cpp` changes do not require CLEAN FULL solely because they alter source code.

## 13. Persistent self-hosted CI build trees

The self-hosted Windows gate SHOULD use persistent build trees outside the GitHub checkout.

Representative structure:

```text
<SS2 CI build root>\
  desktop-debug\
  core-release\
```

The source checkout MUST remain clean and exact-SHA. Source checkout cleanliness MUST NOT be weakened in order to preserve build artifacts.

Each build tree is writable by at most one active job at a time.

## 14. Build fingerprint

Every reusable external build tree MUST have an explicit compatibility fingerprint.

The fingerprint MUST cover at least:

- normalized source root;
- generator;
- architecture;
- compiler identity/version;
- CMake version;
- Qt identity/prefix where relevant;
- OCCT identity/prefix where relevant;
- desktop/core-only mode;
- configuration model;
- relevant build options.

The fingerprint MUST NOT include:

- Git SHA;
- branch;
- PR number;
- ordinary source timestamps.

Fingerprint mismatch MUST invalidate the affected build tree before configure.

The log MUST identify whether the build was:

```text
cold
warm
invalidated
forced-clean
```

and, for invalidation, the reason.

## 15. Force-clean escape hatch

A simple maintainer/CI mechanism MUST be able to ignore the warm tree and force a clean configure/build.

This exists to distinguish:

```text
source/test defect
from
stale/corrupt build-tree defect
```

without manual runner cleanup.

## 16. Parallelism

Build parallelism and test parallelism MUST be independently configurable.

Representative environment controls may be:

```text
SS2_BUILD_PARALLELISM
SS2_TEST_PARALLELISM
```

CTest MAY run independent tests concurrently.

Tests using shared native/viewer/global resources MUST use explicit resource locking or bounded serialization. Parallelism MUST NOT be increased at the cost of flaky verification.

Initial parallelism values are tuning parameters, not product requirements.

## 17. Timing evidence

Before CI-04 changes the build graph, baseline timings SHOULD be recorded on one exact SHA.

Final evidence MUST provide comparable before/after measurements for at least:

- clean product build;
- no-change product rebuild;
- single ordinary `.cpp` incremental product rebuild;
- representative FOCUSED check;
- FAST;
- FULL;
- core-only FULL.

Where practical logs SHOULD separate:

```text
configure
build
test
total
cache state
```

CI-04 does not set an arbitrary required percentage speedup. The purpose is measured reduction of avoidable work, especially in frequent iteration paths.

## 18. Safe rollout

Implementation SHOULD be staged so each step is independently verifiable:

1. baseline timing;
2. unified test metadata/registry;
3. aggregate test build targets;
4. targeted product/test dispatch;
5. exclude tests from normal ALL;
6. CLEAN FULL and registry verification;
7. persistent fingerprinted CI build trees;
8. bounded parallel CTest;
9. final clean/warm comparison and documentation.

Warm build-tree reuse SHOULD NOT be introduced in the same first implementation change that rewrites the test build graph, so regressions can be localized.

## 19. R11 preparation

Before R11 production implementation begins, the repository SHOULD provide stable test targets for the major R11 semantic/integration slices so ordinary R11 changes do not repeatedly modify verification infrastructure.

Most R11 semantics SHOULD be testable through the core-only graph where Qt/OCCT are not semantically required.

Desktop/provider tests should be reserved for viewport projection, controller/UI/presentation and native integration boundaries.

## 20. Deliberately out of scope

CI-04 does not authorize:

- Ninja generator migration;
- sccache/ccache deployment;
- precompiled-header rollout;
- Unity Build;
- compiler/toolchain upgrades;
- compiler optimization experiments;
- test weakening/removal;
- CAD/product semantic changes;
- R11 OSNAP implementation.

Those optimizations may be considered later using CI-04 timing evidence.

## 21. Automated verification / acceptance

At minimum prove on the final CI-04 candidate:

1. `ss2 build` builds the product and does not build ordinary test executables;
2. `ss2 run` does not require the complete test graph;
3. FOCUSED builds only the requested target dependency closure and runs exact requested tests;
4. FAST builds the FAST aggregate and runs `tier-fast`;
5. SUBSYSTEM builds only the requested subsystem aggregate union plus real dependencies;
6. FULL builds and runs every registered desktop test;
7. core-only still passes with Qt/OpenCASCADE package discovery disabled;
8. registry self-tests detect inconsistent metadata;
9. a valid warm no-change build is a no-op or minimal rebuild;
10. force-clean reconstructs a usable build tree;
11. fingerprint mismatch invalidates the build tree deterministically;
12. warm FULL and CLEAN FULL on the same exact SHA produce the same test outcome;
13. native/resource-sensitive tests remain stable under bounded parallel CTest;
14. timing evidence is recorded;
15. exact-head Windows CLEAN FULL passes for the final CI-04 implementation/documentation candidate;
16. required internal documentation and generated Product Browser are current;
17. no CAD/product semantics changed.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: CI-04 changes repository build/test/verification workflow and maintainer commands, not application behavior.

## 23. Expected implementation surface after activation

Bounded implementation may touch:

- root and test CMake files/helpers;
- `scripts/ss2-build.ps1`;
- `scripts/ss2-configure.ps1`;
- `scripts/ss2-test.ps1`;
- `scripts/ss2-check.ps1`;
- shared build helpers under `scripts/`;
- bounded CI helpers under `scripts/ci/`;
- `ss2.ps1`;
- `.github/workflows/windows-pr-gate.yml`;
- build/test infrastructure self-tests;
- `docs/internal/BUILD_AND_TEST.md`;
- generated `docs/browser/index.html`;
- `work/**`.

No CAD/domain source under `src/**` is authorized for semantic mutation by CI-04.

## 24. Activation boundary

The Owner explicitly activated CI-04 on 2026-10-01.

CI-04 implementation is authorized only within this accepted D1 repository/build contract. R11 remains separately proposal-only/inactive and no CAD/product semantic mutation is authorized by CI-04.

Implementation must preserve CI-01/CI-02/CI-03 evidence invariants while proceeding through the staged rollout defined above.

## 25. Implementation evidence

CI-04 implementation candidate evidence was collected on 2026-10-01.

Exact timing/clean verification head:

```text
79e89af2faba96780dab9e85b9f60700a75015c7
Windows FULL #1040: PASS
```

#1040 proved:

- documentation/bootstrap/self-tests PASS;
- test-tier dispatcher PASS;
- corrected FOCUSED dispatcher self-test PASS;
- build-tree manager self-test PASS;
- CLEAN persistent desktop tree preparation PASS;
- complete desktop test graph build PASS;
- core-only build/test with Qt/OpenCASCADE discovery disabled PASS;
- FAST/SUBSYSTEM selector verification PASS;
- full desktop CTest 78/78 PASS;
- bounded parallel CTest with native resource locking PASS;
- comparative timing harness PASS.

Comparative results on runner `DESKTOP-JL6O311`, baseline `8574215c3c73ae015e49ad8a749464c92d81548b` versus exact head above:

| Scenario | Before | After | Speedup |
|---|---:|---:|---:|
| clean product | 229.99 s | 78.19 s | 2.94x |
| no-op product | 6.66 s | 3.88 s | 1.72x |
| one ordinary Sketch `.cpp` | 11.53 s | 4.16 s | 2.77x |
| clean FOCUSED | 12.27 s | 12.16 s | 1.01x |
| clean FAST | 242.32 s | 205.82 s | 1.18x |

Both FOCUSED timing cells executed exactly one test. Both FAST cells executed 62 tests. Timing is evidence only and is not a machine-independent acceptance threshold.

Final parity/closure evidence:

```text
fbc35c3d9efd96891be8fec5671654cad8ecd473
Windows FULL #1044: PASS
classifier: full
clean FULL: true
CLEAN desktop CTest: 78/78 PASS (111.38 s)
warm ss2_tests_full rebuild: 7.60 s
warm desktop CTest: 78/78 PASS (110.31 s)
final windows-msvc summary: PASS
```

The CLEAN and warm verification passes used the same exact SHA and produced the same complete desktop test outcome. CI-04 acceptance is therefore satisfied.

## 26. Completion

CI-04 is completed on 2026-10-01.

Delivered:

- normal `ss2 build` / `ss2 run` product-targeted compilation;
- desktop tests excluded from normal product ALL;
- FAST/FULL/SUBSYSTEM aggregate build graph derived from authoritative test membership;
- exact FOCUSED target/test execution, including correction of the CTest exact-match regex;
- fingerprinted persistent external self-hosted Windows build trees with force-clean escape hatch;
- CLEAN FULL policy for verification-infrastructure changes;
- bounded parallel CTest with native Viewer resource serialization;
- comparative before/after timing evidence;
- same-SHA CLEAN FULL / warm FULL parity evidence;
- current internal build/test documentation and generated Product Browser.

Deferred/out of scope by design: Ninja migration, sccache, PCH, Unity Build and further compiler/toolchain optimization. Any such optimization requires a separate measured decision.

R11 remains separately proposal-only/inactive and was not implemented by CI-04.
