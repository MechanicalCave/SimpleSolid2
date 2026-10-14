# RG-02B1/P4-S2 — Authored Edge-local END/MID Datum v17 (prospective executable Work Contract)

**Status:** PROPOSED / NOT OWNER-ACCEPTED / NOT ACTIVE / NO PRODUCT-CODE AUTHORITY.
**Prepared:** 2026-10-14.
**Program:** Part Modeling v1.33; Foundation v1.0; accepted ADR-0009/0012/0013/0014/0016/0017 and merged P4 design direction.
**Completed prerequisite:** S1 signed Frame3 Owner technical FINAL PASS; PR #371 exact head `2ebc5571ae2d48935d8608ad9842b0dde6df34ee`, Windows FULL #2941 199/199 on source/docs `273a90af36e41fd4ce200bb713c429d2c708c9cc`, squash merged protected main `461daa812470c0615e4d67f7cec97bd7d6d138cf`. S1 work-only closeout PR #373 merged `bb52ba0bd1cf628a73108631234b8a86e84abef9`.
**Existing unaccepted implementation evidence:** Draft PR #372, code head `96f5b93900dc753674a5a03eafc9177fa8864a3c` on an older stacked S1 branch, Windows FAST #2960 93/93 PASS; this is NOT acceptance of native schema, full Edit, Windows FULL, UI, or permission to merge.

## 1. Goal and package boundary

Deliver one **headless, persistent, Part-owned Edge-local Datum Plane source** for independently certified Q1 selected material Edge END or Q2 exact bounded 3D arclength MID, using the accepted S1 signed Frame3 orientation intent. A newly authored Datum owns a stable global-within-Part `DatumId`, source-stage semantic reference, pre-Reverse signed normal/U seed, Reverse, and visibility. The current world Frame3 remains disposable evaluation data.

S2 must cover real command mutation, strict evaluation, v17 Save/Close/Reopen from an independent OCCT process/provider, dependent Sketch/Axis/Feature stage-consistency, source replacement Edit, Undo/Redo, visibility, deletion and repair/fail-closed lifecycle. S2 does not add a user-visible tool or any Qt/Viewer/Command Line/picking code. It does not silently activate S3.

**Product caveat:** the accepted full Edge + Point UX also requires a separately certified independent Point off the infinite supporting line of a straight Edge. Its durable Point + signed Edge anchor provenance is NOT yet proven by Q1/Q2/S1; it is an explicit unresolved D2 prerequisite before declaring the eventual full Datum Plane tool complete. Do not implement or claim off-line Point via XYZ/provider ordinal/screen token. The initial S2 schema supports only END/MID; adding the third source later requires an accepted bounded amendment or successor Work Contract.

## 2. Proposed exact durable source and public contract — Owner D2 decision

1. Add Part-local `EdgePointDatumPlane` with `DatumId id`, tagged `EdgePointDatumSource` equal to `SelectedMaterialEdgeEnd | SelectedMaterialEdgeMid`, `SelectedEdgeFrameIntent orientation` and `bool visible`; retain existing `OffsetDatumPlane` unchanged. A shared `DatumIdCursor` allocates both types and rejects duplicates across types and reused/deleted allocated identity.
2. Persist exactly semantic `MaterialEdgeReference`, END's independently certified `StableEdgeLocalTerminalKey` or MID role, and pre-Reverse normal/U orientation with `reverse`. Never persist current Frame3, runtime Vertex/Edge token, provider order, face centroid, coordinates as reference identity, or last-good result. Reuse S1's accepted 30-degree replay/review bound and numerical policy without silent reset after edits.
3. Add provider-aware `evaluateDatums(document, part_evaluation, exact_geometry_provider)` returning typed Resolved, Missing, Ambiguous, Unsupported or Blocked and specific orientation-review/geometry diagnostics. It must evaluate current exact declared Body stage, never final-Body searching or stale last-good. The existing Offset-only evaluator must retain its semantics; consuming Sketch/Feature/Axis paths requiring Edge-local Datums must use provider-aware derived evaluation when appropriate. A provider-free call must not report an Edge-local Datum as falsely Resolved.
4. Commands through `DocumentSession` + Part Transaction: `CreateEdgePointDatumPlaneCommand{source, expected_revision, reverse, visible}`; `EditEdgePointDatumPlaneCommand{datum_id, expected_revision, reverse}` preserving id/source; **add a distinct semantic source replacement command** with `datum_id`, newly selected strict END/MID source, expected revision and deliberate Reverse intent, reinitializing the signed seed only from newly certified source. Source replacement must be one atomic command/Undo step. This proposed added Edit-source command is a D2 public API decision, not retroactively authorized by PR #372.
5. Visibility is presentation-only; Delete rejects a Datum used by a dependent Sketch/Datum/Feature under existing semantic integrity rules. An upstream feature becoming Missing/Ambiguous/Blocked preserves authored Datum intent and dependent identity, but does not publish fake geometry. A source repair/edit must revalidate at execution, not reuse draft preview authority. Strict stage-cycle guards and global DatumId invariants remain.
6. Native Part schema is explicitly **v17** while modeling semantics version stays at accepted v2: retain legacy `datum_planes` array and add `edge_point_datum_planes` array to the authored payload. Each new item has exactly `id`, `kind:"edge_point_plane"`, `source`, `orientation`, `visible`. Source is tagged `edge_local_end` (strict `edge` + semantic `terminal`) or `edge_local_mid` (strict `edge`). Orientation contains finite unit-orthogonal `signed_normal_reference_world:[x,y,z]`, `u_seed_world:[x,y,z]`, `reverse:boolean`. No other source kinds may be inferred. Old v16 Offset data loads without a new array; writing v17 includes the new empty array when appropriate; old readers reject unknown v17 schema, not discard authored sources. Current Part parsing continues to reject malformed, unknown-kind, duplicate-ID/cursor-invalid and non-unit/nonorthogonal source intent atomically. Save retains ADR-0013 native guard/checkpoint behavior.
7. After a successful transaction, the same current revision and independent provider must resolve the newly authored or edited Datum. Failed Create/Edit never partially commits, consumes `DatumId` or Undo history, nor updates the saved-file checkpoint.

## 3. Finite accepted-test candidates (all future S2-specific claims UNACCEPTED pending Owner review)

- **S2-F01:** Real OCCT box 12 current material Edges / END and MID authored Datum source construction; unique global DatumIds, strict stage binding and no global three-Surface Point promotion.
- **S2-F02:** Atomic Create, Edit Reverse, explicit replacement of semantic END/MID source, one Command/Undo per change, Undo/Redo restores exact authored intent, no transient token stored.
- **S2-F03:** Current provider-derived Datum → Sketch → Axis/Feature or equivalent stage-aware downstream consumer; upstream move updates only derived placement, invalid stage blocks consumers without last-good fallback.
- **S2-F04:** Upstream delete/suppress/split/ambiguity/foreign owner/revision/exact-stage staleness; typed Missing/Ambiguous/Unsupported/Blocked and zero false Resolved.
- **S2-F05:** Source-preserving edit crossing axis-component ties keeps S1 signed U/V/N continuity up to 30°; beyond guard reports `orientation_review_required` without reinitialization or mutation; Reverse twice restores.
- **S2-F06:** Cold v17 Save/Close/Reopen with newly constructed independent OCCT provider; stable DatumId, source, exact signed orientation, dependent Sketch and repairable upstream missing case.
- **S2-F07:** Existing v16 Offset fixture loads correctly; unchanged Offset meaning and IDs. Unknown future version/Datum kind, malformed fields, bogus source, nonfinite or nonunit seed, duplicate global DatumId and invalid cursor are rejected; no partial state.
- **S2-F08:** Visibility and delete dependency integrity; one-transaction behavior, valid Edit/Cancel rejection, file-save guard conflict and Undo/Redo survival.
- **S2-F09:** Actual registered Windows kernel-native FOCUSED, FAST, Ready FULL (Core+Kernel+Desktop), DOC-01 internal current-state Markdown and official generated Browser parity; evidence ledger tied to exact runtime/docs SHA and final work-only closure comparison. Existing PM-03/Q1/Q2/S1/PM-05 regressions not weakened.

No S2-Fxx PASS is inferred from the existing #372 Windows FAST #2960 or #371 S1 Windows FULL #2941. Positive S2 tests must execute on actual native Windows sources, not be counted from source code alone.

## 4. Exact prospective implementation allowlist

After *this exact S2 contract* is Owner accepted, its governance PR squash-merged and a **separate accepted/merged `work/ACTIVE.yaml` activation** points at it, only the following paths may be changed on a new S2 production branch:

- `src/part/include/simplesolid2/part/datum.hpp`
- `src/part/datum.cpp`
- `src/part/include/simplesolid2/part/datum_evaluation.hpp`
- `src/part/datum_evaluation.cpp`
- `src/part/include/simplesolid2/part/part_document.hpp`
- `src/part/part_document.cpp`
- `src/part/include/simplesolid2/part/part_document_store.hpp`
- `src/part/part_document_store.cpp`
- `src/part/feature_evaluation.cpp`
- `src/application/include/simplesolid2/application/document_session.hpp`
- `src/application/document_session.cpp`
- `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp`
- `tests/pm03a_datum_schema_v10_test.cpp`
- `tests/pm03b_datum_evaluation_test.cpp`
- `tests/pm03c_datum_commands_draft_test.cpp`
- `tests/pm03e_datum_backed_sketch_test.cpp`
- `docs/internal/PART_DOCUMENTS.md`
- `docs/browser/index.html` (generate only via `.\\ss2.ps1 docs`; never manually edit)
- `work/RG-02B1_D2_P4_S2_AUTHORED_DATUM_V17_ACCEPTANCE_EVIDENCE.md`

No `src/ui/**`, `src/viewer/**`, `src/viewer_qt_occt/**`, `src/kernel/**`, `src/kernel_occt/**`, `src/sketch/**`, CMake/CI, roadmap/ADR or `ACTIVE.yaml` change inside a S2 implementation PR. Any path or semantic requirement outside this exact allowlist is STOP for Owner D2 amendment. Draft PR #372 currently **mixes S2 and S3** and includes unapproved UI/Viewer changes; do not squash-merge it wholesale as a S2 PR.

## 5. Branch, CI and Owner gates

Current `main` S1 closeout baseline: `bb52ba0bd1cf628a73108631234b8a86e84abef9`. The existing #372 working branch was stacked on pre-squash S1 history, so after #371 squash GitHub reports diverged history/mergeable=false even though its earlier FAST #2960 passed. Before an executable S2 PR, separate/bisect S2-only files from UI S3 changes, base new work on latest protected main, preserve original WIP history and ensure exact same-code semantic regressions. Do not mistake changing PR base for rebase or silently drop S1 evidence.

Strict order: Owner **D2 acceptance of entire §1–4** → exact-head contract governance CI → separate PR-specific contract squash permission and verified main merge → separate ACTIVE-only activation acceptance/merge → S2 code/tests/docs work on approved files → native FOCUSED/FAST/Ready FULL → S2 Owner technical FINAL PASS → independent exact-head production squash permission. After S2 closeout, S3 remains separately gated; no manual user-facing whole-tool PASS is declared by headless S2 tests.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: headless Part semantic Datum source, public commands, native schema v17, lifecycle, typed failures and downstream evaluation require current-state internal Part documentation; visible tool/UX remains in later S3 with its own Polish/English product documentation. Product Browser `docs/browser/index.html` is generated only from Markdown.

## Owner decision required

This draft is a **proposed exact D2 contract**. The Owner must accept or amend the additional versioned `EdgePointDatumPlane` model/v17 grammar, additive source replacement command, strict S2 boundary, nine finite test groups and file allowlist before any new S2 production mutation. Prior Owner S1 FINAL PASS, accepted P4 direction and general instruction to continue are **not** silently rewritten as acceptance of these new durable public/schema details.
