# PM-05A — Edge Feature Topology / Provider Evidence

**Status:** EVIDENCE PASS — OWNER D2 REVIEW REQUIRED  
**Authority:** `work/PM-05_EDGE_FEATURES.md` §15 / §26 PM-05A  
**Baseline:** PM-05 activation `7efc7aa3564be1bb1c5e1afdf30d99e6b4cc5425`  
**Final evidence candidate:** `be3a4830b9a7ea263a7c17a649b251138050db3a`  
**Windows FULL:** #1603 — PASS  
**Production mutation:** PM-05B+ remains BLOCKED pending explicit Owner acceptance of the conclusions in this report

## 1. Purpose

PM-05A was required to remove the architecture uncertainty around production multi-Edge Fillet/Chamfer before durable Feature/schema/UI work.

The gate had to freeze:

1. a concrete durable strict material-Edge selector, including a branch discriminator;
2. deterministic multi-Edge canonicalization;
3. connected-corner generated topology provenance;
4. native-provider tangent-chain behavior and an explicit-input enforcement strategy;
5. refine/unify policy;
6. generated Face/Surface/Edge/Curve/Vertex/Point role families needed for chained edge features;
7. the supported common-corner matrix.

This report proposes the D2 production decisions. Nothing below activates PM-05B until the Owner explicitly accepts these conclusions.

## 2. Automated evidence summary

### 2.1 Exact-head verification

Windows FULL #1603 on `be3a4830b9a7ea263a7c17a649b251138050db3a`:

- core-only: 25/25 PASS;
- kernel-native: 49/49 PASS;
- desktop FULL: 104/104 PASS;
- FAST/SUBSYSTEM dispatcher checks: PASS;
- architecture/kernel-boundary guards: PASS;
- SR-02 / CI-04 evidence: PASS.

Earlier evidence baselines:

- FULL #1587 PASS — initial 14-case provider matrix;
- FULL #1589 PASS — matrix logging/capture infrastructure;
- FULL #1596 PASS — tangent/upstream/chaining evidence.

Failures between those runs were evidence-harness defects or focus-metadata mistakes, not accepted product/provider semantic failures.

### 2.2 Multi-Edge provider matrix

For both constant-radius Fillet and equal-distance Chamfer, the following normal scenarios produced one valid solid:

| Scenario | Fillet | Chamfer |
| --- | --- | --- |
| one Edge baseline | PASS | PASS |
| two disconnected Edges | PASS | PASS |
| two adjacent Edges | PASS | PASS |
| three Edges at one trihedral corner | PASS | PASS |
| closed explicit Edge loop | PASS | PASS |
| mixed connected + disconnected set | PASS | PASS |

Across all 14 probes including the two intentionally excessive-parameter failures:

- explicit provider membership: 14/14 exact for the box matrix;
- reverse insertion-order topology/volume invariance: 14/14;
- normal scenarios successful: 12/12;
- excessive Radius/Distance: 0 successful results, as required;
- partial best-effort result was not accepted.

Therefore the common multi-Edge/corner matrix required by the Work Contract has no provider blocker.

## 3. Tangent-chain evidence and production rule

A dedicated solid contains two bounded collinear/tangent Edges on one geometric boundary.

For both Fillet and Chamfer:

```text
authored/requested Edge set = {E1}
native provider contour      = {E1, E2}
build                        = success
exact membership             = false
```

Therefore native provider contour growth is real.

The same fixture with both bounded Edges explicitly supplied gives:

```text
authored/requested Edge set = {E1, E2}
native provider contour      = {E1, E2}
exact membership             = true
build                        = success
result topology/volume       = same as provider-grown one-Edge request
```

### D2 conclusion T1 — explicit-input enforcement

PM-05 production must inspect the native provider contour membership after registering the authored Edge set and before accepting the operation.

Let:

```text
A = exact set of runtime Edges resolved from authored MaterialEdgeReferences
P = union of native provider contour Edges after all explicit Add/register calls
```

Admission is:

```text
P == A -> provider input admission PASS
P != A -> reject implicit provider expansion
```

No tangent-chain member discovered only by the provider becomes authored intent.

If the user explicitly selects the complete tangent chain, it is a normal explicit multi-Edge input and is allowed.

There is no implicit "Tangent Chain" authored option in PM-05 v1.

## 4. Upstream survival evidence

For both Fillet and Chamfer input Edge meaning:

| Upstream event | candidates | semantic result | downstream provider attempted |
| --- | ---: | --- | --- |
| dimension change | 1 | Resolved | yes, success |
| unchanged boolean stage | 1 | Resolved | yes, success |
| unique trim | 1 | Resolved | yes, success |
| split | 2 | Ambiguous | no |
| remove | 0 | Missing | no |

This reproduces the PM-02P survival contract on an actual future edge-consuming operation.

### D2 conclusion U1 — no fragment promotion

A singular authored material Edge that splits into multiple valid descendants becomes **Ambiguous**.

It does not become:

- the first fragment;
- the longest fragment;
- the closest fragment;
- every fragment;
- the fragment most similar to the old geometry.

A deleted branch remains Missing even if unrelated similar geometry exists.

## 5. Fillet / Chamfer chaining evidence

The evidence adapter builds the first edge Feature, enumerates ordinary non-degenerate material boundary Edges of its generated surface, then attempts the opposite operation on each candidate.

Observed:

```text
Fillet -> Chamfer
generated surface boundary Edges: 4
Chamfer successes at evidence parameter: 2

Chamfer -> Fillet
generated surface boundary Edges: 4
Fillet successes at evidence parameter: 4
```

The two Fillet-boundary Edges that do not accept the sampled Chamfer parameter are geometric operation failures, not proof that those Edges lack engineering identity.

### D2 conclusion C1 — generated engineering boundaries

Ordinary material boundary Edges produced by a successful PM-05 Feature are eligible to receive semantic Edge meaning and to be selected by a later PM-05 Feature.

Semantic referenceability and operation feasibility remain separate:

```text
reference resolves
+ requested parameter impossible
-> Feature Failed

reference unresolved
-> Feature Blocked
```

## 6. Output Face provenance evidence

Refined provider history separated:

- modified inherited source Faces;
- Faces generated from explicitly selected Edges;
- Faces generated from shared selected Vertices;
- new Faces with no defensible provider-history claim.

Observed for both Fillet and Chamfer:

| Scenario | new Faces | modified inherited | selected-Edge generated | shared-Vertex generated | unclaimed |
| --- | ---: | ---: | ---: | ---: | ---: |
| single | 5 | 4 | 1 | 0 | 0 |
| disconnected pair | 8 | 6 | 2 | 0 | 0 |
| adjacent pair | 7 | 5 | 2 | 0 | 0 |
| trihedral corner | 10 | 6 | 3 | 1 | 0 |
| closed loop | 9 | 5 | 4 | 0 | 0 |
| mixed | 9 | 6 | 3 | 0 | 0 |

Critical finding:

```text
unclaimed new Faces = 0 for every accepted normal scenario
```

The trihedral case exposes exactly the special corner condition we needed to understand: three Edge-generated transition surfaces plus one Face generated from the shared source Vertex.

### D2 conclusion P1 — inherited Surface continuity

A Face returned by native Modified history from an existing semantic Surface remains a bounded realization of that existing Surface meaning.

PM-05 does not re-own an inherited Surface merely because Fillet/Chamfer trimmed it.

This follows ADR-0017's ownership/continuation rule.

### D2 conclusion P2 — Edge transition Surface

A generated Fillet/Chamfer transition Surface is durably identified by:

```text
producer FeatureId
+ transition role (FilletSurface | ChamferSurface)
+ source MaterialEdgeReference
```

The source reference itself, not its position in a vector, is provenance.

Therefore adding/removing another Edge from the same Feature does not renumber an unrelated surviving transition Surface.

### D2 conclusion P3 — corner transition Surface

Where provider history produces a distinct shared-Vertex/corner patch, its durable meaning is:

```text
producer FeatureId
+ CornerTransition role
+ source semantic FeaturePointAddress
+ canonical set of incident authored MaterialEdgeReferences participating at that Point
```

The incident Edge set is part of the meaning because a two-Edge and a three-Edge transition at the same semantic Point are not silently the same authored result.

Provider Face order is not used.

## 7. Durable strict MaterialEdgeReference

Current production `FeatureCurveAddress` describes a semantic Curve/carrier family:

```text
producer FeatureId
+ Curve role
+ canonical pair of adjacent semantic Surfaces
```

PM-02P proved that this can describe more than one disconnected bounded current Edge. It is therefore insufficient by itself for Fillet/Chamfer.

### D2 conclusion E1 — production selector

PM-05B should introduce a typed durable selector semantically equivalent to:

```text
MaterialEdgeReference
    BodyStageRef stage
    FeatureCurveAddress curve
    EdgeBranchDiscriminator branch
```

`BodyStageRef` is the exact consumed upstream stage.

`FeatureCurveAddress` remains the semantic Curve/carrier family.

`EdgeBranchDiscriminator` is one of:

```text
SingularAtAuthoredStage

BetweenSemanticPoints
    FeaturePointAddress first
    FeaturePointAddress second
```

The two Point addresses are stored in canonical semantic order.

### E1 authoring rule

At the exact authored/consumed Body stage:

1. resolve the picked runtime Edge through the current topology catalog;
2. require material/referenceable/non-seam/non-partition state;
3. resolve its `FeatureCurveAddress`;
4. if that Curve has exactly one bounded Edge realization at this stage, author `SingularAtAuthoredStage`;
5. if the Curve has multiple bounded branches, require the selected branch to have two defensible semantic endpoint Point addresses that uniquely distinguish it and author `BetweenSemanticPoints`;
6. if a multi-branch Edge cannot be distinguished by semantic Points, strict Edge authoring is Unsupported for that case.

No XYZ, length, provider ordinal or Viewer token is persisted.

A unique closed material Edge, such as an ordinary circular cap boundary, remains authorable through `SingularAtAuthoredStage`; it does not need fake endpoints.

### E1 resolution rule

Resolution occurs only in the declared `BodyStageRef`.

For `SingularAtAuthoredStage`:

- one current descendant branch -> Resolved;
- zero -> Missing;
- more than one after split -> Ambiguous.

For `BetweenSemanticPoints`:

- the Point pair is a semantic branch discriminator at the authored stage, not a coordinate matcher;
- exactly one current branch carrying that authored provenance -> Resolved;
- zero -> Missing;
- multiple descendants of that branch -> Ambiguous.

Provider Modified/Generated history may establish transient lineage while rebuilding the declared stage, but provider identity/history is never persisted.

If an exotic multiple-closed-branch case cannot be semantically discriminated without geometry/provider order, PM-05 v1 fails closed as Unsupported rather than inventing an identity rule.

## 8. Canonical multi-Edge set

### D2 conclusion M1

One Fillet/Chamfer Feature owns a semantic set of `1..N MaterialEdgeReference`.

The durable set is:

- deduplicated structurally;
- sorted by semantic structural ordering of `MaterialEdgeReference`;
- independent of user click order;
- entirely within one consumed `BodyStageRef`.

Provider evidence was reverse-order invariant for all 14 matrix cases.

Generated Surface provenance refers to the source `MaterialEdgeReference`, never to the canonical vector index.

## 9. Generated Curve / Edge / Point role families

### D2 conclusion G1 — Curves and material Edges

After PM-05:

- inherited Curve meaning continues through proven unique trim/Modified lineage;
- a generated boundary between two semantic Surfaces receives the existing semantic Curve model based on producer/role + canonical adjacent Surface meanings;
- its strict bounded material Edge identity is `MaterialEdgeReference`, with the branch discriminator above where the Curve has multiple branches;
- periodic seams remain representation artifacts;
- ADR-0017 same-Surface representation partitions remain non-referenceable by default.

### D2 conclusion G2 — Points / Vertices

Existing `FeaturePointAddress` semantics remain the first production Point meaning:

```text
producer FeatureId
+ canonical semantic Surface intersection provenance
```

PM-05 may use those Points as branch/corner provenance when defensible.

A provider Vertex without defensible semantic Point meaning remains completely accounted but Unsupported as a durable Point/reference discriminator. PM-05 must not fall back to XYZ.

## 10. Refine / unify / healing policy

All accepted normal Fillet/Chamfer matrix cases, connected corners and chaining evidence were produced without a generic post-operation refine/unify/healing pass.

### D2 conclusion R1

PM-05 v1 uses:

- no global same-domain unification;
- no generic B-Rep healing pass;
- no fuzzy escalation;
- no iterative tolerance widening.

The raw successful provider result is topology-accounted directly.

If a future concrete defect requires refine/unify, that is a separate D2 change with explicit topology/reference survival evidence.

## 11. Supported common-corner matrix

### D2 conclusion K1

The PM-05 production target must support, for both constant-radius Fillet and equal-distance Chamfer when geometrically feasible:

- one Edge;
- multiple disconnected Edges;
- two adjacent Edges sharing a corner;
- three Edges at a common trihedral corner;
- a closed explicit Edge loop;
- mixed connected + disconnected explicit sets.

A geometrically impossible parameter is a normal **Failed** Feature result, not Unsupported topology.

The package may not downgrade to a single-Edge-only tool.

## 12. Proposed PM-05B production boundary

If the Owner accepts E1/M1/T1/P1-P3/G1-G2/R1/K1, PM-05B may implement only the already-contracted durable foundation:

- `MaterialEdgeReference` and resolver;
- FilletFeature / ChamferFeature durable definitions;
- structural/canonical set validation;
- schema v14 migration/persistence;
- application command/draft foundation;
- no provider identity persistence.

PM-05C remains responsible for production kernel operations and complete topology publication; PM-05D/E/F remain separately ordered.

## 13. Owner decision required

PM-05A evidence finds **no blocker** to the accepted full multi-Edge / connected-corner PM-05 direction.

Recommended Owner decision:

**ACCEPT PM-05A conclusions E1, M1, T1, U1, C1, P1-P3, G1-G2, R1 and K1 and authorize PM-05B under the existing Work Contract.**

Until that explicit acceptance is recorded:

- PM-05A remains the active checkpoint;
- PM-05B+ production mutation remains blocked;
- no schema v14 / durable FilletFeature / ChamferFeature / production toolbar implementation is authorized.
