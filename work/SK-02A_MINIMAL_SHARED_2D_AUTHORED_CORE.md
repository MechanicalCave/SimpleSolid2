# SK-02A — Minimal Shared 2D Authored Core

**Status:** PROPOSED  
**Owner acceptance:** pending  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.0  
**Roadmap milestone:** R1 — Minimal Shared 2D authored core

## 1. Goal

Implement the smallest real host-neutral Shared 2D authored model needed to prove one persistent semantic primitive: `Line`.

The contract deliberately stops before Part integration, persistence and interactive Viewer/UI work.

The resulting dependency direction must remain:

```text
future Part / Drawing / other host
            ↓
      Shared 2D / Sketch Core
```

and never the reverse.

## 2. Semantic slice

R1-A proves only:

```text
Sketch model
  └─ authored Line entity
       ├─ stable model-local EntityId
       ├─ authored Start (U,V)
       └─ authored End   (U,V)
```

A Line is design intent, not presentation geometry.

Its Start and End coordinates are directly authored 2D values.

Two endpoints from different entities may occupy exactly the same coordinates while remaining completely independent authored data. Equal coordinates do not create shared Point identity or a persistent relation.

No standalone authored Point entity is introduced.

## 3. Scope IN

### 3.1 Opaque stable entity identity

Introduce the minimum Shared 2D entity identity needed by the model.

Required semantics:

- every authored entity owned by one Sketch model has a stable identity;
- identities are unique within that model;
- identity is independent from collection index, insertion address, pointer value, display name and Viewer token;
- ordinary value-copy of authored model state preserves entity identities;
- creating another entity does not change existing identities.

R1-A does **not** freeze:

- external/global uniqueness;
- textual serialization;
- persistence representation;
- delete/reuse policy;
- a universal cross-domain reference type.

The concrete C++ representation remains an implementation detail unless a later accepted contract requires more.

### 3.2 Finite 2D coordinate semantics

Introduce the smallest neutral 2D coordinate/value representation required by Line geometry.

Required semantics:

- coordinates are in Sketch-local U/V space;
- accepted authored coordinates are finite real values;
- NaN and ±Infinity fail closed before authored model mutation;
- no Qt, OCCT, screen/pixel or host placement types cross this boundary.

No unit-display, screen transform, snap or tolerance framework is introduced.

### 3.3 First authored primitive: Line

Introduce one authored Line primitive.

Required semantics:

- one Line owns one EntityId;
- one Line owns authored Start(U,V);
- one Line owns authored End(U,V);
- Start and End are independent authored geometry;
- exact zero-length input, where Start and End have exactly equal U/V values, is rejected as an invalid Line;
- R1-A uses no epsilon/near-zero rejection rule; geometric tolerance policy remains deferred;
- Line geometry contains no constraint, dimension, snap, inference, grip, profile or Viewer state.

### 3.4 Minimal Sketch model ownership

Introduce the smallest value-semantic Sketch model/container needed to own Lines.

Required behavior:

- add one valid Line and obtain/observe its stable EntityId;
- query entity count;
- resolve an existing Line by EntityId;
- unknown EntityId fails cleanly;
- invalid Line input leaves the model unchanged;
- adding multiple Lines produces distinct EntityIds;
- equal coordinates between endpoints of different Lines are allowed and create no implicit relation.

The collection must not imply profile topology, connectivity or constraint graph semantics.

No generalized speculative entity hierarchy is required merely to anticipate Arc/Circle.

### 3.5 Value semantics needed by future host Undo/Redo

The Shared 2D authored state must be safely copyable as ordinary authored state so a future Part host can place it inside its existing state-before/state-after history mechanism.

Copying a model:

- preserves all EntityIds and authored Line geometry;
- creates independent value state rather than aliasing mutable entity storage;
- introduces no runtime/session identity.

This does not integrate with DocumentSession in R1-A.

## 4. Explicit architecture boundaries

The `simplesolid2_sketch` target must remain independent from:

```text
Part
Application / DocumentSession
Persistence
Qt
Viewer
viewer_qt_occt
OCCT
filesystem paths
```

R1-A must not add those dependencies to public or private Sketch Core code.

No Viewer token, pointer, container index or Qt object may become EntityId.

No persistent mutation path outside Shared 2D itself is introduced because no host is integrated yet.

If implementation appears to require Part, persistence, Viewer, constraints, a solver, a generalized topology framework or a universal SubElement/reference API, stop and amend/split the contract instead of expanding scope.

## 5. Deliberately OUT

```text
PartSketch integration
Part authored schema change / schema v3
DocumentSession commands
Save / Close / Reopen for Lines
Undo / Redo integration
Qt / Workbench / toolbar
Command Line
Operations UI
dynamic input
Viewport pointer input
Sketch presentation in Viewer
selection
grips
SubElement public API
standalone Point entity
Circle / Arc
Construction geometry
snap / Object Snap
inference
constraints
dimensions
solver / DOF
measure / diagnostics implementation
intersection engine
profiles / planar regions
Trim / Extend / Offset
Extrude / Body / Feature
Assembly / Drawing integration
```

No placeholder types should be added for these features.

## 6. Acceptance tests

At minimum prove:

1. `simplesolid2_sketch` builds without Part/Application/Persistence/Qt/Viewer/OCCT dependencies.
2. Finite U/V coordinates are accepted.
3. NaN and ±Infinity are rejected before model mutation.
4. A valid non-zero Line can be added to an empty model.
5. The added Line receives a valid stable model-local EntityId.
6. Two added Lines receive different EntityIds.
7. Lookup by existing EntityId returns the authored Line geometry.
8. Lookup by unknown EntityId fails cleanly without mutation.
9. Exact zero-length Line input is rejected and leaves entity count unchanged.
10. Two different Lines may have endpoints with exactly equal coordinates without sharing authored identity or creating another authored object/relation.
11. Copying authored Sketch model state preserves EntityIds and geometry.
12. Mutating/replacing geometry in one copied state during semantic tests cannot mutate the other state through aliasing.
13. Entity identity does not derive from collection index or object address.
14. Existing SketchId, Part, persistence, Workbench, Viewer and full regression tests remain PASS.
15. Exact-head Windows documentation/verify/build/CTest gate is PASS.

The tests should prefer semantic behavior over exact private storage layout.

## 7. Allowed implementation surface

Expected files are limited to:

```text
src/sketch/**
src/CMakeLists.txt
tests/CMakeLists.txt
tests/sk02a_*
docs/internal/SHARED_2D.md
docs/internal/BUILD_AND_TEST.md        if test inventory changes require it
docs/browser/index.html                generated only if canonical docs require regeneration
work/ACTIVE.yaml
work/SK-02A_MINIMAL_SHARED_2D_AUTHORED_CORE.md
```

Changes outside this surface require explicit justification and contract review before mutation.

No `src/part/**`, `src/application/**`, `src/ui/**`, `src/viewer/**`, `src/viewer_qt_occt/**` or persistence implementation file is in scope.

## 8. Implementation slicing

Keep the implementation revertible:

```text
Slice A
neutral coordinate semantics + EntityId + focused tests

Slice B
Line authored value + validation tests

Slice C
minimal Sketch model ownership/lookup/value-copy tests

Slice D
internal as-built docs + full regression + exact-head gate
```

A slice may be merged/reordered locally if that reduces code churn without changing scope.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: SK-02A introduces the first implemented Shared 2D authored entity model and identity semantics, but no user-visible Sketch drawing capability exists yet.

## Roadmap impact

Roadmap milestone: R1 — Minimal Shared 2D authored core  
Roadmap version: 1.0  
Roadmap change: none; this contract implements the first bounded slice of R1.

## 9. Completion

SK-02A completes only when:

- the host-neutral authored Line model and identity semantics are proven by tests;
- the Sketch target remains free of forbidden dependencies;
- required internal as-built documentation is current;
- full regression remains PASS;
- exact-head Windows gate passes;
- completion bookkeeping records what was actually implemented.

Completion does not activate Part integration or interactive Line tooling. The next roadmap work remains a separate explicit contract.
