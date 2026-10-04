# PM-02P — Body Semantic Topology Evidence Report

**Status:** COMPLETION CANDIDATE — final exact-head FULL pending  
**Date:** 2026-10-04  
**Program:** Part Modeling v1 roadmap v1.6  
**Architecture authority:** ADR-0014 + ADR-0016  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Production PM-02:** NOT ACTIVE

## 1. Executive conclusion

PM-02P evidence supports the architecture accepted in ADR-0016.

Within the currently accepted modeling universe:

```text
ordered single Body
+ Extrude Add/Cut
+ planar / cylindrical Extrude carriers
+ current OCCT Boolean history
```

SS2 can build a provider-neutral semantic topology foundation with:

- complete Face / Edge / Vertex accounting at every successful Body stage;
- explicit Face->Surface, Edge->Curve and Vertex->Point semantic layers;
- deterministic planar carrier frames derived from authored provenance;
- fail-closed split / merge / delete semantics;
- no geometry-similarity rebinding;
- cold reconstruction independent of previous provider state;
- stale runtime-token rejection;
- dynamic planar-Surface Sketch support without a second authored world-placement authority;
- bounded ordered-history cycle prevention without a universal dependency graph.

No evidence requires durable provider handles, Face/Edge/Vertex ordinals, first-result rules, nearest geometry or persistent evaluated topology.

The evidence therefore supports proceeding to a separately accepted production PM-02 Work Contract after PM-02P closure.

This report does **not** activate production PM-02.

## 2. Evidence checkpoints

### PM-02P.A — complete topology inventory

Windows FULL #1376, exact source:

`0c460b2d97551546b9a8e61b72efecd11933bf8e`

Merged main:

`9e43ac5cb5f4c941b435672c5bc77d98bb237d60`

Proved the canonical rectangular Extrude baseline:

```text
Faces    6 / 6 accounted
Edges   12 / 12 accounted
Vertices 8 / 8 accounted
```

### PM-02P.B1 — pristine Surface carriers

Windows FULL #1378, exact source:

`dff5d6c3532358bbb4bfa11fb17d5bb1113ecb54`

Merged main:

`6b7b967b5ee40ed87c48efba1607b0806b88524d`

Proved complete semantic Face->Surface claims for caps, planar line sides, cylindrical Arc/Circle sides and hole boundaries, with deterministic XY/XZ/YZ planar frames.

### PM-02P.B2 — Boolean Surface lineage

Windows FULL #1379, exact source:

`24a65c79c40061b7185f49463fe7a7838e1117f4`

Merged main:

`5536e9be1c3da3b3e8f93c4dae229856e8221001`

Proved trim / split / delete / recreate / alias behavior and Cut-exposed Surface provenance.

### PM-02P.B3 — cap roles and frames

Windows FULL #1380, exact source:

`63a577c455b05943a79a5ad7b649088da856b3c8`

Merged main:

`a2db078d01cbb1dccc7112fd22a6083e0266e3e8`

Proved OneSide Forward / Reverse and Midplane semantic cap roles and canonical frames for XY/XZ/YZ supports.

### PM-02P.C — Edge / Curve lineage

Windows FULL #1382, exact source:

`c19efc5d45f1489d433fd75d25f4e9035737f91b`

Merged main:

`484542d598f621dc17a9cea60fd068cf5ab62408`

Proved pristine material Edge ontology, trim / split / delete lineage, Boolean-created intersection Edges, multiple-branch ambiguity and seam representation-artifact handling.

### PM-02P.D — Vertex / Point lineage

Windows FULL #1383, exact source:

`b88cbfd72e6fd8d736358156bbfa433612aa5378`

Merged main:

`8b27e1aaa2060c8e3d3c1bdeca62e7370d721e80`

Proved all eight prism Vertices, semantic movement after upstream dimension edit, deletion, generated-new-Vertex provenance and same-XYZ replacement rejection.

### PM-02P.E — lifecycle / freshness

Windows FULL #1387, exact source:

`e259fef5849bc4707ddcbdda8e75795b8f3fa5b4`

Merged main:

`a0dad01dd584eb1b1c39a4cb282b0bbc009000aa`

Proved stage-complete accounting, cold replay, runtime freshness isolation, geometry-similarity traps, prospective dynamic Surface-backed Sketch movement and bounded cycle rejection.

## 3. Frozen-oracle coverage

| Matrix | Evidence conclusion |
| --- | --- |
| E01 rectangular Extrude | PASS — 6 Face / 12 Edge / 8 Vertex complete accounting |
| E02 arbitrary planar lateral frame | PASS — deterministic XY/XZ/YZ lateral frames |
| E03 circular Extrude / seam | PASS — cylinder carrier Resolved; seam accounted as representation artifact |
| E04 mixed Line + Arc | PASS — Plane/Cylinder side provenance; traversal-stable semantics |
| E05 Profile with hole | PASS — outer/inner carrier provenance distinct |
| E06 attached Add trim | PASS — Surface stays Resolved |
| E07 Cut trim | PASS — Surface/frame stay Resolved |
| E08 Face split | PASS — strict Face Ambiguous; Surface Resolved |
| E09 delete + identical replacement | PASS — old Surface Missing; replacement distinct |
| E10 alias / merge | PASS — no provider-history winner without semantic authority |
| E11 pristine Edge ontology | PASS — 12 material Edges semantically accounted |
| E12 Edge lineage | PASS — unchanged/trim Resolved, delete Missing, split Ambiguous |
| E13 multiple intersection branches | PASS — carrier pair alone Ambiguous |
| E14 seam Edge | PASS — representation artifact / ordinary Edge Unsupported |
| E15 Vertex ontology | PASS — 8/8 semantic Vertices |
| E16 Vertex lifecycle | PASS — moved Resolved, deleted Missing, replacement distinct |
| E17 cap frames | PASS — Forward/Reverse/Midplane, all Origin support planes |
| E18 lateral frame | PASS — authored Line provenance; no traversal/UV authority |
| E19 Cut-exposed carrier | PASS — tool-derived planar Surface/frame Resolved |
| E20 every Feature stage | PASS — every Add/Add/Cut/Cut stage complete |
| E21 cold rebuild | PASS — semantic evidence reproduces without retained provider state |
| E22 stale runtime isolation | PASS — revision/session/evaluation/provider stale evidence rejected |
| E23 geometry similarity | PASS — plane/radius/length/XYZ/class do not rebind identity |
| E24 dynamic Sketch support | PASS — local UV stable, derived world frame moves, stale frame not truth |
| E25 cycle rejection | PASS — at/after-consumer support rejected before mutation |

## 4. Final metrics

The evidence candidate records:

```text
false Resolved                         = 0
unexpected Missing                    = 0
unexpected Ambiguous                  = 0
unexpected Unsupported                = 0
unaccounted Faces                     = 0
unaccounted Edges                     = 0
unaccounted Vertices                  = 0
frame instability                     = 0
cold-rebuild semantic mismatches      = 0
stale publication/runtime acceptances = 0
geometry-similarity automatic rebinds = 0
cycle mutations after rejection       = 0
```

All frozen MUST_RESOLVE rows resolve.

All frozen MUST_MISSING / MUST_AMBIGUOUS / MUST_UNSUPPORTED / REPRESENTATION_ARTIFACT rows retain the declared fail-closed meaning.

## 5. Architecture findings

### 5.1 Topology and carrier geometry must remain distinct

The evidence validates:

```text
Face   != Surface
Edge   != Curve
Vertex != Point
```

A bounded Face can split while the semantic Surface remains stable. Consumers must therefore select the reference layer they actually need.

Sketch support needs a planar Surface attachment.

Future Fillet/Chamfer will normally need strict material Edge semantics rather than only an unbounded carrier Curve.

### 5.2 Stage is part of semantic meaning

The same semantic source role can exist at multiple Body stages.

Resolution must use the declared consumed stage rather than search the final Body globally.

A production reference therefore requires a Body-stage component equivalent in meaning to:

```text
EmptyBody
AfterFeature(FeatureId)
```

The exact type layout remains owned by production PM-02.

### 5.3 Provider history is useful evidence, not authored identity

OCCT Generated / Modified / Deleted history is strong transient evidence for lineage.

It is insufficient as durable CAD identity because:

- one semantic meaning may split into several current topological realizations;
- two semantic claims may collapse onto one provider object;
- provider history may be asymmetric;
- a deleted object may later be replaced by identical geometry;
- runtime provider objects do not survive cold reconstruction.

### 5.4 Geometry similarity must stay diagnostic-only

Equal or near-equal:

- plane equations;
- centroids / areas;
- cylindrical radii;
- Edge lengths;
- XYZ coordinates;
- provider shape classes

do not prove semantic continuity.

No production resolver may use nearest / longest / first / most-similar geometry as automatic rebinding authority.

### 5.5 Edge branch discrimination is required

PM-02P.C proves that two semantic carrier Surfaces can produce more than one disconnected current intersection branch.

Therefore a singular production Edge/Curve selector cannot always be only:

```text
Surface A x Surface B
```

When the accepted operation can yield multiple branches, the selector requires an explicit semantic branch/provenance discriminator.

The evidence does not freeze its serialized representation.

### 5.6 Vertex identity cannot be coordinate identity

A stable semantic Vertex may move numerically after an upstream edit.

An unrelated Vertex may later occupy exactly the old XYZ.

Production Point/Vertex meaning must therefore derive from semantic carrier/Curve relationships or operation provenance, not coordinates.

### 5.7 Planar carrier frames are semantic derived state

For supported Extrude geometry, canonical planar frames can be reconstructed algebraically from authored provenance.

They remain stable through trim/split and cold rebuild.

Provider UV, current trim boundary, first edge, camera, tessellation and bounding boxes are unnecessary and must remain non-authoritative.

### 5.8 Dynamic Sketch support should persist intent, not world placement

The evidence validates ADR-0016's attachment model:

```text
durable:
    SketchId
    semantic planar Surface support
    local U/V geometry
    visibility / authored Sketch state

derived:
    current resolved O/U/V/N support frame
    world Sketch geometry
```

A moved upstream carrier moves derived world geometry without authored document mutation.

Missing/Ambiguous support produces no current world frame rather than publishing stale last-good placement.

### 5.9 Freshness must surround runtime topology

Runtime Face/Edge/Vertex tokens are safe only inside the authority scope that produced them.

Production selection/resolution must reject stale evidence across at least:

- DocumentRevision;
- canonical DocumentSession replacement;
- evaluation generation;
- provider/runtime generation.

Numeric token reuse must not restore authority.

### 5.10 A bounded cycle rule is sufficient for PM-02

For current single-Body ordered Features, support admission can be checked using:

- ordered Feature history;
- explicit Sketch -> Profile -> Feature consumption;
- declared support producer stage.

Support produced at or after the consuming Feature is rejected before mutation.

PM-02 does not need a universal dependency-graph framework.

## 6. Provider contradiction review

No frozen PM-02P semantic expectation required weakening after observing OCCT behavior.

Failures encountered during implementation were bounded test/build defects:

- compile-order visibility in E;
- missing temporary test fixture directory in E.

They did not change the evidence oracle.

No unresolved provider contradiction remains in E01-E25.

## 7. Production recommendation

The Owner-reviewable production recommendation is recorded in:

`work/PM-02_PRODUCTION_ARCHITECTURE_RECOMMENDATION.md`

The recommendation deliberately freezes semantic responsibilities and invariants, not a speculative universal class hierarchy or final persistence byte layout.

## 8. PM-02P completion boundary

This synthesis candidate must receive Windows FULL on its own exact head before PM-02P may be marked COMPLETED — PASS.

After that gate:

- PM-02P may close;
- production PM-02 remains inactive;
- only preparation of a separately Owner-accepted production PM-02 Work Contract is authorized;
- no Datum / Projection / Revolve / Fillet / Chamfer implementation is activated by PM-02P closure.
