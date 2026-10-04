# PM-02P.C — Edge / Curve Lineage Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-04  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact tested source:** `c19efc5d45f1489d433fd75d25f4e9035737f91b`  
**Windows FULL:** #1382 — PASS  
**Merged main:** `484542d598f621dc17a9cea60fd068cf5ab62408`

## 1. Result

PM-02P.C proves the accepted Edge/Curve semantics for the current Extrude Add/Cut evidence universe without introducing a production EdgeReference or CurveReference type.

## 2. Proven outcomes

The exact-head evidence proves:

- canonical rectangular Extrude: all 12 material Edges are accounted and referenceable;
- 8 cap/side and 4 side/side semantic Edge roles are determined from semantic Surface adjacency rather than provider ordering;
- provider curve classification corroborates semantic meaning but does not define identity;
- unchanged Edge -> Resolved;
- uniquely trimmed Edge -> Resolved;
- singular Edge split into multiple valid descendants -> Ambiguous;
- removed Edge -> Missing;
- Boolean-created material intersection Edge is explained by semantic carrier intersection and is absent from both source B-Reps;
- one pair of semantic Surfaces yielding multiple disconnected current branches -> pair-only singular meaning Ambiguous;
- periodic cylindrical seam is fully accounted as KnownRepresentationArtifact and remains Unsupported as an ordinary material design Edge;
- successful C Boolean results preserve complete Face/Edge/Vertex accounting;
- cold reconstruction reproduces the neutral outcomes.

## 3. Architecture consequence

A durable production Edge meaning may be built from semantic carrier/curve provenance and branch semantics, but must not use Edge order, XYZ ordering, length ranking, nearest geometry or provider-first selection.

E13 confirms that a Surface-pair alone is insufficient when more than one disconnected branch exists. Production PM-02 therefore needs an explicit branch/provenance discriminator wherever a singular Curve/Edge must distinguish such branches.

This is a D2 production-design finding to carry into PM-02 synthesis; it does not require changing ADR-0016.

## 4. Verification

Windows PR gate #1382 passed on exact source `c19efc5d45f1489d433fd75d25f4e9035737f91b`.

The tested content was squash-merged as main `484542d598f621dc17a9cea60fd068cf5ab62408`.

## 5. Next

Proceed to PM-02P.D Vertex/Point lineage, bounded to frozen E15-E16.

No production topology API, persistence/schema change, Viewer picking, face-supported Sketch, Datum or Projection is authorized by C.
