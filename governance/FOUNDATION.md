# SimpleSolid 2.0 — Product & Architecture Foundation

**Status:** FROZEN  
**Version:** 1.0  
**Date:** 2026-09-22  
**Project:** SimpleSolid 2.0 (SS2)

---

## 1. Purpose and authority

This document defines the stable product and architecture foundation of SimpleSolid 2.0.

It freezes the **shape of the system, ownership boundaries, durable invariants and product philosophy**. It intentionally does not freeze detailed APIs, algorithms, persistence schemas or implementation class structures that require later evidence.

The Foundation exists to prevent accidental architectural drift while leaving implementation free to evolve deliberately.

Decision strength used in this document:

- **CORE** — product/architecture invariant. Changing it requires an explicit Foundation Amendment approved by the Owner.
- **DIRECTION** — accepted direction. It may evolve deliberately through an ADR or later contract without redefining the whole product.
- **OPEN** — intentionally unresolved. Implementation must not silently choose an architectural answer where an explicit later decision is required.

A freeze means:

> **“Do not change this accidentally.” It does not mean “this can never change.”**

---

## 2. Product philosophy

### 2.1 One coherent CAD application — CORE

SimpleSolid 2.0 is one coherent engineering CAD application.

Part, Sheet Metal, Assembly, Drawing and future domains must share common concepts for document lifecycle, identity, commands, transactions, selection, units, persistence, presentation and automation instead of becoming separate mini-applications.

### 2.2 Design intent is persistent; evaluated geometry is derived — CORE

Persistent files store authored engineering intent and semantic identity.

B-Rep, tessellation, projection output, solver output, caches, viewer objects and provider handles are derived/runtime state unless a specific domain contract explicitly says otherwise.

Deleting disposable derived data must not destroy authored design intent.

### 2.3 UI is never the model — CORE

Qt widgets, trees, toolbars, dialogs, Viewer objects and screen coordinates do not own durable CAD semantics.

The intended mutation path is:

```text
GUI / AI / Script
        ↓
Semantic Command
        ↓
Validation
        ↓
Transaction
        ↓
Owning Domain Document
        ↓
Evaluation / Recompute
        ↓
Presentation
```

### 2.4 Fail closed; never guess design intent — CORE

If identity, semantic reference, dependency or context cannot be resolved unambiguously, SS2 reports an explicit failure.

The system must not silently choose a nearby face, similar edge, first profile, arbitrary file or guessed replacement unless the operation contract explicitly defines a user-approved heuristic.

### 2.5 No mandatory global full-parametric CAD model — CORE

SS2 does not require every edit at every level to participate in one universal parametric dependency graph.

The design-intent emphasis is:

```text
Shared 2D Authoring  → local authored 2D intent
Part                  → local component / shape / manufacturing intent
Assembly              → high-level system relationships and placement intent
Drawing               → engineering documentation intent
```

Local parametric behavior is allowed where it has clear engineering value. It must not silently grow into an unrestricted project-wide dependency system.

---

## 3. Canonical product vocabulary

### 3.1 Project — CORE

A **Project** is the durable logical engineering project identity and project-level configuration.

It has a stable `ProjectId`.

A Project is **not** a CAD Document, runtime session, UI object or hidden file database.

### 3.2 Project Workspace — CORE

**Project Workspace** has exactly one architectural meaning:

> the physical filesystem root in which a Project lives.

Example:

```text
D:/Engineering/PackagingMachine/
```

Moving or renaming that directory changes location, not Project identity.

### 3.3 ProjectSession — CORE concept, DIRECTION name

An opened Project is represented at runtime by a **ProjectSession**.

It coordinates the current Project runtime context, resolution/discovery state and open DocumentSessions.

It disappears when the runtime session ends and is never the durable source of engineering truth.

The concept is frozen. The exact implementation type name may evolve deliberately.

### 3.4 Document — CORE

A Document is an independently persistent CAD semantic unit with its own stable `DocumentId`.

Foundation recognizes three top-level persistent CAD Document kinds:

```text
PartDocument
AssemblyDocument
DrawingDocument
```

Project itself is not a CAD Document.

Sketch is not a top-level Document.

Sheet Metal belongs to the Part domain and does not require a separate top-level Document kind.

### 3.5 DocumentSession — CORE

Opening a Document creates a runtime `DocumentSession`.

A DocumentSession may coordinate:

- loaded Document state;
- save/dirty checkpoint;
- Undo/Redo;
- active edit/transaction state;
- derived evaluation/cache handles;
- runtime diagnostics;
- transient selection and presentation binding.

It is not durable design intent.

### 3.6 Workbench — CORE

A Workbench is a runtime tool/presentation context.

It may contribute commands, panels, tools, selection modes and edit modes.

It does not own persistent CAD model state.

---

## 4. Project and filesystem foundation

### 4.1 Stable identity — CORE

`ProjectId` identifies a Project independently of path.

`DocumentId` identifies a persistent CAD Document independently of filename, path and display name.

```text
path       = where something is
stable ID  = which thing it is
```

### 4.2 Filesystem authority — CORE

The real Project Workspace filesystem is authoritative for physical project content.

All native SS2 documents physically inside the Workspace may be discovered from the filesystem and their embedded identities.

Indexes, catalogs, thumbnails and caches are derived and rebuildable.

Foundation does not require a durable hidden inventory database listing every Project file.

### 4.3 Minimal Project metadata — DIRECTION

The initial Project metadata should remain small:

```text
ProjectId
DisplayName
SchemaVersion
CreatedAt
```

Private SS2 infrastructure belongs under:

```text
.simplesolid/
```

Future real Project-level semantics may be added only when a concrete requirement exists.

### 4.4 Recent Projects — CORE

Recent Projects belongs to application/user state, not Project persistence.

Removing an entry from Recent Projects must not alter the Project.

Relocation or recovery must verify identity and fail closed when identity cannot be confirmed.

### 4.5 Project Hub — CORE

Project Hub is an application/navigation surface.

It may create, open, adopt and show recent Projects.

It is not a persistence authority and does not own CAD domain state.

---

## 5. Part foundation

### 5.1 Purpose — CORE

Part represents one physical component and its local component-level shape/manufacturing intent.

### 5.2 Part owns — CORE

Part owns:

- `PartDocument` durable semantic state;
- local component shape/body semantics;
- local construction geometry/datums where required;
- embedded Part-specific 2D sketch models;
- local modeling operations;
- local parameters where explicitly supported;
- Part-local semantic references and exposed engineering interfaces;
- Part persistence semantics.

### 5.3 Part does not own — CORE

Part does not own:

- Project discovery/membership;
- Assembly occurrence placement or inter-component constraints;
- Drawing sheets/views/annotations;
- application/session UI state;
- durable raw kernel B-Rep ordinals or provider handles;
- project-wide system parametric intent.

### 5.4 Direct/local modeling direction — CORE + DIRECTION

**CORE:** Part must not require a universal fully parametric feature/dependency graph in order to remain editable.

**DIRECTION:** SS2 may combine direct modeling, authored operations, limited replayable operations and local derived geometry where useful.

Foundation does not freeze:

- a specific feature-tree model;
- a universal operation-history model;
- replay of every edit from the first operation;
- one specific persistent naming algorithm.

---

## 6. Shared 2D Authoring foundation

### 6.1 Shared subsystem — CORE

Reusable 2D authoring is architecturally independent from Part and Drawing.

It provides domain-neutral 2D geometry and editing mechanics that may be consumed by multiple host domains.

The user-facing word **Sketcher** may remain appropriate in Part workflows, but the reusable architectural subsystem is not owned by Part.

### 6.2 Shared 2D Authoring owns — CORE

It may own:

- 2D entity geometry;
- stable model-local entity identity;
- line/arc/circle and similar primitive semantics;
- editing operations such as trim, extend, offset, mirror and transform;
- snapping and geometric inference;
- grips/selection/edit mechanics;
- intersections and geometric validation;
- optional local geometric constraints where supported.

### 6.3 Shared 2D Authoring does not own — CORE

It does not own:

- `PartDocument`;
- `DrawingDocument`;
- Assembly semantics;
- 3D B-Rep;
- host document lifecycle;
- Part profile meaning;
- Drawing dimension/annotation meaning;
- generated Drawing projection state;
- host-specific persistence policy.

### 6.4 Part integration — CORE

Part embeds/owns Part-specific 2D models while using Shared 2D Authoring mechanics.

Part owns the host-specific meaning:

- placement of that 2D model in 3D;
- support/reference semantics;
- interpretation of profiles/loops for Part operations;
- relation between authored 2D geometry and Part modeling operations.

### 6.5 Drawing integration — CORE

Drawing may embed authored custom 2D geometry using the same Shared 2D Authoring mechanics.

Drawing owns sheet/view placement, annotation semantics, Drawing persistence and documentation meaning.

---

## 7. Projection foundation

### 7.1 Shared Projection Engine — CORE

SS2 may provide a reusable geometric projection capability:

```text
evaluated source geometry
+ source coordinate context
+ target 2D plane/frame
        ↓
exact/materialized 2D geometry
```

The Projection Engine owns geometric conversion, not dependency policy.

### 7.2 Part-local Project Edge — DIRECTION

Projecting currently evaluated Part geometry into a Part 2D model should default to **capture/snapshot semantics**.

The accepted result becomes authored target 2D geometry.

Later source changes do not silently modify that captured geometry.

A future explicit `Reproject`/`Refresh` command may be introduced without making automatic associativity the default.

### 7.3 Assembly Projection Context — CORE

SS2 may open a transient **Assembly Projection Context** while editing a target Part 2D model.

This is deliberately **not full Part editing in Assembly context**.

Rules:

- Assembly and source occurrences are read-only projection context;
- only the target Part/2D model is mutated;
- source Part/Assembly Documents are not modified;
- source evaluated geometry is transformed through occurrence context;
- accepted projection becomes target-Part-owned authored 2D snapshot geometry;
- no cross-document parametric dependency is created;
- stale or ambiguous runtime context fails closed before capture;
- source topology/occurrence context is not automatic rebinding authority.

### 7.4 Projection provenance — DIRECTION

Captured projection may retain bounded informational provenance for diagnostics/audit.

Provenance does not itself create a dependency and must not silently rebind geometry.

### 7.5 Drawing is intentionally different — CORE

Projection dependency policy belongs to the host domain.

Therefore:

```text
Part local Project Edge          → authored snapshot
Assembly-context Project Edge    → authored snapshot
Drawing model View               → derived / regenerable
Drawing custom overlay           → authored / persistent
```

Sharing geometric projection code does not imply sharing dependency semantics.

---

## 8. Sheet Metal foundation

### 8.1 Domain position — CORE

Sheet Metal belongs to the Part domain.

This is a semantic specialization, not a requirement for a particular C++ inheritance structure.

### 8.2 Sheet Metal owns — CORE

Sheet Metal owns sheet-metal-specific authored intent such as:

- thickness;
- bends;
- flanges;
- relief/corner semantics;
- manufacturing rules;
- semantics needed to derive flat/developed representations.

### 8.3 Boundaries — CORE

Sheet Metal does not own:

- a separate Project model;
- a separate top-level CAD Document kind;
- a duplicate 2D authoring engine;
- Assembly relationships;
- Drawing documentation state;
- provider/kernel topology identity.

Flat/developed geometry is expected to be derived from authored Sheet Metal intent unless a later explicit ADR changes that rule.

Exact bend/unfold algorithms are deferred.

---

## 9. Assembly foundation

### 9.1 Purpose — CORE

Assembly represents occurrences of CAD Documents and the authored engineering intent between them at system level.

It is not merely a 3D scene containing copied Part geometry.

### 9.2 Definition identity versus occurrence identity — CORE

A referenced source Document and an occurrence of that Document are different concepts.

```text
DocumentId
    = which Part/Assembly definition

OccurrenceId
    = which occurrence inside one owning Assembly
```

Many Occurrences may reference the same `DocumentId`.

`OccurrenceId` is stable local identity scoped by its owning `AssemblyDocument`.

Rename/reorder preserves occurrence identity.

A newly inserted/copy-created occurrence receives new identity.

Deleted/rolled-back identity must not be silently reclaimed as the same semantic occurrence.

### 9.3 Assembly owns — CORE

Assembly owns:

- `AssemblyDocument` authored state;
- component `OccurrenceId`;
- semantic reference to the source `DocumentId`;
- authored local occurrence placement;
- grounding/fix intent;
- occurrence visibility/display metadata where persisted;
- Assembly-level relations/constraints when introduced;
- Assembly-level system intent and parameters when explicitly supported;
- recursive occurrence structure through referenced subassemblies.

Assembly does not copy source Part/Assembly authored models into itself as durable truth.

### 9.4 Recursive Assemblies — CORE

An Assembly occurrence may reference another Assembly.

Nested Assembly is a first-class model:

```text
Assembly
├── Part occurrence
├── Part occurrence
└── Assembly occurrence
    ├── Part occurrence
    └── Assembly occurrence
```

Document dependency cycles are invalid and fail closed.

### 9.5 OccurrencePath — CORE concept

Cross-level occurrence addressing is composed from stable occurrence identity through the hierarchy.

Conceptually:

```text
Root Assembly DocumentId
/ OccurrenceId
/ OccurrenceId
/ ...
/ semantic reference in referenced definition
```

Tree row, display name, Viewer instance index and filesystem path are never replacements for this semantic path.

Exact C++ representation is deferred.

### 9.6 Local frames and transform composition — CORE

Every referenced definition retains its own local coordinate frame.

An occurrence placement maps referenced-document local coordinates into the owning Assembly frame.

Nested world/root presentation is derived by transform composition through the occurrence path.

Absolute/root transforms are derived evaluation state and are not written back as child authored placement.

### 9.7 Authored placement versus evaluated placement — CORE

Occurrence authored placement is persistent design intent.

Solved/evaluated placement is derived state.

Without constraints, evaluated placement may equal authored placement.

A future solver may produce another evaluated pose, but evaluation must not overwrite authored placement merely because the solution changed.

### 9.8 Grounding — CORE

Grounding is explicit Assembly design intent.

It is not inferred from Tree order, insertion history or a fake Mate.

Changing grounding after evaluation should preserve the currently evaluated pose by capturing it as authored placement within the same logical mutation unless an explicit command says otherwise.

### 9.9 Semantic Assembly references — CORE

Future Assembly mates/constraints must target semantic engineering references scoped through occurrence identity/path.

Durable Assembly intent must not depend on:

- raw `TopoDS_*` handles;
- mesh/triangle indices;
- Viewer tokens;
- evaluation-local topology ordinals;
- `Face[n]` / `Edge[n]` numbering;
- Tree rows or display names.

The exact interface/port/reference family is intentionally deferred.

### 9.10 Solver boundary — CORE

Assembly authored state and the Assembly solver are different ownership layers.

Conceptually:

```text
Occurrences
+ semantic references
+ constraints
+ grounding
+ explicit parameters
        ↓
Assembly Solver / Evaluator
        ↓
evaluated placements
DOF / constraint status
structured diagnostics
```

The solver computes state.

It does not silently mutate Parts, delete constraints, rewrite design intent or execute arbitrary domain Commands to make a solution possible.

### 9.11 Direct/free placement remains valid — CORE

Assembly must permit useful direct placement without forcing every occurrence into a constraint system.

A component may remain freely/authored positioned.

SS2 does not require a fully constrained Assembly for ordinary work.

### 9.12 Missing dependencies and conflicts — CORE

A missing referenced Document does not delete an occurrence or authored Assembly intent.

Missing references, identity conflicts and cycles are explicit states.

Unaffected content remains usable/evaluable where deterministic isolation permits.

### 9.13 Relink versus Replace — CORE

`Relink/Locate` means locating the **same expected `DocumentId`** at a different location.

`Replace Component` means deliberately changing an occurrence to another source Document identity.

They are different semantic operations and must not be silently merged.

### 9.14 Assembly system parameters — DIRECTION / OPEN boundary

Assembly may own high-level/system parameters.

Foundation does **not** assume unrestricted Assembly-driven mutation of Part geometry.

Cross-document driving of explicitly exposed Part parameters may be considered later under a dedicated contract.

A project-wide arbitrary dependency graph is not implied.

### 9.15 SS1 inheritance — DIRECTION

The following SS1 concepts are strong donor architecture for SS2:

- `DocumentId` / `OccurrenceId` separation;
- occurrence authored placement;
- Grounding;
- recursive nested evaluation;
- occurrence path;
- identity-first reference resolution;
- missing/conflict/cycle fail-closed behavior;
- Command/Transaction mutation;
- semantic Undo/Redo test cases.

The historical M10 `CoincidentFrame` / `OffsetFrame` directed relation graph is **not** the foundation for the future general Assembly solver.

---

## 10. Drawing foundation

### 10.1 Purpose — CORE

Drawing is a first-class persistent CAD Document for engineering documentation derived from Part and Assembly sources.

It is not a rendering mode of Part or Assembly.

### 10.2 Multi-sheet and multi-source — CORE

One `DrawingDocument` may contain multiple stable Sheets.

A DrawingDocument has no mandatory single global source Document.

Each model-derived Drawing View owns its own semantic source reference.

One Sheet may contain views from one or several Part/Assembly Documents.

### 10.3 Sheet ownership — CORE

A Sheet is a durable paper/layout context with stable identity independent of display order.

A Sheet owns:

- paper/layout properties;
- ordered Drawing Views;
- Drawing-owned dimensions and annotations;
- custom authored 2D geometry/overlays;
- sheet-local layout semantics.

Sheet number/order is presentation order, not identity.

### 10.4 Drawing View ownership — CORE

A model-derived Drawing View owns durable view intent, including:

- stable Drawing View identity;
- source `DocumentId`;
- durable projection/view definition;
- placement on the Sheet;
- scale policy/reference where applicable.

A Drawing View must not persist Viewer camera objects, renderer identity, raw B-Rep handles or provider topology ordinals as its semantic identity.

The current 3D orientation may be command input for creating a view, but it must be converted into durable Drawing projection semantics before commit.

### 10.5 Generated versus authored 2D — CORE

Drawing distinguishes two ownership classes.

**Generated / derived model-view geometry:**

- produced from source Part/Assembly + Drawing View definition;
- rebuildable;
- not manually edited as authoritative model-view geometry.

**Authored Drawing geometry:**

- custom user-created 2D geometry;
- persistent Drawing-owned state;
- edited using Shared 2D Authoring.

Generated geometry and authored overlays must not become one ambiguous source of truth.

### 10.6 Dimensions and annotations — CORE

Drawing owns engineering documentation meaning, including future:

- dimensions;
- notes;
- symbols;
- leaders;
- tolerances/GD&T-related semantics where implemented.

Shared 2D Authoring may provide geometric/editor mechanics, but it does not own Drawing annotation meaning or reference semantics.

A model-referencing dimension must eventually use semantic Drawing/View references, not projected line array indices, renderer identity or raw provider topology numbering.

If such a reference can no longer resolve, it becomes explicitly unresolved/broken rather than being guessed.

### 10.7 Drawing associativity — CORE

Model-derived Drawing Views are expected to update/rebuild from their semantic Part/Assembly source.

This is a Drawing-domain dependency and is intentionally different from Part/Assembly Project Edge capture.

Missing/broken source dependencies preserve Drawing intent and degrade locally.

Unrelated Sheets/Views remain usable where deterministic isolation permits.

### 10.8 Scale model — DIRECTION

Projected model geometry remains in true model dimensions.

Paper/layout mapping carries conventional Drawing scale.

Each Sheet may have a base scale.

Local detail/section/auxiliary scale overrides are deferred.

### 10.9 Primary View and metadata context — DIRECTION

A Sheet may have an explicit Primary View used as the default Part/Assembly metadata source for ordinary title-block fields.

Project-, Drawing- and Sheet-owned metadata remain separate scopes.

Exact successor/override policy is deferred.

### 10.10 SS1 inheritance — DIRECTION

Carry forward these SS1 Drawing architecture decisions:

- multi-sheet `DrawingDocument`;
- stable Sheet identity separate from order;
- multi-source Views;
- per-View semantic source `DocumentId`;
- current 3D orientation as command input only;
- per-Sheet layout/scale direction;
- local degradation when a source is missing;
- Primary View as a useful metadata context.

SS1 provides architecture direction here, not mature Drawing implementation code.

---

## 11. Shared Platform foundation

### 11.1 Platform rule — CORE

> **Domain owns meaning. Platform owns reusable mechanism. UI owns neither.**

A capability belongs in Shared Platform only when it is intrinsically platform-level or is genuinely reused across domains.

Do not create speculative universal frameworks merely because a future feature might need them.

### 11.2 Dependency direction — CORE

The intended dependency direction is:

```text
Application / UI / Automation
        ↓
Application Services / Commands
        ↓
CAD Domains
Part / Assembly / Drawing
        ↓
Shared semantic/platform contracts
        ↓
Geometry / Kernel API / persistence primitives
        ↓
Provider / renderer / OS / toolkit adapters
```

Lower layers must not depend upward on CAD domains or presentation/UI.

### 11.3 Core/Foundation primitives — CORE

Core owns only domain-neutral primitives/contracts such as:

- `DocumentId` and generic object identity primitives;
- revisions / dirty-state primitives;
- units and physical-value validation;
- neutral 2D/3D geometry/spatial primitives;
- coordinate frames and rigid transforms;
- serialization/version contract primitives;
- generic typed diagnostic primitives where useful.

Core must not own Part, Assembly, Drawing, Qt or OCCT semantics.

### 11.4 Commands / Transactions / Undo-Redo — CORE

Persistent semantic mutations use shared application mutation infrastructure.

Commands represent semantic actions, not widget callbacks.

Execution revalidates current authoritative context rather than trusting prior UI enablement or stale preview.

Transactions provide atomic commit/rollback.

Undo/Redo restores semantic authored state, not snapshots of Viewer/UI state.

Cross-document commands receive an explicit execution/Command context instead of creating a second mutation architecture.

### 11.5 Persistence infrastructure — CORE

Shared persistence infrastructure may own:

- format/version recognition;
- safe/atomic file replacement mechanics;
- common serialization helpers;
- migration dispatch/contracts;
- normalized filesystem errors where useful.

Each CAD domain owns its durable semantic schema.

Provider/runtime state must not leak into native file semantics.

### 11.6 Units — CORE

Units are shared platform semantics.

Physical values crossing subsystem boundaries have explicit meaning and validation.

### 11.7 Selection — CORE

Selection is layered:

```text
input/pick
    ↓
transient Viewer / 2D interaction token
    ↓
application semantic binding
    ↓
domain semantic selection/reference
```

Viewer tokens, tree rows, Qt objects, mesh indices and evaluated topology ordinals are not durable semantic identity.

### 11.8 Semantic reference architecture — CORE

Durable references express engineering intent in the owning domain.

Shared Platform may provide common reference infrastructure/typing but must not flatten all Part/Assembly/Drawing reference meaning into one universal untyped reference.

Provider-native topology identity remains transient/evaluated.

Resolution is conservative and fail-closed.

### 11.9 Shared Viewer — CORE

SS2 has one shared 3D Viewer mechanism for all 3D CAD domains.

Viewer owns:

- renderer-neutral scene representation;
- reusable scene assets/instances;
- camera/navigation;
- display mechanics;
- transient picking transport;
- common highlight/overlay mechanics.

Application presentation/binding adapts domain semantics to Viewer mechanisms.

CAD domains retain semantic ownership.

Do not create separate PartViewer and AssemblyViewer platform stacks.

### 11.10 Kernel API — CORE

Kernel API is provider-neutral.

CAD domains, persistence and semantic references must not depend on OCCT types, handles, topology numbering or provider-native identity.

OCCT is an implementation provider behind this boundary.

Kernel results are evaluated geometry, not persistent design intent.

### 11.11 Import / Export — CORE

Import/Export is a platform service.

STEP, DXF, SVG and other format adapters do not become CAD domains.

Imported provider identity must not leak into durable SS2 identity.

### 11.12 Recompute / evaluation — CORE

Evaluation is derived, deterministic and disposable.

Each CAD domain owns its evaluation semantics.

Shared infrastructure may later provide scheduling/cache services when real cross-domain requirements justify them.

Foundation explicitly does **not** require one universal global dependency/recompute graph.

### 11.13 Diagnostics — CORE

Failures originate as structured diagnostics at the subsystem that understands them.

UI text is presentation, not model semantics.

---

## 12. Automation and AI foundation

### 12.1 Common semantic operation layer — CORE

GUI, scripting and AI ultimately use the same semantic Commands and domain capabilities.

AI must not require privileged bypass paths around validation/transactions.

### 12.2 AI must not depend on presentation identity — CORE

Automation/AI must not require as its semantic API:

- Qt widgets;
- screen coordinates;
- Tree row identity;
- renderer tokens;
- raw OCCT handles;
- durable `Face[n]` / `Edge[n]` topology numbering.

### 12.3 Query/inspection direction — DIRECTION

The platform should later expose discoverable semantic capabilities including:

- query/inspection APIs;
- command schemas/capabilities;
- explicit preview/transaction boundaries;
- structured diagnostics;
- semantic references suitable for human and machine clients.

Exact reflection/schema protocol is deferred.

---

## 13. Technology baseline

These are the **SS2 v1 implementation baseline**, not eternal product invariants.

### 13.1 Language — DIRECTION

```text
C++20
```

Later Python may be used for automation, scripting, AI/client integration and experimentation over stable semantic interfaces.

Python does not become a parallel CAD model.

### 13.2 Build — DIRECTION

```text
CMake
```

Exact compiler/IDE version belongs to toolchain/CI configuration rather than Foundation.

### 13.3 Desktop UI — DIRECTION

```text
Qt 6 Widgets
```

SS2 v1 is a desktop engineering application. Widgets are the baseline for docks, trees, property editing, commands, dialogs, tabs and viewport hosting.

This does not declare Qt Widgets immutable forever.

### 13.4 Geometry kernel — CORE boundary / DIRECTION provider

Architecture:

```text
CAD Domain
    ↓
Kernel API
    ↓
OCCT Provider
```

Provider-neutral boundary is CORE.

OCCT as the v1 implementation provider is DIRECTION.

### 13.5 Product platform — DIRECTION

SS2 v1 is **Windows-first**, not architecturally Windows-only.

Domain and core code should avoid unnecessary WinAPI coupling.

### 13.6 Repository — DIRECTION

GitHub is the primary repository/collaboration platform for SS2.

---

## 14. Minimal governance bootstrap

### 14.1 Authority principle — CORE governance

```text
Owner decides WHAT and WHY.
Agent decides HOW inside an approved contract.
Repository / CI verifies what can be verified mechanically.
```

The Agent cannot grant itself architectural authority.

Ambiguous execution authority fails closed.

### 14.2 Decision classes — CORE governance

**D0 — Implementation**
- private implementation detail;
- Agent autonomous inside current contract.

**D1 — Local design**
- local internal design choice;
- Agent autonomous if no higher contract/architecture rule changes.

**D2 — Architecture**
Examples:
- public API;
- subsystem ownership;
- dependency direction;
- persistence semantics;
- durable identity/reference changes.

Agent may propose. Owner approval is required.

**D3 — Product / irreversible architecture**
Examples:
- redefining product domain responsibilities;
- replacing direct/local Part philosophy with universal full parametrics;
- changing the Project/Document model;
- major product-scope direction.

Owner decides.

### 14.3 Minimal authority hierarchy — DIRECTION

Initial repository authority should remain small:

```text
CONSTITUTION
    ↓
FOUNDATION / ARCHITECTURE
    ↓
ADR
    ↓
WORK CONTRACT
    ↓
CODE + TESTS
```

Avoid recreating a governance product.

### 14.4 Work contract — DIRECTION

A work contract should define at minimum:

```text
GOAL
IN SCOPE
OUT OF SCOPE
ARCHITECTURE IMPACT
PUBLIC CONTRACT
FAILURE BEHAVIOR
ACCEPTANCE
```

Frozen scope means no silent reinterpretation.

If a contract is wrong, stop and amend it rather than quietly expanding implementation.

### 14.5 Verification — CORE governance

Evidence should be tied to the implementation revision being accepted.

Tests should prove semantic boundaries, persistence/lifecycle behavior and failure behavior — not merely that a picture appeared.

Agents may not weaken/delete tests simply to obtain a pass.

---

## 15. SS1 migration policy

### 15.1 SS1 is a donor repository — CORE migration rule

SS1 is a source of proven concepts, tests and selected implementation.

SS2 Foundation is the authority.

SS1 code must not silently import historical architecture into SS2.

### 15.2 Migration order — DIRECTION

Prefer:

```text
SS2 contract
    ↓
preserved invariant / test
    ↓
audit SS1 donor code
    ↓
direct reuse OR port-with-cleanup OR concept-only reuse
    ↓
SS2 verification
```

Do not start with `copy subsystem → clean later`.

### 15.3 Strong donor candidates

Likely direct/clean reuse candidates include:

- stable identity primitives;
- revision/dirty primitives;
- units;
- neutral spatial transforms;
- selected filesystem/atomic persistence helpers;
- Kernel API provider boundary;
- Viewer scene asset/instance concepts;
- identity-first workspace resolution;
- semantic test cases.

Likely port-with-cleanup candidates include:

- Project creation/adoption metadata logic;
- Recent Projects;
- workspace discovery/resolution;
- DocumentSession lifecycle;
- Commands/Undo patterns;
- Assembly occurrence/persistence foundation;
- recursive Assembly evaluation;
- Viewer scene adaptation.

Concept-only / redesign candidates include:

- historical M10 Assembly relation graph as a general solver;
- legacy locator compatibility;
- old compatibility aliases;
- assumptions tied to full parametric Part history;
- monolithic `SimpleSolid_Next` ownership.

---

## 16. First implementation milestone

### MAIN v0.1 — Testable Project Hub — DIRECTION

The first useful SS2 main proves:

```text
Application start
    ↓
Project Hub
    ↓
Create Project OR Open Project
    ↓
validate Project identity/metadata
    ↓
open ProjectSession / empty Workspace context
    ↓
close
    ↓
restart
    ↓
Recent Projects
    ↓
reopen same ProjectId
```

This milestone proves project lifecycle and architecture, not CAD geometry.

Expected scope includes:

- Project Hub;
- create/open/adopt project workflow as contracted;
- stable ProjectId;
- minimal project metadata;
- Recent Projects;
- real filesystem Workspace;
- ProjectSession;
- empty Workspace/Application shell transition;
- invalid/missing/identity-conflict diagnostics;
- save/close/reopen lifecycle tests.

Explicitly outside the first milestone:

- Part modeling;
- Sheet Metal;
- Assembly editing;
- Drawing;
- OCCT modeling;
- 3D modeling tools;
- Assembly solver;
- advanced parameters;
- plugins;
- full AI CAD operation surface.

---

## 17. Deliberately open / deferred

The following are intentionally **not required before repository Genesis**:

### Part
- exact direct-model representation;
- exact operation/history structure;
- persistent naming algorithm;
- detailed local parameter system.

### Shared 2D
- final subsystem/code namespace name;
- exact constraint vocabulary/solver behavior;
- detailed tool list.

### Sheet Metal
- bend/flange/unfold algorithms;
- manufacturing database/policies.

### Assembly
- exact Mate/constraint vocabulary;
- semantic port/interface representation;
- numerical solver;
- DOF model;
- constrained drag/kinematics;
- configurations;
- cross-document driving of explicitly exposed Part parameters.

### Drawing
- exact `.ssdraw` schema;
- SheetId/DrawingViewId representation;
- hidden-line algorithm;
- section/detail/auxiliary view contracts;
- exact dimension attachment/reference model;
- GD&T scope;
- title-block/template implementation;
- local view scale rules;
- PDF/DXF/SVG publishing;
- large-document lazy evaluation strategy.

### Platform
- plugin ABI;
- advanced parameter/expression engine;
- BOM/material database;
- universal cross-document scheduler;
- generalized presentation-state framework;
- exact AI schema/reflection protocol.

These items require later explicit domain/work contracts when implementation evidence and product need justify them.

---

## 18. Foundation invariants summary

The following statements are intended to survive implementation detail changes:

1. Project identity is independent of filesystem path.
2. Project Workspace means the filesystem location only.
3. Project is not a CAD Document.
4. Persistent top-level CAD Documents are Part, Assembly and Drawing.
5. Durable Document identity is independent of path/name.
6. Runtime sessions are not durable design intent.
7. Workbench/UI/Viewer do not own CAD semantics.
8. Part does not require a universal full-parametric graph.
9. Shared 2D Authoring is reusable across Part and Drawing.
10. Part-local and Assembly-context Project Edge capture are non-associative authored snapshots by default.
11. Drawing model Views are derived/regenerable and therefore intentionally associative to their semantic sources.
12. Sheet Metal belongs to Part.
13. Assembly distinguishes Document definition from Occurrence identity.
14. Assembly authored placement is distinct from evaluated/solved placement.
15. Nested Assembly and semantic occurrence paths are first-class.
16. Future Assembly constraints use semantic references, not durable raw topology IDs.
17. Missing/ambiguous dependencies fail closed without deleting authored intent.
18. Domain owns meaning; Shared Platform owns reusable mechanism.
19. Kernel API is provider-neutral; OCCT identity never becomes CAD identity.
20. GUI, scripts and AI use the same semantic mutation layer.
21. SS1 is a donor; SS2 Foundation is the authority.
22. Foundation does not require one universal project-wide dependency graph.

---

## 19. Freeze status

Foundation v1.0 resolves the prior working-ledger questions for:

- Project / Workspace / Session terminology;
- Document model;
- Part / Shared 2D / Sheet Metal ownership;
- Part and Assembly projection policy;
- Assembly foundation;
- Drawing foundation;
- Shared Platform ownership;
- technology baseline;
- minimal governance.

No remaining OPEN item is considered a blocker for creating the SS2 repository or beginning the Project Hub milestone.

This Foundation is **FROZEN v1.0**. Future changes to CORE rules require an explicit Owner-approved Foundation Amendment.
