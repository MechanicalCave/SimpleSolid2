# PM-02P.A — Complete Topology Inventory Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-03  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact tested source:** `0c460b2d97551546b9a8e61b72efecd11933bf8e`  
**Windows FULL:** #1376 — PASS  
**Merged main:** `9e43ac5cb5f4c941b435672c5bc77d98bb237d60`

## 1. Scope

Checkpoint A establishes the bounded evidence harness for complete Body topology accounting before semantic Face/Surface, Edge/Curve or Vertex/Point promotion.

It deliberately does not freeze a production topology API, durable selector layout, persistence schema or user-facing picking behavior.

## 2. Implemented evidence boundary

The checkpoint adds evidence-only types under the pre-existing PM-00A evidence channel:

- `EvidenceTopologyAccountingClass`;
- `EvidenceTopologyRecord`;
- `EvidenceTopologyKindInventory`;
- `BodyTopologyInventoryEvidence`.

The OCCT evidence adapter independently records:

- provider explorer occurrence count;
- provider unique-subshape count via an indexed shape map;
- one neutral catalog record per unique Face, Edge and Vertex.

No TopoDS handle, provider ordinal or runtime topology token crosses the neutral evidence interface.

## 3. Baseline result

The frozen E01 rectangular Extrude baseline proves:

```text
unique Faces    = 6
unique Edges    = 12
unique Vertices = 8

catalog Faces    = 6
catalog Edges    = 12
catalog Vertices = 8

unaccounted = 0
integrity_failure records = 0
```

The probe reconstructs the same neutral inventory from the declared Profile input in an independent second evaluation.

Full semantic cold-process reconstruction remains owned by E21; checkpoint A does not overclaim that result.

## 4. Verification

Windows PR gate #1376 passed on exact source `0c460b2d97551546b9a8e61b72efecd11933bf8e`.

The run passed:

- documentation dispatcher;
- bootstrap and verification self-tests;
- complete desktop build graph;
- core-only verification;
- kernel-native Release build/test including `pm02p.a_topology_inventory`;
- FAST and SUBSYSTEM selector verification;
- complete desktop FULL tests;
- SR-02 latency evidence;
- CI-04 warm FULL parity and comparative timing evidence.

The exact tested content was squash-merged as main `9e43ac5cb5f4c941b435672c5bc77d98bb237d60`.

## 5. What A proves

A proves that the PM-02P evidence path can account for every unique Face, Edge and Vertex of the canonical simple Extrude without modifying the production solid-modeling contract.

It also proves that the repository verification topology accepts the new evidence test without adding a new public test tier.

## 6. What A does not prove

A does not yet prove:

- which Faces are semantic Surface carriers;
- cap/side carrier provenance;
- planar/cylindrical carrier classification;
- deterministic carrier frames;
- trim/split/delete/merge behavior;
- material Edge versus seam classification;
- durable Edge/Vertex semantic meaning;
- stage-aware topology across chained Features;
- production topology selection or Sketch support.

Those remain owned by later PM-02P checkpoints.

## 7. Next

Proceed to PM-02P.B Face/Surface lineage evidence.

Implementation may be divided into bounded internal slices. The first slice should prove pristine Extrude Face-to-Surface coverage, semantic carrier classification and deterministic cap/planar-side carrier-frame candidates before Boolean lineage is added.
