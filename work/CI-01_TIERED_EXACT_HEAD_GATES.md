# CI-01 — Tiered Exact-Head Verification

**Status:** ACCEPTED — COMPLETED  
**Owner acceptance:** 2026-09-25  
**Decision class:** D1 repository workflow / governance implementation  
**Foundation:** 1.0 (`foundation-v1.0`)

## Goal

Preserve exact-revision verification while avoiding repeated full native builds when the only changes after a green implementation gate are documentation or completion bookkeeping.

The accepted verification model has three tiers:

```text
FULL
  runtime/build/test-affecting changes
  → docs + setup + verify + build + full CTest

DOCS
  documentation/governance-only changes
  → exact checkout + docs generation/freshness + docs validation
  → no CAD build / CTest

CLOSURE
  work-contract / roadmap / ACTIVE bookkeeping only
  → exact checkout + governance/docs validation
  → no CAD build / CTest
```

## In scope

- replace the single unconditional Windows PR gate with fail-closed tier classification;
- keep one stable final required check name;
- detect the nearest exact ancestor in the same PR lineage that already passed the FULL job;
- classify only the suffix after that trusted FULL SHA;
- if no trusted FULL ancestor exists, classify the complete PR diff against its base;
- make any runtime/build/test/workflow/script change require FULL;
- allow documentation/governance-only changes to use DOCS;
- allow `work/**` bookkeeping-only suffixes to use CLOSURE;
- remove PR-description `edited` events as a gate trigger;
- add classifier self-tests for representative path sets;
- document the current gate policy.

## Failure behavior

Classification fails closed.

If prior FULL evidence cannot be queried/verified, or a changed path does not belong to the narrow DOCS/CLOSURE classes, the selected mode is FULL.

A CLOSURE or DOCS suffix after production changes is valid only when an exact ancestor SHA has a successful `windows-msvc-full` job.

No commit message, PR description, label or human assertion can substitute for that evidence.

## Path policy

CLOSURE-only:

```text
work/**
```

DOCS-or-CLOSURE:

```text
work/**
docs/**
governance/**
README.md
AGENTS.md
```

Everything else that triggers the workflow is FULL, including:

```text
src/**
tests/**
scripts/**
CMakeLists.txt
ss2.ps1
ss2.cmd
.github/workflows/**
```

## Exact-head rule

The final merge HEAD is always verified.

The evidence may be:

```text
A. FULL on the final HEAD
```

or:

```text
B. successful FULL on an exact ancestor
   + final suffix proven DOCS/CLOSURE-only
   + successful exact-head DOCS/CLOSURE gate
```

Thus runtime code is always covered by FULL verification while non-runtime closure commits do not force recompilation.

## Acceptance

1. Runtime/source/test/build/workflow change selects FULL.
2. Pure docs/governance diff selects DOCS.
3. Pure `work/**` diff selects CLOSURE.
4. After a green FULL ancestor, an appended `work/**` completion commit selects CLOSURE.
5. Runtime change after the green FULL ancestor selects FULL again.
6. Missing/unavailable prior FULL evidence cannot authorize DOCS/CLOSURE over an otherwise runtime-changing PR.
7. FULL performs the existing docs/setup/verify/build/full-test sequence.
8. DOCS validates generated Browser freshness and canonical documentation without build/CTest.
9. CLOSURE validates repository documentation/governance invariants without build/CTest.
10. A stable final `windows-msvc` summary check reports the selected tier result.
11. Editing PR title/body does not cancel/restart a running gate.
12. This work item proves the mechanism by passing FULL on the implementation head and then CLOSURE on a bookkeeping-only completion head.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: this changes repository build/test/verification workflow, not product behavior.

## Out of scope

- changing CAD/domain semantics;
- weakening production tests;
- test sharding/parallelization;
- incremental compilation/cache policy;
- changing compiler/toolchain;
- remote build farm design.

## Completion record

CI-01 implementation is complete.

- implementation head `d7aa0ae06046d2ac22831b2f4765b56dedac22c3` passed Windows PR gate #261 in FULL mode;
- classifier self-test passed and the implementation head was correctly classified FULL because workflow/scripts changed;
- this completion commit changes only `work/**` and is intentionally used to prove the CLOSURE tier;
- final merge is allowed only if the exact completion HEAD passes the stable `windows-msvc` summary check in CLOSURE mode without Build/CTest.

CI-01 does not authorize R3 implementation. R3 still requires a separate explicit Owner-accepted Work Contract.
