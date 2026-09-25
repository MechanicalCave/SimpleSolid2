# Sketcher Program Roadmap

**Status:** ACCEPTED  
**Version:** 1.0  
**Owner acceptance:** 2026-09-25  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008  
**Current program:** Shared 2D Authoring / Part-hosted Sketcher

## 1. Purpose

This roadmap is the durable program-level execution map for the Sketcher effort.

It exists to preserve implementation direction across many small Work Contracts and conversation/context boundaries.

Authority is deliberately split:

```text
Foundation / accepted ADRs
    define architecture and semantic invariants

Sketcher Program Roadmap
    defines staged goals and dependency order

Active Work Contract
    defines the bounded implementation change being performed now

Code / tests / as-built docs
    prove the current implementation state
```

The roadmap is not a second Foundation and must not override accepted architecture.

## 2. Operating rules

- Every Sketch-related Work Contract must name its roadmap milestone and roadmap version.
- A milestone may be implemented by one or many Work Contracts.
- Contract numbering is not frozen by this roadmap.
- A contract may split a milestone into smaller reversible slices without changing milestone intent.
- A contract must declare whether it changes this roadmap.
- A frozen architectural agreement must not be changed implicitly to make implementation easier.
- If evidence requires changing architecture or milestone dependency order, stop and obtain explicit Owner acceptance before revising the relevant ADR/roadmap.
- Completed implementation evidence belongs in Work Contracts, Git and as-built docs; this roadmap remains concise current program state.

## 3. Frozen agreements

The following are accepted architecture and must be read from ADR-0008 rather than re-decided by individual implementation contracts:

1. Shared 2D / Sketch Core is host-neutral and does not depend on Part/Assembly/Drawing, Qt, OCCT or filesystem paths.
2. Authored primitive geometry is directly editable design intent.
3. Line endpoints are independent authored entity sub-elements; equal coordinates do not imply shared authored identity.
4. Constraints are optional authored relations layered over geometry.
5. Snap, inference, numeric input and grips are runtime interaction concepts unless an explicit command authors persistent intent.
6. One active tool has one runtime semantic tool state; mouse, keyboard/Command Line, dynamic input and Operations are adapters to it.
7. Measurement/inspection/diagnostics are read-only by default and never silently repair authored geometry.
8. Screen/pixel, snap, geometry/intersection, region and future solver tolerances are distinct concerns.
9. Closed planar regions are derived from evaluated geometry and are not authored Sketch entities by default.
10. Shared 2D owns geometric region analysis; Part owns the modeling meaning of consuming a region.
11. Viewer/input/presentation integration remains provider-neutral.
12. Part-hosted Sketch editing remains in the common 3D Document Viewport.

## 4. Milestones

### R0 — Foundation freeze and durable roadmap

**Status:** completed

Goal:

- accept ADR-0008;
- establish this roadmap;
- make roadmap reconstruction mandatory for later Sketch work.

No product/CAD implementation belongs to R0.

### R1 — Minimal Shared 2D authored core

Goal:

- establish the smallest reusable authored 2D model;
- introduce stable model-local entity identity;
- introduce finite 2D coordinate/value semantics;
- implement the first Line primitive with independent authored start/end coordinates;
- establish minimal entity collection/lookup/validation semantics;
- prove behavior with semantic tests.

Deliberately outside R1:

- Part integration;
- persistence;
- Qt/Viewer/OCCT;
- interactive tools;
- selection/grips;
- snapping/inference;
- constraints/solver;
- profiles.

### R2 — Part host integration and durable lifecycle

Goal:

- embed the Shared 2D authored model inside the existing Part-hosted Sketch without transferring host semantics into Sketch Core;
- create semantic mutation path through DocumentSession/Part transaction;
- integrate Undo/Redo and dirty/save checkpoint behavior;
- version Part persistence and preserve prior readable schemas;
- prove Save → Close → Reopen identity/geometry lifecycle.

Deliberately outside R2:

- full interactive Line UX;
- Viewer cursor/preview infrastructure;
- snapping/inference;
- constraints/solver.

### R3 — Provider-neutral Sketch presentation and tool-input boundary

Goal:

- extend the neutral Viewer/application boundary for authored Sketch presentation;
- provide runtime preview presentation separate from authored geometry;
- provide provider-neutral pointer/cursor input suitable for mapping to active Sketch U/V;
- preserve normal camera/navigation behavior;
- establish the runtime tool-state boundary without implementing the whole Sketcher.

This milestone is the gate that prevents Qt/OCCT event details or presentation tokens from defining Sketch semantics.

### R4 — First complete continuous Line workflow

Goal:

- expose Sketch tools while in Sketch edit context;
- implement one active Line tool state;
- first-point and next-point workflow;
- rubber-band preview;
- continuous successive segment creation;
- commit each accepted segment through the semantic command/transaction path;
- predictable finish/cancel behavior;
- define and test the commit/Undo granularity explicitly in the R4 Work Contract rather than inheriting it accidentally from UI implementation;
- introduce the compact Command Line concept as an adapter to the same tool state;
- Operations presents contextual Line state/actions rather than owning another Line implementation.

Advanced snapping, constraints and dynamic input are not prerequisites for proving this workflow.

### R5 — Sketch selection and direct manipulation

Goal:

- generalize document-scoped semantic selection to Sketch entities;
- primary selection remains coherent between Tree/Viewport/Properties/Tools as applicable;
- expose runtime Line grips for start/end/center;
- dragging an endpoint edits only that endpoint unless authored constraints later require otherwise;
- center grip translates the whole Line without creating a midpoint entity;
- no Viewer token becomes durable identity.

### R6 — Inspect / Measure / geometry diagnostics

Goal:

- read-only exact inspection of Sketch geometry;
- line length and direction/angle;
- point coordinates;
- point-to-point distance/gap;
- horizontal/vertical and related geometric deviation checks as introduced;
- diagnostics must distinguish observed geometric facts from persistent constraints;
- no measurement action creates authored dimensions or repairs geometry automatically.

### R7 — Precision input, Object Snap and inference

Goal:

- precise keyboard/Command Line coordinate/value entry;
- object snap foundation for semantic candidates such as Endpoint, Midpoint, Center, Intersection, Quadrant, Perpendicular, Tangent, Nearest and Origin as supported by available entity types;
- geometric inference such as Horizontal/Vertical and later relation candidates;
- optional dynamic input presentation;
- all channels feed one tool state.

A snap/inference may guide exact geometry creation without automatically creating a persistent constraint.

### R8 — Optional geometric relations and local solver/evaluation

Goal:

- introduce the first explicit authored relations such as Coincident/Horizontal only under a separately accepted contract;
- preserve independent endpoint identity;
- introduce a bounded local constraint/evaluation layer;
- direct manipulation re-evaluates legal constrained geometry;
- solver/evaluator reports structured diagnostics and does not silently rewrite unrelated authored intent.

No global project-wide parametric graph is introduced.

### R9 — Geometry breadth and editing operations

Goal:

- add further primitives only when required by concrete workflows, beginning with Circle/Arc as appropriate;
- add editing operations such as Move/Trim/Extend/Offset in small contracts;
- preserve stable entity identity and command/transaction semantics;
- extend snapping, grips, measurement and constraints only as demonstrated by each primitive.

Exact ordering inside R9 is intentionally not frozen.

### R10 — Planar region analysis and Sketch diagnostics

Goal:

- derive intersections/planar arrangement from evaluated non-construction geometry;
- detect bounded regions, open chains and relevant diagnostics;
- support Sketch-level `Find Profiles`/region inspection without authored mutation;
- keep region-analysis tolerances distinct from screen/snap/solver tolerance;
- never close gaps or repair geometry merely because a region appears visually closed.

Derived intersection/region topology must not force automatic authored entity splitting.

### R11 — Part consumption of derived Sketch regions

Goal:

- allow future Part modeling tools to query and present derived Sketch regions as candidate inputs;
- support selection of one or more non-overlapping/atomic usable regions as the Part operation contract requires;
- preserve host ownership: Shared 2D supplies geometry/regions, Part owns modeling semantics.

Before persistent Part feature implementation, explicitly decide associative versus snapshot input semantics, durable reference/rebinding policy and failure behavior. R11 does not pre-authorize a Body/Feature/Extrude architecture.

## 5. Deliberately open decisions

The roadmap does not decide ahead of evidence:

- exact `EntityId` representation/serialization;
- exact public Sketch model C++ API;
- universal sub-element/reference API;
- standalone Point entity semantics;
- exact persistence JSON for entities;
- exact Viewer Sketch-scene and input APIs;
- exact keyboard aliases/Enter/Esc/Space/RMB grammar;
- exact dynamic-input UX;
- exact numerical tolerances;
- exact constraint set and solver technology;
- auto-constraint policy;
- exact Arc/Circle authoring variants;
- exact Trim/Extend topology policy;
- durable region identity;
- future Part feature graph/history/body architecture;
- associative versus snapshot consumption of Sketch regions by future Part operations.

These topics require explicit later evidence and must not be silently frozen by convenience implementations.

## 6. Program state

Current:

```text
R0  completed
R1  next — not started
R2  not started
R3  not started
R4+ not started
```

R1 is the next implementation milestone. It must begin with a separate explicit Owner-accepted Work Contract; this roadmap does not itself authorize production code changes.
