# RG-02B1/P4-S3A — Datum Plane Edge-local END/MID GUI / Viewer — proposed executable Work Contract

**Status: PROPOSED OWNER D2 UI/DELIVERY DECISION — NOT ACCEPTED, NOT ACTIVE, NOT EXECUTABLE.**
**Date:** 2026-10-14.
**Current protected-main baseline:** `0dbfa189fe0e117672a630c66a934923685dc9da`, including Owner-manual-PASS S2 source #376 squash `be5c2eef6fd7ecf1aa5603ccbb56cd3012df0a2a` and separate S2 historical closeout PR #381. `work/ACTIVE.yaml` still points to the last accepted *completed* S2 contract; that does not authorize S3 source edits.
**UX authority:** Owner-accepted *direction* `work/RG-02B1_DATUM_EDGE_POINT_WORK_CONTRACT_DRAFT.md` (PR #345 merged), especially precedence §0A/§0B/§1B: ONE existing Datum Plane tool, no separate on/off-edge modes, either pick order, direct END/MID marker, Edge-local shared corner wins, same global Command Line/focus/Esc/Enter/Reverse grammar. Accepted roadmap `work/PART_MODELING_V1_ROADMAP.md` v1.33 requires one small bounded independently accepted D2 contract and UI consultation before code.
**Existing donated WIP:** Draft #372 (`96f5b93900dc753674a5a03eafc9177fa8864a3c`) contains mixed historical S2/S3 files with divergent pre-squash history and GUI fallback-only tests. It is NOT merge authority and must not be wholesale copied/rebased. Extract only reviewed S3-specific changes to a clean branch off activated current main.

## 0. One explicit Owner decision — stage the UX without silently splitting the tool

**Proposed bounded S3A delivery:** the **single existing** `Datum Plane` tool gains an `Edge + Point` constructor that can actually select a **certified END or MID of its chosen bounded material Edge**, show a transient oriented perpendicular Datum preview, Reverse/Finish/Cancel, perform Edit/resourcing, Undo/Redo and cold v17 Save/Reopen using already accepted S2's Part-owned source. Both Edge→Point and Point→Edge, including a shared corner, use that SAME constructor and input context. The existing Offset constructor remains unchanged.

**Explicit staged limitation requiring this new Owner acceptance:** independently certified **off-infinite-line Point + straight Edge** meaning (plane *containing* the Edge and off-edge Point) is NOT implemented by native v17 S2. During S3A, such a pick must show a typed **unsupported pending future contract** result with **no preview/Finish/Undo/mutation**; it may not be misinterpreted as MID, END or screen XYZ. **No second tool, new user mode, hidden fallback or false “complete P4” claim.** The final accepted one-constructor UX will require a separate evidence-first D2 source/schema contract for off-line Point (possible v18, *not preapproved*), followed by completion of the same constructor. Owner must knowingly accept this **intermediate partial availability** before S3A activates. If Owner requires BOTH interpretations before any user-visible constructor, do not accept this S3A scope; instead propose a separate full-source D2 contract. This staging choice is a new **D2 product delivery decision**, not silently inferred from S2/manual PASS or original all-in-one UX principle.

## 1. Exact S3A behavior and semantic boundary

1. Enter using existing Operations `Datum Plane`, `DATUMPLANE` / `DATUM PLANE` Command Line or selection-first route. Existing `Offset` selected by default and completely unchanged. `Edge + Point` is a constructor value in the same UI and reuses the shared draft/CAD input context, not a new tool, parser or keyboard policy.
2. `Edge → Point`: first click one strict `MaterialEdgeReference` at its current declared Body stage. Keep selection highlight and a transient selection lease. Second click the Edge's own directly selectable END or the exact 3D bounded half-length MID; no extra END1/END2/MID control. MID uses the **same visual marker as END**. Hover is never authorship.
3. `Point → Edge`: first click retains the complete current viewer candidate stack plus cursor evidence, Part `DocumentRevision`, Body stage and Viewer generation. After choosing the Edge, prove the clicked location corresponds uniquely to **this specific selected Edge's** certified semantic END or MID, regardless of whichever other coincident Point was rendered as hover. On the shared corner of Edges A/B/C, Point→Edge B must author B's own END, not A's globally hovered Point. The reverse order must produce identical durable source/frame. No screen distance/provider-token tie decides stored identity.
4. The point-candidate stack is transient and discarded on Edit/Cancel/revision/Body-stage/Viewer-generation invalidation. Native runtime Edge/Vertex tokens, pick index, pointer world XYZ and drawing handles are NEVER persisted. Current S2 `SelectedMaterialEdgeEnd | SelectedMaterialEdgeMid` plus saved `SelectedEdgeFrameIntent` is the **only** durable meaning. If stage/current provider cannot certify uniquely, report Missing/Ambiguous/Unsupported/Blocked rather than constructing a last-good preview.
5. Certified END/MID displays an **immediate default oriented plane**, S1 signed U/V/N; `Odwróć`/Reverse button and existing Command Line `REVERSE` must show precisely the same flipped frame and second Reverse restore exactly. Changing orientation changes neither Edge nor END/MID identity. No unproved tangent-normal sign ambiguity treated as loss of source identity.
6. Reuse existing `FINISH`, owner-context empty Enter, `CANCEL`/`ESC`, and global physical key/focus precedence, shared generation lease and diagnostics. Valid Finish/command obtains exact fresh current-stage source via S2 API, commits **one transaction, one Undo**, shows same new DatumId in Tree and survives Undo/Redo. Cancel/selection-only/preview/stale/unsupported failure changes zero authored state or checkpoint. Do not invent free-XYZ Command Line operands; source picks remain through current semantic viewport acquisition.
7. Edit an existing S2 END/MID Datum: current source, original DatumId and dependent Sketch/Axis must survive Edit/Reverse/source replacement; use accepted `ReplaceEdgePointDatumPlaneSourceCommand` (no Delete+Create or ID renumber). Failed Edit/Cancel does not commit or consume Undo. Tree selection/properties/reference-geometry visibility and stage-based error feedback reuse existing Datum Plane patterns.
8. New Part schema/version/public semantic Part API are **OUT** of S3A: current v17 headless source and provider queries are sufficient only where strictly proven. No v18 placeholder, new point source or fallback persisted data. If implementation demonstrates a missing kernel/Part public API or a true UX input-grammar incompatibility, **STOP** for exact Owner D2 amendment before touching any other file.

## 2. Finite S3A acceptance tests — all PENDING real execution

- **S3A-F01:** Real OCCT straight bounded material Edge END selection from viewport; both endpoint roles certified, faithful stage binding, transient marker and correct perpendicular Frame3; no source accepted from arbitrary screen position.
- **S3A-F02:** Real certified MID on a straight Edge: mouse hit on identical END-style marker, exact bounded half-arclength and tangent, default frame, Reverse twice restoration.
- **S3A-F03:** **Both actual mouse pick orders** `Edge→END/MID` and `END/MID→Edge` on a shared-corner box with first hover on *another* Edge; chosen Edge's own END wins. No chooser/provider ordinal, no global Ambiguous Point promotion.
- **S3A-F04:** Same tool from selection-first, Operations and DATUMPLANE; same Command Line FINISH/empty Enter/CANCEL/ESC/REVERSE, physical focus/keyboard priority; no new global hotkey grammar, Offset unaffected.
- **S3A-F05:** Real Qt/OCCT positive GUI gesture test(s), not merely unsupported-kernel fallback; preview from live provider and deliberate Finish/Cancel/Undo/Redo, no Body scene recreate between the two picks; callback leases stale on revision or viewer generation.
- **S3A-F06:** Real Edit existing END/MID → different Edge or END↔MID, unchanged DatumId, Sketch/Axis dependents, single Undo/Redo; dependent-unavailable source fails closed, unchanged authored state and native save checkpoint.
- **S3A-F07:** Save/Close/Reopen of user-created END/MID Datum+dependent Sketch on independent provider, signed frame/Reverse and metadata preserved; existing Offset, Tree visibility and Feature workflows regressions intact. No post-reopen viewport token persisted.
- **S3A-F08:** Exact unsupported cases: off-infinite-line Point (documented S3A limit), ambiguous/unreferenceable semantic END, stale Body stage, closed periodic circle/degenerate/unsupported Edge, real out-of-scope Point source. No preview/Finish when unsupported; zero mutation/Undo.
- **S3A-F09:** Windows FOCUSED on real registered Qt/OCCT viewport + Datum tests, FAST, Ready FULL Core/Kernel/Desktop; DOC-01 as-built Internal and PL/EN Product Markdown, **Browser only via `.\\ss2.ps1 docs`** and authoritative parity; acceptance ledger exact source/docs SHA, Owner manual click-path PASS. Existing #372 FAST or fallback-only Qt checks cannot be called S3A positive PASS.

The final manual Windows checklist must exercise mouse END/MID (both orders, shared corner), preview and double Reverse, Finish, Edit/repair, Undo/Redo, dependent Sketch, save/cold reopen and Offset regression. No full P4 independent off-line Point acceptance is claimed.

## 3. Exact proposed S3A implementation path allowlist

**Allow no other source/test files unless Owner approves a concrete amendment.** All items here are candidates for new D2 authority, not currently writable under completed S2.

UI/Viewport/Viewer (review/re-extract GUI work from mixed WIP #372 only where still valid against S2 merged main):
- `src/ui/cad_workbench.cpp`
- `src/ui/cad_workbench.hpp`
- `src/ui/part_viewport_controller.cpp`
- `src/ui/part_viewport_controller.hpp`
- `src/ui/part_document_tree_controller.cpp`
- `src/viewer/include/simplesolid2/viewer/reference_presentation.hpp`
- `src/viewer_qt_occt/qt_occt_viewer_widget.cpp`

Reuse established application draft/Command Line boundary, without changing core Part v17:
- `src/application/datum_plane_draft.cpp`
- `src/application/include/simplesolid2/application/datum_plane_draft.hpp`

Already registered exact tests (extend with actual positive interactions; do not weaken any existing fallback test):
- `tests/pm03c2_cad_workbench_datum_plane_test.cpp`
- `tests/pm03c_datum_commands_draft_test.cpp`
- `tests/sk04b_part_viewport_selection_bridge_test.cpp`
- `tests/sk04b_viewer_native_selection_query_test.cpp`
- `tests/viewer_reference_presentation_test.cpp`
- `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp`

As-built and product documentation (both languages) and acceptance:
- `docs/internal/PART_DOCUMENTS.md`
- `docs/product/pl/PARTS.md`
- `docs/product/en/PARTS.md`
- `docs/browser/index.html` (official generator only)
- `work/RG-02B1_D2_P4_S3A_END_MID_GUI_ACCEPTANCE_EVIDENCE.md`

Explicitly excluded: `src/part/**`, `src/kernel/**`, `src/kernel_occt/**`, native schema v18 and persistence, `src/application/document_session.cpp`, new CMake/CI files, Qt app shell other than listed paths, Foundation/ADR/roadmap, `work/ACTIVE.yaml` inside the production PR, new standalone Datum Point/Axis/other constructors, G1, Mirror/Pattern, PM-06. No merge of Draft #372 as an S3A code PR.

## 4. Execution and closure gates

Owner accepts **this exact S3A D2 staged-scope/UI Work Contract**, expressly acknowledging that the **same one user tool** will temporarily refuse independent off-edge Point rather than prematurely claiming full originally accepted P4 UX. Then (1) governance-only Work Contract PR Windows closure and separate expected-HEAD protected-main squash; (2) separate `work/ACTIVE.yaml`-only activation pointing to this S3A contract, its own Windows governance and guarded main squash; (3) implement the small bounded S3A GUI on a NEW clean branch off that activated main, donor-selectively extracting from #372 rather than wholesale merge; (4) real FOCUSED→FAST→Ready FULL native Qt/OCCT, docs and exact-HEAD Browser; (5) separate Owner manual positive mouse/keyboard acceptance and technical FINAL PASS; (6) separate exact-head production squash and final status closeout. A draft PR or general `kontynuuj` instruction is not this explicit new D2 staged-delivery decision.

The later **off-line independent Point** interpretation must join the **same** `Edge + Point` user tool, subject to separately proven point source identity, exact noncollinear straight Edge geometry, potential native schema v18, failure/lifecycle tests and fresh Owner D2/API/versioned persistence acceptance. Do not split it into a second visible tool or silently claim the full one-tool product finished now.

## Documentation impact

Internal docs: required (current S2→S3A Viewport/Command Line/Edit/semantic source evaluation behavior).
User/Product PL and EN: required (real one Datum Plane constructor, END/MID gestures, explicit unsupported off-edge interim limitation, Edit and error feedback).
Browser: regenerate ONLY by `.\\ss2.ps1 docs` on the supported Windows runner and prove exact canonical parity.
