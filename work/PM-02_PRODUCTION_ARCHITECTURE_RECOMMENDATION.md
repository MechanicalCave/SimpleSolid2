# PM-02 — Production Architecture Recommendation after PM-02P

**Status:** OWNER-REVIEWABLE RECOMMENDATION — production PM-02 remains NOT ACTIVE  
**Date:** 2026-10-04  
**Evidence basis:** PM-02P A-E + `work/PM-02P_TOPOLOGY_EVIDENCE_REPORT.md`  
**Decision authority:** ADR-0014 + ADR-0016  
**Purpose:** bound the next production Work Contract without prematurely freezing final persistence byte layout or a universal topology framework

## 1. Recommendation

Production PM-02 should implement a stage-scoped semantic topology catalog and make face-supported Sketch the first product consumer of that catalog.

The production package should preserve the evidence distinction:

```text
bounded topology       semantic carrier
Face                -> Surface
Edge                -> Curve
Vertex              -> Point
```

and keep runtime provider objects outside authored design intent.

The recommended vertical product path is:

```text
Body stage
-> runtime complete topology catalog
-> Viewer picks bounded Face
-> Part resolves Face catalog record
-> semantic planar Surface support
-> derived Sketch frame
-> local Sketch geometry
-> Profile
-> existing Extrude Add/Cut
-> upstream edit
-> stage-aware semantic re-resolution
-> recompute or structured failure
```

Projection remains outside PM-02.

## 2. Production ownership boundaries

### Part owns

- durable semantic Body-stage reference intent;
- reference-kind semantics;
- semantic producer/provenance;
- stage-scoped resolution;
- Sketch support intent;
- Sketch/support dependency validation;
- reference status and structured diagnostics;
- persistence of semantic selectors;
- migration from current Origin-hosted Sketch representation;
- Feature evaluation ordering and fail-closed downstream behavior.

### Kernel neutral layer owns

- typed runtime topology token vocabulary required to connect the current provider result to the current Part evaluation;
- provider-neutral topology/catalog data needed by Part;
- carrier geometry classification;
- current resolution cardinality/status;
- no persistence and no authored identity.

### OCCT provider owns

- mapping current TopoDS topology into the current runtime catalog;
- Generated / Modified / Deleted evidence;
- exact geometry classification;
- B-Rep adjacency/history inspection;
- transient runtime tokens;
- provider failure diagnostics.

### Viewer owns

- display/picking only;
- transient runtime pick identity;
- handoff of the pick to the Part semantic resolver.

Viewer state never becomes CAD identity.

## 3. Body-stage topology catalog

Every successful Body Feature stage should produce a disposable evaluated object conceptually equivalent to:

```text
BodyStageEvaluation
    stage
    RuntimeSolid
    BodyStageTopologyCatalog
        Face records
        Edge records
        Vertex records
        Surface carrier records
        Curve carrier records
        Point carrier records
```

Exact class names remain implementation choices.

Required invariants:

1. every unique provider Face at the stage has exactly one catalog record;
2. every unique provider Edge has exactly one catalog record;
3. every unique provider Vertex has exactly one catalog record;
4. every record has an explicit semantic accounting classification;
5. no unexplained provider topology is silently dropped;
6. current runtime tokens are not serialized;
7. a failed current stage never leaves a stale catalog as final Body truth.

The catalog is derived state and may be rebuilt at any time from authored intent.

## 4. Reference families

Production PM-02 should preserve distinct semantic reference families.

### Strict topology references

Needed when the bounded material object itself matters:

- strict Face reference;
- material Edge reference;
- material Vertex reference.

### Carrier references

Needed when the geometric semantic carrier is the intended object:

- Surface reference;
- Curve reference;
- Point reference.

Sketch support consumes a planar **Surface** reference.

A future Fillet/Chamfer will generally consume strict material Edge references.

Do not collapse these into one generic “TopoReference” whose semantics depend on runtime shape type.

Shared implementation utilities are acceptable; semantic type safety should remain explicit.

## 5. Minimum durable selector meaning

Exact persisted structures should be frozen only by the production PM-02 contract, but every durable topology/carrier selector must encode semantic meaning equivalent to:

```text
reference kind
+ Body producer/stage
+ semantic producer Feature
+ role / source provenance
+ branch discriminator when required
```

Examples of evidence-backed provenance:

- Extrude ProfileCap / ExtentCap / NegativeCap / PositiveCap;
- Extrude Side(Profile boundary use);
- material intersection derived from accepted carrier meanings;
- semantic Vertex/Point from accepted carrier/Curve relations.

Durable selectors must not contain:

- TopoDS objects/handles;
- OCAF/provider labels used as identity;
- provider Face/Edge/Vertex ordinals;
- Viewer tokens;
- mesh indices;
- nearest-geometry witnesses;
- coordinates as identity.

## 6. Stage semantics

A reference resolves in a declared Body stage.

Recommended semantic model:

```text
BodyStageRef
    EmptyBody
    AfterFeature(FeatureId)
```

or an equivalent bounded representation.

A downstream consumer must resolve against its declared upstream stage.

Global final-Body search is forbidden because the same producer meaning can be observable at more than one stage.

## 7. Surface / Face resolver

For a strict Face meaning:

```text
0 candidates -> Missing
1 candidate  -> Resolved
>1 candidates -> Ambiguous
undeclared semantic meaning -> Unsupported
```

For a semantic Surface carrier, multiple bounded Face realizations may still represent one Resolved Surface.

The resolver should preserve Surface continuity through accepted trim/split behavior when semantic lineage proves the same carrier.

Provider history may establish transient candidate lineage, but geometry similarity cannot promote a candidate to Resolved.

## 8. Edge / Curve resolver

Production Edge semantics should be derived from semantic carrier/operation provenance.

Evidence-backed material Edge meanings include:

- cap x side boundary;
- side x side boundary;
- inherited/trimmed material Edge;
- Boolean-created intersection Edge.

Critical PM-02P finding:

```text
Surface A x Surface B
```

is not always enough to identify one singular current branch.

Where an operation can produce several disconnected branches, a singular material Edge/Curve selector requires an explicit semantic branch/provenance discriminator.

The production PM-02 Work Contract must freeze how that discriminator is authored/reconstructed for the supported Extrude Add/Cut universe before persistence is implemented.

Forbidden fallback authorities:

- first provider Edge;
- longest/shortest Edge;
- nearest Edge;
- XYZ sorting;
- Edge index;
- arbitrary provider history winner.

Periodic seams are catalogued representation artifacts unless independent semantic design intent exists.

## 9. Vertex / Point resolver

A production Vertex/Point meaning should arise from semantic relationships such as:

- cap + side + side carrier intersection;
- endpoints/branches of semantic material Curves;
- generated operation provenance.

XYZ is diagnostic only.

Production tests must include:

- stable Point moving under upstream dimensions;
- deleted Point;
- unrelated exact-same-XYZ replacement;
- new generated Point with independent provenance.

## 10. Carrier geometry classification

Surface classification must support the evidence-backed categories required by ADR-0016:

- Plane;
- Cylinder;
- Cone;
- Sphere;
- Torus;
- Other.

Curve classification should include exact classes needed by the activated operation set and must fail closed for unsupported classes rather than approximate them.

Classification is evaluated data, not identity.

## 11. Deterministic planar frames

### Extrude cap

Evidence-backed rule:

```text
O = source support/Profile O + semantic cap offset * source N
U = source U
V = source V
N = source N
```

OneSide Reverse and negative offsets do not silently mirror U/V/N.

Outward/material face normal is not the authority for canonical Sketch orientation.

### planar Extrude side from authored Line

Evidence-backed rule:

```text
O = authored source Line start mapped by source frame
U = normalized authored source Line direction in world
V = source support/Profile N
N = U x V
```

The production implementation must keep material side sense explicit where needed rather than encoding it by silently flipping the canonical carrier frame.

### Boolean descendants

Trim/split preserves the carrier frame.

A Cut-exposed planar Surface uses the semantic tool Surface frame.

Never derive the canonical support frame from:

- provider UV;
- first current Edge;
- Face centroid;
- current trim loop;
- camera;
- tessellation;
- bounding-box orientation.

## 12. Sketch attachment model

The durable face-supported Sketch should own intent equivalent to:

```text
SketchId
SupportReference = semantic planar Surface
authored local U/V geometry
visibility and existing Sketch authored state
optional future support-relative authored transform only if explicitly exposed
```

The evaluated result should own:

```text
ResolvedSketchSupport
    status
    O/U/V/N
    diagnostics
```

The current absolute world `SketchPlacement` must not remain a second independent authority once dynamic support is introduced.

Migration should:

1. verify existing Origin support / placement coherence;
2. preserve SketchId and EntityId;
3. preserve local U/V geometry;
4. convert existing Origin support to the new semantic support representation;
5. stop persisting redundant world placement where the support frame is derivable;
6. fail closed on malformed legacy authored state.

If the product later exposes offset/rotation relative to support, store that as an authored **support-relative** transform, not a second absolute world frame.

## 13. Stage-aware Profile evaluation

Current Profile-to-kernel conversion assumes world placement is already authored.

Face-supported Sketch requires evaluation order equivalent to:

```text
evaluate upstream Feature stage
-> obtain current BodyStageTopologyCatalog
-> resolve Sketch support at that stage
-> derive ResolvedSketchSupport frame
-> map local Sketch/Profile geometry into world
-> build kernel Profile input
-> evaluate consuming Feature
```

A Profile driven by a missing/ambiguous support cannot use stale last-good world geometry as current Body truth.

The existing resolved-prefix presentation rule remains presentation-only and cannot feed downstream modeling.

## 14. Dependency-cycle admission

For the current ordered single-Body Feature model, use the bounded evidence-backed preflight:

1. identify the Feature stage that produces the requested support;
2. identify the first Feature that consumes the Sketch through its Profile;
3. require the support-producing stage to be strictly upstream of every consuming Feature;
4. reject at/after-consumer support before transaction mutation.

No universal dependency graph is needed for PM-02.

The diagnostic should explicitly identify a cycle/dependency admission failure.

## 15. Runtime freshness and selection

Typed runtime tokens should remain separate by topology kind:

- RuntimeFaceToken;
- RuntimeEdgeToken;
- RuntimeVertexToken.

They are valid only inside current evaluated authority equivalent to:

```text
DocumentId
DocumentRevision
canonical session generation
evaluation generation
provider generation
```

Selection path:

```text
Viewer pick
-> current typed runtime topology token
-> current stage catalog record
-> semantic strict topology + carrier meaning
-> tool-specific admission
```

Any stale generation is rejected before mutation.

Numeric token reuse is allowed internally but cannot restore old authority.

## 16. Persistence recommendation

The production PM-02 Work Contract may introduce the next Part schema version, but only after it freezes:

- bounded selector structures for activated reference kinds;
- Body-stage encoding;
- Edge branch discriminator;
- new Sketch support representation;
- migration from schema v8 Origin support/placement;
- load validation and fail-closed diagnostics.

Do **not** persist:

- evaluated topology catalogs;
- runtime tokens;
- TopoDS/OCAF provider identity;
- canonical evaluated frames;
- Viewer selection state;
- provider history.

## 17. Suggested production PM-02 delivery checkpoints

A bounded production contract should separate architecture risk from UI risk.

Recommended order:

1. **PM-02A — production evaluated topology catalog**  
   Complete Face/Edge/Vertex accounting and typed runtime tokens at every Body stage.

2. **PM-02B — production semantic Surface/Face resolution**  
   Extrude cap/side lineage, Boolean trim/split/delete/Cut-exposed surfaces, deterministic frames.

3. **PM-02C — production Edge/Curve + Vertex/Point semantic catalog**  
   Evidence-backed material meanings, branch discriminator, seam classification.

4. **PM-02D — runtime Viewer semantic picking / inspection**  
   Face/Edge/Vertex picks map to current catalog and semantic meaning; stale picks rejected.

5. **PM-02E — authored support model + schema migration**  
   Semantic Origin/planar-Surface support and removal of redundant absolute world placement authority.

6. **PM-02F — stage-aware Sketch/Profile evaluation**  
   Resolve support from upstream stage, derive frame/world Profile, structured failure.

7. **PM-02G — arbitrary planar Body-face Sketch creation / re-support**  
   Cap, lateral and Cut-exposed planar Surfaces. Non-planar admission Unsupported.

8. **PM-02H — existing Extrude Add/Cut from face-supported Sketch**  
   Full upstream edit/recompute behavior and dependency-cycle admission.

9. **PM-02I — lifecycle / persistence / repair**  
   Edit, re-support, Undo/Redo, Delete, Save/Close/Reopen, cold rebuild, Missing/Ambiguous diagnostics.

10. **PM-02J — docs / acceptance / Windows Owner workflow**  
    Internal + PL/EN product docs and final acceptance matrix.

The exact checkpoint names may change in the accepted production Work Contract; the semantic gates should not.

## 18. Explicit non-goals for production PM-02

Do not pull the following into PM-02:

- Projection / Project Edge;
- automatic face-boundary capture into Sketch;
- Datum creation;
- Revolve;
- Fillet;
- Chamfer;
- multi-body;
- universal dependency graph;
- automatic geometry-similarity reference repair;
- arbitrary non-planar Sketch mapping.

PM-03 Datum should consume the semantic reference foundation after PM-02 rather than recreate it.

## 19. D2 decisions that the production Work Contract must explicitly freeze

PM-02P closes the architecture direction but intentionally leaves implementation-layout choices for production.

The next Owner-accepted Work Contract must explicitly freeze:

1. concrete durable selector structures for Surface/Face/Edge/Vertex reference kinds used by PM-02;
2. concrete stage-reference encoding;
3. explicit semantic branch discriminator for singular Edge/Curve cases;
4. persistence/schema-v9 migration meaning;
5. replacement of current absolute world `SketchPlacement` authority;
6. runtime freshness/public selection contract boundaries;
7. exact structured diagnostics exposed to GUI/Command Line.

These choices must remain consistent with ADR-0016 and the PM-02P survival matrix.

## 20. Recommendation outcome

PM-02P evidence does not reveal an architecture blocker to production PM-02.

Recommended next governance action after PM-02P final exact-head FULL and closure:

**prepare, review and explicitly Owner-accept a bounded production PM-02 Work Contract implementing the architecture above.**

Do not activate PM-03 Datum or any later package before production PM-02 completes its own acceptance.
