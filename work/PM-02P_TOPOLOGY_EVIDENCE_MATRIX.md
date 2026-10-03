# PM-02P — Topology Evidence Matrix

**Status:** FROZEN EVIDENCE ORACLE — OWNER ACCEPTED 2026-10-03  
**Version:** 1.0  
**Date:** 2026-10-03  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Architecture authority:** ADR-0014 + ADR-0016  
**Decision class:** evidence expectations only; this matrix does not freeze production schema/API

## 1. Purpose

This document freezes expected semantic outcomes for PM-02P before individual probes are implemented.

It prevents changing the expected answer after observing provider behavior. If real OCCT behavior contradicts an expectation, record the contradiction and STOP at the owning architecture boundary rather than weakening the matrix to obtain PASS.

## 2. Expected-outcome vocabulary

- **MUST_RESOLVE** — one semantic target/carrier must reconstruct uniquely from accepted producer/stage/role/provenance meaning.
- **MUST_MISSING** — the referenced semantic meaning no longer exists; no replacement may steal identity.
- **MUST_AMBIGUOUS** — more than one candidate or incompatible semantic interpretation satisfies the available selector meaning.
- **MUST_UNSUPPORTED** — the requested semantic meaning is outside the accepted model and provider topology must not be promoted merely to return a result.
- **REPRESENTATION_ARTIFACT** — topology exists as B-Rep/provider representation detail and must be accounted for without being promoted to ordinary authored material topology.

Additional non-reference outcomes:

- **GEOMETRIC_FAILURE** — references remain semantically valid but the modeling operation cannot produce an accepted result.
- **STALE_REJECTED** — obsolete revision/session/evaluation evidence cannot publish or commit.
- **INTEGRITY_FAILURE** — topology required by the accepted model cannot be accounted or receives conflicting false semantic claims.

## 3. Global rules

1. False Resolved count must remain exactly zero.
2. Every declared stable case must actually resolve.
3. Every provider Face/Edge/Vertex must be accounted in the evidence catalog.
4. Provider Generated/Modified/Deleted history is evidence, not identity.
5. Face[n], Edge[n], Vertex[n], traversal order, Viewer token and tree row are forbidden semantic identity.
6. Geometry equality, proximity, coordinates, area, length and normals are diagnostics only.
7. Cold rebuild reproduces semantic statuses and carrier frames without previous runtime/provider state.
8. Topology and carrier geometry are distinct: Face->Surface, Edge->Curve, Vertex->Point.
9. A bounded Face split may invalidate a strict Face reference while its semantic Surface remains stable.
10. Unexpected provider behavior is evidence and triggers review; it is not permission to invent heuristics.

## 4. Complete-accounting oracle

Every successful probe stage must independently enumerate provider topology and assert:

```text
provider Face count   == evidence Face count
provider Edge count   == evidence Edge count
provider Vertex count == evidence Vertex count
```

Every record is classified Referenceable, KnownRepresentationArtifact, SemanticallyUnsupported or IntegrityFailure.

Any unaccounted Face/Edge/Vertex is INTEGRITY_FAILURE.

## 5. E01 — rectangular Extrude complete topology

Scenario: Rectangle Profile -> Extrude Add OneSide.

Expected canonical topology for the simple non-degenerate prism:

```text
6 Faces
12 Edges
8 Vertices
```

Expected semantic Surfaces:

- ProfileCap — MUST_RESOLVE, Plane;
- ExtentCap — MUST_RESOLVE, Plane;
- four Side(Profile boundary use) carriers — each MUST_RESOLVE, Plane.

All six planar Surface carriers are future Sketch-support admissible.

All 12 Edges and all 8 Vertices must be accounted without topology ordinals.

COLD: YES.

Failure: any unaccounted topology; any Face identity derived only from orientation/position/order; any planar side excluded merely because it is lateral.

## 6. E02 — arbitrary planar lateral frame

Use legal source Sketch/Profile frames including XY, XZ and YZ and at least one lateral planar carrier whose admissibility cannot be explained as top/bottom.

For every planar side carrier:

- Surface — MUST_RESOLVE;
- frame O/U/V/N — deterministic;
- N = U x V;
- frame remains identical in semantic meaning after camera/tessellation/provider traversal changes and cold rebuild.

Failure: first-edge, raw UV, camera or world-top special case participates in frame identity.

## 7. E03 — circular Extrude and seam

Scenario: Circle Profile -> Extrude Add.

Expected carriers:

- two cap planes — MUST_RESOLVE;
- one cylindrical side Surface — MUST_RESOLVE as semantic carrier;
- cylindrical side Sketch support — MUST_UNSUPPORTED as non-planar.

Any periodic seam Edge/Vertex exposed by the provider is fully accounted but classified REPRESENTATION_ARTIFACT unless independent authored material meaning exists.

Failure: seam becomes ordinary authored Edge merely because OCCT exposes it.

## 8. E04 — mixed Line + Arc Profile

Scenario: closed Profile containing Line and Arc -> Extrude.

Expected:

- Line boundary -> planar side Surface — MUST_RESOLVE;
- Arc boundary -> curved/cylindrical side Surface — MUST_RESOLVE;
- reversing legal Profile traversal preserves semantic carrier identity and does not mirror/flip canonical planar carrier frames.

Failure: provider iteration or loop traversal representation becomes identity.

## 9. E05 — Profile with hole

Scenario: Profile with outer loop and inner hole -> Extrude.

Expected:

- outer side carriers — MUST_RESOLVE from outer boundary provenance;
- inner side carriers — MUST_RESOLVE from hole boundary provenance;
- caps with holes correctly accounted;
- every resulting Face/Edge/Vertex accounted.

Inner and outer carriers with geometrically similar classes remain semantically distinct by provenance.

## 10. E06 — attached Add trimming existing Surfaces

Scenario: Extrude001 Add -> attached Extrude002 Add where the second operation changes at least one existing Face boundary.

Expected stable carrier:

- Surface — MUST_RESOLVE;
- bounded Face realization may remain same or be replaced/trimmed;
- semantic carrier frame remains stable.

Failure: stable carrier becomes Missing only because its TopoDS_Face changed.

## 11. E07 — Cut trimming planar Surface

Scenario: Base Add -> Cut that removes part of an existing planar Face but not its semantic Surface.

Expected:

- SurfaceReference meaning — MUST_RESOLVE;
- canonical carrier frame — unchanged;
- bounded topology catalog — updated and complete.

Prospective Sketch world frame on the carrier remains valid without authored Sketch mutation.

## 12. E08 — one Face splits into multiple Faces

Construct a Boolean case where one previous bounded Face becomes two or more disconnected Faces that all realize the same semantic Surface.

Expected:

- strict singular Face meaning — MUST_AMBIGUOUS;
- semantic Surface meaning — MUST_RESOLVE;
- canonical Surface frame — unchanged.

Failure: first/largest/nearest Face is selected as the old strict Face.

## 13. E09 — Surface deletion and identical replacement trap

Delete a semantic Surface completely.

Expected old semantic Surface — MUST_MISSING.

If a geometrically identical plane/surface is later created by unrelated provenance, old meaning remains MUST_MISSING.

Failure: equal plane equation, same centroid, same area or proximity causes rebinding.

## 14. E10 — semantic Face claim alias/merge

Construct a case where two previously distinct semantic claims map to one current provider Face.

Expected:

- both singular meanings — MUST_AMBIGUOUS unless independent semantic provenance preserves one and removes the other;
- when one meaning survives independently and the other is genuinely removed: survivor MUST_RESOLVE, removed MUST_MISSING.

Provider history asymmetry alone may not choose a winner.

## 15. E11 — pristine Extrude Edge ontology

For E01 classify all 12 material Edges semantically.

Required classes include meanings equivalent to:

- ProfileCap x Side(ProfileUse);
- ExtentCap x Side(ProfileUse);
- Side(ProfileUse A) x Side(ProfileUse B).

Every Edge must be accounted.

Expected durable-evidence candidates that have unique semantic provenance — MUST_RESOLVE.

Forbidden: edge index, edge length ranking, XYZ order, provider traversal order.

## 16. E12 — Edge inheritance / trim / delete / split

Carry a semantic Edge through chained Add/Cut.

Required cases:

- unchanged Edge — MUST_RESOLVE;
- uniquely trimmed descendant preserving meaning — MUST_RESOLVE;
- deleted Edge — MUST_MISSING;
- singular Edge split into multiple valid descendants — MUST_AMBIGUOUS;
- new Boolean material intersection Edge — must be accounted and semantically classified.

Failure: first/longest/nearest split fragment becomes Resolved.

## 17. E13 — multiple intersection branches

Construct two semantic carrier Surfaces whose geometric intersection yields multiple disconnected Curve/Edge candidates.

Expected selector containing only the two carrier meanings — MUST_AMBIGUOUS for a singular Curve/Edge target.

If evidence proves a separate semantic branch discriminator is required, record it as a D2 production-design finding.

Failure: closest, shortest or provider-first branch wins.

## 18. E14 — seam Edge classification

Use periodic geometry.

Every provider seam Edge is accounted.

Expected semantic meaning — REPRESENTATION_ARTIFACT unless an independent accepted authored role exists.

Attempt to treat seam as ordinary material design Edge — MUST_UNSUPPORTED.

## 19. E15 — Vertex ontology

For the E01 prism account all 8 Vertices using semantic relationships such as Curve endpoint role and/or intersection of semantic material Edges.

Every Vertex must be accounted.

Coordinates are diagnostic only.

Stable uniquely proven semantic Vertex meanings — MUST_RESOLVE.

## 20. E16 — Vertex movement / deletion / replacement

Required cases:

- same semantic Vertex moves after upstream dimension edit — MUST_RESOLVE;
- semantic Vertex removed — MUST_MISSING;
- unrelated new Vertex appears at same XYZ — old meaning remains MUST_MISSING;
- generated new Vertex with new provenance — separately accounted.

Failure: XYZ equality becomes identity.

## 21. E17 — deterministic cap frames

Cover:

- OneSide Forward;
- OneSide Reverse;
- Midplane.

Cover semantic caps:

- ProfileCap;
- ExtentCap;
- NegativeCap;
- PositiveCap.

Expected canonical frame is derived algebraically from source Profile/support frame plus semantic extent/cap role.

Frame must be stable across recompute and cold rebuild.

Raw provider UV orientation — MUST_UNSUPPORTED as frame authority.

## 22. E18 — deterministic planar lateral frame

For a planar side generated from a straight Profile boundary, evidence must explicitly freeze candidate rules for:

- Origin O;
- +U;
- +V;
- +N;
- material side sense.

Required stability:

- line length edit;
- translated Profile geometry;
- legal loop traversal reversal;
- Add/Cut trimming;
- cold rebuild.

Any 180-degree flip not explained by authored semantic change is failure.

## 23. E19 — Cut-exposed planar carrier frame

Create a Cut whose tool Surface becomes a current material planar Face.

Expected:

- tool-derived semantic Surface — MUST_RESOLVE when provenance is unique;
- deterministic carrier frame survives cold rebuild;
- admissible prospective Sketch support.

Failure: architecture works only for inherited upstream Faces.

## 24. E20 — complete topology after every Feature stage

Use a multi-stage sequence such as Add -> Add -> Cut -> Cut.

After every successful stage independently assert:

- Face count/accounting;
- Edge count/accounting;
- Vertex count/accounting;
- carrier classification;
- no unexplained topology.

Final-Body-only accounting is insufficient.

## 25. E21 — cold rebuild

Representative stable, split, deleted, seam and ambiguity cases from E01-E20 are replayed after all runtime provider objects/tokens/caches are destroyed.

Required:

- same semantic selector meanings;
- same Resolved/Missing/Ambiguous/Unsupported outcomes;
- same canonical carrier frames;
- complete accounting remains complete.

Runtime token numeric equality is irrelevant.

## 26. E22 — runtime token / stale-generation isolation

Runtime Face/Edge/Vertex evidence must not remain authoritative across:

- DocumentRevision change;
- runtime session replacement;
- evaluation-generation replacement;
- provider teardown.

Any stale selection/evidence attempt — STALE_REJECTED.

Token reuse may never cause semantic acceptance.

## 27. E23 — geometry-similarity trap

Construct decoys with same or near-identical:

- plane;
- radius;
- length;
- XYZ;
- area/shape class.

Expected unrelated old selector — MUST_MISSING or MUST_AMBIGUOUS according to semantic provenance, never Resolved from similarity alone.

A surviving semantic target remains MUST_RESOLVE even if a decoy is geometrically closer to its prior geometry.

## 28. E24 — prospective dynamic Sketch support

Evidence-only simulation:

```text
Surface semantic selector
-> resolved current carrier frame
-> unchanged local Sketch U/V geometry
-> world Sketch geometry
```

After an upstream edit that moves the stable carrier:

- Surface — MUST_RESOLVE;
- local U/V geometry — unchanged;
- world placement — changes with carrier;
- no authored SketchPlacement mutation is required.

If support carrier becomes Missing/Ambiguous, prospective Sketch resolution follows that status and no stale frame is current truth.

## 29. E25 — bounded dependency-cycle rejection

Using current ordered Feature history and existing Sketch/Profile consumption relationships, simulate re-support of a consumed Sketch onto a Surface that first exists at or after its consuming Feature.

Expected — MUST_UNSUPPORTED as an admissible dependency / explicit CycleDependency rejection before mutation.

No universal dependency graph is required.

## 30. Checkpoint mapping

- PM-02P.A: matrix + inventory harness.
- PM-02P.B: E01-E10 + E17-E19.
- PM-02P.C: E11-E14.
- PM-02P.D: E15-E16.
- PM-02P.E: E20-E25.
- PM-02P.F: synthesis, survival matrix and exact-head FULL.

## 31. Final PASS metrics

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

All MUST_RESOLVE rows must resolve.

A provider contradiction is not an automatic failure of the project; it is a mandatory STOP and architecture finding. The matrix may be amended only through explicit accepted reasoning, never to hide inconvenient provider behavior.
