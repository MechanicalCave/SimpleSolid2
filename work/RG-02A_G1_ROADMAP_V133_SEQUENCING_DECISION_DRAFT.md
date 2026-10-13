# RG-02A/G1 vs PG-01E — Owner sequencing record (v1.33 A proposal superseded)

**Status:** **OWNER REVISED THE NEXT-STAGE PRIORITY: PG-01E NEXT; G1 DEFERRED.** No current roadmap version change, no PG-01E D2 scope acceptance, no G1 activation and no implementation authorization.  
**Date:** 2026-10-13  
**Authoritative accepted roadmap on main:** `work/PART_MODELING_V1_ROADMAP.md` **v1.32**, main source `5b495c4bbed8754d7a1f01e87a30716429db786d`; `work/ACTIVE.yaml` still names the already completed RG-02A/G0 evidence contract with roadmap metadata 1.32.  
**Decision history:** Owner first answered `akceptuje kontynuj` to preparing **Variant A** (G1 before PG-01E) as a draft roadmap v1.33 amendment. This authorized **draft preparation only**, not merge or activation. After asking **why** the order was being changed, Owner was explicitly offered the original PG-01E-first order and replied `akceptuje - kontynuu`. **This later answer supersedes the earlier Variant A sequencing proposal.**

## 1. Current Owner-directed priority

`UI-01 [COMPLETE]` → `RG-02A/G0 [COMPLETE, Owner evidence accepted and PR #332 merged]` → **`PG-01E [NEXT FOR D2 SCOPE REVIEW]`** → `PM-06 [separately gated in accepted roadmap]`.

**RG-02A/G1 is deferred and dormant.** Its exact bounded evidence-only Work Contract scope in [Draft PR #334](https://github.com/MechanicalCave/SimpleSolid2/pull/334), and the evidence-first Option B direction in [Draft PR #333](https://github.com/MechanicalCave/SimpleSolid2/pull/333), remain text-accepted only; they do not override PG-01E, authorize native tests or activate G1. **Its eventual position relative to PM-06 is NOT determined here** and requires a future explicit Owner D2 sequencing decision. This avoids silently inserting G1 between PG-01E and PM-06, another change not yet accepted.

**Rationale for deferring G1:** G0 proved real `Unsupported` and `Ambiguous` Body Vertex semantic identity cases; G1 is a bounded investigation of those unresolved cases. These are **not a demonstrated prerequisite for closing PG-01E**, which concerns the previously started Project Geometry feature and product-wide evidence. Favor completing the accepted program's earlier PG-01E obligations before commissioning additional RG-02 provenance research. No G0 evidence or future G1 contract is invalidated by this ordering choice.

## 2. Superseded documents and branches

- [Draft PR #336](https://github.com/MechanicalCave/SimpleSolid2/pull/336) proposed an amendment of the **accepted roadmap to v1.33** placing G1 before PG-01E. Owner's later PG-01E-first decision supersedes that proposal. **PR #336 MUST NOT MERGE** and should be closed as superseded (closing is not a merger).
- The historical A-vs-B proposal that this file originally carried was a review artifact only. The final answer was the Owner's explicit **PG-01E-first** selection. No amendment of `work/PART_MODELING_V1_ROADMAP.md` is required merely to restore this ordering: roadmap v1.32 already places PG-01E immediately after G0.
- Accepted G0 evidence remains on `main` through [PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332), squash `5b495c4bbed8754d7a1f01e87a30716429db786d`; its two documented evidence gaps remain. No false claim of a universal stable `VERTEX` reference, and no schema/UI work.

## 3. Next PG-01E gate (NOT preapproved by prioritization)

The existing [PG-01E D2 proposal PR #311](https://github.com/MechanicalCave/SimpleSolid2/pull/311) (`work/PROJECTION_01E_PRODUCT_CLOSEOUT_D2_PROPOSAL.md`) predates G0 and is based on an older `main`; it is **open and NOT OWNER-ACCEPTED**. It proposes distinct D2-E1–E6 decisions: accepted PG-01A–D capability/evidence traceability, cross-feature native lifecycle/Save/Reopen evidence, modeling-v2 **compatibility/refusal only (NO migration)**, reproducible performance/resource measurements without PM-06 budget freeze, PL/EN/Internal/Browser parity, and exact-head Windows FULL plus final Owner manual acceptance.

**Only a separate Owner D2 acceptance of the reviewed PG-01E proposal can authorize drafting a bounded PG-01E Work Contract.** That contract must spell out allowed source/test/docs paths and test matrix, then obtain its own Owner acceptance, separate merge and **dedicated `work/ACTIVE.yaml` activation** before any PG-01E implementation or test changes. Prioritization is not PG-01E contract acceptance, merge approval, or acceptance of specific E1–E6 implementation claims. If any stale PG-01E design assumptions conflict with current `main`, surface the exact delta for Owner; do not silently implement it.

## 4. Independent gates and exclusions

| Item | Current status |
| --- | --- |
| Owner acceptance of G0 and separate PR #332 merge | **DONE**, bounded evidence accepted with two gaps |
| Owner evidence-first Option B G1 direction | **ACCEPTED FOR DEFERRED RESEARCH ONLY** |
| Exact G1 Work Contract text PR #334 | **OWNER-ACCEPTED TEXT, Draft/unmerged/inactive; DEFERRED** |
| Variant A roadmap v1.33 PR #336 | **SUPERSEDED — NO MERGE PERMISSION** |
| PG-01E as next D2 review | **OWNER DIRECTED** (`akceptuje - kontynuu`, 2026-10-13) |
| PG-01E D2-E1–E6 scope in PR #311 | **NOT OWNER-ACCEPTED; REVIEW REQUIRED** |
| PG-01E Work Contract / activation | **NOT PREPARED / NOT ACTIVE** |
| G1 relative to PM-06 | **UNDECIDED — future explicit sequencing** |
| D2 PR #333, G1 PR #334, any current PR merge | **NO IMPLIED MERGE AUTHORITY** |
| RG-02 technical Vertex identity/schema | **NOT APPROVED** |
| Any RG-02 UI/UX code | **HARD STOP until prior independent UX consultation and explicit Owner approval** |

Do not alter `work/ACTIVE.yaml`, production code, tests, CMake, schemas, accepted roadmap v1.32 or UI while preparing this decision record. All PR merges are independent explicit Owner actions.

## Documentation Impact

Internal docs: not required
User/Product docs: not required
Reason: this is a documentation-only Owner sequencing record; no product behavior or accepted roadmap has been modified.
