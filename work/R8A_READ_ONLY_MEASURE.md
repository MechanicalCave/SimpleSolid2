# R8A — Read-only Measure Core + Single-Selection Diagnostics

**Status:** PROPOSED — INACTIVE  
**Proposed:** 2026-09-29  
**Owner acceptance:** pending  
**Decision class:** D2 Sketch command/tool grammar and public diagnostic semantics + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009, ADR-0011  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.5  
**Milestone:** R8 — Inspect / Measure / diagnostic dimensions — first bounded slice

## 1. Context

Roadmap v1.5 makes R8 the next planned Sketcher milestone and establishes three sequencing guardrails.

For R8 specifically:

- diagnostics are read-only;
- measurement must consume authoritative Sketch geometry rather than Viewer tessellation;
- R8 must not prematurely freeze a durable sub-element identity model merely to display measurements;
- authored dimensions, constraints and solver remain later work;
- R8 precedes Rectangle/Construction UI, Precision Input, OSNAP/Inference and structural editing.

ADR-0008 already requires that read-only measurement/diagnostic queries:

- do not dirty the Document;
- do not create Undo entries;
- do not silently repair geometry;
- do not create authored relations/dimensions merely because a condition is observed.

ADR-0009 requires measurement to consume evaluated geometry where evaluation exists and explicitly does not freeze a universal public SubElement type.

Current Sketch authored geometry is Line, Circle and Arc. Package F has already established the durable `Regular | Construction` role. Both roles remain semantically measurable.

R8 contains three materially different concerns:

1. provider-neutral geometric measurement;
2. relational/sub-element target selection;
3. Viewer diagnostic dimension overlays.

R8A activates only the first concern plus a bounded single-entity Measure tool. Relational Measure and Show Dimensions remain later R8 slices.

## 2. Goal

Add a provider-neutral, read-only **Measure** capability for exactly one Line, Circle or Arc at a time.

Required user workflow:

```text
optional: select exactly one Sketch entity
→ activate Measure
→ if one entity was selected, show its diagnostics immediately
→ otherwise click a Line/Circle/Arc to make it the runtime Measure target
→ diagnostics update
→ click another entity to replace the Measure target
→ Esc ends Measure
```

Measure must not author CAD state, allocate identities, mutate ordinary selection or create a second geometric authority.

## 3. Scope boundary

### 3.1 Scope IN

R8A includes:

- one provider-neutral Shared 2D single-entity measurement query;
- Line diagnostics;
- Circle diagnostics;
- Arc diagnostics;
- runtime Measure tool/session state;
- selection-first entry when exactly one supported entity is selected;
- command-first target pick when no unambiguous single selection exists;
- continuous retargeting by click while Measure remains active;
- toolbar/Operations activation;
- top-level Command Line command `MEASURE`;
- bounded Operations/status presentation of current result;
- Regular and Construction geometry;
- controller/history/context cancellation behavior;
- automated and manual Windows verification;
- internal and PL/EN Product documentation.

### 3.2 Scope OUT

R8A does not authorize:

- point↔point distance;
- DeltaU/DeltaV between arbitrary picked points;
- point↔line perpendicular distance;
- line↔line angle;
- center↔center or minimum curve distance;
- arbitrary runtime/free point targets;
- persistent or universal SubElement type;
- selectable Line Start/End/Midpoint, Circle Quadrant, Arc Midpoint measurement targets;
- Show Dimensions or any dimension overlay in the Viewer;
- authored dimensions;
- constraints/solver/Auto-Constraint;
- unit-system architecture or document units;
- expression parsing or R10 precision-input work;
- OSNAP/tracking/inference;
- RMB context;
- Rectangle/Construction creation UI;
- Trim/Split/Join;
- Part solid modeling;
- persistence/schema changes.

Those remain separately bounded future work.

## 4. Measurement ownership

### 4.1 Shared 2D owns geometric measurement meaning

The single-entity calculation belongs in provider-neutral Shared 2D / Sketch code.

It must consume semantic/evaluated Line/Circle/Arc geometry and must not depend on:

- Qt;
- OCCT;
- Viewer presentation tokens;
- sampled display chords;
- pixel coordinates;
- screen/DPI state.

A later solver may make evaluated geometry differ from authored geometry. The measurement API must remain conceptually compatible with consuming evaluated geometry rather than baking in the assumption that authored storage is always final.

### 4.2 Application/UI own adapters, not formulas

The Part-hosted Sketch interaction path owns:

- active Measure session;
- runtime target EntityId;
- mapping existing semantic Viewer selection hits to EntityId;
- tool activation/cancel/context lifetime;
- projection of the provider-neutral result into Operations/status UI.

Qt/UI code must not independently calculate lengths, angles, areas or arc geometry.

## 5. No durable sub-element identity

R8A deliberately measures whole entities only.

It must not introduce:

- `SubElementId`;
- persisted element roles;
- durable midpoint/quadrant identities;
- provider-token-backed references;
- authored Point entities.

Existing runtime grip roles remain runtime interaction concepts and are not promoted to durable measurement identity by this contract.

If implementation reveals that even whole-entity R8A requires a new durable sub-element/reference contract, stop for Owner review.

## 6. Single-entity semantic results

The provider-neutral result must identify the entity kind and expose the following geometric values.

### 6.1 Line

For authored/evaluated Start `S` and End `E`:

```text
DeltaU = E.u - S.u
DeltaV = E.v - S.v
Length = hypot(DeltaU, DeltaV)
Angle = atan2(DeltaV, DeltaU)
```

Required result fields:

- length;
- DeltaU;
- DeltaV;
- directed angle relative to Sketch +U from Start → End.

The semantic angle is the normal finite `atan2` result in radians. R8A does not invent a new authored orientation or normalize the Line into an undirected object.

### 6.2 Circle

Required result fields:

- radius;
- diameter = `2 * radius`;
- circumference = `2 * pi * radius`;
- area = `pi * radius^2`.

### 6.3 Arc

Required result fields:

- radius;
- canonical start angle;
- derived end angle = `start_angle + signed_sweep_angle`;
- signed sweep angle;
- positive arc length = `radius * abs(signed_sweep_angle)`.

The provider-neutral core preserves the existing Arc start/signed-sweep semantics. It must not change Arc canonical geometry merely to normalize a displayed angle.

UI formatting may show an equivalent wrapped angle for readability only if signed sweep remains visible and no semantic value is changed. Exact presentation formatting is D1.

### 6.4 Role

The result may expose the current semantic `Regular | Construction` role for display/context, but role does not change measurement formulas.

## 7. Numeric and failure policy

R8A introduces no new Product tolerance.

Measurement uses the accepted valid finite geometry already enforced by Sketch model/evaluation boundaries.

Required behavior:

- invalid/missing target → no stale result;
- non-finite derived result → structured/bounded diagnostic and no displayed stale result;
- no screen-space or snap tolerance influences the geometric value;
- no automatic repair, welding, gap healing or approximation is allowed;
- Circle/Arc Viewer tessellation must never define circumference/area/arc length.

Any need for a new Product tolerance or coordinate/size limit is a stop condition.

## 8. Units and formatting boundary

R10 owns future explicit Units / quantity grammar.

R8A therefore must not pretend that a document-unit architecture already exists.

Provider-neutral results are expressed in the same Sketch-local physical coordinate scale as current semantic geometry:

- linear values in current Sketch coordinate units;
- area in squared current Sketch coordinate units;
- semantic angles in radians.

The UI may format angles in degrees for human readability and may use bounded numeric formatting, but it must not label values as `mm`, `in`, etc. unless such unit meaning already exists through an accepted Product contract.

R10 may later add explicit unit conversion/formatting without changing R8A geometric meaning.

## 9. Tool and selection grammar

### 9.1 Activation

Measure is a Sketch tool adapter available only while valid Sketch edit is active.

Activation is available through:

- toolbar/Operations;
- Command Line token `MEASURE`.

No single-letter alias is introduced.

### 9.2 Selection-first

On activation:

- if normal semantic selection contains exactly one supported Line/Circle/Arc, that EntityId becomes the initial runtime Measure target;
- if selection is empty, Measure waits for a target click;
- if selection contains more than one entity, R8A does not silently choose primary or first storage order; it reports that R8A requires one target and waits for an explicit target click.

Existing normal selection is preserved.

### 9.3 Command-first target pick

While Measure is active, clicking a valid Line/Circle/Arc through the existing provider-neutral semantic hit bridge:

- replaces the runtime Measure target;
- recomputes the result;
- does **not** add/remove/toggle ordinary Sketch selection;
- does not create authored mutation.

Blank click clears only the current Measure target/result or is a bounded no-op; exact choice is D1 provided ordinary semantic selection is unchanged and no stale result is shown.

Measure target state must use EntityId after semantic hit resolution. Viewer presentation token must not become the target identity.

### 9.4 Lifetime

Measure remains active for repeated target inspection until:

- Esc;
- another Sketch tool is activated;
- Sketch edit ends;
- active Sketch/Document context changes;
- Undo/Redo boundary cancels the active tool before ordinary global history.

Ending Measure clears only runtime Measure state and preserves ordinary selection.

## 10. Repeat Last Command and CAD input precedence

R8A `MEASURE` is not added to the SK-07D repeatable-command set.

Activating or using Measure must not replace the remembered last repeatable creation/transform command.

While Measure is active:

- it owns its bounded target-pick interaction;
- it introduces no PointRequest;
- numeric CAD input has no new R8A meaning;
- unknown Command Line tokens continue to follow existing context-first rejection rules;
- switching via an accepted top-level Sketch command cancels Measure and activates the requested tool according to existing tool-switch semantics.

The workspace-global WB-02 router remains domain-neutral.

## 11. Read-only history and document invariants

Entering Measure, choosing targets and displaying results must create:

- no DocumentRevision change;
- no dirty-state change;
- no EntityId/ProfileId consumption;
- no Undo entry;
- no persistence/schema data;
- no authored command/transaction.

This is an explicit exception from the mutation path because Measure performs no durable mutation at all.

Save/Close/Reopen must not persist Measure tool state or target/result.

## 12. UI surface

R8A should use the existing right-side editor/Operations architecture rather than a modal measurement dialog.

Minimum product surface:

- an **Inspect** or equivalent bounded toolbar/Operations group containing Measure;
- current target kind/identity information sufficient for user comprehension;
- labeled result fields appropriate to Line/Circle/Arc;
- bounded diagnostic when no valid target is available;
- crosshair/target cursor consistent with ADR-0009 measurement-tool direction.

Exact widget layout, decimal precision and localization wording are D1 provided semantic values remain unambiguous.

R8A does not add Viewer dimension graphics, leaders, arrows or text overlays.

## 13. Construction geometry

Construction Line/Circle/Arc is measurable exactly like Regular geometry.

Measure must not:

- convert Construction to Regular;
- include/exclude an entity based on Profile region participation;
- change role;
- create profile boundaries.

This confirms that Construction is a semantic authoring role, not a prohibition on engineering inspection.

## 14. Verification footprint

R8A should prefer existing test executables where practical so ordinary iterations can use CI-03 FOCUSED without modifying verification infrastructure merely to add test content.

Expected focused surfaces include the semantic equivalents of:

- `sk02a_shared_2d_core_test` / `sk02a.shared_2d_core` — provider-neutral formulas/failure cases;
- `sk04c_part_sketch_interaction_controller_test` / `sk04c.part_sketch_interaction_controller` — Measure session/selection/history semantics;
- `d_cad_input_semantics_test` / `d.cad_input_semantics` — `MEASURE` routing and context-first behavior if the semantic endpoint changes;
- `sk01_workbench_sketch_host_test` / `sk01.workbench_sketch_host` — real Workbench UI/tool wiring.

Exact target choice is D1.

Verification tooling/CMake must not be distorted merely to preserve a FOCUSED classification. If correct architecture requires CMake/build-graph changes, they receive the stronger CI classification required by CI-03.

Final merge evidence remains exact-head Windows FULL.

## 15. Automated acceptance coverage

At minimum verify:

1. provider-neutral Line measurement returns exact expected length, DeltaU, DeltaV and Start→End +U angle;
2. reversed Line direction produces the correspondingly directed angle/Delta signs;
3. Circle returns radius/diameter/circumference/area from semantic radius;
4. Arc returns radius/start/end/signed sweep and positive arc length for CW and CCW arcs;
5. Arc end is derived from `start + sweep` without changing canonical Arc geometry;
6. Regular and Construction entities produce the same geometric measurement for equal geometry;
7. missing/invalid target yields no stale result;
8. activation with exactly one selected entity targets it immediately;
9. activation with empty selection waits for target;
10. activation with multi-selection does not silently choose primary/storage order;
11. Measure target pick does not mutate normal selection;
12. repeated target picks replace only runtime Measure target/result;
13. Measure creates no revision/dirty/history/identity mutation;
14. Esc clears Measure state and preserves ordinary selection;
15. tool switch clears Measure state and preserves accepted tool-switch semantics;
16. Undo/Redo cancels Measure before global history;
17. Sketch/Document replacement clears Measure runtime state;
18. `MEASURE` Command Line activation routes to the same semantic tool;
19. Measure does not replace Repeat Last Command identity;
20. Viewer/provider sampling is absent from provider-neutral formulas;
21. Construction remains measurable;
22. Save/Close/Reopen contains no Measure persistence;
23. existing selection, grips, transforms, Profile and CAD-input regressions remain green;
24. exact-head Windows FULL passes;
25. required internal and PL/EN docs plus Product Browser freshness pass.

## 16. Manual Windows verification

Final candidate requires bounded Owner verification:

- create/select one Line, activate Measure → Length/DeltaU/DeltaV/Angle appear;
- inspect one Circle → Radius/Diameter/Circumference/Area appear;
- inspect one CW or CCW Arc → Radius/Start/End/Signed Sweep/Arc Length appear;
- toggle/create a Construction entity available in the current build and verify it is measurable;
- preserve an existing normal selection, activate Measure, click another entity → result changes but selection does not;
- with multiple entities selected, activate Measure → no hidden target is chosen; explicitly click one entity to inspect it;
- activate via Command Line `MEASURE`;
- Esc → Measure ends and prior normal selection remains;
- confirm no dirty marker/Undo entry appears from measuring;
- quick regression smoke for normal selection/grips, COPY/Grip Copy and Profile presentation.

## 17. Documentation impact

Internal documentation must explain:

- provider-neutral measurement ownership;
- runtime Measure target versus ordinary selection;
- evaluated/semantic geometry authority;
- no durable sub-element identity;
- read-only history/persistence boundary;
- relationship to later R8B/R8C.

Product PL/EN documentation must explain the Measure workflow and current single-entity limitation.

Generated Product Browser must be regenerated and deterministic.

## 18. Stop conditions

Stop for Owner review if implementation requires any of the following:

- durable sub-element identity or universal public SubElement API;
- persistence/schema change;
- authored dimension or relation state;
- solver/constraint behavior;
- new Product geometric tolerance/coordinate limit;
- Viewer/OCCT-derived measurement authority;
- changing global selection grammar;
- changing EntityId lifecycle;
- public Viewer dimension-overlay contract;
- unit-system/document-unit architecture;
- relational Measure beyond this single-entity scope;
- structural editing or Part solid-modeling semantics.

These are D2/D3 scope expansions.

## 19. R8 continuation boundary

R8A is intentionally not the whole R8 milestone.

After R8A completion, R8 still requires separately bounded work for:

### R8B — Relational Measure / semantic runtime targets

Expected later scope includes:

- point↔point distance, DeltaU/DeltaV and direction;
- line↔line angle;
- point↔line perpendicular distance;
- center distance and justified minimum geometric distance;
- semantic runtime element roles such as Line Start/End/Midpoint, Circle Center/Quadrants and Arc Center/Start/End/Midpoint;
- free-point input only if justified by concrete UX.

R8B must preserve the roadmap guardrail against prematurely freezing durable sub-element identity.

### R8C — Show Dimensions diagnostic overlays

Expected later scope includes:

- current-selection and whole-Sketch read-only overlays;
- provider-neutral diagnostic presentation data;
- bounded Viewer API/presentation implementation;
- overlays remain non-selectable, non-editable, non-snappable, without EntityId and without persistence.

Neither R8B nor R8C is activated by accepting R8A.

## 20. Completion boundary

R8A may become ACTIVE only after explicit Owner acceptance of this Work Contract.

Completion requires:

- accepted scope preserved;
- focused/affected iteration evidence;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- required documentation/current Product Browser;
- work-only CLOSURE closeout.

R9 remains inactive until the complete R8 milestone is closed or the Owner explicitly amends sequencing.
