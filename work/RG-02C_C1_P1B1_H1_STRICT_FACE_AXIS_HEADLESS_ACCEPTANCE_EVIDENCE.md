# RG-02C/C1-P1-B1/H1 — strict singleton cylindrical Face Axis: execution evidence ledger

**Status: H1 WIP / NOT TECHNICAL FINAL PASS / NOT AUTHORIZED TO MERGE.** Date: 2026-10-16. This file records only actually returned Windows results. Native Part writer v20 and its real native positive roundtrip test are now **IMPLEMENTED IN DRAFT**, but native positive Save/Reopen/Cold Rebuild remains **NOT YET WITNESSED on current HEAD** pending a completed true OCCT FOCUSED. Earlier source-version authorization is complete.

**Active contract:** `work/RG-02C_C1_P1B1_H1_STRICT_CYLINDRICAL_FACE_AXIS_HEADLESS_D2_WORK_CONTRACT_PROPOSAL.md`, activated by independently authorized and Windows-green #443, squash `249d677cdb98213838643a02ca9d045f78e6d6a8`; architecture A (single current strict bounded Face), Owner UX design #439 separately accepted but no H2 code authorized. The exact additional `src/part/include/simplesolid2/part/part_document_store.hpp` v19→v20 header path was separately Owner-accepted on 2026-10-16 in governance proposal #445. Owner explicitly authorized separate guarded-squash merges of #445 and #446; governance-only [#445](https://github.com/MechanicalCave/SimpleSolid2/pull/445) merged `4627d8c7c72def49cdc47de69f47d7049b1ca12f` after original CLOSURE #3372 and Ready #3374 SUCCESS; active-contract [#446](https://github.com/MechanicalCave/SimpleSolid2/pull/446) merged `b4265c53e5b6677af566733fdfc3deafc14fcad8` after original CLOSURE #3373 and Ready #3376 SUCCESS. Protected `main` is now `b4265c53e5b6677af566733fdfc3deafc14fcad8`; exactly 19 paths are ACTIVE. The sole newly approved header `current_schema_version` was then updated 19→20 in Draft #444; no other change to that header.

## Actual Windows native execution and process provenance

| Real Windows run | Exact SHA | Result actually observed | Interpretation |
| --- | --- | --- | --- |
| [FOCUSED #3370](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38919823886) | `412de0650d7c8aa41b7a9fb91e007312cd8f4547` | **SUCCESS**, MSVC Release native registered `pm04c2.production_revolve` **1/1 PASS** (0.24 s), official DOC-01 PASS | First implementation stage: strict real bounded Face→Surface authoring, native P1-A query, source evaluation, some native fixture assertions. Does NOT prove subsequent changes |
| [FOCUSED #3375](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38925997533) | `35429a4364fc221fb845e0e0ebf1834ae61943d3` | **SUCCESS**, FOCUSED Windows tier | Compiles later allowable cpp native-v20 serialization preparation; still header v19, so source save must fail closed instead of falsely writing v19; does NOT prove a v20 roundtrip |
| #3377 / #3378 | `638e7f014bc98b3c71fe5c3f7e419e81280479b3` / `3b27dd238f9161a98400eb0f9c1737aa4d74d06c` | **CANCELLED** by superseding WIP commits | Never count cancelled native runs as PASS |
| Latest native FOCUSED | Current PR HEAD | **PENDING / NOT TESTED at this HEAD** until actual independently completed Windows run | No extrapolation from earlier passed commits |
| [FAST #3381](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38926789346) | `3752b516adf42387be79e14b486b4daf2c1a32aa` | **SUCCESS**, independent FAST tier with native H1 source draft before v20 schema bump | Earlier stage only; not current writer v20 |
| [FAST #3386](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38929211891) | `6b443bbf7a5c5434d1a77f08712c1d557ee4166a` | **93/93 PASS** Core FAST, `pm04a.axis_schema_v12` **PASS**, DOC-01 **PASS**, MSVC Windows | Current headless writer schema v20 and compatible legacy v19/test malformed source Core; not a real native OCCT cylinder roundtrip |
| Final Native FOCUSED after v20 | Next explicit Focus run | **PENDING / NOT YET TESTED** | Required real MSVC OCCT `pm04c2.production_revolve` with v20 Save/Reopen/Cold Rebuild |
| Ready FULL | Current eventual production HEAD | **PENDING / NOT TESTED** | Core-only, native Kernel/OCCT Release, Qt/OCCT Desktop, official DOC-01; no skip as PASS |
| Final exact-head CLOSURE | Final evidence-only production HEAD | **PENDING / NOT TESTED** | Windows governance/doc verification on actual final HEAD, separate Owner technical FINAL PASS / guarded merge |

## Actually implemented semantic checks (source code, not substitute for completed current-HEAD native proof)

- A picked current runtime Face is accepted only against an exactly matching owning DocumentId, same current DocumentRevision and original `BodyStageRef`, complete actual current Feature-stage Body topology, exactly one matching Face catalog record and exactly one semantic Surface candidate. Carrier is uniquely `resolved`, its `strict_face_status` is `resolved`, `candidate_face_count==1`, `current_faces.size()==1`, and it is analytic cylindrical by real provider native query, not from a role name.
- Provider mints a `ScopedExactCylindricalFace` for that stage's actual resulting Body, returns an exact *infinite unoriented* world-mm axis. The transient Face token, OCCT shape, radius and XYZ do NOT enter `PartAxisSource`; durable source is only `StrictCylindricalFaceAxisSource{SurfaceReference}`.
- Current native fixture exercises real full/partial Revolve, circular Extrude, half-cylinder Cut, circular-rim Fillet/Chamfer, and now explicit DocumentId mismatch, missing stage, authored Part restore and `DocumentSession` Create/Undo/Redo lifecycle probes. Native false branches will be marked PASS only after corresponding completed exact-HEAD FOCUSED proof.
- Native v20 encoding/decoding is prepared in the existing allowed `src/part/part_document_store.cpp`, using existing `FeatureSurfaceAddress` schema codec and strict five-field `cylindrical_body_face_strict` Axis record with `source_surface` stage/address. A preflight guard rejects a strict Face source while `current_schema_version<20`, rather than silently writing an invalid native v19 file. The exact one-path D2 header gate is OPEN/INTEGRATED after #445+#446. The header now declares native v20 in #444; negative Core v20/malformed/v19 tests passed FAST #3386. Real OCCT-positive v20 Save→Reopen→Cold Rebuild awaits separately witnessed native FOCUSED.
- No Part-owned UI Face pick/second Axis Constructor, no H2 `src/ui/**`, no new CMake/CI workflow, no provider ordinal or geometric-equality CAD identity.

## CI non-PASS honesty — exact recent attempts

The initial schema constant update run **#3382** was **RED due to an invalid FOCUSED test selector in commit metadata** (CI classifier `[focus] state=invalid`, no actual product test) and must remain in history as RED, not hidden or counted. Intervening #3383, #3384 and #3385 were superseded **CANCELLED**, not PASS. Later valid Windows FAST #3386 is the actual versioned Core evidence. The new real OCCT-positive native fixture must receive independent FOCUSED proof at the current implementation HEAD before FULL.

## GAP / NOT PROVEN — do not infer these from green aggregate CTest

1. A witnessed real single `resolved` cylinder Surface with **multiple** current native bounded Face realizations, `strict_face_status=ambiguous`, all with independently consistent cylinder axes/radii remains an inherited P1-B0 GAP. Variant A intentionally **rejects it**, never silently generalizes to B or invents a patch discriminator C.
2. Distinct native injections for an unrelated but geometrically identical provenance Face; all ambiguous-owner/wrong-stage/no-current Body negative fixtures; actual OCCT reversed-topology Face orientation; positive straight-edge Fillet-generated cylinder; injected exception/degenerate provider values remain NOT TESTED unless later exact-HEAD logs prove execution.
3. Core FAST #3386 genuinely executed the native v20 writer version checks, v19 Sketch Line compatibility conversion and rejection of unsupported/malformed new Axis kinds; **PASS only for those cases**. Real OCCT-positive v20 Save→Close→Reopen→Cold Rebuild and native Axis-derived Revolve source from headless Create/Edit, independent Ready FULL and Owner technical FINAL PASS remain **NOT YET WITNESSED / PENDING**. The newly added OCCT v20 positive test is not itself proof until genuine Windows FOCUSED executes.
4. User-facing Axis Constructor `Cylindrical Face` is **not implemented** in headless H1 and is separately gated H2 despite Owner accepting its UX design.

## STOP gates before completion

- **CLOSED:** independently Owner-authorized governance #445 (CLOSURE #3372/#3374 PASS) and active H1 contract #446 (CLOSURE #3373/#3376 PASS) are now separately guarded-squash MERGED. Active path #19 header's `current_schema_version` has been changed from 19→20 and Core native version/migration test has passed FAST #3386.
- Finish H1 native v20 writer/reader and earlier-version tests, headless Create/Edit/Revolve and schema life cycle **within exact 19-path amended allowlist**; investigate every actual Windows native RED rather than deleting tests.
- Independent Windows native FOCUSED, Draft FAST, Ready FULL Core+Kernel-native+Desktop, DOC-01, then fact-only ledger and final exact-head CLOSURE. Separate Owner **technical FINAL PASS** decision and later guarded squash authorization for implementation #444. No scope acceptance or earlier general implementation/merge intent substitutes for those gates.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This evidence ledger records the current WIP proof/gaps only; actual headless H1 completion requires the independently declared `docs/internal/PART_DOCUMENTS.md` as-built update and official DOC-01 verification, while product PL/EN/Browser UX changes are H2-owned.
