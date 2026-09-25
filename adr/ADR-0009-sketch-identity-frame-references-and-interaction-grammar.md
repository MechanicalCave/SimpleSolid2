# ADR-0009 — Sketch identity lifecycle, local frame, references and interaction grammar

**Status:** ACCEPTED  
**Date:** 2026-09-25  
**Owner acceptance:** 2026-09-25  
**Decision class:** D2 Architecture  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Related:** ADR-0005, ADR-0006, ADR-0008

## Context

ADR-0008 freezes the Shared 2D / Sketcher semantic foundation before the first authored 2D entity is implemented.

Before accepting the first `Line` contract, several cross-cutting semantics must be made explicit so later Viewer/UI, constraints, support-face Sketches, projection and editing operations cannot silently redefine entity identity, local coordinates or the default interaction model.

The decisions below refine existing Foundation rules. They do not authorize implementation outside an accepted Work Contract.

## Decision

### 1. Sketch-local coordinates are physical U/V coordinates

Shared 2D authored geometry lives in Sketch-local coordinates:

```text
(U,V)
```

They represent physical model-length coordinates, not screen/pixel coordinates.

A host maps Sketch-local coordinates into its spatial context through an explicit local frame. Conceptually:

```text
P3D = Origin3D + U * UAxis3D + V * VAxis3D
```

Shared 2D geometry does not know whether the host frame came from Part XY/XZ/YZ, a future Datum Plane, a planar model face or a Drawing page context.

Unit display/formatting policy is separate from authored coordinates.

### 2. Host Sketch frame is metric, stable and orientation-preserving

A 3D host frame used for Sketch placement must represent an orthonormal metric U/V basis rather than encode arbitrary scale or skew.

For current Part Origin-plane supports, local Sketch `(0,0)` maps to Part Origin.

Future support-frame derivation must not depend on unstable implementation details such as:

- kernel edge/vertex iteration order;
- Tree row or Viewer token;
- bounding-box center chosen as semantic identity;
- first returned edge/vertex;
- provider-native topology ordinal.

Recompute must not silently flip/mirror the established local U/V orientation merely because provider topology orientation/order changed.

Exact stable frame derivation for future planar-face support remains an explicit later decision.

### 3. Every Sketch has an intrinsic Origin reference

Sketch local `(0,0)` is an intrinsic semantic reference.

The Origin is:

- immutable;
- always conceptually available while editing/querying the Sketch;
- later selectable/snappable/inspectable as interaction contracts require;
- eligible for explicit future relations;
- not ordinary user-authored geometry;
- not deletable or movable;
- not a material/profile boundary merely because it exists.

A Sketch containing only intrinsic/reference geometry remains semantically empty with respect to user-authored local geometry.

This decision does not introduce a standalone authored Point entity and does not require Origin to consume an `EntityId` in the first Shared 2D model implementation.

### 4. Entity identity is stable and must not alias another semantic entity

An authored entity has stable model-local `EntityId`.

Within one continuing Sketch identity lineage, an `EntityId` must not silently come to denote a different semantic entity merely because the original entity was deleted, reordered or storage was compacted.

Required semantic distinction:

```text
state copy / Undo state replication
→ preserves existing entity identity

semantic duplication / future Copy-Paste
→ creates fresh entity identity
```

Undo of deletion restores the original entity identity.

Exact allocation representation, persistence encoding and the mechanism used to guarantee non-aliasing across Undo/Redo/save-reopen are deferred to the contracts that introduce those lifecycle boundaries.

### 5. Durable geometry references use identity plus semantic element role

Future durable references to entity parts must address semantic roles, conceptually:

```text
SketchId
EntityId
ElementRole
```

For a Line, roles may conceptually include Start and End.

Coordinates, vector indices, object addresses, Viewer tokens and provider topology objects are not substitutes for semantic element identity.

ADR-0009 does not freeze a universal `SubElement` C++ type.

### 6. Authored geometry and evaluated geometry are distinct layers

Authored geometry is persistent design intent.

Evaluated geometry is the current usable geometric result after any applicable evaluation/constraint solving.

Initially the mapping may be identity:

```text
Authored Sketch
      ↓ identity evaluation
Evaluated Geometry
```

Later:

```text
Authored geometry
+ authored relations
      ↓
local evaluator / solver
      ↓
Evaluated Geometry
```

Presentation, measurement, snapping, intersections and region analysis should consume evaluated geometry where evaluation exists rather than assume authored storage is always the final geometric answer.

Evaluation never silently overwrites authored intent merely because a derived solution exists.

### 7. Deletion and topology-changing edits must preserve referential integrity

Deleting geometry must not leave a committed authored model containing silently dangling semantic references.

A later contract that introduces constraints/references must explicitly choose and test the legal behavior for deletion, such as rejection, explicit dependent deletion or another user-visible semantic action.

Topology-changing operations such as Trim/Split must explicitly define identity outcomes before implementation.

Move/coordinate edits may preserve one entity identity; this does not imply that every topological rewrite may arbitrarily reuse that identity.

### 8. Select is the default Sketch interaction tool

While a Sketch edit context is active, the interaction state does not fall back to an undefined `no tool` mode.

The default tool is conceptually:

```text
Select
```

Entering Sketch edit with no other requested tool activates Select.

Finishing/cancelling a creation/editing tool returns to Select after its local pending stage has been resolved/cancelled.

`Esc` semantics are hierarchical:

```text
cancel current pending stage/input when one exists
        ↓
leave the active non-Select tool
        ↓
Select
```

Exact multi-Esc timing and key aliases remain later UX details, but Esc must not leave Sketch editing in an undefined tool state.

Viewport camera navigation remains available independently from Select/Create/Edit tool state.

### 9. Selection is semantic runtime state

Sketch selection is transient runtime state and is not persisted CAD intent.

The interaction model must support:

- entity-level point picking;
- rectangular/box selection;
- zero-to-many selected entities;
- an optional primary selected entity where useful;
- clear selection by ordinary empty-space interaction according to the accepted UI contract;
- selection transport to stable semantic identity rather than provider tokens.

A click on a Line selects the Line entity, not an implicit persistent endpoint/midpoint object.

Grips/sub-element editing are derived runtime interaction handles layered on top of semantic entity selection.

Exact Ctrl/Shift/window-vs-crossing gesture rules remain deferred.

### 10. User-facing geometry must not become interaction-dead

When an authored primitive reaches a user-facing interactive milestone, the minimum usable lifecycle is:

```text
create
→ present
→ select
→ delete
```

A primitive should not be considered interaction-complete merely because it can be drawn but cannot subsequently be selected or removed through the ordinary semantic command path.

Advanced grips/direct manipulation, constraints and diagnostics may remain later milestones.

### 11. Cursor presentation communicates active interaction mode

Inside the 3D Sketch viewport:

- Select uses a small pick-box/square style cursor that visually communicates a point-pick aperture;
- creation/edit/measurement tools use a small crosshair/target style cursor unless a later tool-specific contract justifies another cursor;
- moving onto ordinary Qt/UI controls returns to the standard system/widget cursor.

Cursor appearance is runtime presentation only.

The visual pick box, actual screen-space pick tolerance, geometric tolerance and snap tolerance are distinct values and must not be conflated.

Snap/inference glyphs are separate presentation overlays rather than being encoded into cursor identity.

Exact pixel dimensions, HiDPI scaling, colors and line thickness remain deferred.

### 12. Pointer input maps to Sketch U/V independently of camera orientation

Part-hosted Sketch editing remains in the common 3D viewport and the user may orbit away from a normal-on-plane view.

Therefore pointer-to-Sketch input must not assume screen X/Y equals Sketch U/V.

The provider-neutral interaction boundary must support deriving a spatial pointer/ray and intersecting it with the active Sketch plane/frame (or equivalent mathematically correct mapping) to obtain local U/V.

Failure to resolve a valid plane intersection fails cleanly; it is not guessed from stale screen coordinates.

### 13. Reference/projected geometry is semantically distinct from local editable geometry

Future projected/reference geometry must be distinguishable semantically from ordinary editable authored geometry.

Reference/projected geometry may later be:

- visible;
- selectable/inspectable;
- snappable;
- usable as explicit relation input;

while remaining read-only according to its reference policy.

It must not be represented merely as an ordinary editable Line/Arc/Circle with a different color if that would erase the semantic distinction.

Exact storage representation remains deferred.

### 14. Projection follows host-domain policy

Foundation remains authoritative:

- Part-local Project Edge defaults to authored capture/snapshot semantics;
- Assembly-context capture defaults to target-Part-owned snapshot semantics without cross-document parametric dependency;
- Drawing model-view projection is intentionally derived/regenerable.

SS1 projection code is a donor of mechanisms/tests only and does not override these SS2 semantics.

For a future Sketch created on a planar model face, automatically presenting/capturing the support-face boundary is accepted product direction.

However the following remain explicitly unresolved until a dedicated contract:

- semantic planar-face support identity/rebinding;
- stable face-derived Sketch frame algorithm;
- whether automatic support-boundary references are snapshot or support-associative;
- whether projected/reference curves participate in region/profile boundaries;
- refresh/reproject/break-link UX.

No raw `TopoDS_Face`, face ordinal or Viewer token may become durable support/reference identity.

### 15. Exact projection geometry fails closed

Future geometric projection should prefer provider-neutral exact/materialized 2D curves where the source curve kind is representable.

Unsupported or degenerate exact projection produces a structured failure/diagnostic rather than silently substituting a tessellated/polyline approximation as authoritative CAD intent unless an explicit later contract allows such approximation.

## Consequences

The first Shared 2D core may remain very small while preserving the seams needed for selection, Origin snap, constraints, face-supported Sketches and projected references later.

The initial `Line` implementation does not need to implement Origin, Viewer selection, projection or solver behavior.

The Viewer/tool boundary must eventually support real plane-based coordinate resolution and runtime cursor/picking presentation without leaking provider identity into CAD semantics.

Selection and deletion become part of the minimum user-facing lifecycle for future interactive primitives, while grips/direct manipulation remain separable.

SS1 may donate tests and projection mechanisms after compatibility review, but SS2 Foundation/ADRs remain authoritative.

## Explicitly deferred

ADR-0009 does not freeze:

- exact `EntityId` representation/allocation algorithm;
- exact persistence mechanism for allocator/non-reuse state;
- universal public sub-element/reference C++ type;
- exact Origin storage representation;
- exact selection container/API;
- Ctrl/Shift/window/crossing selection gestures;
- cursor pixel sizes/colors;
- exact Viewer pointer/ray API;
- exact planar-face semantic reference implementation;
- exact planar-face frame derivation algorithm;
- automatic support-boundary projection associativity;
- projected-reference participation in profile/region construction;
- refresh/reproject/break-link workflow;
- constraint vocabulary/solver;
- exact Trim/Split identity policy.
