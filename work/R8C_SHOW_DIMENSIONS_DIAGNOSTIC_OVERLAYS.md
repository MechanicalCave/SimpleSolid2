# R8C — Show Dimensions Diagnostic Overlays

**Status:** PROPOSED — INACTIVE  
**Proposed:** 2026-09-29  
**Owner acceptance:** pending  
**Decision class:** D2 diagnostic-overlay interaction/presentation contract + provider-neutral Viewer boundary; bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009, ADR-0010  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.5  
**Predecessors:** `work/R8A_READ_ONLY_MEASURE.md`, `work/R8B_RELATIONAL_MEASURE_RUNTIME_TARGETS.md` — completed  
**Milestone:** R8 — Inspect / Measure / diagnostic dimensions — third and final bounded slice

## 1. Context

R8A established provider-neutral, read-only whole-entity measurement for Line, Circle and Arc.

R8B established explicit runtime semantic measurement points, three relational measurement families and one tool-local current-relation cue. R8B deliberately stopped before a general viewport dimension-overlay system.

Roadmap v1.5 still requires the final R8 capability:

- current-selection and whole-Sketch Show Dimensions;
- provider-neutral diagnostic presentation data;
- multiple simultaneous read-only annotations;
- classical dimension graphics where useful;
- no authored dimension/constraint semantics.

R8C completes that bounded diagnostic surface. It must not convert read-only diagnostics into authored dimensions, constraints, solver state, OSNAP semantics or durable sub-element identity.

## 2. Goal

Provide a discoverable **Show Dimensions** display mode for the active Sketch with two explicit scopes:

```text
Off
Selection
Sketch
```

The mode displays a small canonical set of intrinsic dimensions derived from current authoritative Sketch geometry.

The target product behavior is:

```text
Inspect → Show Dimensions
→ choose Selection or Sketch scope
→ see read-only viewport annotations
→ pan / zoom / orbit / select normally
→ annotations relayout as presentation changes
→ geometry-changing transient preview suppresses annotations
→ commit/cancel regenerates them from authoritative geometry
→ Off removes all annotations
```

R8C is a runtime diagnostic display mode, **not** an authored CAD tool.

## 3. Decision summary

R8C adopts these bounded D2 decisions, subject to Owner acceptance:

1. **Show Dimensions is a runtime display mode, not a `SketchTool`.**
2. The runtime state is exactly `Off | Selection | Sketch`.
3. Selection scope means intrinsic dimensions for each currently selected supported entity.
4. Sketch scope means intrinsic dimensions for every current Line/Circle/Arc in the active Sketch.
5. R8C does **not** infer pairwise relations among selected entities.
6. The canonical overlay set is deliberately minimal:
   - Line → aligned Length;
   - Circle → Diameter;
   - Arc → Radius + absolute authored Sweep Angle.
7. Absolute position, Delta U/V, Line angle +U, area, circumference, arc length, start/end angle and pairwise distance/angle remain available through Measure/Operations but are not automatically overlaid.
8. Provider-neutral semantic code owns geometry/value meaning; Viewer/Qt/OCCT owns only projection-aware presentation/layout.
9. Diagnostic overlays are non-selectable, non-editable, non-snappable and identity-free at the Viewer boundary.
10. No display preference is persisted.
11. No generic cross-domain annotation framework is introduced.
12. R8B current-relation cues remain separate and may coexist with R8C overlays.

## 4. Scope IN

R8C includes:

- one runtime `ShowDimensionsMode = Off | Selection | Sketch`;
- a discoverable Inspect / Show Dimensions UI surface;
- Selection-scope overlay generation;
- whole-Sketch overlay generation;
- canonical intrinsic dimension descriptors for Line/Circle/Arc;
- Regular and Construction geometry;
- provider-neutral semantic dimension descriptor generation;
- active-Sketch-frame mapping into provider-neutral Viewer scene geometry;
- formatted viewport labels using the existing R8 diagnostic numeric policy;
- multiple simultaneous overlay items;
- aligned linear dimension presentation for Line Length;
- diameter presentation for Circle;
- radius leader presentation for Arc;
- angular dimension presentation for Arc authored sweep;
- bounded deterministic screen-space/layout handling for multiple annotations;
- DPI/resize/pan/zoom/orbit relayout;
- suppression during geometry-changing transient preview;
- automatic regeneration after commit/cancel/history refresh;
- bounded presentation-failure handling with no stale overlays;
- automated and manual Windows verification;
- required internal and PL/EN Product documentation.

## 5. Scope OUT

R8C does not authorize:

- authored dimensions of any kind;
- driving/reference dimension entities;
- constraints, solver, Auto-Constraint or relation persistence;
- a durable `DimensionId`, `SubElementId` or universal sub-element model;
- persistence/schema changes;
- persistence of Show Dimensions mode or layout;
- editable dimension text;
- dragging dimension labels/lines to author geometry;
- dimension-to-constraint conversion;
- selecting, snapping to or grip-editing overlay graphics;
- OSNAP, tracking, inference, candidate ranking/cycling;
- dynamic numeric input or Dynamic Input fields;
- live preview dimensions during geometry-changing manipulation;
- coordinate/ordinate dimensions;
- automatic dimensions to Sketch Origin;
- automatic pairwise dimensions between selected entities;
- automatic point↔point, point↔Line or Line↔Line R8B relation overlays;
- generic curve↔curve minimum distance;
- intersection/tangent dimensions;
- Line angle relative to +U as an automatic viewport overlay;
- Circle radius and diameter simultaneously;
- Circle circumference/area overlays;
- Arc length, start angle or end angle overlays;
- a generic application-wide annotation/layout framework for future Drawing;
- document-unit architecture or unit conversion;
- R9+ features;
- ordinary RMB context;
- Part solid modeling.

Those require later accepted contracts if concrete workflows justify them.

## 6. Runtime mode ownership

R8C state is runtime-only and conceptually:

```text
ShowDimensionsMode
  Off
  Selection
  Sketch
```

The mode belongs to the active Sketch editing runtime/application context.

It is not:

- authored Sketch state;
- PartDocument state;
- Project state;
- a Workbench-global preference;
- CAD Undo history;
- a persisted user preference.

A new Sketch edit session starts in `Off`.

Switching active Sketch, Document, ending Sketch edit or tearing down the Workbench clears the mode to `Off`.

Ordinary tool switching inside the same active Sketch does not itself turn the mode off.

## 7. UI semantics

The Part-hosted Sketcher exposes **Show Dimensions** under the existing Inspect surface.

The user must be able to choose:

- **Off**;
- **Selection**;
- **Sketch**.

Exact widget geometry is D1. A split button, compact mode buttons or equivalent bounded UI is acceptable if all three states are explicit and discoverable.

Show Dimensions activation:

- does not activate a new `SketchTool`;
- does not cancel ordinary Select/Measure/create/modify tools merely because the display mode changes;
- does not replace Repeat Last Command identity;
- does not mutate semantic selection.

R8C does not require a new Command Line command/keyword. Adding a CAD-input grammar for this display preference is outside this slice unless separately accepted.

## 8. Scope semantics

### 8.1 Selection mode

In `Selection`:

- the current semantic Sketch selection is the source set;
- every selected supported Line/Circle/Arc contributes its canonical intrinsic annotation set;
- zero selected entities means zero annotations while the mode remains active;
- changing selection regenerates the overlay from the new semantic selection;
- primary selection may receive D1 visual emphasis, but primary does not alter numeric meaning;
- provider query order never defines the source set or annotation priority.

Selection mode does **not** form combinations of selected entities.

For example, selecting two Lines shows two independent Line Length annotations. It does not automatically show their relative angle.

### 8.2 Sketch mode

In `Sketch`:

- every current Line/Circle/Arc in the active Sketch contributes its canonical intrinsic annotation set;
- ordinary semantic selection remains independent;
- selecting or clearing entities does not change which entities are dimensioned;
- selected entities may receive D1 visual emphasis without changing the set or values.

No hidden dependency on Viewer/native presentation membership may decide which semantic entities exist.

### 8.3 Unsupported/invalid source

If an entity cannot produce a valid finite diagnostic descriptor:

- no stale annotation for that descriptor is retained;
- the failure remains runtime-only;
- authored state and selection are unchanged.

Accepted Sketch invariants should make this exceptional.

## 9. Canonical intrinsic dimension set

R8C intentionally avoids automatically reproducing every scalar reported by R8A.

The overlay set represents the most recognizable intrinsic size/shape dimensions for each primitive while minimizing clutter and avoiding coordinate/reference semantics.

### 9.1 Line → aligned Length

For Line Start `S` and End `E`:

```text
Length = hypot(E.u - S.u, E.v - S.v)
```

The semantic descriptor contains the exact Line endpoints and Length value.

The viewport presents one classical aligned linear dimension:

- witness/extension geometry from the exact endpoints;
- an offset dimension line approximately parallel to the Line in projection;
- arrowheads at the dimension extents;
- one numeric Length label.

The offset side/distance is presentation-only.

Reversing authored Start/End must not change the numeric value.

R8C does not automatically overlay Delta U, Delta V or Angle +U for a Line.

### 9.2 Circle → Diameter

For Circle Center `C` and Radius `R`:

```text
Diameter = 2 * R
```

The semantic descriptor contains exact Center, Radius and Diameter.

The viewport presents one diameter annotation with the conventional diameter prefix/symbol.

A Circle has no authored preferred diameter direction. Therefore the visual diameter/leader orientation is explicitly **presentation-only** and may be chosen by deterministic projection/layout logic.

That orientation must never become semantic identity or authored intent.

R8C does not simultaneously show Circle Radius, Circumference or Area.

### 9.3 Arc → Radius

For Arc Center `C`, Radius `R`, authored Start Angle and signed Sweep:

- Radius is the canonical semantic radius;
- the leader target uses the exact midpoint along the authored signed sweep:

```text
Mid = C + R * [cos(start + sweep/2), sin(start + sweep/2)]
```

The viewport presents one radial leader from/around the exact Center-to-midpoint radial direction and a conventional `R` label.

The exact leader bend/text placement is presentation-only.

### 9.4 Arc → authored Sweep Angle

Arc angular display uses the canonical authored signed sweep branch.

Semantic value:

```text
SweepDisplay = abs(signedSweep)
```

The provider-neutral descriptor retains enough exact Start/End/Center/signed-sweep geometry to identify the authored branch.

The viewport presents an angular dimension arc with witness rays/arrows and a degree-formatted numeric label.

Important:

- the displayed numeric magnitude is positive;
- the dimension arc follows the **authored sweep branch**, including a valid long arc;
- Viewer must not replace it with the smaller undirected angle;
- reversing/changing authored sweep changes the diagnostic only through current semantic geometry.

This is distinct from R8B Line↔Line, whose result deliberately uses the smaller undirected angle.

### 9.5 Deliberately omitted automatic overlays

The following remain available through quick Measure/Between/Operations but are not part of Show Dimensions:

- Line Delta U/Delta V;
- Line Angle +U;
- Circle Radius;
- Circle Circumference/Area;
- Arc Start/End Angle;
- Arc Length;
- relational Distance/Perpendicular Distance/Line↔Line angle;
- absolute coordinates.

This is deliberate clutter and architecture control, not a claim that those diagnostics are unimportant.

## 10. Semantic descriptor boundary

Provider-neutral Shared 2D/application logic derives diagnostic descriptors from current semantic Sketch geometry.

Conceptually, the semantic side may use a value-semantic form such as:

```text
SketchDiagnosticDimension
  source: EntityId                  // semantic/application side only
  source_role: Regular | Construction
  kind:
    AlignedLength
    Diameter
    Radius
    AngularSweep
  exact semantic anchors/curve data
  exact numeric value in Sketch units/radians
```

The exact C++ representation and namespace are D1.

The source `EntityId` is allowed inside the semantic/application layer for filtering/rebuild bookkeeping.

It must not become a selectable/persistent Dimension identity.

## 11. Viewer scene boundary

Before data crosses the Viewer boundary:

- semantic geometry is revalidated;
- Sketch-local geometry is mapped through the current accepted Sketch frame;
- numeric display text is formatted outside the native provider;
- durable CAD identity is stripped.

The provider-neutral Viewer scene is conceptually a list of identity-free display items containing only what presentation requires, for example:

```text
DiagnosticDimensionSceneItem
  kind
  exact mapped geometric anchors / curve data
  formatted label
  bounded visual role/style hints
```

No Viewer scene item contains:

- `EntityId`;
- `SketchId`;
- `ProfileId`;
- durable `DimensionId`;
- provider-native topology identity as CAD intent.

Because overlays are non-interactive, R8C needs no Viewer hit-query token for them.

## 12. Geometry and numeric authority

R8C follows the same authority boundary as R8A/R8B:

```text
semantic Sketch geometry
→ provider-neutral diagnostic descriptor
→ frame mapping + formatting
→ Viewer presentation
```

The Viewer/Qt/OCCT layer must not independently recompute:

- Line Length;
- Circle Diameter;
- Arc Radius;
- Arc Sweep magnitude or branch.

Native dimension/helper classes may be used as drawing mechanisms only if they do not become numeric/semantic authority.

Provider tessellation, sampled Circle/Arc chords, screen pixels and native presentation-object geometry must never define the diagnostic value.

## 13. Formatting and units boundary

Neutral semantic values remain:

- linear values in current Sketch coordinate scale;
- angles in radians.

UI/Viewer labels may format:

- Line Length as the numeric linear value;
- Circle Diameter with a conventional diameter prefix/symbol;
- Arc Radius with `R`;
- Arc Sweep in degrees with a degree symbol.

R8C introduces no `mm`, `in` or other physical-unit label and no conversion.

The decimal precision/rounding policy should reuse the existing R8 diagnostic presentation policy; exact formatting details are D1.

R10 remains responsible for document-unit architecture and richer quantity formatting.

## 14. Layout and collision behavior

R8C is the first SS2 surface with multiple simultaneous viewport diagnostic labels, so layout is presentation policy, not CAD semantics.

Required properties:

- layout is deterministic for the same scene/camera/viewport;
- annotation geometry is projection-aware;
- text remains legible across normal DPI/resize/zoom conditions;
- preferred offsets do not encode CAD meaning;
- screen-space collision checks may consider annotation/text bounds;
- collision/layout tolerance never becomes geometric tolerance;
- provider/native result ordering never establishes semantic priority;
- layout changes do not modify numeric values;
- no annotation becomes selectable because of layout.

For multiple annotations the provider may use a finite ordered set of candidate offsets/leader placements.

If no collision-free candidate exists:

- use a deterministic fallback even if some overlap remains;
- do not silently delete a semantic dimension merely to make the scene look cleaner.

R8C does not require a global optimization solver for annotation layout.

An unbounded iterative/pairwise layout engine or speculative reusable Drawing annotation framework is outside scope.

## 15. Transient edit/preview policy

Show Dimensions derives from current authoritative accepted Sketch geometry.

During a geometry-changing transient preview, showing stale authored dimensions beside visibly moved preview geometry would be misleading.

Therefore:

- the Show Dimensions mode remains logically active;
- its overlays are temporarily suppressed while a geometry-changing creation/direct-manipulation/common-transform/COPY preview owns transient geometry;
- no preview geometry is dimensioned in R8C;
- after commit, overlays regenerate from the newly accepted authored geometry;
- after cancel, overlays regenerate from the unchanged authored geometry.

This deliberately avoids turning R8C into R10 Dynamic Input/live-edit diagnostics.

Selection rectangle drag by itself is not a geometry-changing preview. Selection-scope overlays update after the semantic selection result is accepted.

## 16. Coexistence with R8A/R8B Measure

Show Dimensions is independent of the Measure tool.

While Show Dimensions is active:

- ordinary R8A quick Measure may still be used;
- R8B Between may still be used;
- the current R8B target markers/highlights/single-relation cue may coexist with R8C overlays.

R8B interaction visuals receive stronger local emphasis where the provider must establish a visual hierarchy. Exact z-order/styling is D1.

R8C does not turn R8B relation results into persistent/multi-relation overlays.

Esc behavior remains owned by the active tool. Esc used by Measure/Between does not silently turn Show Dimensions off.

## 17. Selection invariants

R8C never changes ordinary semantic selection.

Specifically, turning Show Dimensions on/off or changing scope:

- does not select entities;
- does not clear selection;
- does not change primary;
- does not activate grips;
- does not create a selection snapshot;
- does not change selection hit priority.

Diagnostic overlay primitives are excluded from all selection, grip and marker query paths.

## 18. Construction geometry

Construction entities participate in R8C exactly like Regular entities for numeric meaning.

Required canonical annotation families are the same:

- Construction Line → Length;
- Construction Circle → Diameter;
- Construction Arc → Radius + Sweep Angle.

Construction/Regular may receive distinguishable presentation styling as D1.

R8C never changes `EntityRole` and never affects Profile/region participation.

## 19. History and persistence invariants

Show Dimensions operations create:

- no DocumentRevision change;
- no dirty-state change;
- no EntityId/ProfileId consumption;
- no Undo/Redo entry;
- no command/transaction mutation;
- no Part schema change;
- no Project/user-setting persistence.

Undo/Redo while Show Dimensions is active:

1. follows ordinary global history semantics;
2. preserves the runtime Show Dimensions mode if the same Sketch edit context survives;
3. clears the old overlay scene;
4. rebuilds it from the post-history authoritative geometry/selection.

No stale annotation from the pre-history model may remain visible.

## 20. Presentation failure policy

R8C follows E2's fail-closed presentation doctrine.

If diagnostic-overlay scene creation/replacement fails:

- authored CAD state remains authoritative and unchanged;
- selection/history remain unchanged;
- the failed overlay scene is not partially treated as current truth;
- stale R8C overlays are cleared rather than left with old numbers;
- a bounded runtime presentation diagnostic may be reported;
- a later normal refresh/mode change may rebuild from current semantic state.

Provider failure never rolls back a successful CAD mutation that occurred before the presentation refresh.

## 21. Performance boundary

Whole-Sketch mode may produce many annotations.

R8C therefore requires:

- one linear semantic pass over the relevant entity set to derive descriptors;
- bounded per-annotation presentation/layout work;
- no mandatory unbounded all-pairs/global optimization algorithm;
- no durable cache or secondary model authority merely for overlay performance.

Exact implementation structures are D1.

If acceptable whole-Sketch behavior cannot be achieved without introducing:

- a persistent annotation cache;
- a generic retained annotation subsystem;
- a Product-visible hard entity/dimension cap;
- differential Viewer architecture beyond the bounded R8C scene;

implementation must stop for Owner review.

Existing E2 presentation-scale evidence remains relevant. R8C creates no new general performance guarantee for arbitrarily large Sketches.

## 22. CAD input and Repeat Last Command

R8C adds no semantic PointRequest, quantity request or text grammar.

The workspace-global CAD input router remains unchanged and domain-neutral.

Show Dimensions is not Repeat Last Command identity.

Measure/Between command semantics from R8A/R8B remain unchanged.

## 23. No speculative generic annotation framework

R8C is a Sketch diagnostic overlay.

Do not create a shared universal `AnnotationSystem`, `DimensionDocument`, Drawing annotation engine or cross-domain layout subsystem merely because future Drawing may also need dimensions.

Reusable pure mechanics may be factored only where they are intrinsic and already justified by the R8C implementation.

Future authored Part dimensions or Drawing dimensions retain their own domain semantics and must not inherit R8C runtime diagnostic identity by accident.

## 24. Verification footprint

Prefer existing test executables so ordinary implementation iterations remain CI-03 FOCUSED.

Expected affected surfaces:

- `sk02a_shared_2d_core_test` / `sk02a.shared_2d_core` — canonical descriptor generation and values;
- `sk03a_viewer_sketch_contracts_test` / `sk03a.viewer_sketch_contracts` — identity-free overlay scene contract;
- `sk04b_part_viewport_selection_bridge_test` and/or existing native Viewer contract coverage — overlay exclusion from hit/query semantics;
- `sk04c_part_sketch_interaction_controller_test` / `sk04c.part_sketch_interaction_controller` — runtime mode/scope/lifecycle/suppression/rebuild;
- `sk01_workbench_sketch_host_test` / `sk01.workbench_sketch_host` — real Inspect UI workflow;
- existing Qt/OCCT native Viewer test coverage — actual dimension graphics, DPI/resize and non-selectability;
- E2/provider-failure coverage where the implementation touches shared presentation-degraded handling.

Exact target choice is D1.

Verification infrastructure must not be weakened or changed merely to obtain a cheaper tier.

Final merge evidence remains exact-head Windows FULL.

## 25. Automated acceptance coverage

At minimum verify:

1. Show Dimensions mode has exactly Off/Selection/Sketch semantic states;
2. mode is runtime-only and starts Off for a new Sketch edit session;
3. changing Show Dimensions mode does not change ordinary Sketch selection;
4. Selection scope with empty selection produces no annotations;
5. Selection scope dimensions every selected supported entity independently;
6. Selection scope does not infer pairwise relations from multi-selection;
7. Sketch scope dimensions all current Line/Circle/Arc entities;
8. Sketch scope is independent of ordinary selection membership;
9. Line produces exactly one aligned Length semantic descriptor;
10. Line length matches R8A semantic measurement and is Start/End reversal invariant;
11. Circle produces exactly one Diameter descriptor;
12. Circle Diameter equals exactly twice semantic Radius;
13. Circle does not automatically produce Radius/Circumference/Area overlays;
14. Arc produces exactly Radius and Sweep descriptors;
15. Arc Radius uses semantic radius and exact signed-sweep midpoint geometry;
16. Arc Sweep display magnitude is `abs(signedSweep)`;
17. Arc angular geometry preserves the authored short/long sweep branch rather than replacing it with a smaller angle;
18. Arc does not automatically produce Arc Length/Start Angle/End Angle overlays;
19. Regular and Construction use identical numeric formulas;
20. semantic descriptors derive only from authoritative semantic/evaluated Sketch geometry;
21. Viewer scene contains no EntityId/SketchId/ProfileId/durable Dimension identity;
22. Viewer/provider receives formatted labels rather than owning Product unit semantics;
23. provider tessellation/native geometry does not determine numeric values;
24. overlay scene is non-selectable;
25. overlay scene is non-editable;
26. overlay scene is non-snappable;
27. overlay primitives do not alter normal entity/grip/marker hit priority;
28. layout is deterministic for identical viewport inputs;
29. relayout after zoom/pan/orbit/resize changes presentation only;
30. layout collision logic does not change values or CAD geometry;
31. collision fallback does not silently remove a semantic dimension;
32. geometry-changing transient preview suppresses R8C overlays;
33. commit regenerates overlays from accepted new geometry;
34. cancel regenerates overlays from unchanged authored geometry;
35. ordinary selection changes regenerate Selection-scope overlays;
36. tool switch inside the same Sketch does not itself disable Show Dimensions;
37. Measure quick mode may coexist with Show Dimensions;
38. Between mode may coexist with Show Dimensions;
39. R8B relation cue/markers remain tool-local and are not absorbed into R8C;
40. Esc tool hierarchy does not silently turn Show Dimensions off;
41. Undo/Redo clears stale overlays and rebuilds from post-history state;
42. active Sketch/Document replacement clears Show Dimensions mode/scene;
43. Show Dimensions causes no revision/dirty/history/identity/persistence mutation;
44. overlay provider failure clears stale overlay truth and preserves authored state;
45. no generic annotation/document subsystem is introduced;
46. no Command Line grammar or Repeat Last Command identity is introduced;
47. existing R8A/R8B Measure behavior remains unchanged;
48. existing selection/grip/transform/COPY/Grip Copy/Profile/CAD-input regressions remain green;
49. exact-head Windows FULL passes;
50. required internal and PL/EN Product documentation plus Product Browser freshness pass.

## 26. Manual Windows verification

Final candidate requires Owner verification on the exact FULL-tested head.

Minimum manual checklist:

- activate Show Dimensions → Selection;
- select one Line and verify one aligned length annotation with readable witnesses/arrows/text;
- reverse/select geometrically equivalent Line direction where practical and verify the displayed length is unchanged;
- select one Circle and verify exactly one diameter annotation;
- select one Arc and verify radius plus sweep-angle annotations;
- verify a long Arc displays its authored long sweep rather than the smaller complementary angle;
- multi-select Line + Circle + Arc and verify all intrinsic annotations appear without inferred pairwise relations;
- clear selection in Selection mode and verify annotations disappear while the mode stays active;
- switch to Sketch scope and verify all active-Sketch Line/Circle/Arc entities receive the canonical overlay set;
- include Construction Line/Circle/Arc and verify they are dimensioned without changing Profile behavior;
- zoom, pan, orbit and resize; verify annotations remain readable, stable and attached to the intended geometry;
- exercise a moderately dense Sketch and inspect deterministic collision handling/readability;
- verify overlays cannot be selected, grip-edited or used as measurement markers/snap-like targets;
- start Line creation or a geometry-changing grip/Move/Copy/Rotate/Scale/Mirror preview and verify Show Dimensions overlays are suppressed rather than showing stale values;
- commit and cancel such edits and verify overlays return with correct authoritative values;
- with Show Dimensions active, run normal R8A Measure and R8B Between; verify their Operations/markers/current-relation cue still work and remain visually distinguishable;
- verify Esc hierarchy for Measure/Between does not disable Show Dimensions;
- verify Undo/Redo refreshes dimensions without dirty/history side effects from the overlay itself;
- switch/close/reopen Sketch/Document and verify Show Dimensions is runtime-only and does not persist;
- regression smoke ordinary selection/grips, COPY/Grip Copy and Profile.

Manual review must explicitly judge:

1. text and arrow readability at normal zoom/DPI;
2. whether line/circle/arc annotation placement is visually unambiguous;
3. whether dense-scene collision handling remains useful rather than chaotic;
4. whether Construction diagnostics remain distinguishable without looking like authored constraints;
5. whether R8C overlays are visually distinct from edit grips, R8B markers/cues and authored Sketch geometry.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: R8C adds a user-visible multi-annotation viewport diagnostic mode and a new provider-neutral diagnostic-dimension presentation boundary.

Internal documentation must explain:

- Show Dimensions runtime ownership and Off/Selection/Sketch modes;
- the canonical intrinsic dimension set;
- why pairwise relations remain explicit Measure/Between operations;
- semantic descriptor ownership versus identity-free Viewer scene;
- authored-sweep semantics for Arc angular overlays;
- transient-preview suppression;
- layout/collision responsibility and non-semantic screen-space policy;
- coexistence with R8A/R8B;
- provider-failure and lifecycle behavior;
- non-persistence/non-selection/non-snap invariants;
- separation from future authored dimensions, constraints, Drawing annotations and R10/R11 features.

Product PL/EN documentation must explain:

- where Show Dimensions is activated;
- Selection versus Sketch scope;
- which dimensions are shown for Line/Circle/Arc;
- Construction participation;
- current unit-label limitation;
- read-only/non-editable nature;
- interaction with Measure/Between;
- current limitations.

Generated Product Browser must be regenerated and deterministic.

## 28. Stop conditions

Stop for Owner review if implementation requires:

- durable/persisted dimension or sub-element identity;
- persistence/schema changes;
- authored dimension/constraint/solver semantics;
- editable or draggable dimension annotations;
- automatic pairwise relation inference;
- OSNAP/tracking/inference/candidate resolution;
- coordinate/ordinate dimension architecture;
- intrinsic Origin dimension architecture;
- unit-system/document-unit architecture;
- live transient-preview dimension computation;
- Viewer-derived numeric measurement;
- provider tessellation as diagnostic geometry authority;
- overlay hit-testing as CAD selection;
- a generic cross-domain annotation framework;
- a persistent/retained overlay cache that becomes a second truth;
- a Product-visible hard annotation/entity cap;
- an unbounded general annotation-layout optimizer;
- structural editing;
- Part solid modeling.

These are D2/D3 scope expansions or separate later milestones.

## 29. R8 completion boundary

R8C is the final planned bounded slice of Roadmap v1.5 milestone R8.

If R8C completes successfully:

- R8A remains completed;
- R8B remains completed;
- R8C becomes completed;
- the R8 milestone becomes completed.

R8C completion does **not** automatically activate R9.

Roadmap v1.5 then makes R9 Rectangle + Construction UI the next planned Sketcher milestone, but R9 production mutation still requires its own separate Owner-accepted Work Contract.

## 30. Completion boundary

R8C may become ACTIVE only after explicit Owner acceptance of this Work Contract.

Completion requires:

- accepted Off/Selection/Sketch runtime semantics preserved;
- accepted canonical intrinsic dimension set preserved;
- no authored-dimension/constraint/OSNAP/unit-system creep;
- provider-neutral semantic values and identity-free Viewer scene preserved;
- focused/affected iteration evidence;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- required internal and PL/EN Product documentation/current Browser;
- work-only CLOSURE closeout.

Until explicit Owner acceptance, R8C remains PROPOSED — INACTIVE and no product implementation is authorized.
