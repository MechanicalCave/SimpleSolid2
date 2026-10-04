# PM-02P — Reference Survival Matrix

**Status:** ACCEPTED PM-02P D2 SYNTHESIS INPUT — 2026-10-04  
**Date:** 2026-10-04  
**Authority:** ADR-0014 + ADR-0016 + `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact runtime evidence source:** `e259fef5849bc4707ddcbdda8e75795b8f3fa5b4` — Windows FULL #1387 PASS  
**Scope:** current single-Body ordered Extrude Add/Cut evidence universe

## 1. Purpose

This matrix converts PM-02P evidence into explicit reference-survival expectations for production PM-02.

It is semantic, not a persisted selector schema.

The statuses are:

- **Resolved** — exactly one accepted semantic meaning exists;
- **Missing** — the semantic meaning no longer exists;
- **Ambiguous** — more than one accepted candidate/interpretation remains;
- **Unsupported** — the requested semantic meaning is outside the accepted model;
- **RepresentationArtifact** — provider topology is accounted but is not ordinary authored material topology;
- **StaleRejected** — runtime evidence no longer has authority.

## 2. Surface / Face survival

| Semantic target | Change | Strict Face | Surface carrier | Frame | Production rule |
| --- | --- | --- | --- | --- | --- |
| Extrude ProfileCap | extent distance edit | Resolved when singular | Resolved | Recomputed deterministically | Stage + feature/cap role, never position |
| Extrude ExtentCap | extent distance edit | Resolved when singular | Resolved | Origin translated by semantic offset | Geometry movement does not change meaning |
| Midplane NegativeCap / PositiveCap | total distance edit | Resolved when singular | Resolved | Recomputed from source frame + signed offset | Cap role distinguishes meanings |
| planar Line side | line length edit | Resolved when singular | Resolved | Stable semantic orientation | Authored Line direction is frame provenance |
| planar Line side | Profile translation | Resolved | Resolved | Translates with authored source | No world-axis special case |
| planar Line side | legal loop traversal reversal | Resolved | Resolved | No semantic flip | Traversal is representation, not identity |
| planar carrier | attached Add trims boundary | Resolved when one bounded realization | Resolved | Unchanged carrier frame | Boolean trim does not invent new Surface |
| planar carrier | Cut trims boundary | Resolved when one bounded realization | Resolved | Unchanged carrier frame | Same carrier meaning |
| planar carrier | one Face splits into two | **Ambiguous** | **Resolved** | Unchanged carrier frame | Face and Surface identity levels differ |
| planar carrier | complete deletion | Missing | Missing | No current frame | No stale last-good truth |
| deleted planar carrier | unrelated identical plane later appears | Missing | Missing | Old frame non-authoritative | Geometry equality cannot rebind |
| replacement planar carrier | created by new provenance | Resolved when unique | Resolved | New provenance-derived frame | New meaning, not revival |
| two semantic Faces collapse to one provider Face | no independent winner | Ambiguous | Ambiguous | No arbitrary frame winner | Provider history asymmetry insufficient |
| two claims collapse, one independently survives | survivor / removed | Resolved / Missing | Resolved / Missing | Survivor frame remains semantic | Independent provenance decides |
| Cut tool planar side exposed into Body | Cut creates material boundary | Resolved when singular | Resolved | Tool provenance frame | Production cannot handle only inherited Faces |
| cylindrical Extrude side | stable edit | Resolved when singular | Resolved | no planar Sketch frame | Non-planar support Unsupported |
| cylindrical seam provider Face/Edge detail | periodic representation | n/a | carrier still Resolved | n/a | Seam not promoted to material design intent |

## 3. Edge / Curve survival

| Semantic target | Change | Edge result | Curve/carrier meaning | Production rule |
| --- | --- | --- | --- | --- |
| ProfileCap x Side(ProfileUse) | unchanged | Resolved | Resolved | Semantic Surface adjacency |
| ExtentCap x Side(ProfileUse) | unchanged | Resolved | Resolved | Cap role + side provenance |
| Side(ProfileUse A) x Side(ProfileUse B) | unchanged | Resolved | Resolved | Adjacent semantic side carriers |
| material Edge | unique trim | Resolved | Resolved | Provider Modified history may corroborate |
| material Edge | complete delete | Missing | Missing | No nearest replacement |
| material Edge | split into multiple valid descendants | **Ambiguous** | branch family still semantically explained | No first/longest/nearest fragment |
| Boolean-created intersection Edge | unique carrier intersection branch | Resolved | Resolved | Meaning comes from operation/carrier provenance |
| Surface A x Surface B | two disconnected current branches | **Ambiguous** for singular target | pair meaning insufficient | Add explicit semantic branch/provenance discriminator |
| periodic cylindrical seam Edge | provider representation | **RepresentationArtifact** | ordinary material Edge Unsupported | Account but do not persist as design Edge |
| deleted Edge | unrelated equal-length Line Edge appears | Missing | old Curve meaning Missing | Length/type equality cannot rebind |

## 4. Vertex / Point survival

| Semantic target | Change | Vertex result | Point meaning | Production rule |
| --- | --- | --- | --- | --- |
| prism semantic Vertex | unchanged | Resolved | Resolved | Intersection of semantic carrier relationships |
| same Vertex | upstream dimensions move geometry | **Resolved** | Resolved | XYZ may change |
| semantic Vertex | source relation removed | Missing | Missing | No coordinate fallback |
| chamfer-like edit | two new endpoint/intersection meanings | Resolved as new Vertices | Resolved as new Points | New provenance is explicit |
| removed Vertex | unrelated replacement appears at exact same XYZ | **Missing** | old Point Missing | XYZ equality is diagnostic only |
| replacement Vertex at same XYZ | new provenance | Resolved | Resolved | New meaning, not revival |

## 5. Sketch-support survival

For PM-02, a standard face-supported Sketch consumes a **planar semantic Surface**, not a strict bounded Face.

| Support event | Surface status | Current Sketch frame | Authored local U/V | Required behavior |
| --- | --- | --- | --- | --- |
| support unchanged | Resolved | derived current frame | unchanged | normal evaluation |
| upstream edit moves stable carrier | Resolved | moves with carrier | unchanged | no authored world-placement mutation |
| bounded Face trim | Resolved | same semantic carrier frame | unchanged | Sketch remains attached |
| bounded Face split | Resolved Surface despite strict Face Ambiguous | same carrier frame | unchanged | Sketch remains attached to semantic plane |
| support Surface deleted | Missing | none | unchanged | downstream structured failure/blocked; no stale frame |
| support Surface ambiguous | Ambiguous | none | unchanged | repair required; no arbitrary candidate |
| selected Face is cylindrical/non-planar | Surface may be Resolved | no planar Sketch frame | unchanged | Sketch admission Unsupported |
| Cut-exposed planar tool Surface | Resolved | provenance-derived frame | authored after creation | valid support |
| support producer is earlier than consuming Feature | Resolved/admissible | derived | unchanged | dependency valid |
| support producer is consuming Feature or later | n/a | none | unchanged | reject as CycleDependency before mutation |

## 6. Runtime selection / freshness

Runtime topology tokens are not durable semantic references.

| Runtime event | Old Face token | Old Edge token | Old Vertex token | Rule |
| --- | --- | --- | --- | --- |
| same revision/session/evaluation/provider generation | current | current | current | may reach semantic resolver |
| DocumentRevision advances | StaleRejected | StaleRejected | StaleRejected | old evidence cannot commit |
| canonical DocumentSession replaced | StaleRejected | StaleRejected | StaleRejected | session ownership changed |
| evaluation generation replaced | StaleRejected | StaleRejected | StaleRejected | old evaluated catalog unavailable |
| provider runtime replaced/teardown | StaleRejected | StaleRejected | StaleRejected | provider identity expired |
| numeric token value reused by new generation | old remains StaleRejected | old remains StaleRejected | old remains StaleRejected | number equality has no authority |

## 7. Geometry diagnostics policy

The following values may corroborate or diagnose but may **never** independently produce Resolved:

- plane equation / normal;
- centroid;
- area;
- radius;
- Edge length;
- XYZ;
- bounding box;
- nearest distance;
- first provider result;
- provider traversal index;
- Viewer pick index.

They may expose a conflict or aid diagnostics. They are not semantic identity.

## 8. Cold rebuild invariant

After provider objects, runtime tokens and caches are destroyed, durable semantic intent must reconstruct the same:

- Resolved / Missing / Ambiguous / Unsupported meanings;
- planar carrier frames;
- complete topology accounting;
- support admissibility.

Runtime token numeric equality is explicitly irrelevant.

## 9. Production PM-02 acceptance implication

Production PM-02 tests should use this survival matrix as a minimum semantic regression set.

The production implementation may refine type layout and diagnostics, but it must not weaken any survival result above without an explicit D2 decision superseding ADR-0016.
