# RG-02B1/D2-Q1 — selected material Edge-local END: native acceptance evidence

**STATUS: IN PROGRESS — Q1 NOT YET ACCEPTED / NOT MERGED.**
**Owner D2 scope:** merged Work Contract `work/RG-02B1_D2_Q1_SELECTED_EDGE_END_WORK_CONTRACT_DRAFT.md` ([PR #356](https://github.com/MechanicalCave/SimpleSolid2/pull/356)); dedicated activation [PR #357](https://github.com/MechanicalCave/SimpleSolid2/pull/357) squash `0ede31b14e5f1ea20ff4b0861ff9ebdca3962ffb`. The old prose in the Q1 contract header predates these verified GitHub merges; live main `work/ACTIVE.yaml` governs.
**Implementation:** [Draft PR #359](https://github.com/MechanicalCave/SimpleSolid2/pull/359), explicit twelve-path allowlist, no production merge authorized.

## Actual gates — do not extrapolate

| Gate | Exact tested candidate | Native/CTest evidence | Result |
| --- | --- | --- | --- |
| Windows FAST #2884 | `d6239703ebd882b474e53b904a86f491b0270483` | [Actions #2884](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38440959996), desktop FAST 93/93, including registered `pm05b2.strict_edge_resolver` | **PASS 93/93** |
| Q1 initial kernel FOCUSED (`pm05c2b.part_edge_topology`, `pm02p.e_kernel_lifecycle`) | `6c5b0c130179fc09fd00a0ad80de96db8720c873` | [Actions #2885](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38441775808): actual MSVC Release build + registered native CTest **2/2** | **PASS 2/2** |
| Q1 Fillet/Chamfer negative FOCUSED | `9fd299f659ccd9e3b31fbbf2adb2610e7f627db4` | [Actions #2886](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38442307105): real OCCT kernel native registered **2/2 PASS**, including Fillet/Chamfer unsupported global Point negative assertions | **PASS 2/2** |
| Q1 intrinsic Point-status negative FOCUSED | `0940f4ef55f2f669d18523da889cdc42a2206c78` | [Actions #2887](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38442654618): real native CTest **2/2 PASS**, plus simulated current-catalog global Point `Ambiguous/Unsupported/Missing` independently rejected | **PASS 2/2** |
| Q1 post-settle FAST #2888 | `77194e81b2f60a41e4425d1a428f223ae3cea233` | [Actions #2888](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38442922542), desktop FAST **93/93 PASS**, including `pm05b2.strict_edge_resolver` | **PASS 93/93** |
| Ready-for-Review Windows FULL | Not yet recorded | Exact source SHA; Core/Native/Desktop/docs aggregate | **UNRUN** |
| Browser DOC-01 freshness | Not yet recorded | Canonical internal docs plus generated Browser byte comparison | **UNRUN** |
| Owner technical FINAL PASS / independent production merge | Not requested | Requires all evidence and separate explicit PR consent | **PENDING** |

The 93/93 FAST result is **not** evidence for kernel-native Q1. Unlike FAST, exact-head Windows FOCUSED #2885, #2886 and #2887 each built and executed both registered real OCCT CTests (2/2 PASS each) with the canonical 12/24/8/replay, Fillet/Chamfer negative and synthetic Point status witnesses, respectively. These are code-stage checkpoints, not final QA or Owner acceptance. A specific real Part half-cylinder/Revolve Q1 negative family is **not yet witnessed**, while READY FULL on the final source and DOC-01 verification remain **UNRUN**.

## Finite Q1-01…Q1-08 witness ledger

| Contract gate | Executable witness | Status as of this checkpoint |
| --- | --- | --- |
| Q1-01 12 strict Edges / 24 END | Native `verifyQ1CanonicalSelectedEnds`: real OCCT current-stage Extrude and strict author+replay | **PASS in #2885 (native #1/2)** |
| Q1-02 eight trihedral corners | Same native registered target; three distinct selected Edge sources at each shared Point, B not aliased to A | **PASS in #2885 (headless only)** |
| Q1-03 cold provider | Real independent OCCT provider and rebuilt Part, all 24 semantic addresses replay | **PASS in #2885** |
| Q1-04 height edit/Feature decoy | Real height edited same Feature IDs, plus identical geometry with new FeatureId rejects 24 old sources | **PASS in #2885** |
| Q1-05 wrong/stale/branch | Native cross-DocumentId, live-revision stale, foreign endpoint; strict existing branch tests | **PASS native subset #2885; broader matrix pending** |
| Q1-06 negative families | Real Fillet/Chamfer (FOCUSED #2886), global Point status negative matrix (#2887), and **newly added unexecuted** actual Part half-cylinder Cut (expected 4 Ambiguous / 2 Unsupported current vertices) + 90° partial Revolve bounded Line/Arc semantics | **Existing families PASS; NEW REAL HALF-CYLINDER/REVOLVE TESTS UNRUN — NOT YET FINAL PASS** |
| Q1-07 closed/degenerate/slot | Existing native closed-arc guards + new two-token inventory, complete terminal tuple permutation and forged token | **PASS in #2885 (tested families only)** |
| Q1-08 current Q0 membership coupling | Exact stage + same-body native inventory, runtime-only paired Vertex→position/unoriented tangent | **PASS in #2885** |

## Runtime and durability limits

The Q1 value stores `MaterialEdgeReference` plus separately certified `FeaturePointAddress`; `DocumentId` is a read-boundary owner qualifier, not baked into this Part-local source value. `RuntimeVertexToken`, world XYZ, native array index, tangent, provider Body and session lease are **not** durable reference authority. A globally Ambiguous or Unsupported Point remains Ambiguous/Unsupported. No new Part-v17 schema, Datum user-visible constructor, signed Frame3/Reverse, UI picking mode, MID persistence or Q2+ implementation is authorized. Production merge requires new technical Owner FINAL PASS following real Windows native and full-suite evidence.
