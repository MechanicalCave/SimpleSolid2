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

E02 proves the accepted E01/E03/E04/E05 reference semantics across a bounded real OCCT multi-stage chain:

```text
Extrude-like producer
    ↓
Cut
    ↓
Fillet-style downstream operation
```

The probe remains evidence-only. It does not introduce Part Body/Feature persistence or product operations.

## Evidence summary

| Row | Expected | Actual | Cold | Result |
| --- | --- | --- | --- | --- |
| E02-01 | base side survives unrelated Cut and remains Resolved at the consumed stage | selected Extrude-side has one Cut descendant and is Resolved on Cut output | YES | PASS |
| E02-02 | upstream extrusion-distance edit preserves accepted stable references without provider-handle continuity | independent rebuild at heights 10 and 20 keeps Extrude/Cut/downstream stage references Resolved | YES | PASS |
| E02-03 | Cut fully removes selected semantic face | provider reports deletion/no descendant and Cut-stage reference is Missing | YES | PASS |
| E02-04 | Fillet-style probe modifies but preserves accepted semantic input | Fillet consumes one edge inside the already Resolved Cut-face context; selected face has one downstream descendant and remains Resolved | YES | PASS |
| E02-05 | downstream geometry can fail while input references remain valid | Cut-stage reference remains Resolved and input edge cardinality remains one while Fillet outcome is Geometric failure | YES | PASS |
| E02-06 | same semantic meaning exists at several producer stages | explicit stage resolution is Resolved; omitting stage is Ambiguous and never silently searches final shape | YES | PASS |

## Producer-stage finding

E02 confirms that producer/consumed stage is part of the semantic address.

The same semantic side meaning can be observable on:

- Extrude output;
- Cut output;
- downstream Fillet output.

A resolver that omits stage and searches the current/final Body would therefore be semantically under-specified. The evidence prototype returns Ambiguous when several live stages satisfy the same role.

The intended rule is:

```text
semantic producer/consumed stage
    + semantic role/provenance
    + fail-closed lineage cardinality
    -> reference status
```

not:

```text
search final Body for something geometrically similar
```

## Reference resolution versus geometric failure

E02-05 proves the required separation between reference status and operation feasibility.

For the thin-upstream scenario:

- the Cut-output semantic face remains Resolved;
- the Fillet input edge candidate count remains exactly one inside that Resolved semantic context;
- the Fillet operation itself is geometrically infeasible;
- downstream result is therefore **Geometric failure**, not Missing or Ambiguous.

This is direct evidence for the PM-00A three-validity-level model and O-09.

## Provider-history finding

OCCT Modified/Generated/Deleted/unchanged observations are used only as transient evidence for descendant cardinality.

They do not become durable identity and do not choose a semantic winner independently.

The Fillet input fixture uses endpoint geometry only to identify one concrete transient edge **inside the already Resolved Cut-face context**. This is probe plumbing, not a proposed persistent edge selector.

No Face[n], Edge[n], provider handle, Viewer token, topology traversal order or geometry-similarity fallback is used as SS2 semantic identity.

## Cold replay

The first evaluation scope is destroyed before the second pass.

The cold replay reconstructs the complete E02 evidence from declared scenario inputs and reproduces:

- stable Resolved stage outcomes;
- Missing after selected-face removal;
- Geometric failure with still-Resolved Cut input;
- stage ambiguity when producer stage is omitted.

No previous-process/provider object is required.

The broader persisted-document cold-rebuild matrix remains owned by E07.

## Architecture implication

E02 supports the O-05/O-09 direction:

1. a persistent reference must identify the intended producer/consumed stage;
2. provider lineage can support reconstruction but cannot replace semantic stage/role/provenance;
3. downstream geometry failure must not rewrite a still-valid input reference into Missing/Ambiguous;
4. current/final Body search is not a substitute for stage-aware resolution.

This evidence does not freeze the durable selector schema. PM-00B owns that decision.

## Scope boundary

E02 introduces no:

- BodyId / FeatureId persistence;
- persisted Feature history;
- Part Feature Tree;
- product Extrude/Cut/Fillet command or UI;
- persisted B-Rep/provider handles;
- durable topology-reference schema;
- geometry-similarity identity.

The next PM-00A package is **E07 — cold model rebuild parity across the accumulated E01/E02/E03/E04/E05 stable/failure cases**.
