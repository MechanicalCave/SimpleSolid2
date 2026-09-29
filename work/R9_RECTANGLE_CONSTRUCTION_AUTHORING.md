# R9 — Rectangle + Construction Authoring Surface

**Status:** PROPOSED — INACTIVE  
**Proposed:** 2026-09-29  
**Owner acceptance:** pending  
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
3. clear separation between "role for future creation" and "change role of existing selected geometry".

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

One accepted Rectangle placement creates exactly four ordinary authored Lines in one semantic command/transaction/Undo step.

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
18. A Regular rectangle participates in existing region/Profile analysis through its four Lines; a Construction rectangle is excluded through existing role semantics.

## 4. Scope IN

R9 includes:

- `SketchTool::rectangle` or equivalent finite semantic tool identity;
- two-corner axis-aligned Rectangle interaction state;
- provider-independent Rectangle preview intent;
- exact four-Line decomposition;
- one semantic/application Rectangle creation command or equivalent atomic batch command;
- one Part transaction / one revision / one Undo entry per accepted Rectangle;
- fresh EntityId allocation for all four Lines;
- Rectangle toolbar action under Create;
- top-level `RECTANGLE` Command Line activation;
- Repeat Last Command support for Rectangle;
- runtime Creation Role `Regular | Construction`;
- visible Creation Role UI in Sketch edit;
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

## 9. Creation Role and active tools

Creation Role is orthogonal to the active creation tool.

Changing between Line/Circle/Arc/Rectangle does not reset Creation Role.

Each logical authored creation samples the current Creation Role at commit:

- Line — each accepted segment receives the role current for that segment's commit;
- Circle — each accepted Circle receives the role current at commit;
- Arc — each accepted Arc receives the role current at commit;
- Rectangle — all four Lines in one accepted Rectangle receive one identical role current for that Rectangle commit.

The implementation must not produce a mixed-role Rectangle from one logical commit.

If the Creation Role UI can be changed while a Rectangle is between first and second corner, the role used at the final commit is the current explicit Creation Role shown to the user.

Changing Creation Role during a pending creation is runtime state only and does not consume identity or create history.

## 10. Rectangle tool grammar

Rectangle has exactly two geometric stages:

```text
await_first_corner
  ↓ valid point A
await_opposite_corner(A)
  ↓ valid non-degenerate point B
atomic four-Line commit
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

After First Corner and before commit, pointer movement may produce one runtime Rectangle preview made from four derived Line segments.

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
- validate all four exact Lines;
- assign the same accepted Creation Role to all four;
- allocate four fresh EntityIds only inside the accepted transaction;
- apply all four Lines to the same transaction candidate;
- validate the complete resulting Part/Sketch state;
- commit all four or none;
- create one DocumentRevision increment;
- create one Undo entry;
- mark dirty exactly once according to normal Document semantics.

Executing four independent user-visible Line commands/transactions is not acceptable.

## 15. EntityId semantics

One Rectangle commit creates four distinct ordinary Line identities.

Required properties:

- exactly four fresh EntityIds;
- no ID allocation during preview;
- no ID allocation when only First Corner is accepted;
- no ID allocation for degenerate/rejected/cancelled Rectangle;
- no partial identity consumption from a failed atomic commit;
- Undo restores pre-Rectangle geometry;
- Redo restores the same four committed EntityIds;
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

A Rectangle created with Creation Role = Regular creates four Regular Lines.

Those Lines participate in the existing Package-F region/Profile analysis exactly as any four ordinary Regular Lines with exact shared coordinates.

R9 does not create a Profile automatically.

The existing user explicitly creates/edits Profile intent through the accepted Profile workflow.

R9 must not add gap healing or special "rectangle closes regardless" logic.

## 18. Construction Rectangle and Profile behavior

A Rectangle created with Creation Role = Construction creates four Construction Lines.

Those Lines:

- are authored and persisted through existing role semantics;
- are selectable/editable/measurable;
- are excluded from material-region formation;
- do not create or split Profile material regions merely because they geometrically close.

Converting those four Lines later to Regular uses the existing selected-role mutation path and existing region/Profile evaluation behavior.

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
- Rectangle persists as four existing Lines;
- Creation Role is runtime-only.

Save/Close/Reopen must preserve:

- the four created Line entities and their EntityIds;
- their Regular/Construction roles;
- existing next-EntityId high-water semantics;
- existing Profile/RegionIntent behavior.

Save/Close/Reopen must **not** preserve Creation Role; a new Sketch edit starts Regular.

If implementation discovers that a schema change is required solely to implement R9 as specified, stop for Owner review.

## 30. Documentation impact

Internal docs: required  
User/Product docs: required  

Reason: R9 adds a user-visible Rectangle command and a direct Construction creation mode while clarifying the distinction between runtime Creation Role and authored selected-geometry role conversion.

Internal documentation must explain:

- Rectangle is an atomic four-Line command, not a durable primitive;
- exact U/V two-corner decomposition;
- identity allocation/Undo semantics;
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
- current pointer-only rectangle precision limitation.

Generated Product Browser must be regenerated and deterministic.

## 31. Verification footprint

Prefer existing test executables and CI-03 FOCUSED iteration.

Expected affected surfaces include:

- Shared 2D interaction-state tests — Rectangle stages, preview/decomposition, degeneracy and creation-role runtime state;
- Shared 2D/model tests — role correctness and four ordinary Line semantics where appropriate;
- Part/application command tests — one atomic four-Line Rectangle command, fresh identities, stale revision, Undo/Redo and no partial commit;
- Part Sketch interaction controller tests — toolbar/semantic activation, preview/commit, role propagation, selection/lifecycle;
- CAD input semantic tests — `RECTANGLE` activation and context precedence;
- real Workbench Sketch-host tests — Rectangle button, Construction creation control, selected-role distinction and Repeat Last Command;
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
12. one Rectangle commit creates exactly four fresh EntityIds;
13. preview/reject/cancel consume no EntityIds;
14. all four Lines commit atomically or none do;
15. one Rectangle commit creates one DocumentRevision increment;
16. one Rectangle commit creates one Undo entry;
17. Undo removes the complete four-Line logical creation;
18. Redo restores the same four EntityIds;
19. identity high-water obeys existing non-reuse semantics;
20. created endpoints may share exact coordinates without shared endpoint identity;
21. later independent Line edit can break rectangularity without hidden solver behavior;
22. Rectangle remains active after successful commit;
23. first Esc from pending opposite corner cancels pending Rectangle but keeps Rectangle active;
24. next Esc from empty Rectangle returns to Select;
25. tool/history/context replacement clears pending Rectangle preview;
26. pre-existing selection survives Rectangle activation/commit;
27. created Rectangle edges are not automatically selected;
28. `RECTANGLE` Command Line activates the same semantic tool;
29. `RECTANGLE` follows active PointRequest/context precedence;
30. Rectangle becomes Repeat Last Command identity;
31. repeated Rectangle starts with fresh First Corner;
32. Creation Role starts Regular for a new Sketch edit session;
33. Creation Role survives ordinary tool switches in the same edit session;
34. Creation Role resets across Sketch edit teardown/re-entry;
35. Creation Role changes create no revision/dirty/Undo state;
36. Creation Role changes do not mutate selected geometry;
37. selected-geometry role mutation does not change Creation Role;
38. Line commit uses current Creation Role;
39. Circle commit uses current Creation Role;
40. Arc commit uses current Creation Role;
41. all four Rectangle Lines receive one identical current Creation Role;
42. Regular and Construction use identical geometry validation;
43. selected role conversion preserves EntityId/geometry;
44. mixed selected roles can be atomically normalized by the existing role command;
45. Regular Rectangle participates in region analysis through ordinary Lines;
46. Construction Rectangle contributes no material boundary;
47. Construction remains selectable/editable/measurable;
48. no automatic Profile is created;
49. no constraints/dimensions/solver state is created;
50. no Command Line Regular/Construction mode grammar is introduced;
51. Rectangle adds no new numeric/coordinate/unit grammar;
52. Rectangle adds no OSNAP/Ortho/Polar/inference behavior;
53. persistence schema remains unchanged;
54. save/reopen preserves created Lines/roles/IDs but not runtime Creation Role;
55. existing Line/Circle/Arc creation regressions remain green;
56. existing selection/grip/transform/COPY/Grip Copy regressions remain green;
57. existing R8 Measure/Between regressions remain green;
58. existing Profile/RegionIntent regressions remain green;
59. exact-head Windows FULL passes;
60. required internal + PL/EN docs and generated Browser freshness pass.

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
- click a valid Opposite Corner and verify exactly four visible Line edges are created together;
- verify Rectangle remains active and create a second rectangle;
- verify zero-width and zero-height attempts do not commit;
- verify Esc from pending second corner cancels only that pending rectangle; next Esc returns Select;
- activate `RECTANGLE` through Command Line and verify the same workflow;
- in ordinary Select use Repeat Last Command after Rectangle and verify a fresh Rectangle starts;
- with Creation Role = Construction, create Rectangle and verify all four edges are Construction;
- verify a Construction rectangle does not create a material region/Profile candidate by itself;
- with Creation Role = Regular, create Rectangle and verify existing region/Profile workflow recognizes the ordinary closed four-Line boundary;
- select one Rectangle edge and move/reshape it; verify the other edges do not follow as a hidden parametric rectangle;
- Undo immediately after a Rectangle commit and verify all four edges disappear together;
- Redo and verify the complete Rectangle returns;
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
- a new persistent endpoint-sharing model;
- implicit constraints or solver behavior;
- keeping Rectangle rectangular after independent edits;
- a persistence/schema change;
- new Product geometric tolerance;
- new durable sub-element identity;
- changes to existing EntityId lifecycle rules;
- multiple user-visible transactions/Undo entries for one Rectangle;
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
- Rectangle remains four ordinary Lines with no hidden parametric identity;
- accepted runtime Creation Role semantics preserved;
- existing selected-role mutation remains distinct;
- no R10 precision/OSNAP/constraint creep;
- focused/affected iteration evidence;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- required internal and PL/EN Product documentation/current Browser;
- work-only CLOSURE closeout.

R10+ remain inactive after R9 completion unless separately accepted.
