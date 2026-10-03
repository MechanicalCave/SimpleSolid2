# PM-00A — E01–E10 Architecture Evidence Matrix

**Status:** FROZEN FOR PM-00A EVIDENCE EXECUTION  
**Version:** 1.0  
**Date:** 2026-10-02  
**Active contract:** `work/PM-00A_PART_MODELING_ARCHITECTURE_EVIDENCE_GATE.md`  
**Entry gate:** Phase A0 COMPLETED — PASS on source candidate `8438cd739e55a390b1df8d4b7ddaa9e6c070e237`, Windows FULL #1206 PASS  
**Decision class:** evidence expectations only; this matrix does not freeze durable Part schema or final PM-00B architecture

## 1. Purpose

This document freezes the expected evidence outcomes for PM-00A E01–E10 before individual probes are implemented.

The matrix exists to prevent a common failure mode in topology-reference research: changing the expected answer after observing provider behavior.

The evidence implementation must therefore test against the expectations below. If OCCT/provider behavior makes an expectation impossible or misleading, STOP and record the contradiction. Do not silently weaken the matrix to make a probe pass.

PM-00B, not this matrix, owns the later architecture freeze.

## 2. Evidence status vocabulary

Reference-oriented evidence uses exactly these states:

- **Resolved** — one semantic target is reconstructed with sufficient producer/stage/role/provenance evidence and no competing semantic interpretation.
- **Missing** — the referenced semantic target no longer exists in the evaluated result and there is no valid replacement under the declared selector meaning.
- **Ambiguous** — more than one candidate or more than one incompatible semantic interpretation satisfies the available selector evidence.
- **Unsupported** — the requested reference meaning is intentionally outside the tested/accepted semantic model; provider topology must not be promoted to identity merely to return a result.

Additional non-reference outcomes are recorded separately:

- **Geometric failure** — authored/reference input can be structurally valid and resolved, but the Kernel operation cannot produce an accepted geometric result.
- **Stale/rejected publication** — a result belongs to an obsolete revision/session/generation and is not allowed to become current.
- **Evidence-only provider failure** — the bounded probe cannot collect the required provider evidence; this is not automatically a Part authored-state failure.

## 3. Global acceptance rules

The following rules apply to every E01–E10 row:

1. **Zero false Resolved.** A different semantic target may never be reported as Resolved merely because it is topologically first, geometrically near, similarly shaped or provider-selected.
2. **Stable cases must actually resolve.** A resolver that returns Missing/Ambiguous for every supported edit does not pass.
3. **Provider history is evidence, not identity.** Generated/Modified/Deleted data may support a decision but cannot become the durable selector by itself.
4. **No topology ordinals.** Face[n], Edge[n], traversal order, tree row and Viewer token are forbidden as semantic identity.
5. **Cold rebuild parity.** Every row marked COLD must reproduce the same reference status/semantic evidence after previous runtime/provider state is destroyed.
6. **Authored/reference/geometry validity remain separate.** A geometric failure must not be rewritten as Missing/Ambiguous unless the reference itself actually failed.
7. **No similarity fallback.** Coordinates, area, length, normal and proximity may support diagnostics/ranking only; they cannot independently turn a non-match into Resolved.
8. **No durable schema.** Evidence selectors/keys in this phase remain test/evidence structures until PM-00B.
9. **Exact semantic provenance wins over provider accident.** Technical seams, refine/unify output ordering and OCCT handle continuity do not add semantic meaning.
10. **Unexpected provider behavior is evidence.** If a probe contradicts the expected model, record it and STOP at the owning decision boundary rather than hiding it with heuristics.

## 4. Evidence selector archetypes

PM-00A probes may use transient selector prototypes built from these evidence concepts:

- **Producer stage** — the evidence operation/stage that created the candidate.
- **Sub-element kind** — face / edge / wire where relevant.
- **Semantic role** — e.g. Extrude start cap, Extrude end cap, side generated from a specific Profile boundary use.
- **Source provenance** — Profile boundary-use provenance established by A0.
- **Lineage evidence** — provider generated/modified/deleted relationship across one reevaluation.
- **Support frame role** — semantic support identity plus deterministic O/U/V/N construction.

These are evidence concepts, not a frozen serialized format.

## 5. E01 — Profile with hole → Extrude

Goal: prove exact Profile input can produce a valid solid while preserving useful semantic production roles without topology ordinals.

| ID | Scenario / mutation | Expected evidence | COLD | Failure that invalidates the row |
| --- | --- | --- | --- | --- |
| E01-01 | Rectangle outer loop + Circle hole → positive Extrude probe | valid one-solid result; start cap role unique; end cap role unique | YES | invalid B-Rep, hole filled, or cap identity based on Face[n] |
| E01-02 | Enumerate side faces from outer rectangle boundary uses | each side is **Resolved** from its concrete source boundary-use provenance | YES | any side is selected only by normal/area/order |
| E01-03 | Enumerate side face from inner circular hole boundary use | cylindrical inner side is **Resolved** from the Circle-hole boundary use | YES | hole-side identity inferred only from cylinder radius/location |
| E01-04 | Change Extrude distance without changing Profile topology | cap roles and all supported side provenance remain **Resolved** | YES | distance edit causes semantic rebinding to another face |
| E01-05 | Change rectangle dimensions while preserving the same four source EntityIds and valid Profile | corresponding outer side roles remain **Resolved** | YES | geometry similarity required for ordinary stable resolution |
| E01-06 | Remove the source Circle so the Profile intent is unresolved | references derived specifically from that hole boundary become **Missing**; the probe must not choose another circular face | YES | false Resolved to a geometrically similar candidate |

E01 completion evidence must include an explicit mapping table:
`Profile boundary-use provenance -> generated side semantic role -> provider evidence`.

## 5A. E01 execution result

**E01 status:** COMPLETED — PASS  
**Exact source candidate:** `476258b74751a251c1c4fdf7dabaa03b3da63976`  
**Windows FULL:** #1228 — PASS  
**False-Resolved:** 0  
**Evidence report:** `work/PM-00A_E01_PROFILE_EXTRUDE_EVIDENCE.md`

All E01-01…E01-06 expectations are satisfied. The provider-specific finding is that side-lineage collection must use the exact transient edge accepted by the provider's wire builder; that edge remains runtime evidence and is not semantic/durable identity.

## 6. E02 — Extrude → Cut → Fillet-style lineage

Goal: determine what remains stable across multiple producer stages and an upstream edit.

This remains evidence-only: the operations are bounded probes, not Part Features.

| ID | Scenario / mutation | Expected evidence | COLD | Failure that invalidates the row |
| --- | --- | --- | --- | --- |
| E02-01 | Base Extrude then Cut that does not touch a selected base side | selected base side remains **Resolved** at the consumed/evaluated stage | YES | resolver silently switches to a different coplanar face |
| E02-02 | Change upstream Extrude distance; Cut still intersects equivalently | unaffected semantic side/cap roles declared stable remain **Resolved** | YES | provider handle continuity is required |
| E02-03 | Cut fully removes a previously referenced semantic face | reference becomes **Missing** | YES | resolver picks nearest surviving face |
| E02-04 | Fillet-style probe modifies but does not destroy an accepted source edge/adjacency meaning | supported selector remains **Resolved** only when lineage + semantic context is unique | YES | geometry-only matching supplies the identity |
| E02-05 | Upstream edit makes downstream geometric operation fail while its input references still resolve | reference status remains **Resolved**; operation result is recorded separately as **Geometric failure** | YES | geometric failure is misreported as Missing/Ambiguous |
| E02-06 | Same semantic source exists at more than one producer stage | selector must include/derive consumed producer stage; cross-stage accidental match is **not Resolved** | YES | current-final-shape search ignores intended stage |

## 6A. E02 execution result

**E02 status:** COMPLETED — PASS  
**Exact source candidate:** `953cdca42978916754f6ae6cc68d86352aad35ac`  
**Kernel-focused:** #1285 — PASS  
**Windows FULL:** #1286 — PASS  
**False-Resolved:** 0  
**Evidence report:** `work/PM-00A_E02_MULTISTAGE_LINEAGE_EVIDENCE.md`

All E02-01…E02-06 expectations are satisfied.

Key findings:

- an unaffected semantic face remains Resolved through Cut at the intended consumed stage;
- independent upstream-height rebuilds preserve accepted stage outcomes without provider-handle continuity;
- complete source-face removal is Missing and never retargets to a new Cut face;
- Fillet input resolution must be bounded by the already-resolved Cut-face semantic context rather than by a global Body geometry search;
- downstream geometric failure remains separate from reference resolution;
- omitting producer stage while the same meaning is live at multiple stages is Ambiguous, never Resolved;
- cold replay reproduces the complete neutral multi-stage evidence after provider objects are destroyed.

The E02 fixture uses a deterministic box prism as its representative initial producer stage. E01 remains the exact Profile-to-Extrude provenance evidence. Provider history and fixture geometry lookup are transient evidence only.

## 7. E03 — referenced edge splits

Goal: prove singular semantic references fail closed when one prior target becomes multiple plausible descendants.

| ID | Scenario / mutation | Expected evidence | COLD | Failure that invalidates the row |
| --- | --- | --- | --- | --- |
| E03-01 | One referenced edge is split into two provider descendants by downstream operation/edit | singular reference becomes **Ambiguous** | YES | first/longest/nearest fragment is automatically Resolved |
| E03-02 | Split produces one lineage descendant with retained semantic role plus one technical seam/auxiliary edge outside that role | reference is **Resolved** only if semantic evidence excludes the technical candidate independently of geometry ranking | YES | provider order or length decides |
| E03-03 | Original edge is deleted with no semantic descendant | **Missing** | YES | unrelated edge selected by proximity |
| E03-04 | Evidence request asks for “all split descendants” without an explicitly declared aggregate selector type | **Unsupported** | YES | singular selector silently changes meaning into a set |

No aggregate/set persistent-reference type is introduced by PM-00A.

## 8. E04 — candidates merge

Goal: prove that merging topology cannot silently erase previously distinct semantic meanings.

| ID | Scenario / mutation | Expected evidence | COLD | Failure that invalidates the row |
| --- | --- | --- | --- | --- |
| E04-01 | Two previously distinct referenced candidates become one physical provider subshape and their semantic distinction is lost | each singular reference requiring the old distinction becomes **Ambiguous** | YES | both are silently reported Resolved to the same subshape |
| E04-02 | Provider reports one old candidate Modified and another Deleted into the same merged result | provider bookkeeping alone is insufficient; result remains **Ambiguous** unless independent semantic role evidence preserves uniqueness | YES | arbitrary provider history winner becomes identity |
| E04-03 | Merge occurs but producer-role/provenance still uniquely identifies one semantic meaning and the other meaning is genuinely removed | preserved meaning **Resolved**; removed meaning **Missing** | YES | both statuses collapse to the same answer |
| E04-04 | Request asks for a merged aggregate semantic object not declared by the selector model | **Unsupported** | YES | new aggregate semantics invented during resolution |

## 8A. E03/E04 execution result

**E03/E04 status:** COMPLETED — PASS  
**Exact source candidate:** `30886e0d47a8f0bbfba5ab9048cf22fb9fa7be25`  
**Kernel-focused:** #1274 — PASS  
**Windows FULL:** #1275 — PASS  
**False-Resolved:** 0  
**Evidence report:** `work/PM-00A_E03_E04_SPLIT_MERGE_CARDINALITY_EVIDENCE.md`

All E03-01…E03-04 and E04-01…E04-04 expectations are satisfied.

Key provider observations:

- middle-notch Cut: one source edge -> two Modified descendants -> singular reference Ambiguous;
- complete-removal Cut: source edge Deleted with zero descendants -> Missing;
- coplanar Fuse: two source faces each Modified to the same physical descendant -> prior semantic distinction is lost and both singular meanings remain Ambiguous without an independent semantic winner;
- asymmetric Fuse: one source face is Modified while another source candidate is Deleted in the same Boolean result; provider history asymmetry alone does not choose identity;
- absorption: independent semantic-role evidence can preserve one surviving meaning as Resolved while the genuinely removed meaning is Missing;
- undeclared aggregate descendant/merge meanings remain Unsupported;
- cold replay reproduces the same neutral provider-history/cardinality evidence after provider objects are destroyed.

Provider Modified/Generated/Deleted/unchanged observations are evidence only. They are not the durable selector format and cannot independently produce semantic identity.

## 9. E05 — geometrically similar topology

Goal: prove that similarity is diagnostic only.

| ID | Scenario / mutation | Expected evidence | COLD | Failure that invalidates the row |
| --- | --- | --- | --- | --- |
| E05-01 | Two or more faces have same area/normal/shape class but different provenance | correct one **Resolved** only through semantic lineage/provenance | YES | geometry score determines identity |
| E05-02 | Original semantic target is removed; a nearly identical replacement appears elsewhere | **Missing** | YES | replacement becomes false Resolved |
| E05-03 | Original target survives while an even geometrically closer decoy appears | original remains **Resolved** | YES | resolver switches to closer decoy |
| E05-04 | Evidence prototype intentionally omits semantic provenance and leaves only similarity features | **Ambiguous** or **Unsupported**, never Resolved | NO | similarity-only result is accepted |

E05 is a mandatory zero-false-Resolved guardrail.

## 9A. E05 execution result

**E05 status:** COMPLETED — PASS  
**Exact source candidate:** `7e0d6109cf927a2e8d85d2794b5718dcc2feab22`  
**Windows FULL:** #1252 — PASS  
**False-Resolved:** 0  
**Evidence report:** `work/PM-00A_E05_SIMILARITY_FALSE_POSITIVE_EVIDENCE.md`

All E05-01…E05-04 expectations are satisfied.

The evidence deliberately separates diagnostic geometry from identity:

- two equal coplanar side faces with the same surface class/area/axis remain distinct because their Profile boundary-use provenance differs;
- deleting a semantic source and recreating the same geometry under a new EntityId leaves the old target Missing, including after cold reopen;
- a surviving semantic target remains Resolved even when another side is geometrically closer to its previous centroid;
- similarity-only evidence returns Ambiguous when multiple diagnostic matches exist and Unsupported when only one diagnostic match exists; it never returns Resolved.

The neutral face diagnostics added for E05 are evidence-only and are not selector semantics or persistence.

## 10. E06 — full Revolve / periodic seam

Goal: expose provider periodic/seam behavior without making the seam semantic identity.

| ID | Scenario / mutation | Expected evidence | COLD | Failure that invalidates the row |
| --- | --- | --- | --- | --- |
| E06-01 | Full 360° Revolve probe from a supported closed Profile | valid expected B-Rep result | YES | provider seam changes authored/Profile meaning |
| E06-02 | Resolve semantic revolved side generated from a Profile boundary use | side role **Resolved** when unique | YES | seam edge/face ordinal participates in durable meaning |
| E06-03 | Attempt to reference the provider-created periodic seam itself as a durable semantic target | **Unsupported** | YES | technical seam is promoted to semantic identity |
| E06-04 | Upstream dimension change moves/changes provider seam representation but preserves revolved semantic side | semantic side remains **Resolved** | YES | seam placement causes semantic identity flip |

## 10A. E06 execution result

**E06 status:** COMPLETED — PASS  
**Exact source candidate:** `44eda36ba6bdc19d8c252831937fb86f8afabde7`  
**Kernel-focused:** #1305 — PASS  
**Windows FULL:** #1306 — PASS  
**False-Resolved:** 0  
**Evidence report:** `work/PM-00A_E06_REVOLVE_PERIODIC_SEAM_EVIDENCE.md`

All E06-01…E06-04 expectations are satisfied.

Key findings:

- a full 360° Revolve produces one valid solid;
- semantic revolved faces are reconstructed from exact Profile boundary-use provenance;
- real provider periodic seam topology is observed but remains `Unsupported` as semantic identity;
- an upstream dimension edit changes seam geometry while the same semantic side remains uniquely `Resolved`;
- cold replay reproduces the same semantic side and non-semantic seam outcome without previous provider state.

E06 also supplies E07-06.

## 11. E07 — cold model rebuild matrix

A0 already proved cold reconstruction for Profile face evidence. E07 extends the requirement to topology-reference outcomes.

Every row below destroys DocumentSession/evidence inputs/B-Rep/provider objects/provider lineage caches before rebuild.

| ID | Replayed evidence row | Required after cold rebuild |
| --- | --- | --- |
| E07-01 | E01 stable cap/side cases | same **Resolved** semantic roles/provenance |
| E07-02 | E02 stable downstream case | same **Resolved** outcome at the same producer stage |
| E07-03 | E03 split case | same **Ambiguous** outcome; not “healed” by fresh provider ordering |
| E07-04 | E04 lost distinction case | same **Ambiguous/Missing** outcome defined by the source row |
| E07-05 | E05 removed-target + decoy case | same **Missing**; no similarity fallback after reopen |
| E07-06 | E06 semantic revolved side | same **Resolved** semantic side while provider seam remains non-semantic |

Any status change caused solely by loss of previous-process provider history is an E07 failure.

## 11A. E07 accumulated execution result

**E07 status:** COMPLETED — PASS  
**Accumulated source candidate:** `44eda36ba6bdc19d8c252831937fb86f8afabde7`  
**Windows FULL:** #1306 — PASS  
**Kernel-native:** 24/24 PASS  
**False-Resolved:** 0  
**Evidence report:** `work/PM-00A_E07_ACCUMULATED_COLD_REBUILD_EVIDENCE.md`

E07-01…E07-06 are satisfied by their owning cold-replay regressions on the accumulated #1306 candidate:

- E01 stable cap/side meanings remain Resolved after authored Save/Close/Reopen;
- E02 stable downstream meaning remains Resolved at the same producer/consumed stage after provider teardown/rebuild;
- E03 split remains Ambiguous after provider-history teardown;
- E04 lost distinction remains Ambiguous/Missing according to the source-row semantic meaning;
- E05 removed target remains Missing after reopen even with a geometry-similar/identical decoy;
- E06 semantic revolved side remains Resolved after provider teardown/rebuild while the periodic seam remains Unsupported.

The frozen E07 matrix is therefore fully COMPLETED — PASS.

## 12. E08 — support-frame stability

Goal: prove support orientation is semantic and deterministic.

The first PM-00A frame is the existing Origin-plane Sketch support frame established by durable Part support + explicit SketchPlacement. Future planar-face support is not activated here.

| ID | Scenario / mutation | Expected evidence | COLD | Failure that invalidates the row |
| --- | --- | --- | --- | --- |
| E08-01 | Reevaluate unchanged Origin-plane support | identical O/U/V/N | YES | frame depends on Viewer/camera/provider state |
| E08-02 | Edit Profile dimensions/geometry without changing support | identical support O/U/V/N | YES | 180° U/V/N flip |
| E08-03 | Reorder provider topology/evidence traversal without changing semantic support | identical O/U/V/N | YES | topology enumeration changes frame |
| E08-04 | Requested future support meaning cannot be represented by the currently accepted support contract | **Unsupported** at PM-00A evidence level | N/A | evidence invents planar-face support semantics early |

E08 gathers O-04 evidence; it does not activate planar-face Sketch support.

## 13. E09 — stale generation/session result

Goal: prove runtime evaluation publication cannot cross revision/session ownership.

| ID | Scenario / mutation | Expected publication outcome | Failure that invalidates the row |
| --- | --- | --- | --- |
| E09-01 | Start evidence at revision R, accept authored mutation to R+1 before old result publishes | old result = **Stale/rejected publication** |
| E09-02 | Start evidence in DocumentSession A, close/replace it with canonical Session B for the same durable document | result owned by A = **Stale/rejected publication** |
| E09-03 | Old provider result is geometrically identical to the new result but belongs to stale generation | still **Stale/rejected publication** |
| E09-04 | Cancelled/failed evidence request completes after a newer request | older completion cannot replace current evidence |

Geometric equality never grants publication authority.

## 14. E10 — tolerance / refine / healing matrix

Goal: separate modeling semantics from display/picking tolerances and collect evidence for O-11.

Exact numerical thresholds are not frozen before measurement.

| ID | Scenario / mutation | Expected evidence | Failure that invalidates the row |
| --- | --- | --- | --- |
| E10-01 | Same valid model evaluated under different zoom/camera/projection | identical semantic/Kernel result class and reference outcomes | any modeling change caused by camera |
| E10-02 | Same valid model evaluated under different pick aperture / OSNAP display settings | identical semantic/Kernel result class and reference outcomes | screen/pick tolerance leaks into modeling |
| E10-03 | Provider refine/unify OFF vs candidate policy ON for a case where semantic result should remain equivalent | record topology/lineage differences; semantic reference outcome must remain explainable under one explicit policy | hidden provider default decides identity |
| E10-04 | Clearly separated geometry well above any candidate modeling tolerance | deterministic accepted result independent of display settings | fuzzy escalation needed for ordinary valid case |
| E10-05 | Clearly invalid gap/overlap beyond candidate modeling tolerance | deterministic **Geometric failure** or structurally invalid Profile as appropriate; no silent healing | increasing fuzzy tolerance until success |
| E10-06 | Near-threshold family swept around candidate tolerances | collect transition evidence; do not label final O-11 threshold “accepted” until recommendation review | undocumented threshold or non-deterministic transition |
| E10-07 | Same authored input under a changed refine/healing/modeling-semantics policy candidate | evidence must identify the policy/version change explicitly | silent result change under same claimed semantics version |

E10 produces an O-11 recommendation; it does not authorize user-configurable modeling tolerance.

## 15. Execution order

The expected execution order is intentionally risk-first:

1. **E01** — establish Extrude producer roles and Profile-boundary provenance.
2. **E05** — immediately attack false-positive similarity behavior before building a larger resolver.
3. **E03 + E04** — split/merge fail-closed behavior.
4. **E02** — multi-stage lineage once the singular-reference failure semantics are explicit.
5. **E07** — cold-rebuild parity across the accumulated stable/failure cases.
6. **E06** — periodic/full-Revolve seam evidence.
7. **E08** — support-frame stability evidence.
8. **E09** — stale publication/generation evidence.
9. **E10** — numerical/refine/healing matrix and O-11 evidence.

This order may be changed only when a concrete probe dependency requires it. Changing order must not change expected outcomes.

## 16. Per-row evidence record

Every implemented row must record:

- exact source candidate SHA;
- exact test/probe name;
- operation inputs and semantic selector prototype;
- relevant provider generated/modified/deleted observations;
- expected state/outcome;
- actual state/outcome;
- cold-rebuild result where COLD=YES;
- whether any similarity/ordinal/provider-handle information was observed and whether it influenced resolution;
- false-Resolved count;
- finding / limitation;
- owning open decision O-xx affected by the evidence.

A row is PASS only when actual outcome equals the frozen expectation and no forbidden identity mechanism influenced the result.

## 17. Stop rules for the matrix

STOP before changing the expected outcome if a probe suggests that passing would require:

- provider topology handles/ordinals as durable identity;
- geometry similarity as automatic identity;
- durable Body/Feature schema;
- a global dependency graph;
- multi-body;
- Assembly infrastructure;
- UI/Viewer semantics;
- new persistent aggregate-reference semantics;
- silent fuzzy/healing escalation.

Such a finding is architecture evidence for Owner/PM-00B review, not permission to weaken PM-00A.

## 18. Completion condition

The matrix completes only when E01–E10 rows required by the active contract have recorded evidence and:

- false-Resolved count is exactly zero;
- every stable case declared supported actually resolves;
- every required COLD row reproduces its semantic outcome after runtime/provider teardown;
- numerical/refine behavior is captured well enough to propose O-11;
- evidence is sufficient to propose O-01/O-04/O-05/O-09/O-11/O-12 resolutions and O-03/O-06 scope;
- PM-00B ADR/Work Contract candidates can be written without inventing missing evidence.

Until then PM-00A remains ACTIVE and PM-00B/PM-01 remain inactive.
