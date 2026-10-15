# RG-02B1 / P4-S4B — 3 END Points in the ONE existing Datum Plane tool: GUI / CAD Command Line executable Work Contract proposal

**Status: OWNER D2 REVIEW PROPOSAL — NOT ACCEPTED / NOT ACTIVE / NOT EXECUTABLE.**
**Date:** 2026-10-15.
**Base protected main:** `30bd70e8d17c90d9c8defd187e240ffb4f321c09`; accepted S4A real native Windows Ready FULL #3166 **199/199**, exact-head Ready CLOSURE #3169 SUCCESS, Owner S4A technical FINAL PASS and separately authorized guarded squash PR #395 MERGED.
**Original Owner-approved product direction:** `work/RG-02B1_D2_P4_S4_THREE_END_POINTS_DESIGN_PROPOSAL.md`, governance PR #389 MERGED. Preserve exact bounded native precondition P0 PR #392 and S4A headless v18 `ThreePointDatumPlane` PR #395; never redefine durable Point identity.
**Authority:** Constitution, Foundation v1.0, Architecture, accepted ADR-0012/0013/0014/0016/0017, accepted Part Modeling roadmap v1.33, DOC-01, design #389, completed S4A contracts/evidence. `work/ACTIVE.yaml` still points to **completed S4A**, so there is **NO S4B source mutation authority** until independent Owner acceptance of THIS scope, guarded governance squash and separately authorized `ACTIVE.yaml`-only activation.
**Milestone:** RG-02B1/P4-S4B, single bounded interaction layer completing already-delivered P4-S4A headless v18 Part Datum. This proposal itself changes ONE `work/**` file and no production code.

## 0. Product decision to ratify as exact S4B Work Contract

1. **The same existing Datum Plane tool**, no new tool/action/modal window. In its existing Operations constructor selector, the third choice is labeled **`3 Points`** after `Offset` (default) and `Edge + Point`. No separate Edge operand or Edge-first hidden requirement. END-only. The existing Edge+Point END/MID (including MID-first), Offset, Preview, Properties, Reference Geometry Tree, hover, Command Line and keyboard behaviors retain their established meanings.
2. The user explicitly clicks three current-stage **END points** as P1 → P2 → P3; each click acquires an *independently strictly certified semantic Point*, **not** a selected Edge and not a picked XYZ. The three clicks may originate on different material Edges but must belong to the same Part document and same exact Body stage. A trihedral corner shared by multiple incident Edges is **one** source identity. Three incident-edge ENDs of the same corner cannot count as three different points.
3. **Before three valid distinct, noncollinear points**: no plane preview, `Finish` disabled, no transient-only source committed, no Undo or native-save checkpoint change. Once three strictly resolved Points form a valid frame, show the existing oriented datum-plane draft preview with P1 anchor; `Reverse` flips signed V/N, not source addresses, and a second Reverse restores frame. The display frame is an ephemeral view of a fresh Part computation — never a new authored field or cached "last good" frame after an invalid pick.
4. **Explicit slot visibility/replacement:** one compact P1/P2/P3 source/status area within the existing Operations widget, with a deterministic active acquisition slot; selecting a slot replaces only that slot with the next certified END click (and revalidates the entire frame). Do not create a second independent draft/operation system or silently change P1 when choosing P3. On Edit preload ordered existing source slots and `reverse`; keep existing `DatumId`, dependent Sketch/Axis IDs, visibility and one atomic Undo step. Cancel restores the original document and does not consume a DatumId; invalid or same-corner candidate does not change authoring.
5. UI `Finish`, owning-context Enter and CAD Command Line `FINISH` submit exactly **one existing S4A transactional** `CreateThreePointDatumPlaneCommand` or `EditThreePointDatumPlaneCommand` at current expected `DocumentRevision`. Failed/stale/Unsupported/Ambiguous/Missing/Blocked/degenerate candidate must fail closed and show a legible diagnostic; it must not append a Datum Plane, inflate Undo or use nearby Vertex/token fallback. Existing shared `SetDatumPlaneVisibility` and `DeleteDatumPlane` UI actions apply to all three datum kinds.
6. **No generic Point subsystem, no MID inside 3 Points, no off-infinite-line Point+Edge variant** (explicitly withdrawn in design #389), no extended topology provider interface, no new native Part format/schema beyond accepted v18, no new default keyboard route or global hover policy, no new CAD command tool, no PM-06 presentation fill, no planning/roadmap rewrite inside this executable contract.

### CAD Command Line constructor tokens — exact bounded D2 proposal

Existing `DATUMPLANE` and `DATUM PLANE` still start the **same** tool; `Offset` remains default. In a Datum-owned input context, propose `3POINTS`, `EDGEPOINT` and `OFFSET` for switching its constructor (also available from Operations). `P1` / `P2` / `P3` choose a replacement slot in an active 3-Point draft; actual Point values **must still come from a certified END click**, never text XYZ or a freehand coordinate parser. `REVERSE`, `FINISH`, `CANCEL`, owning Enter and physical Esc retain their established focus/precedence semantics. `FINISH` is refused until a valid triple; a first physical Esc during an incomplete draft may clear its active pick stage only if this is consistent with the existing Datum Esc policy, while explicit `CANCEL` always exits with zero authored mutation. Tokens are case-insensitive within Datum context and must not intercept unrelated Sketch/Feature tools. **Owner D2 accepts or edits these exact new tokens together with this proposal**; do not invent extra verbs at implementation time.

## 1. Existing as-built integration seams, verified at the post-S4A protected-main commit

1. `src/ui/cad_workbench.cpp` has **one** `datumPlaneToolButton`, **one** `datumPlaneConstructorCombo` (currently Offset=0, Edge+Point=1), **one** `datumPlaneOperationsWidget`, `datumPlaneFinishButton`, `datumPlaneReverseButton`, `datumPlaneResultLabel`, `datumPlaneSourceLabel` and current `startDatumPlaneTool/startDatumPlaneEdit/selectDatumPlaneConstructor/tryStageDatumPlaneFromBodyTopology/refreshDatumPlaneEvaluation/syncDatumPlaneUi/finishDatumPlane/submitDatumPlaneCadInput/clearDatumPlaneRuntimeContext` paths. Extend these rather than adding a second toolbar action or a new GUI draft package. Current `submitDatumPlaneCadInput` only accepts the Offset `datum_plane_draft_`, so explicitly extend it to route the third constructor and preserve original Offset/Edge+Point grammar without silent diversion.
2. `src/ui/cad_workbench.hpp` owns ephemeral Offset + EdgePoint draft contexts, revisions and generation lease; add minimal 3-Point transient P1/P2/P3, mode/edit-ID/reverse/revision/generation/selected-slot state **only while that one Datum Plane tool owns the input context**. A source-click stage switch invalidates stale Viewer pick leases and cancels any old preview.
3. `src/ui/part_viewport_controller.hpp/.cpp` already has `BodyTopologyInspection` containing **`stage`, `strict_referenceability`, `semantic_candidate_count`, optional `point_address`, optional `provider_point`** and current Viewer BodyTopology selection generation. It also has the existing END picking / exact native selected-Edge-terminal proof and reusable `setDatumPlaneDraftPreview`. A clicked Vertex is eligible as P1/P2/P3 only if independently strict current-stage catalog evidence certifies its semantic `FeaturePointAddress` as **one current Point with exactly one Vertex**, one finite native location, and an actual supported material-Edge END. The Viewer `runtime_token`, selected incident Edge, hit-order, screen tolerance or XYZ is a one-generation *gesture* only; durable S4A `StageSemanticPointReference` is `BodyStageRef + FeaturePointAddress`. If `BodyTopologyInspection` lacks evidence of material END eligibility, add one narrow **read-only same-revision certified Point inspection method** in these existing viewport-controller files, using current catalog and already-accepted native queries. No new Kernel API or Viewer engine/renderer changes; any such need is D2 STOP.
4. `src/ui/part_document_tree_controller.cpp` currently lists only `datumPlanes()` and `edgePointDatumPlanes()` under Reference Geometry and aggregates visibility targets from those two collections. Extend the **same** tree group to S4A `threePointDatumPlanes()`; stable `DatumId` role, existing selected-state/visibility, one indexed `Datum Plane … (3 Points)` row, typed evaluation warning and user-facing source status, and existing Edit/Delete routing. No separate three-Point tree root. The single DatumId namespace must not collide by display index.
5. `src/application/document_session.cpp/.hpp` from S4A **already has** transactional `CreateThreePointDatumPlaneCommand`, `EditThreePointDatumPlaneCommand` and shared Delete/Visibility. GUI must call these directly and revalidate on Finish, **not** invent Part mutation logic, v19, a second public command framework or a new persistent draft.
6. Existing `tests/pm03c2_cad_workbench_datum_plane_test.cpp` has registered same-tool Offset/Edge+Point/Command Line/Enter/Cancel/Undo coverage, and `tests/pm05f_r2_native_workbench_edit_test.cpp` has **real Windows Qt/OCCT** mouse/hover/shared END corner, MID-first, dependent Sketch+Axis, Edit and cold reopen. Extend only these existing registered targets with bounded S4B cases, retaining their original assertions. `tests/CMakeLists.txt` and all existing native-Qt `RESOURCE_LOCK` scheduling remain unchanged; S4B must not regress the existing 199/199 native suite or the known CI-04 build-vs-label behavior.

## 2. Fixed proposed S4B implementation allowlist — no writes until separate activation

**Workbench one-tool UX and transient input:**
- `src/ui/cad_workbench.cpp`
- `src/ui/cad_workbench.hpp`

**Current certified native END selection + reusable preview; only if necessary:**
- `src/ui/part_viewport_controller.cpp`
- `src/ui/part_viewport_controller.hpp`

**Existing Reference Geometry tree / visibility / edit actions:**
- `src/ui/part_document_tree_controller.cpp`

**Already registered tests, no new executable or CMake/CI change:**
- `tests/pm03c2_cad_workbench_datum_plane_test.cpp`
- `tests/pm05f_r2_native_workbench_edit_test.cpp`

**DOC-01 current-state internal, bilingual product and generated Browser:**
- `docs/internal/PART_DOCUMENTS.md`
- `docs/product/pl/PARTS.md`
- `docs/product/en/PARTS.md`
- `docs/browser/index.html` (**official `.\\ss2.ps1 docs` generated output only**)

**S4B native acceptance/evidence ledger:**
- `work/RG-02B1_D2_P4_S4B_THREE_END_POINTS_GUI_COMMAND_LINE_ACCEPTANCE_EVIDENCE.md`

**Everything else is OUT**, notably `src/part/**`, `src/kernel/**`, `src/kernel_occt/**`, `src/viewer/**`, `src/viewer_qt_occt/**`, `src/application/**`, any persistence schema or current v18 JSON grammar, `tests/CMakeLists.txt`, `.github/workflows/**`, `scripts/**`, frozen roadmaps/ADRs/Foundation, new toolbar command/tool, another test target, PM-06, unrelated Axis geometry, 3-Point MID or future general Point. If even one necessary path/API is missing, gather exact evidence and STOP for an Owner D2 amendment; do not silently widen scope.

## 3. Testable S4B acceptance matrix

- **S4B-F01 — one tool:** existing Offset / Edge+Point workflows PASS unchanged, constructor index 2 visibly `3 Points` under one existing Datum Plane Operations widget; Offset still default and no independent toolbar action. Command Line `DATUMPLANE`/optional alias enters the same tool; new constructor keywords and GUI selector produce the same transient state.
- **S4B-F02 — true three END picks:** genuine Windows Qt/OCCT pointer hover and clicks P1/P2/P3 on distinct current-Body material Edge END, preserving order and same stage. No separate Edge pick; END marker same as before, MID ignored in `3 Points`; body/scene generation and revision checked on every click.
- **S4B-F03 — identity/dedup:** three incident Edges of one corner yield **one** Point source, no Finish; unsupported/ambiguous/stale/foreign-stage/mixed-document picks remain disabled and typed, never silently choose provider order or screen XYZ.
- **S4B-F04 — geometric invalids + preview:** 0/1/2 points => no plane; 3 distinct collinear/near-collinear => no plane; 3 valid current Points => same S4A signed Frame3 and existing preview; Reverse flips V/N and twice restores; pointer leave clears hover marker while staged P points remain source intents; no last-good preview on invalidation.
- **S4B-F05 — Edit and explicit slots:** editing a real v18 3-Point Datum from Reference Geometry/Properties preloads P1/P2/P3 and reverse. Explicit selected P slot can be replaced with next certified END, one-step atomic Edit preserves `DatumId`, dependent Sketch/Axis/Feature identities and visibility; Undo/Redo consistent. Cancel/Esc/no-op invalid/revision diverged => zero mutated state, no extra Undo.
- **S4B-F06 — lifecycle/tree:** one Reference Geometry group includes three kinds; new Datum row, selection, warnings/visibility toggle, Delete dependency blocking and Properties/Edit action are consistent with existing two kinds. Save/Close/Reopen on independently reconstructed real OCCT Part v18 reproduces three source slots, frame, tree item and editable source intent, no Viewer token in file.
- **S4B-F07 — CAD Command Line + keyboard:** `3POINTS`/`OFFSET`/`EDGEPOINT` constructor switching, explicit `P1`/`P2`/`P3` replacement targeting, `REVERSE`, `FINISH`, `CANCEL`, owning Enter/Esc, focus restoration and command-context generation. Command Line is not a bypass for incomplete/unsupported mouse source, and existing Edge+Point MID-first and Offset precision-input grammar must remain intact.
- **S4B-F08 — real native positive/negative mouse:** registered `pm05f_r2.native_workbench_edit` extended with actual nonmocked Qt+OCCT END hover/pointer clicks, shared corner, Ctrl/mode switching where applicable, edit, cold replay and an explicit unsupported source; no test weakening. Original Q1/Q2/P0, S3A MID↔END, PM05F operations invariants remain green.
- **S4B-F09 — Windows evidence / documentation:** registered Windows FOCUSED desktop GUI + native Qt/OCCT; Draft FAST and genuine Ready FULL **Core+Kernel+Desktop**, all existing native Qt `RESOURCE_LOCK` cases executed; official DOC-01 internal + PL/EN Markdown plus regenerated Product Browser parity; exact code/test/docs HEAD with every historical RED recorded, no skipped test mistaken for PASS.
- **S4B-F10 — Owner manual GUI acceptance + gated merge:** demonstrate one existing Datum Plane tool, 3 END clicks, Reverse, Edit slot, Undo/Redo, Command Line, visibility, Save/Reopen on Windows for Owner. Owner **manual/technical FINAL PASS** of this S4B user-facing scope is separate from S4A headless PASS; guarded exact-head squash requires separate authorization and immutable evidence, not merely passing CI.

## 4. Execution gates — all distinct

1. **DONE:** accepted D2 design PR #389, native P0 source proof PR #392, headless S4A v18 PR #395 Owner technical FINAL PASS, guarded squash merged protected main `30bd70e8d17c90d9c8defd187e240ffb4f321c09`; Windows FULL #3166 **199/199**, closure #3169 **SUCCESS**. Do not reinterpret completed S4A as automatically activating GUI/S4B.
2. **PENDING OWNER D2:** approve/modify the **exact** S4B scope above — particularly the proposed Command Line mode tokens and fixed file/test allowlist. The earlier accepted high-level UX *does not itself* approve these new concrete implementation tokens, writable files or test obligations.
3. **PENDING governance-only:** this **one-work-file Draft proposal PR** must pass exact-head Windows governance CI; obtain separate Owner-provided authorization for its guarded protected-main squash; no `src/**` or test code changes on this branch.
4. **PENDING ACTIVE-only:** after contract merge, a separate, Owner-approved `work/ACTIVE.yaml`-only PR switches the sole active Work Contract from completed S4A to accepted S4B. Require Windows exact-head Ready closure and guarded protected-main squash. Until then no S4B implementation edits.
5. **PENDING production:** new branch **from activated main**; only §2 allowlist; actual Qt/OCCT FOCUSED→FAST→Ready FULL, DOC-01, test ledger; Owner Windows manual GUI/technical FINAL PASS; separately approved exact-head guarded production squash.
6. **STOP/ESCALATE D2:** Viewer cannot certify an intended END as one current strict S4A Point without a new kernel/provider/renderer API; common unsupported END family violates approved UX; source cannot be recomputed on the same revision; CAD input ownership requires global routing change; any schema/modeling-semantics version change or new file/test target; repeated red native Qt despite existing lock. Do not weaken tests or substitute mock/XYZ/ordinal source.

## Documentation impact

Internal docs: required

User/Product docs: required

Reason: users acquire an existing durable semantic v18 Datum type through a new constructor in an existing tool, with ordered END clicks, source-slot Edit, signed Reverse, status/visibility and Command Line semantics. Update *current-state* internal docs and PL/EN product guidance only after S4B code actually behaves as accepted, then regenerate `docs/browser/index.html` using the official generator. No new docs/UI capability claim in this governance proposal.
