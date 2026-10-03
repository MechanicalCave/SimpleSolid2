# PM-00A — E07 Accumulated Cold-Rebuild Parity Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-02  
**Accumulated source candidate:** `44eda36ba6bdc19d8c252831937fb86f8afabde7`  
**Windows FULL:** #1306 — PASS  
**Kernel-native suite:** 24/24 PASS  
**Matrix:** `work/PM-00A_E01_E10_EVIDENCE_MATRIX.md` v1.0

## Result

E07-01 through E07-06 PASS across the accumulated accepted evidence set.

E07-06 is supplied by the E06 full-Revolve / periodic-seam regression on exact source candidate `44eda36ba6bdc19d8c252831937fb86f8afabde7`.

False-Resolved count across the completed E07 rows: **0**.

No new runtime or test harness was added for this package. The existing E01/E02/E03/E04/E05 regressions already contain the required teardown/cold-replay assertions. Windows FULL #1286 ran those regressions together on one exact kernel-native source candidate and all passed.

## Why no duplicate E07 harness was added

The frozen E07 rows replay previously accepted evidence. The owning regressions already perform the required cold boundary:

- durable Part Save/Close/Reopen where the source row is authored-state based;
- destruction of provider/B-Rep/history objects and reconstruction from declared scenario inputs for evidence-only provider probes.

Adding a second implementation of those scenarios would duplicate fixtures and could drift from the actual accepted source-row semantics.

E07 Phase 1 therefore treats the existing cold assertions as the executable source of evidence and requires them to pass together on one accumulated source candidate.

## Accumulated exact-head evidence

Windows FULL #1306 on `44eda36ba6bdc19d8c252831937fb86f8afabde7` ran the accumulated kernel-native suite with:

```text
pm00a.e01_extrude_evidence          PASS
pm00a.e05_similarity_guardrails    PASS
pm00a.e03_e04_cardinality          PASS
pm00a.e02_multistage_lineage       PASS
pm00a.e06_revolve_periodic         PASS
pm00a.profile_cold_rebuild         PASS
kernel-native total                24/24 PASS
```

The same FULL also passed desktop, core-only, selector validation and the complete unfiltered desktop suite.

## E07 row results

| Row | Owning regression | Cold boundary | Required outcome | Actual | Result |
| --- | --- | --- | --- | --- | --- |
| E07-01 | `pm00a.e01_extrude_evidence` | authored Profile Save/Close/Reopen; previous Kernel/OCCT evidence destroyed | same Resolved cap/side semantic roles/provenance | same semantic role/status/provenance/cardinality after reopen | PASS |
| E07-02 | `pm00a.e02_multistage_lineage` | first-pass Cut/Fillet/B-Rep/history objects destroyed; scenarios reevaluated | same Resolved stable downstream meaning at the same producer stage | complete neutral E02 evidence set equal across cold replay | PASS |
| E07-03 | `pm00a.e03_e04_cardinality` | first-pass Boolean/history/provider objects destroyed; split scenario reconstructed | same Ambiguous singular split result | split remains two semantic descendants / Ambiguous | PASS |
| E07-04 | `pm00a.e03_e04_cardinality` | first-pass Boolean/history/provider objects destroyed; merge scenarios reconstructed | same Ambiguous/Missing outcomes defined by E04 | collapsed meanings remain Ambiguous; genuinely removed meaning remains Missing under the source-row semantic winner | PASS |
| E07-05 | `pm00a.e05_similarity_guardrails` | authored scenario Save/Close/Reopen; provider evidence rebuilt | removed target remains Missing despite similar/identical decoy | old semantic target remains Missing; no similarity fallback | PASS |
| E07-06 | `pm00a.e06_revolve_periodic` | first-pass full-Revolve sweep/B-Rep/provider state destroyed; Profile + evidence-axis reconstructed | same Resolved semantic revolved side while seam remains non-semantic | same outer provenance/status/cardinality after cold replay; provider seam remains Unsupported | PASS |

## E07-01 — E01 stable Profile/Extrude roles

The E01 regression destroys the active document/runtime/provider state, reloads the authored native Part document and reevaluates the Profile before rebuilding Extrude evidence.

The accepted semantic side map is compared by:

- role;
- source boundary-use provenance;
- reference status;
- candidate cardinality.

Mutable geometry diagnostics are not identity.

Result: supported cap/side meanings remain **Resolved** after cold reopen.

## E07-02 — E02 stable downstream stage

E02 evaluates the stable, removed and geometric-failure scenarios once, destroys all first-pass provider/B-Rep/history objects, then reevaluates the declared scenario inputs.

The complete neutral `MultiStageLineageEvidence` set compares equal between passes.

Result: the stable downstream meaning remains **Resolved at the same producer/consumed stage**. No previous provider handle continuity is required.

## E07-03 / E07-04 — split and merge outcomes

E03/E04 constructs the provider Boolean scenarios twice with no first-pass operation/history object reuse.

The complete neutral history/cardinality evidence compares equal between passes.

Result:

- split singular target remains **Ambiguous**;
- deleted target remains **Missing**;
- collapsed merge meanings remain **Ambiguous** without an independent semantic winner;
- a genuinely removed meaning remains **Missing** where the source-row semantic role preserves the other target.

Fresh provider ordering does not “heal” an ambiguous or missing reference.

## E07-05 — removed target plus geometry decoy

E05 persists the authored scenario, closes/reopens it and rebuilds neutral Kernel/OCCT evidence.

A replacement entity can reproduce the removed target's geometry while carrying a different semantic source identity.

Result: the original target remains **Missing** after reopen. Geometry similarity does not recover identity.

## E07-06 — full-Revolve periodic seam

E06 evaluates the full-Revolve scenario, destroys the first provider sweep/B-Rep/TopoDS state and reconstructs the same legal Profile + evidence-only axis input.

The cold replay reproduces:

- the same semantic outer boundary-use provenance;
- `Resolved` status;
- candidate cardinality = 1;
- periodic-surface evidence;
- a provider-created seam that remains `Unsupported` as semantic identity.

The edited-dimension Revolve case is also replayed cold. The provider seam geometry changes with the edited model, but semantic side identity remains stable.

Result: **E07-06 PASS**. The complete E07 matrix is now COMPLETED — PASS.

## Architecture implication

Completed E07 strengthens O-05:

1. accepted semantic status survives provider/runtime teardown;
2. previous-process topology handles/history are not required for accepted reconstruction;
3. ambiguity and missing states are stable failure semantics, not temporary provider-cache accidents;
4. similarity fallback remains prohibited after reopen;
5. producer stage remains part of reconstruction meaning across cold replay.

## Scope boundary

This package is evidence synthesis only. It introduces no source-code behavior, no new test fixture, no persistent selector schema, no Body/Feature state and no product modeling command.

**Next:** E08 support-frame stability evidence.
