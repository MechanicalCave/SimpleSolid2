# PM-00A — E07 Accumulated Cold-Rebuild Evidence

**Status:** PARTIAL — E07-01…E07-05 COMPLETED — PASS; E07-06 PENDING E06  
**Date:** 2026-10-02  
**Accumulated source candidate:** `953cdca42978916754f6ae6cc68d86352aad35ac`  
**Windows FULL:** #1286 — PASS  
**Matrix:** `work/PM-00A_E01_E10_EVIDENCE_MATRIX.md`

## Result

E07-01 through E07-05 PASS on one combined exact source candidate.

E07-06 remains intentionally pending because its prerequisite E06 full-Revolve / periodic-seam evidence does not yet exist.

This package introduces no new executable or provider behavior. The required cold-rebuild assertions already exist inside the accepted E01/E02/E03/E04/E05 regressions; FULL #1286 executed them together on the same exact revision.

## Combined execution evidence

Windows FULL #1286 on `953cdca42978916754f6ae6cc68d86352aad35ac` ran the kernel-native Release suite and explicitly passed:

- `pm00a.e01_extrude_evidence`;
- `pm00a.e05_similarity_guardrails`;
- `pm00a.e03_e04_cardinality`;
- `pm00a.e02_multistage_lineage`.

Those regressions contain the teardown/rebuild behavior required by E07 rather than relying on a separate meta-test.

## E07 row summary

| Row | Replayed evidence | Cold-rebuild mechanism | Required outcome | Actual | Result |
| --- | --- | --- | --- | --- | --- |
| E07-01 | E01 stable Extrude cap/side roles | real Part Save/Close/Reopen; prior DocumentSession, neutral input and OCCT B-Rep/provider state destroyed | same Resolved semantic cap/side roles and boundary-use provenance | preserved | PASS |
| E07-02 | E02 stable stage-scoped downstream lineage | first-pass B-Rep/Boolean/Fillet/provider-history objects destroyed; scenario reevaluated from declared semantic inputs | same Resolved status at the same producer/consumed stage | preserved | PASS |
| E07-03 | E03 split cardinality | first-pass OCCT Boolean/history objects destroyed; split probe recreated from scenario inputs | same Ambiguous singular-reference outcome | preserved; no provider-order healing | PASS |
| E07-04 | E04 merge/lost-distinction cases | first-pass OCCT Fuse/history objects destroyed; merge probes recreated from scenario inputs | same Ambiguous/Missing/Resolved outcomes defined by semantic winner rules | preserved | PASS |
| E07-05 | E05 removed target + geometry-similar decoy | real Part Save/Close/Reopen; prior runtime/provider state destroyed | old semantic target remains Missing; similarity cannot retarget | preserved | PASS |
| E07-06 | E06 semantic revolved side | prerequisite E06 not yet executed | same Resolved revolved side while provider seam stays non-semantic | not executable yet | PENDING |

## Why no new E07 meta-test was added

A second orchestration executable would only duplicate accepted cold assertions and could create a weaker test than the evidence it wraps.

The stronger evidence is:

1. each owning regression performs its own teardown/reconstruction at the level appropriate to that scenario;
2. the final E02 source candidate contains all accepted E01/E02/E03/E04/E05 regressions together;
3. Windows FULL #1286 executes the complete kernel-native test graph on that exact revision;
4. all required accumulated cold rows pass in the combined state.

E07 therefore records a cross-package evidence conclusion rather than inventing new runtime semantics.

## Cold boundary by evidence family

### E07-01 / E07-05 — authored Part lifecycle

These rows use native `.ss2part` persistence and destroy the active `DocumentSession` before reopening.

They prove that semantic Profile/Entity identity required for the evidence comes from durable authored state rather than:

- prior-process OCCT handles;
- Viewer tokens;
- transient Kernel inputs;
- cached B-Rep;
- provider history objects.

### E07-02 / E07-03 / E07-04 — evidence-only operation lifecycle

These rows are intentionally not persisted Part Features. Their legal source of truth in PM-00A is the declared evidence scenario input.

The first-pass OCCT operation objects, B-Rep and provider-history objects leave scope. The probe is rebuilt from the same declared semantic scenario, and neutral evidence/status must reproduce exactly.

This is sufficient for PM-00A architecture evidence and does not authorize persistence of the probe operations.

## Architecture implication

E07-01…05 strengthen the O-05 direction:

- accepted reference meaning must be reconstructible without previous-process topology handles;
- provider history is transient evidence, not durable identity;
- Ambiguous and Missing outcomes are first-class stable results and must survive cold reconstruction;
- geometry similarity does not become a repair mechanism after reopen;
- producer/consumed stage remains part of semantic context after cold reconstruction.

E07-01…05 do not freeze the final serialized reference selector. PM-00B owns that decision.

## Remaining E07 work

E07 is **not fully complete**.

E07-06 can only be executed after E06 establishes:

- the supported full-Revolve evidence path;
- semantic revolved-side provenance;
- explicit non-semantic treatment of provider periodic/seam topology.

After E06 passes, E07-06 must cold-replay that accepted case before the overall E07 package can be marked COMPLETED.

## Scope boundary

This evidence closure introduces no:

- BodyId / FeatureId persistence;
- product solid feature;
- durable topology selector schema;
- persisted OCCT handles/history;
- new Kernel/provider operation;
- UI or Part Feature Tree behavior.

**Next PM-00A package:** E06 — full Revolve / periodic seam evidence.
