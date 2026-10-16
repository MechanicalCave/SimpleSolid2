# RG-02C/C1-P1-B0 — native picked-Face / semantic Surface admission evidence

**Status: IMPLEMENTATION EVIDENCE IN PROGRESS — NO NATIVE P1-B0 PASS YET.** This ledger records only actually executed observations and does not establish A/B/C durable Axis source semantics or a Part Axis product. Baseline: protected `main` `6ff550a0b448300a0441954ab67825f7fbecc7c7` after separately Owner-authorized, Windows-CLOSURE-verified ACTIVE-only [PR #437](https://github.com/MechanicalCave/SimpleSolid2/pull/437), which activates the exact already Owner-accepted [P1-B0 D2 evidence contract](RG-02C_C1_P1B0_FACE_SURFACE_ADMISSION_NATIVE_EVIDENCE_D2_PROPOSAL.md) merged through governance [PR #436](https://github.com/MechanicalCave/SimpleSolid2/pull/436).

## Exact scope and test wiring

The D2-allowlisted change paths are **only** the already registered OCCT `tests/kernel_native/pm04c2_production_revolve_test.cpp` and this new factual `work/RG-02C_C1_P1B0_FACE_SURFACE_ADMISSION_NATIVE_EVIDENCE.md`. The native test adds only a read-only helper `observeC1P1B0NativeFaceSurfaceAdmission` to existing genuine resulting single-Body Part evaluations. It does **not** create a Part-level authoring/resolution function, write a model or change persistence, UI, Kernel API, docs or build targets.

The helper independently:
- enumerates each current actual bounded Face token in the complete stage catalog; obtains a real provider-minted Face scope and uses the existing P1-A exact analytic cylinder query;
- rejects noncylindrical Faces, and refuses prospective durable A/B admission when `surface_candidates.size()!=1`, the unique Surface carrier is not `resolved` and `cylinder`, or the selected token is not exactly once in its current member collection;
- examines **every current bounded Face member of that one certified carrier** (unique inventory membership, unique carrier address, Body-scoped binding and separate analytic query);
- compares finite analytic radius within 1e-6 mm, squared direction cross under 1e-10 (unoriented ±), and squared perpendicular axis separation under 1e-8 mm²; it does **not** treat equal geometry or Feature kind as source identity;
- records **hypothetical A** only when a certified cylinder has strict singleton bounded Face `resolved` and exactly one member, and **hypothetical B** only when every member of that one uniquely proven semantic carrier returns consistent native cylinder geometry; neither rule is shipped or accepted product design;
- counts real partitioned semantic cylinders only when one `resolved` carrier has `strict_face_status==ambiguous`, `current_faces.size()>1`, and each actual member passes the analytic all-member check. No partition-positive result may be claimed until that counter is genuinely observed nonzero.

Existing P1-A tests of invalid/null bound Face and different Body generations are retained and run in the same registered target; B0 does not falsely claim a new independent native exception or true Face-orientation reversal fixture.

## Intended fixture matrix — not executed yet

| Existing native Part fixture | Source Body stage | Native Face / Surface matrix | Analytic member proof | A/B candidates | Execution |
| --- | --- | --- | --- | --- | --- |
| Circular-profile Extrude full cylinder | actual current `after_feature` | native Face enumeration; status, strict Face, membership | each member separately queried | conditional on real observation | **PENDING** |
| Full 360° Revolve | actual current `after_feature` | same | same | conditional | **PENDING** |
| 90° partial Revolve | actual current `after_feature` | same | same | conditional | **PENDING** |
| Half-cylinder Boolean Cut | actual current `after_feature` | same | same | conditional | **PENDING** |
| Real circular-rim Fillet/Chamfer | actual current `after_feature` | same, no assumed cylinder | classify toroidal/other vs actual native cylinder | conditional | **PENDING** |
| One semantic carrier with more than one current analytic-cylinder Face | source-stage exact | `resolved` + `strict_face_status=ambiguous` + `current_faces>1` | all-member same axis/radius required | B-only if genuinely witnessed | **GAP / NOT YET WITNESSED** |
| Wrong Body generation, invalid/null native Face | previous P1-A registered tests | negative provider checks reused | no standalone P1-B0 fixture claimed | neither | **PENDING re-run** |

`RG02C_C1_P1B0_FACE` diagnostic lines include per certified native picked cylinder Face: fixture, stage, strict Face status, current member count, all-member geometric consistency and hypothetical A/B predicate. `RG02C_C1_P1B0_SUMMARY` prints per-fixture counts. These are **test instrumentation**, not persisted selections or feature behavior. Successful ordinary non-verbose CTest output may hide these lines; do **not** invent counts merely from a green aggregate test.

## Real Windows validation — pending

| Gate | Exact source HEAD / run | Result |
| --- | --- | --- |
| Windows native FOCUSED `pm04c2.production_revolve` (real OCCT/MSVC) | Not run yet | PENDING |
| Independent Windows Draft FAST | Not run yet | PENDING |
| Windows Ready FULL Core-only, Kernel-native, Qt/OCCT Desktop, DOC-01 | Not run yet | PENDING |
| Final evidence-only exact-head CLOSURE | Not run yet | PENDING |
| Owner B0 technical FINAL PASS | Not requested | PENDING |
| Independent guarded squash of evidence PR | Not authorized | NOT AUTHORIZED |

## Deliberate STOPs and inherited GAPs

B0 cannot decide strict Face vs same-Surface carrier vs a possible future semantic patch discriminator without review of genuine fixtures. No witnessed multi-Face analytic-cylinder carrier means **B partition-positive GAP/NOT TESTED** despite possible strict-singleton PASS. Explicit B0 negatives for orphaned/nonunique owner, live stage mismatch, inconsistent cylinders within one uniquely certified carrier and equal-geometry-different-provenance cases remain **NOT TESTED** unless a true available fixture executes that branch; code containing a conditional is not native evidence that it fired. Existing Owner-deferred P1-A straight-edge Fillet genuine cylinder positive, actual native reversed `TopoDS_Face` topology, exception/fault/degenerate geometry and independent torus/cone negatives remain deferred and are not promoted to PASS by B0. No `PartAxisSource` update, schema v20, UI, new `Constructor: Cylindrical Face`, persistence, Revolve consumer, `src/**`, more paths, or product artifact.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: Additive internal native tests and a factual governance ledger without shipped interface, UX, persistence or runtime semantic changes.
