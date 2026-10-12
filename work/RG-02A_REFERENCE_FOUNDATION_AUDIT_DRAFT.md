# RG-02A — Stage-aware Body point/reference foundation: technical audit (DRAFT)

**Status:** DRAFT technical evidence, NOT implementation authority  
**Date:** 2026-10-12  
**Baseline:** `main` at `fa0f8074dc7bb0eb5e2c45ab0fa313644febf55c`  
**Parent design draft:** `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` (mandatory Owner UX approval before any UI work)

## 1. Audited current production contracts

1. `src/part/include/simplesolid2/part/semantic_topology_reference.hpp`: `BodyStageRef` gives an explicit after-Feature history stage. `MaterialEdgeReference` stores this stage, semantic `FeatureCurveAddress` and a strict bounded-edge branch discriminator (`SingularAtAuthoredStage`, `BetweenSemanticPoints`, `AtSingleSemanticPoint`). `FeaturePointAddress` presently represents ONLY the intersection of exactly **three** distinct semantic Surfaces; it is not a universal identifier for every visible Body Vertex.
2. `src/part/include/simplesolid2/part/feature_evaluation.hpp` and `src/part/feature_evaluation.cpp`: `BodyStageTopologyCatalog` exposes current semantic Surface/Curve/Point classifications and runtime realization evidence. `authorMaterialEdgeReference` / `resolveMaterialEdgeReference` already enforce strict stage-scoped bounded material Edge meaning and fail closed on missing/ambiguous/unsupported cases. `semanticPointForRuntimeVertex` can certify a current Vertex only when exactly one referenceable semantic Point resolves to exactly that Vertex; ordinary visual Vertex picking alone is NOT a promise of durable referenceability. `authorProjectedMaterialEdgeReference` contains a special one-certified-Point branch fallback for PG-01D, not general permission to silently rebind.
3. `src/kernel_occt/solid_modeling_kernel.cpp`: production OCCT holds exact native `TopoDS_Edge`/`TopoDS_Vertex`, uses `BRepAdaptor_Curve` and `BRepAdaptor_Surface`, classifies `GeomAbs_Line`/`GeomAbs_Circle` and `GeomAbs_Cylinder`, and extracts exact provider Vertex XYZ via `BRep_Tool::Pnt`. Projection code already interprets analytic straight edges/circular edges but is scoped to **edge projection into a target sketch frame**; it is not a general stage-scoped 3D MID/CENTER point query.
4. `src/kernel/include/simplesolid2/kernel/solid_modeling.hpp`: provider-neutral SurfaceKind/CurveKind and runtime tokens exist; `BodyPresentation` exports display-only sampled Edge paths and Vertex locations. There is no dedicated provider-neutral exact Body-Edge-derived point/axis inspection contract yet. Display polyline midpoints and tokens may NEVER become authored truth.
5. `src/ui/part_viewport_controller.cpp`: current Body faces, edges and vertices have native picking/preselection; ordinary pickability excludes some representation artifacts/integrity failures but does not automatically imply strict semantic referenceability. Sketch `SnapKind::midpoint` and `SnapKind::center` in `src/sketch/snap.cpp` apply to Sketch geometry, not to arbitrary Body edges in 3D. **Do not implement UI marker or picking changes before separate Owner UX approval**.
6. Existing regression footholds: `tests/pm05b2_strict_edge_resolver_test.cpp`, `tests/pm03b_datum_evaluation_test.cpp`, and `work/PM-02P_REFERENCE_SURVIVAL_MATRIX.md`. Existing tests are evidence of old contracts, NOT proof that every RG-02A point family is already supported.

## 2. Specific gaps requiring bounded new contracts

| User intent | Reusable current mechanism | Missing / to prove |
|---|---|---|
| VERTEX | Semantic `FeaturePointAddress` / Point resolution, exact OCCT Vertex | Source authoring/resolution API for RG-02; acceptance matrix for clicked Vertices not expressible as three Surface intersection Points; no fallback to screen XYZ |
| MID — straight bounded material Edge | Strict `MaterialEdgeReference`, analytic line recognition | Exact on-Edge half-length world point query (fresh scoped Body), persisted derived-point role, native geometry/identity tests |
| MID — open circular arc material Edge | Strict Edge reference, analytic circle/arc recognition | Exact half-**arc-length** point with proper trim and orientation; exclude full periodic circle from MID |
| CENTER — open arc or closed circle | Strict Edge reference, analytic circle | Exact circle center (which generally is **not** on the Edge); stable reference role; do not confuse with topological Vertex, centroid or arc MID |
| History/cold rebuild | `BodyStageRef`, semantic catalog, Part reevaluation | Derived-point source recomputation for correct history stage, typed unresolved statuses, persistence/version decision, no stale last-good or provider token rebinding |
| Viewport snap/marker | Existing Body picking, Sketch snap experience | UI behavior and marker priority/visibility to be designed with Owner first; do not treat visual pickability as semantic acceptance |

## 3. Proposed architecture direction (NOT YET accepted technical D2)

- Retain `FeaturePointAddress` meaning for existing true semantic three-Surface Vertices, and `MaterialEdgeReference` meaning for strict bounded material Edges. Do not silently reinterpret, weaken, or overload existing persisted semantic references.
- Introduce a **Part-owned typed construction-point source** as an extension (conceptual only): a resolvable Vertex source OR an `Edge-derived MID` source OR an `Edge-derived CENTER` source. Edge-derived variants persist the strict semantic `MaterialEdgeReference`, source kind, and stage via the Edge; they do **not** persist numeric coordinates. A provider-neutral runtime query computes current 3D coordinates/analytic curve metadata only after stage-bound semantic resolution.
- For 3D Vertex sources, first collect a real native sample matrix across Extrude, Revolve, Cut, Fillet, Chamfer and inherited/split carriers. **STOP and request Owner D2** if normal clicked endpoints cannot be given durable identity under the existing three-Surface Point contract; decide separately whether/how to broaden Vertex identity without breaking Part schema.
- An exact query must reject unsupported curves, degeneracy, periodic full-circle MID, missing stages, aliasing and stale runtime handles. Do not use midpoint of sampled `BodyEdgePresentationPath`, nearest-match repair, OCCT ordinal, or UI token persistence.
- A construction tool should not accept a pick merely because the Body Viewer can highlight it. On Create/Finish and cold rebuild, assert source is semantically authorable and uniquely resolvable.
- Do not make a separate Datum Point domain object. Datum Plane and Axis eventually consume these typed derived source values.

## 4. Evidence-first acceptance plan (proposal only)

1. **G0 current native reality:** fixture matrix of sample bounded line Edges, open arcs, full circles, real 3-Surface Vertex and non-three-Surface Vertex, with supported vs unsupported authoring outcomes; document provider exactness and one-stage selection limitations.
2. **G1 semantic source & cold truth:** prove same `VERTEX/MID/CENTER` source resolves deterministically after upstream dimensional edits, stays tied to pre-Chamfer stage under a later Chamfer, and returns Missing/Ambiguous/Unsupported without geometry fallback under appropriate edits. Strict resolver, generation and provider-collision negatives.
3. **G2 exact kernel geometry:** native OCCT Line MID, open Arc half-length MID, open/closed Circle CENTER; reject full-circle MID and nonanalytic/degenerate curves. Cover trim boundaries, orientation and exact-source scope.
4. **G3 persistent lifecycle:** after an accepted schema/contract change, prove Save/Reopen/cold rebuild and Undo/Redo for authored dependent Datums/Axis, including broken-source edit/repair. Do not claim full vertical slice before downstream tooling exists.
5. **G-UI separately gated:** provide UX wireframes/interaction scenarios and receive separate explicit Owner approval **before** creating MID/CENTER Body snap markers, highlighting, pick-priority changes, Tree/Operations behavior, or user-facing CAD Input changes.

These are prospective gates, not approval to edit production code or tests. Exact provider-neutral method signature, persistence schema migration and supported VERTEX families are open D2 architecture decisions.

## 5. Immediate recommended Owner checkpoint

**First settle scope of VERTEX identity after native G0 proof**, not a speculative universal point framework. The accepted product expectation is that normal visible Body endpoints should be usable where defensibly semantically referencable; if current three-Surface representation cannot certify them, report the exact case and ask for a bounded D2 extension. Do not silently mark all viewport-pickable Vertices as supported.

## Documentation impact

Internal docs: required upon accepted implementation of durable point-source/Kernel API semantics.  
User/Product docs: required upon accepted release of new construction-point UX (PL/EN).  
Draft audit only: no as-built documentation changes, no production claims.
