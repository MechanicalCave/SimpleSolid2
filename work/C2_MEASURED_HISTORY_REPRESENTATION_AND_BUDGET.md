# C2 — Measured History Representation and Budget Decision

**Status:** ACCEPTED — ACTIVE (PHASE A COMPLETE / SECOND OWNER DECISION PENDING)  
**Proposed:** 2026-09-27  
**Owner acceptance:** 2026-09-27  
**Decision class:** D1 measurement infrastructure first; any deeper history architecture or product-visible history budget is a later explicit Owner decision  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package C2  
**Baseline:** `main` at `7d858d61b14eda6923bbfd3c7999b465d44f1dc5` after completed C1

## 1. Goal

Measure the real Release cost of the current C1 history model before choosing any deeper Undo/Redo representation or memory/depth budget.

C2 starts from evidence, not from a preferred data structure.

The current baseline intentionally remains:

```text
DocumentSession
  history_: vector<HistoryEntry>
  HistoryEntry:
    before = PartAuthoredState
    after  = PartAuthoredState
```

C1 removed the history-depth-dependent copy of all prior entries when adding a command. C2 now measures the remaining costs of the two-snapshot model, command staging, transaction staging, Undo/Redo and branch creation.

## 2. Two-gate authority model

Initial C2 acceptance authorizes **measurement only**.

It may add benchmark-only build/tooling code and durable benchmark evidence, but it does not authorize changing the production history representation or introducing a product history limit.

After measurements, C2 stops at a second Owner decision gate.

Possible outcomes include:

- keep the current C1 two-snapshot representation because measured behavior is acceptable;
- approve a bounded internal representation change;
- approve a product-visible history memory/depth budget;
- request another experiment before deciding.

No option is preselected by this contract.

## 3. Scope IN — measurement phase

- a headless semantic history benchmark harness that exercises `DocumentSession` without Qt/Viewer/OCCT rendering;
- Release configuration measurements on the supported Windows/MSVC environment;
- 1,000-entity and 10,000-entity Part/Sketch scenarios where safely feasible;
- target history depths 10, 100 and 1,000 where safely feasible;
- semantic operations required by AUDIT-01:
  - add entity;
  - edit one entity;
  - multi-object transform;
  - Undo;
  - Redo;
  - create a new history branch;
- median, p95 and maximum operation latency;
- peak process working-set evidence per benchmark case/run;
- explicit environment/build/revision metadata;
- durable C2 measurement report in the repository;
- clear cutoff records for cases that cannot be prepared or executed safely;
- comparison against the completed C1 baseline only; rendering is excluded from semantic timings.

## 4. Scope OUT before the second Owner decision

- replacing `HistoryEntry{before, after}`;
- delta/event-sourced history;
- command replay history;
- structural sharing/copy-on-write production state;
- persistent Undo/Redo;
- user-visible Undo depth;
- automatic history eviction;
- a production memory cap;
- compression/deduplication policy;
- changing Part transaction semantics;
- changing B1 save/checkpoint behavior;
- changing EntityId lifetime/high-water rules;
- UI/Viewer timing claims;
- changing semantic commands merely to make a benchmark look better;
- third-party benchmark frameworks/dependencies.

Event sourcing is explicitly **not authorized** merely because Undo exists.

## 5. Benchmark harness boundary

The benchmark is evidence tooling, not product runtime behavior.

Preferred dependency shape:

```text
benchmark executable / runner
        ↓
DocumentSession semantic commands
        ↓
Part / Sketch authored model
```

The harness must not require:

- QWidget;
- Viewer scene construction;
- OCCT;
- input/picking adapters;
- filesystem Save for the timed semantic operation.

Setup may construct a valid Part/Sketch authored baseline directly through legal model/reconstruction APIs so that building 10,000 entities does not itself require 10,000 history entries.

A Windows-only process-memory probe is allowed inside benchmark tooling only. WinAPI/PSAPI measurement code must not enter Application, Part, Sketch or shared production contracts.

## 6. Build and execution model

The current repository already supports multi-config builds and `ss2-build.ps1 -Config Release`.

C2 may add a benchmark target behind an explicit opt-in CMake option, default OFF, so normal product/CTest builds do not execute the measurement matrix.

A bounded direction is:

```powershell
.\ss2.ps1 configure -BuildDir build\c2-release
.\ss2.ps1 build -BuildDir build\c2-release -Config Release
# explicit C2 benchmark runner/target
```

Exact runner naming is D1.

The benchmark must print or record at least:

- tested git SHA;
- Release configuration;
- compiler/toolchain identification available to the runner;
- machine/OS identification;
- entity count;
- requested/prepared history depth;
- operation;
- sample count;
- median;
- p95;
- maximum;
- peak working set;
- completion/cutoff status.

## 7. Scenario construction

### 7.1 Entity counts

Primary authored geometry scenarios:

- 1,000 entities;
- 10,000 entities.

Use deterministic valid Shared 2D geometry in one hosted Sketch unless evidence shows that multiple-Sketch ownership materially changes the history cost being measured.

Geometry generation is setup, not part of the timed command.

### 7.2 History depths

Target cursor/history depths:

- 10;
- 100;
- 1,000.

Scenario preparation must use ordinary semantic commands so the history representation under test is real.

If a requested depth cannot be prepared safely, record:

- last completed depth;
- entity count;
- observed peak memory;
- elapsed setup time if available;
- failure/cutoff reason.

Do not force the process into OS-level memory exhaustion merely to obtain a data point.

### 7.3 Benchmark-only safety ceiling

The runner may use an explicit **benchmark-run safety ceiling** for working set or elapsed setup time.

That ceiling:

- protects the measurement machine only;
- is recorded in the evidence;
- is not a Product/DocumentSession history limit;
- must not alter production code;
- must not be presented as the recommended product budget.

If the ceiling is reached, the matrix cell is recorded as a cutoff.

## 8. Timed semantic operations

### 8.1 Add entity

Time one accepted semantic entity-add command on a prepared scenario.

Between samples, restore a comparable cursor/state through untimed history movement or reconstruct the scenario so timed samples do not include setup.

Fresh EntityId/high-water rules remain authoritative.

### 8.2 Edit one object

Time one accepted geometry edit of one existing entity through the normal DocumentSession command path.

Use deterministic alternating target geometry so the command is never a no-op.

### 8.3 Multi-object transform

Primary case transforms a deterministic set of 100 existing entities, or all entities when the scenario contains fewer than 100.

This is intended to distinguish fixed history snapshot overhead from the direct cost of a non-trivial semantic edit.

An optional all-entity secondary case may be reported but must not replace the required primary case.

### 8.4 Undo / Redo

Measure one Undo and its matching Redo on an existing prepared history entry.

Report them separately.

### 8.5 New history branch

Prepare Redo by Undo, then time one accepted edit that replaces that Redo path.

The repeatable primary statistics may use one-step branch replacement to keep scenario depth stable across samples.

Where feasible, additionally record a half-depth branch case to expose the cost of destroying a long Redo suffix. Heavy half-depth cases may be recorded as cutoff rather than destabilizing the machine.

## 9. Sampling and statistics

Target at least 20 measured samples per feasible primary matrix cell after warm-up.

Report:

- sample count;
- median;
- p95;
- maximum.

If a heavy case cannot safely reach 20 samples, report the completed sample count and mark the percentile evidence as limited rather than fabricating precision.

Scenario preparation and cleanup time are not included in the semantic operation latency, but setup time may be reported separately because history construction itself can be material.

Do not merge UI/render/presentation time into these semantic timings.

## 10. Memory evidence

For each prepared entity-count/history-depth case, record peak process working set at minimum.

When available without new external dependencies, also record current/private process memory useful for diagnosis.

Memory data must be associated with:

- entity count;
- history depth;
- operation/sample phase;
- Release build SHA;
- measurement machine.

C2 must not infer a product memory budget from one machine without explicit Owner decision.

## 11. Reproducibility and benchmark correctness

Before using timings for a decision, the benchmark must prove that each measured operation has the expected semantic result.

Examples:

- add increases authored entity count and one Undo entry;
- edit preserves EntityId and changes expected geometry;
- transform changes the expected affected set atomically;
- Undo/Redo returns the expected authored states;
- new branch removes only abandoned Redo and keeps the accepted branch;
- no benchmark helper bypasses DocumentSession history for the timed operation.

The harness should consume results so Release optimization cannot eliminate the work.

Performance evidence is invalid if semantic checks fail.

## 12. Durable evidence

C2 shall add a current measurement record such as:

`work/C2_HISTORY_BENCHMARK_RESULTS.md`

The record must include:

- exact tested SHA;
- environment/toolchain;
- benchmark-run safety ceiling;
- matrix results;
- cutoff cells;
- observed bottlenecks;
- distinction between measured facts and interpretation.

Raw machine-readable output may be retained when useful, but the repository must contain enough summarized evidence to reproduce and audit the decision.

## 13. Second Owner decision gate

After the measurement report is complete, stop before production history redesign.

Present the Owner with evidence and bounded alternatives.

The decision record must explicitly answer:

1. Is deeper C2 history optimization required now?
2. If yes, which representation problem is being solved: latency, peak memory, both, or another measured constraint?
3. Does the selected approach change only private runtime representation, or any public/product behavior?
4. Is a history depth/memory budget required?
5. If a budget is required, what user-visible behavior occurs when it is reached?
6. What current Undo/Redo, dirty-state, identity and strong-consistency invariants must remain unchanged?

Any production implementation after this gate requires explicit Owner acceptance of the selected option/amended C2 contract.

A product-visible history budget is not an Agent-autonomous decision.

## 14. Candidate post-measurement directions

These are evaluation categories, not approved designs:

- **No deeper change** — keep C1 snapshots if measurements are acceptable.
- **Bounded private representation optimization** — for example reduced duplicate ownership where strong consistency and identity semantics remain provable.
- **History budget** — only with explicit Owner-approved product behavior and Product documentation.
- **Larger redesign** — delta/event/command-replay style representation only if measurements justify complexity and a new/updated architecture decision authorizes it.

Do not rank or implement these categories before measurement evidence.

## 15. Required regression boundary

Measurement tooling must not weaken or replace correctness tests.

During measurement-phase implementation:

- existing DocumentSession/Part/Sketch/persistence/UI tests remain unchanged in authority;
- benchmark target is not a substitute for CTest;
- default benchmark option OFF must leave normal build/test behavior unchanged;
- verification-infrastructure changes still require exact-head Windows FULL under current CI rules.

If the second gate later authorizes production history changes, that implementation must add focused correctness tests appropriate to the chosen representation before completion.

## Documentation impact

Internal docs: required  
User/Product docs: not required  
Reason: C2 initially adds internal measurement tooling/evidence and characterizes runtime history cost without changing user-visible Undo/Redo behavior. If the second Owner decision authorizes a visible history budget or workflow change, User/Product docs must be reclassified to required before production implementation.

## 16. Stop conditions

Stop for Owner review before or during the measurement phase if:

- valid matrix construction requires bypassing DocumentSession semantics for timed operations;
- benchmark tooling would require a new third-party dependency;
- measuring peak memory requires WinAPI leakage into production layers;
- the harness itself materially changes the history path being measured;
- a requested matrix case threatens machine stability beyond the recorded benchmark safety ceiling;
- evidence suggests correctness regression rather than merely performance cost;
- any production representation, persistent history or user-visible limit would be introduced.

## 17. Activation gate

This proposal does **not** activate C2.

Activation requires explicit Owner acceptance of:

- measurement-first authority;
- the Release semantic benchmark matrix;
- benchmark-only safety cutoff semantics;
- no preselected history representation;
- no production history limit during measurement;
- the second explicit Owner decision before any deeper history production mutation.

Only after acceptance may `work/ACTIVE.yaml` switch from completed C1 to active C2 measurement work.

## 18. Completion gate

C2 completes only after:

1. the accepted measurement phase is implemented and the feasible matrix is recorded;
2. exact benchmark SHA/environment/cutoffs are documented;
3. the Owner explicitly accepts the evidence-driven C2 decision;
4. if the decision is **no deeper change**, that decision is recorded and no production redesign is required;
5. if the decision selects a redesign/budget, the C2 contract/decision record is amended and explicitly Owner-accepted before implementation;
6. any authorized production change has focused correctness evidence and current internal/product docs as required;
7. exact-head Windows FULL passes for the final candidate;
8. governance closeout/CLOSURE passes;
9. merge to main.

After C2 completion, AUDIT-01 schedules Package D next. C2 does not activate D automatically.

## 19. Phase A evidence record

**Measured SHA:** `54605075e8ad7ac3d168dfd820cf84004c3a1023`  
**Windows FULL:** #686 — PASS  
**Dedicated C2 Release benchmark:** #686 — PASS  
**Artifact:** `c2-history-54605075e8ad7ac3d168dfd820cf84004c3a1023`, id `10943125647`, digest `sha256:f23291717d21a35bb7e585290aa94d4a813eee8459284e46ef5b27388c23f827`  
**Durable report:** `work/C2_HISTORY_BENCHMARK_RESULTS.md`

Phase A completion facts:

- all six primary 1,000/10,000 entity × history depth 10/100/1,000 cells completed;
- each primary operation has 20 measured samples after three warm-ups;
- no working-set or timeout cutoff occurred under the 4 GiB / 5 minute benchmark safety envelope;
- largest primary median/p95/max were 421.45/457.20/479.40 µs;
- largest process peak working set was 787.4 MiB at 10,000 entities/depth 1,000;
- the supplemental half-depth branch sample at that largest cell was 7.9412 ms and is explicitly limited single-sample evidence;
- internal as-built history performance documentation records the measurement boundary.

No production history representation, eviction policy, persistent history or Product history budget is authorized by these measurements alone.

C2 is now stopped at the second Owner decision gate in section 13. The next production mutation, if any, requires the Owner to explicitly accept the evidence-driven C2 direction and any necessary contract amendment.
