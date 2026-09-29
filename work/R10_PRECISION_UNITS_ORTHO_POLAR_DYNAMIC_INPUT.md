# R10 — Precision Input / Units / Ortho / Polar / Dynamic Input

**Status:** PROPOSED — INACTIVE  
**Proposed:** 2026-09-29  
**Owner acceptance:** pending  
**Decision class:** D2 shared semantic precision-input / units / runtime mode grammar + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009, ADR-0010, ADR-0011  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.6  
**Predecessor:** R9 Rectangle + Construction Authoring Surface — completed  
**Milestone:** R10 — Precision Input / Units / Ortho / Polar / Dynamic Input

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

The current precision capability is intentionally narrow. A submitted scalar can mean Direct Distance only where the active PointRequest explicitly allows it. Circle radius, Rotate angle, Scale factor, absolute/relative coordinates, polar coordinates, unit suffixes, Ortho/Polar and Dynamic Input are not yet implemented.

R10 must extend that one existing input architecture. It must not create per-tool parsers, a second command buffer, a second point resolver or a Dynamic-Input-only state machine.

## 2. Goal

R10 makes existing Sketch creation/editing deliberately placeable by engineering values.

The target user model is:

```text
active semantic request
        ↓
pointer + explicit numeric locks / typed quantity
        ↓
Ortho / Polar direction assistance where applicable
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
24. Scale final placement accepts an explicit positive dimensionless Scalar factor while pointer placement remains valid.
25. Move/Copy/grip point requests accept coordinate/polar/direct-distance forms through the same resolver.
26. Mirror remains two point requests; its axis can be placed precisely through coordinate/polar input.
27. Explicit numeric locks outrank Ortho/Polar and raw pointer.
28. Ortho and Polar are mutually exclusive runtime Sketch modes.
29. Ortho constrains a based point direction to Sketch-local ±U or ±V.
30. Polar constrains a based point direction to configured angular tracks.
31. Polar default primary increment is 45 degrees; additional angles are initially empty.
32. Polar supports Absolute reference from Sketch +U and Relative reference from the current operation reference direction.
33. Ortho/Polar/Dynamic Input mode state is runtime-only in R10 and retained across tool changes within one active Sketch edit session.
34. These runtime modes reset when Sketch edit ends; R10 does not yet create application-wide preference persistence for them.
35. Dynamic Input is a runtime UI adapter to the same semantic request and CAD input buffer; it owns no CAD meaning.
36. Dynamic Input is OFF by default at the start of a Sketch edit session in R10.
37. Dynamic Input may show lockable point fields such as dU, dV, Distance and Angle, and typed Radius/Angle/Factor fields for concrete non-point requests.
38. Tab cycles the currently available Dynamic Input fields.
39. Locking a field constrains the existing semantic resolver; it does not create authored dimensions or constraints.
40. Esc first clears the current non-empty CAD token per ADR-0011; with an empty token it may clear active numeric locks before existing tool-stage Esc behavior.
41. Accepted point/value submission clears locks belonging to that request.
42. R10 introduces no OSNAP, Object Snap Tracking, geometric inference, Grid Snap or candidate cycling.
43. R10 introduces no authored dimension/constraint/solver state.
44. Construction dash-gap cadence becomes fixed in screen/presentation space and independent of entity length; committed and preview Construction geometry must use the same visual cadence policy.
45. Construction dash styling remains Viewer presentation only and never becomes authored geometry or persistence state.

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
- Ortho;
- Polar;
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
- persistence of Ortho/Polar/Dynamic Input mode state;
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

- unitless length input;
- Dynamic Input length formatting;
- Measure linear/area presentation;
- precision prompts/diagnostics where a unit label is shown.

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

### 7.2 Angle quantities

Accepted examples:

```text
45
45deg
0.785398rad
```

A bare angle uses degrees.

### 7.3 Scalar quantities

Scale Factor and other dimensionless scalar requests accept no physical unit suffix.

Scale factor must be finite and strictly positive.

### 7.4 Bounded arithmetic expressions

R10 supports a deliberately small arithmetic grammar:

- parentheses;
- unary + / -;
- +;
- -;
- *;
- /.

Dimension rules are enforced.

Required examples include:

```text
25mm + 1in
2 * 12.5mm
(50 + 25)mm
90deg / 2
1 + 0.25
```

Invalid dimension combinations fail closed, for example adding an angle to a length or multiplying two lengths when the active request expects a length.

No variables, functions, document parameters, named dimensions or external expression engine are introduced.

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

### 8.2 Relative Cartesian

```text
@dU;dV
```

The active request must provide a semantic base point.

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

### 8.4 Direct Distance

A single length expression is Direct Distance only when the active PointRequest explicitly enables it.

Direction comes from the current highest-priority free direction after numeric locks and Ortho/Polar resolution.

PointRequest ownership remains context-first. While a PointRequest is active, a token that is not valid for that request does not silently activate a top-level command.

## 9. Numeric locks and point resolution

R10 extends the existing Input Resolution seam.

Priority during R10 is:

```text
1. complete explicit point coordinate / locked numeric fields
2. Ortho or Polar
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
- Ortho/Polar may constrain pointer-derived direction.

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
- Ortho/Polar pointer direction.

COPY repeated placement clears request-local locks/candidate direction after each accepted placement and keeps the existing frozen source snapshot.

Existing fresh-ID/high-water and one-placement-one-Undo semantics remain unchanged.

## 15. Grip Reshape / Grip Move / Grip Copy

Existing grip PointRequest remains the owner.

The interaction-start grip/pivot remains the semantic relative base.

R10 adds coordinate/polar/unit-aware input and Dynamic Input to that same request.

Space CycleEditMode and Grip Copy `C` keyword remain authoritative.

Numeric precision input must not change the affected-set rules or duplication identity semantics.

## 16. Rotate

Existing Rotate selection/base/reference workflow remains.

The final Rotate stage gains an explicit Angle input path.

Angle semantics:

- positive = counter-clockwise in Sketch U/V;
- negative = clockwise;
- zero follows existing no-op behavior;
- bare value = degrees;
- `deg` and `rad` accepted.

Pointer destination remains a valid adapter to the same Rotate semantic result.

Dynamic Input shows Angle at the final stage.

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

Dynamic Input shows Factor at the final stage.

R10 does not add non-uniform scale or Scale+Copy.

## 18. Mirror

Mirror remains a two-point axis workflow.

Axis Start and Axis End are precision-capable point requests.

Axis End may use relative Cartesian, relative polar and Ortho/Polar.

No Mirror angle-only shortcut or Mirror+Copy is introduced.

## 19. Ortho

Ortho is a runtime Sketch mode.

Default: OFF.

Enabling Ortho disables Polar.

For a based point request without a fully explicit coordinate:

- resolve raw pointer vector in Sketch U/V;
- choose the nearest of ±U / ±V;
- exact angular ties choose the U-axis family deterministically;
- magnitude remains pointer-derived unless a numeric Distance lock supplies it.

Ortho does not author Horizontal/Vertical constraints.

## 20. Polar

Polar is a runtime Sketch mode.

Default: OFF.

Enabling Polar disables Ortho.

Initial configuration:

- primary increment: 45 degrees;
- additional angles: none;
- reference mode: Absolute.

### 20.1 Absolute Polar

Tracks are measured from Sketch +U.

### 20.2 Relative Polar

Tracks are measured from the active operation reference direction.

Examples:

- continuous Line: previous committed segment direction when available;
- Rotate/Mirror/reference stages: the operation's accepted semantic reference direction when defined;
- otherwise Relative mode has no valid reference and fails back to raw pointer rather than inventing one.

### 20.3 Additional angles

Additional angles are runtime-only values in R10.

They augment the primary increment.

Duplicate/equivalent normalized angles are collapsed deterministically.

Invalid/non-finite angle configuration is rejected without changing the previous valid mode configuration.

## 21. Dynamic Input

Dynamic Input is a runtime UI adapter to the same semantic input request and global CAD input session.

Default per Sketch edit: OFF.

When enabled and the current request has precision fields:

- a compact overlay is displayed near the cursor/current semantic point;
- it never owns authored state;
- it never provides Viewer/provider identity;
- it mirrors the same active semantic values used by Command Line submission;
- printable keyboard input continues through the shared CAD input buffer;
- there is no second hidden text buffer.

### 21.1 Point fields

For based point requests, Dynamic Input must make these semantic fields available where meaningful:

- dU;
- dV;
- Distance;
- Angle.

Exact visual arrangement is D1, but all lockable fields must be reachable by Tab.

### 21.2 Typed value fields

Concrete request fields include:

- Circle Radius;
- Rotate Angle;
- Scale Factor;
- Polar increment/additional-angle configuration where exposed.

### 21.3 Tab

Tab cycles the active Dynamic Input field.

When the current buffer contains a valid value for that field, Tab may lock it and advance.

If the token is invalid, the field remains active and the request is unchanged.

### 21.4 Enter and Esc

Enter with a complete valid semantic value submits through the same request path as Command Line.

Esc ordering:

1. non-empty CAD buffer clears first;
2. otherwise active numeric field locks clear;
3. otherwise existing tool-stage Esc semantics apply.

Dynamic Input overlay/locks clear on:

- request acceptance;
- tool/stage completion;
- Esc cancellation;
- tool change;
- history boundary;
- Sketch/Document/runtime teardown.

## 22. Precision mode UI

R10 does not add more tool buttons to the top Create/Modify strip.

Precision modes/options belong to a contextual right-panel surface.

Expected bounded UI:

- current Part display/input unit selector;
- Ortho toggle;
- Polar toggle;
- Dynamic Input toggle;
- Polar increment;
- Polar Absolute/Relative choice;
- additional Polar angles entry/configuration.

The exact widget composition is D1, but Ortho/Polar mutual exclusion and current active state must be visually obvious.

Changing runtime Ortho/Polar/Dynamic Input configuration creates no CAD history.

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
- runtime Ortho/Polar/Dynamic Input modes and numeric locks are never serialized.

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
- `src/sketch/**` for expanded PointRequest resolution / numeric locks / Ortho-Polar direction resolution;
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
12. deg/rad conversion is correct;
13. bounded arithmetic respects dimensions;
14. invalid dimensional arithmetic fails closed;
15. absolute Cartesian uses semicolon component separator;
16. relative Cartesian requires a valid semantic base;
17. relative polar requires a valid semantic base;
18. Direct Distance remains request-gated;
19. PointRequest still outranks top-level command activation;
20. Line first/next points accept the specified coordinate grammar;
21. continuous Line advances the relative base and clears stale locks;
22. Circle center accepts point precision;
23. Circle radius accepts typed Length;
24. invalid/zero Circle radius fails closed;
25. Arc Start/Through/End accept point precision without changing Arc construction method;
26. Rectangle First Corner accepts absolute Cartesian;
27. Rectangle Opposite Corner accepts `@Width;Height`;
28. Rectangle bare scalar remains rejected;
29. Rectangle atomic 4/6-Line R9 semantics do not regress;
30. Move Base/Destination accept precision input;
31. Copy repeated placements accept precision input and retain fresh-ID/high-water semantics;
32. grip Reshape/Move/Copy accept precision point input without changing affected-set rules;
33. Rotate accepts explicit signed Angle;
34. Rotate positive/negative direction is deterministic;
35. Scale accepts explicit positive Factor;
36. Mirror axis accepts precision point input;
37. Ortho default is OFF;
38. Ortho constrains to Sketch U/V axes deterministically;
39. Polar default is OFF;
40. enabling Ortho disables Polar;
41. enabling Polar disables Ortho;
42. Polar primary increment defaults to 45 degrees;
43. additional Polar angles affect pointer resolution deterministically;
44. Absolute Polar uses Sketch +U;
45. Relative Polar uses only a valid semantic reference direction;
46. explicit complete point input outranks Ortho/Polar;
47. locked Distance outranks pointer magnitude;
48. locked Angle outranks Ortho/Polar direction;
49. conflicting locks fail closed;
50. Dynamic Input default is OFF per Sketch edit;
51. Dynamic Input has no authored state/history impact;
52. Dynamic Input and Command Line feed the same request;
53. Dynamic Input uses the shared CAD input buffer rather than a second text buffer;
54. Tab cycles available fields deterministically;
55. request acceptance clears request-local locks;
56. Esc follows buffer → locks → tool hierarchy;
57. Sketch/tool/Document teardown clears precision runtime state;
58. Measure displays correct current units without changing measurement semantics;
59. changing units updates Measure presentation without geometry mutation;
60. Construction dash-gap cadence is independent of entity length;
61. Construction preview and committed geometry use the same cadence policy;
62. Construction dash polish introduces no authored/persistent style state;
63. R8 Measure/Between regressions remain green;
64. R9 Rectangle/Construction/Profile regressions remain green;
65. selection/grips/transforms/COPY/Grip Copy regressions remain green;
66. global keyboard-first CAD input/focus arbitration regressions remain green;
67. persistence backward-read coverage remains green;
68. exact-head Windows FULL passes;
69. required internal + PL/EN docs and Product Browser freshness pass.

## 30. Manual Windows verification

Final implementation candidate requires Owner manual verification on the exact FULL-tested runtime head.

Minimum checklist:

- verify an existing pre-R10 Part opens as mm without geometry rescale;
- switch document display/input unit and verify geometry does not move;
- Save/Close/Reopen and verify selected document unit persists;
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
- Rotate typed positive and negative Angle;
- Scale typed Factor;
- Mirror precise axis points;
- enable Ortho and verify pointer direction locks to U/V without creating constraints;
- enable Polar and verify Ortho turns off;
- verify primary 45-degree tracks;
- test at least one additional Polar angle;
- verify Absolute versus Relative Polar reference;
- enable Dynamic Input and verify the overlay follows active semantic request;
- use Tab across Dynamic Input fields and lock at least one field;
- verify Command Line and Dynamic Input produce the same geometry;
- verify Esc clears live text, then numeric locks, then normal tool stage;
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
- persisting Ortho/Polar/Dynamic Input runtime state;
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
- accepted Ortho/Polar mode semantics;
- accepted Dynamic Input adapter semantics;
- Construction dash-gap carry-over completed without authored-style creep;
- no R11 OSNAP/inference or R12 structural-edit creep;
- affected/FOCUSED evidence;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- required internal + PL/EN Product documentation/current Browser;
- work-only CLOSURE closeout.

R11+ remain inactive after R10 completion unless separately accepted.
