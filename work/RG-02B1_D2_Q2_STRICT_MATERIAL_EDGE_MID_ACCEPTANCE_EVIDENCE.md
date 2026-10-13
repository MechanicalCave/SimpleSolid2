# RG-02B1/D2-Q2 — strict material Edge-local MID: acceptance evidence

**STATUS: SOURCE/TEST/DOCS FULL PASS — TECHNICAL OWNER FINAL PASS AND SEPARATE PRODUCTION SQUASH-MERGE PERMISSION PENDING / PR #363 UNMERGED.**
**Accepted authority:** complete Q2 Work Contract `work/RG-02B1_D2_Q2_STRICT_MATERIAL_EDGE_MID_WORK_CONTRACT_DRAFT.md`, Owner accepted D2 and independently squash-merged [PR #361](https://github.com/MechanicalCave/SimpleSolid2/pull/361) as `e1824e7af12979c33ce4e4307fc9cd6397ea3609`. Owner separately accepted Q1→Q2 ACTIVE-only scope and independently squash-merged [PR #362](https://github.com/MechanicalCave/SimpleSolid2/pull/362) as `b1eba6f59453a8e7fdc95a6cb091462a64e577bb`; `main` `work/ACTIVE.yaml` now points at Q2. Foundation v1.0 and roadmap v1.33 untouched.
**Production candidate:** [Ready-for-Review PR #363](https://github.com/MechanicalCave/SimpleSolid2/pull/363) on `feat/rg02b1-q2-strict-material-edge-mid-20261013`; EXACT ten-path allowlist; **no Kernel Q0 source modification**, native schema, Datum, Frame3/Reverse or UI changes. Native FOCUSED #2907/#2908 2/2 PASS, Windows FAST #2909 93/93 PASS, and exact-source Ready FULL #2910 199/199 PASS plus DOC-01. A subsequent ledger-only update is not new code test evidence and must receive its own governance validation.

## Required real proof ledger — Q2-M01…M08

| Gate | Executable registered witness and oracle | State |
| --- | --- | --- |
| Q2-M01 real 12/12 Extrude MID | `pm05c2b.part_edge_topology`: author+resolve twelve distinct strict material Line MID values, current exact Body half-length positions and unit unoriented tangents; Q1 24 END unaffected | **PASS native #2907 (initial code)** |
| Q2-M02 exact native 3D open Arc MID | `pm02p.e_kernel_lifecycle`: signed sweep positive/negative/wrapped open circular Arc, exact 3D half bounded arclength not chord; closed circle negative; real Part partial Revolve Line/open Arc | **PASS native #2907 (initial code)** |
| Q2-M03 cold provider | Real new OCCT provider and reconstructed Part; all MID semantic references survive, own new Body token, without XYZ or enumeration matching | **PASS native #2907 (initial code)** |
| Q2-M04 upstream edit / FeatureId decoy | Real Extrude height change with MID source continuity, actual MID position movement, identical shape/new producer FeatureId stays `edge_missing` | **PASS native #2907 (initial code)** |
| Q2-M05 typed source negatives | Foreign `DocumentId`, absent ownership evidence, stale revision, bad Stage, missing/split/unsupported Curve and missing current Edge, no partial observation | **PASS native #2907 (initial code)** |
| Q2-M06 genuinely foreign Body / Q0 failures | A test-only binder returns Edge tied to a **different actual OCCT Body**, unlike an innocuous different provider instance; forced native failure, nonfinite MID, zero tangent/length rejected atomically | **PASS native #2907 (initial code)** |
| Q2-M07 Point semantics untouched | Real half-cylinder Cut ambiguity remains global Ambiguous/Unsupported, no synthetic MID-as-Point; strict Edge's MID is independent of that Point's status; real Revolve positive; added real Fillet/Chamfer strict MID examination | **PASS #2908: real half-cylinder, partial Revolve, and Fillet/Chamfer, no global Point promotion** |
| Q2-M08 Q0/Q1 regression | Registered `pm05b2.strict_edge_resolver`, `pm05c2b.part_edge_topology`, `pm02p.e_kernel_lifecycle`, then native FOCUSED, FAST, FULL, DOC-01, and no product-UI claims | **PASS native #2908; FAST #2909 93/93; FULL #2910 199/199 and DOC-01** |

## Actual CI, code SHA and DOC-01

| Gate | SHA and URL | Result |
| --- | --- | --- |
| Q2 initial native FOCUSED #2907 | `3ae7be6518afc1815b4f9df08bb6d549f594b7b6`, [Actions #2907](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38455192467); Windows MSVC kernel build and **both registered actual native CTests 2/2 PASS** | **PASS 2/2 — BEFORE further real Fillet/Chamfer test and docs update** |
| Q2 post-family native FOCUSED #2908 | `ac5b13ad2d28cf81321f372e73f18e7b721211ce`, [Actions #2908](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38455736197): **2/2 MSVC-built native registered CTests PASS**, including added real Fillet/Chamfer MID checks and canonical/internal docs | **PASS 2/2** |
| Windows FAST #2909 | `204b76e7c78cf2c763f01f131991948a7d16ebae`, [Actions #2909](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38455957190): **93/93 CTest PASS**, including `pm05b2.strict_edge_resolver` and Windows aggregate SUCCESS | **PASS 93/93** |
| Windows Ready FULL #2910 | **exact source/test/docs and then-current ledger HEAD** `204b76e7c78cf2c763f01f131991948a7d16ebae`, [Actions #2910](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38456690359): **Core 25/25 + Kernel Native 57/57 + Desktop 117/117 = 199/199**, aggregate success | **PASS 199/199** |
| DOC-01 canonical Markdown → Browser parity | **same FULL #2910** on `204b76e7c78cf2c763f01f131991948a7d16ebae`: `./ss2.ps1 docs` and generated `docs/browser/index.html` unchanged, documentation dispatcher and Browser checks green | **PASS on exact source/docs** |
| Owner technical FINAL PASS and **separate** production PR-specific squash permission | Not requested | **PENDING** |

Windows #2907 and #2908 **really built and executed both registered OCCT-native CTests (2/2 PASS)**. #2908 covers the later Fillet/Chamfer strict-MID and global Point-status assertions; #2909 **FAST 93/93 PASS**. Ready FULL #2910 **Core 25/25 + Kernel Native 57/57 + Desktop 117/117 = 199/199 PASS** on the EXACT unchanged source/test/docs HEAD `204b76e7c78cf2c763f01f131991948a7d16ebae`, with generated Browser DOC-01 parity PASS and aggregate Windows success. This later **acceptance-evidence-only** update must be checked by an exact two-SHA compare plus final-head governance CI, **not** mislabeled as another 199/199 code rerun. No Q2 technical Owner FINAL PASS or PR #363 merge permission has yet been given.

## Architecture boundaries

`SelectedMaterialEdgeMid{MaterialEdgeReference}` is a distinct **Part-local semantic value**, never a Q1 `SelectedMaterialEdgeEnd`, transient Vertex token, arbitrary parameter or global Point. Strict author+replay is read-only, current owning DocumentId/live revision/exact successful Feature Stage, current native same-Body material Edge and Q0 half-bounded-arclength MID. A globally ambiguous Point is never promoted. No persistence, signed Datum frame/Reverse, Viewer picker, user-facing Datum constructor, schema v17 or new Q2+ scope. All failed observations leave Edge/XYZ/tangent empty and report typed status.

## Final source SHA freeze; acceptance and merge still gated

The exact-source/test/internal canonical Markdown/generated Browser candidate is `204b76e7c78cf2c763f01f131991948a7d16ebae`. It passed the two registered Q2 native tests in the earlier source-compatible FOCUSED #2908, FAST #2909 **93/93**, and Ready FULL #2910 **199/199 + DOC-01**. This update changes **only this evidence ledger** to record the verified FULL and is **not a source code mutation**. Before asking Owner for closure, verify GitHub's compare from `204b76e7c78cf2c763f01f131991948a7d16ebae` to the final PR HEAD lists precisely `work/RG-02B1_D2_Q2_STRICT_MATERIAL_EDGE_MID_ACCEPTANCE_EVIDENCE.md`, that the final-head governance run succeeded, and that the PR remains within the exact ten files.

**STOP:** Q2-M01…M08 executed evidence does **not** authorize production squash-merge automatically. Require distinct explicit Owner **technical FINAL PASS** and **PR #363-specific squash-merge permission** on its exact final validated HEAD. Q2 is strictly read-only Part MID semantics, not a persisted Datum/Frame3/Reverse/UI constructor or new schema. No Q2+ activation or any other program slice is implied by its acceptance.
