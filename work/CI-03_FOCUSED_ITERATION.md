# CI-03 — Focused Iteration

**Status:** ACCEPTED — ACTIVE  
**Owner acceptance:** 2026-09-29  
**Decision class:** D1 repository workflow / build implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** CI-01, CI-02, Sketcher Roadmap v1.4

## 1. Context

CI-01 and CI-02 preserve strong exact-head verification and introduced FAST, SUBSYSTEM, FULL, DOCS and CLOSURE. The current draft FAST path still performs a full desktop build before running the short regression set.

A cost audit against the accepted Package F candidate showed that the Windows FULL job spent about 222 seconds in the desktop build, about 57 seconds in the separate core-only build/test and about 117 seconds in the 77-test desktop suite. Of the desktop test time, the single FULL-only native Workbench stress test accounted for about 105 seconds. The FAST-labelled tests themselves were short, but FAST still paid for the whole desktop build.

The same audit showed that many Sketch semantic changes can be compiled and proven through a single concrete test target and its dependency closure. On the measured cold build, `simplesolid2_sketch` plus a small Sketch test was on the order of seconds rather than minutes.

The Owner accepted a bounded optimization on 2026-09-29: add a FOCUSED iteration path that builds only explicitly requested CMake test targets and runs only explicitly requested CTest tests, while preserving FULL as mandatory runtime merge evidence.

## 2. Goal

Add a small developer/AI iteration mechanism:

```text
edit
  -> FOCUSED (explicit target(s) + exact test(s))
  -> optional FAST/SUBSYSTEM checkpoint
  -> FULL merge candidate
```

FOCUSED exists to answer only: "does the smallest explicitly identified affected slice compile and pass its direct regression evidence?"

It does not answer whether the complete product is merge-safe.

## 3. Required behavior

### 3.1 Local focused command

The canonical root dispatcher shall expose a bounded command equivalent to:

```powershell
.\ss2.ps1 check -Target <cmake-target>[,<target>...] -Test <ctest-name>[,<test>...]
```

The command shall:

1. configure the normal development build if required;
2. build only the requested CMake target or targets; CMake remains responsible for their dependency closure;
3. run only the requested exact CTest test names;
4. fail if a requested target cannot be built;
5. fail if no requested CTest name is registered;
6. reject empty, malformed or unbounded input;
7. print the selected targets/tests clearly.

The implementation may use a dedicated `scripts/ss2-check.ps1` dispatcher.

### 3.2 Exact test selection

FOCUSED shall use exact requested CTest names, not a broad substring match.

Multiple requested tests are allowed so one focused request can represent a small affected set.

FOCUSED does not depend on `tier-fast` labels. A newly added or FULL-only test may be run explicitly when appropriate.

### 3.3 Draft PR focused gate

A draft PR runtime revision may request FOCUSED verification using bounded machine-readable metadata on the HEAD commit.

The initial accepted form is commit trailers:

```text
SS2-Focus-Target: sk06a_circle_arc_interaction_state_test
SS2-Focus-Test: sk06a.circle_arc_interaction_state
```

Multiple trailers of each kind are allowed.

The focused gate shall parse only the exact HEAD commit. Missing or invalid focused metadata must not guess.

For ordinary draft runtime/test-content changes:

- valid focused metadata -> FOCUSED;
- no focused metadata -> existing FAST behavior.

Ready-for-review runtime changes remain FULL.

### 3.4 Test content versus verification infrastructure

CI-03 distinguishes ordinary product regression source from the machinery that defines verification policy.

Ordinary `tests/*_test.cpp` source is test content. Editing an existing product regression source shall not by itself force FULL on a draft PR.

Verification infrastructure remains FULL-sensitive, including at minimum:

- `tests/CMakeLists.txt`;
- test boundary/check scripts;
- `scripts/**`;
- root build/test dispatchers;
- any `CMakeLists.txt`;
- `.github/workflows/**`.

A bounded declarative registration mechanism for future tests may be introduced only if it cannot weaken FULL membership or silently change tier policy. If that cannot be demonstrated simply, CI-03 shall leave new-test registration FULL-sensitive rather than create a speculative framework.

### 3.5 FOCUSED is never trusted merge evidence

FOCUSED is iteration evidence only.

It must never:

- become a trusted FULL ancestor;
- authorize DOCS/CLOSURE suffix reuse;
- replace FULL on a ready runtime PR;
- remove any test from unfiltered FULL;
- weaken the existing `windows-msvc` final summary requirement.

Only a successful `windows-msvc-full` job remains trusted runtime merge evidence.

### 3.6 Failure and escalation

The system fails closed.

Examples:

- malformed target/test metadata -> FAST or FULL, not silent success;
- verification-infrastructure change -> FULL;
- ready-for-review runtime revision -> FULL;
- non-PR/manual workflow dispatch -> FULL;
- focused target build failure -> focused gate failure;
- requested test not registered -> focused gate failure.

FOCUSED may be intentionally broader by listing several targets/tests. This is the bounded AFFECTED use case; no separate AFFECTED gate is required in CI-03.

## 4. Verification footprint doctrine

Future Work Contracts may contain a short non-normative execution aid:

```text
Verification footprint
Primary affected:
Crossed boundaries:
Focused evidence:
Checkpoint:
Final: FULL
```

This does not transfer product authority to CI. It records the smallest expected verification path so implementation can iterate cheaply and escalate when a boundary is crossed.

Public headers, cross-layer contracts and broader dependency changes require the implementer to expand the focused target/test set or use FAST/SUBSYSTEM. FOCUSED is not an excuse to under-test a known affected boundary.

## 5. CI-01 / CI-02 compatibility

CI-03 extends CI-02; it does not redefine existing tiers.

Stable meanings remain:

- FAST: broad short regression for draft iterations;
- SUBSYSTEM: explicit subsystem checkpoint;
- FULL: complete unfiltered runtime merge evidence;
- DOCS/CLOSURE: exact-head suffix verification backed by trusted FULL evidence.

The stable final check remains `windows-msvc`.

## 6. Automated verification

At minimum prove:

1. the focused dispatcher rejects missing target/test arguments;
2. malformed focused values are rejected;
3. one exact test target can be built without requesting the whole default build;
4. one exact CTest name is executed;
5. multiple target/test values normalize deterministically;
6. an unknown test name fails;
7. draft runtime source + valid trailers selects FOCUSED;
8. draft runtime source without trailers selects FAST;
9. ordinary existing `*_test.cpp` content may use FOCUSED/FAST;
10. `tests/CMakeLists.txt`, scripts, root dispatchers and workflow changes select FULL;
11. ready runtime changes select FULL regardless of focus trailers;
12. FOCUSED cannot become trusted FULL evidence;
13. FAST/SUBSYSTEM/FULL behavior remains unchanged;
14. unfiltered FULL still runs every registered test;
15. exact-head Windows FULL passes for the CI-03 implementation candidate;
16. documentation verification and generated Product Browser are current.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: CI-03 changes repository build/test/verification workflow and maintainer/automation commands, not product behavior.

## 8. Expected implementation surface

Bounded changes may touch:

- `scripts/ss2-check.ps1`;
- `scripts/ss2-test.ps1` only if exact-test support is cleanly shared there;
- `scripts/ss2-build.ps1` only if targeted-build support is cleanly shared there;
- `scripts/ci/ss2-pr-gate-classifier.ps1`;
- a bounded focused-metadata parser under `scripts/ci/`;
- `ss2.ps1`;
- `.github/workflows/windows-pr-gate.yml`;
- `tests/CMakeLists.txt` and build/test self-tests only as required to prove CI-03;
- `docs/internal/BUILD_AND_TEST.md`;
- generated `docs/browser/index.html`;
- `work/**`.

No CAD/domain source under `src/**` is authorized for mutation by CI-03.

## 9. Deliberately out of scope

- CAD/Sketch/Part product behavior;
- changing persistence, identity, ownership or public CAD contracts;
- compiler/toolchain upgrades;
- compiler cache deployment;
- distributed build farms;
- test sharding/parallel Windows workers;
- splitting `simplesolid2_ui` or other production targets solely for build speed;
- speculative dependency-graph inference from arbitrary C++ source;
- automatic selection of semantic tests by AI without explicit bounded evidence;
- weakening/removing FULL-only native stress coverage.

## 10. Completion boundary

CI-03 is complete only when:

- the focused local command works on representative Sketch semantic and cross-layer test targets;
- draft focused classification is fail-closed;
- ordinary test content no longer forces FULL solely because it is under `tests/**`;
- verification infrastructure remains FULL-sensitive;
- exact-head Windows FULL passes on the final implementation/documentation candidate;
- internal build/test documentation and Product Browser are current;
- completion bookkeeping passes CLOSURE;
- no CAD/product semantics changed.

After completion, feature development still requires its own accepted Work Contract. CI-03 changes only how approved implementation work is verified during iteration.
