# PM-00B — Part Feature Architecture Freeze

**Status:** ACTIVE  
**Owner acceptance:** 2026-10-03  
**Decision class:** D2 architecture-freeze Work Contract  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.4  
**Entry gate:** PM-00A COMPLETED — PASS, final source candidate `ac34a713c6fee4a53d513bfcce2a2044ce1a0ffb`, Windows FULL #1320  
**Primary review input:** `work/PM-00A_PART_FEATURE_ARCHITECTURE_ADR_CANDIDATE.md`

## 1. Goal

Convert completed PM-00A evidence into explicit Owner-approved Part Feature architecture before any durable Body/Feature schema or user-facing solid operation is implemented.

PM-00B is governance/architecture freeze, not a second research phase and not PM-01 implementation.

## 2. Activation rule

The Owner explicitly accepted PM-00B and ADR-0014 without amendment on 2026-10-03. The activation condition is satisfied.

PM-00B authorizes only the governance/architecture work in Section 3. Production Body/Feature schema, migration and user-facing solid-modeling mutation remain inactive until a separately accepted PM-01 contract.

## 3. Scope IN after Owner acceptance

PM-00B may:

- review/amend the PM-00A ADR candidate using the completed evidence;
- close O-01, O-04, O-05, O-09, O-11 and required O-12 foundations by explicit Owner decision;
- accept/amend the O-03/O-06 initial product-scope matrix;
- materialize the accepted Part Feature Architecture ADR in `adr/`;
- synchronize the accepted decision into the Part Modeling roadmap and `work/ACTIVE.yaml`;
- draft the bounded PM-01 First Solid Vertical Slice Work Contract;
- update architecture/internal documentation required to describe the newly accepted contract;
- regenerate the Product Browser through the normal generator if canonical docs change;
- run governance/documentation verification.

## 4. Scope OUT

PM-00B does not authorize:

- Body/Feature production C++ implementation;
- persistent schema migration;
- product Extrude/Cut/Revolve/Fillet/Chamfer commands;
- Part Feature Tree behavior;
- planar-face Sketch support implementation;
- projection implementation;
- public Viewer API change;
- Assembly occurrence/solver work;
- multi-body;
- topology/provider identity persistence;
- numerical healing/fuzzy behavior beyond the Owner-accepted architecture wording.

Those remain PM-01+ work under separately accepted contracts.

## 5. Required Owner decisions

The Owner resolved all required PM-00B decisions without amendment on 2026-10-03:

1. O-01 single-Body/Empty/no-effect/multi-solid semantics;
2. O-04 frame/re-support rules and the explicit deferral boundary for planar-face frame derivation;
3. O-05 semantic selector/stage/cardinality/similarity guarantees;
4. O-09 Failed/Blocked/Delete/Suppress/retry semantics;
5. O-11 modeling-semantics version plus fuzzy/healing/refine policy;
6. O-12 typed IDs/runtime freshness/read-snapshot/transform-direction foundations;
7. O-03/O-06 bounded first operation/projection scope.

The accepted decisions are materialized in ADR-0014. This PM-00B acceptance is not acceptance of PM-01.

## 6. Deliverables

PM-00B completes only when:

- an Owner-accepted Part Feature Architecture ADR exists in `adr/`;
- `work/ACTIVE.yaml -> accepted_adrs` includes it;
- the accepted roadmap/decision ledger reflects the frozen decisions;
- a bounded PM-01 Work Contract candidate exists and remains NOT ACTIVE;
- documentation impact is satisfied;
- required governance/docs gate passes;
- `work/ACTIVE.yaml` ends PM-00B with production still inactive pending separate Owner acceptance of PM-01.

## 7. Verification

PM-00B should be documentation/governance-only unless Owner amendments expose a concrete missing evidence dependency.

Expected verification is DOCS/CLOSURE according to repository classifier rules.

If an Owner-requested architecture change requires new executable evidence, STOP PM-00B completion, amend the evidence plan explicitly and obtain the required exact-head verification before accepting the ADR.

No manual UI verification is required because PM-00B has no user-visible product implementation.

## 8. STOP conditions

STOP and return to Owner if the proposed freeze would require:

- a Foundation amendment;
- new subsystem ownership direction;
- provider/Viewer topology as durable identity;
- geometry similarity as automatic identity;
- durable Body/Feature implementation before the architecture is accepted;
- Assembly/global dependency infrastructure;
- multi-body as a prerequisite;
- a public Viewer API change;
- a hidden persistence migration;
- a user-facing solid tool.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: PM-00B accepts durable Part Feature architecture that maintainers must understand, while it introduces no user-visible product behavior.

## 10. Completion boundary

PM-00B acceptance does not activate PM-01.

After PM-00B completion, the Owner must separately accept the bounded PM-01 Work Contract before any durable Body/Feature schema or Extrude Add production mutation begins.
