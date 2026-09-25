# ADR-0008 — Shared 2D Authoring and Sketcher semantic foundation

**Status:** ACCEPTED  
**Date:** 2026-09-25  
**Owner acceptance:** 2026-09-25  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** ADR-0003, ADR-0005, ADR-0006, ADR-0007

## Context

SK-01 established a durable Part-hosted Sketch identity, support, placement and edit lifecycle without 2D entities. SK-01A established the editor toolbar and contextual Operations surface. WS-01 separated Project Workspace navigation from the one-active-Document Part Workbench.

Before introducing the first 2D entity, SS2 needs a stable semantic foundation that prevents interactive CAD UX, Qt/OCCT presentation details, persistence concerns or future solver behavior from accidentally defining the reusable Sketch Core.

The intended user experience takes useful interaction grammar from classical CAD systems — continuous commands, object snap, inference, keyboard input, command prompts and grips — while retaining SS2's semantic authored-model architecture.

## Decision

### 1. Shared 2D owns reusable authored 2D meaning

Shared 2D / Sketch Core remains independent from PartDocument, AssemblyDocument, DrawingDocument, Qt, OCCT and filesystem paths.

It owns reusable authored 2D entity meaning and may later own optional local constraints, local evaluation/solver integration, intersections, editing mechanics and geometric diagnostics as required by accepted contracts.

Hosts own host-specific placement, support/reference meaning, persistence policy, lifecycle integration and domain interpretation.

### 2. Authored geometry is directly editable design intent

Primitive 2D entity geometry is authored state.

For a Line, start and end coordinates are authored geometry. Two endpoint sub-elements may have exactly equal coordinates while remaining independent authored parts of their respective entities.

Equal coordinates do not create shared authored identity and do not by themselves create a persistent relation.

A later explicit relation such as Coincident may constrain independent endpoint sub-elements to remain coincident during legal evaluation/editing.

This decision does not introduce a standalone Point entity.

### 3. Constraints are optional authored relations

A Sketch remains useful without constraints or driving dimensions.

Optional geometric constraints, when introduced, represent persistent geometric intent layered over authored entities. They do not become a mandatory universal parametric graph.

The exact constraint vocabulary, solving algorithm, degree-of-freedom model and dimension semantics remain deferred.

### 4. Runtime inference is not authored intent

The following are runtime interaction/evaluation concepts unless an explicit accepted command converts the user's action into authored intent:

```text
snap candidate
inference candidate
hover
preview
grip
cursor location
dynamic input presentation
Command Line text
Operations widgets
```

Therefore:

```text
snap ≠ constraint
inference ≠ constraint
numeric input ≠ driving dimension
grip ≠ authored entity
```

A tool may use any of these to resolve command input. Persistent consequences occur only through semantic command/transaction execution.

### 5. One active tool state, multiple input adapters

An interactive Sketch tool has one runtime semantic tool state.

Mouse/pointer interaction, keyboard/Command Line input, dynamic input and the Operations surface are adapters to that same state. They must not become independent tool implementations or competing model authorities.

Continuous commands are supported in principle. For example, a future Line tool may accept first point, successive points and explicit finish/cancel without requiring the user to relaunch Line for every segment.

Exact key bindings, command aliases and coordinate-entry syntax remain UX details for later contracts.

### 6. Measurement and diagnostics are read-only by default

Sketch inspection must support exact geometric questions independently from authored dimensions.

Examples include length, angle, point coordinates, endpoint distance/gap, horizontal/vertical deviation, intersection state and profile/open-chain diagnostics.

Read-only measurement/diagnostic queries:

- do not dirty the Document;
- do not create Undo entries;
- do not silently repair geometry;
- do not create constraints/dimensions merely because a condition is observed.

Any repair or authored relation requires a separate explicit semantic command.

### 7. Tolerances have separate meanings

Screen/pixel tolerance, snap tolerance, geometric/intersection tolerance, region-analysis tolerance and future solver tolerance are distinct concerns.

A screen-space appearance must not become authority for whether authored/evaluated geometry is coincident or whether a planar region is closed.

Exact numerical policies remain deferred until their first concrete consumers are contracted.

### 8. Planar regions/profiles are derived from evaluated geometry

Closed planar regions are derived results of current evaluated Sketch geometry. They are not authored Sketch entities merely because a user can select or diagnose them.

A future planar region evaluator may derive intersections, loops, containment, bounded regions, open chains and structured diagnostics without mutating authored geometry.

A Sketch-level `Find Profiles`-style tool may expose those derived results diagnostically.

Part may reuse the same derived region capability when a Part operation such as a future Extrude needs the user to select one or more usable planar regions.

Shared 2D owns geometric region analysis. Part owns the modeling meaning of consuming those regions.

Durable profile identity and associative rebinding policy for future Part features remain deferred.

### 9. Construction and editing roles remain semantic, presentation remains derived

Future construction geometry may participate in selection, snapping, inference and constraints while being excluded from material-region construction according to an explicit authored semantic role.

Interaction handles such as Line start/end/center grips remain runtime handles. A center grip may translate a Line without implying a durable midpoint entity.

Exact role and sub-element APIs remain deferred.

### 10. Viewer and input boundaries remain provider-neutral

Shared/domain semantic state must not depend on OCCT objects, provider handles or Viewer presentation tokens.

Future Sketch presentation, preview, pointer-to-Sketch-coordinate input and selection transport must cross provider-neutral contracts.

The current Viewer API does not yet define these contracts; they are intentionally left to a later roadmap milestone instead of being invented inside the first Line entity implementation.

### 11. Sketch edit UX direction

Part-hosted Sketch editing remains in the same common 3D Document Viewport.

The accepted UX direction is:

```text
Part edit context
→ Part editor tools above the Viewport

Sketch edit context
→ Sketch editor tools above the same Viewport

Command prompt/input
→ compact Command Line concept associated with the editor surface

Operations
→ contextual active-tool state, structured inputs/actions and diagnostics
```

Command Line, dynamic input and Operations must reflect one tool state.

Exact widgets, grouping, icons, shortcuts, RMB menus and final visual layout remain deferred.

## Consequences

The first Shared 2D implementation can remain small and UI-independent.

Interactive Line implementation must not force Viewer/OCCT identity into the authored model.

Direct geometry editing remains useful without constraints, while optional relations can later preserve engineering intent where requested.

Measurement and profile analysis can become powerful engineering diagnostics without turning observations into automatic mutations.

Part modeling can consume derived Sketch regions later without making Profile an authored Sketch object by default.

## Explicitly deferred

This ADR does not freeze or implement:

- exact `EntityId` serialization or allocation API;
- exact Sketch model/container C++ API;
- a universal `SubElement` type;
- standalone authored Point entities;
- persistence schema for 2D entities;
- concrete Viewer Sketch-scene API;
- concrete pointer/input event API;
- constraint vocabulary or solver;
- driving/reference dimensions;
- auto-constraint policy;
- numerical tolerance values;
- coordinate-entry syntax;
- exact Command Line key grammar;
- Arc/Circle/Trim/Extend/Offset behavior;
- durable derived-region identity;
- associative versus snapshot consumption by future Part features;
- Body/Feature/Extrude architecture.
