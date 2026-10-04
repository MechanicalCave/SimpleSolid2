# PM-02P.D — Vertex / Point Lineage Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-04  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact tested source:** `b88cbfd72e6fd8d736358156bbfa433612aa5378`  
**Windows FULL:** #1383 — PASS  
**Merged main:** `8b27e1aaa2060c8e3d3c1bdeca62e7370d721e80`

## 1. Result

PM-02P.D proves Vertex/Point semantic meaning for the current Extrude evidence universe without introducing a production VertexReference or PointReference type.

## 2. Proven outcomes

The exact-head evidence proves:

- canonical rectangular Extrude: all 8 Vertices are completely accounted and semantically referenceable;
- each pristine Vertex meaning is the intersection of one semantic cap Surface and two semantic side Surfaces with provenance;
- XYZ is diagnostic only and does not participate in semantic identity;
- an upstream dimension edit moves a stable Vertex while the same semantic carrier intersection remains Resolved;
- a chamfer-like Profile edit removes the former bottom/left Vertex -> Missing;
- the same edit creates two new Vertices with new chamfer provenance -> separately Resolved;
- the edited 10-Vertex solid remains completely accounted;
- an unrelated replacement solid can place a Vertex at exactly the old XYZ while the old semantic key remains Missing and the replacement semantic key resolves;
- cold reconstruction reproduces the same neutral outcomes.

## 3. Architecture consequence

Point/Vertex semantics can be expressed from semantic carrier/curve relationships rather than coordinates. Production PM-02 must preserve this asymmetry: moved stable provenance may remain Resolved; coincident replacement geometry must not steal identity.

## 4. Verification

Windows PR gate #1383 passed on exact source `b88cbfd72e6fd8d736358156bbfa433612aa5378`.

The tested content was squash-merged as main `8b27e1aaa2060c8e3d3c1bdeca62e7370d721e80`.

## 5. Next

Proceed to PM-02P.E lifecycle/freshness evidence E20-E25.

No production topology API, persistence/schema change, Viewer picking, face-supported Sketch, Datum or Projection is authorized by D.
