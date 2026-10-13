# RG-02B1/D2-Q2 — strict material Edge-local MID: acceptance evidence

**STATUS: IN IMPLEMENTATION — TWO REAL NATIVE FOCUSED WINDOWS RUNS PASS; FAST/FULL/DOC-01 AND OWNER FINAL PASS STILL PENDING; NOT MERGED.**
**Accepted authority:** complete Q2 Work Contract `work/RG-02B1_D2_Q2_STRICT_MATERIAL_EDGE_MID_WORK_CONTRACT_DRAFT.md`, Owner accepted D2 and independently squash-merged [PR #361](https://github.com/MechanicalCave/SimpleSolid2/pull/361) as `e1824e7af12979c33ce4e4307fc9cd6397ea3609`. Owner separately accepted Q1→Q2 ACTIVE-only scope and independently squash-merged [PR #362](https://github.com/MechanicalCave/SimpleSolid2/pull/362) as `b1eba6f59453a8e7fdc95a6cb091462a64e577bb`; `main` `work/ACTIVE.yaml` now points at Q2. Foundation v1.0 and roadmap v1.33 untouched.
**Production candidate:** [Draft PR #363](https://github.com/MechanicalCave/SimpleSolid2/pull/363) on `feat/rg02b1-q2-strict-material-edge-mid-20261013`; EXACT ten-path allowlist, no change to native Kernel Q0 implementation, Part schema, Datum, Frame3/Reverse or UI. Actual native FOCUSED #2907 and follow-on full family+docs #2908 have now both PASS 2/2. FAST/FULL/DOC-01 still unrun.

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
| Q2-M08 Q0/Q1 regression | Registered `pm05b2.strict_edge_resolver`, `pm05c2b.part_edge_topology`, `pm02p.e_kernel_lifecycle`, then native FOCUSED, FAST, FULL, DOC-01, and no product-UI claims | **Native FOCUSED 2/2 PASS #2908; FAST/FULL/DOC-01 UNRUN** |

## Actual CI, code SHA and DOC-01

| Gate | SHA and URL | Result |
| --- | --- | --- |
| Q2 initial native FOCUSED #2907 | `3ae7be6518afc1815b4f9df08bb6d549f594b7b6`, [Actions #2907](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38455192467); Windows MSVC kernel build and **both registered actual native CTests 2/2 PASS** | **PASS 2/2 — BEFORE further real Fillet/Chamfer test and docs update** |
| Q2 post-family native FOCUSED #2908 | `ac5b13ad2d28cf81321f372e73f18e7b721211ce`, [Actions #2908](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38455736197): **2/2 MSVC-built native registered CTests PASS**, including added real Fillet/Chamfer MID checks and canonical/internal docs | **PASS 2/2** |
| Windows FAST `pm05b2.strict_edge_resolver` and desktop regression | Not yet issued | **UNRUN** |
| Windows Ready FULL Core + Native + Desktop | Not yet issued | **UNRUN** |
| DOC-01 canonical Markdown → Browser parity | Not yet issued | **UNRUN** |
| Owner technical FINAL PASS and **separate** production PR-specific squash permission | Not requested | **PENDING** |

Initial Windows FOCUSED #2907 **really built and executed 2/2 native CTests PASS** on its exact code/test SHA. The later Windows FOCUSED #2908 independently built and executed both native CTests **2/2 PASS** on the code/test/docs SHA containing extra Fillet/Chamfer Q2-M07 assertions and current generated Browser; the generated Browser **DOC-01 parity itself has not yet been checked**. FAST and Ready FULL remain **UNRUN**, and a docs-only classification/closure run does not qualify. Any failed test (RED) must be preserved here and fixed without relaxing the accepted algorithm. After FULL on source/tests/docs and later evidence-only updates, prove an exact two-SHA allowlist diff and rerun final-head governance validation.

## Architecture boundaries

`SelectedMaterialEdgeMid{MaterialEdgeReference}` is a distinct **Part-local semantic value**, never a Q1 `SelectedMaterialEdgeEnd`, transient Vertex token, arbitrary parameter or global Point. Strict author+replay is read-only, current owning DocumentId/live revision/exact successful Feature Stage, current native same-Body material Edge and Q0 half-bounded-arclength MID. A globally ambiguous Point is never promoted. No persistence, signed Datum frame/Reverse, Viewer picker, user-facing Datum constructor, schema v17 or new Q2+ scope. All failed observations leave Edge/XYZ/tangent empty and report typed status.
