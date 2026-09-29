# R9 — Rectangle + Construction Authoring Surface

**Status:** ACTIVE  
**Proposed:** 2026-09-29  
**Owner acceptance:** 2026-09-29  
**Decision class:** D2 Sketch creation/role interaction grammar + atomic multi-Line creation semantics; bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009, ADR-0011  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.6  
**Predecessor:** R8 Measure / relational diagnostics — completed  
**Milestone:** R9 — Rectangle and Construction authoring surface

## 1. Context

Roadmap v1.6 closes R8 through R8A + R8B and makes R9 the next planned Sketcher milestone.

The current product already has more Construction semantics than the old roadmap wording might suggest:

- `EntityRole::regular | construction` is durable authored Sketch state;
- Line, Circle and Arc already carry that role;
- persistence/history already preserve it;
- `SetSketchEntityRoleCommand` already converts the current semantic selection atomically;
- Construction geometry remains selectable/editable/measurable;
- Construction geometry is excluded from material-region/Profile formation by the Package-F region foundation;
- the current Operations surface already exposes Regular / Construction conversion for selected geometry.

R9 therefore must **not** invent a second Construction model.

What is still missing is a coherent authoring surface:

1. Rectangle creation as one logical CAD action that authors four ordinary Lines;
2. a deliberate runtime creation-role choice so Line/Circle/Arc/Rectangle can be created directly as Regular or Construction;
3. clear separation between "role for future creation" and "change role of existing selected geometry";
4. an optional runtime **Draw Diagonals** Rectangle mode that adds two ordinary Construction Lines across the accepted rectangle in the same atomic creation operation.

R9 must remain non-parametric. A created Rectangle is not a durable Rectangle object and receives no hidden horizontal/vertical/perpendicular/coincident constraints.

## 2. Goal

Provide classical, predictable profile-authoring breadth with:

```text
Create role: Regular | Construction
Create:
  Line
  Circle
  Arc
  Rectangle
```

Rectangle uses two opposite corners in the current Sketch U/V frame.

One accepted Rectangle placement creates exactly four ordinary authored perimeter Lines in one semantic command/transaction/Undo step. When **Draw Diagonals** is enabled, that same logical operation additionally creates the two diagonals as ordinary Construction Lines, for six Lines total in the same transaction/Undo step.

Construction creation uses the exact same geometry tools and command path, differing only in the authored `EntityRole` assigned at commit.

Existing selected-geometry role conversion remains available and independent.

## 3. Decision summary

Subject to explicit Owner acceptance, R9 adopts these bounded decisions:

1. **Rectangle is a creation tool, not a durable primitive.**
2. Rectangle geometry is axis-aligned to the active Sketch local U/V axes.
3. Interaction is exactly **First Corner → Opposite Corner**.
4. One Rectangle commit creates four ordinary Lines.
5. The four Lines receive four fresh EntityIds in one atomic transaction and one Undo step.
6. No RectangleId, group identity, hidden relation or constraint is created.
7. Shared exact corner coordinates do not imply shared authored endpoint identity.
8. After creation, every edge behaves exactly like an ordinary Line.
9. A runtime **Creation Role** has exactly `Regular | Construction`.
10. Creation Role applies to future Line/Circle/Arc/Rectangle commits and does not mutate existing geometry.
11. Creation Role defaults to Regular at the start of each Sketch edit session, persists across tool changes within that edit session and is not persisted.
12. Existing selected-geometry Regular/Construction conversion remains a separate authored operation and does not alter Creation Role.
13. Rectangle is available from the Create UI and as top-level `RECTANGLE` Command Line command.
14. Rectangle participates in Repeat Last Command like Line/Circle/Arc.
15. R9 introduces no Command Line aliases for changing Creation Role.
16. R9 introduces no solver, dimensional, OSNAP, Ortho/Polar or Dynamic Input behavior.
17. Rectangle's second corner is pointer-resolved through the existing provider-neutral Sketch point-input path; R9 adds no new numeric grammar.
18. A Regular rectangle participates in existing region/Profile analysis through its four perimeter Lines; a Construction rectangle is excluded through existing role semantics.
19. Rectangle exposes a runtime **Draw Diagonals** option, default OFF for a new Sketch edit session.
20. When Draw Diagonals is ON, the same Rectangle commit additionally creates `A→C` and `B→D` as ordinary `EntityRole::Construction` Lines.
21. Diagonals never inherit Regular role: a Regular rectangle yields four Regular perimeter Lines + two Construction diagonals; a Construction rectangle yields six Construction Lines.
22. Draw Diagonals is runtime-only, retained within the active Sketch edit session, non-persistent and non-authoritative for any later grouping.
23. The diagonal intersection does not create a center Point, RectangleCenter identity, constraint or durable relation; future OSNAP/Intersection may derive the center from ordinary geometry.

## 4. Scope IN

R9 includes:

- `SketchTool::rectangle` or equivalent finite semantic tool identity;
- two-corner axis-aligned Rectangle interaction state;
- provider-independent Rectangle preview intent;
- exact four-Line perimeter decomposition;
- optional two-diagonal Construction-Line decomposition controlled by runtime Draw Diagonals;
- one semantic/application Rectangle creation command or equivalent atomic batch command;
- one Part transaction / one revision / one Undo entry per accepted Rectangle;
- fresh EntityId allocation for all four perimeter Lines and, when enabled, both diagonal Lines;
- Rectangle toolbar action under Create;
- top-level `RECTANGLE` Command Line activation;
- Repeat Last Command support for Rectangle;
- runtime Creation Role `Regular | Construction`;
- visible Creation Role UI in Sketch edit;
- visible Rectangle **Draw Diagonals** option, default OFF, runtime-only and retained within the same edit session;
- direct Construction creation for Line/Circle/Arc/Rectangle;
- preservation of the existing selected-geometry role conversion UI/command;
- Regular/Construction visual distinction through the existing presentation policy;
- existing region/Profile behavior for Regular versus Construction;
- automated and manual Windows verification;
- required internal and PL/EN Product documentation.

## 5. Scope OUT

R9 does not authorize:

- a durable Rectangle entity;
- RectangleId or compound/group identity;
- Polyline semantics;
- shared endpoint identity;
- implicit Coincident constraints;
- implicit Horizontal/Vertical/Parallel/Perpendicular constraints;
- automatic geometric constraints of any kind;
- authored dimensions or solver state;
- editable width/height parameters;
- a parametric Rectangle feature;
- a durable Rectangle center point or RectangleCenter identity;
- durable grouping/membership tying diagonal Lines back to a Rectangle;
- automatic midpoint/intersection constraints for Rectangle diagonals;
- rotated Rectangle;
- center-based Rectangle;
- 3-point Rectangle;
- filleted/chamfered Rectangle variants;
- polygon tools;
- Construction as a separate primitive kind;
- a second Construction persistence representation;
- role-specific duplicate geometry types;
- a new persistence schema version solely for R9;
- Command Line `REGULAR` / `CONSTRUCTION` mode commands;
- numeric width/height entry;
- absolute/relative Cartesian input;
- polar input;
- unit expressions;
- Dynamic Input;
- Ortho/Polar;
- Grid Snap;
- OSNAP/tracking/inference;
- automatic profile creation;
- Trim/Split/Join;
- ordinary RMB context;
- authored constraints/parametric dimensions;
- Part solid modeling.

Those remain later accepted work.

## 6. Existing Construction authority is preserved

The current semantic role remains authoritative:

```text
EntityRole
  Regular
  Construction
```

R9 must reuse it directly.

No adapter may introduce a second flag such as:

- `isConstruction`;
- provider color meaning;
- toolbar-only construction state persisted independently;
- duplicated Line/Circle/Arc types;
- a special Construction presentation token treated as authored identity.

For an authored entity, role is read from the semantic Sketch model.

Presentation derives from that role.

## 7. Two independent role concepts

R9 must keep these concepts distinct.

### 7.1 Creation Role

Creation Role answers:

> What role should the **next newly committed geometry** receive?

Properties:

- runtime-only;
- exactly Regular or Construction;
- defaults to Regular on Sketch edit entry;
- retained across tool changes within the same active Sketch edit session;
- retained across Undo/Redo while the same edit session survives;
- reset when Sketch edit ends, the active Sketch/Document changes or the owning runtime is destroyed;
- never persisted;
- never dirties the Document;
- never creates Undo;
- changing it does not change selection.

### 7.2 Selected geometry role

Selected geometry role answers:

> What role should these **already-authored selected entities** have?

This is the existing authored mutation path through `SetSketchEntityRoleCommand` or its accepted equivalent.

Properties remain unchanged:

- valid only on semantic authored selection under the existing command rules;
- preserves EntityId;
- preserves geometry;
- may affect region/Profile validity because Construction is excluded from material-region analysis;
- is one atomic semantic role update for the accepted selected set;
- creates normal revision/dirty/history effects when it changes authored state;
- no-op role assignment must remain a clean no-op under existing command semantics.

Changing selected geometry role must not silently change Creation Role.

Changing Creation Role must not silently rewrite selected geometry.

## 8. Creation Role UI

The Sketch Create surface exposes the current Creation Role clearly.

The preferred bounded interaction is a visible checkable **Construction** creation toggle:

```text
Construction OFF → Creation Role = Regular
Construction ON  → Creation Role = Construction
```

Exact widget placement, iconography and styling are D1 as long as:

- the current creation role is visible before geometry is committed;
- it is visually associated with Create rather than selected-entity Properties/Operations;
- it does not masquerade as selection state;
- it remains usable with Line/Circle/Arc/Rectangle;
- existing selected-geometry Regular/Construction controls remain semantically distinct.

A two-button Regular/Construction creation selector is also acceptable if the distinction remains clear.

R9 does not require a new global application preference.

### 8.1 Rectangle Draw Diagonals option

The Rectangle creation surface also exposes a checkable **Draw Diagonals** option (`Rysuj przekątne` in Polish UI).

Properties:

- runtime-only;
- default OFF on entry to a new Sketch edit session;
- retained across Rectangle repetitions and ordinary tool switches while that edit session survives;
- reset to OFF when Sketch edit ends, the active Sketch/Document changes or the owning runtime is destroyed;
- never persisted;
- never dirties the Document and never creates an Undo entry by itself;
- may be changed while a Rectangle is pending; preview and final commit use the current visible option at commit time.

When OFF, Rectangle creates only the four perimeter Lines.

When ON, Rectangle additionally creates the two exact diagonals:

```text
A → C
B → D
```

Both diagonals are ordinary authored Lines with `EntityRole::Construction`, regardless of the current Creation Role for the perimeter.

This option creates no durable Rectangle membership, center point, relation or constraint. The intersection of the two independent Construction Lines is ordinary derived geometry that a future OSNAP/Intersection capability may use without any special Rectangle-center model.

## 9. Creation Role and active tools

Creation Role is orthogonal to the active creation tool.

Changing between Line/Circle/Arc/Rectangle does not reset Creation Role.

Each logical authored creation samples the current Creation Role at commit:

- Line — each accepted segment receives the role current for that segment's commit;
- Circle — each accepted Circle receives the role current at commit;
- Arc — each accepted Arc receives the role current at commit;
- Rectangle perimeter — all four perimeter Lines in one accepted Rectangle receive one identical role current for that Rectangle commit;
- Rectangle diagonals, when enabled — both receive `Construction` regardless of perimeter Creation Role.

The implementation must not produce mixed roles among the four perimeter Lines. The only intentional role difference inside one Rectangle operation is the explicit Construction role of enabled diagonals.

If the Creation Role UI can be changed while a Rectangle is between first and second corner, the role used at the final commit is the current explicit Creation Role shown to the user.

Changing Creation Role during a pending creation is runtime state only and does not consume identity or create history.

## 10. Rectangle tool grammar

Rectangle has exactly two geometric stages:

```text
await_first_corner
  ↓ valid point A
await_opposite_corner(A)
  ↓ valid non-degenerate point B
atomic Rectangle commit (four perimeter Lines, plus two Construction diagonals when enabled)
  ↓
await_first_corner
```

After a successful Rectangle commit the Rectangle tool remains active for another Rectangle.

This matches continuous CAD creation behavior and avoids relaunching the tool repeatedly.

Explicit Finish/Cancel UI follows the existing creation-tool surface.

## 11. Rectangle geometry

Let the first accepted corner be:

```text
A = (u0, v0)
```

and the opposite corner:

```text
C = (u1, v1)
```

Define:

```text
B = (u1, v0)
D = (u0, v1)
```

The authored perimeter is exactly:

```text
A → B
B → C
C → D
D → A
```

Each edge is an ordinary Line.

When Draw Diagonals is enabled, the authored diagonal geometry is exactly:

```text
A → C
B → D
```

Those two Lines are ordinary Construction geometry. Their intersection is not authored as a Point and receives no special persistent identity.

No Viewer projection or screen-axis geometry defines these points. They are computed in the current authoritative Sketch U/V frame after pointer resolution.

The first-corner/input order determines the authored Start/End orientation of the four resulting Lines. R9 does not canonicalize edge directions by world coordinates because there is no durable Rectangle identity whose orientation needs separate semantics.

## 12. Degenerate rectangle policy

A Rectangle is valid only when both extents are non-zero:

```text
u1 != u0
and
v1 != v0
```

Exact equality is used.

R9 introduces no Product geometric epsilon or minimum Rectangle size.

If the second point yields zero U extent or zero V extent:

- no Rectangle command is issued;
- no EntityId is consumed;
- no revision/dirty/history mutation occurs;
- the tool remains at the opposite-corner stage;
- a bounded diagnostic may explain the degenerate input.

Non-finite input fails closed.

## 13. Rectangle preview

After First Corner and before commit, pointer movement may produce one runtime Rectangle preview made from four derived perimeter Line segments and, when Draw Diagonals is enabled, two derived diagonal Line segments.

Preview:

- is provider-neutral in semantic geometry;
- uses the same exact decomposition as commit;
- is mapped to Viewer presentation through the existing Sketch preview boundary;
- is not authored;
- has no EntityIds;
- is not selectable;
- is not snap/inference authority;
- creates no revision/dirty/Undo state.

Invalid/degenerate pointer positions show no misleading valid Rectangle preview.

The provider must not independently infer Rectangle corners from pixels.

## 14. Atomic Rectangle commit

One accepted Rectangle is one logical authored operation.

The application/domain command may be named conceptually:

```text
CreateSketchRectangleCommand
```

or implemented through an equivalent bounded atomic multi-Line command.

The implementation shape is D1, but externally visible semantics are fixed:

- validate active Sketch identity/context;
- validate expected DocumentRevision according to current mutation rules;
- validate all four exact perimeter Lines and, when enabled, both exact diagonal Lines;
- assign the same accepted Creation Role to all four perimeter Lines;
- assign `EntityRole::Construction` to both enabled diagonals;
- allocate four fresh EntityIds when Draw Diagonals is OFF, or six fresh EntityIds when it is ON, only inside the accepted transaction;
- apply the complete four-Line or six-Line creation to the same transaction candidate;
- validate the complete resulting Part/Sketch state;
- commit the complete four-Line or six-Line Rectangle operation or none;
- create one DocumentRevision increment;
- create one Undo entry;
- mark dirty exactly once according to normal Document semantics.

Executing independent user-visible Line commands/transactions for the perimeter or diagonals is not acceptable.

## 15. EntityId semantics

One Rectangle commit creates four distinct ordinary perimeter Line identities, plus two distinct ordinary diagonal Line identities when Draw Diagonals is enabled.

Required properties:

- exactly four fresh EntityIds with Draw Diagonals OFF;
- exactly six fresh EntityIds with Draw Diagonals ON;
- no ID allocation during preview;
- no ID allocation when only First Corner is accepted;
- no ID allocation for degenerate/rejected/cancelled Rectangle;
- no partial identity consumption from a failed atomic commit;
- Undo restores pre-Rectangle geometry by removing the complete four-Line or six-Line logical creation;
- Redo restores the same four or six committed EntityIds;
- identity high-water follows the existing non-aliasing rules;
- later new geometry must not reuse identities that existing history semantics reserve against reuse.

R9 introduces no parent Rectangle identity.

## 16. No hidden endpoint relation

The four Lines share exact coordinate values at their corners, but their endpoint parts remain independent authored geometry.

For example, the end of `A→B` and start of `B→C` have equal coordinates at creation.

That means:

```text
equal coordinate
≠ shared endpoint identity
≠ Coincident constraint
≠ permanent topology link
```

After Rectangle creation, ordinary direct manipulation may move one Line endpoint and open the rectangle.

R9 does not silently preserve rectangularity after later edits.

That behavior is intentional until future authored constraints/parametric dimensions are explicitly introduced.

## 17. Regular Rectangle and Profile behavior

A Rectangle created with Creation Role = Regular creates four Regular perimeter Lines. If Draw Diagonals is enabled, it also creates two Construction diagonal Lines.

The four Regular perimeter Lines participate in the existing Package-F region/Profile analysis exactly as any four ordinary Regular Lines with exact shared coordinates. Enabled Construction diagonals remain excluded from material-region formation and therefore do not split or redefine the intended material region merely by crossing it.

R9 does not create a Profile automatically.

The existing user explicitly creates/edits Profile intent through the accepted Profile workflow.

R9 must not add gap healing or special "rectangle closes regardless" logic.

## 18. Construction Rectangle and Profile behavior

A Rectangle created with Creation Role = Construction creates four Construction perimeter Lines and, when Draw Diagonals is enabled, two additional Construction diagonal Lines.

Those Lines:

- are authored and persisted through existing role semantics;
- are selectable/editable/measurable;
- are excluded from material-region formation;
- do not create or split Profile material regions merely because they geometrically close.

Converting any of those ordinary Lines later to Regular uses the existing selected-role mutation path and existing region/Profile evaluation behavior. No durable membership causes the diagonals to follow perimeter role changes automatically.

No special Rectangle-specific Profile rule exists.

## 19. Existing role conversion remains authoritative

R9 preserves the current ability to select one or more Line/Circle/Arc entities and set them Regular or Construction through the existing semantic role command.

R9 may improve labels/layout so the distinction from Creation Role is obvious, but it must not fork the underlying mutation semantics.

Mixed-role selection remains a valid selection state.

Applying Regular or Construction to a mixed selection applies the requested role atomically under the existing batch role command.

Role conversion preserves EntityIds and exact primitive geometry.

## 20. Creation of Line/Circle/Arc with Construction role

R9 extends the existing creation commit path so Creation Role is honored for all current primitives.

For equal geometric input:

- Regular and Construction Line use the same Line geometry validation;
- Regular and Construction Circle use the same Circle geometry validation;
- Regular and Construction Arc use the same Arc geometry validation.

Role does not change:

- geometric validity;
- PointRequest resolution;
- preview geometry;
- EntityId allocation rules;
- commit atomicity.

Only the authored `EntityRole` differs.

## 21. Rectangle Command Line behavior

R9 adds case-insensitive top-level:

```text
RECTANGLE
```

It activates the same semantic Rectangle tool as the toolbar.

It is not a second implementation.

Context rules follow ADR-0011 and existing CAD input precedence:

- an active semantic PointRequest retains precedence over top-level command parsing;
- stale context fails closed;
- no inactive/hidden Document receives the command;
- Rectangle activation itself creates no authored mutation;
- rejected tokens create no mutation.

R9 adds no short alias such as `REC` or `R`.

## 22. No Command Line Creation Role grammar in R9

R9 deliberately does not add:

```text
CONSTRUCTION
REGULAR
CONSTRUCTION ON/OFF
```

as Command Line creation-mode keywords.

Reason:

- the existing Product also has authored selected-geometry role conversion;
- overloading the same words between "future creation role" and "mutate selected entities" would require additional context grammar;
- R9 does not need that grammar to deliver coherent direct Construction authoring;
- later command-surface convergence can add explicit non-ambiguous tokens if real workflow evidence justifies them.

The workspace/global CAD input router remains unchanged and domain-neutral.

## 23. Point input and precision boundary

Rectangle pointer points use the existing provider-neutral pointer → Sketch U/V input mapping.

R9 does not add new precision grammar.

After First Corner, the active semantic point request may expose that corner as a base for architectural consistency, but R9 does not define a new scalar Direct Distance meaning for "opposite rectangle corner".

A bare distance must not be guessed as:

- width;
- height;
- diagonal;
- square side;
- projected U or V extent.

R10 remains responsible for absolute/relative Cartesian input, units, Ortho/Polar and Dynamic Input.

This is a deliberate ambiguity boundary.

## 24. Repeat Last Command

Rectangle becomes a normal repeatable creation command.

Successful explicit Rectangle activation through toolbar or Command Line updates the existing session-local Repeat Last Command identity.

In ordinary Select with the accepted viewport focus rules:

- Enter/Space may repeat Rectangle through the same existing repeat mechanism;
- repeated Rectangle starts fresh at First Corner;
- prior rectangle points, preview or role snapshot are not replayed.

Repeat uses the **current Creation Role** at future commit time.

Changing Creation Role itself does not replace Repeat Last Command identity.

## 25. Selection invariants

Creation behavior follows the existing creation-tool doctrine.

Activating Rectangle:

- preserves pre-existing semantic selection;
- hides/deactivates grips according to current creation-tool behavior;
- does not turn selection into Rectangle input;
- does not auto-select created edges.

A committed Rectangle does not replace the existing selection.

Creation Role changes do not affect selection.

Selected-role conversion remains selection-based authored mutation and stays outside active creation tools according to the existing UI/command authority.

## 26. Esc / finish / lifecycle

Rectangle follows the existing hierarchical creation semantics.

Required behavior:

- at `await_opposite_corner`, first Esc clears First Corner/preview and returns Rectangle to `await_first_corner`;
- at empty `await_first_corner`, Esc returns to Select;
- Finish Rectangle / generic creation finish returns to Select;
- switching to another tool clears pending Rectangle preview/input;
- active Sketch/Document replacement clears pending Rectangle state;
- history boundary clears pending Rectangle transient state before ordinary history executes.

Already committed Rectangles remain authored as four Lines.

Creation Role is independent:

- cancelling Rectangle does not reset Creation Role;
- tool switching does not reset Creation Role;
- ending the Sketch edit/context does reset Creation Role to Regular for the next edit session.

## 27. Stale revision and command failure

Rectangle commit follows the current stale-context/stale-revision doctrine.

If the Document revision no longer matches the semantic command's accepted base:

- no partial Lines are committed;
- no Rectangle identities are accepted;
- transient Rectangle state is cleared or reconciled according to the current stale-command policy;
- a bounded diagnostic is reported.

A failure after semantic validation but before commit must remain all-or-nothing.

Provider presentation failure after a successful semantic commit does not roll back the committed four Lines; normal presentation recovery rebuilds from authoritative Document state.

## 28. Construction presentation

R9 reuses the existing Regular/Construction presentation distinction.

Exact line style/color is D1.

Presentation must make Construction distinguishable enough for conscious authoring, but:

- provider styling does not define EntityRole;
- selection/hover/grips remain semantic runtime projections;
- Construction remains selectable/editable;
- no role is inferred from color/dash pattern when mapping Viewer tokens back to semantic identity.

R9 does not require a new Viewer public identity concept.

## 29. Persistence

R9 is expected to require **no persistence schema change** because:

- Line/Circle/Arc roles already persist;
- Rectangle persists as four existing perimeter Lines plus, when enabled at creation, two existing Construction diagonal Lines;
- Creation Role and Draw Diagonals are runtime-only.

Save/Close/Reopen must preserve:

- the four created perimeter Line entities and, when present, both diagonal Line entities and their EntityIds;
- their Regular/Construction roles;
- existing next-EntityId high-water semantics;
- existing Profile/RegionIntent behavior.

Save/Close/Reopen must **not** preserve Creation Role or the Draw Diagonals runtime toggle; a new Sketch edit starts Regular with Draw Diagonals OFF.

If implementation discovers that a schema change is required solely to implement R9 as specified, stop for Owner review.

## Documentation impact

Internal docs: required  
User/Product docs: required  

Reason: R9 adds a user-visible Rectangle command, optional Construction diagonals and a direct Construction creation mode while clarifying the distinction between runtime Creation Role and authored selected-geometry role conversion.

Internal documentation must explain:

- Rectangle is an atomic four-Line command, not a durable primitive;
- exact U/V two-corner decomposition;
- identity allocation/Undo semantics for four-Line and optional six-Line Rectangle commits;
- Draw Diagonals runtime lifecycle, exact A→C/B→D geometry, always-Construction role and absence of center/group identity;
- no hidden endpoint relation or constraints;
- runtime Creation Role lifecycle;
- direct Construction creation for Line/Circle/Arc/Rectangle;
- separation from selected-role mutation;
- Construction/Profile exclusion through existing semantic role;
- Command Line `RECTANGLE` and Repeat Last Command behavior;
- precision boundary versus R10.

Product PL/EN documentation must explain:

- Create → Rectangle;
- First Corner → Opposite Corner;
- Rectangle remains active for repeated creation;
- Rectangle becomes four independently editable Lines;
- Creation Role Regular/Construction;
- difference between creating as Construction and converting selected geometry;
- Construction exclusion from material Profiles;
- lack of automatic rectangular constraints;
- Draw Diagonals behavior and its Construction semantics;
- current pointer-only rectangle precision limitation.

Generated Product Browser must be regenerated and deterministic.

## 31. Verification footprint

Prefer existing test executables and CI-03 FOCUSED iteration.

Expected affected surfaces include:

- Shared 2D interaction-state tests — Rectangle stages, preview/decomposition, optional diagonals, degeneracy and creation-role/runtime-option state;
- Shared 2D/model tests — role correctness and four ordinary Line semantics where appropriate;
- Part/application command tests — one atomic four-Line/six-Line Rectangle command, fresh identities, diagonal Construction role, stale revision, Undo/Redo and no partial commit;
- Part Sketch interaction controller tests — toolbar/semantic activation, preview/commit, role propagation, selection/lifecycle;
- CAD input semantic tests — `RECTANGLE` activation and context precedence;
- real Workbench Sketch-host tests — Rectangle button, Construction creation control, Draw Diagonals control, selected-role distinction and Repeat Last Command;
- persistence/history regressions — save/reopen and identity high-water;
- Package-F/Profile regressions — Regular rectangle can participate; Construction rectangle is excluded;
- existing Viewer/native regression — preview/presentation/selection remains stable.

Exact executable/target mapping is D1.

Verification infrastructure must not be weakened merely to obtain a cheaper tier.

Final runtime merge evidence remains exact-head Windows FULL.

## 32. Automated acceptance coverage

At minimum verify:

1. Rectangle has exactly First Corner → Opposite Corner geometric stages;
2. activation itself creates no mutation;
3. First Corner creates no authored geometry or EntityId;
4. preview derives exact A/B/C/D geometry in Sketch U/V;
5. opposite-corner drag works in all four U/V quadrants;
6. zero U extent is rejected/no mutation;
7. zero V extent is rejected/no mutation;
8. non-finite point input fails closed;
9. no new Product geometric tolerance is introduced;
10. valid Rectangle decomposes exactly to A→B, B→C, C→D, D→A;
11. the four results are ordinary Lines and there is no durable Rectangle entity;
12. Draw Diagonals OFF creates exactly four fresh EntityIds;
13. Draw Diagonals ON creates exactly six fresh EntityIds;
14. enabled diagonals are exactly A→C and B→D;
15. both enabled diagonals are always Construction regardless of perimeter Creation Role;
16. Regular perimeter + enabled diagonals yields exactly four Regular + two Construction Lines;
17. Construction perimeter + enabled diagonals yields exactly six Construction Lines;
18. Draw Diagonals defaults OFF on a new Sketch edit and is retained only within the active edit session;
19. changing Draw Diagonals creates no revision/dirty/Undo state;
20. Draw Diagonals creates no center Point, group identity, constraint or durable Rectangle membership;
21. preview/reject/cancel consume no EntityIds;
22. the complete four-Line or six-Line operation commits atomically or none do;
23. one Rectangle commit creates one DocumentRevision increment;
24. one Rectangle commit creates one Undo entry;
25. Undo removes the complete four-Line or six-Line logical creation;
26. Redo restores the same four or six EntityIds;
27. identity high-water obeys existing non-reuse semantics;
28. created endpoints may share exact coordinates without shared endpoint identity;
29. later independent Line edit can break rectangularity without hidden solver behavior;
30. Rectangle remains active after successful commit;
31. first Esc from pending opposite corner cancels pending Rectangle but keeps Rectangle active;
32. next Esc from empty Rectangle returns to Select;
33. tool/history/context replacement clears pending Rectangle preview;
34. pre-existing selection survives Rectangle activation/commit;
35. created Rectangle edges/diagonals are not automatically selected;
36. `RECTANGLE` Command Line activates the same semantic tool;
37. `RECTANGLE` follows active PointRequest/context precedence;
38. Rectangle becomes Repeat Last Command identity;
39. repeated Rectangle starts with fresh First Corner and uses current Draw Diagonals state;
40. Creation Role starts Regular for a new Sketch edit session;
41. Creation Role survives ordinary tool switches in the same edit session;
42. Creation Role resets across Sketch edit teardown/re-entry;
43. Creation Role changes create no revision/dirty/Undo state;
44. Creation Role changes do not mutate selected geometry;
45. selected-geometry role mutation does not change Creation Role;
46. Line commit uses current Creation Role;
47. Circle commit uses current Creation Role;
48. Arc commit uses current Creation Role;
49. all four Rectangle perimeter Lines receive one identical current Creation Role;
50. Regular and Construction use identical geometry validation;
51. selected role conversion preserves EntityId/geometry;
52. mixed selected roles can be atomically normalized by the existing role command;
53. Regular Rectangle participates in region analysis through ordinary perimeter Lines;
54. enabled Construction diagonals do not create/split material regions;
55. Construction Rectangle contributes no material boundary;
56. Construction remains selectable/editable/measurable;
57. no automatic Profile is created;
58. no constraints/dimensions/solver state is created;
59. no Command Line Regular/Construction mode grammar is introduced;
60. Rectangle adds no new numeric/coordinate/unit grammar;
61. Rectangle adds no OSNAP/Ortho/Polar/inference behavior;
62. persistence schema remains unchanged;
63. save/reopen preserves created Lines/roles/IDs but not runtime Creation Role or Draw Diagonals toggle;
64. existing Line/Circle/Arc creation regressions remain green;
65. existing selection/grip/transform/COPY/Grip Copy regressions remain green;
66. existing R8 Measure/Between regressions remain green;
67. existing Profile/RegionIntent regressions remain green;
68. exact-head Windows FULL passes;
69. required internal + PL/EN docs and generated Browser freshness pass.

## 33. Manual Windows verification

Final implementation candidate requires Owner manual verification on the exact FULL-tested runtime head.

Minimum checklist:

- enter Sketch edit and confirm Creation Role starts Regular;
- create an ordinary Line, Circle and Arc and confirm they remain Regular;
- switch Creation Role to Construction;
- create Line, Circle and Arc directly and confirm visible Construction presentation;
- verify those new Construction entities can still be selected, measured and grip-edited;
- verify Creation Role switching itself creates no dirty/Undo step;
- verify selected-geometry Regular/Construction controls still change existing entities and are visibly distinct from the Creation Role control;
- verify changing selected geometry role does not change the current Creation Role;
- switch between Line/Circle/Arc tools and verify Creation Role is retained;
- activate Rectangle from toolbar;
- click First Corner and move across each quadrant to inspect preview;
- click a valid Opposite Corner with Draw Diagonals OFF and verify exactly four visible Line edges are created together;
- enable Draw Diagonals and verify preview adds both diagonals;
- commit a Regular rectangle with Draw Diagonals ON and verify four Regular perimeter Lines + two Construction diagonals are created together;
- verify both diagonals are ordinary selectable/editable/measurable Construction Lines and no center Point is authored;
- switch Creation Role to Construction with Draw Diagonals ON and verify all six resulting Lines are Construction;
- toggle Draw Diagonals while a Rectangle is pending and verify preview/final commit follow the current visible option without creating history;
- verify Rectangle remains active and create a second rectangle;
- verify zero-width and zero-height attempts do not commit;
- verify Esc from pending second corner cancels only that pending rectangle; next Esc returns Select;
- activate `RECTANGLE` through Command Line and verify the same workflow;
- in ordinary Select use Repeat Last Command after Rectangle and verify a fresh Rectangle starts;
- with Creation Role = Construction, create Rectangle and verify all four edges are Construction;
- verify a Construction rectangle does not create a material region/Profile candidate by itself;
- with Creation Role = Regular, create Rectangle and verify existing region/Profile workflow recognizes the ordinary closed four-Line boundary;
- select one Rectangle edge and move/reshape it; verify the other edges do not follow as a hidden parametric rectangle;
- Undo immediately after a Rectangle-with-diagonals commit and verify all six Lines disappear together;
- Redo and verify the complete six-Line creation returns;
- Save/Close/Reopen and verify rectangle edges, roles and ordinary editability persist;
- re-enter Sketch edit and verify Creation Role starts Regular rather than persisting;
- regression smoke Measure/Between, selection/grips, Move/Copy/Rotate/Scale/Mirror, Grip Copy and Profile.

Manual visual review must also confirm:

1. Rectangle preview is stable and clearly distinct from committed geometry;
2. Construction presentation is readable and distinguishable from Regular geometry;
3. Creation Role UI cannot be confused with selected-geometry role conversion;
4. Rectangle/Construction controls fit the existing Select/Create/Modify/Inspect organization without introducing a competing tool surface.

## 34. Stop conditions

Stop for Owner review if implementation requires or attempts:

- a durable Rectangle/compound identity;
- a durable Rectangle-center identity or authored center Point created solely by Draw Diagonals;
- durable membership/grouping that binds the two diagonal Lines to the Rectangle after creation;
- a new persistent endpoint-sharing model;
- implicit constraints or solver behavior;
- keeping Rectangle rectangular after independent edits;
- a persistence/schema change;
- new Product geometric tolerance;
- new durable sub-element identity;
- changes to existing EntityId lifecycle rules;
- multiple user-visible transactions/Undo entries for one Rectangle, including separate diagonal commits;
- automatic Profile creation from Rectangle;
- special Profile semantics that bypass ordinary Regular/Construction region analysis;
- Command Line role-mode grammar beyond `RECTANGLE`;
- new numeric width/height/diagonal interpretation;
- absolute/relative coordinate syntax;
- units architecture;
- Dynamic Input;
- Ortho/Polar/Grid Snap;
- OSNAP/tracking/inference;
- topology-changing structural editing;
- authored dimensions/parametric relations;
- Part solid modeling.

These are separate D2/D3 scope expansions.

## 35. R10 continuation boundary

R9 does not activate R10.

After R9 completes, Roadmap v1.6 places R10 Precision Input / Units / Ortho / Polar / Dynamic Input next.

R10 remains a separate architecture/interaction contract.

In particular, R9's pointer-only Rectangle must not be used as justification to smuggle width/height, coordinate or unit parsing into the current input layer.

## 36. Completion boundary

R9 may become ACTIVE only after explicit Owner acceptance of this Work Contract.

Completion requires:

- accepted two-corner U/V Rectangle semantics preserved;
- Rectangle remains four ordinary perimeter Lines, with optional two ordinary Construction diagonals, and no hidden parametric/group identity;
- accepted runtime Creation Role semantics preserved;
- existing selected-role mutation remains distinct;
- no R10 precision/OSNAP/constraint creep;
- focused/affected iteration evidence;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- required internal and PL/EN Product documentation/current Browser;
- work-only CLOSURE closeout.

R10+ remain inactive after R9 completion unless separately accepted.
