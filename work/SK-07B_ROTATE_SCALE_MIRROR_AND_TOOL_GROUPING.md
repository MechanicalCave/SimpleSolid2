# SK-07B — Rotate, Scale, Mirror and Sketch Tool Grouping

**Status:** COMPLETED  
**Proposed and Owner-accepted:** 2026-09-26  
**Owner manual Windows verification:** PASS — 2026-09-27  
**Initial runtime Windows FULL:** #503 — `90ac09b8af6b7da2656fc8088415aebcec452ebc` — PASS  
**Documentation gate:** #508 — `e4b8408fec7aa551132d28440303be60ab48ec67` — PASS  
**Final exact-head Windows FULL:** #510 — `dc58b767e2e3effbff90ba62d90a7a5f70fda5c5` — PASS  
**Decision class:** D2 transform/interaction semantics + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009, ADR-0010  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.2  
**Milestone:** R7 — Common transforms, Copy and command grammar — second bounded slice

## 1. Goal

Extend the SK-07A common transform architecture with normal `ROTATE`, positive uniform `SCALE` and `MIRROR` for mixed Line/Circle/Arc selections, while reorganizing Sketch toolbar actions into clear **Create** and **Modify** sections.

SK-07B must prove that Move/Rotate/Scale/Mirror share one semantic frozen-selection → transform specification → preview → atomic commit pipeline without introducing provider-owned CAD semantics.

`COPY` and identity-producing transform grammar remain explicitly deferred to SK-07C or later.

## 2. Preserved authority and invariants

The Constitution, Foundation 1.0, ADR-0008, ADR-0009 and Sketcher roadmap v1.2 remain authoritative.

Preserved invariants:

- authored Sketch geometry remains Shared 2D semantic state;
- Part owns hosted Sketch lifecycle and persistence;
- EntityId remains opaque, stable and SketchModel-local;
- Qt/OCCT handles, Viewer presentation tokens and projected geometry never become CAD identity;
- one Sketch interaction authority owns semantic selection and transient command state;
- all durable mutation goes through semantic application command → validation → DocumentSession → Part transaction/history;
- all pointer-derived positions enter through the existing resolved-input seam;
- preview is runtime-only;
- common transforms affect the complete frozen semantic selection snapshot;
- existing owner-only Reshape behavior remains unchanged;
- no persistence schema change is authorized;
- no multiple-active-grip feature is introduced.

## 3. Common transform geometry core

The provider-independent transform layer established by SK-07A is extended to support:

- translation;
- rotation about a finite Base Point;
- positive uniform scale about a finite Base Point;
- reflection about a finite two-point axis.

The layer operates on `SketchTransformGeometry` or an equivalent neutral semantic representation and never on Viewer/Qt/OCCT objects.

The exact C++ API is D1, but the implementation must not duplicate transform mathematics in toolbar, Command Line, controller or provider code.

### 3.1 Rotate

Given finite Base Point `B` and finite rotation angle `a`:

- Line: rotate Start and End around B;
- Circle: rotate Center around B; Radius unchanged;
- Arc: rotate Center around B; Radius and SignedSweepAngle unchanged; StartAngle changes by the same rotation.

Equivalent finite angle representations are acceptable; exact normalization range is D1 provided geometry is deterministic and persistence remains stable.

EntityIds are preserved.

### 3.2 Positive uniform Scale

Given finite Base Point `B` and finite factor `s > 0`:

- Line: scale Start and End around B;
- Circle: scale Center around B and set Radius = Radius × s;
- Arc: scale Center around B and set Radius = Radius × s;
- Arc StartAngle and SignedSweepAngle remain unchanged.

Non-finite, zero or negative scale is invalid and fails closed.

EntityIds are preserved.

### 3.3 Mirror

Given two distinct finite points `A` and `B` defining the mirror axis:

- Line: reflect Start and End;
- Circle: reflect Center; Radius unchanged;
- Arc: reflect Center and Start direction; Radius unchanged; SignedSweepAngle changes sign.

The resulting Arc must represent the geometrically reflected directed arc. Equivalent finite StartAngle representations are acceptable.

Two identical axis points are invalid and fail closed.

EntityIds are preserved.

## 4. Common affected-set semantics

Move, Rotate, Scale and Mirror use the same affected-set rules.

The affected set is a frozen snapshot of semantic EntityIds:

- any mixture of Line/Circle/Arc is valid;
- the snapshot is frozen when object collection completes or selection-first activation begins;
- membership cannot change while transform reference points or preview are active;
- commit revalidates all frozen targets against the current SketchModel and captured DocumentRevision;
- any missing/invalid target fails the entire mutation;
- partial mutation is forbidden.

## 5. Selection-first and command-first entry

`ROTATE`, `SCALE` and `MIRROR` reuse SK-07A MOVE object-selection semantics.

### 5.1 Selection-first

If the command starts with a non-empty semantic selection:

- use that selection immediately;
- freeze it;
- skip Select objects;
- enter the command's first reference-point stage.

### 5.2 Command-first

If the command starts with an empty semantic selection:

- enter Select objects;
- ordinary click adds and makes primary;
- re-picking an already selected entity preserves membership and makes it primary;
- Ctrl+click toggles membership;
- Window/Crossing reuse existing semantic selection rules;
- Ctrl+rectangle toggles returned EntityIds;
- blank LMB is a no-op;
- grips are hidden/inactive;
- Enter, Space or RMB completes object collection only when non-empty;
- Esc cancels the command and preserves collected objects as normal selection.

No second provider-owned command selection is permitted.

## 6. ROTATE interaction

Normal ROTATE stages:

```text
Select objects (command-first only)
→ Specify Base Point
→ Specify Reference Point
→ Specify destination point / preview
→ commit or cancel
→ Select
```

Let:

- `B` = Base Point;
- `R` = Reference Point;
- `P` = current destination.

Reference vector `R - B` must be non-zero.

The rotation is the signed angle from vector `R - B` to vector `P - B`, measured in the Sketch frame where positive is CCW.

If `P == B`, destination is invalid because no direction exists.

A destination collinear with the reference vector on the same ray produces zero rotation and is a valid no-op completion.

Preview always derives from the interaction-start geometry snapshot and never compounds frame-to-frame.

## 7. SCALE interaction

Normal SCALE stages:

```text
Select objects (command-first only)
→ Specify Base Point
→ Specify Reference Point
→ Specify destination point / preview
→ commit or cancel
→ Select
```

Let:

- `B` = Base Point;
- `R` = Reference Point;
- `P` = current destination.

Reference distance `|R-B|` must be non-zero.

```text
factor = |P-B| / |R-B|
```

Factor must be finite and strictly positive.

Therefore `P == B` is invalid.

Factor exactly `1` is a valid no-op completion.

No negative scale and no mirror-through-scale behavior is authorized.

Preview derives from the interaction-start geometry snapshot.

## 8. MIRROR interaction

Normal MIRROR stages:

```text
Select objects (command-first only)
→ Specify first axis point
→ Specify second axis point / preview
→ commit or cancel
→ Select
```

The two axis points define an infinite mirror line in Sketch coordinates.

The points must be finite and distinct.

After the first axis point, pointer movement previews the reflected frozen selection using the current second axis point when valid.

LMB or Enter commits the current valid axis.

If the resulting complete authored geometry is exactly unchanged, completion is a no-op with no revision/dirty/history change.

SK-07B does not add a "keep source" / Copy option to Mirror.

## 9. Commit, no-op and history semantics

For Move/Rotate/Scale/Mirror:

- preview creates no authored mutation, revision, dirty-state or history entry;
- one accepted non-no-op transform = one semantic command, one staged Part state, one transaction, one revision increment and one Undo entry;
- all edited EntityIds are preserved;
- stale revision, missing target, invalid input/output or non-finite geometry fails the whole operation;
- exact semantic no-op produces no revision increment, dirty-state change or Undo entry;
- Undo/Redo cancels any active transient common transform before global history;
- after successful commit, no-op or Esc cancellation, affected semantic selection remains selected and primary remains deterministic.

Persistence schema remains v4. Save → Close → Reopen must preserve transformed geometry and EntityIds.

## 10. Input and cancellation grammar

Common transform commands use existing resolved Sketch input.

Required behavior:

- LMB accepts the current point at the active point stage;
- Enter commits a valid final preview;
- Esc cancels current transform and returns to Select with affected selection preserved;
- subsequent Esc in ordinary Select follows the existing selection-clear grammar;
- tool switch cancels only uncommitted transform state and preserves selection;
- Space in text-entry focus remains text input;
- Space is only an object-collection completion key in the command-first Select objects stage.

Semantic `Space CycleEditMode` remains out of scope.

## 11. Toolbar organization

SK-07B implements the Owner-requested Sketch tool organization without introducing a new ribbon/framework.

Required grouping in Sketch Edit:

```text
Select

Create
  Line
  Circle
  Arc

Modify
  Move
  Rotate
  Scale
  Mirror
```

The exact bounded Qt composition (labels, separators or grouped containers) is D1, but the visual grouping must be clear and stable.

Requirements:

- Select is visually separate from creation/modification tools;
- Create groups Line/Circle/Arc;
- Modify groups Move/Rotate/Scale/Mirror;
- current active tool check state remains unambiguous;
- Operations and Command Line continue to reflect semantic command stage;
- toolbar organization must not own command state or geometry.

No general ribbon, customizable toolbar framework or icon system is authorized.

## 12. Command Line and Operations surface

Required Command Line keywords:

- `MOVE` — existing SK-07A behavior unchanged;
- `ROTATE`;
- `SCALE`;
- `MIRROR`.

Operations/status must expose finite current stages, including:

- Select objects;
- Specify Base Point;
- Specify Reference Point;
- Specify destination point;
- Specify first axis point;
- Specify second axis point.

Toolbar buttons and Command Line keywords are adapters to the same semantic command lifecycle.

## 13. Interaction-state architecture

One semantic runtime authority must represent Move/Rotate/Scale/Mirror command stages.

Exact state representation is D1, but it must satisfy:

- no parallel Qt-owned transform state;
- no Viewer-owned CAD command state;
- no second semantic selection owner;
- one common frozen-selection representation;
- one common preview/commit/cancel flow;
- transform-specific reference points are finite semantic runtime values;
- transient state cancels deterministically on Esc, tool switch, Undo/Redo, document switch or active-Sketch loss.

If implementation requires a second controller-owned semantic state machine, stop for Owner review.

## 14. Automated verification

At minimum prove:

1. Rotate math for mixed Line/Circle/Arc preserves EntityIds and unaffected canonical parameters;
2. Scale math for mixed Line/Circle/Arc preserves EntityIds and scales radius correctly;
3. Mirror math for mixed Line/Circle/Arc preserves IDs and flips Arc sweep sign correctly;
4. invalid/non-finite transform input fails closed;
5. selection-first Rotate/Scale/Mirror skips object collection;
6. command-first object collection matches MOVE selection grammar;
7. blank LMB is no-op during command object collection;
8. Enter/Space/RMB object-collection completion requires non-empty selection;
9. Rotate Base/Reference stages create no authored/history mutation;
10. Scale Base/Reference stages create no authored/history mutation;
11. Mirror first axis point creates no authored/history mutation;
12. preview always derives from the frozen interaction-start snapshot;
13. zero Rotate creates no authored/history change;
14. Scale factor 1 creates no authored/history change;
15. unchanged Mirror geometry creates no authored/history change;
16. non-zero transforms commit atomically as one revision/history entry;
17. stale revision/missing target causes no partial mutation;
18. successful transforms preserve selection/primary and EntityIds;
19. Esc/tool switch/Undo/Redo cancellation hierarchy is correct;
20. Save → Close → Reopen preserves transformed mixed geometry and EntityIds under schema v4;
21. MOVE and Center-grip Move regressions remain green;
22. owner-only Line/Circle/Arc reshape regressions remain green;
23. toolbar exposes clear Select / Create / Modify grouping;
24. toolbar and Command Line entry route to the same semantic transform state;
25. text-focus Space remains text input;
26. native Viewer/navigation/grip regressions remain green;
27. FAST/SUBSYSTEM Sketch/UI checkpoints pass during implementation;
28. final runtime candidate passes exact-head Windows FULL;
29. internal and PL/EN product docs plus Product Browser freshness pass.

## 15. Manual Windows verification

Final candidate requires Owner verification of:

- toolbar visibly separates Select, Create and Modify groups;
- selection-first and command-first ROTATE on mixed Line/Circle/Arc;
- selection-first and command-first SCALE on mixed Line/Circle/Arc;
- selection-first and command-first MIRROR on mixed Line/Circle/Arc;
- Arc orientation after Mirror is geometrically correct;
- Circle/Arc radii scale correctly;
- zero Rotate and factor-1 Scale produce no visible/history mutation;
- invalid zero reference distance / zero mirror axis do not commit;
- selection remains after commit and Esc;
- Enter/LMB/Esc hierarchy is coherent;
- command-first blank LMB does not clear collected objects;
- Space in text entry remains text;
- MOVE and existing grips/reshape remain unchanged;
- Undo/Redo and Save/Close/Reopen preserve transformed geometry and EntityIds;
- Pan/Orbit/Zoom/ViewCube remain stable with no stale pixels or hover leakage.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-07B adds three user-visible transform commands and changes Sketch toolbar organization.

## 17. Expected implementation surface

Expected bounded production changes:

- `src/sketch/**`;
- `src/application/**` only where common semantic geometry update plumbing requires it;
- `src/ui/**`;
- `src/viewer/**` only for bounded runtime preview/query state if necessary;
- `src/viewer_qt_occt/**` only if bounded presentation behavior requires it;
- `tests/**`;
- affected internal/Product docs and generated Browser;
- `work/**`.

No persistence schema migration is expected.

Stop for Owner review if implementation requires:

- COPY/fresh-identity semantics;
- generic matrices in persistence;
- provider identity in CAD semantics;
- numeric parser work;
- snapping/inference;
- generic overlay framework;
- a second semantic selection/interaction authority.

## 18. Explicitly out of scope

SK-07B does not authorize:

- Copy or repeated Copy;
- fresh EntityId creation from transforms;
- Copy modifier during grip manipulation;
- semantic Space CycleEditMode;
- ordinary-Select RMB context menu;
- Repeat Last Command;
- numeric angles, distances or scale values;
- Dynamic Input;
- Object Snap/tracking/inference;
- Ortho/Polar/Grid Snap;
- constraints/solver/authored dimensions;
- Rectangle/Polyline or new primitive breadth;
- planar-face Sketch support;
- region/profile work;
- general ribbon/customizable toolbar infrastructure.

These remain later R7+ work.

## 19. Completion boundary

SK-07B is active after the Owner explicitly accepted this scope on 2026-09-26.

Completion requires:

- Rotate/Scale/Mirror share the common semantic transform pipeline with Move;
- selection-first and command-first workflows are implemented for all three new commands;
- toolbar grouping is visibly reorganized into Select / Create / Modify;
- no COPY/fresh-identity behavior enters the slice;
- no persistence schema change occurs;
- required internal and PL/EN product documentation is current;
- final runtime candidate passes exact-head Windows FULL;
- Owner manual Windows verification passes;
- closeout CLOSURE passes;
- remaining R7 work stays inactive until separately accepted.

## 20. Completion evidence

SK-07B completed its bounded second R7 slice on 2026-09-27.

- Runtime head `90ac09b8af6b7da2656fc8088415aebcec452ebc` passed Windows FULL #503: Build succeeded and 63/63 unfiltered CTest tests passed.
- Documentation head `e4b8408fec7aa551132d28440303be60ab48ec67` passed DOCS #508, including deterministic Product Browser regeneration/freshness and stable `windows-msvc`.
- Owner manual Windows verification passed the accepted Rotate/Scale/Mirror and toolbar-grouping checklist on 2026-09-27.
- Numeric angle/distance/scale entry was manually clarified as intentionally out of scope: number keys do not override pointer-derived preview, and Enter commits the current valid preview.
- A non-functional common-transform authority comment established the final exact-head candidate `dc58b767e2e3effbff90ba62d90a7a5f70fda5c5` without changing runtime behavior.
- Final exact-head Windows FULL #510 passed on that candidate: documentation freshness, Build, FAST/SUBSYSTEM selector checks, 63/63 unfiltered CTest tests and stable `windows-msvc` all succeeded.
- Copy/repeated Copy, fresh transform-created EntityIds, Repeat Last Command, numeric/Dynamic Input, snapping/inference and R8+ remain outside this completed contract.
