# RG-02B1/D2-Q2 — strict material Edge-local MID: acceptance evidence

**STATUS: IN IMPLEMENTATION — TEST GATES NOT YET EXECUTED / NO OWNER FINAL PASS / NO MERGE.**
**Accepted authority:** complete Q2 Work Contract `work/RG-02B1_D2_Q2_STRICT_MATERIAL_EDGE_MID_WORK_CONTRACT_DRAFT.md`, Owner accepted D2 and independently squash-merged [PR #361](https://github.com/MechanicalCave/SimpleSolid2/pull/361) as `e1824e7af12979c33ce4e4307fc9cd6397ea3609`. Owner separately accepted Q1→Q2 ACTIVE-only scope and independently squash-merged [PR #362](https://github.com/MechanicalCave/SimpleSolid2/pull/362) as `b1eba6f59453a8e7fdc95a6cb091462a64e577bb`; `main` `work/ACTIVE.yaml` now points at Q2. Foundation v1.0 and roadmap v1.33 untouched.
**Production candidate:** fresh `feat/rg02b1-q2-strict-material-edge-mid-20261013` branch / Draft PR (number pending at the time of authoring); EXACT ten-path allowlist, no change to native Kernel Q0 implementation, Part schema, Datum, Frame3/Reverse or UI.

## Required real proof ledger — Q2-M01…M08

| Gate | Executable registered witness and oracle | State |
| --- | --- | --- |
| Q2-M01 real 12/12 Extrude MID | `pm05c2b.part_edge_topology`: author+resolve twelve distinct strict material Line MID values, current exact Body half-length positions and unit unoriented tangents; Q1 24 END unaffected | **UNRUN** |
| Q2-M02 exact native 3D open Arc MID | `pm02p.e_kernel_lifecycle`: signed sweep positive/negative/wrapped open circular Arc, exact 3D half bounded arclength not chord; closed circle negative; real Part partial Revolve Line/open Arc | **UNRUN** |
| Q2-M03 cold provider | Real new OCCT provider and reconstructed Part; all MID semantic references survive, own new Body token, without XYZ or enumeration matching | **UNRUN** |
| Q2-M04 upstream edit / FeatureId decoy | Real Extrude height change with MID source continuity, actual MID position movement, identical shape/new producer FeatureId stays `edge_missing` | **UNRUN** |
| Q2-M05 typed source negatives | Foreign `DocumentId`, absent ownership evidence, stale revision, bad Stage, missing/split/unsupported Curve and missing current Edge, no partial observation | **UNRUN** |
| Q2-M06 genuinely foreign Body / Q0 failures | A test-only binder returns Edge tied to a **different actual OCCT Body**, unlike an innocuous different provider instance; forced native failure, nonfinite MID, zero tangent/length rejected atomically | **UNRUN** |
| Q2-M07 Point semantics untouched | Real half-cylinder Cut ambiguity remains global Ambiguous/Unsupported, no synthetic MID-as-Point; strict Edge's MID is independent of that Point's status; real Revolve positive | **UNRUN** |
| Q2-M08 Q0/Q1 regression | Registered `pm05b2.strict_edge_resolver`, `pm05c2b.part_edge_topology`, `pm02p.e_kernel_lifecycle`, then native FOCUSED, FAST, FULL, DOC-01, and no product-UI claims | **UNRUN** |

## Actual CI, code SHA and DOC-01

| Gate | SHA and URL | Result |
| --- | --- | --- |
| Q2 native FOCUSED `pm05c2b.part_edge_topology` + `pm02p.e_kernel_lifecycle` | Not yet issued | **UNRUN** |
| Windows FAST `pm05b2.strict_edge_resolver` and desktop regression | Not yet issued | **UNRUN** |
| Windows Ready FULL Core + Native + Desktop | Not yet issued | **UNRUN** |
| DOC-01 canonical Markdown → Browser parity | Not yet issued | **UNRUN** |
| Owner technical FINAL PASS and **separate** production PR-specific squash permission | Not requested | **PENDING** |

**No green Q2 outcomes are claimed until the exact implementation SHA has compiled and executed the registered native suites.** A docs-only classification/closure run does not qualify. Any failed test (RED) must be preserved here and fixed without relaxing the accepted algorithm. After FULL on source/tests/docs and later evidence-only updates, prove an exact two-SHA allowlist diff and rerun final-head governance validation.

## Architecture boundaries

`SelectedMaterialEdgeMid{MaterialEdgeReference}` is a distinct **Part-local semantic value**, never a Q1 `SelectedMaterialEdgeEnd`, transient Vertex token, arbitrary parameter or global Point. Strict author+replay is read-only, current owning DocumentId/live revision/exact successful Feature Stage, current native same-Body material Edge and Q0 half-bounded-arclength MID. A globally ambiguous Point is never promoted. No persistence, signed Datum frame/Reverse, Viewer picker, user-facing Datum constructor, schema v17 or new Q2+ scope. All failed observations leave Edge/XYZ/tangent empty and report typed status.
