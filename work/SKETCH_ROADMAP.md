# Sketcher Program Roadmap

**Status:** ACCEPTED  
**Version:** 1.1  
**Owner acceptance:** 2026-09-25  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009  
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

The following are accepted architecture and must be read from ADR-0008 and ADR-0009 rather than re-decided by individual implementation contracts:

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
13. Sketch-local U/V are physical model-length coordinates hosted by a stable orthonormal frame; current Origin-plane Sketch (0,0) maps to Part Origin.
14. Sketch Origin (0,0) is an intrinsic immutable reference, not ordinary user-authored Point geometry.
15. Entity identity must not silently alias a different semantic entity after delete/reorder/storage compaction; state copies preserve identity while semantic duplication creates fresh identity.
16. Authored and evaluated Sketch geometry are distinct architectural layers even while initial evaluation is identity.
17. Durable future sub-element references use stable entity identity plus semantic element role, never coordinate/index/provider identity.
18. Select is the default Sketch tool; cancelling/finishing other tools returns to Select rather than an undefined no-tool state.
19. Sketch selection is transient semantic runtime state supporting point and rectangular selection; it is never persisted CAD intent.
20. When a geometry primitive reaches a user-facing interactive milestone, create/present/select/delete is the minimum usable lifecycle.
21. Select uses pick-box style cursor presentation in the Viewport; create/edit tools use crosshair-style presentation; UI controls use normal widget/system cursors.
22. Pointer-to-Sketch input must remain correct after camera orbit and therefore maps spatial pointer/ray input onto the active Sketch plane/frame.
23. Projected/reference geometry is semantically distinct from ordinary editable local geometry and follows host-domain projection policy.

## 4. Milestones

### R0 — Foundation freeze and durable roadmap

**Status:** completed

Goal:

- accept ADR-0008;
- establish this roadmap;
- make roadmap reconstruction mandatory for later Sketch work.

No product/CAD implementation belongs to R0.

### R1 — Minimal Shared 2D authored core

**Status:** completed

Goal:

- establish the smallest reusable authored 2D model;
- introduce stable model-local entity identity;
- introduce finite 2D coordinate/value semantics;
- implement the first Line primitive with independent authored start/end coordinates;
- establish minimal entity add/lookup/erase/validation semantics;
- prove stable identity does not derive from storage/index and is not immediately reused after erase in the continuing model instance;
- preserve value-copy identity semantics needed by future host history;
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

**Status:** completed

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

**Status:** next — not started

Goal:

- extend the neutral Viewer/application boundary for authored Sketch presentation;
- provide runtime preview presentation separate from authored geometry;
- provide provider-neutral spatial pointer/cursor input suitable for mathematically mapping to active Sketch U/V even after orbit;
- provide Sketch presentation for authored geometry plus intrinsic Origin/reference overlays without turning them into Viewer-owned CAD state;
- establish runtime cursor-mode presentation (Select pick-box versus create/edit crosshair) separately from snap/pick/geometric tolerances;
- preserve normal camera/navigation behavior;
- establish the runtime tool-state boundary without implementing the whole Sketcher.

This milestone is the gate that prevents Qt/OCCT event details or presentation tokens from defining Sketch semantics.

### R4 — First complete continuous Line workflow

Goal:

- expose Sketch tools while in Sketch edit context;
- make Select the default active Sketch tool;
- implement one active Line tool state;
- first-point and next-point workflow;
- rubber-band preview;
- continuous successive segment creation;
- commit each accepted segment through the semantic command/transaction path;
- predictable finish/cancel behavior;
- define and test the commit/Undo granularity explicitly in the R4 Work Contract rather than inheriting it accidentally from UI implementation;
- introduce the compact Command Line concept as an adapter to the same tool state;
- Operations presents contextual Line state/actions rather than owning another Line implementation;
- returning from Line activation/finish/cancel ends in Select;
- make committed Line geometry point-selectable by semantic EntityId;
- support rectangular selection for geometry that is impractical to point-pick;
- support semantic Delete of selected Line geometry through the ordinary command/transaction/Undo path;
- keep selection transient and provider-token-free.

Advanced snapping, constraints, grips/direct manipulation and dynamic input are not prerequisites for proving this workflow.

### R5 — Sketch direct manipulation and selection refinement

Goal:

- refine the R4 semantic selection foundation for multi-selection/primary-selection interactions as required;
- keep selection coherent between Viewport/Properties/Tools as applicable;
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


### R12 — Planar-face Sketch support and projected reference geometry

Goal:

- introduce a semantically stable Part planar-face support contract without persisting raw kernel/provider face identity;
- derive a stable orthonormal Sketch frame whose orientation does not silently flip with provider topology order/orientation;
- support provider-neutral exact projection/materialization of representable support/reference edges into Sketch-local U/V;
- expose projected/reference curves as semantically read-only reference geometry that can be inspected/snapped according to later interaction contracts;
- respect Foundation projection policy: explicit Part-local Project Edge remains snapshot/capture by default;
- explicitly decide automatic support-face boundary snapshot-versus-support-associative behavior before implementation;
- explicitly decide projected-reference participation in profile/region construction before implementation.

SS1 projection/reference code may be audited as donor mechanism/test material only after compatibility review.


## 5. Deliberately open decisions

The roadmap does not decide ahead of evidence:

- exact `EntityId` representation/serialization/allocation mechanism across persistence and history;
- exact public Sketch model C++ API;
- universal sub-element/reference API;
- standalone Point entity semantics;
- exact persistence JSON for entities;
- exact Viewer Sketch-scene, spatial pointer/ray and input APIs;
- exact selection modifier/window-vs-crossing gestures;
- exact cursor pixel sizes/colors/HiDPI rendering;
- exact keyboard aliases/Enter/Esc/Space/RMB grammar beyond the accepted Esc-to-Select direction;
- exact dynamic-input UX;
- exact numerical tolerances;
- exact constraint set and solver technology;
- auto-constraint policy;
- exact Arc/Circle authoring variants;
- exact Trim/Extend/Split identity policy;
- exact planar-face semantic reference and stable frame derivation;
- automatic support-boundary projection snapshot-versus-associative policy;
- projected/reference geometry participation in region/profile construction;
- durable region identity;
- future Part feature graph/history/body architecture;
- associative versus snapshot consumption of Sketch regions by future Part operations.

These topics require explicit later evidence and must not be silently frozen by convenience implementations.

## 6. Program state

Current:

```text
R0  completed
R1  completed
R2  active — SK-02B
R3  not started
R4+ not started
```

R1 is completed by `work/SK-02A_MINIMAL_SHARED_2D_AUTHORED_CORE.md`. R2 remains active under `work/SK-02B_PART_HOST_DURABLE_LIFECYCLE.md` until its final exact-head gate passes. R3 remains unauthorized.
