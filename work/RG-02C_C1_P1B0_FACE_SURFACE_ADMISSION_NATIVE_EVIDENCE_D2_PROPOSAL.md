# RG-02C/C1-P1-B0 — picked cylindrical Face versus durable semantic Surface: native admission evidence D2 proposal

**Status:** OWNER ACCEPTED EXACT P1-B0 D2 TWO-PATH NATIVE EVIDENCE CONTRACT — 2026-10-16; GOVERNANCE PR #436 STILL DRAFT / NOT MERGED; B0 NOT ACTIVE OR EXECUTABLE.  
**Date:** 2026-10-16.  
**Decision class:** D2 architecture/evidence planning; later persistence/product/UX choices remain independently Owner-gated.  
**Baseline:** protected `main` `8488b8c20077bb39689cfb009d79686f1082db71`, resulting from separately Owner-authorized guarded-squash [P1-A PR #433](https://github.com/MechanicalCave/SimpleSolid2/pull/433).  
**Program:** [Part Modeling v1 roadmap](PART_MODELING_V1_ROADMAP.md) accepted v1.33, RG-02 Datum Plane + Axis focus; Foundation 1.0, Architecture 1.0 and accepted ADR-0014, ADR-0016, ADR-0017.  
**Prior evidence:** P0 [native Face/Surface evidence](RG-02C_C1_CYLINDRICAL_FACE_AXIS_NATIVE_EVIDENCE.md); P1-A [exact Face-axis query native evidence](RG-02C_C1_P1A_EXACT_CYLINDRICAL_FACE_AXIS_QUERY_NATIVE_EVIDENCE.md); product-intent boundary in [C1/P1 proposal](RG-02C_C1_P1_EXACT_CYLINDRICAL_FACE_AXIS_D2_PROPOSAL.md).

## Recorded Owner exact D2 acceptance — separately gated governance

On 2026-10-16 the Agent explicitly asked: **“czy akceptujesz dokładny, dwuplikowy kontrakt dowodowy P1-B0 D2 z PR #436?”** and expressly excluded permission for proposal merge, `ACTIVE.yaml` activation and product implementation. The Owner answered **`akcepyuję - kontynuuj`** (affirmative acceptance). This is **acceptance of this exact B0 native evidence contract**, limited to future additive use of the already registered OCCT test and one factual evidence ledger, with honest GAP/RED handling and the real FOCUSED → FAST → FULL → DOC-01 → exact-head CLOSURE gates.

**Not accepted by that answer:** any durable Axis source/persistence or schema choice; option A/B/C (strict Face versus whole Surface carrier versus future patch discriminator); a product-facing `Constructor: Cylindrical Face` UX; new Part/Kernel public APIs; starting B0 test mutations before dedicated activation; merging PR #436 or any later implementation PR. The sole Draft PR changes only this proposal file. The initial [Windows governance CLOSURE #3347](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38908131921) **SUCCESS** at `e52363438025c4400a32a9d589a5c572bd3c3bc4` preceded this decision-only documentation change; the updated PR HEAD must pass a **new exact-head CLOSURE** before asking separately for a guarded governance squash.

## 0. Actual upstream checkpoint; no implicit extension authority

C1-P0 was Owner-final and merged as #429. P1-A is likewise **Owner technical FINAL PASS with explicit native fixture GAPs deferred** and was guarded-squash merged to `main` by #433. Genuine independent Windows P1-A FOCUSED #3342 (1/1), FAST #3343 (93/93), FULL #3344 (199/199; Core 25 / native Kernel 57 / Desktop 117, official DOC-01 Browser parity), and final exact-head CLOSURE #3346 SUCCESS are recorded in the P1-A ledger. Historical #3335/#3336 DOC-01 RED remains factual. P1-A supplies a **read-only, evaluation-local** `IExactCylindricalFaceAxisQuery` of the **actual current Body-bounded Face**; it does not supply a durable Part Axis source, persistent selector, picked-Face admission rule or UI.

The current `work/ACTIVE.yaml` still points to the completed P1-A Work Contract and roadmap 1.33; this **work-only draft proposal does not update ACTIVE**, activate P1-B0 or claim continuation production authority. In particular, the Owner's authorization to squash #433 only integrated P1-A. The Owner's later response explicitly **accepts the exact D2 B0 evidence scope**, but **does not select option A/B/C below or authorize merge/activation/product mutation**.

## 1. Precise architectural question — not a UI question

The user's intended source is **one picked bounded Face of the current resulting single Body**, not a producing operation. P1-A reads that Face's *native* analytic cylinder line and radius, even for a partial/trimmed patch. Part, however, persists **semantic provenance**, not the provider token of the picked Face.

Existing Part architecture differentiates:
- `BodyFaceTopologyRecord.runtime_token`: **transient** current bounded Face identity; `surface_candidates` can be zero, one or many;
- `SurfaceReference{BodyStageRef,FeatureSurfaceAddress}`: **durable**, stage-qualified semantic Surface carrier;
- `FeatureSurfaceResolution.status`: semantic carrier resolution, independent of `strict_face_status`;
- `FeatureSurfaceResolution.current_faces`: one or several runtime bounded Face realizations for a single resolved semantic Surface;
- `FeatureSurfaceResolution.strict_face_status`: may be `ambiguous` when the carrier itself remains uniquely `resolved` after Boolean partition;
- `kernel::SurfaceKind::cylinder`: carrier classification, not proof by itself that all current native Face realizations produce the **same** analytic axis/radius.

These are implemented in `src/part/include/simplesolid2/part/semantic_topology_reference.hpp` and `feature_evaluation.hpp`, and exercised in existing `tests/kernel_native/pm04c2_production_revolve_test.cpp`. ADR-0016 and ADR-0017 demand strict stage/provenance and prohibit geometry-nearest identity rebinding. Persisting `RuntimeFaceToken`, `TopoDS_Face`, current Face ordinal, a cached world line or a fitted/same-radius substitute is forbidden.

**Question for a future D2:** When a picked bounded Face has **exactly one certified semantic Surface carrier** that is **resolved**, **cylindrical** and may own **multiple current bounded Faces**, is it legal to author a durable Axis source naming that **entire Surface carrier**? Or must product authoring require a uniquely resolved **strict bounded Face**? How should a future repair flow react to changes in partition, analytic carrier geometry, stage and line orientation?

## 2. Deliberately open D2 alternatives (no option selected in this draft)

**A — strict single Face only:** admit a picked Face only when its carrier and **strict bounded Face** both uniquely resolve at the declared stage and `current_faces.size()==1`. Fail closed on every certified multi-Face partition even if all native patches lie on the same exact cylinder. Smallest identity claim; potentially rejects a sound Axis when the cylinder was merely subdivided.

**B — certified semantic carrier with native geometric consistency:** admit a picked Face when its **unique resolved semantic Surface** has `SurfaceKind::cylinder` and **each** of that carrier's current bounded Face realizations is independently verified through the existing P1-A provider query to lie on one **same infinite unoriented analytic cylinder axis** with the **same finite positive radius**, at one valid source `BodyStageRef` and one current runtime Body generation. `strict_face_status==ambiguous` **alone** would not be rejection if the ambiguity consists solely of certified current partitioned realizations belonging to the one carrier. Fail closed if membership, semantic ownership, actuality, geometry or stage cannot be proven. **This is the provisional direction to test, not an approved authoring rule**; native counterexamples may disqualify it. It must not silently mean the originally picked *patch* is durably recovered—only the carrier-derived Axis intent could be.

**C — durable bounded-patch discriminator:** introduce an independently proven structural semantic patch identity when the engineering requirement truly depends on the particular bounded fragment. This risks new identity, branch-selection and persistence machinery, and must not be introduced without a genuine observed case that A/B cannot represent. **Out of B0 and not approved.**

Do not collapse the choice by guessing that equal axis/radius implies semantic identity: **proven unique stage-semantic ownership comes first**, then analytic consistency is a numerical admission predicate *inside* that ownership. Equal/coaxial cylinders with different Feature provenance stay different source candidates.

## 3. Owner-accepted single P1-B0 evidence-only D2 contract — activation separately gated

**One deliverable:** a read-only native OCCT/Part-stage admission matrix distinguishing selected bounded Face, semantic Surface owner, strict Face status, all owner-current Face realizations and their P1-A geometric cylinder queries. This stage chooses **no durable Axis product semantics** and writes no provider/interface/Part schema/UI implementation.

**Exact TWO paths ACCEPTED by Owner (executable only AFTER this governance proposal is separately squash-merged and dedicated ACTIVE-only activation is independently Owner-authorized and merged):**

1. `tests/kernel_native/pm04c2_production_revolve_test.cpp` — additive assertions/helpers in the **existing already-registered** OCCT native target, reusing C1-P0 and C1-P1-A fixtures and the current public provider/Part catalog; no duplicated standalone tests/CMake.
2. `work/RG-02C_C1_P1B0_FACE_SURFACE_ADMISSION_NATIVE_EVIDENCE.md` — new factual evidence ledger with counts, PASS/RED/GAP, actual cases and relevant run/commit SHAs.

**No other source/test/docs file is executable under this proposed contract.** This proposal itself changes **only this `work/**` file**. No `src/**`, provider API, `PartAxisSource`, native v19 schema/writer/readers, `work/ACTIVE.yaml`, roadmap, ADR, CMake/CI, GUI/Viewer/Tree, Operations/Command Line, user PL/EN or `docs/browser/index.html` is allowed as B0 implementation. If reality demands an additional file, STOP for a separately accepted exact-path D2 amendment. No weakening or deleting existing tests.

### B0-01 — read-only picked-Face provenance gate

For the actual **same current stage and Body**, enumerate the complete `BodyStageTopologyCatalog.faces` and current `surfaces`. Choose actual Face inventory tokens; do not fabricate persisted selectors. Classify zero, one and multiple Face `surface_candidates`; assert **only** exactly one existing certified `FeatureSurfaceAddress` matching one `FeatureSurfaceResolution.status==resolved` can be tentatively considered. Prove selected token occurs exactly once in that `current_faces` collection. Empty stage, wrong Body generation, unresolved/ambiguous/missing/unsupported carrier, no Face inventory, or duplicate membership must refuse (or be reported as a genuinely unconstructable GAP), never pick first by provider order.

### B0-02 — the strict-Face versus Surface partition discriminator

Record `surface.status`, `strict_face_status`, `current_faces.size()` and **which verified native fixture produced them**, individually for full/partial Revolve, circular Extrude, genuine Boolean Cut/trim and any real provider-produced multi-Face cylinder carrier. **Require a witnessed `status==resolved`, `strict_face_status==ambiguous` and `current_faces.size()>1` on a genuinely cylindrical carrier before claiming B's partition-positive branch executed.** If no such OCCT outcome is produced within the approved file, report **GAP / NOT TESTED**, not an invented partition proof. A circular-rim Fillet/Chamfer must not be misclassified as a straight-edge Fillet cylinder.

### B0-03 — exact all-member analytic consistency (B candidate, not authoring)

For **every actual current Face** in one uniquely proven carrier, independently `bindExactCylindricalFaceToBody` then `queryExactCylindricalFaceAxis` using the **same current RuntimeSolidHandle**. Positive only if each returns `ok`, valid finite geometry/radius, line direction equal modulo ±, minimum perpendicular line-to-line distance within explicit existing-mm numerical policy, and radii equal within an independently stated tolerance; no inferred result from Feature kind, `SurfaceKind` alone, cap geometry, a single picked Face, or order. If one candidate fails, classify carrier Axis admission **as failed**, never fall back to another Face. Do not introduce a public 'authorAxisFromSurface' helper during B0.

### B0-04 — negative/stage/invariance evidence

Include actual planar/non-cylinder refusals; different Body generation, null/invalid/stale Face, wrong stage, orphaned or nonunique carrier and same-geometry-different-provenance non-rebinding **where native fixtures can genuinely witness them**. Compare stable source semantic address across lawful upstream rebuild only as expressly demonstrated, never assume global preservation. Show signed `gp_Dir`/Face orientation is not an authored Axis sign; do not claim an actual native `TopoDS_Face` reversal test if not executed. Clearly enumerate unsupported fixture GAPs, including the P1-A Owner-deferred straight-edge Fillet cylinder, native orientation reversal and fault/degenerate cases; P1-B0 does not silently reopen those P1-A accepted gaps as historical failures.

### B0-05 — decision matrix deliverable

The B0 ledger must tabulate at minimum for each **actually executed** fixture: source stage, native selected Face count, number of certified Surface candidates, unique carrier status, `strict_face_status`, carrier `current_faces` count, each native query outcome, radius/line consistency, and separately A/B authorability predicates. Explicitly distinguish:
- a strict singleton cylinder Face **supported by both A and B**;
- certified multi-Face same-cylinder carrier **B candidate only** (if actually witnessed);
- nonunique semantic Surface ownership / invalid stage **neither**;
- one resolved Surface whose native Face geometries are inconsistent or noncylindrical **neither**;
- the absence of a producible partition fixture **OPEN GAP**, not acceptance of B.

Keep acceptance denominators and actual logs, not invented cardinalities. If separate observer stdout requires a logging option unavailable in normal successful CTest, record what assertions actually ran and do not invent per-case counts.

## 4. Mandatory test and governance gates if B0 is independently activated

- Real Windows OCCT native FOCUSED on existing `pm04c2_production_revolve_test` / `pm04c2.production_revolve`; independent Draft FAST; Ready FULL Core-only + Kernel-native Release + Qt/OCCT Desktop; official DOC-01; exact-head evidence-only Windows CLOSURE. Existing tests and DOC-01 must remain intact.
- RED means RED. Native compile failures, missing partition fixture, wrong-Body ambiguities, tolerances that make geometric comparison unsafe or unexpected noncoaxial members require diagnosis and either a bounded explicit Owner D2 amendment or truthful GAP.
- Owner technical B0 FINAL PASS is an independent decision with explicit gap deferrals if any; the eventual implementation PR guarded squash requires its own Owner authorization.
- A **separate** future decision must choose A/B/C after reviewing B0 evidence. Only then prepare Owner-reviewed `Constructor: Cylindrical Face` UX compatible with the existing Work Features Axis (currently **one accepted Edge constructor**) and a separate Part-owned lifecycle/persistence/legacy native v19/readback/Revolve contract. No automatic widening of the Axis selector, UI signoff, D2 implementation, model schema writer version or `ACTIVE.yaml` switch from B0.

## 5. STOP matrix and independent decision sequence

1. **Now**: the one-file D2 proposal has exact Owner scope **ACCEPTANCE**; its Draft PR remains a review artifact only. No merge, activation or native tests are implied.
2. **Owner D2 scope decision — CLOSED / ACCEPTED 2026-10-16**: exact two-file B0 evidence contract approved as written, **without automatic governance merge permission**.
3. **Governance merge decision**: after real exact-head governance Windows check, independently authorize protected-main squash of the proposal.
4. **ACTIVE-only activation decision**: separately propose and merge one `work/ACTIVE.yaml` pointer/history update after Owner approval and its own exact-head CLOSURE.
5. **B0 evidence execution**: only from activated main and only inside the two approved files; actual Windows gates and separately accepted technical PASS/merge.
6. **D2 product semantics + separate Owner UX**: choose Face vs Surface/patch meaning using actual proof, followed by a new **exact** bounded product Work Contract, separate activation and normal full lifecycle native schema/repair/Undo/Revolve/product DOC-01 evidence.

**Non-goals:** no P1-B product implementation yet; no v20/native schema or migration; no transient provider identity persistence; no `AxisReference` breaking change; no implicit signed Axis choice/Reverse default; no B1 Edge Axis or existing Sketch-Line Axis behavior changes; no arbitrary two-Body source, generic TopoNaming, PM-06, G1, Mirror/Pattern, Assembly or unrelated work.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This governance-only proposed bounded read-only native-test evidence decision adds no shipped interface, product capability, user workflow or as-built change. A later independently activated B0 test-only scope also changes no shipping contract; any future accepted Part Axis authoring/UX scope must separately declare Internal, PL/EN and generated Browser obligations.
