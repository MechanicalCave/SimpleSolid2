# RG-02B1 / P4-S4A — Datum Plane 3 Points (END-only): Part-owned v18 headless executable Work Contract

**Status: D2 OWNER REVIEW PROPOSAL — NOT ACCEPTED, NOT ACTIVE, NOT EXECUTABLE.**
**Proposed:** 2026-10-15.
**Parent D2 product/design direction:** `work/RG-02B1_D2_P4_S4_THREE_END_POINTS_DESIGN_PROPOSAL.md` — Owner accepted and merged PR #389.
**Positive source-identity prerequisite:** `work/RG-02B1_D2_P4_S4_P0_END_POINT_IDENTITY_NATIVE_PROOF_CONTRACT_DRAFT.md` + `work/RG-02B1_D2_P4_S4_P0_END_POINT_IDENTITY_NATIVE_PROOF_EVIDENCE.md`: P0 Owner technical FINAL PASS, real native FOCUSED #3094 1/1, Ready FULL #3095 199/199, closure #3098 SUCCESS, P0 PR #392 squash-MERGED main `1254562132b3c3fe7d39b08c1452afff1135f729`.
**Normative authorities:** Foundation v1.0, Constitution, Architecture, accepted ADR-0012/0013/0014/0016/0017, Part Modeling roadmap accepted v1.33, DOC-01. `work/ACTIVE.yaml` still points to historical completed P0. No current S4A production mutation authority.
**Next stage:** separately scoped S4B GUI / CAD Command Line integration of this same constructor. **Do not implement S4B here.**

## 0. Precise Owner D2 decision this proposal needs

Accept **headless Part-native production** of `Datum Plane → 3 Points (END-only)` using exactly three ordered **unique strict same-Part, same-Body-stage semantic Point references** proven by P0, together with new additive native Part **schema v18** and semantic create/edit/replace/delete/visibility/dependency/lifecycle tests. This is not a general Point authoring subsystem or a new user tool.

The third constructor will later be part of the **same existing Datum Plane tool** alongside Offset and Edge + Point, with the **same Operations/Properties, existing END hover, Edit, Reverse, Finish/Cancel, Undo/Redo and global CAD Command Line grammar** in S4B. Do **not** add separate `Three Points` tool, new hover framework, global key routing, independent Edge operand, or MID for this constructor. Off-infinite-line standalone Point+Edge interpretation was explicitly removed from the product target in parent D2 design.

This proposal is limited to S4A implementation **after** independent Owner approval, contract governance merge and `ACTIVE.yaml`-only activation. User's broad permission to finish *P0* does not itself authorize this separate persisted-schema D2 contract.

## 1. Exact Part semantics / public boundary

1. Add `ThreePointDatumPlane` as a separate typed authored kind with existing `DatumId`, visible flag, three ordered `StageSemanticPointReference` values and minimal signed orientation intent. **Do not reinterpret** existing `EdgePointDatumPlane` or `OffsetDatumPlane`.
2. A durable point source contains **`BodyStageRef` plus `FeaturePointAddress`**; document scope is owning `PartDocument::DocumentId` and execution revalidates it. No durable source Edge, `SelectedMaterialEdgeEnd`, provider token, vertex ordinal, XYZ, topology traversal index, runtime revision or cached world frame.
3. All three points must be **distinct semantic keys in one currently valid exact Body stage of this Part**; a shared corner selected from three incident Edges is one key and cannot count as P1/P2/P3. P0 proves the canonical box family, **not a universal class**. Missing/Ambiguous/Unsupported source must fail closed without promotion based on incidence, nearest geometry or equal XYZ.
4. Validate at Part command execution and evaluation freshness: every key resolves to exactly **one current stage `FeaturePointResolution` with one current Vertex** and current finite physical 3D position. An invalid original stage, missing producer, stage mismatch, stale `DocumentRevision`, wrong document or more than one candidate rejects authoring; do not auto-fix any source.
5. **Frame convention:** P1 origin; `U=unit(P2-P1)`, `N=unit((P2-P1)×(P3-P1))`, `V=N×U` in current Part world coordinates, right-handed metric. Reverse changes `(U,V,N)→(U,-V,-N)`; twice restores original frame. Store ordered semantic sources and the minimal orientation choice, not Frame3 or world geometry. Reject duplicate, coincident, collinear and numerically unstable triples using one fixed explicitly documented Part geometric-admissibility policy (scale-aware, independent of Viewer/display tolerance; no iterative tolerance escalation or geometry repair). If a change to modeling semantics version is needed, **STOP for Owner D2** rather than silently changing it.
6. New `ThreePointDatumPlane` participates in the **single shared DatumId cursor, find/lookup, visibility, Delete and dependency management** with Offset and EdgePoint Datum. A duplicate ID across any Datum kind is invalid. Sketch/Axis/Feature dependents referencing the same DatumId must resolve or fail strictly, never attach to another Datum kind with a guessed ID.
7. Existing `DatumPlaneReference{DatumId}` and dependency-floor semantics must work with the new kind. An invalid upstream point after Feature mutation yields typed Missing/Ambiguous/Unsupported/Blocked with **no stale last-good plane** published as resolved. Feature/Sketch dependency cycle checks and stage ordering remain deterministic; no global parametric graph.
8. Add typed semantic Commands to **Create**, **Edit/Replace 1–3 points and Reverse**, **Delete** and **SetVisibility** through existing application DocumentSession/Transaction infrastructure; Edit keeps the original DatumId and dependent identities, one successful operation produces one Undo step. Validation and full-snapshot PartTransaction revision guard precede mutation. No GUI/Viewer object may mutate Part directly.
9. Canceled/invalid preview is not authored state; S4A may use a provider-neutral headless evaluation/read function for later S4B but **must not implement a second persistent draft or UI state machine** in this contract.
10. Old Offset Datum and EdgePoint END/MID semantics—including v17 native persistence and accepted S3A mouse/Command Line behavior—must remain byte/semantic compatible where their declared version requires it.

## 2. Versioned persistence and migration — v18 only

1. Bump `PartDocumentStore::current_schema_version` from v17 to **v18**. Add a **distinct explicit collection** `three_point_datum_planes` to the v18 authored JSON, mandatory even when empty. Preserve existing `datum_planes` and `edge_point_datum_planes` shapes and meanings, no renumber/implicit union reinterpretation.
2. Each v18 3-Point entry has **exactly five keys**: `id` (string DatumId), `kind` (literal `three_point_plane`), `points` (array of exactly three ordered source objects), `orientation` (object containing exactly one boolean `reverse`), `visible` (boolean). Each `points[i]` object contains exactly `stage` and `point`. `stage` is exactly `{"kind":"after_feature","feature_id":"<serialized FeatureId>"}`, matching the accepted stage shape already serialized by MaterialEdgeReference. `point` is precisely the existing `featurePointAddressJson` payload `{"producer_feature_id":"<FeatureId>","adjacent_surfaces":[...three sorted semantic surface addresses...]}`, parsed by the existing `parseFeaturePointAddressV14` grammar with v18 version context; preserve its recursively strict Surface provenance, sorted canonical source and exact field-size validation. Do not add `edge`, `terminal_ordinal`, `vertex_token`, `xyz`, `frame`, `revision` or other data. Structural validation checks stage feature is currently declared in the authored history and point producer belongs to that valid stage or its legal upstream ancestry. Runtime geometric feasibility is separately revalidated on current evaluation. Use existing native JSON `DatumId` and `FeatureId` serializers, never human display names. A schematic full entry:

   ```json
   {
     "id": "<DatumId>",
     "kind": "three_point_plane",
     "points": [
       {"stage": {"kind": "after_feature", "feature_id": "<FeatureId>"},
        "point": {"producer_feature_id": "<FeatureId>", "adjacent_surfaces": ["<existing FeatureSurfaceAddress JSON>", "<...>", "<...>"]}},
       {"stage": {"kind": "after_feature", "feature_id": "<same FeatureId>"},
        "point": {"producer_feature_id": "<FeatureId>", "adjacent_surfaces": ["<...>", "<...>", "<...>"]}},
       {"stage": {"kind": "after_feature", "feature_id": "<same FeatureId>"},
        "point": {"producer_feature_id": "<FeatureId>", "adjacent_surfaces": ["<...>", "<...>", "<...>"]}}
     ],
     "orientation": {"reverse": false},
     "visible": true
   }
   ```

   The placeholder strings in `adjacent_surfaces` above are **diagrammatic only**, not legal JSON values: actual entries must be valid existing `FeatureSurfaceAddress` objects. The parser must reject the example placeholders as invalid. No new version or interpretation of `FeaturePointAddress` is introduced.
3. v17 and earlier supported legacy native documents load with **empty 3-Point collection** and retain all old meaning, including historical stored EdgePoint END/MID data. v18 serializes all three Datum collections; cold load on independent Part/OCCT re-evaluates from semantic sources. No persisted provider handles, point XYZ, preview frame or viewer click evidence.
4. Reject malformed v18 triples, unknown source kinds, wrong stage, invalid/duplicate/cross-kind DatumIds, invalid cursors, invalid/cyclic dependencies and unsupported future field layouts instead of silently dropping content. Do not allow a v18→v17 masquerading downgrade, guessed migration or schema version spoof.
5. Existing guarded Save / atomic replace / file checkpoints and stale transaction invariants (ADR-0012/0013) are unchanged. Failed Save/parse/commit leaves Part/history/file checkpoint unchanged. All legacy native format fixtures remain legitimate regression fixtures; do not weaken assertions to force v18.
6. Part v18 means a deliberate **format compatibility change**; record the exact migration and negative matrix in internal and product documentation, using DOC-01 and generated Browser only.

## 3. Exact proposed file allowlist for S4A — not writable until activation

**Part model / evaluation / native persistence:**
- `src/part/include/simplesolid2/part/datum.hpp`
- `src/part/include/simplesolid2/part/datum_evaluation.hpp`
- `src/part/include/simplesolid2/part/part_document.hpp`
- `src/part/include/simplesolid2/part/part_document_store.hpp`
- `src/part/datum.cpp`
- `src/part/datum_evaluation.cpp`
- `src/part/part_document.cpp`
- `src/part/part_document_store.cpp`

**Application Command / Transaction only:**
- `src/application/include/simplesolid2/application/document_session.hpp`
- `src/application/document_session.cpp`

**Already registered existing test targets:**
- `tests/pm03a_datum_schema_v10_test.cpp` (v18 grammar / legacy migration and malformed fixtures; original v10/v17 assertions preserved)
- `tests/pm03b_datum_evaluation_test.cpp` (headless Frame3, dependency floor, failure statuses)
- `tests/pm03c_datum_commands_draft_test.cpp` (actual application Command and Undo/Redo boundary, NO new GUI)
- `tests/pm03e_datum_backed_sketch_test.cpp` (dependent Sketch/Axis lifecycle)
- `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` (real OCCT strict Point source + cold replay, retain full P0/Q1/Q2)
- `tests/pm05b1_edge_feature_schema_v14_test.cpp` (legacy schema fixture version-gate assertions **only if** necessary; never alter genuine legacy witness semantics)

**As-built / product DOC-01 parity:**
- `docs/internal/PART_DOCUMENTS.md`
- `docs/product/pl/PARTS.md`
- `docs/product/en/PARTS.md`
- `docs/browser/index.html` (only regenerated with official `.\ss2.ps1 docs` command)

**Completion evidence:**
- `work/RG-02B1_D2_P4_S4A_THREE_END_POINTS_V18_HEADLESS_ACCEPTANCE_EVIDENCE.md`

Everything else is **OUT**. In particular no `src/ui/**`, `src/viewer/**`, `src/viewer_qt_occt/**`, kernel/provider internals or new public Kernel API, `src/application/datum_plane_draft.cpp` or UI draft, CMake/CI, new test executable, `work/ACTIVE.yaml` inside production PR, Foundation/ADR, additional Datum tool, independent off-line Point/Edge, PM-06, universal Point subsystem, global UI hover redesign, drawing/assembly. If exact requirements cannot be met with this allowlist and accepted interfaces, STOP for one concrete Owner D2 amendment, not permissive refactoring.

## 4. Finite acceptance matrix — no inferred PASS

- **S4A-F01:** native OCCT box: 24 certified END map to 8 unique stage-qualified Points (reuse P0 proof); three distinct noncollinear current Points create one separately typed ThreePointDatumPlane with one DatumId, right-handed Frame3.
- **S4A-F02:** one/repeated Reverse exact signed frame, point order stays constant; noncollinear orientation and frame move correctly after same-producer dimension edit, with no authored Frame3.
- **S4A-F03:** same trihedral point via another Edge does not create another point; duplicate/non-collinear/collinear/near-collinear tested explicitly. Unsupported/ambiguous Points, stale stages, document/body mismatch and deleted/replaced FeatureId reject without false Resolved.
- **S4A-F04:** Create, Replace P1/P2/P3, Reverse, Delete, visibility; single atomic Command/Transaction/Undo, valid no-op semantics; failed/stale/Cancel cases zero authored side effects and no Undo inflation.
- **S4A-F05:** downstream Datum-backed Sketch and Axis (including dependent Feature where current Part rules permit), correct required Body stage and cycle rejection; failed upstream source remains repairable and blocks stale geometry.
- **S4A-F06:** native v18 Save/Close/Reopen on separately instantiated real OCCT provider reproduces point identity, frame sign, DatumId, visibility and dependent IDs; old v17/v16 and earlier supported fixtures round-trip without false new collection semantics.
- **S4A-F07:** malformed/unknown-kind/duplicate-ID/cross-kind collision/stale-point/cyclic dependency/native schema negatives, v18→v17 downgrade refusal, ADR-0012 transaction and ADR-0013 guarded Save regressions PASS. No test deletion or weakening.
- **S4A-F08:** Windows real registered FOCUSED, FAST, Ready FULL Core+Kernel+Desktop, official DOC-01 internal + PL/EN Markdown and generated Browser parity; exact source/test/docs SHA. No mocked native proof substitute.
- **S4A-F09:** bounded Owner **technical** acceptance of headless v18 semantics plus separate guarded production merge. S4A **does not claim Owner GUI/manual PASS** or 3 Points Command Line usability; those are S4B.

## 5. Execution / authority gates

1. **COMPLETED:** parent S4 design PR #389 D2 accepted/merged; P0 contract #390 accepted/merged/activated; native P0 PR #392 real Windows FOCUSED+FULL **PASS**, Owner technical PASS and guarded squash-MERGED `main` `1254562132b3c3fe7d39b08c1452afff1135f729`.
2. **PENDING:** Owner approves this **exact S4A D2 persisted source/v18/Command/test/document allowlist and finite F01–F09 scope**. Do not treat the separate P0 blanket-formality permission as new D2 product/schema approval.
3. A **work-only contract governance PR** passes its Windows closure and receives guarded squash merge authority; then a **separate `work/ACTIVE.yaml`-only activation PR** sets `active_work` to this accepted S4A contract after its own governance closure and protected-main squash.
4. Only then a NEW production branch from activated `main` may modify exactly S4A allowlisted files. FOCUSED → FAST → Ready FULL with actual Windows OCCT tests; exact-head evidence + required DOC-01 docs/Browser parity; Owner technical acceptance; guarded merge.
5. S4A closure does not authorize **S4B UI / Command Line**. Its independent narrow contract must guarantee one existing Datum Plane tool and parity with Offset, Edge+Point END/MID, hover, Operations, global Command Line `DATUMPLANE`/`FINISH`/`CANCEL`/`REVERSE`, owning Enter and physical Esc, Edit, Undo/Redo, cold reopen and actual native Qt mouse test. No new separate Point tool or MID in 3 Points.
6. **STOP (D2):** Point provenance cannot be uniquely resolved in a supported family; persistence needs a different source identity or new schema rule; tolerance/modeling-semantics version needs change; new Part/kernel public API or file outside allowlist; additional product gesture or UI redesign is needed. Escalate the concrete evidence; never silently widen scope.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: durable Part Datum semantic type, cross-kind DatumId/dependency behavior and v18 native format affect as-built ownership and file compatibility. Update current-state internal plus PL/EN product Markdown, regenerate `docs/browser/index.html` with official `.\ss2.ps1 docs`; never hand-edit generated HTML.
