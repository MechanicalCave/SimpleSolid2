# RG-02B1/D2-Q2 — strict material Edge-local MID: prospective executable Work Contract

**STATUS: OWNER ACCEPTED COMPLETE PROSPECTIVE Q2-MID D2 WORK CONTRACT (2026-10-13) — GOVERNANCE PR #361 STILL UNMERGED / Q2 NOT ACTIVE / NO IMPLEMENTATION AUTHORITY.** Exactly reviewed PR #361 HEAD `2b2a12055d21671f6fe7feb69233ee1d3164e199`; this subsequent annotation records only the Owner decision and leaves all accepted types, algorithms, allowlist, test gates, DOC-01 and exclusions unchanged.
**Prepared:** 2026-10-13. **Verified source baseline:** `main` `ec741e47f57850454880689c8846f0a427d78a92` after Owner-authorized Q1 production [#359](https://github.com/MechanicalCave/SimpleSolid2/pull/359) squash `53bcf43ed31cbd0a99120da354a4480a476cf030` and separate Q1 status closeout [#360](https://github.com/MechanicalCave/SimpleSolid2/pull/360) squash `ec741e47f57850454880689c8846f0a427d78a92`. Q1 real native/typed END proof, FAST and exact-source FULL are **completed**, not to be reimplemented.
**Present authority:** `work/ACTIVE.yaml.active_work` is still `work/RG-02B1_D2_Q1_SELECTED_EDGE_END_WORK_CONTRACT_DRAFT.md` and program roadmap **v1.33**. Its pointer now records **COMPLETED Q1**, not authority to implement Q2. Foundation **1.0**, existing accepted ADRs and current native Part schema v16/modeling semantics v2 remain unchanged.

## 0. Owner decision sought — one bounded next slice

**Recommendation:** choose **one small, headless Q2 milestone**: a Part-local **strict material Edge + semantic MID role** and a read-only, exactly current-Body-stage **MID position + unoriented tangent** resolver. Complete it before introducing any authored Datum constructor, signed frame, Reverse, persisted MID role, Viewer MID marker, UI gesture or Command Line meaning.

This is a **different durable semantic reference value** from Q1's `SelectedMaterialEdgeEnd`: MID is **not** an END, a synthetic `FeaturePointAddress`, an arbitrary edge parameter, an OCCT midpoint-vertex or an index into a tessellated rendering polyline. A uniquely resolved strict bounded material Edge suffices as the source identity of its **one half-arclength MID**, provided Q0 exact native geometry is available at the same successful Part stage. The result is Part-local; qualifying `DocumentId` belongs at the live read boundary.

**Request for a single future decision:** accept/reject **the complete types/API, finite families, exact ten-file allowlist, native test matrix, fail-closed statuses and documentation/CI obligations** below as the **prospective executable Q2 Work Contract**. This does not request approval for merge or activation; those are separate gates, after the contract review.

## 1. Audited shipped source and unimplemented gap

| Shipped baseline | Verified contract | Small missing Q2 deliverable |
| --- | --- | --- |
| `src/part/include/simplesolid2/part/semantic_topology_reference.hpp` | `MaterialEdgeReference` records producer-semantic Curve+branch and exact `BodyStageRef`; Q1 `SelectedMaterialEdgeEnd` adds independently certified semantic `FeaturePointAddress`. | Add an **MID-specific value** storing only a strict `MaterialEdgeReference`; no endpoint Point/provenance dependency. |
| `src/part/include/simplesolid2/part/feature_evaluation.hpp` and `src/part/feature_evaluation.cpp` | Q1 already checks owning `PartDocument.documentId()` against runtime `PartEvaluation.source_document_id`, live revision, exact successful Feature stage, its `result_solid` and complete `result_topology`. Strict `authorMaterialEdgeReference` and `resolveMaterialEdgeReference` reject missing/ambiguous/unsupported edges and representation artifacts. | Reuse/refactor **existing bounded admission rules** for strict Edge-local MID authoring and replay. Do not duplicate or relax ownership, stage or source checks. |
| `src/kernel/include/simplesolid2/kernel/exact_bounded_edge_geometry.hpp` + `src/kernel_occt/solid_modeling_kernel.cpp` | Shipped Q0 `IExactBoundedEdgeGeometryQuery`: analytic bounded Line/open circular Arc, exact `bounded_length_mm`, `midpoint_at_half_bounded_length_world`, `midpoint_tangent_line`, typed failures, exact `ScopedExactEdge` Body lease. Q1 adds only transient paired terminal Vertex tokens. | **No Kernel source/API edit planned.** Consume precisely these Q0 MID fields after current stage+Edge certification. A different OCCT provider instance is **not** a foreign Body; compare exact Body-object identity. |
| `src/part/include/simplesolid2/part/datum.hpp` and `src/part/datum_evaluation.cpp` | Current authored Datum is `OffsetDatumPlane` with deterministic derived `kernel::Frame3`. | **Do not touch this model in Q2.** A signed Datum frame and new persisted Datum constructor require a separate D2 contract; `Frame3::valid()` alone does not prove deterministic U/V/N orientation across edits. |
| `work/RG-02B1_DATUM_EDGE_POINT_WORK_CONTRACT_DRAFT.md`, `work/RG-02B1_D2_EDGE_POINT_SEMANTIC_DESIGN_PROPOSAL.md`, `work/RG-02B1_D2_P4_FRAME_REVERSE_PERSISTENCE_REVIEW.md` | Owner-accepted *design direction*: one future Datum Plane tool, Edge+Point either order, END/MID/off-line role auto-classification, MID default oriented preview and Reverse, existing Command Line/focus parity; no arbitrary screen/native identity, no silent schema mutation. Some old headers refer to historical unmerged baselines; verify live PR state rather than treating those headers as current implementation state. | Deliver **only** the headless MID semantic prerequisite. This contract does not claim Q2 ships the intended Datum product UX. |

**Technical consequence:** Q0 knows where the MID is **today**, and Q1 can durably identify **END** for independently resolved semantic Points. Neither presently defines a distinct, authorable **Part-local MID source value** or a strict source-bound Part read API for its current 3D MID observation. This is the one gap addressed here.

## 2. Exact semantic value and proposed public boundary (D2)

### 2.1 MID identity, not MID geometry

Proposed additive type in `semantic_topology_reference.hpp`:

```cpp
struct SelectedMaterialEdgeMid final {
    MaterialEdgeReference source_edge;

    [[nodiscard]] bool valid() const noexcept {
        return source_edge.valid();
    }
    friend bool operator==(
        const SelectedMaterialEdgeMid&,
        const SelectedMaterialEdgeMid&) = default;
};
```

The type itself is the **role discriminator**: `SelectedMaterialEdgeMid{edge}` and `SelectedMaterialEdgeEnd{edge, certified_point}` are never interchangeable. MID is **half actual bounded material Edge arc length**, NOT middle of a B-Rep parameter interval, display polyline, 2D projection, nearest clicked position, tessellation, current native Vertex or an independently persisted Point. Do not add a generic `MID` to global `FeaturePointAddress`.

Two MID values compare equal iff their strict semantic material Edge references compare equal **within the same owning Document**. Cross-document equality must never be used as authority, even where numerically colliding Feature IDs and identical geometry occur. `DocumentId` is externally checked on read; do not persist it inside this Part-local value.

### 2.2 Exact typed author/replay APIs

Appenditive proposed declarations in `feature_evaluation.hpp`:

```cpp
enum class SelectedEdgeMidStatus {
    resolved, invalid_context,
    edge_missing, edge_ambiguous, edge_unsupported,
    native_geometry_unavailable,
};

struct SelectedEdgeMidAuthoringResult {
    SelectedEdgeMidStatus status{SelectedEdgeMidStatus::invalid_context};
    std::optional<SelectedMaterialEdgeMid> reference;
    [[nodiscard]] bool ok() const noexcept;
};

struct SelectedEdgeMidResolution {
    SelectedEdgeMidStatus status{SelectedEdgeMidStatus::invalid_context};
    std::optional<kernel::RuntimeEdgeToken> current_edge;
    std::optional<kernel::Point3> current_position_world;
    std::optional<kernel::UnorientedUnitTangent3> current_tangent_line;
    std::optional<kernel::ExactEdge3dStatus> native_error;
    [[nodiscard]] bool resolved() const noexcept;
};

[[nodiscard]] SelectedEdgeMidAuthoringResult
authorSelectedMaterialEdgeMid(
    const PartDocument& owning_document,
    const PartEvaluation& evaluation,
    const BodyStageRef& source_stage,
    kernel::RuntimeEdgeToken picked_material_edge,
    kernel::IExactBoundedEdgeGeometryQuery& provider);

[[nodiscard]] SelectedEdgeMidResolution
resolveSelectedMaterialEdgeMid(
    const PartDocument& owning_document,
    const SelectedMaterialEdgeMid& authored,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider);
```

Exact spellings above are **proposed** and become binding only on complete Owner D2 acceptance. No generic `MID` branch on Q1's `SelectedEdgeEndStatus`, no new broader Point/Vertex identity, and no signed normal or Datum frame in either result.

### 2.3 Mandatory admission order and failure behavior

1. **Owner and stage first:** `PartEvaluation.source_document_id` equals `owning_document.documentId()`; evaluation revision equals live `owning_document.revision()`; source producer exists; `BodyStageRef` matches exactly one successful `FeatureEvaluation` with own `result_solid` and complete `result_topology`. Reject foreign/stale/missing/invalid/synthetic unqualified evaluation before calling Q0. No final Body or history-prefix substitution. Deleted original producer is `edge_missing` once valid owner/current context is established.
2. **Strict material Edge:** author through the shipped exact-stage `authorMaterialEdgeReference`; replay through `resolveMaterialEdgeReference`. It must be uniquely Resolved and current material, with the same authored bounded branch. Representation seams/partition edges, missing/ambiguous/unsupported branch, split sources or producer identity decoys are rejected with typed Edge statuses. Authoring must confirm the chosen runtime token is the one resolved by its semantic source. Never select based on XYZ resemblance.
3. **Q0 same Body+Edge:** `bindExactEdgeToBody(stage.result_solid, resolved_current_edge)`; require the binder result to refer to **that identical Body object** and selected current Edge, then `queryExactBoundedEdge3d` on the same Body. An unrelated valid Body is not allowed; a new compatible OCCT provider instance alone is not a mismatch. Q0 `status != ok` maps to `native_geometry_unavailable` with underlying exact status, no synthesized midpoint.
4. **Atomic observation:** only if every gate above passes may return current material Edge, `midpoint_at_half_bounded_length_world`, `midpoint_tangent_line` and `resolved`. Validate current Q0 tuple's finite coordinates, positive finite length and nonzero finite unit tangent according to existing numeric policy without introducing a new public tolerance. Unsupported closed circle, seam, degenerate, unsupported nonanalytic curve, trim or native failure return **no partial Edge/position/tangent observation**.
5. **No source repair by runtime observation:** no persisted world MID, current native token, B-Rep/OCCT start parameter, arc endpoint order, runtime Vertex candidate, nearest position, coordinate equality or viewer pick rank in `SelectedMaterialEdgeMid`. The MID remains a role of the selected strict semantic Edge, not of its two ENDs.

**Initial exactly supported families:** uniquely Resolved, strictly bounded material **Line** and **genuinely open circular Arc**, with Q0 exact successful native MID query on the matching current Body. Unlike Q1 END, MID **does not require an independently Resolved semantic terminal Point**. This does **not** upgrade globally Ambiguous/Unsupported Points; it only avoids asking for a terminal Point when none is authored.

## 3. Finite required Q2 tests — actual executable witnesses, not planned PASS

| Gate | Registered witness and acceptance oracle |
| --- | --- |
| **Q2-M01 12/12 rectangular Extrude** | Real native `pm05c2b.part_edge_topology`: all twelve independently strict material Line Edge sources each author one MID, replay to its current exact half-length center and unit unoriented tangent, no reused MID identity across unrelated Edge sources. Q1's 24 ENDs remain distinct values and untouched. |
| **Q2-M02 3D Arc half-arclength** | Real provider `pm02p.e_kernel_lifecycle`: open trimmed arcs for both sweep signs/trim orientations produce exact Q0 midpoint-at-half-bounded-length, not 2D projection or display chord midpoint; reject full/closed circle, seam, degenerate and unsupported kind. |
| **Q2-M03 cold provider reconstruction** | Real Part cold re-evaluation in a fresh OCCT provider: all canonical MID source values replay to their correct semantic Edges without relying on previous native token, ordered endpoints or XYZ from former run; Q0 query binds current Body only. |
| **Q2-M04 upstream edits** | Existing Extrude height-change fixture: all unaffected strict Edge sources preserve semantic MID identity, while their current geometry moves/changes when appropriate. A geometrically equal replacement with different producer FeatureId stays `edge_missing`, not rebound. |
| **Q2-M05 typed negative strict source** | Existing `pm05b2.strict_edge_resolver` and native Part fixture: missing/split/ambiguous or representation-only Edge, wrong history stage, deleted producer, foreign `DocumentId`, stale revision, no source `DocumentId`; typed non-Resolved with no partial results. |
| **Q2-M06 actual foreign Body and Q0 failure** | Test-only binder that supplies a **different** `RuntimeSolidHandle` with valid other-Body Edge must be rejected. Inject malformed Q0/unavailable native result; verify current observations remain empty. A second provider instance querying the **same** exact Body must not be falsely rejected. |
| **Q2-M07 no Point status promotion** | Reuse real Fillet/Chamfer and half-cylinder negatives: unchanged global Ambiguous/Unsupported semantics. MID on a strict Edge may be legal **only** if its own Edge+Q0 geometry is valid, regardless of terminal Point, but never becomes a globally resolved `FeaturePointAddress`. Unsupported Edge stays typed unavailable. |
| **Q2-M08 cross-tier regression** | Existing Q1 12 Edge/24 END/8 corners and Q0 Line/Arc tests remain PASS; `pm05b2.strict_edge_resolver`, native `pm05c2b.part_edge_topology` and `pm02p.e_kernel_lifecycle` actually execute. No UI/manual MID or Datum product PASS is claimed. |

Do not mark any row PASS until exact source/test HEAD Windows build+registered CTest actually runs it. New MID tests must augment existing registered targets; adding CMake/CI test registration is **out of scope**. Keep the prior #2894/#2895 wrong-provider-instance test lesson.

## 4. Exact implementation path allowlist after independent D2 + ACTIVE gates

**Only the following ten paths may be changed in a later fresh implementation PR**, after this exact contract is Owner-accepted, independently governance-MERGED and Q1→Q2 is independently Owner-accepted and squash-MERGED via a **one-line `work/ACTIVE.yaml`-only activation**:

1. `src/part/include/simplesolid2/part/semantic_topology_reference.hpp` — additive distinct MID source value
2. `src/part/include/simplesolid2/part/feature_evaluation.hpp` — additive typed MID author/replay API
3. `src/part/feature_evaluation.cpp` — exact-stage/owner strict resolver and Q0 MID observation coupling
4. `tests/pm05b2_strict_edge_resolver_test.cpp` — distinct source/status semantics with no global Point promotion
5. `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` — actual Part 12 MID, cold, upstream edit, foreign Body/source negatives
6. `tests/kernel_native/pm02p_e_kernel_lifecycle_test.cpp` — existing exact Line/open Arc half-length MID and negative guards
7. `docs/internal/PART_DOCUMENTS.md` — as-built MID Part source and ownership boundary
8. `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` — as-built Q0 MID geometry/reuse limits
9. `docs/browser/index.html` — regenerate **only** with `./ss2.ps1 docs` from canonical Markdown; never hand-design Browser content
10. `work/RG-02B1_D2_Q2_STRICT_MATERIAL_EDGE_MID_ACCEPTANCE_EVIDENCE.md` — actual Q2-M01…08 source/head/results, including RED and final split-SHA evidence

**Not allowed without a NEW complete Owner D2 decision:** Q0 `src/kernel/**` API modification or `src/kernel_occt/**` implementation modification; `Datum` storage/evaluation; `PartDocument` model/transaction; native v17/schema, serializer, modeling semantics version; `src/application/**`, `src/ui/**`, `src/viewer/**`, `src/sketch/**`; new pick mode/MID marker, signed Frame3, Reverse, edited datum preview, Command Line/focus/Undo/Redo; new CMake/CI test registration, product PL/EN docs, PM-06, Axis, G1/CI #302. No opportunistic common refactor outside exact ten paths. If source/test changes require any other file, **STOP** and present the dependency and a new bounded Owner decision.

## 5. CI, DOC-01 and acceptance gates

1. After Q2 text/scope acceptance: independently validate and explicitly Owner-authorize squash-merge **this contract's governance PR**. Only after that, prepare a distinct `work/ACTIVE.yaml` change to the exact accepted path, with Foundation v1.0, roadmap v1.33, accepted ADRs and `next` history preserved. Obtain **another** explicit Owner scope acceptance and PR-specific merge permission. Do not begin source work before activation is merged on `main`.
2. On the fresh code branch, execute actual native kernel FOCUSED for `pm05c2b.part_edge_topology` and `pm02p.e_kernel_lifecycle`, Desktop/semantic `pm05b2.strict_edge_resolver`, then Windows FAST, then exact-source Ready-for-Review Windows FULL including Core, native and Desktop. Do not count classifier/closure jobs or skipped test tiers as real proof.
3. Under DOC-01, update both named canonical internal Markdown docs and deterministically regenerate Browser. Run `./ss2.ps1 docs` and check generated file parity on the actual tested source/documentation HEAD. Any docs/evidence-only follow-up after code FULL needs a verified two-SHA path compare plus exact final-head Windows governance validation, not a fabricated second FULL PASS.
4. Publish finite Q2-M01…08 evidence, report genuine negatives/RED and zero false Resolved, require **technical Owner FINAL PASS** of the tested narrowed scope plus **separate exact-HEAD Owner approval to production squash-merge**. A green Windows run alone is not Owner acceptance.

## 6. Explicit next-stage fences

This **does not implement the user's final Datum Plane → Edge + Point tool**. It establishes a second *read-only semantic Part reference* for the future **MID** constructor, complementing Q1 END. No UX pick handling, MID visual marker, Edge/Point acquisition-order binding, off-line independent Point plane, canonical signed `Frame3`, right-handed Reverse, Datum edit/preview/transaction, Undo/Redo, Save/Reopen persistent MID source, v17 migration or downstream Sketch basis continuity are included.

The owner-accepted UX principle is preserved: **one existing Datum Plane tool** with Edge+Point chosen in either order, no separate END/MID mode or overlapping Point chooser, default oriented preview for valid MID and Reverse with existing CAD input parity. Those user-visible obligations are neither deleted nor met by this headless Q2 step. Future **separately accepted** work must solve them; do not silently weaken UX goals to match these backend limits.

**Hard STOP:** if MID needs a new native primitive, global Point reinterpretation, unapproved numeric tolerance, authored B-Rep parameter, signed tangent as identity, an implicit winner for ambiguous Edge, a new persisted Datum kind or any changed files beyond §4, stop and escalate with exact witness to Owner D2/D3. A Q2 MID value may survive semantic Edge re-evaluation in memory; it is **not** proof of Save/Reopen persistence of a new authored Datum source.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: only if this prospective contract is subsequently activated and executed, the strictly headless read-only Part MID reference/API and existing Q0 MID coupling will change internal as-built semantics. Then update the two canonical internal files specified in §4 and regenerate `docs/browser/index.html` using `./ss2.ps1 docs`. No user-visible Datum UI, CAD grammar, persistent Part schema or product feature changes under Q2, so PL/EN Product docs are not required. **This governance-only draft itself changes no production behavior and no as-built files.**

## 7. Owner complete-text Q2-MID D2 decision — ACCEPTED; merge and activation separately pending

**Owner accepted the entire prospective executable RG-02B1/D2-Q2-MID Work Contract on 2026-10-13** by replying “akceptuję - kontynuuj” to the request for full D2 scope acceptance, after reviewing PR #361 HEAD `2b2a12055d21671f6fe7feb69233ee1d3164e199`. The accepted scope is exactly: one strict MID semantic source type, append-only typed Part author/replay API, Q0 existing exact bounded Line/open Arc MID reuse, owning DocumentId/live revision/exact Body stage validation, ten exact future allowed implementation paths, Q2-M01…08 real-native proof matrix, CI/DOC-01 obligations and explicit no-Datum/UI/Frame3/Reverse/persistence exclusions. **This paragraph is only a governance status annotation and does not change the reviewed executable scope.**

**Still required — two independent future gates:** (1) verify this PR #361 on its *final annotated HEAD* with governance Windows; obtain explicit **PR-specific Owner permission to squash-merge** that exact HEAD, and verify it merged into `main`; (2) propose a distinct **`work/ACTIVE.yaml`-only** Q1→Q2 pointer activation, obtain separate Owner **activation scope acceptance**, exact-head Windows governance and **independent permission to merge that activation PR**, then verify actually merged. **Only after both gates may Q2 implementation begin.** Today's D2 contract acceptance is **not** permission to merge PR #361, change ACTIVE, mutate the ten future source/test/docs paths or declare Q2 tests PASS. Q1 remains completed; Q2+ product/Datum and further work remain unactivated.
