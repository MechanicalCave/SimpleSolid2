# PM-02P — Body Semantic Topology Evidence Work Contract

**Status:** ACTIVE — OWNER ACCEPTED 2026-10-03  
**Decision class:** D2 bounded architecture-evidence Work Contract; D0/D1 implementation permitted only inside this contract  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.6  
**Architecture authority:** ADR-0014 + ADR-0016  
**Entry gate:** PM-01 COMPLETED — PASS  
**Owner acceptance:** 2026-10-03  
**Production CAD mutation:** NOT AUTHORIZED  
**Persistence schema mutation:** NOT AUTHORIZED  
**User-facing topology/Sketch feature:** NOT AUTHORIZED

## 1. Goal

Prove the semantic-topology architecture required by production PM-02 before durable data structures, persistence schema or user-facing Body topology selection are introduced.

PM-02P must provide sufficient real OCCT evidence to freeze:

```text
Body stage
    -> complete Face / Edge / Vertex accounting
    -> semantic Surface / Curve / Point carriers
    -> stable lineage and fail-closed resolution
    -> deterministic planar carrier frames
    -> prospective face-supported Sketch semantics
```

The evidence must support the accepted future product sequence:

```text
Body Semantic Topology
-> Face-Supported Sketch
-> Datum Reference Geometry
-> Revolve / Fillet / Chamfer consumers
```

Projection is unrelated and remains outside this Work Contract.

## 2. Activation rule

Owner explicitly accepted this Work Contract on 2026-10-03.

Activation authorizes bounded evidence/probe implementation only. It does not activate production PM-02.

Completion of PM-02P likewise does not automatically authorize production mutation. After PM-02P PASS, a separate production Work Contract must be proposed and explicitly accepted.

## 3. Governing architecture

Implementation must preserve:

- one semantic owner for every concept;
- provider-neutral durable design intent;
- OCCT identity as transient evidence only;
- stage-scoped reference meaning;
- zero automatic rebinding from geometry similarity alone;
- Missing / Resolved / Ambiguous / Unsupported fail-closed semantics;
- no stale last-good geometry as current Body truth;
- Viewer and presentation identity never becoming Part identity;
- no global universal dependency framework as a prerequisite.

ADR-0016 additionally requires:

```text
Topology != Carrier Geometry

Face   -> Surface
Edge   -> Curve
Vertex -> Point
```

and complete topology accounting.

## 4. Scope IN

PM-02P may implement only evidence necessary to answer the accepted PM-02 architecture questions.

### 4.1 Complete topology inventory

For every successful probe Body stage, all provider Faces, Edges and Vertices must be enumerated and matched one-to-one with evidence catalog records.

Required invariant:

```text
provider Faces    == catalog Faces
provider Edges    == catalog Edges
provider Vertices == catalog Vertices
```

No unexplained topology may be silently dropped.

### 4.2 Semantic accounting classes

Evidence records may classify topology as:

- Referenceable;
- KnownRepresentationArtifact;
- SemanticallyUnsupported;
- IntegrityFailure.

KnownRepresentationArtifact is explicitly permitted for provider/B-Rep constructs such as periodic seams.

SemanticallyUnsupported is permitted only when the topology is not required to be durable-referenceable by accepted PM-02 product scope.

IntegrityFailure blocks PASS.

### 4.3 Carrier geometry probes

Evidence may prototype provider-neutral SurfaceCarrier, CurveCarrier and PointCarrier evidence types and transient Face->Surface, Edge->Curve, Vertex->Point relationships.

These types are evidence-only unless separately promoted by the later production Work Contract.

### 4.4 Face lineage

PM-02P may extend existing PM-01 face lineage probes to cover generated, inherited, modified/trimmed, split, deleted, semantic claim alias/merge and Boolean tool-exposed Faces.

The current PM-01 production Face lineage may be consumed as evidence but must not be silently redefined.

### 4.5 Edge lineage

Evidence may introduce runtime/test-only Edge tokens and semantic Edge/Curve provenance required to study:

- cap/side boundaries;
- side/side boundaries;
- Boolean intersection Edges;
- inherited Edges;
- trimmed Edges;
- split Edges;
- deleted Edges;
- periodic seams;
- multiple intersection branches.

No durable EdgeId or serialized Edge selector is authorized.

### 4.6 Vertex lineage

Evidence may introduce runtime/test-only Vertex tokens and semantic Point/Vertex provenance required to study:

- curve endpoint roles;
- semantic Edge intersections;
- stable moved Vertices;
- deleted Vertices;
- same-XYZ semantic replacements;
- generated Boolean Vertices.

No durable VertexId or serialized Vertex selector is authorized.

### 4.7 Carrier classification

Evidence may classify runtime carrier geometry including at least:

```text
Surface:
    Plane
    Cylinder
    Cone
    Sphere
    Torus
    Other

Curve:
    Line
    Circle
    supported exact curve classes
    Other
```

Classification is evidence/runtime information and must not become identity by itself.

### 4.8 Deterministic planar frames

Evidence may prototype canonical O/U/V/N frames for:

- Extrude caps;
- planar lateral Surfaces produced from straight Profile boundaries;
- planar Surfaces exposed by Cut;
- inherited/trimmed planar carriers.

Frame evidence must be independent of camera, Viewer state, tessellation, TopoDS traversal order, first current Edge, raw OCCT UV orientation, current Face centroid and bounding-box orientation.

### 4.9 Prospective Sketch-support simulation

Evidence may simulate:

```text
semantic planar Surface
-> resolved carrier frame
-> local Sketch U/V geometry
-> world geometry
```

to prove that upstream carrier motion changes evaluated world placement without rewriting authored Sketch geometry.

No actual production Sketch support schema is authorized.

### 4.10 Bounded cycle probe

Evidence may use current ordered Feature and Profile/Sketch relationships to prove rejection of a support dependency that would reference geometry produced downstream of its own consuming Feature.

No general dependency graph may be introduced.

## 5. Scope OUT

PM-02P must not implement:

- production BodyTopologyCatalog public API;
- production Face/Edge/Vertex selection;
- production Viewer topology picking;
- persistent Surface/Curve/Point selectors;
- schema v9;
- persistent face-supported Sketch;
- Sketch re-support UI;
- Datum Plane / Axis / Point;
- Projection / Project Edge / Reproject / Refresh;
- Revolve;
- Fillet;
- Chamfer;
- multi-body;
- Assembly;
- global dependency graph;
- public APIs invented only for hypothetical future operations.

PM-02P must not modify existing durable CAD meaning merely because a probe is easier to implement that way.

## 6. Permitted repository areas

After activation, implementation may mutate only the following bounded areas unless this Work Contract is explicitly amended.

### 6.1 Evidence/tests

Permitted:

```text
tests/kernel_native/**
tests/core/**
```

where required by exact evidence ownership.

New PM-02P-specific evidence files use a clear `pm02p_*` prefix where practical.

Existing production regression tests may be extended only where the evidence directly exercises already-existing production behavior. Tests may not be weakened or deleted to obtain PASS.

### 6.2 Kernel provider evidence

Permitted bounded modifications:

```text
src/kernel/**
src/kernel_occt/**
```

only for runtime/evidence topology introspection, provider-neutral transient probe contracts, Edge/Vertex history evidence, exact carrier classification, deterministic-frame evidence and test support required by PM-02P.

Any modification that creates a durable/public production contract beyond the evidence requirement is STOP for Owner review.

### 6.3 Part evidence

Permitted bounded modifications:

```text
src/part/**
```

only when required to reuse existing Feature stage/provenance semantics, construct test-only semantic addresses, evaluate existing PM-01 lineage or prototype stage-aware support resolution without persistent authored mutation.

No Part schema or persisted authored type may be changed.

### 6.4 Build/test registration

Permitted:

```text
CMakeLists.txt
tests/**/CMakeLists.txt
cmake/**
scripts/verification-related files
```

only as required to register evidence in the existing verification topology.

PM-02P must reuse the existing FOCUSED / FAST / SUBSYSTEM / FULL model. No new public test tier is authorized.

### 6.5 Governance/evidence documents

Permitted:

```text
work/**
adr/**
docs/internal/**
```

only for accepted ADR-0016 synchronization, PM-02P evidence matrix, evidence findings, exact-head results, final synthesis and activation/completion state.

No Product documentation is required for evidence-only behavior because no product behavior is introduced.

## 7. Explicitly forbidden areas without amendment

PM-02P must not require user-facing changes in:

```text
src/ui/**
src/viewer/**
docs/product/**
```

unless evidence demonstrates that topology picking itself must be studied to answer an architecture question. If that dependency appears, STOP.

Do not extend PM-02P into a desktop feature implementation.

## 8. Evidence matrix freeze

Before implementation of individual evidence cases, `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` is authoritative for expected outcomes.

Every case declares its expected semantic outcome before observing provider behavior.

Allowed expected outcomes:

```text
MUST_RESOLVE
MUST_MISSING
MUST_AMBIGUOUS
MUST_UNSUPPORTED
REPRESENTATION_ARTIFACT
```

Expected outcomes may not be retroactively changed merely because OCCT returned a different topology. A changed expectation requires architectural reasoning and, when D2, Owner review.

## 9. Mandatory evidence areas

PM-02P must close E01-E25, covering at minimum:

```text
E01  rectangular Extrude complete topology
E02  arbitrary planar lateral frame
E03  circular Extrude + seam
E04  Line + Arc carrier provenance
E05  Profile with hole
E06  attached Add trimming
E07  Cut trimming planar Surface
E08  one Face -> multiple Faces, carrier survives
E09  Surface deletion
E10  semantic Face claim alias/merge

E11  pristine Extrude Edge ontology
E12  Edge inheritance/trim/delete/split
E13  multiple intersection branches
E14  seam Edge classification
E15  Vertex ontology
E16  Vertex move/delete/replacement

E17  deterministic cap frames
E18  deterministic planar lateral frames
E19  Cut-exposed planar frame
E20  complete accounting after every Feature stage

E21  cold rebuild
E22  stale/runtime-token isolation
E23  geometry-similarity trap
E24  prospective dynamic Sketch support
E25  bounded dependency-cycle rejection
```

Individual scenarios may share one compiled harness where that reduces build cost without reducing independent test reporting.

## 10. Evidence API discipline

Probe code must prefer explicit evidence types over prematurely final production names.

Acceptable examples:

```text
EvidenceFaceRecord
EvidenceEdgeRecord
EvidenceVertexRecord

EvidenceSurfaceCarrier
EvidenceCurveCarrier
EvidencePointCarrier
```

Do not create a production-looking API and let downstream production code depend on it before evidence acceptance.

Promotion from evidence type to production contract belongs to the production PM-02 Work Contract.

## 11. Runtime identity rules

PM-02P may introduce runtime-only Face/Edge/Vertex tokens or equivalent bounded probe forms.

These must never be serialized, treated as stable across evaluation generation, compared as durable semantic identity or carried across cold rebuild as identity.

Token numerical equality after rebuild has no semantic meaning.

## 12. Edge/Vertex decision boundary

PM-02P is explicitly authorized to discover that the accepted ontology needs additional semantic structure such as intersection branch role, curve endpoint role, surface-side role, periodic seam role or topological realization discriminator.

Such findings may be proposed but may not be silently frozen into production durable semantics.

If an additional discriminator changes public/durable identity meaning, STOP for Owner D2 review.

## 13. Frame decision boundary

Evidence must establish exact algebraic frame rules.

A PASS requires enough information to state production rules without reference to OCCT implementation convenience.

For each accepted planar carrier class the report must define:

```text
Origin rule
+U rule
+V rule
+N rule
side-sense rule
behavior after trim
behavior after split
behavior after cold rebuild
```

If no deterministic semantic frame can be defended for a required planar Face class, PM-02P fails.

Using arbitrary current topology as fallback is forbidden.

## 14. Verification strategy

Development verification:

```text
FOCUSED
-> exact PM-02P evidence case(s)
```

Checkpoint verification uses SUBSYSTEM kernel or SUBSYSTEM part,kernel depending on touched ownership.

FAST must remain green where appropriate.

Final candidate must pass FULL on the same exact source revision used for the PM-02P synthesis.

Kernel-native Release evidence is mandatory. Desktop FULL must remain regression-clean even though PM-02P introduces no user-facing desktop feature.

## 15. Checkpoints

### PM-02P.A — matrix and harness

Deliver frozen E01-E25 expected-outcome matrix, bounded topology inventory harness, independent Face/Edge/Vertex provider enumeration and catalog coverage assertions.

Gate: simple Extrude complete accounting works with no production API/schema introduced.

### PM-02P.B — Face/Surface lineage

Close E01-E10 and Face/Surface-dependent E17-E19.

Gate: zero false Resolved; planar Surface/Face distinction proven; carrier frame survives trim/split as specified.

### PM-02P.C — Edge/Curve lineage

Close E11-E14.

Gate: all accepted Extrude Add/Cut Edges accounted; material-vs-seam distinction proven; split/branch behavior explicit.

### PM-02P.D — Vertex/Point lineage

Close E15-E16.

Gate: all accepted Extrude Add/Cut Vertices accounted; same-XYZ replacement does not steal identity.

### PM-02P.E — lifecycle/freshness

Close E20-E25.

Gate: stage accounting, cold rebuild, stale isolation, similarity trap, dynamic-support simulation and cycle rejection.

### PM-02P.F — synthesis

Produce:

```text
work/PM-02P_TOPOLOGY_EVIDENCE_REPORT.md
work/PM-02P_REFERENCE_SURVIVAL_MATRIX.md
```

and an Owner-reviewable production architecture recommendation.

No schema/API production implementation begins in this checkpoint.

## 16. Required metrics

Every final evidence report states:

- false Resolved count;
- unexpected Missing count;
- unexpected Ambiguous count;
- unexpected Unsupported count;
- unaccounted Face count;
- unaccounted Edge count;
- unaccounted Vertex count;
- frame instability count;
- cold-rebuild semantic mismatch count;
- stale publication count.

PASS requires:

```text
false Resolved = 0
unaccounted Faces = 0
unaccounted Edges = 0
unaccounted Vertices = 0
frame instability = 0
cold-rebuild semantic mismatches = 0
stale publications = 0
```

Cases declared MUST_RESOLVE must resolve.

## 17. STOP conditions

STOP immediately for Owner review if evidence requires or suggests:

- persistent OCCT/OCAF identity;
- topology ordinal identity;
- geometry proximity/similarity as automatic identity;
- arbitrary candidate selection;
- silent omission of topology;
- raw provider UV as Sketch frame authority;
- first-edge orientation fallback;
- authored absolute SketchPlacement mutation during recompute;
- production schema mutation;
- user-facing topology feature implementation;
- global dependency graph;
- multi-body prerequisite;
- Projection as prerequisite;
- Foundation ownership change.

STOP also if an accepted PM-02 product case cannot be represented by ADR-0016 without changing its D2 identity/reference model.

## 18. Documentation Impact

Classification:

```text
Internal architecture/evidence documentation: YES
Product documentation: NO
Product Browser update: NO unless canonical product documentation changes
```

Required before completion:

- accepted ADR-0016 synchronized;
- PM-02P evidence matrix current;
- evidence report current;
- reference survival matrix current;
- roadmap/ACTIVE state current.

If PM-02P unexpectedly changes user-visible behavior, STOP and reassess Documentation Impact before continuing.

## 19. Completion gate

PM-02P may be marked COMPLETED — PASS only when:

- E01-E25 required evidence is complete;
- all complete-accounting invariants pass;
- false Resolved count is zero;
- all declared stable cases resolve;
- frame rules are deterministic and expressible provider-neutrally;
- Edge/Vertex semantics required by current Extrude Add/Cut topology are understood;
- cold rebuild produces the same semantic results;
- all runtime-token/stale-generation tests pass;
- exact-head FULL passes;
- evidence documents are synchronized;
- no unresolved D2 question remains hidden inside the proposed production architecture.

## 20. Completion consequence

PM-02P PASS authorizes only preparation of the production PM-02 Work Contract:

`PM-02 — Body Semantic Topology and Face-Supported Sketch`.

That later contract may propose production topology catalog, provider-neutral semantic selectors, runtime topology picking, deterministic planar Surface frames, schema migration, face-supported Sketch, re-support, stage-aware Profile evaluation, GUI/CLI semantics, lifecycle/persistence/docs and Windows manual acceptance.

No such product item becomes authorized merely because PM-02P passes.
