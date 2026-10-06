# PM-03 — Datum Reference Geometry / Offset Datum Plane

**Status:** COMPLETED — PASS; OWNER FINAL WINDOWS ACCEPTANCE 2026-10-05  
**Decision class:** D2 production Work Contract  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.24  
**Architecture authority:** Constitution + Foundation + ADR-0014 + ADR-0016 + ADR-0017  
**Entry gate:** PM-02 Body Semantic Topology / Face-Supported Sketch COMPLETED — PASS  
**Owner UI acceptance:** 2026-10-05 — single Datum Plane tool, Offset constructor, Extrude-style preview/default 10 mm, Command Line parity, intersection overlay, Reference Geometry tree grouping/visibility  
**Production mutation:** CLOSED — future production change requires separately accepted authority

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

## 2. Accepted package narrowing

The Part-v1 roadmap names Datum Plane/Axis/Point constructors that are justified by accepted workflows.

This contract delivers **Offset Datum Plane only** because it is the only Datum constructor required by the currently accepted next workflow: create a stable construction plane and host a Sketch/Profile that can feed the already-delivered Extrude Feature.

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

### 4.11 Datum Plane tool / Operations panel

The product exposes one Part tool named **Datum Plane**.

`Offset` is the first constructor of that tool, not a separate user-facing tool type. Future accepted constructors extend the same Datum Plane tool/panel rather than creating unrelated workflows.

Both selection-first and command-first entry are required.

Accepted source acquisition:

- Origin XY/XZ/YZ plane;
- any uniquely resolved planar semantic Body Surface at its explicit `BodyStageRef`, acquired through the clicked bounded Face but persisted as `SurfaceReference`;
- an existing resolved Datum Plane.

The right Operations panel exposes at minimum:

- **Constructor:** `Offset` (the only PM-03 value);
- **Source:** current semantic source;
- **Offset:** signed unit-aware length;
- **Reverse:** convenience action that negates the signed Offset value; it is not independent authored state;
- **Finish**;
- **Cancel**.

For a newly acquired valid source, the default draft Offset is **10 mm** in document unit-aware input semantics. This is draft/UI default only; it does not alter an existing Datum during edit.

Tool interaction follows the established Extrude grammar:

- valid source -> immediate transient preview;
- changing Offset updates preview without authored mutation;
- Reverse updates the same signed Offset draft value;
- Finish revalidates current source/revision/context and commits exactly one transaction;
- Cancel, hover, source preview, rejected Finish and no-op do not create authored history.

Exact iconography, spacing and panel geometry remain D0/D1.

### 4.12 Command Line parity

Datum Plane is supported through the Command Line from the first delivered PM-03 slice.

GUI and Command Line must drive the **same runtime draft and semantic command path**. Command Line is not allowed to invoke UI widgets as mutation authority and must not reimplement Datum semantics independently.

The Command Line can at minimum:

- start/enter the Datum Plane Offset draft;
- acquire/set a legal semantic Source;
- set unit-aware signed Offset;
- invoke Reverse through the same sign-change semantics;
- Finish;
- Cancel.

GUI and Command Line must produce equivalent validation, preview meaning, Finish/Cancel behavior, diagnostics and authored result for the same semantic inputs.

### 4.13 Datum presentation, intersection overlay and picking

Datum Plane display is derived presentation.

Every visible Datum Plane preview/committed presentation contains:

1. a finite translucent plane patch or equivalent neutral plane fill;
2. a clear border for spatial orientation;
3. when the currently presented Body intersects the plane, a **virtual plane/Body intersection overlay** showing that intersection in 3D.

The intersection overlay is required because the finite plane patch alone is insufficient spatial feedback in common engineering views.

Rules:

- finite display extent is presentation-only and not authored engineering size;
- the intersection overlay is derived presentation only: it is not authored geometry, not an Edge/Curve semantic reference, not serialized and not Projection;
- intersection is computed against the current Body presentation truth available to the Viewer/Application binding; if there is no current intersecting Body result, the overlay is absent;
- any diagnostic resolved-prefix use must remain presentation-only and must never become successful modeling/reference authority;
- Viewer/presentation tokens are transient and never serialized;
- Application/Part binding maps current Datum presentation tokens to DatumId;
- stale presentation generation cannot commit an edit or Sketch support;
- Datum Plane picking must not be confused with Body Face topology picking;
- the plane patch and border pick the owning Datum Plane;
- clicking the virtual intersection overlay may also select the owning Datum Plane, but **never** exposes an Edge/Curve pick or semantic identity;
- hidden Datum Plane is not ordinarily pickable.

No new general Viewer reference-geometry framework and no hidden Projection implementation are authorized by this presentation requirement.

### 4.14 Tree, grouping, visibility and Properties

Reference geometry is projected in the Part tree directly below `Origin` as one explicit group:

```text
Origin
Reference Geometry
  Datum Plane 1
  Datum Plane 2
  ...
<remaining Part model/history projection>
```

The group is the future product home for accepted Datum Plane/Axis/Point objects, but PM-03 populates it with Datum Plane only.

Visibility semantics:

- every Datum Plane has its own authored persistent visibility flag;
- toggling the `Reference Geometry` group performs a normal authored bulk visibility operation over its Datum children;
- the group does **not** introduce a second independent persisted visibility truth;
- group show/hide must create a normal bounded semantic command/transaction and remain Undo/Redo-correct;
- hidden Datum objects remain semantically present and may still be referenced; visibility is presentation state only.

Properties exposes normal user-facing identity/meaning:

- Datum Plane label/name;
- Constructor = Offset;
- semantic Source meaning;
- signed Offset;
- visibility;
- current evaluation status/diagnostic.

No provider/runtime token is shown as semantic identity. Tree location does not determine evaluation ownership or dependency order.

### 4.15 Persistence

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

### 4.16 Cold rebuild

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

## Architecture impact

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

## Public contract

The proposed user-visible contract is intentionally narrow:

- launch one **Datum Plane** tool whose PM-03 Constructor is `Offset`;
- create one Offset Datum Plane from an Origin plane, any uniquely resolved planar semantic Body Surface at an explicit Body stage, or an existing Datum Plane;
- receive immediate Extrude-style transient preview with a default new-draft Offset of 10 mm;
- edit signed Offset/source while preserving DatumId; Reverse is only a convenience sign flip;
- use GUI or Command Line through the same runtime draft/semantic command path;
- see a finite plane patch/border plus a presentation-only virtual plane/Body intersection overlay when the plane intersects the current Body;
- inspect Datum objects under the `Reference Geometry` tree group directly below `Origin`;
- show/hide individual Datum Planes or bulk-toggle the group through authored visibility commands;
- create/re-support Sketch on a resolved Datum Plane;
- use the resulting Profile with the existing Extrude Add/Cut workflow;
- receive explicit Missing/Ambiguous/Unsupported/Blocked failure instead of stale placement or guessed repair.

No Datum Axis/Point or Projection surface is promised by this contract.

## Failure behavior

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

## Proposed checkpoint sequence

### PM-03A — semantic Datum foundation + schema

**State:** COMPLETED — PASS; exact candidate `41ea901c873f1453e27b2b4973332ecb5295388c`, Windows FULL #1485, merged main `542d7b50fcd3f7566326f1b2c849d9453a3dcd00`. Completion evidence: `work/PM-03A_DATUM_FOUNDATION_SCHEMA_V10_COMPLETION.md`.

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

**State:** COMPLETED — PASS; exact candidate `e12f6c606132eadab5283372dee1a9f3f3008abd`, Windows FULL #1491, merged main `42b36d427bdb4c5c75b4961444576c7905032eb2`. Completion evidence: `work/PM-03B_DATUM_EVALUATION_COMPLETION.md`.

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

### PM-03C — commands + Extrude-style draft / Command Line parity

**State:** COMPLETED — PASS; C1 exact candidate `c633a9714602801e1253d119198ea9e46f73f9a9` / Windows FULL #1496 / merged `4147eba3697dfc345f49762508e92293b6373ff5`; C2 exact candidate `8b9185261e2cf32006f6af7a8c8a9da5e7c9ec04` / Windows FULL #1507 / merged `ad9e64df059091556bca2dc18515fa25a5cc5a32`. Completion evidence: `work/PM-03C_COMMANDS_DRAFT_COMMAND_LINE_COMPLETION.md`.

Deliver:

- Create/Edit/Delete/Show/Hide commands;
- one runtime draft shared by GUI/Command Line;
- single `Datum Plane` tool with `Offset` constructor;
- default new-draft Offset 10 mm;
- Reverse as signed-offset negation;
- transient preview;
- one Finish transaction;
- Undo/Redo.

Gate:

- GUI/Command Line semantic parity;
- Cancel/rejected Finish = zero mutation/history;
- edit preserves DatumId;
- Delete dependency rejection is atomic;
- stale draft cannot commit.

### PM-03D — Viewer / intersection overlay / Tree / Properties

**State:** COMPLETED — PASS; D1 exact candidate `b236e54028a64ba00569a995b592b4ab8de2cd17` / Windows FULL #1511 / merged `28c86f2cb62c5c9156cd2d1ca3f305432957eedf`; D2 exact candidate `adc9dd5c1f970932a03aa43422a105643db9052d` / Windows FULL #1514 / merged `3aa23632892853dbf9bf51574e5b2c762c2ac426`. Completion evidence: `work/PM-03D_VIEWER_TREE_PROPERTIES_COMPLETION.md`.

Deliver:

- finite Datum Plane patch + border presentation/pick binding;
- required presentation-only plane/current-Body intersection overlay when an intersection exists;
- `Reference Geometry` group directly below Origin;
- persistent per-Datum visibility and authored bulk group toggle;
- Properties/status;
- selection-first and command-first source workflows.

Gate:

- display size does not become authored plane size;
- intersection overlay never becomes Edge/Curve/reference/Projection authority;
- clicking overlay may select only the owning Datum Plane;
- group visibility introduces no second persisted visibility truth;
- stale presentation cannot commit;
- Viewer token never becomes CAD identity.

### PM-03E — Datum-backed Sketch + existing Extrude

**State:** COMPLETED — PASS; exact candidate `f4c691c50ce28bc311076b94925b719c889e8172`, Windows FULL #1520, merged main `66f31391894eb6a0adfb430d4dd78b37e749f150`. Completion evidence: `work/PM-03E_DATUM_BACKED_SKETCH_EXTRUDE_COMPLETION.md`.

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

**State:** COMPLETED — PASS

Closed:

- Save/Close/Reopen;
- true cold rebuild;
- integrated repair/failure matrix;
- internal as-built docs;
- PL/EN product docs;
- Product Browser regeneration;
- supported Windows Owner workflow.

Final Owner Windows result on 2026-10-05: **PASS**, with one explicit D2 acceptance amendment. The functional Datum workflow passed; the missing neutral translucent plane fill is accepted as a presentation-only defer to PM-06, where it must be implemented coherently for both Origin and Datum planes. Border/footprint presentation, virtual Body intersection, picking, semantics, lifecycle, persistence and downstream modeling remain accepted and are not deferred.

## Acceptance

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
- individual Show/Hide affects presentation only and persists as authored visibility;
- Reference Geometry group Show/Hide bulk-toggles child authored visibility with Undo/Redo and no second group visibility truth;
- default new-draft Offset is 10 mm and Reverse only negates the same signed value;
- GUI/Command Line use the same Datum Plane draft/semantic command;
- preview/Cancel/no-op produce no CAD history;
- plane footprint/border and plane/Body virtual intersection are presentation-only; the neutral translucent fill portion of the accepted presentation is explicitly deferred to PM-06 together with Origin-plane fill treatment;
- virtual intersection never becomes Edge/Curve semantics or Projection;
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

## Manual Windows acceptance themes

The final Owner workflow should cover:

1. create Base Body;
2. launch the single Datum Plane tool and verify Constructor = Offset, default Offset = 10 mm and immediate Extrude-style preview after valid source acquisition;
3. create Datum Plane offset from an Origin plane; use Reverse and verify it is equivalent to changing the signed Offset;
4. repeat the same draft through Command Line and verify GUI/Command Line parity;
5. edit offset positive/negative/zero and verify deterministic orientation;
6. create Datum Plane from a planar Body Face and verify the persisted source is the semantic planar Surface, not bounded Face/provider identity;
7. verify the finite plane border/footprint and virtual plane/Body intersection overlay clearly locate the plane; clicking the overlay selects Datum Plane, never an Edge. The neutral translucent fill is an Owner-accepted presentation-only defer to PM-06, paired with Origin-plane fill treatment;
8. create a second Datum Plane from the first Datum Plane;
9. inspect `Reference Geometry` directly below Origin; verify individual Show/Hide and bulk group Show/Hide with Undo/Redo;
10. inspect Properties/status;
11. create Sketch on Datum Plane;
12. author Profile and Extrude Add/Cut;
13. edit upstream Body dimension so Body-Surface-backed Datum moves;
14. edit Datum offset and verify Sketch local geometry remains unchanged;
15. exercise source Missing/Ambiguous and verify no stale modeled result;
16. repair source/re-support;
17. verify cycle-causing re-reference rejects with zero mutation;
18. Undo/Redo;
19. Save, close, reopen and verify cold reconstruction.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: PM-03 introduces durable Datum identity/state, new native persistence, Datum Plane creation/editing/visibility, Datum-backed Sketch support and new failure/repair behavior.

Before completion:

- internal Part/persistence/viewer docs describe as-built Datum ownership/evaluation and the accepted presentation-only fill defer;
- PL/EN product docs describe Offset Datum Plane and Sketch-on-Datum workflow truthfully for the current unfilled plane presentation;
- Product Browser is regenerated from canonical Markdown;
- deferred Datum Axis/Point must not be documented as implemented.

## STOP conditions

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

## Activation and completion boundary

The Owner accepted the PM-03 D2 scope and the UI/interaction amendments materialized in this contract on 2026-10-05: Offset-Datum-Plane-only scope, single Datum Plane tool, 10 mm default Extrude-style preview, Command Line parity, required presentation-only plane/Body intersection overlay, and Reference Geometry tree grouping/visibility semantics.

This contract is activated only by the governance change that points `work/ACTIVE.yaml` here with `status: active` and passes its repository gate. Production mutation is legal only after that activation change is merged.

PM-03A through PM-03F completed in order. Owner final Windows acceptance on 2026-10-05 closes this contract with the explicit presentation-only translucent-fill defer recorded above. PM-03 production mutation authority is now closed.

PM-03 completion authorizes only the delivered Offset Datum Plane / Datum-backed Sketch vertical slice. No Datum semantic, lifecycle, persistence, repair or downstream-modeling obligation is deferred.

PM-04 Axis / Revolve and Projection remain separately gated. The Origin/Datum translucent-fill presentation polish is tracked as a PM-06 final Part-v1 obligation and does not activate PM-04 or any other production package.
