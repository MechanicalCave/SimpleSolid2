# Sketcher Program Roadmap v1.2 — PROPOSED AMENDMENT

**Status:** PROPOSED  
**Proposed:** 2026-09-26  
**Supersedes if accepted:** `work/SKETCH_ROADMAP.md` v1.1  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009  
**Current program:** Shared 2D Authoring / Part-hosted Sketcher  
**UX reference donors:** classical CAD interaction patterns proven by AutoCAD and nanoCAD; donor behavior does not override SS2 Foundation or semantic ownership rules

## 1. Why v1.2 is proposed

Roadmap v1.1 intentionally used Line as the first vertical slice through the Sketcher architecture.

That was appropriate for R1–R4 because one simple primitive was sufficient to prove:

- stable entity identity;
- Part-hosted persistence;
- semantic Commands/Transactions;
- Undo/Redo;
- provider-neutral presentation and pointer mapping;
- one runtime interaction authority;
- Select;
- continuous creation;
- point and rectangle selection;
- Delete;
- runtime preview.

Continuing to design all later Sketcher systems only against Line would create a new risk: architecture could become accidentally specialized around endpoint-based linear geometry.

Before freezing precision input, object snap, inference, measurement and solver behavior, SS2 should exercise the same interaction architecture against geometrically different primitives, especially Circle and Arc.

The amendment therefore keeps Line as the first direct-manipulation probe, then introduces Circle/Arc early, and only afterwards freezes broader cross-primitive precision and constraint systems.

## 2. Preserved frozen agreements

All accepted ADR-0008 / ADR-0009 invariants remain unchanged.

In particular:

- authored geometry remains directly editable design intent;
- equal coordinates do not imply shared authored identity;
- constraints are optional authored relations;
- snap/inference/dynamic input/grips are runtime interaction concepts unless an explicit command authors persistent state;
- one active semantic tool/edit state has multiple input adapters;
- Viewer/provider identity never becomes durable CAD identity;
- pointer input resolves through the active Sketch frame rather than assuming screen XY = Sketch UV;
- measurement/diagnostics are read-only by default;
- tolerance domains remain distinct;
- selection is transient semantic runtime state;
- closed regions remain derived rather than authored Sketch entities by default.

## 3. New architecture-readiness rules

The following rules are proposed specifically so future classical-CAD interaction features can be added without redesigning the Sketcher core.

### 3.1 Edit handle role is not edit operation

A runtime edit handle identifies **where / what semantic handle is active**.

The current edit mode identifies **how the gesture is being interpreted**.

Conceptually:

```text
EditHandle
  SketchId
  EntityId
  entity-specific HandleRole

DirectManipulationSession
  EditHandle
  EditMode
  base/reference point
  authored geometry at interaction start
  current resolved input
  runtime preview
```

A Line may initially expose Start / End / Center handles.

Future Circle, Arc and other primitives may expose different handle roles.

No universal assumption such as "endpoint grip always means Stretch" may be embedded into Viewer/provider transport.

### 3.2 Edit modes are semantic actions, not key codes

Future direct manipulation may support modes such as:

```text
Stretch / Reshape
Move
Rotate
Scale
Mirror
Copy variant
entity-specific grip action
```

The interaction layer must model mode changes semantically.

Keyboard bindings are adapters only, for example:

```text
Space or Enter
→ CycleEditMode
```

The semantic layer must not depend on Qt key enums or a hard-coded physical key.

This leaves the same action available later from Command Line, RMB/context menu, multifunction grip menu or another input device.

### 3.3 Raw pointer and resolved CAD input are separate

Interactive geometry must not commit directly from the raw pointer-plane intersection.

Required conceptual pipeline:

```text
pointer / ray
  ↓
raw Sketch-local UV
  ↓
input-resolution pipeline
  ↓
resolved Sketch-local input
  ↓
tool/direct-manipulation preview
  ↓
semantic command on explicit commit
```

Initially:

```text
resolved UV = raw UV
```

Future resolution stages may include:

- Ortho;
- Polar Tracking;
- angle lock;
- direct-distance entry;
- Cartesian/polar numeric input;
- Object Snap;
- Object Snap Tracking;
- geometric inference;
- temporary acquired tracking points;
- dynamic-input field overrides.

No individual geometry tool should implement a private duplicate of this pipeline.

### 3.4 Candidate generation, resolution and presentation are separate

Future snap/tracking/inference behavior should distinguish:

```text
candidate generation
→ candidate ranking/filtering
→ optional lock/acquisition
→ resolved point/value
→ visual explanation
```

A displayed glyph, guide line or tooltip is evidence of a runtime candidate/resolution; it is not authored CAD state.

This separation is required for future endpoint/midpoint/center/quadrant/intersection/perpendicular/tangent/nearest/extension candidates without making provider graphics authoritative.

### 3.5 Precision aids are orthogonal capabilities

The architecture must not conflate:

- Ortho;
- Polar Tracking;
- Object Snap;
- Object Snap Tracking;
- Grid Snap;
- numeric coordinate entry;
- direct-distance entry;
- dynamic input;
- geometric inference;
- persistent constraints.

A later product contract may define interaction between them, including mutually exclusive modes where appropriate, but the data model must not make them the same concept.

### 3.6 Dynamic Input and Command Line share the same semantic request

Future near-cursor fields and the compact Command Line must feed the same active interaction state.

Possible future value forms include:

- absolute Cartesian coordinates;
- relative Cartesian coordinates;
- absolute/relative polar coordinates;
- distance;
- angle;
- radius/diameter;
- entity-specific parameter values.

Tab/field navigation, text focus and formatting remain UI concerns.

Numeric input does not automatically create a driving dimension.

### 3.7 Tracking origin / base point is explicit runtime state

Operations such as Move/Rotate/Scale and tracking may require a base/reference point different from the initially selected handle.

The interaction session must therefore be able to carry an explicit runtime base/reference point without changing authored geometry.

Future "Base point" or "Reference" options must not require redesign of persistent entities.

### 3.8 Transform semantics are independent from presentation

Move/Rotate/Scale/Mirror are semantic geometry transformations or tool intents.

The provider may preview them, but Qt/OCCT must not define transformation meaning.

A future common transform layer may operate on one or more editable entities, while entity-specific reshape operations remain owned by the relevant geometry semantics.

### 3.9 Multi-selection and multi-handle activation remain possible

The R4 selection model already supports zero-to-many selected entities.

Future interaction may need:

- one primary selected entity;
- one active base handle;
- multiple active handles;
- multiple selected entities transformed together.

The first Line grip contract need not implement all of this, but it must not encode an invariant that only one selected entity or one selected handle can ever participate.

### 3.10 Copy during direct manipulation creates semantic duplication

Future Ctrl-copy or repeated-copy grip workflows must create fresh semantic entity identity.

They must not clone provider tokens or reuse the source EntityId.

Exact commit/Undo granularity for repeated copies remains a future Work Contract decision.

### 3.11 User interaction preferences are not CAD geometry

Future settings such as:

- Polar Tracking enabled;
- polar angle increment / additional angles;
- Object Snap enabled and enabled snap kinds;
- Object Snap Tracking enabled;
- Ortho enabled;
- Grid/Grid Snap settings;
- Dynamic Input enabled;
- grip size / visual preferences;

are runtime/user/application settings unless a later explicit product decision makes a particular setting document-owned.

They must not silently become authored Sketch geometry or alter durable entity identity.

### 3.12 Runtime visual channels remain explicit

Future Sketch interaction may require provider-neutral presentation channels for:

- edit handles;
- active/hover handle state;
- preview geometry;
- polar/ortho tracking rays;
- object-snap markers;
- snap-tracking acquisition markers;
- inference guides;
- angle/distance tooltips;
- dynamic-input fields;
- base/reference point markers;
- selection highlights.

These are runtime presentation.

The architecture must avoid returning to QWidget-over-native-viewport composition for graphics that belong inside the CAD surface.

### 3.13 Commit remains explicit and transactional

Regardless of how many runtime aids participate:

```text
pointer
+ tracking
+ snaps
+ numeric input
+ edit mode
+ preview
```

remain non-authored until an explicit accepted interaction commits a semantic command.

One accepted user operation must have deliberate, testable transaction/Undo granularity.

## 4. Revised milestone order

### R0 — Foundation freeze and durable roadmap

**Status:** completed

Unchanged from v1.1.

### R1 — Minimal Shared 2D authored core

**Status:** completed

Unchanged from v1.1.

### R2 — Part host integration and durable lifecycle

**Status:** completed

Unchanged from v1.1.

### R3 — Provider-neutral Sketch presentation and tool-input boundary

**Status:** completed

Unchanged from v1.1.

### R4 — First complete continuous Line workflow

**Status:** completed

Unchanged from v1.1.

### R5 — Direct-manipulation foundation on Line

Goal:

- refine primary selection semantics required for editing;
- establish provider-neutral runtime EditHandle transport;
- keep entity-specific HandleRole separate from EditMode;
- expose Line Start / End / Center runtime handles;
- implement endpoint reshape and whole-Line move as the first two modes actually delivered;
- establish a DirectManipulationSession with explicit base point and preview/commit separation;
- preserve Line EntityId through edit/Undo/Redo/persistence;
- insert the raw-input → resolved-input architectural seam even while initial resolution is identity;
- keep keyboard actions semantic so future Space/Enter mode cycling does not require Qt-specific domain changes.

Deliberately not required in R5:

- Rotate/Scale/Mirror;
- Space/Enter mode cycling in product UX;
- copy-via-grip;
- multi-active grips;
- snapping/tracking;
- numeric/dynamic input;
- constraints.

### R6 — Core geometry breadth: Circle and Arc

Goal:

- introduce Circle and Arc as genuinely different authored/evaluated geometric primitives;
- each primitive reaches the minimum lifecycle:
  create → present → select → delete;
- integrate Undo/Redo and persistence;
- define primitive-specific runtime handle roles;
- provide a minimal direct-edit path sufficient to test the R5 handle/session architecture;
- prove the Viewer/input contract is not Line-specific;
- establish exact geometry validation and degenerate-case policy per primitive.

Exact authoring variants for Arc remain contract-level decisions.

R6 should complete before broad Object Snap or solver contracts are frozen.

### R7 — Cross-primitive direct manipulation and grip-mode grammar

Goal:

- validate one direct-manipulation architecture across Line/Circle/Arc;
- add bounded common transform/edit modes as justified:
  Move, Rotate, Scale and Mirror;
- support semantic CycleEditMode action suitable for Space/Enter adapters;
- support explicit Base Point / Reference input where required;
- support entity-specific multifunctional grip actions without converting them into universal persistent sub-elements;
- consider multi-active-grip and multi-entity transform behavior under separate bounded contracts;
- consider copy-during-manipulation with fresh EntityIds and explicit Undo granularity;
- allow hover/context grip menus as presentation adapters to the same interaction state.

Not every listed mode must ship in one contract; R7 is the program area in which they are introduced incrementally.

### R8 — Inspect / Measure / geometry diagnostics

Goal:

- exact read-only geometry inspection across Line/Circle/Arc;
- point coordinates;
- length / arc length where applicable;
- line direction / angle;
- radius / diameter;
- center coordinates;
- endpoint distance/gap;
- horizontal/vertical deviation;
- intersection/tangency diagnostics as supported;
- observed geometry remains distinct from persistent constraints/dimensions.

### R9 — Precision input, Ortho, Polar Tracking and Dynamic Input

Goal:

- establish the shared input-resolution pipeline in product behavior;
- absolute and relative Cartesian coordinate entry;
- polar coordinate/value entry;
- direct-distance entry;
- angle input / temporary angle lock;
- Ortho mode;
- Polar Tracking with configured increments/additional angles;
- runtime tracking rays and angle/distance feedback;
- optional near-cursor Dynamic Input fields;
- Command Line and Dynamic Input remain adapters to one semantic request;
- numeric input remains distinct from authored dimensions.

Exact key bindings and preference persistence remain separate UX decisions.

### R10 — Object Snap, Object Snap Tracking and geometric inference

Goal:

- candidate generation and deterministic resolution over the available primitive set;
- support applicable candidates such as:
  Endpoint,
  Midpoint,
  Center,
  Quadrant,
  Intersection,
  Perpendicular,
  Tangent,
  Nearest,
  Origin,
  Extension;
- distinguish candidate identity from provider hit identity;
- support temporary acquisition/tracking from snap locations;
- combine Object Snap Tracking with the accepted Polar/Ortho resolution policy;
- expose runtime glyphs/guides/tooltips;
- keep snap/inference distinct from persistent constraints.

Exact candidate priority, cycling and temporary overrides require explicit contracts.

### R11 — Optional geometric relations and local solver/evaluation

Goal:

- introduce explicit authored relations under bounded contracts;
- begin with relations justified by the available Line/Circle/Arc geometry;
- preserve independent authored sub-element identity;
- direct manipulation re-evaluates legal constrained geometry;
- solver/evaluator produces structured diagnostics;
- constraints may consume semantic entity/sub-element roles but never screen/provider identity;
- snap/inference suggestions do not silently become constraints.

Potential future relation families include, only when explicitly contracted:

- Coincident;
- Horizontal / Vertical;
- Parallel / Perpendicular;
- Tangent;
- Equal;
- Concentric;
- radius/diameter or distance/angle dimensional intent.

This roadmap entry does not pre-authorize the exact relation set or solver technology.

### R12 — Geometry breadth and editing operations

Goal:

- add further primitives/tools only as required by concrete workflows;
- candidates may include Polyline, Rectangle tool semantics, Ellipse/Elliptical Arc and further curve types;
- add editing operations incrementally, such as:
  Move,
  Rotate,
  Scale,
  Mirror,
  Copy,
  Trim,
  Extend,
  Offset,
  Fillet,
  Chamfer;
- introduce construction/reference roles when required;
- preserve semantic identity rules and explicit topology-change policies.

A Rectangle may be a creation tool producing ordinary geometry rather than necessarily becoming a durable Rectangle entity; that decision remains explicit.

Trim/Split/Join identity outcomes must be decided before implementation.

### R13 — Planar region analysis and Sketch diagnostics

Goal:

- derive intersections/planar arrangement from evaluated non-construction geometry;
- detect bounded regions, open chains and relevant diagnostics;
- support Sketch-level profile/region inspection without authored mutation;
- keep region-analysis tolerance distinct from screen/snap/solver tolerances;
- never silently repair gaps merely because geometry appears visually closed.

### R14 — Part consumption of derived Sketch regions

Goal:

- allow Part modeling tools to query/select derived Sketch regions;
- keep Shared 2D ownership of geometry/regions and Part ownership of modeling meaning;
- explicitly decide associative versus snapshot consumption, durable rebinding policy and failure behavior before persistent Part feature implementation.

### R15 — Planar-face Sketch support and projected/reference geometry

Goal:

- introduce semantically stable planar-face support without persisting raw provider topology identity;
- derive stable Sketch frames;
- support exact provider-neutral projected/reference curves;
- keep projected/reference geometry read-only according to its accepted policy;
- decide support-boundary associativity and profile participation explicitly.

## 5. Future interaction backlog

The following behaviors are intentionally recorded as future capabilities so current architecture must not block them.

### Grip/direct-edit family

- hot/active/hover grip states;
- multifunctional grips;
- Space/Enter edit-mode cycling;
- context/RMB grip menu;
- Stretch/Reshape;
- Move;
- Rotate;
- Scale;
- Mirror;
- explicit Base Point;
- Reference angle/length;
- copy during grip manipulation;
- repeated copies during one manipulation session;
- multiple active grips;
- multi-entity transforms.

### Precision-input family

- absolute Cartesian input;
- relative Cartesian input;
- absolute/relative polar input;
- distance + direction;
- direct-distance entry;
- angle lock;
- Ortho;
- Polar Tracking;
- additional polar angles;
- Grid and Grid Snap;
- Dynamic Input;
- Tab navigation between dynamic fields;
- unit-aware display/entry.

### Snap/tracking family

- Endpoint;
- Midpoint;
- Center;
- Quadrant;
- Intersection;
- apparent/derived intersection if later justified;
- Perpendicular;
- Tangent;
- Nearest;
- Extension;
- Origin;
- Object Snap Tracking;
- acquisition/release of tracking points;
- candidate cycling when several valid candidates overlap;
- temporary snap overrides.

### Command/interaction family

- semantic keyboard actions independent from Qt key values;
- Command Line aliases/history/repeat-last-command;
- RMB/context command menu;
- hover grip menu;
- selection cycling;
- richer additive/subtractive rectangle selection;
- tool-specific prompts;
- cancel/finish hierarchy;
- temporary mode overrides.

### Editing/modeling family

- construction geometry;
- Move/Copy/Rotate/Scale/Mirror tools;
- Trim/Extend;
- Offset;
- Fillet/Chamfer;
- later curve primitives;
- constraint-aware direct manipulation;
- read-only diagnostics distinct from dimensions;
- derived profile/region workflows.

Recording an item here does not authorize implementation and does not freeze exact UX.

## 6. Important future decisions still deliberately open

The amendment does not decide:

- exact Circle/Arc authored representation;
- exact Arc creation variants;
- exact enum/class representation of HandleRole and EditMode;
- exact Space/Enter/Ctrl/Shift/RMB binding grammar;
- whether Ortho and Polar are mutually exclusive or composable in SS2;
- exact polar angle preference model;
- exact snap candidate ranking/cycling algorithm;
- exact temporary override behavior;
- exact dynamic-input widget/presentation;
- exact preference storage scope;
- exact multi-grip behavior;
- exact repeated-copy Undo granularity;
- exact transform pivot/base-point UX;
- exact topology identity behavior for Trim/Split/Join;
- exact constraint vocabulary and solver;
- exact auto-constraint policy;
- exact dimensional-constraint semantics;
- exact further primitive ordering after Circle/Arc.

These require evidence and separate accepted Work Contracts.

## 7. Proposed program state after acceptance

```text
R0  completed
R1  completed
R2  completed
R3  completed
R4  completed
R5  next — not active until an accepted R5 Work Contract
R6+ not started
```

Acceptance of roadmap v1.2 alone would not authorize R5 implementation.

A revised SK-05A contract must conform to the accepted v1.2 architecture-readiness rules before implementation begins.
