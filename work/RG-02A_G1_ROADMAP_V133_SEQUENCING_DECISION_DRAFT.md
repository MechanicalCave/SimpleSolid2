# RG-02A/G1 — Part-v1 roadmap v1.33 sequencing proposal (OWNER DECISION DRAFT)

**Status:** **OWNER-ACCEPTED VARIANT A FOR PREPARING A DISTINCT v1.33 ROADMAP AMENDMENT; NO ROADMAP PR MERGE, G1 ACTIVATION OR IMPLEMENTATION AUTHORITY**  
**Date:** 2026-10-13  
**Main baseline:** `5b495c4bbed8754d7a1f01e87a30716429db786d`; accepted `work/PART_MODELING_V1_ROADMAP.md` **v1.32**.  
**Owner sequence decision:** 2026-10-13 — explicit `akceptuje kontynuj` in response to the question whether to approve **Variant A solely to prepare a v1.33 roadmap PR**: G0 completed → G1 native provenance evidence only → PG-01E → PM-06. This is a D2 sequencing approval for drafting the amendment, **not** authority to merge the roadmap PR, merge PRs #333/#334/#335, activate G1 or mutate tests/UI.  
**Already accepted, distinctly:** G0 bounded native evidence with two limitations, [PR #332](https://github.com/MechanicalCave/SimpleSolid2/pull/332) merged after separate Owner permission; Option B evidence-first direction accepted; exact **G1 evidence-only Work Contract text** in [Draft PR #334](https://github.com/MechanicalCave/SimpleSolid2/pull/334) accepted 2026-10-13 (`akceptujekontynuuj`), **NOT merged and NOT activated**. The G1 accepted text does not itself decide package sequencing.

## 1. Previously accepted v1.32 order and now-approved Variant A for amendment drafting

Roadmap **v1.32** authorizes only:
`UI-01 [COMPLETE] → RG-02A/G0 [COMPLETE, evidence merged] → PG-01E [NOT ACCEPTED] → PM-06 [SEPARATELY GATED]`.

PR #311 proposes separate PG-01E reassessment; it is not Owner-accepted. No authority to auto-defer or close PG-01E comes from G0/G1 investigation approval. The G0 findings are measurable: Fillet/Chamfer each 13 native Vertices (10 Resolved, 3 Unsupported); half-cylinder Cut 6 (4 Ambiguous, 2 Unsupported). The independent-kernel rebuild in V11 does not prove future saved RG-02 source persistence or stale interactive Viewer rejection.

| Choice | Next program order | Interpretation |
| --- | --- | --- |
| **A: OWNER-ACCEPTED 2026-10-13 FOR v1.33 AMENDMENT DRAFTING ONLY** | `RG-02A/G0 [COMPLETE] → RG-02A/G1 [native provenance evidence ONLY] → PG-01E → PM-06` | Keep the proven VERTEX semantics investigation coherent, without adding RG-02 production features; requires explicit roadmap v1.33 sequencing approval and a separate roadmap-only PR |
| **B: preserve v1.32 default** | `RG-02A/G0 [COMPLETE] → PG-01E [independent contract/activation decision] → RG-02A/G1 [only under later sequencing decision] → PM-06 [its own gate]` | Honors PG-01E-first after G0; exact G1 contract remains accepted as text only, dormant until explicitly sequenced and activated |

**Owner decision:** A, solely as **one more finite evidence-only exception** before returning to PG-01E; no merge/activation approval. This does not preapprove new VERTEX selectors or supersede PG-01E obligations.

## 2. Approved Variant A — roadmap-only v1.33 amendment to prepare for separate merge review

A separate roadmap-only PR may now be **prepared and checked** (not merged without explicit Owner permission) to:
1. Update `work/PART_MODELING_V1_ROADMAP.md` **1.32 → 1.33** with a dated explicit Owner D2 sequencing acceptance. Preserve all previously frozen Part-v1 scopes, ADRs, schema versions and historical decisions.
2. Mark RG-02A/G0 **COMPLETED, Owner-accepted with two limitations**, cite G0 merge `5b495c4bbed8754d7a1f01e87a30716429db786d` and exact-head Windows FAST **93/93 PASS**, FULL **199/199 PASS**. Preserve both limits: Fillet/Chamfer unsupported raw-vs-semantic provenance unknown; future V11 stale UI source / RG-02 persistence not yet proved.
3. Add **RG-02A/G1 — bounded real-OCCT native provenance evidence only** after G0 and before PG-01E, referring to the exact accepted scope in `work/RG-02A_G1_VERTEX_SEMANTIC_PROVENANCE_WORK_CONTRACT_DRAFT.md` in PR #334, G1-F01/C01/A01/A02/S01/P01/L01 cases and at most **three named existing native test sources** plus a `work/**` evidence report. No `src/**`, CMake, APIs, persistence, MID/CENTER, Datum/Axis, UI, Mirror/Pattern.
4. Keep **PG-01E NOT ACCEPTED** and **PM-06 SEPARATELY GATED**. No retroactive completion, no feature-scope creep, no auto-activation.
5. Do **not** modify `work/ACTIVE.yaml` in that roadmap PR; activation of G1 after all separately authorized merges requires its **own** PR, exact-head CI, Owner permission, and matching `program_roadmap_version: "1.33"`.

This decision proposal itself makes no change to the accepted `work/PART_MODELING_V1_ROADMAP.md`; any roadmap-only PR remains unmerged until its own explicit Owner merge permission.

## 3. Independent decisions and STOP controls

| Gate | Current status |
| --- | --- |
| G0 evidence findings and PR #332 merge | **DONE**, two explicit boundaries retained |
| Option B investigation direction | **OWNER-ACCEPTED**, not technical schema |
| Exact bounded G1 Work Contract in PR #334 | **OWNER-ACCEPTED TEXT ONLY**; Draft/unmerged/inactive |
| **G1 vs PG-01E order** | **OWNER-ACCEPTED A for preparation of v1.33 roadmap amendment only** (`akceptuje kontynuj`, 2026-10-13); merge and activation still pending |
| D2 discussion PR #333 merge | **PENDING separate Owner consent** |
| G1 contract PR #334 merge | **PENDING separate Owner consent** |
| Roadmap-only v1.33 PR, if A | **AUTHORIZED TO PREPARE ONLY**; requires exact-head CI and separate explicit Owner merge approval |
| G1 `work/ACTIVE.yaml` activation | **NOT CREATED / NOT ACTIVE** |
| G1 native test/evidence implementation | **NOT AUTHORIZED** until all applicable merges and activation |
| Technical Vertex schema or production Kernel/Part logic | **NOT AUTHORIZED**, future evidence-backed D2 |
| RG-02 UI/UX | **HARD STOP:** reviewable proposal, prior Owner consultation and separate explicit approval **before ANY VERTEX/MID/CENTER marker, picking, Tree/Operations, preview or Command Line UI code** |

**Neither a D2 roadmap choice nor merging documents would itself authorize G1 testing.** G1 activation must be separately merged into `work/ACTIVE.yaml` after the accepted roadmap and accepted Work Contract are on `main`. No automatic merge or mutation.

## Documentation Impact

Internal docs: not required
User/Product docs: not required
Reason: this is a nonexecuting `work/**` decision proposal only. Any eventual separately authorized RG-02 implementation needs corresponding Internal and PL/EN Product documentation and generated Browser verification.
