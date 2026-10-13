# RG-02B1/D2-P4-S1 — strict selected Edge-local signed Frame3 (prospective executable Work Contract)

**STATUS: PROPOSED COMPLETE OWNER D2 SCOPE — NOT ACCEPTED, NOT MERGED, NOT ACTIVE; NO SOURCE-CODE AUTHORITY.**
**Prepared:** 2026-10-13. **Verified `main` baseline:** `ac22ef5c3d2500e8dcfb2386be0bcb217757e61f` after Owner-accepted/merged design direction [PR #365](https://github.com/MechanicalCave/SimpleSolid2/pull/365), Q1 semantic END production #359, Q2 strict MID production #363 and Q2 closure #364. `work/ACTIVE.yaml.active_work` remains the **completed Q2 MID contract**; Foundation v1.0, roadmap v1.33 and current Part schema v16/modeling semantics v2 are unchanged. Owner subsequently accepted the **separate S1 → S2 → S3 execution sequence** in [preparation PR #366](https://github.com/MechanicalCave/SimpleSolid2/pull/366), which remains OPEN/DRAFT and does **not** authorize implementation or automatically merge this contract.

## 0. One bounded executable scope decision requested

**S1 is read-only and headless:** derive an initially deterministic **signed right-handed `kernel::Frame3`** from the **current, independently certified selected material Edge-local END or MID** with strict ownership/Body-stage checks; return separate, transient **orientation intent** (initial signed normal reference, normalized U seed, `reverse`) for caller-supplied subsequent re-evaluations. Preserve small-edit U/V/N orientation without guessing a new world-axis basis on each Part evaluation. Fail closed on significant line-direction change, degenerate stored seed or unavailable semantic source.

This is a **proposed numeric and API freeze**, not a claim that Owner has already accepted the constants below. Only explicit **complete-text D2 acceptance** makes them binding for a later implementation PR. S1 does **not** store authored orientation, create/edit a Datum, add `EdgePointDatumPlane`, change native schema, implement a picker or preview, or persist anything across Save/Reopen. Orientation-intent storage and schema versioning belong to S2; existing Datum Plane Reverse/Command Line/UI belong to S3.

## 1. Shipped semantic sources and strict trust boundary

- Q1 `SelectedMaterialEdgeEnd{MaterialEdgeReference, StableEdgeLocalTerminalKey{FeaturePointAddress}}` must have an independently Resolved selected terminal. Do **not** manufacture a terminal from global Ambiguous/Unsupported Point, nearest XYZ, native Vertex slot or display pick rank.
- Q2 `SelectedMaterialEdgeMid{MaterialEdgeReference}` is exactly the half **bounded 3D material arclength** MID; it does not require a global terminal Point or represent an endpoint.
- The shipped `resolveSelectedMaterialEdgeEnd(owner, source, evaluation, provider)` and `resolveSelectedMaterialEdgeMid(owner, source, evaluation, provider)` already check owning DocumentId, live revision, exact successful Feature/Body stage, strict material Edge and exact Q0 Line/open-Arc current Body geometry. S1 **must call those bounded read APIs**; it must not repeat or relax their source admission, read the final Body, infer semantic identity from XYZ, or bind provider-instance address as Body identity.
- A Q1/Q2 read returns `kernel::Point3` origin and `kernel::UnorientedUnitTangent3` current tangent line. `kernel::Frame3` fields are `origin`, `u_axis`, `v_axis`, `normal`; shipped `Frame3::valid()` checks only finite nondegenerate right-handedness, **not** unit length, strict orthogonality or continuity. S1 adds a **separate** orthonormal Frame checker; does not modify global `Frame3::valid()`.

## 2. Exact proposed additive public types and API

In existing `src/part/include/simplesolid2/part/feature_evaluation.hpp`, add:

```cpp
enum class SelectedEdgeFrameStatus {
    resolved,
    invalid_context,
    source_missing,
    source_ambiguous,
    source_unsupported,
    native_geometry_unavailable,
    invalid_geometry,
    invalid_orientation_intent,
    orientation_review_required,
};

// Pure S1 caller-carried orientation intent; NOT a persisted Datum
// and NOT semantic Edge/Point identity. S2 must separately version it.
struct SelectedEdgeFrameIntent final {
    kernel::Point3 signed_normal_reference_world{};
    kernel::Point3 u_seed_world{};
    bool reverse{false};
    friend bool operator==(
        const SelectedEdgeFrameIntent&,
        const SelectedEdgeFrameIntent&) = default;
};

struct SelectedEdgeFrameResult final {
    SelectedEdgeFrameStatus status{
        SelectedEdgeFrameStatus::invalid_context};
    std::optional<kernel::Frame3> frame;
    // Set on successful INITIAL author only, never from replay.
    std::optional<SelectedEdgeFrameIntent> initialized_intent;
    std::optional<kernel::ExactEdge3dStatus> native_error;
    [[nodiscard]] bool resolved() const noexcept;
};

[[nodiscard]] SelectedEdgeFrameResult
initializeSelectedEdgeEndFrame(
    const PartDocument& owner,
    const SelectedMaterialEdgeEnd& source,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider,
    bool reverse = false);

[[nodiscard]] SelectedEdgeFrameResult
initializeSelectedEdgeMidFrame(
    const PartDocument& owner,
    const SelectedMaterialEdgeMid& source,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider,
    bool reverse = false);

[[nodiscard]] SelectedEdgeFrameResult
resolveSelectedEdgeEndFrame(
    const PartDocument& owner,
    const SelectedMaterialEdgeEnd& source,
    const SelectedEdgeFrameIntent& intent,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider);

[[nodiscard]] SelectedEdgeFrameResult
resolveSelectedEdgeMidFrame(
    const PartDocument& owner,
    const SelectedMaterialEdgeMid& source,
    const SelectedEdgeFrameIntent& intent,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider);
```

Implement all four in existing `src/part/feature_evaluation.cpp` with private reusable vector arithmetic/orthonormality routines. Do not add CMake targets or alter `src/kernel/**` / `src/kernel_occt/**`. For replay the intent is **input only**, not mutated; for init, the returned `initialized_intent` is normalized and captures **pre-Reverse** base N and U. `reverse` flips V,N on the output frame only. No new source identity, hidden state, cached Frame3, native handle or ambiguous-point promotion. Every non-resolved result has **no frame or initialized intent**, optional `native_error` only if Q0 genuinely returned one.

## 3. Proposed exact numeric/geometry policy — needs Owner D2 acceptance

All vector math in world Cartesian coordinates and `double`; dimensionless orientation tolerances independent of model millimeters. The constants here are **proposals for acceptance**, not new global Kernel policies:

1. **Source first:** call the shipped Q1/Q2 semantic resolver, fail typed when not `resolved`. Validate finite origin and current tangent components; require tangent squared length within **`1.0e-6`** of 1 (aligned with shipped Q2 validation) and normalize. Any nonfinite/degenerate/wrong-unit observation returns `invalid_geometry` with no frame; don't infer a signed direction from Q0 terminal order or OCCT arc parameter sign.
2. **Initial signed N:** take normalized unoriented representative `t`; choose axis with largest `abs(t[i])`, with **exact** equality broken X then Y then Z. Change sign so chosen component of N is strictly positive. `t` and `-t` produce **exactly the same N within floating-point comparison tolerance**. Global sign rule is applied **only for initialization**, never as an edit-time orientation rewrite.
3. **Initial U seed:** candidates world +X, +Y, +Z in that order. For candidate axis `a`, `u0=a-dot(a,N)*N`; accept the **first** candidate with `|u0|² >= 1/16` (projected length >= 0.25). Normalize U; `V=N×U`. A parallel first axis triggers deterministic fallback, **not** failure. Store `signed_normal_reference_world=N` and `u_seed_world=U` as independently caller-carried, pre-Reverse orientation intent.
4. **Intent input validity on replay:** both reference vectors finite and unit to **`abs(|v|²-1)<=1.0e-10`**, and mutually orthogonal to **`abs(dot(Nref,Uref))<=1.0e-10`**; else `invalid_orientation_intent`. Do not normalize malformed/stale intent into an arbitrary accepted policy.
5. **Small-edit continuity:** independently certified current tangent line `±t` must satisfy `abs(dot(t,Nref)) >= sqrt(3)/2` (**at most 30° acute line-direction difference from authored reference**). At `abs(dot)<sqrt(3)/2`, return **`orientation_review_required`** atomically. When admitted, select the signed N from the `±t` member with `dot(N,Nref)>0`. Do **not** apply the dominant-axis sign rule again; crossing X/Y/Z component ties must not flip the frame. This reference-angle guard includes the ambiguous vicinity of 90° and intentionally blocks significant orientation changes rather than choosing an unproved historical sign.
6. **Remembered U projection:** `u0=Uref-dot(Uref,N)*N`; if `|u0|²<1/16`, return **`orientation_review_required`** rather than silently substituting another world axis on existing authored orientation. Otherwise `U=normalize(u0)`, `V=N×U`. A world-axis fallback is legal **only at initialization**; existing Sketch coordinates must not silently jump on replay.
7. **Right-handed validation:** all frame vectors and O finite, each axis squared length within `1.0e-10` of 1, every pairwise `abs(dot)<=1.0e-10`, and `dot(U×V,N)>=1-1.0e-10`, plus `Frame3::valid()`. Failure returns `invalid_geometry`, no partial frame. No new `kernel::Frame3` global numerical policy.
8. **Reverse:** for either initializer or resolver, first compute pre-Reverse `F=(O,U,V,N)`; if `intent.reverse`, output `(O,U,-V,-N)`. The **pre-Reverse** intent references are unchanged. Re-running with `reverse=false` restores F; two toggles are an involution. Don't confuse this with shipped `DatumPlaneDraft::reverse()` which negates legacy Offset distance; that application API is **out of scope**.
9. **Information-theoretic fence:** un-oriented current tangent alone cannot reconstruct the *path* of a large >90° rotation or distinguish a new nearly antiparallel analytic representative from a physical reversal. This S1 policy only guarantees **small-change (<=30° line-angle) continuity relative to authored reference**; it cannot promise arbitrary large-rotation continuity or native parameter-order intent. The eventual S2/S3 orientation reset/review UX requires its own contract. No hidden history-dependent update in S1.

Boundary values (exact 30°, exact `1/16`, exact ties, reversed input representation and `double` rounding close to comparisons) must have explicit deterministic tests. No hard-coded screen/viewport camera, last-good-frame or native-order fallback.

## 4. Typed mapping, atomic status, S1 not persistence

`invalid_context` means caller/source/owner/document/stage/revision invalid as reported by shipped Q1/Q2; `source_missing/ambiguous/unsupported` maps their independently typed Edge or certified END statuses; `native_geometry_unavailable` maps genuine Q0 provider/body/query failures, with optional exact `kernel::ExactEdge3dStatus` propagated; `invalid_geometry` covers finite/unit/frame checks; `invalid_orientation_intent` rejects malformed references; `orientation_review_required` means changed line exceeds policy or remembered U seed is no longer usable. Never publish a Frame on any failure.

All S1 routines must be observational, deterministic for equivalent source+current geometry+intent, and **idempotent** across repeated evaluation and cold native provider. They do not mutate `PartDocument`, `PartEvaluation`, `DatumPlaneDraft`, `ACTIVE.yaml` or a file. Caller may carry orientation intent in memory; S1 makes **no claim of Save/Reopen orientation persistence or authored Datum**. No new global Point, signed tangent identity, signed authoring `FeaturePointAddress` or forced END index.

## 5. Finite S1 acceptance matrix — executable registered witnesses, all UNRUN

| Gate | Actual mandatory witness |
| --- | --- |
| S1-F01 | Native Part 12 material Extrude Lines: author Q2 MID and selected certified Q1 END frames, initial N/U/V unit right-handed, deterministic per semantic Edge context, real shared 3-Edge corner, no global Point promotion |
| S1-F02 | Real partial Revolve Line/open circular Arc with positive/negative signed sweep and reversed endpoint order: exact current origin/tangent, frame independent of OCCT parameter/traversal sign; native full circle/unsupported/degenerate failure |
| S1-F03 | Cold independent OCCT provider and recreated Part: same authored semantic END/MID + **unchanged supplied orientation intent** give equivalent signed Frame3; no provider-instance identity or stale runtime token |
| S1-F04 | Real Extrude height edit: source identity preserved, O moves when appropriate, initial intent replays without N/U 90° jumps for <=30° changes, non-source FeatureId decoy and foreign owner/revision/stage typed failure |
| S1-F05 | Synthetic controlled signed `±t` normals near tie X=Y, both native representative signs, U fallback when +X parallel; exact first-seed `|u0|²=1/16` and orthonormality, no hidden uninitialized V |
| S1-F06 | Intent validation: NaN/Inf/zero/nonunit or nonorthogonal intent; mismatched current unoriented line at 29°, 30°, 31°, 89°, 90°, 91°; replay review status vs signed Frame, no cached last-good result |
| S1-F07 | A remembered U seed becoming near-parallel at the chosen threshold: typed `orientation_review_required`; first-time seed fallback still succeeds; toggling Reverse twice preserves O/U/source/intent and restores Frame |
| S1-F08 | Existing registered `pm05b2.strict_edge_resolver` and native `pm05c2b.part_edge_topology`, `pm02p.e_kernel_lifecycle`: Q1 END 24, Q2 MID 12, Fillet/Chamfer/half-cylinder ambiguity all unchanged; real semantic negatives fail atomically |
| S1-F09 | Windows actual native FOCUSED and Desktop/Part FAST; Ready FULL Core + native + Desktop; canonical `docs/internal/PART_DOCUMENTS.md` to generated Browser DOC-01 parity; bounded zero-P4-UI/Datum-schema regression |

Specific tests must execute in **already registered CTest targets**, not merely be written or claimed; synthetic angle fixtures may use a test-only exact-geometry provider and must also include real OCCT fixtures for Q1/Q2 semantic admission. No new CMake/CI test registration allowed.

## 6. Exact later implementation allowlist — EIGHT paths only

After complete-text Owner D2 acceptance, independently Owner-approved **contract PR squash**, independently accepted and squash-merged **one-line `work/ACTIVE.yaml` Q2→P4-S1 activation**, a fresh production branch may modify **only**:

1. `src/part/include/simplesolid2/part/feature_evaluation.hpp` — additive S1 types/status and four APIs, no persisted fields
2. `src/part/feature_evaluation.cpp` — existing Q1/Q2 certified read wrapper + private signed Frame math
3. `tests/pm05b2_strict_edge_resolver_test.cpp` — typed default/negative and pure semantic regression
4. `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` — real Part box, cold, edits, invalid owner, seed and Reverse witnesses
5. `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` — exact bounded analytic Line/open Arc sweeps and frame native regression
6. `docs/internal/PART_DOCUMENTS.md` — as-built signed Frame3 ownership and S1 non-persistence fence
7. `docs/browser/index.html` — regenerate **only via `./ss2.ps1 docs`**, verify deterministic parity; no manual generated-content edits
8. `work/RG-02B1_D2_P4_S1_SIGNED_EDGE_FRAME_ACCEPTANCE_EVIDENCE.md` — actual S1-F01…F09 proof/RED history and exact SHA split

**Outside allowlist:** `work/ACTIVE.yaml` in code PR, all `src/kernel/**` and `src/kernel_occt/**`, `src/part/datum*`, `src/part/part_document*`, native schema/serializer, `src/application/**`, `src/ui/**`, `src/viewer/**`, `src/sketch/**`, CMake/CI/roadmap/ADR, PL+EN product docs or new Datum kind/selection UI. A discovered dependency requiring any other path is a **HARD STOP** for a fresh bounded Owner decision, not implicit scope growth.

## 7. Windows, DOC-01, Owner acceptance and final gates

1. **Now:** request Owner acceptance/rejection of **this entire prospective executable S1 Work Contract** — all status fields, numeric thresholds, replay rules, eight paths, nine witness groups and out-of-scope fences. This is separate from earlier agreement to the S1→S2→S3 plan.
2. After scope acceptance: validate this contract proposal PR on its exact final text via Windows governance, request **separate PR-specific Owner permission to squash-merge** it, verify it is on `main`. Prepare **another** dedicated `work/ACTIVE.yaml`-only activation PR, request separate Owner approval of exact Q2→S1 pointer and then independent exact-HEAD squash authorization; verify merged before implementation.
3. On an implementation PR restricted to §6: run actual registered kernel-native **FOCUSED**, Windows **FAST**, Ready Windows **FULL** (Core, kernel-native, Desktop), update internal canonical Part Markdown and regenerate Browser with `./ss2.ps1 docs`, and verify DOC-01 diff parity. Record actual source SHA, success or RED failures, and repeat until green. A work-only closure cannot stand in for native proof.
4. Freeze tested source/test/docs HEAD and record S1-F01…F09 in the evidence ledger. If ledger-only update follows FULL, verify exact two-SHA compare changes **only that ledger**, and run exact final-head Windows governance. Require **Owner technical FINAL PASS** of S1 and an **independently explicit exact-head PR-specific production squash merge permission**. S1 code merge neither activates S2 nor authorizes Datum Plane UI/persistence/Reverse product behavior.

## Documentation impact

Internal docs: required **for future S1 implementation only**
User/Product docs: not required **for S1**
Reason: S1 adds **headless read-only Part frame-policy APIs**, not a user-visible Datum constructor, parser, command or new native persistence; as-built internal Part documentation and deterministic generated Browser must be updated and tested. The later S2/S3 work will independently assess native-schema docs and paired PL/EN Product UX docs before feature release.

## 8. Requested one Owner D2 decision

**Accept/reject this entire S1 signed Edge-local Frame prospective executable contract**, with the exact constants and typed boundaries in §3–4, the registered actual tests §5 and EIGHT future implementation paths §6. It is only a scope decision. **No S1 implementation, merge of this PR, ACTIVE activation, S2 persisted Datum, or S3 UI is authorized without its own separate approved governance gates.**
