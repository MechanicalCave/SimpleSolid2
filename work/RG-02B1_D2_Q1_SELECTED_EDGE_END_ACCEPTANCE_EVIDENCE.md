# RG-02B1/D2-Q1 — selected material Edge-local END: native acceptance evidence

**STATUS: IN PROGRESS — Q1 NOT YET ACCEPTED / NOT MERGED.**
**Owner D2 scope:** merged Work Contract `work/RG-02B1_D2_Q1_SELECTED_EDGE_END_WORK_CONTRACT_DRAFT.md` ([PR #356](https://github.com/MechanicalCave/SimpleSolid2/pull/356)); dedicated activation [PR #357](https://github.com/MechanicalCave/SimpleSolid2/pull/357) squash `0ede31b14e5f1ea20ff4b0861ff9ebdca3962ffb`. The old prose in the Q1 contract header predates these verified GitHub merges; live main `work/ACTIVE.yaml` governs.
**Implementation:** [Draft PR #359](https://github.com/MechanicalCave/SimpleSolid2/pull/359), explicit twelve-path allowlist, no production merge authorized.

## Actual gates — do not extrapolate

| Gate | Exact tested candidate | Native/CTest evidence | Result |
| --- | --- | --- | --- |
| Windows FAST #2884 | `d6239703ebd882b474e53b904a86f491b0270483` | [Actions #2884](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38440959996), desktop FAST 93/93, including registered `pm05b2.strict_edge_resolver` | **PASS 93/93** |
| Q1 kernel FOCUSED (`pm05c2b.part_edge_topology`, `pm02p.e_kernel_lifecycle`) | Not yet recorded | Existing registered native targets; must compile and actually execute | **UNRUN** |
| Q1 post-settle FAST | Not yet recorded | Must test final source/test snapshot | **UNRUN** |
| Ready-for-Review Windows FULL | Not yet recorded | Exact source SHA; Core/Native/Desktop/docs aggregate | **UNRUN** |
| Browser DOC-01 freshness | Not yet recorded | Canonical internal docs plus generated Browser byte comparison | **UNRUN** |
| Owner technical FINAL PASS / independent production merge | Not requested | Requires all evidence and separate explicit PR consent | **PENDING** |

The sole 93/93 FAST result is **not** evidence for the kernel-native Q1 12/24/8 cases, which are excluded from that tier. Current provider-native tests have been written but have **not yet been executed** on this new source. No Q1 native PASS is claimed by this document.

## Finite Q1-01…Q1-08 witness ledger

| Contract gate | Executable witness | Status as of this checkpoint |
| --- | --- | --- |
| Q1-01 12 strict Edges / 24 END | Native `verifyQ1CanonicalSelectedEnds`: real OCCT current-stage Extrude and strict author+replay | **UNRUN** |
| Q1-02 eight trihedral corners | Same native registered target; three distinct selected Edge sources at each shared Point, B not aliased to A | **UNRUN** |
| Q1-03 cold provider | Real independent OCCT provider and rebuilt Part, all 24 semantic addresses replay | **UNRUN** |
| Q1-04 height edit/Feature decoy | Real height edited same Feature IDs, plus identical geometry with new FeatureId rejects 24 old sources | **UNRUN** |
| Q1-05 wrong/stale/branch | Native cross-DocumentId, live-revision stale, foreign endpoint; strict existing branch tests | **UNRUN** |
| Q1-06 negative families | Preserve global Ambiguous/Unsupported Point meaning in real negative families; no implicit promotion | **PARTIAL TEST COVERAGE; UNRUN** |
| Q1-07 closed/degenerate/slot | Existing native closed-arc guards + new two-token inventory, complete terminal tuple permutation and forged token | **UNRUN** |
| Q1-08 current Q0 membership coupling | Exact stage + same-body native inventory, runtime-only paired Vertex→position/unoriented tangent | **UNRUN** |

## Runtime and durability limits

The Q1 value stores `MaterialEdgeReference` plus separately certified `FeaturePointAddress`; `DocumentId` is a read-boundary owner qualifier, not baked into this Part-local source value. `RuntimeVertexToken`, world XYZ, native array index, tangent, provider Body and session lease are **not** durable reference authority. A globally Ambiguous or Unsupported Point remains Ambiguous/Unsupported. No new Part-v17 schema, Datum user-visible constructor, signed Frame3/Reverse, UI picking mode, MID persistence or Q2+ implementation is authorized. Production merge requires new technical Owner FINAL PASS following real Windows native and full-suite evidence.
