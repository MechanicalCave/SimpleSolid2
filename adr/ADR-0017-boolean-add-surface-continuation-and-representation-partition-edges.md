# ADR-0017 — Boolean Add Surface Continuation and Representation-Partition Edges

**Status:** ACCEPTED  
**Proposed:** 2026-10-05  
**Owner acceptance:** 2026-10-05 — accepted during PM-02J manual-remediation review  
**Decision class:** D2 bounded Part semantic-topology amendment  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Parent authority:** `work/PM-02_BODY_SEMANTIC_TOPOLOGY_FACE_SUPPORTED_SKETCH.md`  
**Amends:** ADR-0016 §6 Surface lineage, §16 Edge semantics and PM-02 Feature-contribution interpretation  
**Preserves:** semantic identity, explicit Body stage, provider isolation, fail-closed resolution, zero geometry-similarity rebinding, no provider identity persistence  
**Explicitly excludes:** general same-domain healing, generic dependency graph, Projection, Drawing implementation and a future authored Split/Divide Face feature

## Context

PM-02J Owner manual acceptance found one bounded semantic-topology defect in the existing Extrude Add universe.

When an Add extends an existing planar material carrier, OCCT may retain a topological partition between bounded Face realizations. The result can be visually one continuous engineering surface while provider Boolean history lets both:

- one inherited semantic Surface; and
- one newly-created tool Surface

claim overlapping current Face realization.

The current fail-closed implementation marks such overlapping Surface claims Ambiguous. That is safe against false rebinding, but it has three undesirable product consequences:

1. the visually continuous planar region may reject Create Sketch as Ambiguous;
2. the topological partition Edge is rendered/picked as if it were an engineering material boundary;
3. future Drawing/Projection could incorrectly treat that representation partition as a visible design edge.

Geometry equality or coplanarity alone must not be used to repair this. Equal geometry is not identity.

## Decision

### 1. Add continuation is lineage-driven, never proximity-driven

Within the currently supported Extrude **Add** Boolean only, a newly-created planar tool Surface may continue one inherited planar semantic Surface when provider Boolean history proves a unique overlap in the current result.

The bounded continuation test is:

- operation is Add;
- created claim is planar;
- one or more current Face descendants of the created claim are also descendants of an inherited planar Surface claim;
- exactly one inherited semantic Surface is the continuation candidate;
- no second inherited Surface competes for that continuation.

No plane-distance tolerance, normal comparison, centroid, area, nearest geometry, provider traversal order or current Face ordinal may select the winner.

If zero or more than one inherited continuation candidate exists, existing fail-closed Missing/Ambiguous/Unsupported behavior remains authoritative.

### 2. Inherited Surface identity wins a proven continuation

For a proven Add continuation:

- the inherited durable Surface address/provenance remains the semantic carrier identity;
- its current realization expands to the union of its own current Face descendants and the continued created-surface Face descendants;
- strict bounded Face meaning may remain split/Ambiguous while the Surface carrier remains Resolved;
- the created tool Surface does not become a second independent durable carrier for those continued Face descendants.

This is continuation of existing meaning, not geometry-based rebinding.

### 3. Feature Contribution stays separate from Surface ownership

Surface carrier ownership and Current Feature Contribution are distinct runtime queries.

Absorbing a created tool Surface into an inherited carrier must not erase the fact that the current Add contributed material/topology in that region.

Production implementation may retain runtime-only contribution membership/evidence sufficient to highlight the current Feature contribution, but that evidence:

- is never durable reference identity;
- is never serialized;
- cannot change Sketch support ownership;
- cannot create a second selectable topology authority.

### 4. Same-Surface partition Edge is a representation artifact

If a current Edge separates two bounded Face realizations that both resolve to the same semantic Surface carrier, that Edge is a **same-Surface representation partition**.

It remains in complete provider topology accounting, but it is not an ordinary material/design boundary.

Therefore it is:

- non-referenceable by default;
- excluded from ordinary Edge preselection/picking;
- hidden in normal `Shaded + Edges` and `Shaded + Hidden Edges` engineering presentation;
- available to diagnostics/accounting if needed.

This generalizes the existing representation-artifact principle beyond periodic seams without classifying every semantically-unsupported Edge as an artifact.

### 5. Ordinary visible Edge policy

An ordinary visible engineering Edge represents a defensible material/design boundary, such as:

- a boundary between two distinct semantic Surfaces;
- a supported material Curve relation;
- a future explicitly-authored split boundary.

A provider partition inside one semantic Surface is not sufficient.

### 6. Future authored Split/Divide Face remains distinct

A future explicit Split/Divide Face tool may deliberately create persistent design partition meaning.

Such an authored split may remain visible/pickable and may later project into Drawing.

This ADR does not implement that feature and does not infer authored split intent from a Boolean partition.

### 7. Drawing/Projection consequence

Future Drawing/Projection must not blindly project every provider B-Rep Edge.

Its visible-edge policy must respect semantic/material boundary classification so same-Surface representation partitions do not appear as drawing edges unless an explicit future authored split gives them design meaning.

No Drawing or Projection implementation is authorized by this ADR.

### 8. Persistence

No schema change is required.

Continuation, current Face realization, contribution membership and representation-partition classification are derived runtime evaluation state only.

## Consequences

Positive:

- a planar engineering surface extended by Add remains one usable semantic Sketch-support carrier;
- artificial Add partition lines disappear from normal engineering edge display and picking;
- stable upstream Surface references survive Add continuation;
- Feature Contribution can remain truthful without stealing carrier identity;
- future Drawing receives a cleaner semantic boundary model.

Costs:

- provider/Part lineage handling must distinguish continuation from ordinary alias ambiguity;
- Feature Contribution can no longer rely solely on Surface producer ownership in the continuation case;
- topology regressions must prove no false continuation for coincident/similar but lineage-unrelated surfaces.

## Rejected directions

- merge Surfaces because their planes are numerically equal;
- use angular/distance tolerance as identity;
- let the newest Add always steal Surface ownership;
- keep both claims Ambiguous when unique Boolean lineage proves continuation;
- hide every semantically-unsupported Edge;
- physically delete provider topology merely to improve rendering;
- persist a merged provider Face/Surface token;
- implement Drawing or Split Face as part of this remediation.
