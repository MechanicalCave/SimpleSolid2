# PM-00A — E02 Multi-Stage Lineage Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-02  
**Exact source candidate:** `953cdca42978916754f6ae6cc68d86352aad35ac`  
**Kernel-focused:** #1285 — PASS  
**Windows FULL:** #1286 — PASS  
**Regression:** `pm00a.e02_multistage_lineage`  
**Matrix:** `work/PM-00A_E01_E10_EVIDENCE_MATRIX.md` v1.0

## Result

E02-01 through E02-06 PASS.

False-Resolved count: **0**.

The evidence demonstrates that multi-stage topology references must resolve in the intended producer/consumed-stage context. Provider history can supply transient lineage/cardinality evidence, but it does not define durable identity and a final-Body-wide geometry search is not an acceptable substitute for semantic context.

## Evidence summary

| Row | Expected | Actual | Cold | Result |
| --- | --- | --- | --- | --- |
| E02-01 | selected base side survives an unrelated Cut | selected side is Resolved at Extrude and Cut stages with one Cut descendant | YES | PASS |
| E02-02 | upstream Extrude distance edit preserves supported stable meanings | independent rebuilds at heights 10 and 20 preserve stage statuses/cardinality without provider-handle continuity | YES | PASS |
| E02-03 | Cut removes referenced semantic face | Cut history reports deletion / zero descendants and Cut-stage reference is Missing | YES | PASS |
| E02-04 | Fillet-style modification remains Resolved only with unique lineage + semantic context | real OCCT Fillet consumes one edge scoped inside the already-resolved Cut face; selected face has one downstream descendant | YES | PASS |
| E02-05 | downstream geometric failure does not rewrite valid input-reference status | thin upstream geometry leaves Cut-stage input Resolved while the unchanged fillet radius produces a separate Geometric failure | YES | PASS |
| E02-06 | producer/consumed stage participates in the semantic address | the same meaning is Resolved at multiple stages; omitting stage is Ambiguous and never manufactures Resolved | YES | PASS |

## Key finding — semantic context bounds downstream selection

The first focused execution, #1284, intentionally exposed a defect in the probe:

```text
whole Cut result
→ search endpoint-matching edge globally
→ more than one candidate
→ E02-04 FAIL
```

The correction does not choose the first, nearest or otherwise geometrically preferred edge. The Fillet input is instead resolved inside the already-resolved semantic Cut-face context:

```text
resolved Cut-face meaning
→ candidate edges belonging to that face
→ exact bounded fixture edge cardinality = 1
→ Fillet input accepted
```

This is the required `lineage + semantic context` behavior. Searching the final Body indiscriminately is rejected.

## Reference resolution versus geometric feasibility

The thin-geometry scenario deliberately preserves the Cut-stage input reference while making the fixed-radius Fillet infeasible.

The accepted result is:

```text
Cut-stage reference = Resolved
Fillet input edge cardinality = 1
downstream operation = Geometric failure
```

Geometric failure therefore does not retroactively become Missing or Ambiguous.

## Producer stage is part of meaning

The evidence records separate stages:

- Extrude output;
- Cut output;
- downstream/Fillet output.

A stage-qualified request can be Resolved at each supported stage. A request that intentionally omits the stage while several live stage meanings exist is Ambiguous, not Resolved.

This supports the PM-00A rule that a reference authored against an earlier producer stage must not be resolved by searching only the final Body result.

## Provider boundary and fixture limits

E02 uses real OCCT Cut and real `BRepFilletAPI_MakeFillet` operations and records provider Modified/Generated/Deleted/unchanged evidence.

The initial bounded “Extrude stage” in this E02 fixture is a deterministic one-solid prism created with `BRepPrimAPI_MakeBox`. It is a representative producer-stage topology fixture only. It is **not** the production Extrude implementation and it does not replace E01, which proves the exact Profile → Extrude provenance boundary.

Known centroid/endpoint lookup helpers are used only to construct deterministic provider test inputs. They are not reference selectors, authored identity or proposed persistent-reference semantics.

Provider B-Rep objects, handles and history objects remain transient and are discarded between cold passes.

## Cold replay

The complete scenario set is evaluated twice. All first-pass B-Rep, Boolean, Fillet, provider-history and TopoDS objects leave scope before the second pass.

The complete neutral `MultiStageLineageEvidence` set compares equal between passes.

PASS confirms that E02 outcomes do not require previous-process/provider object continuity.

## Architecture implications

E02 strengthens O-05 and O-09:

1. **Stage is semantic context.** Producer/consumed stage must participate in resolution.
2. **Context narrows cardinality.** Downstream sub-element resolution occurs inside the already-resolved semantic input context rather than by global Body search.
3. **Provider history is evidence only.** Modified/Generated/Deleted/unchanged supports reconstruction evidence but does not become identity.
4. **Reference status and operation feasibility are separate.** A downstream operation may fail geometrically while its input reference remains Resolved.
5. **Rebuild, not handle continuity.** Supported outcomes survive independent reevaluation and cold replay without prior provider handles.

These are evidence results; PM-00B still owns the durable selector/schema architecture freeze.

## Scope boundary

E02 introduced no:

- persistent topology-reference schema;
- BodyId / FeatureId persistence;
- Part Feature Tree;
- product Extrude, Cut or Fillet command/UI;
- persisted B-Rep/provider history;
- whole-Body geometry-similarity fallback;
- topology ordinal or Viewer identity.

The next PM-00A work is the accumulated **E07 cold-rebuild parity** package for evidence already available from E01/E02/E03/E04/E05. E07-06 remains pending until E06 supplies the Revolve/periodic-seam case; the frozen E07 expectation itself is not changed.
