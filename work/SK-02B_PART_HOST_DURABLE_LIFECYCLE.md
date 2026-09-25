# SK-02B — Part Host Integration and Durable Sketch Lifecycle

**Status:** ACCEPTED — COMPLETED  
**Owner acceptance:** 2026-09-25  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.1  
**Roadmap milestone:** R2 — Part host integration and durable lifecycle

## 1. Goal

Integrate the completed host-neutral Shared 2D authored model into the existing Part-hosted Sketch lifecycle without transferring Part, persistence, application or UI semantics into Shared 2D.

R2 proves the durable path:

```text
semantic Sketch command
        ↓
DocumentSession validation
        ↓
Part authored-state transaction
        ↓
PartSketch owns Shared 2D SketchModel
        ↓
Undo / Redo / dirty checkpoint
        ↓
Part schema v3 persistence
        ↓
Save → Close → Reopen
        ↓
same SketchId + EntityId + authored Line geometry
```

R2 deliberately stops before Viewer presentation and interactive Line tooling.

## 2. Accepted design carried into R2

R2 must preserve ADR-0008/0009 and completed SK-02A semantics:

- Shared 2D remains host-neutral;
- `EntityId` is model-local semantic identity;
- Line Start/End are independent authored coordinates;
- exact zero-length Line is invalid;
- no epsilon/near-zero policy is introduced;
- equal coordinates do not create shared Point identity or a relation;
- state/history copying preserves entity identity;
- semantic duplication would require fresh identity;
- UI/Viewer tokens never become authored identity;
- authored and evaluated geometry remain architecturally distinct, although R2 still has identity evaluation only.

## 3. Scope IN

### 3.1 PartSketch owns one Shared 2D authored model by value

Each persistent `PartSketch` owns exactly one `sketch::SketchModel` as embedded authored state.

Required ownership:

```text
PartSketch
├─ SketchId
├─ Part support
├─ Part placement
├─ Part visibility
└─ Shared 2D SketchModel
   └─ authored Lines / EntityIds
```

Part continues to own support, placement, visibility, host lifecycle and persistence policy.

Shared 2D continues to own entity identity and 2D authored geometry.

The embedded model is value state so ordinary `PartAuthoredState` copy/history remains independent and deterministic.

No separate Sketch file/document/session is introduced.

### 3.2 EntityId remains model-local and gets a canonical persistence encoding

R2 freezes the first durable `EntityId` file representation.

Required semantics:

- `EntityId` remains semantically opaque to Part/Application/UI;
- implementation remains a positive monotonic model-local integer identity;
- native Part schema v3 encodes EntityIds as canonical unsigned decimal strings, not JSON floating-point numbers;
- no leading zeroes are accepted except that zero itself is not a valid EntityId;
- the encoding is persistence transport, not permission for Part/UI to perform arithmetic on EntityIds;
- the same serialized EntityId value may legally occur in two different SketchModels because scope is model-local;
- outside a model, a durable entity address requires its owning Sketch identity plus EntityId.

Exact helper/class names used to encode/decode remain implementation detail.

### 3.3 Durable identity high-water prevents reuse

A SketchModel must retain a technical monotonic allocation cursor/high-water in addition to its current entity set.

Schema v3 persists this cursor as:

```json
"next_entity_id": "3"
```

Required semantics:

- every currently stored entity ID is strictly lower than `next_entity_id`;
- erasing an entity never lowers the cursor;
- Save/Reopen restores the cursor;
- a deleted and durably saved EntityId is not reused after reopen;
- malformed/duplicate/out-of-range identity state fails closed.

The allocation cursor is durable semantic-identity infrastructure, not user geometry.

It must not be used as collection order or display numbering.

### 3.4 Undo/Redo must not rewind the live identity lineage

R1 ordinary value-copy preserves the complete model state. R2 must additionally handle history branching correctly.

Required runtime rule:

> applying an older historical geometry state must not lower the active SketchModel identity allocation high-water for a continuing Sketch.

Conceptually:

```text
Line1 allocated
Line2 allocated
Undo Line2
new branch: Add Line

new Line must NOT become Line2
```

The history application path therefore preserves the maximum identity allocation cursor already observed by that continuing Sketch runtime lineage.

This technical high-water does not by itself make the document semantically dirty.

Semantic authored equality / dirty checkpoint behavior is based on current authored entities and their geometry/identity, not solely on a larger technical allocation cursor.

Consequences:

- Add → Undo back to the saved geometry may become clean;
- a subsequent Add in the same live session still receives a fresh EntityId;
- if deletion/identity history is actually saved, the current high-water is serialized so reopen also avoids reuse.

Exact private helper names/mechanics for preserving the cursor are implementation detail.

### 3.5 Minimal semantic Sketch entity commands

Introduce only the semantic commands needed to prove the R2 lifecycle.

#### AddSketchLine

Conceptual input:

```text
SketchId
Start(U,V)
End(U,V)
```

Required behavior:

- target Sketch must exist;
- geometry must satisfy Shared 2D Line validation;
- command allocates a fresh EntityId in that SketchModel;
- success is one Part transaction, one history entry and one document revision increment;
- result returns the created EntityId;
- failure leaves state, revision, dirty checkpoint and history unchanged.

#### EraseSketchEntity

Conceptual input:

```text
SketchId
EntityId
```

Required behavior:

- target Sketch must exist;
- EntityId must resolve in that SketchModel;
- successful erase is one Part transaction / one history entry / one revision increment;
- unknown/stale target fails closed instead of becoming an implicit no-op;
- Undo restores exactly the same EntityId and Line geometry;
- Redo removes exactly that same entity again.

R2 does not introduce selection or multi-selection Delete. A later interactive contract may compose one or many selected semantic targets into its own command policy.

### 3.6 Command-level Undo/Redo granularity

R2 freezes only command-level granularity:

```text
one successful AddSketchLine command
→ one Undo entry

one successful EraseSketchEntity command
→ one Undo entry
```

This does not decide R4 continuous-Line UX granularity. R4 must still explicitly decide whether multiple continuously created segments are separate commands/history entries or are grouped by a higher-level interaction transaction.

No UI event becomes an Undo boundary merely because it occurred.

### 3.7 Dirty/save checkpoint behavior

Required behavior:

- successful Add/Erase makes the session dirty when authored geometry differs from the saved checkpoint;
- failed/rejected/no-change operations do not dirty the session;
- Undo back to the saved semantic authored state becomes clean;
- Redo away from it becomes dirty;
- successful Save advances the saved authored checkpoint;
- failed Save does not alter dirty/history state.

Technical identity high-water alone must not create a false user-visible dirty state after Undo restores the saved authored geometry.

### 3.8 Part persistence schema v3

R2 advances:

```text
Part domain schema 2 → 3
```

Schema v3 keeps existing Sketch host fields and adds the embedded Shared 2D model.

Proposed canonical Sketch record shape:

```json
{
  "id": "<SketchId>",
  "support": {
    "kind": "builtin_origin_plane",
    "builtin_plane": "xy_plane"
  },
  "placement": {
    "origin": [0, 0, 0],
    "u_axis": [1, 0, 0],
    "v_axis": [0, 1, 0]
  },
  "visible": true,
  "model": {
    "next_entity_id": "3",
    "lines": [
      {
        "id": "1",
        "start": [0.0, 0.0],
        "end": [25.0, 0.0]
      },
      {
        "id": "2",
        "start": [25.0, 0.0],
        "end": [25.0, 10.0]
      }
    ]
  }
}
```

R2 deliberately uses a `lines` collection rather than inventing a speculative universal entity-variant persistence framework before another primitive exists.

Required strict validation:

- exact expected object structure for schema v3;
- valid SketchId/support/placement/visibility as today;
- `model` required for every v3 Sketch;
- canonical valid `next_entity_id`;
- each Line ID canonical, valid and unique inside its SketchModel;
- each Line ID strictly lower than `next_entity_id`;
- same numeric/textual EntityId in another SketchModel is allowed;
- Start/End arrays contain exactly two finite numbers;
- exact zero-length Line is rejected;
- malformed model state fails closed before exposing a PartDocument.

No selection, tool, cursor, Origin runtime state, constraints, preview or Viewer state is serialized.

### 3.9 Backward readability

Required compatibility:

- schema v1 remains readable as today: no hosted Sketches;
- schema v2 remains readable: existing hosted Sketch records restore with an empty Shared 2D model and initial identity cursor;
- opening v1/v2 does not rewrite the file;
- the next successful Save writes current schema v3;
- schema v3 round-trip preserves SketchId, host semantics, EntityIds, allocation high-water and Line geometry.

No unsupported future schema is guessed or partially loaded.

### 3.10 Save → Close → Reopen proof

R2 must prove a real lifecycle equivalent to:

```text
create/open Part
→ create/find PartSketch
→ AddSketchLine
→ Save
→ destroy runtime DocumentSession
→ reload Part from native file
→ create fresh runtime session
→ resolve same SketchId
→ resolve same EntityId
→ exact same Start/End
```

It must additionally prove persisted non-reuse:

```text
allocate Line A
allocate Line B
erase B
Save
Close/Reopen
add another Line
→ new EntityId != B
```

## 4. Explicit architecture boundaries

R2 may change Shared 2D only where required for durable identity transport/history integration.

It must not make Shared 2D depend on:

```text
Part
Application / DocumentSession
Persistence implementation
Qt
Viewer
viewer_qt_occt
OCCT
filesystem paths
```

Part persistence may consume a neutral Shared 2D identity/state transfer seam, but host-specific JSON/schema policy remains in Part.

No raw pointer, vector index, Viewer token, Tree row or OCCT/provider identity may be persisted as Sketch entity identity.

If implementation requires a generalized entity hierarchy, constraints, Viewer input, profile framework or universal topology/reference system, stop and amend/split rather than expanding R2.

## 5. Deliberately OUT

```text
interactive Line tool
rubber-band preview
Select tool implementation
point/rectangle selection
pick-box / crosshair cursor implementation
Viewport pointer → Sketch U/V
Origin runtime presentation/snap
Command Line UI
Operations UI
dynamic input
grips / direct manipulation
Circle / Arc
Construction role
Object Snap / inference
constraints / dimensions / solver / DOF
measurement UI
intersections
profiles / planar regions
planar-face Sketch support
projected/reference geometry
Trim / Extend / Split / Offset
Extrude / Body / Feature
Assembly / Drawing integration
```

No placeholders for these features are added.

## 6. Acceptance tests

At minimum prove:

1. `PartSketch` owns one value-semantic Shared 2D `SketchModel`.
2. Existing newly created PartSketch begins with an empty model.
3. AddSketchLine to an existing Sketch succeeds and returns a valid EntityId.
4. Add to an unknown SketchId fails with no state/revision/history/dirty mutation.
5. NaN/Infinity/exact-zero Line input fails with no mutation.
6. Successful Add creates exactly one history entry and one Part revision increment.
7. Undo Add removes the Line; Redo restores the same EntityId and geometry.
8. After Add → Undo → new Add branch, the new entity does not reuse the undone entity ID.
9. Erase existing entity succeeds as one history entry.
10. Erase unknown/stale entity fails closed with no mutation/history truncation.
11. Undo Erase restores the exact same EntityId and geometry; Redo erases it again.
12. Undo back to saved semantic state becomes clean even when live identity high-water is greater.
13. Two different SketchModels may legally contain the same local EntityId value without aliasing because SketchId scopes the address.
14. Schema v3 Save/Reopen preserves SketchId, support, placement, visibility, EntityId, Line Start/End and identity cursor.
15. Persisted deletion followed by Save→Close→Reopen→Add does not reuse the deleted EntityId.
16. Schema v2 files remain readable and produce empty SketchModels.
17. Schema v1 files remain readable as before.
18. Saving a loaded v1/v2 document publishes schema v3.
19. Duplicate EntityIds inside one persisted SketchModel are rejected.
20. EntityId >= persisted `next_entity_id` is rejected.
21. Non-canonical/zero/out-of-range persisted EntityIds are rejected.
22. Non-finite or exact-zero persisted Lines are rejected.
23. Same serialized local EntityId in two different Sketches is accepted.
24. No Part/Application/Persistence/Qt/Viewer/OCCT dependency leaks into Shared 2D.
25. Existing R1, Part, persistence, Workbench and Viewer regressions remain PASS.
26. Internal as-built documentation is current.
27. Final exact-head Windows docs/verify/build/CTest gate is PASS.

Tests should prove semantic lifecycle behavior rather than private container layout.

## 7. Expected implementation surface

Expected changes are limited to:

```text
src/sketch/**
src/part/include/simplesolid2/part/part_sketch.hpp
src/part/include/simplesolid2/part/part_document_store.hpp
src/part/part_document_store.cpp
src/application/include/simplesolid2/application/document_session.hpp
src/application/document_session.cpp
src/CMakeLists.txt                     only if required
tests/CMakeLists.txt
tests/sk02b_*
existing focused Part/persistence tests where compatibility assertions belong
docs/internal/SHARED_2D.md
docs/internal/PART_DOCUMENTS.md
docs/internal/PERSISTENCE.md
docs/internal/BUILD_AND_TEST.md
docs/browser/index.html                generated
work/ACTIVE.yaml
work/SK-02B_PART_HOST_DURABLE_LIFECYCLE.md
work/SKETCH_ROADMAP.md                 completion bookkeeping only
```

No `src/ui/**`, `src/viewer/**` or `src/viewer_qt_occt/**` product implementation is in scope.

## 8. Implementation slicing

Keep changes independently reviewable where practical:

```text
Slice A
embed SketchModel in PartSketch + focused ownership/value tests

Slice B
EntityId canonical persistence transfer + identity high-water/history semantics

Slice C
AddSketchLine / EraseSketchEntity semantic commands + Undo/Redo/dirty tests

Slice D
Part schema v3 + v1/v2 compatibility + Save/Close/Reopen tests

Slice E
as-built docs + full regression + completion bookkeeping + exact-head gate
```

Slices may be locally merged/reordered when that reduces churn without changing contract semantics.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: R2 changes internal authored ownership, semantic command/history behavior and native Part persistence, but still adds no user-visible interactive Sketch drawing workflow.

## Roadmap impact

Roadmap milestone: R2 — Part host integration and durable lifecycle  
Roadmap version: 1.1  
Roadmap change: none; this contract implements the accepted R2 milestone.

## 9. Completion

SK-02B completes only when:

- PartSketch durably owns the Shared 2D model;
- Add/Erase commands use the existing semantic command/transaction/history path;
- entity identity remains non-reused across live Undo branching and persisted deletion/reopen;
- Part schema v3 round-trip and v1/v2 backward readability pass;
- Save→Close→Reopen preserves identity and geometry;
- dirty/Undo/Redo semantics are proven;
- Shared 2D dependency boundaries remain intact;
- internal documentation reflects as-built behavior;
- full regression passes;
- final exact-head Windows gate passes;
- completion bookkeeping reflects the tested implementation.

Completion does not activate R3 Viewer/input/presentation work. R3 requires a separate explicit Owner-accepted Work Contract.


## 10. Completion record

SK-02B is complete.

Implemented:

- each persistent PartSketch owns one value-semantic Shared 2D SketchModel;
- canonical decimal EntityId and EntityIdCursor transport with no public numeric identity arithmetic;
- validated SketchModel state/restore transfer;
- session-local per-Sketch identity high-water that is not rewound by Undo/Redo history;
- AddSketchLine and EraseSketchEntity semantic commands through the existing Part transaction/history path;
- semantic dirty-state behavior that ignores high-water-only advancement;
- Part schema v3 persistence of next_entity_id and authored Lines;
- strict persisted model validation;
- schema-v1 and schema-v2 backward readability with Save migration to v3;
- Save → Close → Reopen preservation of SketchId, EntityId, authored geometry and persisted non-reuse cursor;
- focused R2 semantic/persistence regression tests.

Deliberately not implemented:

- Viewer presentation or pointer-to-Sketch input;
- Select/rectangle selection/cursor UX;
- interactive continuous Line tooling;
- Origin runtime snap;
- grips, Object Snap, inference, constraints/solver, profiles or projection.

Final exact-head Windows documentation/verify/build/CTest verification is required on the completion head before merge.

Completion authorizes no R3 production mutation. R3 requires a separate explicit Owner-accepted Work Contract.
