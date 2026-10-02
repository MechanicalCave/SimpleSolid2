# Sketcher Program Roadmap

**Status:** ACCEPTED  
**Version:** 1.9  
**Owner acceptance:** 2026-10-02  
**Previous accepted version:** 1.8 — 2026-10-01  
**Foundation:** 1.0 (foundation-v1.0)  
**Architecture:** ADR-0008, ADR-0009, ADR-0011, ADR-0012  
**Current program:** Shared 2D Authoring / Part-hosted Sketcher with cross-cutting CAD Input dependency  
**UX direction:** classical CAD interaction grammar, adapted to SS2 semantic ownership and command/transaction rules
**Current scheduling:** R0–R12, WB-02, AUDIT-01 A–F, SR-01 and SR-02 are completed. Initial profile-authoring readiness evidence exposed bounded interaction-correctness, latency and shell-layout blockers. SR-03 Responsive Workbench Shell is ACTIVE by Owner acceptance on 2026-10-02 under its bounded Work Contract. The readiness checkpoint must be re-tested after SR-03. Split/Join remain deferred; Part Feature Tree and solid modeling remain inactive behind separate future gates.  

## 1. Why v1.8 narrows R12 structural editing

Roadmap v1.7 completed the R10 directional amendment and placed structural editing after R11. After R11 completion, Owner design review narrowed R12 to the three engineering operations actually required for current profile authoring:

- Trim;
- Extend;
- Extend Both to Virtual Intersection for two Lines.

Split and Join are no longer R12 requirements. They remain deferred until a concrete product need justifies reopening topology-branching or merge identity semantics.

This is a bounded R12 sequencing/scope amendment. The accepted sequence is:

```text
R8  Measure / relational diagnostics — completed
→ R9  Rectangle + Construction UI — completed
→ R10 Precision Input / Units / Polar / Dynamic Input — completed
→ R11 OSNAP / Tracking / Inference — completed
→ R12 Trim / Extend / Extend Both to Virtual Intersection — completed
→ initial Sketcher profile-authoring readiness review — concrete blockers found
→ SR-01 Interaction Correctness and Profile UX — completed
→ SR-02 Sketch Interaction & Presentation Latency — completed
→ SR-03 Responsive Workbench Shell — ACTIVE, accepted responsive-shell stabilization
→ Sketcher profile-authoring readiness re-test
→ separate Part Feature Tree architecture gate
→ only then may a first solid-operation contract be considered
```

Roadmap v1.6's Show Dimensions decision remains unchanged: standalone Show Dimensions stays deferred until future authored/parametric dimension and constraint architecture is ready.

Five guardrails remain explicit:

1. R8A/R8B runtime measurement references remain diagnostic and do not become durable dimension/sub-element identity by implication.
2. Standalone Show Dimensions is not a prerequisite for profile-authoring readiness; future viewport dimensions should follow the eventual authored/reference dimension architecture rather than pre-empt it.
3. R12 structural editing is authorized only by the accepted `work/R12_TRIM_EXTEND_MUTUAL_EXTEND.md` identity/reference rules; Split/Join remain deferred and must not be inferred from Trim implementation.
4. The profile-authoring checkpoint must have concrete acceptance evidence; it is not satisfied by a subjective statement that the Sketcher “feels sufficient”.
5. Readiness findings are closed only through the bounded SR-01 → SR-02 → SR-03 stabilization sequence. SR-01 and SR-02 are complete; SR-03 is active by separate Owner acceptance. None of these packages activates Part Feature Tree or solid modeling.

Ordinary RMB context remains deferred as a later UX-convergence item after the command set is broad enough to design the menu from stable operations. Existing already-accepted RMB semantics in command-first object collection remain unchanged.

Roadmap v1.8 recorded the Owner-accepted narrowing and completed delivery of R12. R12 completed after final exact-head Windows FULL #1104 and Owner manual Windows PASS. Roadmap v1.9 records concrete post-R12 readiness findings and the bounded pre-Part stabilization sequence SR-01 → SR-02 → SR-03 → readiness re-test. SR-01 completed on 2026-10-02 after exact-head Windows FULL #1128 and Owner manual Windows PASS. SR-02 completed on 2026-10-02 after exact-head Windows FULL #1169 and Owner manual Windows PASS on the final curve-style-corrected candidate. SR-03 is active by explicit Owner acceptance on 2026-10-02; its accepted shell direction is compact one-row Workspace information, one aligned active-Document top row, narrow default side panels and center-Viewport stretch priority. Split/Join and R13+ remain inactive.

## 2. Preserved accepted invariants

ADR-0008 and ADR-0009 remain authoritative.

In particular:

- Shared 2D owns reusable authored 2D meaning and remains host-neutral;
- authored primitive geometry is directly editable design intent;
- equal coordinates do not imply shared authored identity;
- optional future relations are distinct from geometry storage;
- snap, inference, hover, preview, grips and dynamic input are runtime concepts unless an explicit semantic command authors durable state;
- one active semantic interaction state has many UI/input adapters;
- Viewer/provider identity never becomes durable CAD identity;
- pointer input resolves through the active Sketch frame;
- Sketch coordinates are local physical U/V coordinates;
- Origin is an intrinsic semantic reference;
- authored and evaluated geometry remain distinct layers;
- durable references use semantic identity plus role rather than screen/provider identity;
- selection is transient semantic runtime state;
- diagnostics are read-only by default;
- closed regions are derived rather than authored Sketch entities by default;
- durable mutation remains Command → Validation → Transaction → owning Document → Evaluation.

## 3. Cross-cutting interaction doctrine

### 3.1 Consistency rule

A semantic operation should behave the same for one entity and many entities, and the same whether entered through grips, toolbar actions or Command Line, unless the nature of the operation makes that impossible.

Adapters may differ in how required inputs are obtained. The semantic operation, validation, preview meaning, commit meaning, identity rules and Undo granularity should remain shared.

### 3.2 Selection grammar

The Select grammar is:

- ordinary LMB on an unselected entity adds it to the current selection set;
- ordinary LMB on an already selected entity preserves membership and makes it primary;
- Ctrl+LMB toggles membership;
- ordinary Window/Crossing adds returned entities;
- Ctrl+Window/Crossing toggles returned entities;
- ordinary LMB on blank space clears selection in normal Select;
- Esc in normal Select clears selection and hides grips;
- selection is semantic runtime state keyed by EntityId;
- all selected editable entities may expose their runtime grips;
- exactly one grip is active at a time;
- multiple simultaneously active grips are not planned;
- when a manipulation starts, its selection set is frozen as a session snapshot until commit or Esc.

Primary remains a runtime convenience for context/property presentation; provider ordering must never define it.

### 3.3 Hover/preselection

Hover is separate runtime state:

- hovering an entity or grip may lightly highlight it;
- hover does not change semantic selection;
- LMB performs the actual selection or activates the grip;
- visible grip hit testing has priority over underlying geometry.

### 3.4 Edit handle role is not edit operation

A runtime edit handle identifies where/what is active. EditMode identifies how the current interaction is interpreted.

Conceptually a DirectManipulationSession contains:

- active SketchId;
- active EntityId and entity-specific HandleRole;
- frozen selection snapshot;
- current EditMode;
- pivot/base/reference point implied by the entry path;
- authored/evaluated geometry snapshot required for preview;
- current resolved input;
- transient preview state.

HandleRole must not be encoded as a provider token or durable entity.

### 3.5 Direct-manipulation entry and exit

Grip-started interaction follows this grammar:

- click a visible grip to begin the session;
- the active grip is the pivot/reference point;
- there is no separate user-changeable Base Point inside a grip-started session;
- pointer movement and other resolved inputs produce preview only;
- LMB or Enter may commit the current valid result;
- Esc cancels only the current uncommitted preview/session and returns to Select with the selection preserved;
- a subsequent Esc in Select clears selection;
- switching to another tool cancels the current uncommitted preview, preserves prior committed results and activates the new tool;
- Undo/Redo are boundaries of active manipulation: cancel current transient interaction first, then perform ordinary global history.

A normal non-Copy commit ends the manipulation session and returns to Select with the edited selection preserved.

### 3.6 Grip modes and common transform modes

The intended grip mode family is:

- Reshape/Stretch;
- Move;
- Rotate;
- Scale;
- Mirror.

Space is a semantic CycleEditMode adapter while viewport CAD interaction has focus.

For a reshape-capable grip the intended cycle is:

Reshape → Move → Rotate → Scale → Mirror → Reshape

For a grip whose default mode is already Move:

Move → Rotate → Scale → Mirror → Move

Exact implementation may introduce these modes incrementally by milestone, but the architecture must not require a redesign when later modes arrive.

Space inside text-entry focus remains ordinary text input and does not invoke CAD actions.

### 3.7 Affected-set rule

The operation determines the affected set.

- entity-specific Reshape/Stretch affects the owner of the active handle;
- common Move/Rotate/Scale/Mirror affects the frozen selection snapshot;
- if a handle's default mode is Move, multi-selection Move begins immediately and uses that handle as pivot;
- Copy applies to the same affected set as the current operation rather than defining a separate inconsistent set.

This rule applies equally to single- and multi-selection.

### 3.8 Shared transform core

Move, Copy, Rotate, Scale and Mirror must use one provider-independent semantic transform core regardless of entry adapter.

Grip-started transforms obtain the pivot/base point from the active grip.

Toolbar/Command-Line-started transforms use explicit command inputs:

- MOVE/COPY/ROTATE/SCALE: explicit Base Point;
- MIRROR: first and second points of the mirror axis.

Selection-first and command-first entry are both supported.

Selection-first:

- the existing selection set is used immediately;
- no extra "selection complete" confirmation exists;
- grip activation is a direct transition from Select into manipulation.

Command-first:

- the command enters a Select objects stage;
- successive picks/window/crossing build the command selection;
- blank LMB is a no-op in this stage;
- grips are hidden or inactive while command selection is being collected;
- Enter, Space or RMB completes the Select objects stage;
- Esc cancels the command but preserves the objects collected so far as the normal selection set.

After a completed transform, the same semantic selection remains selected and the editor returns to Select.

### 3.9 Copy modifier and repeated copy

Copy is an orthogonal modifier to the current edit/transform mode, not a replacement EditMode.

When Copy is ON:

- the source/affected set remains unchanged;
- each accepted placement creates fresh EntityIds;
- original entities remain the selected/reference set;
- created copies do not take over selection;
- each accepted repeated placement is one atomic semantic command, one transaction and one Undo entry;
- repeated placement remains active until Esc;
- each placement is derived from the original source state and the same base/pivot, not from the previously created copy;
- changing EditMode turns Copy OFF; the user must explicitly re-enable it for the new mode;
- Undo during an active Copy interaction cancels the transient session first, then performs normal global Undo and returns to Select.

For multi-selection, Copy duplicates the entire affected set for common transforms.

For Reshape+Copy, only the owner entity of the active reshape handle is copied and reshaped.

### 3.10 Transform details

Uniform Scale:

- factor must be greater than zero;
- zero and negative factors are invalid and create no commit;
- Mirror remains a separate semantic operation.

Mirror:

- command-started Mirror uses an explicit two-point axis;
- grip-started Mirror uses the active grip as the first axis point and resolved input for axis direction/second point;
- Mirror without Copy transforms existing entities;
- Mirror+Copy preserves originals and creates mirrored duplicates with fresh EntityIds.

Rotate uses the active/base pivot and the global Sketch angle convention.

### 3.11 Identity and atomicity

Editing existing geometry preserves EntityId.

Semantic duplication/Copy always creates fresh EntityIds.

For multi-object operations:

- one accepted user commit is one semantic command;
- one command is one staged transaction;
- one transaction produces one Undo entry;
- validation is all-or-nothing;
- partial mutation is forbidden.

This includes one accepted placement in repeated multi-copy.

### 3.12 Selection survives other tools

Starting another tool does not silently discard the existing selection.

For creation tools such as Line/Circle/Arc:

- existing selection is retained;
- grips are hidden/inactive while the creation tool is active;
- after finish/cancel and return to Select, the prior selection becomes fully interactive again;
- newly created geometry is not automatically added to selection.

### 3.13 Tool switching and Repeat Last Command

Explicitly activating another tool cancels only the current uncommitted transient state and activates the requested tool.

In ordinary Select with no active command:

- Enter or Space repeats the last repeatable CAD command;
- RMB opens the context menu;
- Repeat Last Command remembers command identity, not prior transient points, pivot, selection snapshot or dynamic-input values;
- current persistent user preferences continue to apply.

## 4. Primitive semantics fixed for early breadth

### 4.1 Line

Line remains authored start and end coordinates.

Runtime grips:

- Start → default Reshape;
- Center → default Move;
- End → default Reshape.

Center is a derived runtime location and does not create a durable midpoint entity.

### 4.2 Circle

Canonical authored Circle:

- center;
- radius.

Initial creation adapter:

- Center + Radius.

Runtime grips:

- Center → default Move;
- four quadrant grips → default radius Reshape with center fixed.

Different future creation methods normalize to the same canonical Circle and are not persisted as "creation method".

### 4.3 Arc

Canonical authored Arc:

- center;
- radius;
- startAngle;
- signedSweepAngle.

Angle convention:

- 0 degrees is +U of the Sketch;
- positive is counter-clockwise;
- convention is independent of camera orientation;
- signed sweep preserves CW/CCW and short/long arc meaning;
- a full 360-degree primitive remains Circle rather than Arc.

Initial creation adapter:

- 3 Point.

Runtime grips:

- Center → default Move;
- Start → reshape start while preserving center/radius and updating start/sweep;
- End → reshape end while preserving center/radius and updating sweep;
- Arc/Mid → radius reshape with center and start/end directions retained.

Different future Arc creation methods normalize to the same canonical representation and are not persisted as creation method.

### 4.4 Rectangle

Rectangle is planned as a creation tool, not a durable RectangleEntity.

Initial semantic result:

- four ordinary Line entities;
- each Line receives its own EntityId;
- later Auto-Constraint may optionally author relations, but Rectangle itself is not a required durable object.

### 4.5 Polyline

Polyline semantics remain deliberately open.

Continuous Line continues to create independent Line entities.

The architecture must not block a later real PolylineEntity if concrete workflows justify one.

## 5. Shared Input Resolution architecture

### 5.1 One resolver for creation and editing

Geometry creation and direct manipulation use the same Input Resolution layer.

Conceptual flow:

raw pointer/ray
→ raw Sketch-local U/V
→ candidate generation / locks / numeric constraints
→ deterministic resolution
→ resolved input
→ transient preview
→ explicit semantic commit

No geometry tool owns a private snap/precision pipeline.

### 5.2 Resolution priority

The priority is:

1. explicit numeric input / locked numeric fields;
2. Temporary Snap Override;
3. Object Snap / Object Snap Tracking;
4. Polar / geometric inference;
5. Grid Snap;
6. raw pointer.

A higher-priority explicit constraint is never violated to satisfy a lower-priority candidate.

Example: if Distance=50 is explicitly locked, an Endpoint at distance 48 is rejected. Snap/inference may only resolve remaining free parameters compatible with the locked value.

### 5.3 Polar directional assistance

R10 defines Polar as the sole angular directional-assistance mode. There is no separate Ortho runtime mode or resolver.

Classic orthogonal-only behavior is represented by:

```text
Polar spacing = 360/4 = 90°
```

Object Snap remains higher priority than Polar.

Polar supports:

- a user-configurable primary angle increment;
- individual additional user-angle tracks;
- Absolute reference relative to Sketch +U;
- Relative reference relative to the current operation's explicit semantic reference direction;
- magnetic screen-space capture/release rather than unconditional nearest-angle quantization.

The accepted R10 Work Contract owns exact defaults, runtime lifetime, shortcut and capture-hysteresis semantics.

### 5.4 Object Snap and tracking

Object Snap is a user-configurable set of enabled modes. Planned modes include as applicable:

- Endpoint;
- Midpoint;
- Center;
- Quadrant;
- Intersection;
- Perpendicular;
- Tangent;
- Nearest;
- Origin;
- Extension when justified.

Temporary Snap Override may force one snap type for the next point even if that type is not normally enabled, then automatically returns to normal OSNAP configuration.

There is no manual candidate cycling.

When several candidates are valid, deterministic resolution uses:

1. active Temporary Snap Override;
2. smallest screen-space distance to the cursor;
3. fixed semantic tie-break priority for practically equal candidates;
4. stable final tie-break by semantic identity/role.

Object Snap Tracking:

- acquires tracking points through hover plus a short dwell, without a click;
- may hold several acquired points for the current point request;
- uses the same active Polar/inference direction rules as the rest of Input Resolution;
- may expose intersections of tracking guides;
- acquired points are runtime-only;
- the acquired set clears when the current point is accepted or Esc cancels that point/request.

### 5.5 Inference

Inference may provide transient guidance such as:

- horizontal;
- vertical;
- parallel;
- perpendicular;
- tangent;
- collinear.

Inference remains runtime assistance.

With Auto-Constraint OFF, inference never creates durable relations.

A future optional Auto-Constraint setting may explicitly convert selected accepted relations into authored constraints. Exact constraint vocabulary and solver semantics remain future work.

### 5.6 Grid and Grid Snap

Grid is visual only.

Grid Snap is a separate optional input aid.

They can be enabled independently.

Grid Snap is lower priority than Object Snap and Polar/inference.

### 5.7 Dynamic Input and Command Line

Dynamic Input and Command Line are two adapters to one semantic input request.

Dynamic Input may expose separate fields such as distance/angle or DeltaU/DeltaV.

Tab cycles active fields. A user-entered field may lock one parameter while unresolved parameters continue to derive from pointer/snap/tracking.

Supported future numeric forms include:

- absolute Cartesian;
- relative Cartesian;
- relative polar;
- direct distance when direction is already resolved;
- operation-specific values such as radius, angle or scale factor.

Relative input always uses the active semantic base/reference point of the current operation.

Examples:

- continuous Line: the last committed endpoint;
- command-started Move: the chosen Base Point;
- grip edit: the active grip's interaction-start location.

### 5.8 Numeric syntax, units and locale

The parser should support:

- plain numbers in current document/display units;
- explicit unit suffixes such as mm, cm or in;
- both comma and dot as decimal separators;
- semicolon as the unambiguous Cartesian component separator in Command Line.

Examples of intended grammar direction:

- 12,5;30,25
- 12.5;30.25
- @25;10
- @50<45
- 12,5mm;2in

Exact prefix characters may be refined in the implementing precision-input contract, but the grammar must remain unambiguous.

## 6. Diagnostics and construction geometry

### 6.1 Measure

Measure is read-only and uses the same selection/input grammar as other tools.

Selection-first uses the existing selection.

Command-first may gather required semantic entities/sub-elements/points.

Single-primitive diagnostics should include at least:

- Line: length, DeltaU, DeltaV, angle relative to +U;
- Circle: radius, diameter, circumference, area;
- Arc: radius, start angle, end angle, signed sweep angle, arc length.

For multiple inputs, Measure chooses context-appropriate relations, for example:

- point-to-point distance, DeltaU/DeltaV and direction;
- angle between lines;
- point-to-line perpendicular distance;
- center distance and minimal geometric distance where applicable.

Measure targets semantic geometry/sub-element references such as Line Start/End/Midpoint, Circle Center/Quadrants and Arc Center/Start/End/Midpoint, not provider tessellation points.

### 6.2 Show Dimensions

Read-only diagnostic dimension overlays may support both:

- current selection only;
- the whole active Sketch.

These overlays are runtime-only:

- not selectable;
- not editable;
- not snap targets;
- no EntityId;
- no persistence;
- no Undo impact.

This does not decide authored driving/reference dimension semantics.

### 6.3 Construction geometry

Construction is a durable semantic role of authored geometry, not merely a display color.

Construction geometry remains available for:

- selection;
- snapping;
- measurement;
- inference;
- future constraints.

It is excluded from material region/profile construction by default.

## 7. Revised milestone order

### R0 — Foundation freeze and durable roadmap
**Status:** completed

Unchanged.

### R1 — Minimal Shared 2D authored core
**Status:** completed

Unchanged.

### R2 — Part host integration and durable lifecycle
**Status:** completed

Unchanged.

### R3 — Provider-neutral Sketch presentation and input boundary
**Status:** completed

Unchanged.

### R4 — First complete continuous Line workflow
**Status:** completed

Unchanged.

### R5 — Direct-manipulation and selection foundation on Line
**Status:** completed — SK-05A Owner manual PASS and FULL #402 on 2026-09-26

Goal:

- adopt the revised additive/toggle Select grammar;
- add hover/preselection;
- expose Start/Center/End grips for every selected Line;
- enforce one active grip and grip-over-geometry hit priority;
- establish frozen selection snapshot in DirectManipulationSession;
- implement endpoint Reshape and center-grip Move using the final affected-set rules;
- center-grip Move supports one or many selected Lines atomically;
- click-to-activate, preview-only movement, LMB/Enter commit and hierarchical Esc;
- preserve EntityId through edit/history/persistence;
- make Undo/Redo cancel transient manipulation before global history;
- preserve selection across creation-tool activation and after direct edits;
- establish the resolved-input seam with identity resolution initially;
- keep later EditModes and Copy structurally possible but inactive.

R5 does not need to ship Rotate/Scale/Mirror/Copy, precision input, snaps or constraints.

### R6 — Circle and Arc core breadth
**Status:** completed — SK-06A FULL #443 + Owner manual PASS on 2026-09-26

Goal:

- implement canonical Circle center+radius;
- implement canonical Arc center+radius+startAngle+signedSweepAngle;
- initial Circle Center+Radius creation;
- initial Arc 3-Point creation;
- create → present → select → delete → Undo/Redo → persistence;
- add the agreed primitive grips and minimal direct reshape/move paths;
- prove the R5 interaction architecture is not Line-specific.

### R7 — Common transforms, shared precision foundation, Copy and command grammar
**Status:** completed — SK-07A through SK-07G plus WB-02

Delivered:

- one shared transform core for grip, toolbar and Command Line entry;
- Move, Rotate, positive uniform Scale and Mirror;
- semantic Space CycleEditMode;
- selection-first and command-first workflows;
- explicit Base Point for normal Move/Copy/Rotate/Scale;
- two-point mirror axis for normal Mirror;
- repeated normal Copy with fresh EntityIds and per-placement Undo;
- shared semantic PointRequest / context-first Command Line routing;
- Direct Distance;
- workspace-global keyboard-first CAD Input through WB-02;
- Repeat Last Command;
- Grip Copy;
- common preview/commit/cancel foundations.

Ordinary RMB context menu is no longer treated as a blocker for R7 completion. The already-accepted RMB behavior in command-first Select objects remains unchanged; the ordinary Select context menu is deferred to a later UX-convergence slice when the available operation set is broader.

No multiple-active-grip feature is planned.

### R8 — Inspect / Measure diagnostics
**Status:** completed — R8A Read-only Measure + R8B Relational Measure / Runtime Semantic Targets

Delivered:

- read-only whole-entity Measure across Line/Circle/Arc;
- explicit runtime semantic point targets without durable sub-element identity;
- point↔point, point↔Line and Line↔Line relational diagnostics;
- bounded tool-local marker/relation cues;
- provider-neutral measurement derived from authoritative semantic geometry;
- no authored dimensions, constraints or solver.

Standalone Selection/whole-Sketch **Show Dimensions** viewport overlays are deliberately deferred by Roadmap v1.6. They are not required for R8 completion and should be reconsidered with future authored/parametric dimensions and constraints rather than built as an independent temporary overlay architecture.

### R9 — Rectangle and Construction authoring surface
**Status:** completed — exact-head Windows FULL #912 + Owner manual Windows PASS

Delivered:

- Rectangle as First Corner → Opposite Corner creation in Sketch-local U/V, decomposed to four ordinary independent authored Lines;
- optional Draw Diagonals as two additional ordinary Construction Lines in the same atomic commit/Undo step;
- runtime Creation Role for direct Regular/Construction Line/Circle/Arc/Rectangle authoring without duplicating the existing durable EntityRole model;
- distinct selected-geometry role mutation versus future-creation role;
- tool-only top Create strip with contextual Construction/Draw Diagonals options in the right Operations panel;
- `RECTANGLE` Command Line activation and Repeat Last Command;
- ordinary persistence/history/Profile behavior with no Rectangle-specific identity, grouping, center, constraint or schema.

Owner accepted one non-blocking visual follow-up for a later suitable Sketcher milestone: Construction dash-gap cadence should not stretch/normalize with entity length. The exact screen-space versus model-space cadence policy remains a later bounded presentation decision and does not reopen R9.

### R10 — Precision input expansion, Units, Polar and Dynamic Input
**Status:** completed — final exact-head Windows FULL #1009 + Owner manual Windows PASS

Goal:

- extend the workspace-global CAD input transport established by WB-02;
- extend the shared semantic Input Resolution path established by SK-07F rather than replace it;
- absolute/relative Cartesian and relative polar input;
- explicit unit-aware quantity/expression grammar beyond the bare scalar foundation;
- comma/dot decimal and semicolon Cartesian component grammar;
- Dynamic Input fields and deterministic lock/cycle behavior over the same CAD buffer/request;
- one Polar magnetic directional-assistance mode with configurable primary spacing, individual additional angles and Absolute/Relative reference;
- no separate Ortho mode; `360/4 = 90°` provides orthogonal-only attraction;
- accepted precision workflows for current creation tools, transforms and grip manipulation;
- Command Line and Dynamic Input feed the same semantic request.

Exact R10 semantics, defaults, scope exclusions, persistence behavior and completion evidence are owned by the completed `work/R10_PRECISION_UNITS_POLAR_DYNAMIC_INPUT.md` Work Contract.

### R11 — Object Snap, Object Snap Tracking and inference
**Status:** completed — final exact-head Windows FULL #1088 + Owner manual Windows PASS on 2026-10-01

Goal:

- user-configurable OSNAP mode set;
- Temporary Snap Override;
- deterministic no-cycling candidate resolution;
- Object Snap Tracking hover acquisition with multiple temporary acquired points;
- shared tracking directions with Polar/inference;
- endpoint/midpoint/center/quadrant/intersection/perpendicular/tangent/nearest/origin and justified extension modes;
- inference guides;
- no silent authored constraints.

R11 depends on the R10 input-resolution foundation rather than creating a second point-resolution system.

### R12 — Structural editing: Trim / Extend / Extend Both to Virtual Intersection
**Status:** completed — final exact-head Windows FULL #1104 + Owner manual Windows PASS on 2026-10-01

Goal:

- implement the minimum structural editing operations currently required for practical engineering profile authoring;
- Trim exactly one connected target fragment only when the result remains exactly one representable authored primitive;
- Extend one Line/Arc endpoint to an exact intersection with finite authored boundary geometry;
- Extend two Lines atomically to the unique virtual intersection of their infinite supporting lines when both finite Lines require extension;
- preserve explicit EntityId/reference outcomes: Line→Line and Arc→Arc preserve identity; Circle→Arc allocates fresh identity;
- keep Profile live-reference behavior fail-closed with no automatic RegionIntent rebinding;
- preserve atomic Command → Validation → Transaction → owning Document → Evaluation authority;
- introduce no authored constraints/solver, tolerance healing or hidden Split/Join semantics.

Current R12 does **not** include Split, Join, middle-fragment removal that leaves two target entities, Offset, Fillet, Chamfer, Break, Polyline or general topology genealogy.

The Work Contract is authoritative for exact operation/failure/identity rules.

### Sketcher profile-authoring readiness checkpoint
After R8–R12 are completed, the program must stop and verify that Sketcher can be used deliberately to author real Profiles.

Minimum checkpoint evidence must demonstrate, on supported Windows:

- practical creation/editing of Line, Circle, Arc and Rectangle;
- usable Regular/Construction workflow;
- read-only geometric inspection/measurement;
- precise numeric/unit-aware point entry;
- Polar and Dynamic Input;
- OSNAP/Tracking/Inference sufficient for deliberate geometric placement;
- Trim/Extend/Extend-Both with accepted stable identity/history behavior;
- Undo/Redo and Save/Close/Reopen across representative workflows;
- deliberate creation of valid Profile geometry without automatic gap healing or Viewer-dependent closure.

This checkpoint does **not** require authored constraints/solver, ordinary RMB context menu, planar-face Sketch support or every future editing tool.

Passing the checkpoint does not activate solid modeling.

### Cross-program gate — Part Feature Tree architecture before solid modeling
After the Sketcher profile-authoring readiness checkpoint, and before any first solid operation such as Extrude, SS2 requires a separate Owner-accepted Part architecture contract.

That gate must define at least:

- Part Body/Feature ownership and tree structure;
- durable feature identity;
- ordered history/dependency/evaluation semantics;
- references from a feature to Profile/Sketch inputs;
- edit/recompute/failure behavior;
- Delete and dependency behavior;
- visibility and Properties semantics;
- Undo/Redo and persistence meaning;
- separation of authored feature identity from derived kernel/provider topology;
- how future operations compose without relying on OCCT topology ordinals.

Package F provides Profile/Region technical readiness only. It does not satisfy this Part Feature Tree gate.

Even after the architecture gate passes, the first solid operation still requires its own separately accepted Work Contract.

### Later Sketcher evolution — not a prerequisite for the profile-authoring checkpoint

#### R13 — Authored relations, parametric dimensions and local solver
Goal:

- introduce explicit durable relation semantics only under later accepted contracts;
- keep snap/inference distinct from authored constraints;
- define stable semantic references for authored relations/dimensions only after R12 structural-edit identity outcomes are accepted;
- decide driving/reference dimension behavior, parameter editing and solver ownership from concrete workflows;
- optionally support an explicit Auto-Constraint user setting;
- reconsider Show Dimensions as a read-only presentation/projection of the accepted dimension/reference architecture rather than a separate runtime-only imitation.

R13 is deliberately after structural-edit identity stabilization. It is not automatically a prerequisite to the first Part architecture gate; the Owner may move it earlier if profile-authoring evidence shows that constraints or parametric dimensions are required for practical use.

#### R14 — Broader geometry and editing operations
Goal:

- add Offset, Fillet, Chamfer and further primitives/curve types only from concrete workflows;
- make any Polyline decision explicit;
- extend region analysis for future Regular curve kinds through the existing Package-F evaluated-curve seam;
- preserve identity/reference rules established by earlier structural editing.

#### R15 — Planar-face Sketch support and projected/reference geometry
Goal:

- stable semantic support identity and frame;
- provider-neutral exact projected/reference curves;
- explicit reference/profile participation and associativity policy.

### Deferred UX convergence — ordinary RMB context
Ordinary Select RMB context remains reserved for a later bounded contract after a sufficiently broad command set exists.

The future menu must adapt existing semantic operations rather than create parallel Move/Copy/Delete/Measure/etc. implementations. Exact menu contents, blank-space behavior, hit-under-cursor behavior and selection interaction remain open until that contract.

### Region/Profile roadmap synchronization

The old v1.4 R13/R14 direction for planar region analysis and Part consumption is no longer future work at its original scope.

AUDIT-01 Package F has already delivered:

- exact provider-neutral Shared 2D region analysis for current Regular Line/Circle/Arc;
- durable Part-owned ProfileId + RegionIntent live-reference semantics;
- Construction exclusion from material regions;
- Profile invalidation/recovery and diagnostics;
- persistence/lifecycle and common semantic authority across relevant UI/Command Line surfaces.

Future region/profile work is incremental extension of this completed foundation, not a restart of those old milestones.

## 8. Decisions deliberately still open

The following remain future contract decisions because current evidence does not require them:

- exact C++ type hierarchy/enum representation for HandleRole, EditMode and Input Resolution;
- exact visual styling/size of grips, snap glyphs, tracking guides and Dynamic Input;
- exact application settings persistence implementation;
- exact dwell timing for Object Snap Tracking acquisition;
- exact numeric tolerances for snap/intersection/solver/region analysis;
- whether future measurement sub-element references ever need durable identity beyond runtime diagnostics;
- exact Polyline durable semantics;
- R12 Trim/Extend identity rules are frozen by the accepted R12 contract; Split/Join identity semantics remain deliberately deferred;
- exact future constraint vocabulary/solver technology;
- authored driving/reference dimension model;
- exact later curve primitive ordering;
- exact planar-face/projected-reference rebinding policy;
- exact Part Body/Feature-tree architecture — reserved for the mandatory post-checkpoint Part architecture gate;
- exact future RMB context-menu contents and selection interaction.

These open details must not be guessed by an implementation contract outside its scope.

## 9. Program state

R0 completed  
R1 completed  
R2 completed  
R3 completed  
R4 completed  
R5 completed — SK-05A closed after exact-head FULL #402 and Owner manual PASS  
R6 completed — SK-06A closed after exact-head FULL #443 and Owner manual PASS  
R7 completed — SK-07A through SK-07G plus WB-02; ordinary RMB menu deferred by roadmap sequencing  
AUDIT-01 A–F completed — Package F already supplies the region/Profile foundation previously scheduled later  
R8 completed — R8A completed after exact-head FULL #867 and Owner manual PASS; R8B completed after exact-head FULL #889 and Owner manual PASS; standalone R8C Show Dimensions deferred by v1.6  
R9 completed — exact-head FULL #912 and Owner manual Windows PASS on bfe36544d71593a407bada63c07ae0e4b912808f  
R10 completed — final exact-head Windows FULL #1009; Owner manual Windows PASS; Polar-only directional assistance retained  
R11 completed — final exact-head Windows FULL #1088; Owner manual Windows PASS  
R12 completed — final exact-head Windows FULL #1104; Owner manual Windows PASS; Split/Join deferred  
R13–R15 inactive  
Initial Sketcher profile-authoring readiness review — concrete blockers recorded; checkpoint not passed  
SR-01 Interaction Correctness and Profile UX — completed 2026-10-02 after Windows FULL #1128 and Owner manual Windows PASS  
SR-02 Sketch Interaction & Presentation Latency — completed 2026-10-02 after Windows FULL #1169 and Owner manual Windows PASS  
SR-03 Responsive Workbench Shell — ACTIVE by Owner acceptance 2026-10-02; bounded responsive-shell contract  
Sketcher profile-authoring readiness re-test — blocked until SR-03 completes  
Part Feature Tree architecture gate inactive  
Solid modeling / Extrude inactive  
Ordinary Select RMB context menu deferred as a later UX-convergence slice

Roadmap v1.9 is authoritative for Sketcher feature sequencing. R9–R12, SR-01 and SR-02 are completed. Concrete readiness evidence now requires SR-03 before the readiness re-test. SR-03 is the only active production stabilization package; its accepted Responsive Workbench Shell contract is authoritative. Split/Join and R13+ remain inactive; no Part Feature Tree or solid-modeling scope is active.

## AUDIT-01 program interlock — completed

AUDIT-01 A-F is completed and no longer has scheduling precedence over Sketcher feature work.

The audit did not itself activate later Sketcher features. R7 resumed only by explicit Owner acceptance of SK-07G on 2026-09-29, and SK-07G is completed after FULL #849 and Owner manual PASS. Roadmap v1.9 keeps ordinary RMB context deferred, retains R8 complete through R8A/R8B, preserves the completed narrowed R12, records SR-01 and SR-02 as completed, and activates SR-03 as the remaining bounded stabilization package before the readiness re-test. Package F technical readiness does not activate solid modeling; a separate Part Feature Tree architecture gate and later solid-operation Work Contract remain mandatory.
