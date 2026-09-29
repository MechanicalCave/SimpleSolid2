# R8B — Relational Measure + Runtime Semantic Targets

**Status:** PROPOSED — INACTIVE  
**Proposed:** 2026-09-29  
**Owner acceptance:** pending  
**Decision class:** D2 measurement target/interaction grammar + provider-neutral runtime marker/query contract; bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009, ADR-0011  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.5  
**Predecessor:** `work/R8A_READ_ONLY_MEASURE.md` — completed  
**Milestone:** R8 — Inspect / Measure / diagnostic dimensions — second bounded slice

## 1. Context

R8A established a provider-neutral, read-only Measure tool for one whole Line, Circle or Arc at a time.

R8A deliberately did **not** freeze a universal durable SubElement model. It left relational measurement and semantic runtime point targets to a later bounded slice.

ADR-0009 already establishes two relevant principles:

- durable geometry references, when later required, use stable entity identity plus semantic element role rather than coordinates/provider topology;
- ADR-0009 does not freeze a universal public SubElement C++ type.

Existing direct-manipulation grips already prove that semantic roles such as Line Start/Center/End, Circle Center/Quadrants and Arc Center/Start/End/Mid can be derived at runtime. Those edit grips are not, however, the product meaning of measurement targets.

R8B therefore reuses the **geometric meaning** of those locations where appropriate, but introduces a separate measurement-target vocabulary and presentation contract. It does not promote edit-grip identity into durable CAD identity.

## 2. Goal

Extend the existing Measure tool with an explicit relational mode that can inspect:

1. point ↔ point;
2. point ↔ Line;
3. Line ↔ Line.

The user must be able to choose semantic points explicitly without OSNAP, provider tessellation or hidden nearest-candidate inference.

The target workflow is:

```text
Inspect → Measure
→ activate Between
→ choose first explicit runtime target
→ choose second compatible runtime target
→ show read-only relational result
→ next explicit target starts the next relation
→ Esc returns to ordinary R8A Measure
→ Esc again returns to Select
```

R8B must remain runtime-only, read-only and provider-neutral.

## 3. Scope IN

R8B includes:

- a runtime-only semantic measurement point reference;
- explicit semantic point roles for current Line/Circle/Arc geometry;
- a runtime relational target variant consisting only of semantic point or Line;
- provider-neutral target resolution from semantic/evaluated Sketch geometry;
- provider-neutral point↔point measurement;
- provider-neutral point↔Line perpendicular measurement;
- provider-neutral Line↔Line angle measurement;
- an explicit **Between** mode inside the existing Measure tool;
- active-Measure Command Line keyword `BETWEEN`;
- provider-neutral Viewer contracts for runtime measurement point-marker presentation/query;
- mapping Viewer marker owner tokens back to stable Sketch `EntityId`;
- marker-before-entity hit priority while Between is active;
- fail-closed handling of ambiguous marker hits;
- bounded Operations/status presentation;
- Regular and Construction geometry;
- automated and manual Windows verification;
- required internal and PL/EN Product documentation.

## 4. Scope OUT

R8B does not authorize:

- persistent/universal `SubElement` or `SubElementId`;
- persistence/schema changes;
- authored dimensions or driving/reference dimensions;
- constraints, solver or Auto-Constraint;
- OSNAP, Object Snap Tracking, inference or candidate cycling;
- implicit nearest Endpoint/Midpoint/Center selection from raw pointer position;
- intersection/tangent/perpendicular snap targets;
- free runtime point input;
- intrinsic Sketch Origin as a selectable Measure target;
- circle↔circle, line↔circle, line↔arc, arc↔arc minimum geometric distance;
- closest-point-on-curve computation for arbitrary curve pairs;
- segment↔segment minimum distance;
- intersection-point measurement targets;
- Viewer dimension lines, extension lines, leaders, arrows or measurement text overlays;
- R8C Show Dimensions;
- document-unit architecture or unit conversion;
- R9+ features;
- RMB context;
- Part solid modeling.

Those require later contracts if concrete workflows justify them.

## 5. Runtime semantic measurement point references

### 5.1 Dedicated measurement vocabulary

R8B introduces a bounded runtime measurement concept, conceptually:

```text
MeasurePointRef
  owner: EntityId
  role: MeasurePointRole
```

This is **not** a durable authored reference and must not be persisted.

The measurement vocabulary is deliberately separate from `SketchGripRef` / `SketchGripRole`. Implementation may share pure geometry helpers, but it must not make “edit grip” synonymous with “measurement point”.

### 5.2 Required point roles

For Line:

- Start;
- Midpoint;
- End.

For Circle:

- Center;
- Quadrant +U;
- Quadrant +V;
- Quadrant -U;
- Quadrant -V.

For Arc:

- Center;
- Start;
- End;
- Midpoint along the signed sweep.

These roles resolve from current semantic/evaluated geometry.

### 5.3 Exact geometry

Line midpoint:

```text
M = (Start + End) / 2
```

Circle quadrants are defined in Sketch-local U/V from Center and Radius.

Arc Start:

```text
Center + Radius * [cos(start), sin(start)]
```

Arc End:

```text
Center + Radius * [cos(start + sweep), sin(start + sweep)]
```

Arc Midpoint:

```text
Center + Radius * [cos(start + sweep/2), sin(start + sweep/2)]
```

Signed sweep semantics remain authoritative.

No Viewer tessellation point may define any runtime measurement point.

## 6. Relational target model

R8B relational targets are intentionally limited to:

```text
MeasureRelationTarget =
    MeasurePointRef
  | MeasureLineRef(EntityId)
```

`MeasureLineRef` is valid only when the owner resolves to a current Line.

A whole Circle or whole Arc is not a relational target in R8B. To use its Center/Quadrant/Start/End/Midpoint, the user chooses the explicit runtime point marker.

This keeps the relation grammar small and deterministic.

## 7. Point ↔ point semantics

For first point `A` and second point `B`:

```text
DeltaU = B.u - A.u
DeltaV = B.v - A.v
Distance = hypot(DeltaU, DeltaV)
Direction = atan2(DeltaV, DeltaU)
```

Required result:

- Distance;
- Delta U;
- Delta V;
- directed Angle +U from first point to second point.

Target order is meaningful for Delta/Direction. Reversing A/B negates Delta U/Delta V and changes direction accordingly; Distance remains equal.

Zero distance is a valid diagnostic result.

Center↔Center distance is not a separate operation; it naturally uses Circle/Arc Center point roles.

## 8. Point ↔ Line semantics

A point target `P` and Line `S→E` produce **unsigned perpendicular distance to the infinite supporting line**, not minimum distance to the finite segment.

Let:

```text
V = E - S
Distance = abs(cross(V, P - S)) / length(V)
```

Required result:

- Perpendicular Distance.

Target order is irrelevant: Point→Line and Line→Point mean the same relation.

R8B does not clamp the perpendicular foot to the finite segment and does not silently switch to endpoint distance when the foot lies beyond Start/End.

That alternative is a different “minimum point-to-segment” operation and remains out of scope.

## 9. Line ↔ Line angle semantics

R8B treats a Line as an **undirected infinite supporting direction** for this relation.

The result is the smaller undirected angle between the two line directions:

```text
0° ≤ Angle ≤ 90°
```

Consequences:

- parallel or anti-parallel lines → 0°;
- perpendicular lines → 90°;
- reversing the authored Start/End order of either Line does not change the result;
- line selection order does not change the result.

Implementation may compute this using normalized direction vectors and the absolute dot product, clamping only for floating-point domain safety before `acos`. That clamp is numerical safety, not a Product tolerance.

R8B does not attempt to choose a 135° vs 45° branch around an intersection. Such branch-sensitive angle semantics belong to future authored/reference-dimension work if required.

## 10. Numeric and failure policy

R8B introduces no new Product geometric tolerance.

Required behavior:

- missing/deleted owner → target invalid;
- point role not valid for the current entity kind → target invalid;
- non-finite resolved point/result → reject and show no stale result;
- zero-length Line → reject if ever encountered, although accepted Sketch invariants should already prevent it;
- no gap healing, welding or geometric repair;
- no screen-space pick tolerance may alter the geometric result.

Marker hit aperture is presentation/UI behavior only and must not become a geometric/snap tolerance.

## 11. Measure tool mode grammar

### 11.1 Existing R8A Inspect mode remains default

`MEASURE` continues to activate the existing R8A quick whole-entity inspection mode.

R8B must not change R8A selection-first behavior or whole-entity diagnostic values.

### 11.2 Between activation

While Measure is active, Operations exposes a **Between** action.

The active Measure semantic grammar also accepts case-insensitive token:

```text
BETWEEN
```

`BETWEEN`:

- has meaning only while Measure is active;
- is not a new top-level Sketch command;
- is not a single-letter global alias;
- does not alter Repeat Last Command identity.

Outside active Measure, `BETWEEN` remains unknown.

### 11.3 Entering Between

Entering Between:

- clears the R8A whole-entity runtime target/result;
- preserves ordinary Sketch selection;
- starts `await_first_target`;
- shows runtime measurement point markers;
- creates no authored mutation.

Ordinary selection is never automatically converted into relational targets.

## 12. Explicit marker presentation and query

### 12.1 Separate from edit grips

R8B introduces a provider-neutral runtime **measurement marker** presentation/query contract.

Measurement markers are not:

- edit grips;
- semantic selection;
- snap candidates;
- authored Points;
- durable references;
- dimension overlays.

They exist only while Measure Between mode is active.

### 12.2 Marker availability

While Between is active, supported semantic point markers for current Line/Circle/Arc entities in the active Sketch are visibly available for explicit picking.

Exact marker size, shape and color are D1, but they must be visually distinguishable from ordinary edit grips.

Construction geometry exposes the same semantic marker roles as Regular geometry.

### 12.3 Viewer identity boundary

At the Viewer boundary a marker key may contain:

- owner presentation token;
- runtime marker role.

It must not carry durable CAD identity.

The owning controller maps the presentation token back through the existing Sketch presentation binding to:

```text
SketchId + EntityId + MeasurePointRole
```

before accepting a semantic target.

Provider token order or provider object address must never define target meaning.

### 12.4 Hit priority

While Between is active:

1. explicit measurement marker query has priority;
2. if no marker is hit, whole-Line entity query may produce a `MeasureLineRef`;
3. whole Circle/Arc body hits are not valid relational targets and produce a bounded hint to choose one of their semantic markers;
4. blank hit follows the relational-stage clear rules below.

No nearest semantic point is inferred from an ordinary curve-body click.

## 13. Ambiguous marker hits

The Viewer marker query must be capable of reporting all marker keys hit by the explicit marker aperture; ordering carries no semantic meaning.

After mapping candidates back to semantic references:

- if all valid hit markers resolve to one exact semantic point coordinate, R8B may choose a deterministic semantic identity/role only for runtime labeling because the geometric target is equivalent;
- if hit markers resolve to more than one distinct semantic point coordinate, R8B rejects the click as ambiguous and asks the user to zoom/retry;
- R8B does not cycle candidates and does not choose the nearest semantic point.

This is deliberate fail-closed behavior and must not become an early implementation of R11 OSNAP resolution.

## 14. Relational state machine

Conceptually:

```text
Inspect
  ↓ Between
Between.await_first
  ↓ valid target
Between.await_second(first)
  ↓ compatible target
Between.result(first, second, result)
```

### 14.1 Compatibility

Valid pairs:

- Point + Point → point↔point;
- Point + Line → point↔Line;
- Line + Point → point↔Line;
- Line + Line → Line↔Line angle.

No other pair is accepted.

### 14.2 Incompatible target

An incompatible click:

- does not mutate Document or selection;
- does not discard a valid first target;
- emits a bounded diagnostic;
- remains in `await_second`.

### 14.3 After a result

The result remains visible until the next accepted relational target or mode/tool cancellation.

The next accepted target:

- becomes the first target of a new relation;
- clears the previous result;
- enters `await_second`.

This supports repeated engineering inspection without leaving Between mode.

### 14.4 Blank click

Blank click while Between is active:

- clears pending first target/result;
- returns to `await_first`;
- preserves ordinary selection;
- remains in Between.

If already in empty `await_first`, blank click is a bounded no-op.

## 15. Esc and lifecycle

Esc is hierarchical:

```text
Between
  Esc
→ ordinary R8A Measure / Inspect

Measure / Inspect
  Esc
→ Select
```

The first Esc from Between clears all relational runtime targets/result/markers but does not end Measure.

The second Esc follows existing R8A semantics and returns to Select.

The following also clear all R8B runtime state:

- another Sketch tool activation;
- Sketch edit termination;
- active Sketch/Document replacement;
- Undo/Redo boundary before ordinary global history;
- any fail-closed stale-context condition.

No R8B runtime state is persisted.

## 16. Selection and history invariants

R8B never mutates ordinary semantic selection.

Entering Between, choosing targets, obtaining a result, clearing a result and leaving Between create:

- no DocumentRevision change;
- no dirty-state change;
- no EntityId/ProfileId consumption;
- no Undo entry;
- no persistence/schema data;
- no authored command/transaction.

R8B is a read-only query path.

## 17. CAD input precedence

The WB-02 workspace router remains domain-neutral.

Context-first behavior:

- active Measure + `BETWEEN` → enter relational mode;
- active Measure + unknown token → reject/no mutation;
- Between mode introduces no numeric PointRequest or quantity request;
- bare numeric input has no new R8B meaning;
- accepted top-level Sketch tool command switches tools according to existing semantics and clears Measure state.

`MEASURE` and `BETWEEN` are not Repeat Last Command identities.

## 18. Units and presentation boundary

Provider-neutral relational results remain in current Sketch coordinate scale:

- linear values in current Sketch coordinate units;
- angles in radians.

UI may display angles in degrees.

R8B does not label linear values with `mm`, `in`, etc. until the accepted R10 Units architecture exists.

Operations must identify enough about Target A / Target B for the user to understand the relation without exposing Viewer tokens.

Exact wording/precision/layout is D1.

## 19. Viewer scope boundary versus R8C

R8B's Viewer extension is limited to **explicit runtime point-marker presentation and hit query** required to choose semantic measurement targets.

R8B does not authorize:

- dimension/extension lines;
- arrows;
- leader geometry;
- numeric labels inside the viewport;
- persistent graphics;
- selectable diagnostic geometry;
- snap targets.

Those remain R8C or later work.

## 20. Minimum-distance operations deliberately deferred

The roadmap mentions minimum geometric distance “where applicable”, but R8B deliberately does not implement generic closest-distance relations.

Reason:

- finite segment vs infinite-line semantics differ;
- Circle/Arc closest-point behavior introduces additional branch/case rules;
- intersecting/tangent cases introduce exact-contact and candidate semantics;
- general closest-point UX begins to overlap future R11 snapping/inference and later curve breadth.

R8B first establishes explicit semantic runtime targets and three unambiguous relation families. A later bounded slice may add minimum curve distance if concrete engineering workflows justify it.

## 21. Construction geometry

Construction geometry participates in R8B measurement exactly like Regular geometry.

Construction semantic role does not change:

- marker point geometry;
- point↔point result;
- point↔Line perpendicular result;
- Line↔Line angle result.

R8B never converts role or affects Profile participation.

## 22. Verification footprint

Prefer existing test executables so ordinary development iterations remain CI-03 FOCUSED.

Expected affected surfaces:

- `sk02a_shared_2d_core_test` / `sk02a.shared_2d_core` — point-role resolution and three provider-neutral relational formulas;
- `sk03a_viewer_sketch_contracts_test` / `sk03a.viewer_sketch_contracts` — marker scene/query validity;
- `sk04b_part_viewport_selection_bridge_test` / `sk04b.part_viewport_selection_bridge` — marker token→semantic target bridge and ambiguity behavior;
- `sk04c_part_sketch_interaction_controller_test` / `sk04c.part_sketch_interaction_controller` — Between state, selection/history/lifecycle;
- `d_cad_input_semantics_test` / `d.cad_input_semantics` — active-tool `BETWEEN` routing/boundaries if semantic endpoint changes;
- `sk01_workbench_sketch_host_test` / `sk01.workbench_sketch_host` — real Operations/UI workflow.

Exact target choice is D1.

Verification infrastructure must not be changed merely to obtain a cheaper CI tier. Final merge evidence remains exact-head Windows FULL.

## 23. Automated acceptance coverage

At minimum verify:

1. Line Start/Midpoint/End resolve exactly from semantic geometry;
2. Circle Center and four U/V Quadrants resolve exactly;
3. Arc Center/Start/End/Midpoint respect canonical signed-sweep semantics;
4. Regular and Construction resolve identically for equal geometry;
5. invalid owner/role combination fails closed;
6. point↔point Distance/DeltaU/DeltaV/Direction are correct;
7. reversed point target order preserves Distance and reverses directed deltas/direction;
8. same-coordinate point↔point returns valid zero distance;
9. Center↔Center works through ordinary point roles;
10. point↔Line uses the infinite supporting line and does not clamp to segment endpoints;
11. Point+Line and Line+Point produce the same perpendicular distance;
12. Line↔Line angle is order-independent and Start/End-reversal-independent;
13. parallel/anti-parallel → 0° and perpendicular → 90°;
14. numeric domain clamp is bounded to floating-point safety only;
15. marker scene contains only runtime semantic measurement markers;
16. marker keys never expose EntityId to Viewer;
17. marker query supports multiple hits without assigning ordering meaning;
18. same-coordinate overlapping marker candidates are measurement-equivalent;
19. distinct-coordinate ambiguous marker candidates reject/no target mutation;
20. marker hit has priority over entity-body hit in Between;
21. Line-body hit creates only a Line target;
22. Circle/Arc body hit does not silently infer Center/Quadrant/endpoint;
23. entering Between preserves ordinary selection and clears only Measure quick target/result;
24. first target → second compatible target produces one read-only result;
25. incompatible second target preserves first target;
26. next accepted target after result starts the next relation;
27. blank click clears pending/result but remains Between;
28. first Esc leaves Between and returns to ordinary Measure;
29. second Esc returns Measure to Select;
30. tool/history/context replacement clears R8B runtime state;
31. no revision/dirty/history/identity/persistence mutation occurs;
32. active Measure `BETWEEN` is case-insensitive and context-local;
33. `BETWEEN` outside active Measure remains unknown;
34. Measure/Between do not replace Repeat Last Command identity;
35. no OSNAP/tracking/inference code path is introduced;
36. existing R8A quick Measure remains unchanged;
37. existing selection/grip/transform/Profile/CAD-input regressions remain green;
38. exact-head Windows FULL passes;
39. required docs and Product Browser freshness pass.

## 24. Manual Windows verification

Final candidate requires Owner verification:

- Measure one entity normally and confirm R8A behavior is unchanged;
- activate **Between**;
- verify Start/Mid/End markers on a Line;
- verify Center/Quadrant markers on a Circle;
- verify Center/Start/End/Mid markers on an Arc;
- point→point: choose two explicit markers and confirm Distance/DeltaU/DeltaV/Angle +U;
- reverse the point order and confirm Distance stays equal while direction/deltas reverse;
- point→Line: choose a point marker plus a Line body and verify perpendicular distance;
- use a point whose perpendicular foot lies outside the finite segment and verify the infinite-line distance semantics;
- Line→Line: verify parallel ≈ 0°, perpendicular ≈ 90° and a simple oblique case;
- verify Construction points/lines behave the same;
- verify Circle/Arc body does not silently become Center;
- verify ordinary selection is unchanged throughout;
- blank click clears relation state but remains Between;
- Esc once → normal Measure; Esc again → Select;
- activate `BETWEEN` through Command Line while Measure is active;
- verify `BETWEEN` outside Measure is rejected;
- confirm measuring creates no dirty/Undo step;
- regression smoke R8A Measure, grips, COPY/Grip Copy and Profile.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: R8B adds user-visible relational Measure semantics, runtime semantic point markers and a new provider-neutral Viewer marker/query boundary that require as-built architecture and PL/EN workflow documentation.

Internal documentation must explain:

- runtime measurement point references versus edit grips and durable references;
- marker scene/query ownership and token→EntityId mapping;
- explicit fail-closed ambiguity behavior;
- three relational formulas and infinite-line / smaller-angle semantics;
- read-only lifecycle/history boundary;
- separation from R8C and R11.

Product PL/EN documentation must explain the Between workflow, target roles, supported relation pairs and current limitations.

Generated Product Browser must be regenerated and deterministic.

## 26. Stop conditions

Stop for Owner review if implementation requires:

- durable/persisted sub-element identity;
- persistence/schema changes;
- generic snap/inference candidate resolver;
- nearest semantic point inference from raw curve click;
- candidate cycling;
- intrinsic Origin target architecture;
- free-point input;
- generic curve↔curve minimum distance;
- intersection/tangent semantic target generation;
- new Product geometric tolerance;
- unit-system/document-unit architecture;
- Viewer-derived geometric measurement;
- viewport dimension overlays;
- authored dimensions/constraints/solver;
- changing EntityId lifecycle;
- changing normal semantic selection grammar;
- structural editing;
- Part solid modeling.

These are D2/D3 scope expansions.

## 27. R8 continuation boundary

R8B does not complete the whole R8 milestone.

After R8B, R8C remains separately inactive:

### R8C — Show Dimensions diagnostic overlays

Expected scope:

- read-only current-selection and whole-Sketch dimension overlays;
- provider-neutral diagnostic presentation data;
- bounded Viewer presentation implementation;
- overlays non-selectable, non-editable, non-snappable, without EntityId/persistence/Undo.

R9 remains inactive until R8 is completed or the Owner explicitly amends sequencing.

## 28. Completion boundary

R8B may become ACTIVE only after explicit Owner acceptance of this Work Contract.

Completion requires:

- accepted target/relation semantics preserved;
- no durable sub-element identity or OSNAP creep;
- focused/affected iteration evidence;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- required internal and PL/EN Product documentation/current Browser;
- work-only CLOSURE closeout.

R8C and R9+ remain inactive after R8B completion unless separately accepted.
