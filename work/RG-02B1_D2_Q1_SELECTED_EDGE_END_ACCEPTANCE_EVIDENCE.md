# RG-02B1/D2-Q1 — selected material Edge-local END: native acceptance evidence

**STATUS: IN PROGRESS — Q1 NOT YET ACCEPTED / NOT MERGED.**
**Owner D2 scope:** merged Work Contract `work/RG-02B1_D2_Q1_SELECTED_EDGE_END_WORK_CONTRACT_DRAFT.md` ([PR #356](https://github.com/MechanicalCave/SimpleSolid2/pull/356)); dedicated activation [PR #357](https://github.com/MechanicalCave/SimpleSolid2/pull/357) squash `0ede31b14e5f1ea20ff4b0861ff9ebdca3962ffb`. The old prose in the Q1 contract header predates these verified GitHub merges; live main `work/ACTIVE.yaml` governs.
**Implementation:** [Ready-for-Review PR #359](https://github.com/MechanicalCave/SimpleSolid2/pull/359), explicit twelve-path allowlist, no production merge authorized. Technical Owner FINAL PASS and independent PR-specific merge consent are pending.

## Actual gates — do not extrapolate

| Gate | Exact tested candidate | Native/CTest evidence | Result |
| --- | --- | --- | --- |
| Windows FAST #2884 | `d6239703ebd882b474e53b904a86f491b0270483` | [Actions #2884](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38440959996), desktop FAST 93/93, including registered `pm05b2.strict_edge_resolver` | **PASS 93/93** |
| Q1 initial kernel FOCUSED (`pm05c2b.part_edge_topology`, `pm02p.e_kernel_lifecycle`) | `6c5b0c130179fc09fd00a0ad80de96db8720c873` | [Actions #2885](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38441775808): actual MSVC Release build + registered native CTest **2/2** | **PASS 2/2** |
| Q1 Fillet/Chamfer negative FOCUSED | `9fd299f659ccd9e3b31fbbf2adb2610e7f627db4` | [Actions #2886](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38442307105): real OCCT kernel native registered **2/2 PASS**, including Fillet/Chamfer unsupported global Point negative assertions | **PASS 2/2** |
| Q1 intrinsic Point-status negative FOCUSED | `0940f4ef55f2f669d18523da889cdc42a2206c78` | [Actions #2887](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38442654618): real native CTest **2/2 PASS**, plus simulated current-catalog global Point `Ambiguous/Unsupported/Missing` independently rejected | **PASS 2/2** |
| Q1 post-settle FAST #2888 | `77194e81b2f60a41e4425d1a428f223ae3cea233` | [Actions #2888](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38442922542), desktop FAST **93/93 PASS**, including `pm05b2.strict_edge_resolver` | **PASS 93/93** |
| Q1 real Part half-cylinder / partial Revolve FOCUSED #2891 | `c16768f36063d45b594d0029d3df1e6e406ca70e` | [Actions #2891](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38444493164): native `pm05c2b.part_edge_topology` and `pm02p.e_kernel_lifecycle`, actual OCCT **2/2 PASS** | **PASS 2/2** |
| Ready-for-Review Windows FULL #2893 | `113b4f0c18edfd5bd802264cb7303a7d8a6f51a6` | [Actions #2893](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38445041608): **Core 25/25; Kernel Native 57/57; Desktop 117/117 = 199/199 PASS** | **PASS on previous exact HEAD; subsequent tests not covered** |
| Q1 final structural failure FOCUSED | Not yet run | Duplicate Q0 terminal token, missing/ambiguous/unsupported semantic Curve, missing/unsupported Point, invalid stage, foreign provider and invalid current material Edge in existing registered native target | **UNRUN on new test SHA** |
| Browser DOC-01 freshness #2893 | `113b4f0c18edfd5bd802264cb7303a7d8a6f51a6` | Same FULL job: `./ss2.ps1 docs`, exact generated Browser diff must be empty, docs dispatcher test PASS | **PASS on previous exact HEAD; no documentation changes since** |
| Owner technical FINAL PASS / independent production merge | Not requested | Requires all evidence and separate explicit PR consent | **PENDING** |

The 93/93 FAST result is **not** evidence for kernel-native Q1. Unlike FAST, exact-head Windows FOCUSED #2885, #2886 and #2887 each built and executed both registered real OCCT CTests (2/2 PASS each) with the canonical 12/24/8/replay, Fillet/Chamfer negative and synthetic Point status witnesses, respectively. These are code-stage checkpoints, not final QA or Owner acceptance. Real Part half-cylinder Cut and 90-degree partial Revolve are witnessed in exact-head FOCUSED #2891. The half-cylinder has **4 Ambiguous + 2 Unsupported** current native Vertices (2 multiply realized semantic Point addresses), with no false END authoring on any incident Edge; the partial Revolve independently exercises **successful** certified Line and open-Arc END author/replay. These are different negative/positive source classes, not evidence of newly supported ambiguous Points. Exact-head FULL #2893 passed **199/199 plus DOC-01**, but new Q1-05/07 test assertions were appended afterwards: repeat focused tests, FAST and eventually FULL on final tested HEAD before technical Owner approval.

## Finite Q1-01…Q1-08 witness ledger

| Contract gate | Executable witness | Status as of this checkpoint |
| --- | --- | --- |
| Q1-01 12 strict Edges / 24 END | Native `verifyQ1CanonicalSelectedEnds`: real OCCT current-stage Extrude and strict author+replay | **PASS in #2885 (native #1/2)** |
| Q1-02 eight trihedral corners | Same native registered target; three distinct selected Edge sources at each shared Point, B not aliased to A | **PASS in #2885 (headless only)** |
| Q1-03 cold provider | Real independent OCCT provider and rebuilt Part, all 24 semantic addresses replay | **PASS in #2885** |
| Q1-04 height edit/Feature decoy | Real height edited same Feature IDs, plus identical geometry with new FeatureId rejects 24 old sources | **PASS in #2885** |
| Q1-05 wrong/stale/branch | Native cross-DocumentId, live-revision stale, foreign endpoint (#2885); now additionally stage-invalid/Curve missing or extra candidate, material Edge Unsupported, vanished/Unsupported Point, invalid Edge, foreign provider (new structural checks) | **Earlier subset PASS; new structural checks UNRUN** |
| Q1-06 negative families | Real Fillet/Chamfer native Unsupported (#2886), global Point Ambiguous/Unsupported/Missing mutation (#2887), real half-cylinder Cut **4 ambiguous + 2 unsupported** native Vertices with all incident END attempts rejected and two ambiguous Point addresses (#2891); 90° partial Revolve positive Line/open Arc author+replay (#2891) | **PASS for these executed real/synthetic families in #2886/#2887/#2891; no false promotion** |
| Q1-07 closed/degenerate/slot | Native closed-arc guards + two-token inventory; complete tuple permutation and forged token PASS #2885, new **duplicate Q0 endpoint token pair** fail-closed checks | **Earlier families PASS; duplicate pair regression UNRUN** |
| Q1-08 current Q0 membership coupling | Exact stage + same-body native inventory, runtime-only paired Vertex→position/unoriented tangent | **PASS in #2885** |

## Runtime and durability limits

The Q1 value stores `MaterialEdgeReference` plus separately certified `FeaturePointAddress`; `DocumentId` is a read-boundary owner qualifier, not baked into this Part-local source value. `RuntimeVertexToken`, world XYZ, native array index, tangent, provider Body and session lease are **not** durable reference authority. A globally Ambiguous or Unsupported Point remains Ambiguous/Unsupported. No new Part-v17 schema, Datum user-visible constructor, signed Frame3/Reverse, UI picking mode, MID persistence or Q2+ implementation is authorized. Production merge requires new technical Owner FINAL PASS following real Windows native and full-suite evidence.
