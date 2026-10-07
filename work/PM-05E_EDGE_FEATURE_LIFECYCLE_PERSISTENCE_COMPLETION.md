# PM-05E — Edge Feature Edit / Repair / Lifecycle / Persistence Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-05_EDGE_FEATURES.md`  
**Checkpoint:** PM-05E — edit / repair / lifecycle / persistence  
**Date:** 2026-10-07

## Final runtime evidence

- exact candidate: `88e07b1a2dbdb6ded73d0c2f90dd0a1443242cbe`;
- Windows FULL #1721: **PASS**;
- core-only: **25/25 PASS**;
- kernel-native: **54/54 PASS**;
- desktop: **112/112 PASS**;
- final `windows-msvc` aggregate: **PASS**;
- squash merge #292: `1a1491f7687f32089c1424025123329c74d7b6ca`.

## E1 — FeatureId-preserving Edit

Delivered:

- shared Fillet/Chamfer draft mode distinguishes create vs edit;
- Edit restores the complete authored semantic Edge set, parameter and name;
- existing `FeatureId` is preserved;
- edit preview/evaluation is bound to the exact authored predecessor Body stage;
- exact Finish binding includes DocumentId, revision, draft generation, mode, FeatureId, Edge set, parameter and name;
- stale evaluation is rejected;
- successful edit is one semantic transaction / one Undo step;
- Undo/Redo restores the previous/exact edited authored definition.

Dedicated kernel-native regression:

- `pm05e1.edge_feature_edit`.

## E2 — Workbench repair and lifecycle

Delivered:

- Tree/Properties Edit for Fillet and Chamfer;
- exact target-stage Viewer scene for edit/repair rather than final-Body picking;
- explicit repair for unresolved authored Edge intent;
- Missing/Ambiguous/Unsupported remain distinct structured states;
- failing Edge input is surfaced rather than silently rebound;
- unresolved intent remains authored until the user explicitly removes/replaces it;
- no nearest/similar/geometry fallback;
- Fillet/Chamfer Suppress and Delete use the existing semantic Feature lifecycle;
- Cancel performs zero authored mutation and restores the edited Feature selection;
- command/UI paths continue to share the same application draft/validation meaning.

Dedicated desktop regression:

- `pm05e2.cad_workbench_edge_lifecycle`.

## E3 — Save/Reopen, cold rebuild and bidirectional chaining

Delivered and proven on native schema v14 without a schema change:

- successful Fillet/Chamfer durable Save/Reopen;
- fresh-process/fresh-provider cold reconstruction with no persisted runtime topology handles;
- `Fillet -> Chamfer` chaining through durable generated engineering Edge meaning;
- `Chamfer -> Fillet` chaining through durable generated engineering Edge meaning;
- durable Blocked intent survives Save/Reopen after upstream removal;
- runtime identity/provider history remains derived and disposable.

The reverse `Chamfer -> Fillet` cold-rebuild evidence exposed a real semantic-topology publication defect. Diagnostic candidate `a23f80199144493498ca841fdbaafb4b9c02edf9` / FULL #1719 showed:

- the second Fillet provider operation succeeded;
- semantic Edge resolution was not failing;
- the Feature failed specifically with `topology_integrity_failure`.

Root cause: during proven continuation of the same semantic planar Surface, `propagateCurrentReferences()` overwrote the Part-owned canonical planar frame with the provider's current U/V basis. ADR-0016 and the accepted PM-05C2b evidence explicitly forbid provider UV from becoming semantic frame authority.

Final remediation in `88e07b1a2dbdb6ded73d0c2f90dd0a1443242cbe`:

- preserves the existing semantic canonical frame for a resolved continued planar Surface;
- still refreshes current runtime token/current Face realization;
- validates provider-reported frame presence/validity but does not adopt its U/V basis as semantic authority;
- clears the canonical frame when the Surface becomes unresolved;
- makes no geometry-nearest/provider-order fallback and introduces no new durable identity.

Dedicated kernel-native regression:

- `pm05e3.edge_feature_cold_rebuild`.

## E4 — strict upstream lifecycle matrix

Dedicated whole-set resolver evidence proves for both Fillet and Chamfer:

- preserve -> **Resolved**;
- split singular authored meaning -> **Ambiguous**;
- merge incompatible authored meanings onto one runtime Edge -> **Ambiguous**;
- remove -> **Missing**;
- unsupported semantic meaning -> **Unsupported**;
- no first/nearest/longest/provider-order winner;
- no geometry fallback.

Dedicated regression:

- `pm05e4.edge_lifecycle_matrix`.

## Verification history

Several pre-final FULL runs intentionally failed while the new E evidence was converging:

- #1707 exposed an E2 Cancel/re-entry UI lifecycle defect; Cancel now restores the edited Feature selection;
- #1713/#1715/#1716 exposed incorrect E3 test-fixture assumptions around chain evidence extraction;
- #1718 established that `Fillet -> Chamfer` passed while `Chamfer -> Fillet` reached 12 successful first Chamfers, 48 authorable generated candidates and 48 explicit contour authoring successes, but zero successful second Fillet evaluations;
- #1719 classified that reverse-chain failure as semantic topology-integrity failure after a successful provider operation, leading to the canonical-frame remediation above;
- final exact candidate `88e07b1a2dbdb6ded73d0c2f90dd0a1443242cbe` passed FULL #1721.

No failing pre-final candidate is completion authority; #1721 on the exact final candidate is the runtime authority.

## Architecture boundary

PM-05E does **not** complete:

- final internal documentation synchronization;
- paired PL/EN Product documentation;
- Product Browser regeneration;
- cumulative PM-05 final acceptance matrix;
- final Owner supported-Windows acceptance;
- PM-05 closure;
- PM-06 work or any excluded advanced Fillet/Chamfer variant.

Those remain PM-05F only.

## Result

PM-05E is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-05F — documentation / cumulative automated evidence / Owner Windows acceptance** under the unchanged Owner-accepted PM-05 Work Contract. PM-05 remains ACTIVE until explicit Owner PASS closes the final workflow.
