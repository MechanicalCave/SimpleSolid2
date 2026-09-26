# SK-06A — Circle and Arc Core Breadth

**Status:** COMPLETED  
**Proposed:** 2026-09-26  
**Owner acceptance:** 2026-09-26  
**Owner manual Windows verification:** PASS — 2026-09-26  
**Final exact-head Windows FULL:** #443 — `bd42ac6954cfe36881cb187832f0f0ca745d22e0` — PASS  
**Decision class:** D2 durable primitive/persistence extension + bounded D1 interaction implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.2  
**Milestone:** R6 — Circle and Arc core breadth

## 1. Goal

Complete the accepted R6 primitive-breadth milestone by adding canonical Circle and Arc authored entities to the existing Shared 2D / Part-hosted Sketch path and proving that the R5 selection/direct-manipulation architecture is not Line-specific.

The slice must provide:

- canonical Circle and Arc authored geometry;
- initial Circle Center+Radius creation;
- initial Arc 3-Point creation;
- present → select → delete → Undo/Redo → persistence;
- agreed runtime grips and bounded direct reshape/move;
- the agreed square-grip visual language;
- no snapping/constraints/precision framework expansion.

## 2. Preserved authority and invariants

The Constitution, Foundation, ADR-0008, ADR-0009 and Sketcher roadmap v1.2 remain authoritative.

In particular:

- authored geometry is semantic Shared 2D state;
- Part owns the hosted Sketch lifecycle and persistence;
- EntityId remains opaque, stable and model-local;
- Viewer/Qt/OCCT identity is never CAD identity;
- hover, grips, preview and future snap feedback are runtime-only;
- durable edits go through semantic commands, validation and the normal Part transaction/history path;
- creation/editing use the existing resolved-input seam;
- one active semantic Sketch interaction state remains authoritative;
- provider-native presentation may not become a second semantic owner.

## 3. Circle canonical semantics

A Circle is authored as:

- `EntityId`;
- finite center `Point2`;
- finite radius `r > 0`.

No start angle, construction orientation or creation-method metadata is persisted.

Equal center/radius values do not imply equal identity.

### 3.1 Circle creation

Initial adapter: **Center + Radius**.

Interaction:

1. activate Circle;
2. first accepted resolved point establishes the center;
3. pointer movement previews the Circle only;
4. second accepted resolved point determines radius as the Sketch-plane distance from center;
5. exact zero radius is invalid and does not commit;
6. successful commit creates one Circle with one fresh EntityId and one Undo entry;
7. Circle remains active for another Circle until Esc/Select/tool switch, matching the existing continuous creation grammar;
8. pre-existing selection is preserved but grips remain hidden/inactive while Circle is active;
9. newly created Circle is not auto-selected.

Esc with an incomplete Circle request cancels only that request. A subsequent Esc after returning to Select follows the normal Select hierarchy.

## 4. Arc canonical semantics

An Arc is authored as:

- `EntityId`;
- finite center `Point2`;
- finite radius `r > 0`;
- finite `startAngle`;
- finite signed `sweepAngle`.

Angle convention is inherited from roadmap v1.2:

- 0 is +U;
- positive is counter-clockwise;
- convention is independent of camera orientation;
- signed sweep preserves CW/CCW and short/long meaning;
- a full 360-degree primitive is not an Arc.

Canonical Arc validation rejects:

- non-finite values;
- radius <= 0;
- zero sweep;
- |sweep| >= 360 degrees.

Equivalent angle representations may be normalized internally, but normalization must preserve the same geometric start/end and signed sweep meaning.

### 4.1 Arc 3-Point creation

The three accepted resolved points are:

1. Start;
2. Through;
3. End.

They define the unique circumcircle and the directed Arc from Start to End that passes through Through.

Fail closed with no authored mutation when:

- any two accepted points are exactly equal;
- the three points are collinear / no finite circumcircle exists;
- the resulting radius/sweep is invalid.

The Through point determines which of the two possible Start→End circular paths is authored and therefore determines CW/CCW plus short/long choice.

After one successful Arc commit, Arc remains active awaiting a new Start until Esc/Select/tool switch.

## 5. Shared 2D model and identity

`SketchModel` expands from Line-only authored state to a finite entity set containing:

- Line;
- Circle;
- Arc.

The implementation may use separate typed collections or another bounded representation. It must preserve:

- one shared EntityId namespace/high-water cursor across all primitive types;
- no EntityId reuse within a continuing model instance;
- lookup/erase by EntityId without provider identity;
- strict restore validation;
- value-copy semantics;
- existing Line behavior unchanged.

Adding, editing or deleting any primitive must preserve the same command/history atomicity rules already established for Line.

## 6. Persistence

R6 changes durable Part Sketch meaning and therefore requires an explicit schema extension.

Proposed policy:

- current writer advances Part domain schema from v3 to **v4**;
- v1/v2/v3 remain readable;
- v4 Sketch model persists Line, Circle and Arc records with explicit semantic kind;
- all primitive records persist EntityId plus canonical authored parameters only;
- `next_entity_id` remains the shared model high-water cursor;
- malformed/unknown entity kinds or inconsistent geometry fail closed;
- Save → Close → Reopen preserves primitive kind, geometry and EntityId;
- no provider/tessellation/grip/creation-method data is persisted.

No migration rewrites an existing file unless the user saves it through the ordinary current writer.

## 7. Presentation and hit testing

The provider-neutral Sketch scene expands only as much as needed to present Circle and Arc authored/preview geometry and semantic query tokens.

Qt/OCCT may use provider-native curve objects internally. Those objects remain derived presentation.

Point/rectangle selection must support Line/Circle/Arc through the existing semantic token bridge.

Window/Crossing membership is evaluated from current-view projected authored geometry according to the existing selection grammar. Exact provider algorithms are D1 if deterministic and bounded.

## 8. Runtime grips

### 8.1 Unified grip visual language amendment

The Owner-approved visual convention from the R5 review is recorded here.

All manipulable Sketch grips use the same square marker. The marker does **not** encode semantic role such as endpoint, midpoint, center, radius or arc midpoint; role is already visually clear from the grip's geometric location and may additionally be reflected by cursor/status text.

State is encoded instead:

- available / idle grip: small **hollow square**;
- hovered grip: **hollow square** with cyan emphasis;
- active / captured grip: **filled square** with yellow emphasis and may be slightly larger;
- after commit or cancel, the grip returns to the non-active hollow state;
- marker size is screen-space / DPI-aware, not world-space;
- hit tolerance remains a separate interaction parameter and must not be inferred from visible marker pixels.

"Active/captured" follows the R5 click-to-activate model and does not mean that the physical mouse button must remain held.

This changes presentation only; existing Line grip semantics remain unchanged.

### 8.2 Circle grips

Circle exposes:

- Center → default Move;
- +U quadrant → radius Reshape;
- +V quadrant → radius Reshape;
- -U quadrant → radius Reshape;
- -V quadrant → radius Reshape.

Circle Center and radius grips use the same square marker; only hover/active state changes the marker styling.

Radius Reshape:

- affects only the owning Circle;
- preserves center and EntityId;
- new radius = distance(center, resolved input);
- exact zero is invalid;
- preview is transient;
- LMB/Enter commits one semantic command;
- Esc cancels preview and preserves selection.

Center Move follows the R5 common-transform affected-set rule: it moves the complete frozen selected set, not only the Circle owner.

## 9. Arc grips

Arc exposes:

- Center → default Move;
- Start → owner-only start reshape;
- End → owner-only end reshape;
- Arc/Mid → owner-only radius reshape.

Visual convention:

- Center, Start, End and Arc/Mid use the same square marker;
- geometric location communicates the grip role;
- idle/hover/active state follows the unified hollow/cyan/filled-yellow grip language used by Line and Circle.

### 9.1 Arc Start reshape

Preserve center, radius and the previous End direction.

Resolved input determines the new Start direction.

Update start/sweep so:

- End direction remains geometrically fixed;
- sweep sign remains on the same CW/CCW branch when a valid non-zero result exists;
- the prior short/long branch is preserved where the new geometry admits an unambiguous continuation;
- ambiguous exact boundary cases fail closed rather than silently flipping branch.

### 9.2 Arc End reshape

Preserve center, radius and Start direction.

Resolved input determines the new End direction and signed sweep, preserving the existing CW/CCW branch unless the result would be invalid.

### 9.3 Arc/Mid radius reshape

Preserve:

- center;
- Start direction;
- End direction;
- signed sweep.

Change only radius to distance(center, resolved input). Exact zero is invalid.

### 9.4 Arc Center move

Uses the same frozen-selection common Move semantics as Line Center and Circle Center.

## 10. Selection, primary and delete

Existing additive/toggle selection grammar remains unchanged for all three primitives.

- ordinary pick adds;
- ordinary pick on selected entity preserves membership and makes it primary;
- Ctrl toggles;
- blank LMB/Esc clearing rules remain;
- Window/Crossing additive behavior remains;
- provider result order does not define primary;
- Delete removes the full selected set atomically across mixed Line/Circle/Arc entities;
- invalid/missing target in the frozen command set fails the whole mutation.

## 11. Commands, preview and history

Creation, delete and direct editing use semantic application commands through the current transaction path.

One accepted commit means:

- one semantic command;
- one staged authored state;
- one Part transaction;
- one revision increment if changed;
- one Undo entry.

Preview:

- creates no EntityId;
- does not dirty the Document;
- does not increment revision;
- creates no history entry.

Undo/Redo during an active Circle/Arc creation or manipulation cancels transient state first and then executes ordinary global history.

## 12. Interaction/tool surface

R6 adds Circle and Arc to the existing Sketch tool surface and command adapter only to the extent required for:

- toolbar/button activation;
- Command Line activation by `CIRCLE` and `ARC`;
- current resolved pointer path;
- Enter/LMB commit where applicable;
- Esc/tool-switch cancellation hierarchy.

No numeric field parser or Dynamic Input UI is activated.

## 13. Automated verification

At minimum prove:

1. Circle validation/add/find/erase/state/restore;
2. Arc validation/add/find/erase/state/restore;
3. one shared EntityId cursor across mixed primitive kinds;
4. Circle Center+Radius preview/commit/no-op-invalid behavior;
5. Arc Start/Through/End canonicalization and CW/CCW plus short/long cases;
6. duplicate/collinear Arc points fail closed;
7. mixed selection primary/toggle behavior remains deterministic;
8. mixed Delete is atomic and Undo/Redo restores kinds/identity;
9. Circle and Arc presentation/query works through semantic tokens;
10. Save → Close → Reopen preserves mixed Line/Circle/Arc geometry and EntityIds;
11. v1/v2/v3 documents remain readable and v4 malformed/unknown records fail closed;
12. Circle center Move uses frozen multi-selection;
13. Circle quadrant reshape is owner-only;
14. Arc Center Move uses frozen multi-selection;
15. Arc Start/End/Mid reshape preserves the contract invariants;
16. preview creates no revision/dirty/history mutation;
17. Line behavior and R5 regressions remain green;
18. native grip/query coverage includes square-marker presentation lifecycle and camera navigation;
19. FAST/SUBSYSTEM Sketch checkpoint passes during implementation;
20. exact-head Windows FULL passes for the final runtime candidate;
21. documentation verification and Product Browser freshness pass.

## 14. Manual Windows verification

Final exact-head candidate requires Owner verification of:

- all Line grips use one square shape, with hollow idle/hover and filled-yellow active/captured state;
- Circle creation feels continuous and preserves prior selection;
- Arc 3-Point preview/commit follows Start → Through → End as expected;
- Circle and Arc are visibly selectable with the existing additive/Ctrl grammar;
- Circle/Arc grips are clear, stable under zoom/DPI and reliably hit-testable;
- center-grip mixed selection Move is coherent;
- owner-only reshape does not move unrelated selected entities;
- Esc/Enter/LMB/Undo/Redo follow the established hierarchy;
- no stale native pixels or provider-hover leakage returns.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: R6 adds durable Circle/Arc geometry, persistence schema v4, creation tools, selection/direct-manipulation behavior and a user-visible square-grip language.

## 16. Expected implementation surface

Expected bounded production changes:

- `src/sketch/**`;
- `src/application/**`;
- `src/part/**`;
- `src/viewer/**`;
- `src/viewer_qt_occt/**`;
- `src/ui/**`;
- `tests/**`;
- affected internal/Product docs and generated Browser;
- `work/**`.

If implementation requires constraints, snapping/inference, authored dimensions, a generic overlay framework, a new cross-domain reference system or topology/provider identity in CAD semantics, stop for a separate D2/D3 decision.

## 17. Out of scope

Not authorized by SK-06A:

- Object Snap or snap target markers;
- Object Snap Tracking;
- Ortho/Polar/Grid Snap;
- numeric coordinates or Dynamic Input;
- constraints/solver/Auto-Constraint;
- authored dimensions;
- Rectangle/Polyline durable work;
- Move/Copy/Rotate/Scale/Mirror command grammar from R7;
- generic overlays;
- planar-face Sketch support;
- region/profile analysis;
- projected/reference geometry expansion.

## 18. Completion boundary

SK-06A is active after explicit Owner acceptance on 2026-09-26.

Completion requires:

- all durable Circle/Arc semantics remain within this contract;
- persistence v4/backward readability passes;
- required internal and PL/EN product documentation is current;
- final runtime candidate passes exact-head Windows FULL;
- Owner manual Windows verification passes;
- closeout CLOSURE passes;
- R7+ remains inactive.


## 19. Completion evidence

SK-06A completed its bounded R6 implementation and verification on 2026-09-26.

- Windows FULL #440 passed the 58-test suite before the documentation-only suffix.
- DOCS #441 and ready-for-review DOCS #442 passed documentation generation, freshness and validation.
- Owner manual Windows verification passed the accepted checklist.
- A non-functional source comment was then added to establish an exact-head FULL checkpoint over the complete documented candidate.
- Final exact-head Windows FULL #443 passed on `bd42ac6954cfe36881cb187832f0f0ca745d22e0`: Build succeeded and 58/58 CTest tests passed.
- R7+ remains outside this completed contract.
