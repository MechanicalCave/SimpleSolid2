# PM-03 — Datum Reference Geometry / Offset Datum Plane

**Status:** PROPOSED — NOT ACTIVE; OWNER D2 REVIEW REQUIRED  
**Decision class:** D2 production Work Contract candidate  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.17  
**Architecture authority:** Constitution + Foundation + ADR-0014 + ADR-0016 + ADR-0017  
**Entry gate:** PM-02 Body Semantic Topology / Face-Supported Sketch COMPLETED — PASS  
**Production mutation:** NOT AUTHORIZED until explicit Owner acceptance and `work/ACTIVE.yaml` activation

## 1. Goal

Deliver the first Part-owned construction-reference feature on top of the completed PM-02 semantic reference foundation:

```text
Origin Plane OR resolved planar Body Surface OR existing Datum Plane
    -> authored Offset Datum Plane
    -> deterministic derived O/U/V/N frame
    -> Sketch support on Datum Plane
    -> Profile
    -> existing Extrude Add/Cut
    -> upstream/source edit
    -> datum re-evaluation
    -> dependent Sketch/Profile re-resolution
    -> recompute or structured failure
    -> Undo/Redo
    -> Save/Close/Reopen
    -> cold rebuild
```

The package proves durable Datum identity, bounded Datum dependency semantics and Datum-backed Sketch support without introducing a general dependency framework.

## 2. Proposed package narrowing — Owner decision

The Part-v1 roadmap names Datum Plane/Axis/Point constructors that are justified by accepted workflows.

This contract proposes that PM-03 deliver **Offset Datum Plane only** because it is the only Datum constructor required by the currently accepted next workflow: create a stable construction plane and host a Sketch/Profile that can feed the already-delivered Extrude Feature.

PM-03 therefore does **not** create Datum Axis or Datum Point merely to populate a generic reference-geometry framework.

Rationale:

- PM-04 Revolve already permits a non-degenerate straight Sketch line as an axis, so Datum Axis is not yet a prerequisite;
- no accepted current workflow requires an authored Datum Point;
- Plane/Plane and Axis/Plane intersections would introduce new numerical degeneracy policy whose product need has not yet been demonstrated;
- Foundation forbids speculative universal frameworks.

Owner acceptance of this contract explicitly accepts this bounded PM-03 narrowing. Datum Axis/Point may be introduced later by a separately accepted contract or amendment when a concrete consumer requires them.

## 3. Governing invariants

Implementation must preserve:

- Part owns Datum semantics; Viewer/UI/Kernel do not;
- Datum authored state is design intent; displayed plane patches/grids are derived presentation;
- `DatumId` is durable semantic identity and is independent of vector index, tree row, display name and Viewer token;
- Body planar support uses the existing durable `SurfaceReference` and its explicit `BodyStageRef`;
- no provider handle, topology ordinal, Face[n], tessellation identity or geometry fingerprint enters Datum persistence;
- Datum world frames are derived from semantic sources and are never a second authored placement truth;
- revalidation occurs at Finish/commit;
- stale revision/session/evaluation/presentation input fails closed;
- no stale last-good Datum frame may feed Sketch/Profile/Feature evaluation;
- no universal project-wide dependency graph is introduced.

## 4. Scope IN

### 4.1 Durable Datum identity

Introduce Part-local `DatumId` with the same non-aliasing/high-water lifecycle principles already used for BodyId, FeatureId, SketchId/ProfileId-related durable identity.

Required semantics:

- creation allocates a fresh DatumId;
- edit preserves DatumId;
- Undo of creation/deletion restores the original semantic identity;
- abandoned identities are not silently reused after Undo branching;
- outside the Part, Datum identity is qualified by DocumentId.

A `DatumIdCursor` or equivalent Part-owned allocator is permitted.

### 4.2 One authored Datum type: Offset Datum Plane

The only PM-03 authored Datum constructor is conceptually:

```text
OffsetDatumPlane
    id: DatumId
    source: PlaneReference
    offset: signed Length
    visibility: authored presentation flag
```

Exact C++ names may differ.

`offset`:

- is finite;
- is unit-aware through existing length semantics;
- may be positive, negative or zero;
- is authored design intent.

Zero offset is legal. It creates a distinct semantic Datum identity even when the evaluated plane is geometrically coincident with its source.

Geometry equality never aliases Datum identity.

### 4.3 PlaneReference

PM-03 introduces the smallest Part-owned plane-source reference required by this package.

Accepted source variants:

1. built-in Origin plane: XY / XZ / YZ;
2. existing PM-02 resolved planar Body Surface using durable `SurfaceReference`;
3. another Datum Plane using `DatumId`.

A PlaneReference must not contain:

- Viewer/presentation token;
- runtime Face/Surface token;
- provider handle;
- topology index;
- geometric nearest/equality selector.

Body Surface resolution remains stage-scoped and fail-closed under PM-02 semantics.

### 4.4 Deterministic Datum Plane frame

The source plane resolves to a right-handed metric frame:

```text
O_source, U_source, V_source, N_source
N_source = U_source x V_source
```

The Offset Datum Plane frame is exactly:

```text
O = O_source + offset * N_source
U = U_source
V = V_source
N = N_source
```

No camera orientation, provider UV, first edge, current trim loop, bounding box or face centroid may affect the Datum frame.

For a Body Surface source, PM-02 canonical Surface frame is authoritative.

For a Datum source, the evaluated source Datum frame is authoritative.

### 4.5 Local Datum dependency semantics

Datum-to-Datum dependency is Part-local and bounded.

PM-03 may implement explicit local dependency traversal/cycle detection only for the relationships introduced by this package.

Rules:

- a Datum Plane may reference another Datum Plane;
- direct or indirect Datum cycle is rejected before mutation;
- cycle rejection performs zero authored mutation and creates no Undo entry;
- a global dependency graph is not required.

Evaluation must be deterministic from authored Part state and accepted Body-stage results.

### 4.6 Body-stage dependency floor

A Datum Plane sourced from a Body Surface inherits the source `BodyStageRef`.

A chain of Datum Planes carries the transitive latest required Body stage of its sources.

When a Sketch supported by a Datum Plane is consumed by a Feature, the consuming Feature must be semantically downstream of every required Body stage in the Datum chain.

A create/edit/re-support operation that would create a self/downstream cycle is rejected before mutation.

The implementation may reuse the bounded ordered-Feature/support reasoning established by PM-02. It must not create a universal dependency scheduler.

### 4.7 Datum evaluation status

Authored Datum state and evaluated Datum state are distinct.

Required evaluated outcomes include at least:

- Resolved / UpToDate;
- Missing source;
- Ambiguous source;
- Unsupported source;
- Blocked by unavailable upstream Datum/Body stage.

A valid authored Datum remains present and editable if an upstream source later becomes unresolved.

There is no stale fallback frame.

For a Body Surface source:

- Missing Surface -> Datum Missing;
- Ambiguous Surface -> Datum Ambiguous;
- non-planar/Unsupported Surface -> Datum Unsupported.

For a Datum source, dependent failure propagates structurally as Blocked/unavailable without rewriting authored references.

### 4.8 Sketch support on Datum Plane

Extend Part Sketch support with a durable Datum-plane support variant conceptually:

```text
DatumPlaneSketchSupport
    datum_id: DatumId
```

World Sketch placement is derived from the current resolved Datum Plane frame.

Required behavior:

- Create Sketch on a resolved Datum Plane;
- Change Sketch Support to a resolved Datum Plane;
- re-support from Datum Plane to Origin/Body Surface and vice versa;
- SketchId, EntityIds, ProfileId and authored local U/V geometry remain unchanged on re-support;
- Missing/Ambiguous/Unsupported Datum support provides no current frame;
- downstream Feature evaluation cannot consume stale Datum-backed Profile geometry.

### 4.9 Commands / Transactions / history

All durable Datum mutations use ordinary semantic command authority:

```text
GUI / Command Line / semantic caller
    -> Command
    -> Validation
    -> revision/context revalidation
    -> Part transaction
    -> PartDocument
    -> evaluation
```

Required semantic operations:

- Create Offset Datum Plane;
- Edit Offset Datum Plane;
- Delete unreferenced Datum Plane;
- Show/Hide Datum Plane;
- Create/Re-support Sketch on Datum Plane through the existing Sketch command path.

One accepted Finish equals one transaction and one Undo step.

Preview, selection, hover, invalid Finish and Cancel create no authored mutation.

### 4.10 Delete semantics

Datum deletion is conservative.

Delete is rejected atomically if the Datum is directly or transitively required by:

- another Datum Plane;
- a Sketch support;
- an authored downstream relationship introduced by this package.

PM-03 does not silently convert dependents to Missing merely because the producer is deleted by the same-document user action.

This mirrors the existing support-producing Feature Delete safety direction from PM-02I.

A future explicit Delete-with-dependents or dependency-repair workflow requires separate scope.

### 4.11 UI / Tree / Properties

PM-03 may add the minimum Part UI required for the accepted workflow.

Expected product surface:

- Reference Geometry action: Datum Plane;
- selection-first or command-first source acquisition;
- source can be Origin plane, planar Body Face/Surface or existing Datum Plane;
- Operations panel exposes source, signed Offset and Finish/Cancel;
- unit-aware Command Line input uses the same draft/semantic command;
- Part tree exposes a Reference Geometry / Datum Plane item or equivalent clear Part-owned projection;
- Properties exposes DatumId-facing user identity only through normal labels, constructor type, source meaning, offset, visibility and current evaluation status;
- no provider/runtime token is shown as semantic identity.

Exact iconography/layout is D0/D1.

### 4.12 Datum presentation and picking

Datum Plane display is derived presentation.

The Viewer may receive a neutral finite plane patch/grid or equivalent geometry with a presentation token.

Rules:

- finite display extent is presentation-only and not authored engineering size;
- Viewer token is transient and never serialized;
- Application/Part binding maps the current presentation token to DatumId;
- stale presentation generation cannot commit an edit or Sketch support;
- Datum Plane picking must not be confused with Body Face topology picking;
- hidden Datum Plane is not ordinarily pickable.

No new general Viewer reference-geometry framework beyond the concrete Datum Plane need is authorized.

### 4.13 Persistence

PM-03 may advance the native Part schema from v9 to the next version required for:

- DatumId high-water state;
- authored Offset Datum Plane records;
- authored Datum visibility;
- Datum-backed Sketch support.

Migration from valid v9:

- creates an empty Datum collection;
- preserves all existing DocumentId/BodyId/FeatureId/SketchId/EntityId/ProfileId values;
- preserves current PM-02 Sketch support meaning;
- introduces no provider/runtime state.

Malformed Datum state fails closed during load/reconstruction.

### 4.14 Cold rebuild

A true cold rebuild must reconstruct:

- Datum identity;
- Datum source references;
- resolved Datum frames/status;
- Datum-backed Sketch world frames;
- downstream Profile/Extrude behavior

from durable authored state and legal dependencies without previous-process runtime/provider tokens or caches.

## 5. Scope OUT

PM-03 does not include:

- Datum Axis;
- Datum Point;
- arbitrary plane-through-points constructors;
- angle-to-plane Datum;
- tangent Datum;
- plane normal-to-curve;
- midpoint/bisector Datum;
- Sketch-line-to-Datum-Axis conversion;
- Projection / Project Edge;
- automatic selected-face boundary capture into Sketch;
- Revolve;
- Fillet / Chamfer;
- Through All / Up To Face Extrude;
- non-planar Sketch mapping;
- multi-body;
- generic persistent EdgeReference/VertexReference;
- provider-native identity;
- geometry-similarity rebinding;
- global dependency graph;
- Assembly or Drawing implementation.

## 6. Public / architecture contract impact

This package deliberately proposes D2 changes in these bounded areas:

- new durable Part-local DatumId;
- new durable Part authored Datum Plane state;
- new PlaneReference family reusing existing semantic sources;
- new Datum-backed Sketch support meaning;
- native Part schema migration;
- Part-local dependency/cycle semantics for Datum-backed support.

It does not change Foundation ownership:

- Part owns Datum meaning;
- Shared 2D remains unaware of Datum;
- Viewer owns only presentation/picking mechanics;
- Kernel/provider identity remains non-authoritative.

Owner acceptance of this exact Work Contract is required before implementation.

## 7. Failure behavior

Fail closed.

Creation/edit Finish fails with no mutation when:

- source is stale;
- source reference is Missing/Ambiguous/Unsupported;
- Body source is not planar;
- Datum source is unresolved;
- offset is non-finite;
- operation would create a Datum cycle;
- operation would create a Body-stage/Sketch/Feature dependency cycle;
- DocumentRevision/session/evaluation/presentation context is stale.

If an already-authored Datum becomes unresolved after an upstream edit:

- preserve DatumId and authored constructor/reference/offset;
- publish structured current status;
- do not publish a stale frame;
- dependent Sketch support becomes unavailable;
- downstream Features become appropriately Blocked/unavailable;
- repair occurs only through explicit source/edit/re-support commands.

## 8. Proposed checkpoint sequence

### PM-03A — semantic Datum foundation + schema

Deliver:

- DatumId / high-water allocation;
- authored Offset Datum Plane model;
- PlaneReference;
- schema migration;
- Part authored-state validation;
- semantic/core tests.

Gate:

- no provider/UI dependency in authored Datum model;
- v9 migration preserves all existing durable IDs;
- invalid/cyclic Datum state rejected fail-closed.

### PM-03B — evaluator + dependency/cycle semantics

Deliver:

- deterministic frame derivation;
- Body Surface stage resolution;
- Datum chaining;
- current status/diagnostics;
- transitive Body-stage dependency floor.

Gate:

- source move updates derived frame without authored placement mutation;
- Missing/Ambiguous/Unsupported yields no frame;
- cycle cases reject before mutation;
- cold semantic evaluation deterministic.

### PM-03C — commands + preview/edit/history

Deliver:

- Create/Edit/Delete/Show/Hide commands;
- one runtime draft shared by GUI/Command Line;
- transient preview;
- one Finish transaction;
- Undo/Redo.

Gate:

- Cancel/rejected Finish = zero mutation/history;
- edit preserves DatumId;
- Delete dependency rejection is atomic;
- stale draft cannot commit.

### PM-03D — Viewer / Tree / Properties

Deliver:

- neutral Datum Plane presentation/pick binding;
- tree projection;
- Properties/status;
- visibility;
- selection-first source workflow.

Gate:

- display size does not become authored plane size;
- stale presentation cannot commit;
- Viewer token never becomes CAD identity.

### PM-03E — Datum-backed Sketch + existing Extrude

Deliver:

- Create Sketch on Datum Plane;
- Change Sketch Support to/from Datum Plane;
- stage-aware Datum-backed Profile evaluation;
- existing Extrude Add/Cut reuse.

Gate:

- local Sketch U/V geometry preserved;
- upstream source/offset edit moves Datum-backed Sketch correctly;
- invalid source produces no stale downstream modeling;
- Body-stage cycle rejected.

### PM-03F — lifecycle / persistence / docs / Owner acceptance

Close:

- Save/Close/Reopen;
- true cold rebuild;
- integrated repair/failure matrix;
- internal as-built docs;
- PL/EN product docs;
- Product Browser regeneration;
- supported Windows Owner workflow.

## 9. Acceptance requirements

PM-03 cannot complete without automated and manual evidence for at least:

- DatumId non-aliasing/high-water lifecycle;
- v9 -> new schema migration preserving existing IDs;
- Offset Datum Plane from XY/XZ/YZ;
- positive, negative and zero offset;
- Offset Datum Plane from planar Body Surface;
- Offset Datum Plane from another Datum Plane;
- source Body Surface movement after upstream edit;
- source Surface Missing;
- source Surface Ambiguous;
- non-planar Body Surface rejected/Unsupported;
- Datum-to-Datum cycle rejection;
- Datum/Sketch/Feature stage-cycle rejection;
- edit preserves DatumId;
- Delete unreferenced Datum succeeds;
- Delete referenced Datum rejects atomically;
- Show/Hide affects presentation only;
- preview/Cancel/no-op produce no CAD history;
- stale revision/session/evaluation/presentation cannot commit;
- Create Sketch on Datum Plane;
- re-support Sketch to/from Datum Plane preserving IDs/local U/V;
- Profile + existing Extrude Add/Cut from Datum-backed Sketch;
- upstream Datum offset/source change recomputes downstream result or produces structured failure;
- no stale Datum frame after source failure;
- Undo/Redo;
- Save/Close/Reopen;
- cold rebuild with fresh runtime/provider tokens;
- GUI / Command Line / semantic command parity;
- semantic/core, kernel-native where applicable and desktop verification;
- final Owner Windows manual workflow.

## 10. Manual Windows acceptance themes

The final Owner workflow should cover:

1. create Base Body;
2. create Datum Plane offset from an Origin plane;
3. edit offset positive/negative/zero and verify deterministic orientation;
4. create Datum Plane from a planar Body Face;
5. create a second Datum Plane from the first Datum Plane;
6. inspect Tree/Properties/status and Show/Hide;
7. create Sketch on Datum Plane;
8. author Profile and Extrude Add/Cut;
9. edit upstream Body dimension so Body-Surface-backed Datum moves;
10. edit Datum offset and verify Sketch local geometry remains unchanged;
11. exercise source Missing/Ambiguous and verify no stale modeled result;
12. repair source/re-support;
13. verify cycle-causing re-reference rejects with zero mutation;
14. Undo/Redo;
15. Save, close, reopen and verify cold reconstruction.

## 11. Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: PM-03 introduces durable Datum identity/state, new native persistence, Datum Plane creation/editing/visibility, Datum-backed Sketch support and new failure/repair behavior.

Before completion:

- internal Part/persistence/viewer docs describe as-built Datum ownership/evaluation;
- PL/EN product docs describe Offset Datum Plane and Sketch-on-Datum workflow;
- Product Browser is regenerated from canonical Markdown;
- deferred Datum Axis/Point must not be documented as implemented.

## 12. STOP conditions

STOP and return to Owner review if implementation would require:

- changing Foundation domain ownership;
- making Shared 2D own Datum meaning;
- provider-native or Viewer identity in durable Datum state;
- geometry similarity/proximity as rebinding authority;
- a Datum Axis/Point implementation without explicit scope amendment;
- a new general durable Edge/Vertex reference family;
- a global dependency graph;
- multi-body;
- Projection as a prerequisite;
- non-planar Sketch mapping;
- a new numerical tolerance policy that changes semantic success/failure outside already accepted PM-02 reference resolution;
- a public dependency direction contrary to Architecture baseline.

## 13. Activation and completion boundary

This file is a proposal only.

No production mutation is legal until:

1. the Owner explicitly accepts this exact Work Contract (including the Offset-Plane-only PM-03 narrowing);
2. repository governance records that acceptance;
3. `work/ACTIVE.yaml` points to this contract with `status: active`;
4. the activation change passes the required repository gate.

PM-03 completion authorizes only the delivered Offset Datum Plane / Datum-backed Sketch vertical slice.

PM-04 Axis / Revolve and Projection remain separately gated.
