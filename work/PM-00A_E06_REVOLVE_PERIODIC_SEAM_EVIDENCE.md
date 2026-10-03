# PM-00A — E06 Full-Revolve / Periodic-Seam Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-02  
**Exact source candidate:** `44eda36ba6bdc19d8c252831937fb86f8afabde7`  
**Kernel-focused:** #1305 — PASS  
**Windows FULL:** #1306 — PASS  
**Regression:** `pm00a.e06_revolve_periodic`  
**Matrix:** `work/PM-00A_E01_E10_EVIDENCE_MATRIX.md` v1.0

## Result

E06-01 through E06-04 PASS.

E07-06 also PASS.

False-Resolved count: **0**.

The evidence demonstrates that a supported closed Profile can be revolved through a full 360 degrees into one valid solid while semantic side identity remains attached to exact Profile boundary-use provenance. Provider-created periodic seam topology is observed as diagnostics only and is never promoted to semantic identity.

## Evidence summary

| Row | Expected | Actual | Cold | Result |
| --- | --- | --- | --- | --- |
| E06-01 | full 360° Revolve produces a valid expected B-Rep | one valid B-Rep solid; all four boundary-generated faces uniquely Resolved | YES | PASS |
| E06-02 | semantic revolved side resolves from Profile boundary-use provenance | outer cylindrical side is uniquely Resolved from the `outer` boundary use | YES | PASS |
| E06-03 | provider-created periodic seam is not a durable semantic target | real seam edges are detected, while seam reference status is `Unsupported` | YES | PASS |
| E06-04 | upstream dimension edit changes seam representation but preserves semantic side | height/radius edit changes seam length while the same outer provenance remains uniquely Resolved | YES | PASS |
| E07-06 | cold rebuild preserves semantic revolved side and non-semantic seam | first-pass provider/B-Rep state is destroyed; rebuilt evidence reproduces the same semantic snapshot | YES | PASS |

## Provider observations

The bounded OCCT provider uses a full `BRepSweep_Revol` over the exact transient Profile face.

For each Profile boundary use:

```text
Profile boundary-use provenance
        ↓
exact transient basis edge
        ↓
full Revolve provider sweep
        ↓
generated face cardinality
        ↓
Resolved / Missing / Ambiguous
```

A uniquely generated face is associated with the source boundary-use provenance. The evidence does not use face ordinals, traversal order, proximity or surface similarity as identity.

Periodic seam topology is detected with provider diagnostics on the generated periodic face. The seam is real provider topology, but it has no authored source/provenance under PM-00A. Therefore:

```text
provider periodic seam
→ observable diagnostic
→ ReferenceStatus::Unsupported
```

It is never `Resolved`.

## Dimension-edit evidence

The accepted edit changes:

- Profile height from 10 to 15;
- outer radius from 20 to 25;
- inner radius remains 10.

The actual seam representation changes: the seam length differs from the baseline.

Despite that provider-topology change, the semantic outer side keeps:

- the same Profile boundary-use provenance;
- `Resolved` status;
- candidate cardinality = 1;
- periodic surface diagnostics.

This demonstrates that seam placement/geometry cannot define revolved-side identity.

## E07-06 cold replay

The Revolve scenario is evaluated once and all provider sweep/B-Rep/TopoDS state leaves scope.

The same legal neutral Profile + evidence-only axis input is then reconstructed and reevaluated.

Required and actual result:

```text
same outer semantic provenance
same Resolved status
same candidate cardinality
periodic seam remains Unsupported
provider seam diagnostics reconstructed from scratch
```

The edited-dimension case is also cold-replayed with the same semantic outcome.

This completes E07-06 and therefore closes the full E07 cold-rebuild matrix.

## Axis scope

E06 passes the Revolve axis as neutral local 2D origin + direction solely for the bounded evidence probe.

This is **not** a durable Axis/Datum schema and does not activate O-02/O-07 product semantics.

## Architecture implications

E06 strengthens O-05 and the later O-06 operation-scope decision:

1. semantic revolved-side identity can be expressed from producer Profile boundary provenance without seam identity;
2. provider periodic seams are explicitly non-semantic;
3. supported semantic outcome survives full provider teardown/rebuild;
4. upstream geometry changes may alter seam representation without changing semantic identity;
5. provider topology remains reconstruction evidence only.

PM-00B still owns the durable selector/schema and production Revolve architecture freeze.

## Scope boundary

E06 introduced no:

- persistent topology-reference schema;
- durable Axis/Datum object;
- BodyId / FeatureId persistence;
- Part Feature Tree;
- product Revolve command or UI;
- persisted B-Rep/provider seam;
- topology ordinal identity;
- geometry-similarity fallback.

**Next:** E08 support-frame stability evidence.
