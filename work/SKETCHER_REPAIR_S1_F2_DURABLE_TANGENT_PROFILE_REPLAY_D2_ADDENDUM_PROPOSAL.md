# Sketcher S1/F2 — durable interior tangent Profile anchor replay — bounded D2 ADDENDUM PROPOSAL

**Status: D2 PROPOSAL / NOT OWNER ACCEPTED / NOT MERGED / NOT ACTIVE FOR ADDITIONAL PATHS.** Date 2026-10-17. No production permission is inferred from this document. Existing accepted S0/S1 Work Contract is `work/SKETCHER_REPAIR_S0_S1_CONTACT_REGION_PROFILE_D2_WORK_CONTRACT_PROPOSAL.md`; its exact original **15 conditional implementation paths, test gates and STOP rules remain in force**. No Foundation, schema or ADR is changed.

**Owner-signed direction preserved:** Sketcher B01/B02 before S2/B03a/B04 and S3/B03b/B05/B06. Existing Sketch Roadmap v1.10 and Part Modeling v1.34 accepted; ACTIVE-only [#459](https://github.com/MechanicalCave/SimpleSolid2/pull/459) merged main `0eec634398f235d3be282f1a223aeaca41c3854c`. Implementation Draft [#460](https://github.com/MechanicalCave/SimpleSolid2/pull/460) has independent S0 RED evidence and bounded Shared-2D S1 changes. This addendum is proposed because current native/Part durable Profile acceptance is blocked at the accepted `src/part/**` boundary, which is outside §5's 15 paths.

## 0. Exact Owner D2 decision sought

**Authorize exactly TWO additive implementation paths**, and no other source/test path:
1. `src/part/profile.cpp` — amend only `resolveProfileRegionIntent`'s narrow semantic **intersection-anchor replay** policy for exactly certified **interior tangent** contact; preserve fail-closed stale/missing/ambiguous semantics, `ProfileId`, `RegionIntent` payload and current native schema. No permission to modify any other Part source or public header.
2. `tests/sk02b_part_sketch_model_test.cpp` — add F2 interior Circle/Line two-cell durable replay and real Add/Subtract negative/positive assertions in the existing registered Part/Sketch test; preserve existing **two externally tangent disks = disconnected material** regression unchanged. No CMake/test-target edits.

All **previous 15 paths** and every original requirement remain unchanged and conditionally allowed under the activated parent contract. The new two paths become editable **only after** this exact addendum is Owner-accepted, independently guarded-squash integrated to protected main after its own exact-HEAD Windows governance CLOSURE, and any required separate ACTIVE-only addendum/activation record has also been approved and integrated. **NO implicit autoactivation or merge from accepting this proposal.**

## 1. Actual current-run evidence: one RED boundary, not speculative scope

On the activated main `0eec6343...`, S0 first registered genuine Windows RED before editing production sources:

- [#3464](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38974425176), test-only SHA `6c63734bb7821818a4c38f88790c1f84b5a943c8`: F1 12/24 zero regions and 12/24 one; F2 24/24 one; FAST 92/93 (expected intentional new B01 RED).
- [#3465](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38974691902), test-only SHA `e75573939279ba842264d39261c979b2f6dcd065`: F3 both exact oblique tangent contacts incorrectly `disjoint`, zero intersections, genuine B02 RED.
- After bounded `region_analysis.cpp` fixes, Windows FAST [#3471](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38975536644) **93/93 SUCCESS** on `d6ae2dda886da9726c94b9c9cefb551baf8bddfa`, including existing `sk02b.part_sketch_model` point-contact/no-connected-material regression.
- A new **positive durable Part Profile replay requirement** was added solely in already-allowed `tests/pm02f_stage_aware_profile_evaluation_test.cpp`. Windows FAST [#3472](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38975757976) at `233fd50617a2bdadcbc7c6198b1ec108233c3a50` **RED**, 92/93, exactly one newly added `pm02f.stage_aware_profile_evaluation` assert:
  - `SKETCH_S0_B01_ENDPOINT_24 correct=24 ... region2=24`
  - `SKETCH_S0_B01_INTERIOR_24 correct=24 ... region2=24`
  - `SKETCH_S0_B02_F3_RELATION left=1 right=1 ... status=2` (`discrete`)
  - `SKETCH_S0_PROFILE_REPLAY_SUMMARY extended=0 resolved=2 expected=2` (F1 durable PASS)
  - **F2:** `SKETCH_S0_PROFILE_REPLAY extended=1 region_area=50.7301 status=4` where Part `ProfileIntentResolutionStatus::unresolved_intent == 4`; `extended=1 resolved=1 expected=2`.
  
This is a genuine **remaining defect** after successful Shared-2D cell enumeration: the F2 lower pocket's exact two interior Line/Circle tangent anchors cannot be replayed in the Part durable Profile model. It is NOT a failed numerical tolerance or a failed Circle/Line geometry test. Do not claim F2 durable/Profile/Extrude/native PASS until fixed and verified.

**Exact as-built restriction in unchanged `src/part/profile.cpp`:** `resolveProfileRegionIntent` → `resolve_anchor` obtains the current `analyzeCurveRelation` result and explicitly returns `std::nullopt` whenever `found->contact == tangent && !found->first_endpoint && !found->second_endpoint`. This was a conservative old fail-closed rule preventing arbitrary interior tangent binding, but now prevents deterministic replay of F2 region anchors after S1 makes them actual semantic arrangement vertices. Since the original contract §5 does NOT allow editing `src/part/profile.cpp`, proceeding with a code patch there requires exact independent Owner D2 scope approval. Do not forge endpoint flags or transform anchors into disposable geometry.

## 2. Narrow semantics of proposed Part replay correction

1. Permit replay of a given **interior tangent anchor** ONLY when the current source entities and exact canonical semantic branch uniquely resolve at the current Sketch revision, their current analytic `CurveRelationStatus` is `discrete` and `CurveContactKind::tangent`, and the anchor matches an actual **current bounded arrangement vertex** used by a valid current cell/loop. A tangent relation alone is not blanket permission. Fail closed on missing source, non-regular source, stale/ambiguous relation/branch or incomplete/invalid current topological context.
2. Preserve distinct authored source references and `RegionBoundaryAnchorKind::intersection` plus canonical branch: no closest-point, no recomputed ephemeral EntityId guess, no generated new durable role, no schema changes, no retroactive remap of stale user intent.
3. Validate replayed loop/holes and material topology with existing `sketch::validateRegionBoundary`; two circles that only touch at one vertex **must** remain distinct material islands, their Add Area remains `disconnected_result`. Neither tangent nor identical viewport position implies a material edge. Do not remove or dilute existing negative tests.
4. Existing accepted F1 (Line endpoint contact), independent Circle Profile, and all current valid legacy Profile intent must replay exactly as before, including Save/Close/Reopen and undo. New positive F2 lower pocket replay should survive intent round-trip, Part-owned stable `ProfileId`, cold native persistence/OCCT Profile Face evidence and dependent Extrude; downstream integration tests remain in parent allowlist.
5. If a valid F2 interior anchor cannot be unambiguously admitted without changing the **public** `ProfileRegionIntent` schema, ownership, geometry tolerance, source identity or other Part paths, **STOP for another D2 decision**; do not widen this amendment or create ad hoc auto-healing.

## 3. Test matrix and formal gates

- Existing FAST `pm02f.stage_aware_profile_evaluation` F1/F2 24-permutation and F3 relation; **new** `sk02b.part_sketch_model` positive durable F2 replay with reversible line/arc orientation and current stage, no relaxation of externally tangent Circle/Circle `disconnected_result`.
- Existing allowed `tests/pm02h_face_supported_extrude_workflow_test.cpp`, `tests/pm02i_face_supported_lifecycle_survival_test.cpp`, `tests/kernel_native/pm00a_profile_cold_rebuild_test.cpp` and R12 tests cover actual Profile composition and native OCCT cold lifecycle as applicable. Do NOT claim any unexecuted fixture PASSED.
- Real Windows focused registered tests, independent FAST, Ready FULL Core+native OCCT+Qt Desktop, official DOC-01 PL/EN generated Browser, final exact-head CLOSURE, manual Owner Windows and separate technical FINAL PASS/guarded merge remain binding.
- This governance document itself changes **one `work/**` path only**. Before any executable Part edit, independently Owner-accept exact addendum, pass separate Windows governance CLOSURE, obtain separate guarded squash authorization, and complete any independently required active-scope addendum sequencing. No activation/production source code is authorized by this proposal.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: Accepted B01/B02 repair already requires documenting as-built geometric tangent arrangement, exact durable Profile intent replay, resulting visible Profile/Trim behavior and cold persistence in canonical internal and Polish/English product docs. This addendum proposes no new documentation file outside the original accepted S0/S1 documentation allowlist.
