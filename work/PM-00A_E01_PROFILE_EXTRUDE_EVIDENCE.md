# PM-00A — E01 Profile-to-Extrude Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-02  
**Exact source candidate:** `476258b74751a251c1c4fdf7dabaa03b3da63976`  
**Windows FULL:** #1228 — PASS  
**Regression:** `pm00a.e01_extrude_evidence`  
**Matrix:** `work/PM-00A_E01_E10_EVIDENCE_MATRIX.md` v1.0

## Result

E01-01 through E01-06 PASS.

False-Resolved count: **0**.

The evidence demonstrates that a Rectangle outer loop plus one circular hole can be converted from durable Part/Profile intent into exact neutral Kernel boundary uses, extruded by the bounded OCCT evidence provider, and interpreted through semantic producer roles without Face[n]/Edge[n], Viewer tokens or geometry-similarity identity.

## Evidence summary

| Row | Expected | Actual | Cold | Result |
| --- | --- | --- | --- | --- |
| E01-01 | one valid solid; unique start/end caps | one valid B-Rep solid; start/end cap each uniquely Resolved | YES | PASS |
| E01-02 | four outer sides Resolved from exact Profile boundary-use provenance | four outer sides uniquely Resolved from the four authored Line EntityIds / boundary uses | YES | PASS |
| E01-03 | inner cylindrical side Resolved from Circle-hole provenance | one hole side uniquely Resolved from the authored Circle EntityId / boundary use | YES | PASS |
| E01-04 | Extrude distance edit preserves semantic roles | distance 10 -> 25 preserves cap roles and all side provenance/status | YES | PASS |
| E01-05 | rectangle dimension edit with same Line EntityIds preserves side roles | 40x30 -> 60x45 keeps the same four source EntityIds and all corresponding sides Resolved | YES | PASS |
| E01-06 | removing source Circle must not retarget to another circular face | Profile resolves to `missing_source_entity`; no neutral Kernel input/provider replacement is fabricated | YES | PASS |

## Provider finding

The investigation produced an important provider-boundary finding.

A source `TopoDS_Edge` constructed before insertion into `BRepBuilderAPI_MakeWire` is not a reliable transient key for later sweep lineage. The builder may replace/copy the edge while reconciling geometrically coincident vertices.

The working evidence path therefore retains the exact transient edge reported by the completed `MakeWire` operation and uses that edge only inside the OCCT provider when querying the concrete `BRepSweep_Prism`.

This does **not** promote the OCCT edge/handle to SS2 semantic or durable identity.

The durable/semantic meaning remains:

```text
Profile boundary-use provenance
        ↓
provider transient exact basis edge
        ↓
BRepSweep_Prism generated side evidence
        ↓
unique candidate count
        ↓
Resolved / Missing / Ambiguous
```

Provider handles are discarded with evaluated runtime state.

## Failed approaches retained as evidence

The following approaches were explicitly tested and rejected:

1. high-level `BRepPrimAPI_MakePrism::Generated(edge)` using the pre-wire constructed edge — no side candidate for the first outer Line;
2. locating an `IsSame()` edge in the built face and querying high-level Generated history — still no candidate;
3. using the concrete sweep through the high-level wrapper — API constness prevented the required lookup in the runner's OCCT version;
4. direct mutable `BRepSweep_Prism` with the pre-wire edge — still no candidate;
5. treating a generated root `TopoDS_Face` as if face counting should only traverse child faces — produced a false zero count.

The accepted probe uses the exact edge accepted by `MakeWire`, direct mutable `BRepSweep_Prism`, and root-aware generated-shape counting.

No geometry-proximity fallback, topology ordinal, “first face”, longest-edge or nearest-face heuristic was introduced.

## Architecture implication

E01 supports the PM-00A hypothesis that semantic side roles can be based on Profile boundary-use provenance while provider lineage remains transient evidence.

It also shows that provider lineage collection must be attached to the actual provider operation boundary. Reconstructing lineage later from superficially equivalent provider shapes is not reliable enough to become SS2 identity.

This is evidence for O-05. It does not freeze the durable selector schema; PM-00B owns that decision.

## Scope boundary

E01 introduced no:

- BodyId / FeatureId persistence;
- Part Feature Tree;
- product Extrude command or UI;
- persisted B-Rep;
- persisted OCCT handles;
- persistent topology-reference schema;
- similarity-based automatic identity.

The next matrix package is **E05 — geometrically similar topology / false-positive protection**.
