# ADR-0016 — Body Semantic Topology, Carrier Geometry and Face-Supported Sketch

**Status:** ACCEPTED  
**Proposed:** 2026-10-03  
**Owner acceptance:** 2026-10-03  
**Decision class:** D2 Part architecture / product sequencing amendment  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.6 — PM-02P / PM-02  
**Amends:** ADR-0005 §4 SketchPlacement semantics; ADR-0014 §4 planar-face support boundary; ADR-0015 §8 PM-02 sequencing  
**Preserves:** ADR-0014 provider-neutral identity, stage-scoped resolution, fail-closed behavior, freshness and numerical policy  
**Explicitly excludes:** Projection / Project Edge / face-boundary capture

## Context

PM-01 proved the first production solid-modeling path:

`Profile -> ordered Extrude Add/Cut Features -> one Body`

and introduced semantic face lineage for Extrude caps and sides.

The previously planned next step was a bounded Origin-based Offset Datum Plane followed later by model-face Sketch support. Owner review rejected that sequencing because an Origin-only Datum implementation would establish a temporary reference/support architecture before Body topology, face support, edges and vertices were available. Full Datum semantics, Revolve, Fillet and Chamfer all ultimately depend on stable Body references.

The accepted direction is therefore to establish the semantic Body-topology foundation first. The first product consumer of that foundation is Sketch support on arbitrary planar Body faces, including planar lateral faces. Projection is independent and is not part of this decision.

## Decision

### 1. PM-02 becomes the semantic Body-topology package

PM-02 is no longer an Origin Offset Datum package.

Its primary architecture/product scenario is:

```text
existing Sketch/Profile
    -> Extrude Add/Cut
    -> Body
    -> semantic Face / Edge / Vertex topology
    -> select arbitrary planar Body Face
    -> Sketch
    -> Profile
    -> existing Extrude Add/Cut
    -> upstream edit
    -> semantic re-resolution
    -> recompute or structured failure
```

Datum reference geometry moves after this foundation. Projection remains a separately owned later concern.

### 2. Complete topology accounting

Every successfully evaluated Body stage has a complete transient provider-neutral topology catalog.

Conceptually:

```text
BodyStageTopologyCatalog
    faces[]
    edges[]
    vertices[]
    surfaces[]
    curves[]
    points[]
```

For every Face, Edge and Vertex present in the current B-Rep at that stage, exactly one catalog entry must exist. A provider topology item may not silently exist outside Part's topology accounting.

Complete accounting does not mean that every provider artifact automatically receives durable authored identity. Every item must instead be explicitly classified as semantically referenceable, semantically known but not durably referenceable, unsupported semantic meaning, or integrity failure. Provider seams and other representation artifacts are therefore visible to the semantic layer without being falsely promoted to authored design intent. Silent omission is forbidden.

### 3. Topology and carrier geometry are distinct concepts

SS2 distinguishes bounded B-Rep topology from the semantic geometric carrier on which that topology lies:

```text
Face   -> Surface
Edge   -> Curve
Vertex -> Point
```

A Face is a bounded material-boundary fragment. A Surface is the semantic geometric carrier from which one or more Faces may currently be realized. Likewise, Edge/Curve and Vertex/Point are not interchangeable identity concepts.

This distinction is required because Boolean operations may trim or split topology without changing its underlying semantic carrier.

### 4. Durable references remain semantic selectors

PM-02 does not introduce durable provider topology numbers such as `Face[17]` or `Edge[42]`.

Durable references are structural semantic selectors derived from design provenance. Their meaning includes at minimum:

- semantic producer;
- source role/provenance;
- declared Body stage;
- reference kind.

Exact persisted type layout is owned by the production PM-02 implementation after PM-02P evidence, but it must not contain OCCT handles, topology ordinals, Viewer tokens, mesh indices or geometry-proximity identity.

### 5. Body stage is part of reference meaning

Topology and carrier references resolve in an explicit Body stage.

Conceptually:

```text
BodyStageRef =
    EmptyBody
  | AfterFeature(FeatureId)
```

A downstream input is resolved in its declared semantic stage, never by globally searching the final Body.

### 6. Surface lineage

Every supported modeling operation must account for how semantic surfaces propagate.

For the accepted Extrude Add/Cut universe:

- Extrude creates semantic cap and side surfaces from Profile provenance;
- Boolean operations may preserve, trim, split or remove existing surfaces;
- Boolean operations may expose tool surfaces as new material boundaries;
- Boolean trimming does not by itself invent a new unrelated semantic surface;
- descendants retain the semantic carrier of their accepted source unless the owning operation explicitly defines a new carrier meaning.

Geometry similarity alone never establishes lineage. Provider Generated/Modified/Deleted history is transient evidence, not durable authority.

### 7. Strict Face and Surface references have different split behavior

A strict bounded Face reference represents a particular topological realization:

- zero semantic candidates -> Missing;
- one semantic candidate -> Resolved;
- more than one candidate -> Ambiguous;
- undeclared meaning -> Unsupported.

A Surface reference represents the semantic carrier.

If one previously bounded Face is split into several current Faces that all unambiguously realize the same semantic Surface, the Surface may remain Resolved while the strict Face reference becomes Ambiguous.

### 8. Picking and Sketch support use different identity levels

The user selects a bounded Face in the Viewer. Selection resolves:

```text
runtime picked Face
    -> Face catalog entry
    -> semantic Surface carrier
```

For Sketch creation on a planar Body face, the selected bounded Face is the admission gesture. The durable Sketch support is a semantic planar-surface attachment rather than a provider Face handle or topology ordinal.

Picking two fragments produced by splitting the same semantic planar surface intentionally selects the same support plane when their semantic side sense is the same.

### 9. All Body Faces are accounted; all planar semantic Faces are eligible for Sketch support

PM-02 must not special-case Extrude top caps.

Within the topology generated by the accepted Extrude Add/Cut feature set:

- both cap faces are handled;
- all planar lateral faces are handled;
- planar faces preserved through Boolean operations are handled;
- planar faces exposed from Cut tool surfaces are handled;
- split/trimmed planar realizations are handled through their semantic carrier;
- non-planar faces remain fully catalogued and selectable.

Sketch admission is based on resolved carrier geometry, not Face position or Feature role.

A planar Face is Sketch-support admissible. Cylindrical and other non-planar Faces remain catalogued/referenceable but Sketch support is explicitly unsupported.

### 10. Carrier geometry classification

The provider-neutral topology catalog classifies carrier geometry sufficiently for semantic consumers.

Surface classification includes at least Plane, Cylinder, Cone, Sphere, Torus and Other. Curve classification includes exact classes required by supported geometry, without approximating unsupported curves.

Classification is derived semantic/runtime data. It is not itself durable identity.

### 11. Deterministic planar-surface frame

Every accepted planar Sketch support resolves a deterministic right-handed metric frame `O/U/V/N`, with `N = U x V`.

The frame is derived from semantic provenance. It must not be derived from camera, Viewer orientation, arbitrary provider UV axes, first current edge, topology traversal order, face centroid or bounding-box orientation.

For an Extrude cap, the source Profile/support frame and semantic cap role provide the frame provenance. For a planar Extrude side generated from a straight Profile boundary, the authored boundary direction and Extrude semantic direction provide the frame provenance. Boolean trimming preserves the carrier frame.

Exact orientation and side-sense rules must be evidenced before production PM-02 activation.

### 12. Sketch support frame becomes derived state

ADR-0005 §4 is amended.

`SketchPlacement` is no longer an independent durable world-space authority for supports whose frame is semantically resolvable.

The durable Sketch owns its SketchId, SupportReference, visibility and authored local U/V Sketch geometry. The world-space frame is derived from the current resolved support.

This prevents two conflicting persistent truths and prevents hidden document mutation merely to move a supported Sketch after upstream recompute.

### 13. Re-support semantics

ADR-0014 remains authoritative: re-support preserves authored local U/V geometry by default and changes the host mapping.

Re-support preserves SketchId, EntityId, ProfileId and local U/V geometry. Missing, Ambiguous or unsupported targets fail without partial mutation.

### 14. Dependency and cycle rule

A Sketch supported by a Body stage may only be consumed by Features semantically downstream of that support stage.

A command that would create a dependency cycle is rejected before mutation. The bounded rule may use ordered Body Feature history plus explicit Sketch/Profile consumption relationships; a global universal dependency graph is not required.

### 15. Evaluation becomes support-aware and stage-aware

Face-supported Sketches require Profile-to-Kernel conversion to use the resolved upstream modeling context:

```text
evaluate stage N
    -> BodyStageTopologyCatalog(N)
    -> resolve Sketch supports declared at N
    -> ResolvedSketchFrame
    -> resolve Profile world geometry
    -> evaluate consuming Feature N+1
```

Authored local Sketch geometry remains provider-independent. A downstream operation never consumes a stale last-good support frame after the current upstream stage becomes unavailable.

### 16. Edge and Vertex semantics

Every B-Rep Edge and Vertex is accounted for.

Material Edge/Curve provenance must be semantic where SS2 can defend the meaning, including cap/side boundaries, side/side boundaries, Boolean intersection edges and inherited lineage. Provider order, length ranking, XYZ sorting, nearest geometry or first result are forbidden identity authorities.

Vertex/Point meaning must arise from semantic provenance such as curve endpoint roles, semantic curve intersections or inherited/generated lineage. XYZ coordinates are diagnostics, never durable identity.

Periodic seams and similar provider representation artifacts remain accounted but are not automatically promoted to authored material-edge semantics.

### 17. Runtime tokens and Viewer selection

Provider/runtime Face/Edge/Vertex tokens are transient only. They may bind current provider objects to the current topology catalog but are never serialized or treated as stable CAD identity.

Viewer selection uses runtime pick identity only long enough to reach the Part semantic resolver. Stale DocumentRevision/session/evaluation-generation selection cannot commit.

### 18. Complete-accounting integrity

For every successfully evaluated Body stage, provider Face/Edge/Vertex counts must equal catalogued counts under consistent B-Rep duplicate semantics.

An unexplained current topology object is not silently ignored. Conflicting semantic claims are not arbitrarily resolved. A false Resolved reference is an integrity failure.

### 19. Persistence

The topology catalog, runtime provider tokens and evaluated frames are never persisted.

Production PM-02 may introduce the next Part schema version only after PM-02P evidence and a separately accepted production Work Contract.

Migration from the current Origin-plane Sketch representation must verify old support/placement coherence, preserve stable authored IDs/local geometry and remove redundant authored world placement.

### 20. Product boundary and sequencing

PM-02 owns complete Body Face/Edge/Vertex accounting, semantic Surface/Curve/Point carrier foundations, topology picking, deterministic planar carrier frames and face-supported Sketch lifecycle.

PM-02 does not own Projection, Datum creation, Revolve, Fillet, Chamfer, multi-body or a global dependency graph.

Program sequencing becomes:

```text
PM-01  Extrude Add/Cut — COMPLETED
PM-02P Body Semantic Topology Evidence Gate
PM-02  Body Semantic Topology + Face-Supported Sketch
PM-03  Datum Reference Geometry
PM-04  Axis / Revolve and remaining reference geometry required by Revolve
PM-05  Edge Features: Fillet / Chamfer
PM-06  Part v1 Completion
```

Projection remains a Part-v1 requirement but is outside the PM-02/Datum critical path and requires separate later activation.

## Consequences

Positive:

- Datum begins on a real reference architecture rather than an Origin-only special case;
- planar side faces are first-class Sketch supports;
- topology splits can be treated differently from carrier-surface continuity;
- Sketch world placement follows semantic support without hidden document mutation;
- Edge/Vertex semantics are established before later tools depend on them;
- runtime OCCT representation remains replaceable and non-authoritative;
- Projection remains independent.

Costs:

- PM-02 becomes a substantial architecture/product package;
- evaluator, provider and Viewer selection boundaries require expansion;
- ADR-0005 placement semantics require migration;
- Edge and Vertex provenance after Boolean operations require significant evidence;
- topology and carrier geometry become deliberately distinct reference layers.

## Rejected directions

- Origin Offset Datum as the next implementation while model topology is deferred;
- top-cap-only Sketch support;
- handling caps but not planar lateral Faces;
- using current Face boundary to derive Sketch orientation;
- persisting OCCT Face/Edge/Vertex handles;
- durable Face/Edge ordinals;
- geometry proximity as automatic rebinding;
- treating one Face split into two as automatic failure of a carrier-Surface Sketch support;
- making every OCCT seam an authored design Edge;
- maintaining authored absolute SketchPlacement as a second authority beside dynamic support;
- silently moving authored SketchPlacement during recompute;
- solving Projection as part of face-supported Sketch;
- implementing a universal dependency graph as a prerequisite.
