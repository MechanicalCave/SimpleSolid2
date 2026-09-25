# SK-R0 — Sketcher Foundation & Program Roadmap

**Status:** ACCEPTED — COMPLETED  
**Owner acceptance:** 2026-09-25  
**Decision class:** D2 Architecture / governance  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related ADRs:** ADR-0003, ADR-0005, ADR-0006, ADR-0007  
**Accepted ADR:** ADR-0008  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.0

## 1. Goal

Freeze the agreed Shared 2D / Sketcher architectural foundation and establish a durable, versioned implementation roadmap before the first 2D entity is implemented.

This work exists so later small Sketch contracts can reconstruct:

```text
where the Sketcher is going
what decisions are already frozen
what remains deliberately open
which milestone the current contract implements
```

without relying on chat history or agent memory.

## 2. Scope IN

### Architecture foundation

Add an accepted ADR defining the agreed Sketcher / Shared 2D boundaries, including:

- Shared 2D neutrality toward Part, Assembly and Drawing hosts;
- authored 2D geometry as direct editable design intent;
- independent entity endpoints: equal coordinates do not imply shared authored identity;
- optional authored constraints as relations layered over geometry;
- separation of snap/inference/numeric input from authored constraints/dimensions;
- one runtime tool state with multiple UI/input adapters;
- read-only measurement/diagnostic queries;
- derived planar-region/profile analysis;
- fail-closed diagnostics and no automatic geometry repair;
- provider-neutral Viewer/input/presentation boundaries.

### Program roadmap

Add `work/SKETCH_ROADMAP.md` v1.0 that:

- records frozen agreements by reference to accepted architecture;
- records staged milestones and dependency order;
- distinguishes milestone direction from exact future Work Contract slicing;
- records deliberately deferred/open decisions;
- requires each Sketch Work Contract to identify its roadmap milestone and roadmap impact.

### Reconstruction path

Update repository handoff rules so an accepted program roadmap referenced by `work/ACTIVE.yaml` is read during context reconstruction before the active Work Contract.

Update `work/ACTIVE.yaml` to point to this contract and the Sketch roadmap.

## 3. Scope OUT

```text
production CAD code
EntityId implementation
Line / Arc / Circle implementation
Part Sketch authored 2D payload
Part schema v3
DocumentSession Sketch-entity commands
Viewer Sketch presentation
pointer/cursor tool input transport
Command Line implementation
Operations redesign
snapping / inference
constraints / dimensions / solver
grips / direct manipulation
measure implementation
profile/region implementation
Extrude / Part feature implementation
Assembly / Drawing implementation
```

R0 must not introduce placeholder production APIs for later milestones.

## 4. Architecture invariants

- Foundation remains authoritative for product/domain boundaries.
- ADR-0008 may refine the already accepted Shared 2D direction but must not contradict Foundation CORE.
- The roadmap is planning authority for the Sketch program, not a second Foundation or replacement for accepted ADRs.
- A future Work Contract may subdivide a roadmap milestone without changing the milestone intent.
- A future Work Contract must not silently change a frozen roadmap/ADR decision.
- Any roadmap architecture change requires explicit Owner acceptance and the appropriate ADR/roadmap revision.
- Exact C++ classes, persistence schemas, Viewer APIs, constraint vocabulary and UI key bindings remain unfrozen unless explicitly stated.

## 5. Acceptance

1. ADR-0008 is present and records the agreed Shared 2D / Sketcher foundation.
2. `work/SKETCH_ROADMAP.md` exists with version 1.0, staged milestones, frozen agreements and deferred/open decisions.
3. The roadmap explicitly separates architecture authority from implementation sequencing.
4. `AGENTS.md` requires reading an accepted roadmap referenced by `work/ACTIVE.yaml` before the active Work Contract.
5. `work/ACTIVE.yaml` references the roadmap and this R0 Work Contract.
6. No production source, CAD persistence schema or product behavior changes.
7. Documentation verification remains PASS.
8. The branch passes the repository exact-head verification gate applicable to this governance-only change.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: R0 changes normative architecture/planning and repository context-reconstruction rules only; it does not change the as-built CAD implementation or user-visible product behavior.

## Roadmap impact

Roadmap milestone: R0 — Foundation freeze  
Roadmap version: 1.0  
Roadmap change: establishes the initial accepted Sketch program roadmap.

## 6. Completion

R0 completes only when the accepted foundation, roadmap and reconstruction path are present together and repository verification passes.

Completion authorizes no 2D implementation by itself. The next product mutation still requires a separately accepted small Work Contract derived from the roadmap.


## 7. Completion record

SK-R0 governance/architecture work is complete.

Completed state:

- ADR-0008 freezes the accepted Shared 2D / Sketcher semantic foundation before the first 2D entity implementation;
- `work/SKETCH_ROADMAP.md` v1.0 establishes milestones R0–R11 and separates architecture authority, program sequencing and bounded Work Contracts;
- independent authored Line endpoint semantics, optional authored relations, runtime-only snap/inference/input mechanics, read-only diagnostics and derived planar-region semantics are recorded as accepted architecture;
- provider-neutral Viewer/input/presentation direction is preserved and no Qt/OCCT detail becomes Sketch semantic identity;
- `AGENTS.md` requires reading any accepted roadmap referenced by `work/ACTIVE.yaml` before the active Work Contract;
- `work/ACTIVE.yaml` records this roadmap as the durable Sketch program context;
- no production CAD source, persistence schema or user-visible product behavior changed;
- Windows PR gate #233 passed documentation dispatch/verification, bootstrap verification, build and the complete CTest suite on the pre-completion R0 head.

This completion bookkeeping changes the branch head, therefore the PR must pass one final exact-head Windows gate before merge.

Completion does not authorize R1 implementation. R1 requires a separate explicit Owner-accepted Work Contract derived from `work/SKETCH_ROADMAP.md`.
