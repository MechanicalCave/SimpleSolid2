# Sketcher Program Roadmap

**Status:** ACCEPTED  
**Version:** 1.2  
**Owner acceptance:** 2026-09-26  
**Foundation:** 1.0 (foundation-v1.0)  
**Architecture:** ADR-0008, ADR-0009  
**Current program:** Shared 2D Authoring / Part-hosted Sketcher  
**UX direction:** classical CAD interaction grammar, adapted to SS2 semantic ownership and command/transaction rules

## 1. Why v1.2 changed the program

Roadmap v1.1 correctly used Line as the first vertical slice through identity, persistence, Viewer input, Select, continuous creation and Delete.

That remains valid for R1–R4.

The next risk is different: if direct manipulation, precision input, snapping, measurement and future relations are all designed only against Line, the architecture can accidentally become endpoint/linear-geometry-specific.

The revised order therefore:

- keeps R0–R4 unchanged;
- establishes direct-manipulation foundations on Line;
- introduces Circle and Arc early;
- validates one interaction model across unlike primitives;
- only then expands precision input, Object Snap, tracking and later constraint-capable behavior.

R5 implementation is authorized only by an accepted R5 Work Contract. SK-05A is the active accepted R5 contract.

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
4. Ortho / Polar / geometric inference;
5. Grid Snap;
6. raw pointer.

A higher-priority explicit constraint is never violated to satisfy a lower-priority candidate.

Example: if Distance=50 is explicitly locked, an Endpoint at distance 48 is rejected. Snap/inference may only resolve remaining free parameters compatible with the locked value.

### 5.3 Ortho and Polar

Ortho and Polar Tracking are mutually exclusive.

Enabling one disables the other.

Object Snap remains higher priority than either.

Polar Tracking supports:

- user-configurable primary angle increment;
- additional user angles;
- Absolute mode relative to +U;
- Relative mode relative to the current operation's reference direction, such as the preceding segment;
- Absolute/Relative choice as user preference.

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
- uses the same active Ortho/Polar/inference direction rules as the rest of Input Resolution;
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

Grid Snap is lower priority than Object Snap and Ortho/Polar/inference.

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
**Status:** active — SK-06A accepted 2026-09-26

Goal:

- implement canonical Circle center+radius;
- implement canonical Arc center+radius+startAngle+signedSweepAngle;
- initial Circle Center+Radius creation;
- initial Arc 3-Point creation;
- create → present → select → delete → Undo/Redo → persistence;
- add the agreed primitive grips and minimal direct reshape/move paths;
- prove the R5 interaction architecture is not Line-specific.

### R7 — Common transforms, Copy and command grammar
Goal:

- one shared transform core for grip, toolbar and Command Line entry;
- Move, Rotate, positive uniform Scale and Mirror;
- semantic Space CycleEditMode;
- selection-first and command-first workflows;
- explicit Base Point for normal Move/Copy/Rotate/Scale;
- two-point mirror axis for normal Mirror;
- repeated Copy with fresh EntityIds and per-placement Undo;
- Copy modifier behavior during grip manipulation;
- context RMB behavior;
- Repeat Last Command;
- common preview/commit/cancel pipeline.

No multiple-active-grip feature is planned.

### R8 — Inspect / Measure / diagnostic dimensions
Goal:

- implement read-only Measure across Line/Circle/Arc and semantic sub-elements;
- context-dependent multi-entity measurements;
- Selection and whole-Sketch Show Dimensions runtime overlays;
- no authored dimensions or constraints.

### R9 — Precision input, Ortho, Polar and Dynamic Input
Goal:

- productionize the shared Input Resolution path;
- absolute/relative Cartesian input;
- relative polar input;
- direct distance;
- unit-aware and locale-safe numeric parser;
- Dynamic Input fields and Tab locking;
- Ortho/Polar mutual exclusion;
- configurable polar increments/additional angles;
- Absolute/Relative Polar preference;
- Command Line and Dynamic Input feed one request.

### R10 — Object Snap, Object Snap Tracking and inference
Goal:

- user-configurable OSNAP mode set;
- Temporary Snap Override;
- deterministic no-cycling candidate resolution;
- Object Snap Tracking hover acquisition with multiple temporary acquired points;
- shared tracking directions with Ortho/Polar/inference;
- endpoint/midpoint/center/quadrant/intersection/perpendicular/tangent/nearest/origin and justified extension modes;
- inference guides;
- no silent authored constraints.

### R11 — Optional authored relations / local solver
Goal:

- introduce explicit durable relation semantics only under later accepted contracts;
- keep snap/inference distinct from constraints;
- optionally support an explicit Auto-Constraint user setting;
- do not pre-decide driving/reference dimension architecture here.

### R12 — Geometry breadth and editing operations
Goal:

- Rectangle as a tool creating four ordinary Lines;
- construction role when concrete UI is delivered;
- later Polyline decision remains explicit;
- further primitives/edit operations only as justified;
- Trim/Split/Join must define identity outcomes before implementation;
- possible Offset, Fillet, Chamfer and further curve types.

### R13 — Planar region analysis and Sketch diagnostics
Goal:

- derive intersections/arrangement from evaluated non-construction geometry;
- detect bounded regions/open chains and diagnostics;
- preserve distinct region-analysis tolerances;
- never silently repair gaps.

### R14 — Part consumption of derived Sketch regions
Goal:

- Part consumes derived regions under an explicit persistence/rebinding contract.

### R15 — Planar-face Sketch support and projected/reference geometry
Goal:

- stable semantic support identity and frame;
- provider-neutral exact projected/reference curves;
- explicit reference/profile participation and associativity policy.

## 8. Decisions deliberately still open

The following remain future contract decisions because current evidence does not require them:

- exact C++ type hierarchy/enum representation for HandleRole, EditMode and Input Resolution;
- exact visual styling/size of grips, snap glyphs, tracking guides and Dynamic Input;
- exact application settings persistence implementation;
- exact dwell timing for Object Snap Tracking acquisition;
- exact numeric tolerances for snap/intersection/solver/region analysis;
- exact Polyline durable semantics;
- exact Trim/Split/Join identity rules;
- exact future constraint vocabulary/solver technology;
- authored dimension model;
- exact later curve primitive ordering;
- exact planar-face/projected-reference rebinding policy.

These open details must not be guessed by an implementation contract outside its scope.

## 9. Program state

R0 completed  
R1 completed  
R2 completed  
R3 completed  
R4 completed  
R5 completed — SK-05A closed after exact-head FULL #402 and Owner manual PASS  
R6+ not started

Roadmap v1.2 remains authoritative. R6+ is not authorized until a separate explicit Owner-accepted Work Contract is activated.
