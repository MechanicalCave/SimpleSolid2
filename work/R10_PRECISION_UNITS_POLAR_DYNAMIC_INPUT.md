# R10 — Precision Input / Units / Polar / Dynamic Input

**Status:** PROPOSED — INACTIVE  
**Proposed:** 2026-09-29  
**Owner acceptance:** pending  
**Decision class:** D2 shared semantic precision-input / units / runtime mode grammar + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009, ADR-0010, ADR-0011  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.6  
**Predecessor:** R9 Rectangle + Construction Authoring Surface — completed  
**Milestone:** R10 — Precision Input / Units / Polar / Dynamic Input

## 1. Context

R9 completed conscious Rectangle/Construction authoring and left R10 as the next planned Sketcher milestone.

The current product already has the correct transport and semantic seams for a precision milestone:

- one workspace-global CAD input buffer and endpoint routing;
- context-first submission precedence;
- one provider-neutral Sketch interaction state;
- `PointRequest` plus pointer resolution;
- Direct Distance for the currently supported point stages;
- keyboard-first input without Command Line focus;
- provider-neutral Sketch-local U/V coordinates;
- existing Rotate and Scale semantic transform stages;
- no OSNAP/tracking/inference yet.

The current precision capability is intentionally narrow. A submitted scalar can mean Direct Distance only where the active PointRequest explicitly allows it. Circle radius, Rotate angle, Scale factor, absolute/relative coordinates, polar coordinates, unit suffixes, Polar direction assistance and Dynamic Input are not yet implemented.

R10 must extend that one existing input architecture. It must not create per-tool parsers, a second command buffer, a second point resolver or a Dynamic-Input-only state machine.

## 2. Goal

R10 makes existing Sketch creation/editing deliberately placeable by engineering values.

The target user model is:

```text
active semantic request
        ↓
pointer + explicit numeric locks / typed quantity
        ↓
Polar directional magnet where applicable
        ↓
one provider-neutral resolved semantic value
        ↓
existing preview / validation / command / transaction path
```

Command Line and Dynamic Input are two adapters to the same active request and the same locks.

R10 additionally absorbs the previously accepted non-blocking Construction presentation carry-over: dashed Construction presentation must use a fixed visual dash-gap cadence rather than normalizing the dash pattern to entity length.

## 3. Decision summary

Subject to explicit Owner acceptance, R10 adopts these bounded decisions:

1. Existing workspace CAD input transport and ADR-0011 context-first routing remain authoritative.
2. Existing `PointRequest` remains the point-acquisition semantic owner and is expanded rather than replaced.
3. R10 introduces typed non-point semantic requests only where concrete current tools require them: physical Length, Angle and dimensionless Scalar.
4. No universal speculative request hierarchy is created beyond these concrete clients.
5. Sketch/Part physical model length is formalized as canonical millimetres for R10 precision semantics.
6. Existing persisted Sketch coordinate doubles are interpreted 1:1 as millimetres; R10 does not geometrically rescale existing files.
7. Part gains a durable display/input length-unit preference using shared Core unit semantics.
8. Initial supported length display/input units are `mm`, `cm`, `m`, `in` and `ft`.
9. Existing documents without a unit property load as `mm`.
10. Changing display/input unit does not change authored geometry; it changes formatting and the meaning of future unitless length input.
11. Explicit unit suffixes always override the current document display/input unit.
12. Angles are displayed in degrees in R10; `deg` and `rad` are accepted input units.
13. Both `.` and `,` are accepted decimal separators independent of UI locale.
14. Cartesian components are separated only by semicolon `;`, never comma.
15. Absolute Cartesian syntax is `U;V`.
16. Relative Cartesian syntax is `@dU;dV`.
17. Relative polar syntax is `@Distance<Angle`.
18. Direct Distance remains a bare length expression only at stages that explicitly allow Direct Distance.
19. Rectangle opposite-corner input accepts absolute/relative Cartesian and relative polar point input; `@Width;Height` is therefore the deliberate width/height workflow.
20. Rectangle does not reinterpret one bare scalar as width, height, side, diagonal or square size.
21. Circle Center is a point request; Circle Radius becomes a typed positive Length request.
22. Arc Start / Through / End become precision-capable point requests; no new Arc construction method is introduced.
23. Rotate final placement accepts an explicit Angle value as a numeric lock/commit path while pointer placement remains valid.
24. Rotate typed-angle sign is explicit and pointer-independent: bare/positive `30` means +30° counter-clockwise and `-30` means 30° clockwise.
25. Scale final placement accepts an explicit positive dimensionless Scalar factor while pointer placement remains valid.
26. Grip Scale captures the current pivot→pointer radius as factor 1.0 on mode entry (or the first later valid radius), and explicit Factor input fully overrides subsequent pointer motion.
27. Move/Copy/grip point requests accept coordinate/polar/direct-distance forms through the same resolver.
28. Non-center grip Space CycleEditMode is `Reshape → Move → Rotate → Scale → Mirror → Reshape`.
29. Center grip Space CycleEditMode is `Move → Rotate → Scale → Mirror → Move`.
30. Grip mode cycling preserves active grip, interaction-start pivot and frozen semantic source geometry and never compounds preview.
31. Ordinary Mirror remains a two-point axis workflow.
32. Grip Mirror fixes Axis Start at the active grip/pivot and additionally accepts a typed Axis Angle measured from Sketch +U.
33. R10 has no separate Ortho mode. Polar is the single runtime directional-attraction mechanism; classic Ortho behavior is represented by a Polar full-turn subdivision of four directions, i.e. `360/4 = 90°`.
34. Polar is a magnet, not a hard angular quantizer: it captures pointer direction only inside a bounded attraction neighborhood of a configured track; outside that neighborhood raw pointer direction remains free.
35. Polar primary spacing is an Angle expression. The shared arithmetic parser therefore accepts forms such as `45`, `90/2`, `360/8`, `360/12` or `360/7`.
36. Polar default primary spacing is `360/8 = 45°`.
37. Polar spacing must be finite, positive and no greater than 180°. Track generation is bounded to one full turn around the selected reference so arbitrary valid positive spacing never creates an unbounded/dense track set.
38. Polar supports Absolute reference from Sketch +U and Relative reference from the current operation reference direction.
39. Additional Polar angles remain available as runtime offsets augmenting the primary family.
40. Explicit complete coordinates / numeric locks outrank Polar capture; Polar capture outranks raw pointer only while the pointer is inside the attraction neighborhood.
41. Polar and Dynamic Input mode/configuration state is runtime-only in R10 and retained across tool changes within one active Sketch edit session.
42. Polar and Dynamic Input runtime state resets when Sketch edit ends; R10 does not create application-wide preference persistence for them.
43. Dynamic Input is a runtime UI adapter to the same semantic request, field-selection state and CAD input buffer; it owns no CAD meaning and never creates a second parser/buffer.
44. Dynamic Input is OFF by default at the start of a Sketch edit session in R10. DYN OFF hides the overlay/field-navigation UI only; keyboard-first precision input and Command Line syntax remain fully active.
45. Dynamic Input presents request values in three semantic states where applicable: Free (pointer-derived), Assisted (for example Polar-captured) and Locked (explicit numeric field value).
46. Typing while DYN is visible does not require clicking the overlay; printable input continues to the one workspace CAD buffer and is interpreted by the request's current Dynamic Input field.
47. Tab with a valid current token locks that field and advances; Tab with an empty buffer advances field focus without creating a lock; Shift+Tab moves backward. Enter accepts/executes the current semantic request from the available locks plus remaining Polar/pointer degrees of freedom.
48. Dynamic field locks constrain the existing resolver only and never create authored dimensions/constraints.
49. Esc order remains live token → request-local numeric locks → existing tool-stage cancellation.
50. Accepted point/value submission clears locks belonging to that request.
50. R10 introduces no OSNAP, Object Snap Tracking, geometric inference, Grid Snap or candidate cycling.
51. R10 introduces no authored dimension/constraint/solver state.
52. Construction dash-gap cadence becomes fixed in screen/presentation space and independent of entity length; committed and preview Construction geometry use the same visual cadence policy.
53. Construction dash styling remains Viewer presentation only and never becomes authored geometry or persistence state.

## 4. Scope IN

R10 includes:

- shared Core physical quantity/unit primitives needed by current precision clients;
- canonical millimetre conversion for Sketch/Part length precision semantics;
- persistent Part display/input length-unit preference;
- native Part persistence migration for that unit preference;
- unit-aware length/angle/scalar parsing;
- bounded arithmetic quantity expressions;
- absolute Cartesian point entry;
- relative Cartesian point entry;
- relative polar point entry;
- Direct Distance with explicit units/expressions;
- precision PointRequest integration;
- typed Circle Radius input;
- typed Rotate Angle input;
- typed Scale Factor input;
- precision Arc point stages;
- precision Rectangle opposite-corner input including `@Width;Height`;
- Polar directional magnet;
- primary polar increment;
- additional polar angles;
- Absolute/Relative Polar reference mode;
- Dynamic Input runtime overlay/fields;
- Tab field cycling and numeric-lock lifecycle;
- shared Command Line/Dynamic Input semantic submission;
- formatting of current precision values using the Part display unit;
- Measure linear/area display with physical unit labels;
- Construction fixed dash-gap presentation carry-over;
- automated and manual Windows verification;
- internal + PL/EN Product documentation and regenerated Product Browser.

## 5. Scope OUT

R10 does not authorize:

- OSNAP modes;
- Object Snap Tracking;
- endpoint/midpoint/center/intersection acquisition;
- geometric inference;
- Grid Snap;
- Grid preference architecture;
- authored dimensions;
- reference/driving dimension entities;
- constraints;
- solver;
- Auto-Constraint;
- Trim/Split/Join;
- persistent Dynamic Input field locks;
- persistence of Polar/Dynamic Input mode state;
- application-wide/user-profile CAD preferences;
- custom linetype editor or Construction linetype persistence;
- width/height authored Rectangle parameters;
- durable Rectangle identity/group/center;
- alternative Rectangle construction methods;
- new Circle construction methods;
- new Arc construction methods;
- Rotate/Scale/Mirror Copy modes;
- ordinary Select RMB context menu;
- command aliases not required by this contract;
- solid modeling / Extrude.

R11+ remain inactive.

## 6. Units architecture

Foundation 1.0 requires units to be shared platform semantics.

R10 introduces the minimum concrete reusable unit vocabulary in Core rather than in Qt, Part UI or Sketch.

Conceptually:

```text
LengthUnit = mm | cm | m | in | ft
LengthValue = canonical millimetres
AngleValue = canonical radians
ScalarValue = dimensionless finite value
```

Exact type names are D1.

Core unit code must not depend on Qt, Part, Sketch, Viewer or persistence.

### 6.1 Canonical model length

For R10 and future physical-value clients:

```text
1 Sketch coordinate length unit = 1 millimetre
```

Existing authored numeric geometry is not rescaled.

A legacy/current Line from `(0,0)` to `(25.4,0)` is therefore interpreted as 25.4 mm after R10.

This is a semantic interpretation/migration policy, not a geometry rewrite.

### 6.2 Part display/input unit property

Part authored state gains one display/input length unit.

Required values:

- mm;
- cm;
- m;
- in;
- ft.

Existing native Part schemas that do not store this field load it as mm.

A later ordinary Save writes the current schema with the explicit unit field.

Changing the Part unit:

- uses Command → Validation → Transaction → PartDocument;
- preserves all geometry and identities exactly;
- increments DocumentRevision when the value changes;
- participates in Undo/Redo;
- marks the Document dirty;
- creates a clean no-op when unchanged.

The unit property affects:

- every unitless Length value accepted by precision input, including Direct Distance, point-coordinate components, dU/dV locks, Circle Radius and other Length requests;
- Dynamic Input length formatting;
- Measure linear/area presentation;
- precision prompts/diagnostics where a unit label is shown.

An explicit length suffix always overrides the current Part display/input unit for that quantity. For example, in an mm document `2in` resolves to canonical 50.8 mm; in an inch document `25mm` resolves to canonical 25 mm.

After parsing/locking, normal presentation formats the canonical value in the current Part display/input unit. The live token may still show exactly what the user is typing, but accepted/locked presentation is normalized to the document unit.

It does not affect:

- canonical stored geometry numbers;
- angles;
- EntityIds;
- Profile references;
- region topology.

## 7. Quantity parser

R10 replaces the current bare-distance-only parser with one reusable provider-neutral quantity parser.

The parser must accept signed finite decimal/scientific numbers with either decimal separator:

```text
12
-12.5
12,5
1e-3
```

Grouping separators are rejected.

### 7.1 Length quantities

Accepted examples:

```text
25
25mm
2.5cm
0,25m
1in
2ft
```

A number without a suffix uses the current Part display/input length unit.

An explicit suffix overrides the document unit for that quantity.

Examples in an mm document:

```text
25      -> 25 mm
25mm    -> 25 mm
2cm     -> 20 mm
1in     -> 25.4 mm
0.1m    -> 100 mm
```

Examples in an inch document:

```text
2       -> 2 in
25mm    -> 25 mm
```

The accepted suffix vocabulary in R10 is exactly `mm`, `cm`, `m`, `in`, `ft`. Quote/apostrophe feet-inch notation such as `1'-2 1/2"` is not part of R10.

### 7.2 Angle quantities

Accepted examples:

```text
45
45deg
0.785398rad
```

A bare angle uses degrees, independent of the Part length unit.

This same rule applies wherever an Angle request/expression is accepted, including Rotate, grip Rotate, grip Mirror Axis Angle, Polar spacing and the Angle component of point-polar grammar.

### 7.3 Scalar quantities

Scale Factor and other dimensionless scalar requests accept no physical unit suffix.

Scale factor must be finite and strictly positive.

### 7.4 Bounded arithmetic expressions

R10 supports a deliberately small arithmetic grammar:

- parentheses;
- unary + / -;
- binary +;
- binary -;
- binary *;
- binary /.

Standard arithmetic precedence applies: parentheses, then unary sign, then multiplication/division, then addition/subtraction. Evaluation is deterministic and bounded.

Required examples include:

```text
25mm + 1in
2 * 12.5mm
1in / 2
(50 + 25)mm
90deg / 2
1 + 0.25
```

A unit suffix may bind to a numeric literal or to one parenthesized dimensionless subexpression, so `(50 + 25)mm` means 75 mm.

Supported dimensional combinations are deliberately small:

- Length ± Length -> Length;
- Angle ± Angle -> Angle;
- Scalar ± Scalar -> Scalar;
- Scalar * Length or Length * Scalar -> Length;
- Scalar * Angle or Angle * Scalar -> Angle;
- Scalar * Scalar -> Scalar;
- Length / Scalar -> Length;
- Angle / Scalar -> Angle;
- Scalar / Scalar -> Scalar.

Other combinations fail closed in R10. In particular, Length * Length, Angle * Angle, Length / Length, Angle / Angle, and mixed Length/Angle arithmetic are not used to infer new dimensions.

The active semantic request supplies the expected result dimension. A final expression whose dimension does not match that request is rejected without mutation.

No variables, functions, constants such as `pi`, trigonometry, square roots, document parameters, named dimensions or external expression engine are introduced.

## 8. Point grammar

A point request may accept one of four forms according to its active capabilities.

### 8.1 Absolute Cartesian

```text
U;V
```

Example:

```text
25;10
25mm;1in
```

The values are absolute Sketch-local coordinates.

Each unitless coordinate component uses the current Part display/input length unit independently. Explicit suffixes may override per component, for example `100mm;2in`.

### 8.2 Relative Cartesian

```text
@dU;dV
```

The active request must provide a semantic base point.

Unitless dU/dV components use the current Part display/input length unit; explicit suffixes override per component.

Example:

```text
@50;25
@2in;10mm
```

### 8.3 Relative polar

```text
@Distance<Angle
```

The active request must provide a semantic base point.

Examples:

```text
@50<45
@2in<30deg
@25mm<0.5rad
```

The Distance component uses the current Part length unit when unitless; an explicit length suffix overrides it. The Angle component is degrees when unitless; `deg` and `rad` override explicitly.

### 8.4 Direct Distance

A single length expression is Direct Distance only when the active PointRequest explicitly enables it.

Direction comes from the current highest-priority free direction after numeric locks and Polar attraction resolution.

PointRequest ownership remains context-first. While a PointRequest is active, a token that is not valid for that request does not silently activate a top-level command.

## 9. Numeric locks and point resolution

R10 extends the existing Input Resolution seam.

Priority during R10 is:

```text
1. complete explicit point coordinate / locked numeric fields
2. captured Polar track
3. raw pointer
```

R11 will later insert OSNAP/tracking/inference at its roadmap priority without replacing this resolver.

For a based point request, runtime locks may include:

- dU;
- dV;
- Distance;
- Angle.

Rules:

- two locked Cartesian components fully resolve the point;
- locked Distance + Angle fully resolve the point;
- locked Distance alone uses current resolved free direction;
- locked Angle alone uses current pointer-derived magnitude along that angle;
- a complete submitted absolute/relative/polar point outranks mode assistance;
- incompatible/conflicting locks fail closed rather than silently relaxing one;
- no numeric lock becomes authored CAD state.

Exact internal lock representation is D1.

## 10. Line

Line first point accepts absolute Cartesian input.

After the first point:

- absolute Cartesian is accepted;
- relative Cartesian is based at the current Line anchor;
- relative polar is based at the current Line anchor;
- Direct Distance remains available;
- Dynamic Input exposes the current point fields;
- Polar may attract pointer-derived direction to a configured track.

Continuous Line advances its base to the newly committed endpoint.

Stale direction/locks are not silently reused for the next segment.

## 11. Circle

Circle remains Center → Radius.

Center becomes a precision-capable point request.

Radius becomes a typed positive Length request.

Command Line examples:

```text
CIRCLE
25;25
10
```

or:

```text
CIRCLE
25mm;25mm
0.5in
```

Dynamic Input shows Center point fields during center placement and Radius during radius placement.

Zero/negative radius remains rejected with no authored mutation.

## 12. Arc

Arc remains the accepted 3-Point construction method.

Start, Through and End are precision-capable point requests.

Relative base semantics:

- Start has no relative base;
- Through uses Start as its relative base;
- End uses Through as its relative base.

Existing circumcircle/sweep validation remains authoritative.

R10 does not add Center/Start/End, Center/Radius/Angles or other Arc creation modes.

## 13. Rectangle

Rectangle remains First Corner → Opposite Corner and remains four ordinary perimeter Lines, plus optional two Construction diagonals.

First Corner accepts absolute Cartesian input.

Opposite Corner accepts:

- absolute Cartesian;
- relative Cartesian from First Corner;
- relative polar from First Corner.

The deliberate width/height workflow is:

```text
RECTANGLE
<First Corner>
@Width;Height
```

Example:

```text
RECTANGLE
0;0
@50;30
```

This is point-coordinate precision, not durable Rectangle parameters.

A single bare scalar at Opposite Corner remains rejected. R10 does not guess diagonal/side/square meaning.

Existing Draw Diagonals and Creation Role semantics are unchanged.

## 14. Move and Copy

Base Point is a precision-capable point request.

Destination/placement supports:

- absolute Cartesian;
- relative Cartesian from Base Point;
- relative polar from Base Point;
- Direct Distance;
- Dynamic Input locks;
- Polar pointer direction.

COPY repeated placement clears request-local locks/candidate direction after each accepted placement and keeps the existing frozen source snapshot.

Existing fresh-ID/high-water and one-placement-one-Undo semantics remain unchanged.

## 15. Grip Reshape / Grip Move / Grip Copy

Existing grip PointRequest remains the owner.

The interaction-start grip/pivot remains the semantic relative base.

R10 adds coordinate/polar/unit-aware input and Dynamic Input to that same request.

R10 also completes the intended CAD-style Space CycleEditMode grammar that earlier milestones deliberately left partial.

For non-center grips:

```text
Reshape → Move → Rotate → Scale → Mirror → Reshape
```

For center grips:

```text
Move → Rotate → Scale → Mirror → Move
```

Mode cycling is runtime-only. It must preserve the same active grip, interaction-start pivot and frozen semantic selection/source geometry. Pointer candidate may remain where meaningful, but every mode recomputes preview from the interaction-start frozen authored geometry; cycling must never compound the previous preview.

The existing Grip Copy `C` keyword remains authoritative. R10 does not silently invent Rotate+Copy, Scale+Copy or Mirror+Copy semantics.

Numeric precision input must not change the affected-set rules or duplication identity semantics.

## 16. Rotate

Existing Rotate selection/base/reference workflow remains.

The final Rotate stage gains an explicit Angle input path.

Angle semantics:

- bare positive value, for example `30`, means +30° counter-clockwise in Sketch U/V;
- explicit `+30` has the same meaning;
- negative value, for example `-30`, means 30° clockwise;
- pointer side never changes the sign of an explicit typed angle;
- zero follows existing no-op behavior;
- bare value = degrees;
- `deg` and `rad` accepted.

Before an explicit Angle value is entered, pointer destination remains a valid adapter for live Rotate preview.

For grip Rotate entered through Space CycleEditMode:

- the active grip is the pivot;
- on entry to Rotate, the current finite pointer direction from pivot is captured as the zero/reference direction;
- later pointer motion previews the signed angle from that frozen reference direction to the current pointer direction;
- counter-clockwise preview is positive and clockwise preview is negative in Sketch U/V;
- changing pointer radius does not change the preview angle;
- if no finite non-zero pointer direction exists at mode entry, Rotate waits for the first valid pointer direction and captures that as the zero/reference direction;
- cycling away from Rotate discards this runtime reference;
- cycling back to Rotate captures a new zero/reference direction from the current pointer candidate rather than reusing the previous Rotate session reference.

Once an explicit Angle owns the request, preview and final commit use that exact signed angle and subsequent pointer motion does not alter its sign or magnitude. The pointer reference remains runtime-only and creates no authored state/history.

Dynamic Input shows Angle at the final stage and uses the same sign rule.

R10 does not add Rotate+Copy.

## 17. Scale

Existing positive uniform Scale workflow remains.

The final Scale stage gains a dimensionless Factor request.

Examples:

```text
2
0.5
1 + 0.25
```

Factor must be finite and strictly positive.

Factor 1 follows existing no-op behavior.

For grip Scale entered through Space CycleEditMode:

- the active grip remains the scale pivot;
- on entry to Scale, the current finite non-zero pointer radius from pivot is captured as the reference radius representing factor 1.0;
- later pointer motion previews `current_radius / reference_radius`;
- pointer direction does not affect the scale factor;
- if no finite non-zero pointer radius exists on entry, Scale waits for the first valid non-zero radius and captures that as factor 1.0;
- cycling away from Scale discards that runtime reference radius;
- cycling back to Scale captures a fresh reference radius from the current pointer candidate;
- every preview is recomputed from the interaction-start frozen authored geometry rather than compounded from a previous mode/preview.

Once an explicit Factor owns the request, preview and final commit use that exact factor and subsequent pointer motion no longer changes it.

Dynamic Input shows Factor at the final stage.

R10 does not add negative Scale as hidden Mirror, non-uniform scale or Scale+Copy.

## 18. Mirror

Ordinary Mirror remains a two-point axis workflow.

Axis Start and Axis End are precision-capable point requests.

Axis End may use relative Cartesian, relative polar and Polar.

Grip Mirror entered through Space CycleEditMode has a bounded shorthand:

- the active grip/pivot is the Mirror Axis Start;
- pointer position supplies the live Axis End and therefore immediate mirror preview;
- the user may instead submit a typed Axis Angle;
- typed Axis Angle is measured from Sketch +U through the grip/pivot;
- bare/positive `45` means an axis at +45°;
- `-30` means an axis at -30°;
- pointer side/radius never changes an explicit typed Axis Angle;
- once an explicit Axis Angle owns the request, any finite non-zero axis length is presentation/runtime-only; the semantic mirror axis is the infinite line through the pivot at that exact angle;
- a zero-length pointer vector cannot define pointer preview but does not invalidate an already explicit Axis Angle;
- cycling away from Mirror clears the runtime mirror-axis angle/reference state;
- cycling back to Mirror starts from the current pointer candidate, without reusing a previous Mirror mode reference.

The typed angle shortcut is specific to grip Mirror because the first axis point is already semantically fixed by the active grip. Ordinary toolbar/Command-Line Mirror keeps its explicit two-point axis grammar.

No Mirror+Copy is introduced.

## 19. Polar directional magnet

Polar is the only runtime directional-assistance mode in R10.

There is no separate Ortho toggle or resolver path. A classic orthogonal-only setup is simply:

```text
Polar spacing = 360/4 = 90°
```

Default per Sketch edit: OFF.

When OFF, pointer direction remains raw unless explicit numeric locks fully determine it.

When ON, Polar behaves as an angular **magnet**, not a global quantizer.

The accepted R10 capture model is screen/presentation-space based:

- configured semantic tracks are derived from the current Polar reference and primary spacing;
- the pointer adapter measures the screen-space distance from the cursor to nearby projected tracks;
- Polar may capture only after the pointer is outside a small dead-zone around the semantic base point;
- initial accepted tuning target: capture at approximately 9 logical/device-independent pixels;
- initial accepted tuning target: release at approximately 15 logical/device-independent pixels;
- release tolerance is deliberately larger than capture tolerance to provide hysteresis and prevent flicker;
- initial accepted base-point dead-zone target is approximately 12 logical/device-independent pixels;
- an additional bounded angular sanity guard may reject obviously unrelated tracks near the base point; its exact threshold is D1 tuning;
- if the pointer enters the capture neighborhood of one track, that track becomes captured;
- while captured, semantic direction is exactly that track angle;
- pointer radius remains free while captured and therefore continues to control magnitude until an explicit Distance/value lock owns it;
- leaving the release neighborhood releases capture and raw pointer direction becomes free again;
- while one track is captured, the resolver does not jump directly to a neighboring track; it releases first, then may capture another track;
- free-direction regions must remain between neighboring tracks;
- Polar must never become unconditional nearest-angle rounding.

The screen-space tolerance belongs only to runtime pointer attraction. It must not enter Document state, Commands, persistence, authored geometry, EntityId/reference semantics or provider identity.

Once capture occurs, the semantic resolver receives only the exact captured direction plus ordinary request state. Pixel distances are not CAD semantics.

A captured track is indicated by one transient provider-neutral guide/cue. R10 does not display every configured track simultaneously. The guide begins at the active semantic base/pivot and extends along the captured direction. A compact label may show the captured value, for example `Polar: 45°` or `Polar: +30° rel`.

The guide/cue is runtime presentation only and has no CAD identity.

### 19.1 Primary spacing expression

The primary spacing is an Angle expression parsed by the same provider-neutral arithmetic/quantity parser used by R10.

Examples:

```text
45
360/8
360/12
90/2
360/7
```

A bare expression in this field is interpreted in degrees.

Required validation:

- finite;
- strictly positive;
- no greater than 180°.

Track generation is bounded to one full turn around the selected reference. For a spacing that does not divide 360° exactly, generate integer multiples within that one-turn bound only; do not continue modulo 360 into an unbounded/dense sequence.

The visible UI should show both the entered expression/result where practical, for example:

```text
POLAR ON   360/8 = 45°   ABS
```

or:

```text
POLAR ON   360/12 = 30°   REL
```

The default primary spacing is `360/8 = 45°`.

R10 does not expose user-adjustable magnet-strength/capture-tolerance controls. Capture/release tuning is an implementation/manual-verification concern for this milestone, not a new preference surface.

### 19.2 Polar reference mode

Polar has one runtime reference-mode selector:

```text
Reference: Absolute | Relative
```

Default per Sketch edit: **Absolute**.

Changing the reference mode is runtime-only and creates no DocumentRevision, dirty state or Undo entry.

### 19.3 Absolute

Absolute is the default and always has a valid reference.

Tracks are measured from Sketch +U.

For example, with spacing `30°`, tracks are `0°`, `30°`, `60°`, `90°`, and so on around one bounded full turn, independent of previously authored geometry.

### 19.4 Relative

Relative rotates the same Polar track family by one semantic reference direction supplied explicitly by the active interaction context.

Examples:

- continuous Line after at least one committed segment: previous committed segment direction;
- grip/transform/reference stages: the operation's accepted semantic reference direction when that stage defines one;
- other requests may supply a reference only when their contract defines it explicitly.

Relative must never search geometry, infer a nearby edge, reuse stale hover state or guess a reference from presentation.

If the active request does not provide a valid finite reference direction:

- Polar does not capture any Relative track;
- raw pointer direction remains free;
- UI may show `Relative — no reference`;
- no fallback to Absolute occurs silently.

Switching back to Absolute immediately restores Sketch +U as the reference without changing authored state.

### 19.5 Additional angles

Additional angles are runtime-only offsets in R10.

They augment the primary track family without creating authored constraints.

Duplicate/equivalent normalized angles are collapsed deterministically.

Invalid/non-finite configuration is rejected without changing the previous valid configuration.

### 19.6 Priority with numeric input

Polar assists only degrees of freedom that remain unspecified.

Examples:

- complete `@100<30` supplies both magnitude and direction, so Polar is ignored;
- locked Distance = 100 leaves direction free, so captured Polar direction may supply it;
- locked Angle = 30 supplies direction exactly, so Polar is ignored;
- with no locks, Polar may capture pointer direction while pointer magnitude remains pointer-derived.

## 21. Dynamic Input

Dynamic Input is a runtime presentation/input adapter to the same active semantic request, the same request-local numeric locks and the same workspace-global CAD input session.

Default per Sketch edit: OFF.

**DYN OFF does not disable precision input.** With the overlay hidden, keyboard-first Command Line input, explicit coordinate/polar syntax, Direct Distance and typed Length/Angle/Factor requests continue to work exactly through the same semantic endpoint. DYN controls only the near-cursor field presentation and field-navigation workflow.

Dynamic Input must never own:

- a second text buffer;
- a second parser;
- separate tool state;
- authored dimensions/constraints;
- Viewer/provider identity.

### 21.1 Visibility and value states

When DYN is ON and the current semantic request exposes precision fields, show a compact overlay near the cursor/current semantic point.

When no precision-capable request is active, show no floating Dynamic Input overlay.

Each displayed value is conceptually one of:

- **Free** — currently derived from pointer geometry;
- **Assisted** — currently supplied by runtime assistance such as a captured Polar direction;
- **Locked** — explicitly supplied/locked by numeric field input.

These states must be visually distinguishable. Exact styling is D1, but an Assisted Polar angle must not look identical to a keyboard Locked angle.

The overlay is positioned with a small logical-pixel offset from the cursor so it does not cover the picked point/grip. Initial presentation target is approximately 16 logical pixels diagonally from the cursor, with automatic edge-aware flipping/clamping near viewport boundaries.

### 21.2 One buffer, keyboard-first

The user never has to click the floating field before typing.

Printable keyboard input continues into the one ADR-0011 workspace CAD buffer. When DYN is ON, the active request's current Dynamic Input field provides the explicit semantic field context for that token.

The Command Line may mirror the same live token. There is no duplicate hidden text state.

This permits field-context workflows that would otherwise be ambiguous in plain Command Line grammar. Example: Rectangle Opposite Corner may accept `50` as the currently focused dU/Width field only because Dynamic Input has explicitly selected that field; with DYN OFF, the existing rule remains that a bare Rectangle scalar is rejected rather than guessed.

### 21.3 Field order by request

Field order is part of the interaction contract.

For an unbased point request:

```text
U → V
```

For a normal based point request such as Line next point, Move/Copy destination, grip Reshape/Move and ordinary Mirror Axis End:

```text
Distance → Angle → dU → dV
```

Distance is the default field because it preserves the fast CAD workflow: point/base → indicate direction with pointer/Polar → type length → Enter.

Rectangle First Corner:

```text
U → V
```

Rectangle Opposite Corner:

```text
dU → dV
```

The overlay may present these as `W (dU)` and `H (dV)` for usability, but they remain transient Cartesian deltas and do not create durable Rectangle Width/Height parameters.

Circle:

- Center uses the applicable point-field order;
- Radius uses one `Radius` field.

Arc Start/Through/End each use their applicable point-field order from the semantic bases defined in section 12.

Rotate and grip Rotate:

```text
Angle
```

Scale and grip Scale:

```text
Factor
```

Grip Mirror:

```text
Axis Angle
```

Ordinary Mirror remains point-based and uses the ordinary point fields.

### 21.4 Tab and Shift+Tab

Tab means **lock current field if a valid token exists, then advance**.

Rules:

- valid non-empty token + Tab parses against the current field type, creates/replaces that request-local lock, clears the live token and advances;
- empty buffer + Tab advances field focus without creating/changing a lock;
- invalid token + Tab leaves field/request unchanged and publishes the ordinary bounded diagnostic;
- Shift+Tab moves to the previous available field;
- field traversal wraps deterministically within the current request;
- changing field focus alone creates no authored state and no Undo entry.

Examples:

```text
Line:
100  Tab  30  Enter
=> Distance = 100 [Locked]
   Angle    = 30° [Locked]
```

```text
Rectangle Opposite Corner:
50  Tab  30  Enter
=> dU/W = 50 [Locked]
   dV/H = 30 [Locked]
```

### 21.5 Enter

Enter means **accept/execute the current semantic request**, not merely "move to the next field".

The request may be completed from a mixture of sources:

- explicit Locked fields;
- Polar Assisted direction;
- remaining Free pointer-derived values.

Examples:

- Line with Distance `100` Locked and Angle supplied by captured Polar commits exact length 100 along that Polar track;
- Line with Distance and Angle both Locked commits independently of pointer position;
- Rotate with Angle Locked commits that exact signed angle;
- Scale with Factor Locked commits that exact factor.

If the request is still semantically incomplete or invalid, Enter fails closed with no authored mutation.

### 21.6 Esc and lifecycle

Esc ordering:

1. non-empty live CAD token clears first;
2. otherwise current request-local numeric locks clear;
3. otherwise existing tool-stage Esc semantics apply.

Request-local Dynamic Input field focus/locks clear on:

- request acceptance;
- tool/stage completion;
- Esc cancellation;
- tool change;
- history boundary;
- Sketch/Document/runtime teardown.

### 21.7 Overlay examples

A based point while Polar supplies the angle may be presented conceptually as:

```text
D  83.42 mm        Free
A  45°             POLAR
```

After typing and locking Distance:

```text
D  100 mm          LOCK
A  45°             POLAR
```

After also locking Angle:

```text
D  100 mm          LOCK
A  30°             LOCK
```

Exact typography/icons are D1. The semantic distinction Free / Assisted / Locked is not optional.

During live editing, the active field may display the literal token exactly as typed, for example `2in`. After a valid value is locked/accepted, Length presentation is normalized to the current Part display/input unit. Example in an mm document:

```text
typing:  [2in]
locked:  D 50.8 mm   LOCK
```

The preserved semantic value is canonical; the original spelling is not durable CAD state.

## 22. Precision mode UI

R10 does not add more tool buttons to the top Create/Modify strip.

Precision modes/options belong to a contextual right-panel surface.

Expected bounded UI:

- current Part display/input unit selector, clearly separated conceptually from runtime Polar/DYN controls because Units is durable document state;
- clearly visible Polar ON/OFF state while Sketch edit is active;
- clearly visible current Polar spacing/result, for example `360/8 = 45°`;
- editable Polar spacing expression;
- Polar Reference selector with `Absolute` default and optional `Relative`;
- additional Polar angles entry/configuration;
- Dynamic Input toggle with visibly discoverable `DYN ON/OFF` state while Sketch edit is active.

A compact persistent status affordance may therefore read conceptually:

```text
POLAR ON   360/8 = 45°   ABS    DYN ON
```

Polar and DYN are Sketch-session interaction modes, not geometry tools. It must not masquerade as another Create/Modify tool button. Exact placement remains D1 for UI review. A compact persistent affordance near the Command Line/status area, with fuller configuration in a contextual panel or popover, is explicitly allowed.

Changing runtime Polar/Dynamic Input configuration creates no CAD history.

Changing the durable Part display/input unit follows the document-property mutation semantics defined in section 6.

## 23. Measure presentation

R10 does not change Measure geometric meaning.

It changes physical presentation:

- Line lengths / deltas show current document length unit;
- Circle radius/diameter/circumference show current length unit;
- Circle area shows squared current length unit;
- Arc radius/arc length show current length unit;
- point↔point and point↔Line distances show current length unit;
- angles continue to show degrees.

Changing display unit updates presentation without changing measured geometry.

## 24. Construction dash-gap carry-over

R9 Owner manual review accepted one visual follow-up.

R10 includes that bounded presentation polish.

Required invariant:

- dash and gap cadence is independent of the geometric length of the Construction entity;
- extending a Construction Line does not stretch the existing dash pattern to keep a fixed dash count;
- short and long Construction Lines use the same visual cadence;
- preview and committed Construction use the same cadence policy;
- cadence is defined in screen/presentation space so zoom and model-unit changes do not mutate authored geometry or require persistence;
- Regular geometry presentation is unchanged.

This is presentation only.

No linetype scale property, authored style entity, persistence field or semantic Construction subtype is introduced.

## 25. Persistence

R10 is expected to advance the Part domain schema because the Part display/input length unit is durable.

Migration rules:

- schemas v1–v6 remain readable;
- missing display/input length unit defaults to mm;
- current writer emits the new unit property;
- geometry values are not rescaled during load/save migration;
- unknown/malformed unit values fail closed;
- runtime Polar/Dynamic Input modes and numeric locks are never serialized.

No other R10 state requires persistence.

## 26. Failure behavior

All precision input remains fail closed.

Invalid syntax, incompatible units, non-finite results, division by zero, missing relative base, invalid Polar reference, conflicting locks, stale semantic request or stale Document context must create:

- no geometry mutation;
- no EntityId allocation;
- no DocumentRevision;
- no dirty-state change;
- no Undo entry.

Existing active tool/request remains authoritative unless existing stale-state policy requires cancellation.

No parser may guess a different request kind or top-level command when a higher-priority semantic request owns the token.

## 27. Selection/history/lifecycle invariants

R10 must preserve:

- one active semantic interaction state;
- current selection grammar;
- current Creation Role;
- Draw Diagonals;
- Grip Copy semantics;
- Repeat Last Command;
- Space CycleEditMode;
- existing command-first/selection-first transform grammar;
- transaction/identity semantics;
- stale-revision fail-closed behavior.

Numeric locks and modes are runtime assistance only and do not affect EntityId.

Undo/Redo cancels transient precision input first according to existing interaction/history boundaries, then performs ordinary history.

## 28. Expected implementation surface

Expected bounded production changes after Owner acceptance may include:

- `src/core/**` for shared physical unit/value primitives;
- `src/application/**` for reusable quantity/coordinate grammar and typed semantic input transport;
- `src/sketch/**` for expanded PointRequest resolution / numeric locks / Polar direction attraction;
- `src/part/**` for durable display/input unit property and validated persistence state;
- `src/persistence/**` for Part schema migration;
- `src/ui/**` for Operations precision controls and Dynamic Input adapter;
- `src/viewer**` / Qt-OCCT provider for Dynamic Input presentation bridge if needed and fixed Construction dash cadence;
- tests/CMake;
- internal + PL/EN Product docs and Product Browser;
- `work/**`.

Exact class/file split is D1.

## 29. Automated acceptance coverage

At minimum verify:

1. no tool-specific numeric parser is introduced;
2. unit/quantity parser is provider-neutral and Qt-free;
3. canonical length conversion uses millimetres;
4. existing geometry values are not rescaled by R10 migration;
5. old Part schema loads with mm display/input unit;
6. current schema save/reopen preserves selected display/input unit;
7. changing display unit preserves geometry and EntityIds;
8. changing display unit has correct revision/dirty/Undo behavior;
9. both decimal dot and decimal comma parse;
10. grouping separators fail closed;
11. mm/cm/m/in/ft conversions are exact within accepted numeric precision;
12. explicit length suffix overrides the current Part display/input unit;
13. unitless coordinate components and Length fields use the current Part display/input unit;
14. locked/accepted Dynamic Input Length values normalize presentation to the current Part unit while live token may retain typed spelling;
15. feet/inches quote/apostrophe notation is rejected in R10;
16. deg/rad conversion is correct and bare Angle remains degrees regardless of Part length unit;
17. bounded arithmetic obeys standard precedence and respects the accepted dimensional operation table;
18. postfix unit on a parenthesized dimensionless expression, e.g. `(50+25)mm`, resolves correctly;
19. unsupported dimensional arithmetic fails closed;
20. variables/functions/constants/named parameters are rejected;
21. absolute Cartesian uses semicolon component separator;
22. relative Cartesian requires a valid semantic base;
23. relative polar requires a valid semantic base;
24. Direct Distance remains request-gated;
25. PointRequest still outranks top-level command activation;
26. Line first/next points accept the specified coordinate grammar;
27. continuous Line advances the relative base and clears stale locks;
28. Circle center accepts point precision;
29. Circle radius accepts typed Length;
30. invalid/zero Circle radius fails closed;
31. Arc Start/Through/End accept point precision without changing Arc construction method;
32. Rectangle First Corner accepts absolute Cartesian;
33. Rectangle Opposite Corner accepts `@Width;Height`;
34. Rectangle bare scalar remains rejected;
35. Rectangle atomic 4/6-Line R9 semantics do not regress;
36. Move Base/Destination accept precision input;
37. Copy repeated placements accept precision input and retain fresh-ID/high-water semantics;
38. grip Reshape/Move/Copy accept precision point input without changing affected-set rules;
39. non-center grip Space cycle is `Reshape → Move → Rotate → Scale → Mirror → Reshape`;
40. center grip Space cycle is `Move → Rotate → Scale → Mirror → Move`;
41. grip mode cycling preserves pivot/selection/source state and never compounds preview;
42. grip Rotate captures the current pivot→pointer direction as its zero/reference direction on entry, or the first later valid direction if none exists;
43. grip Rotate pointer preview is the signed angle from the frozen zero/reference direction and is independent of pointer radius;
44. re-entering grip Rotate captures a fresh zero/reference direction rather than reusing a prior one;
45. Rotate accepts explicit signed Angle;
46. Rotate positive/negative direction is deterministic and pointer-independent after explicit input;
47. Scale accepts explicit positive Factor;
48. grip Scale captures/re-captures factor-1 reference radius deterministically and explicit Factor overrides pointer;
49. ordinary Mirror axis accepts precision point input;
50. grip Mirror accepts exact signed Axis Angle through the pivot, measured from Sketch +U and independent of pointer after explicit input;
51. no separate Ortho runtime mode/control exists;
52. Polar default is OFF;
53. Polar default spacing is `360/8 = 45°`;
54. `360/4 = 90°` reproduces orthogonal-only directional attraction without a separate resolver;
55. Polar spacing expressions such as `360/8`, `360/12` and `360/7` resolve deterministically;
56. Polar capture is based on logical screen-space distance to the projected track, not on entity length or a global nearest-angle quantizer;
57. initial capture/release tuning targets are approximately 9/15 logical pixels with a small base-point dead-zone around 12 logical pixels;
58. Polar releases outside the release neighborhood and free pointer direction returns;
59. Polar capture/release hysteresis is stable and does not flicker at the threshold;
60. Polar does not jump directly between neighboring captured tracks without first releasing;
61. Polar does not hard-quantize every pointer direction while enabled;
62. captured Polar constrains direction only; pointer radius/magnitude remains free unless an explicit numeric lock owns it;
63. pixel tolerances/guide state never enter authored CAD state, Commands or persistence;
64. additional Polar angles affect pointer resolution deterministically;
65. Polar Reference defaults to Absolute for each new Sketch edit;
66. Absolute uses Sketch +U and is independent of previous geometry;
67. Relative uses only a valid semantic reference direction explicitly supplied by the active interaction context;
68. Relative with no valid reference performs no Polar capture and does not silently fall back to Absolute;
69. switching Absolute/Relative is runtime-only and has no revision/dirty/Undo effect;
70. explicit complete point input outranks Polar;
71. locked Distance may combine with captured Polar direction;
72. locked Angle outranks Polar direction;
73. conflicting locks fail closed;
74. Dynamic Input default is OFF per Sketch edit;
75. DYN OFF hides only overlay/field navigation and does not disable keyboard-first precision input;
76. Dynamic Input has no authored state/history impact;
77. Dynamic Input and Command Line feed the same request and same live CAD token;
78. Dynamic Input uses the shared CAD input buffer rather than a second text buffer;
79. Free / Assisted / Locked value states are semantically distinguishable in presentation;
80. unbased point field order is U → V;
81. normal based-point field order is Distance → Angle → dU → dV;
82. Rectangle Opposite Corner field order is dU → dV and does not create durable Width/Height parameters;
83. Rotate/grip Rotate expose Angle; Scale/grip Scale expose Factor; grip Mirror exposes Axis Angle;
84. valid token + Tab locks current field and advances;
85. empty-buffer Tab advances without creating a lock;
86. Shift+Tab moves backward deterministically;
87. Enter accepts the current request from locks plus remaining Polar/pointer values rather than merely advancing field focus;
88. request acceptance clears request-local locks;
89. Esc follows buffer → locks → tool hierarchy;
90. Sketch/tool/Document teardown clears precision runtime state;
91. Measure displays correct current units without changing measurement semantics;
92. changing units updates Measure presentation without geometry mutation;
93. Construction dash-gap cadence is independent of entity length;
94. Construction preview and committed geometry use the same cadence policy;
95. Construction dash polish introduces no authored/persistent style state;
96. R8 Measure/Between regressions remain green;
97. R9 Rectangle/Construction/Profile regressions remain green;
98. selection/grips/transforms/COPY/Grip Copy regressions remain green;
99. global keyboard-first CAD input/focus arbitration regressions remain green;
100. persistence backward-read coverage remains green;
101. exact-head Windows FULL passes;
102. required internal + PL/EN docs and Product Browser freshness pass.

## 30. Manual Windows verification

Final implementation candidate requires Owner manual verification on the exact FULL-tested runtime head.

Minimum checklist:

- verify an existing pre-R10 Part opens as mm without geometry rescale;
- switch document display/input unit and verify geometry does not move;
- Save/Close/Reopen and verify selected document unit persists;
- in mm mode verify `25`, `2cm`, `1in` resolve to the expected physical lengths;
- switch to inch mode and verify unitless `2` means 2 in while explicit `25mm` remains 25 mm;
- verify mixed expression `25mm + 1in`, scalar multiplication/division and `(50 + 25)mm`;
- verify invalid dimensional expressions fail closed;
- verify bare Angle remains degrees after changing document length unit;
- with DYN in mm mode type `2in` and verify the live token can show `2in` while the locked presentation normalizes to 50.8 mm;
- verify feet/inch quote notation is rejected rather than guessed;
- LINE first point by absolute Cartesian;
- LINE next point by `@dU;dV`;
- LINE next point by `@Distance<Angle`;
- LINE Direct Distance with explicit mm/in unit suffix;
- verify decimal comma and dot;
- CIRCLE center by coordinates and radius by typed Length;
- ARC Start/Through/End by coordinate/polar precision;
- RECTANGLE First Corner + `@Width;Height`;
- verify Rectangle single bare scalar still rejects;
- MOVE/COPY by relative Cartesian and polar input;
- verify repeated COPY retains correct fresh-ID behavior;
- grip Reshape and grip Move precision input;
- non-center grip: cycle through Reshape → Move → Rotate → Scale → Mirror and back to Reshape;
- center grip: cycle Move → Rotate → Scale → Mirror and back to Move;
- verify grip cycling preserves the same pivot/selection/source geometry and does not compound preview;
- enter grip Rotate with the pointer at a known direction, confirm that direction becomes 0°, then move around the pivot and verify signed preview from that frozen reference;
- vary pointer radius without changing direction and verify the preview angle is unchanged;
- cycle away from Rotate and back, then verify a fresh current pointer direction becomes the new 0° reference;
- Rotate typed `30` and verify exact +30° CCW regardless of which side the pointer currently occupies;
- Rotate typed `-30` and verify exact 30° CW regardless of pointer side;
- enter grip Scale at a known pointer radius, confirm that radius is factor 1.0, then verify doubled/halved radius previews factor 2/0.5;
- type Scale factor `2` and `0.5` and verify pointer motion no longer changes the exact factor;
- cycle away from grip Scale and back and verify a fresh current radius becomes factor 1.0;
- ordinary Mirror precise axis points;
- grip Mirror pointer preview with pivot as Axis Start;
- grip Mirror typed `45` and verify exact +45° axis through the pivot regardless of pointer position;
- grip Mirror typed `-30` and verify exact -30° axis through the pivot regardless of pointer position;
- enable Polar and verify it behaves as an attraction magnet rather than unconditional angular quantization;
- approach one track slowly and verify capture near the accepted screen-space threshold;
- move away slowly and verify release occurs farther out than capture, without flicker;
- while captured, move farther/closer along the same guide and verify length remains free;
- cross toward a neighboring track and verify the current track releases before another track can capture;
- verify only the currently captured guide is shown rather than all configured tracks;
- set Polar spacing to `360/4` and verify orthogonal-only attraction at 0/90/180/270 without any separate Ortho mode;
- set Polar spacing to `360/8` and verify 45-degree family;
- set at least one nontrivial expression such as `360/7` and verify deterministic bounded tracks;
- move just outside the attraction neighborhood and verify direction becomes free again;
- test at least one additional Polar angle;
- verify Reference defaults to Absolute on a fresh Sketch edit;
- verify Absolute tracks stay anchored to Sketch +U regardless of previous segment direction;
- switch to Relative on a continuous Line and verify the previous committed segment becomes the 0° reference;
- use Relative in a context with no valid semantic reference and verify Polar does not capture and does not silently fall back to Absolute;
- switch back to Absolute and verify Sketch +U reference is restored without authored/history change;
- with DYN OFF, type precision input from the viewport and verify keyboard-first Command Line behavior remains fully functional;
- enable DYN and verify overlay appears only for an active precision-capable semantic request;
- verify Free, Polar-Assisted and keyboard-Locked values are visually distinguishable;
- LINE: indicate a Polar direction, type `100`, Enter, and verify exact Distance 100 with direction supplied by Polar;
- LINE: type `100`, Tab, `30`, Enter and verify exact Distance=100 / Angle=30° independent of pointer;
- with an empty buffer, Tab through based-point fields and verify no numeric lock is created;
- use Shift+Tab and verify deterministic reverse traversal;
- RECTANGLE Opposite Corner: type `50`, Tab, `30`, Enter and verify exact dU/W=50 and dV/H=30 without durable rectangle parameters;
- Rotate/grip Rotate: verify single Angle field; Scale/grip Scale: single Factor field; grip Mirror: single Axis Angle field;
- verify Command Line and Dynamic Input resolve through the same request/live token and produce identical geometry for equivalent values;
- verify Esc clears live text, then numeric locks, then normal tool stage;
- move the cursor near viewport edges and verify overlay stays visible without covering the semantic point/grip unnecessarily;
- Measure values show current physical unit labels;
- inspect short and long Construction Lines and confirm equal visual dash-gap cadence;
- zoom in/out and verify Construction pattern remains a presentation effect rather than geometry;
- regression smoke Rectangle/Draw Diagonals/Construction, Measure/Between, selection/grips, Move/Copy/Rotate/Scale/Mirror, Grip Copy, Profile, Undo/Redo and Save/Reopen.

## 31. Stop conditions

Stop for Owner review if implementation requires or attempts:

- a second CAD input buffer;
- Dynamic-Input-owned semantic state;
- per-tool parsers;
- Qt/Viewer types in shared quantity/unit parser contracts;
- re-scaling existing geometry when document display unit changes;
- changing EntityId/reference semantics;
- persisting Polar/Dynamic Input runtime state;
- OSNAP/tracking/inference;
- Grid Snap;
- authored dimensions/constraints/solver;
- durable Rectangle parameters;
- new Circle/Arc construction methods;
- generic parameter/expression variables;
- named dimensions/parameters;
- a custom persistent linetype/style system;
- topology-changing structural editing;
- Part feature tree or solid modeling.

These are separate scope/architecture decisions.

## 32. R11 continuation boundary

R10 does not activate R11.

After R10 completes, Roadmap v1.6 places R11 Object Snap / Tracking / Inference next.

R11 must consume the accepted R10 resolver priority and numeric locks. It must not create a second point-resolution system.

## 33. Activation and completion boundary

R10 may become ACTIVE only after explicit Owner acceptance of this Work Contract.

Completion requires:

- accepted canonical unit/conversion semantics;
- accepted quantity and coordinate grammar;
- one shared semantic precision-input path;
- accepted Polar mode semantics;
- accepted Dynamic Input adapter semantics;
- Construction dash-gap carry-over completed without authored-style creep;
- no R11 OSNAP/inference or R12 structural-edit creep;
- affected/FOCUSED evidence;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- required internal + PL/EN Product documentation/current Browser;
- work-only CLOSURE closeout.

R11+ remain inactive after R10 completion unless separately accepted.
