# RG-02B1/D2-P4 — next executable Work Contract preparation: signed frame + Reverse + versioned Datum

**STATUS: TECHNICAL CONTRACT-PREPARATION DRAFT — NOT ACCEPTED, NOT MERGED, NOT ACTIVE, NOT AN EXECUTABLE WORK CONTRACT.**
**Prepared:** 2026-10-13 from live `main` `00b6020e1cdc5fbf008eab444e6c311ca51bbb7d` (Q2 owner-approved production + historical closeout are MERGED). **Prerequisite design:** [Owner-accepted two-stage orientation-policy PR #365](https://github.com/MechanicalCave/SimpleSolid2/pull/365), exact current HEAD `c542ad13febe138cec15bf0352c469369755572b`, **OPEN/UNMERGED**, last Windows governance #2917 SUCCESS. P4 acceptance records architecture policy, **NOT** merge approval or full code authority. `work/ACTIVE.yaml` still points at **completed Q2**; Foundation 1.0/roadmap v1.33 untouched.

## 1. What is already settled versus missing

Already implemented on `main`: Q1's Part-local **selected material Edge+independently certified END** and Q2's **strict material Edge-local MID** author/read APIs, tied to owning DocumentId, live revision, exact producer Body stage and current native bounded Q0 Line/open Arc 3D observations. Both sources return **unoriented** tangents. Neither may be reinterpreted as a persisted new Datum.

Already Owner-endorsed design **direction**, requiring precise implementation policy before product work:
- Choose a deterministic *initial* right-handed signed `Frame3(O,U,V,N)` for certified END/MID even when ±tangent sign is free; show valid oriented preview, not an orientation error.
- For every subsequent regeneration, use independently **authored orientation intent** (signed normal reference, U seed and reverse), not fresh global-axis guesses, to preserve small-edit Sketch-axis continuity.
- `Reverse` means `(O,U,V,N)→(O,U,-V,-N)` with unchanged source, O and U; two toggles restore the same default.
- New authored Datum kind must use explicit **new schema-version gating**, preserve old Offset/v16 meaning and reject unknown variants; no v16 writer silently emits new kind.

These are direction-level; the following technical questions **are not yet decided**. Do not treat candidate rules below as approved limits or C++/JSON fields.

## 2. Exact technical D2 choices to freeze (future Owner full-text Work Contract)

| Decision | Concrete design candidate to review | Required STOP oracle |
| --- | --- | --- |
| **T1 — first sign** | Given unit unoriented exact tangent, choose signed `N` with positive largest-absolute XYZ component; exact tie X→Y→Z. Equivalent native endpoint order cannot change it. | Nonfinite/zero tangent fails; arbitrary OCCT parameter sign or camera may not determine identity or orientation |
| **T2 — first U** | Try ordered canonical axes X→Y→Z; project each onto plane orthogonal to N, choose first with enough nonzero projected magnitude; normalize and set V=N×U. | Specify numeric parallel threshold and ordered fallback. A parallel *first* seed is not an error |
| **T3 — authored continuity** | Store normalized initial signed `normal_reference` and normalized `u_reference` **as orientation intent**, plus `reverse`. On valid live source reevaluation choose ±current tangent maximizing dot with the authored signed reference, then reproject authored U. | No recomputation of initial sign/U on every edit, no last-good frame fallback, no rebinding source from orientation vectors |
| **T4 — large-edit sign boundary** | When the new tangent line approaches 90° from stored signed reference, prefer a distinct typed `orientation_review_required` result instead of silently switching normal sign; after an explicit authorized orientation reset, save new orientation intent. | Specify dot-threshold, equality/tie behavior, when to block downstream Sketch and how user approves intentional changes |
| **T5 — U seed degeneration** | If authored U projects to approximately zero after a large edit, block with typed orientation review required (or a separately approved explicit fallback) rather than silently rotating Sketch UV. Initial canonical fallback remains allowed. | Specify unit and orthogonality tolerances; late provider result or stale stage never offers Finish |
| **T6 — through-line off-line Point** | Strict straight source Edge + independent one-to-one semantic Point, with **proven stable signed terminal/anchor** before computing line+Point Frame. Q2 MID alone is not an anchor. | No native first Vertex, current XYZ as source identity, unverified global Ambiguous Point, near-collinear U×d |
| **T7 — native source schema** | A tagged new `EdgePointDatumPlane` kind, `DatumId`, source strict Edge and exactly one END/MID/independent Point role, orientation-intent version, `reverse`, `visible` in an explicit *new* Part schema version (v17 only if separately accepted). | Prove all tags/defaults/validation, v16 Offset read/roundtrip, unknown-kind/version rejection, no silent downgrade |
| **T8 — lifecycles** | Atomic create/edit, preview/Finish revalidation, Undo/Redo, Save/Close/Reopen with cold OCCT provider, typed dependency failures; stable DatumId and authored source across edits. | No state mutation on failure; no cached Frame3 ghost, no silent reparent/FeatureId decoy |
| **T9 — UI parity** | *Same* existing Datum Plane tool; one additional Edge + Point constructor; either selection order, selected Edge B owns clicked B END at shared corner, MID uses END visual marker, default preview/Reverse; existing CAD keyboard/focus/Command Line finish/cancel. | No independent parser, extra overlapping Point chooser, new END/MID mode or Offset regression |

**Specific numeric values and persistence encoding are deliberately NOT invented here.** T1–T9 must be converted to exact typed C++ API, explicit native version fields, tolerances, test fixture oracles and failing statuses in one later complete Owner D2 executable contract, or deliberately split into separately complete, independently accepted non-deceptive bounded stages that cannot produce a persistently inconsistent Datum. A design suggestion is not a PASS.

## 3. Audited high-impact source graph; exact mutation allowlist still UNFROZEN

Existing audited integration points:
- `src/part/include/simplesolid2/part/datum.hpp`, `src/part/datum.cpp`: authored Offset `DatumId`, `PlaneReference`; future tagged EdgePoint variant.
- `src/part/include/simplesolid2/part/part_document.hpp`, `src/part/part_document.cpp`: `PartDocumentState::datum_planes` currently `std::vector<OffsetDatumPlane>`. A tagged new variant changes structural validation, references and history; do **not** change blindly.
- `src/part/include/simplesolid2/part/part_document_store.hpp`, `src/part/part_document_store.cpp`: current Part native schema **16**; serializer writes `datum_planes: [{kind: offset_plane}]`, loader parses only Offset. Exact future new-version/migration contract is mandatory.
- `src/part/include/simplesolid2/part/datum_evaluation.hpp`, `src/part/datum_evaluation.cpp`: current only Offset derived frame and typed failure; existing `Frame3::valid()` does **not** prove orthonormality or edit-stable basis.
- `src/application/include/simplesolid2/application/datum_plane_draft.hpp`, `src/application/datum_plane_draft.cpp`: existing create/edit/Reverse/Finish transaction, draft-generation and CAD-input lifecycle.
- `src/ui/cad_workbench.cpp` and associated UI/Viewer topology-pick files: existing selection/preview and overlapping candidates; exact touch points still require a **read-only per-file audit** before executable allowlist.
- Existing registered tests `tests/pm03a_datum_schema_v10_test.cpp`, `tests/pm03b_datum_evaluation_test.cpp`, `tests/pm03c2_cad_workbench_datum_plane_test.cpp`, `tests/pm03e_datum_backed_sketch_test.cpp`, `tests/part_document_store_test.cpp`, `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp`: candidate extension witnesses, **not** permission to edit or claim new PASS.

**One nonnegotiable next action before authorizing code:** compile a **complete numbered allowlist** of every changed source, tests, canonical internal/Product PL+EN docs and generated Browser, with no CMake/CI expansion by implication. Confirm whether native tagged `datum_planes` necessitates additional transaction, Sketch or application paths. Any extra needed path after Owner acceptance requires an amended D2 decision, not opportunistic edits.

## 4. Finite future test and documentation gate

Tests must actually cover P4-F01…F09 from the accepted design proposal: exact selected END at three-Edge corner irrespective of hover, straight MID, signed open Arc trims, strict off-line Point, Reverse involution, numeric/unit/orthogonality, small-edit frame continuity and large-edit review status, cold native new-schema serialization, legacy v16 Offset, Sketch stability and full CAD input/Preview/Finish/Cancel parity. Real OCCT Body/stage and independently reconstructed provider required; synthetic status-only examples do not replace native evidence. Q1/Q2 green 199/199 is **not** evidence of these future tests.

Future implementation and explicit path allowlist require actual Windows kernel/desktop **FOCUSED**, FAST, Ready FULL **Core+Native+Desktop**, deterministic DOC-01 generator parity, generated Browser and paired PL+EN product docs as appropriate; record actual RED history, final code/test/docs SHA, evidence-only compare, final governance Windows and two **separate** Owner technical FINAL PASS / exact-HEAD production squash decisions.

## 5. Owner decision/governance ordering

1. Finish the separate Owner-endorsed D2 design review [PR #365](https://github.com/MechanicalCave/SimpleSolid2/pull/365) by **separately authorizing squash merge** of its exact governance-validated HEAD, then verify actual `main`. Design acceptance already given is **not** that merge authorization.
2. Owner reviews this **technical preparation** and makes explicit decisions on T1–T9, including concrete tolerances, future schema fields/migration, off-line anchor and the complete affected-file allowlist. This document **is not executable**; it must become one or more complete Work Contracts with exact scope after decisions, separately Owner accepted and merged.
3. Prepare **separate `work/ACTIVE.yaml`-only** next-contract activation, obtain independent Owner activation scope and exact-head merge approval; verify merged before a line of product code.
4. Only then implement, test and seek another technical FINAL PASS + independent production squash-merge. No UI, Datum, Frame3, Reverse, schema v17 or Q3 code is authorized by this document.

## Documentation impact

Internal docs: not required for this work-only planning draft
User/Product docs: not required for this work-only planning draft
Reason: no as-built behavior changes. Future executable Datum/UI/schema work explicitly requires internal as-built, paired Product PL/EN and deterministically generated Browser parity.
