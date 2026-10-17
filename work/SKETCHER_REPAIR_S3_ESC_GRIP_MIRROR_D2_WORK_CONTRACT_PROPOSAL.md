# Sketcher Repair S3 — Esc / Grip Copy / Mirror Keep Original — D2 Work Contract — OWNER-ACCEPTED SCOPE / GOVERNANCE DRAFT

**Status:** **OWNER-ACCEPTED EXACT S3 D2 §§0–9 + 26 CONDITIONAL PATHS** (2026-10-17) and explicitly authorized bounded future implementation, **subject to** integration and activation gates. Governance-only PR #487 **NOT MERGED, NOT ACTIVE; NO PRODUCTION MUTATION AUTHORITY YET**. The Owner has NOT separately authorized a guarded squash of #487 or an ACTIVE-only merge.
**Decision:** D2 cross-layer CAD Input / Shared 2D / Qt interaction **OWNER-ACCEPTED EXACT SCOPE**; D3 B (`C` + Enter) Owner-chosen independently. Later guarded governance/ACTIVE integration and product merge each retain independent gates.
**Baseline:** protected `main` = `6f8dc4f62452c749a53804ba79e661acd5a65251`, the verified Owner-authorized guarded squash of S2 implementation PR #483 (2026-10-17). Earlier B01/B02 S1 and B03a/B04 S2 are completed; don't re-open them.
**Program:** Owner-accepted `work/SKETCH_ROADMAP.md` v1.10, **S3 B03b/B05/B06**; companion Owner-accepted `work/PART_MODELING_V1_ROADMAP.md` v1.34 defers new RG-02 packages/PM-06.
**Authority:** `AGENTS.md` → frozen Constitution/Foundation/Architecture v1.0 → accepted ADR-0008 (Shared 2D), ADR-0009 (Sketch interaction), ADR-0011 (global CAD Input/focus), ADR-0012 (revision-bound transactions) → `work/ACTIVE.yaml` → program roadmap → future accepted/integrated/activated S3 contract → DOC-01.
**Current `ACTIVE.yaml` on `main`:** still points to `work/SKETCHER_REPAIR_S2_POINT_REQUEST_OSNAP_GRIP_D2_WORK_CONTRACT_PROPOSAL.md`. S2 product merge **does not** change that pointer or authorize S3. This PR proposes ONLY the S3 contract file, no `ACTIVE.yaml` edit.

## 0. Governance decision and D3 fail-closed boundary

S0/S1 → S2 → S3 scheduling had only given direction. The Owner subsequently selected **D3 B** (`C` + Enter) and, when asked explicitly whether to approve **the exact S3 D2 contract §§0–9 and all 26 conditional paths**, answered **`akceptuje i autoryzuje implementacje  - kontynuuj pracę`** on 2026-10-17. This **accepts the entire exact D2 scope and grants future bounded implementation authority** but does not waive the separate governance PR squash, separate ACTIVE-only acceptance/merge and separate product PR merge gates. Historic D3 decision: `Wybieram D3 B dla skrótu Grip Copy w proponowanym kontrakcie S3 PR #487. Kontynuuj procedurę zatwierdzania D2.`

**D3 shortcut decision (§5): CLOSED — Owner chose B (`C` + Enter).** One-key immediate `C` is excluded. **D2 approval: SATISFIED.** Implementation remains gated by independent Owner-authorized guarded integration of this contract and separate ACTIVE-only integration on protected `main`; do not change source, tests, product docs or ACTIVE meanwhile.

**Protected integration sequence:** (1) **DONE: Owner accepted exact S3 D2 §§0–9 and all 26 conditional paths plus D3 B**; (2) run Windows exact-HEAD CLOSURE on this acceptance update; (3) obtain separate explicit Owner permission for guarded squash merge of governance-only PR #487 and verify `main`; (4) create a distinct ACTIVE-only PR switching only `active_work`, with independent exact-head Windows CLOSURE and separate explicit Owner acceptance/guarded merge permission; (5) only after verifying protected `main` activation begin already Owner-authorized test-first S3 source/test/docs changes within the conditional 26-path scope; (6) each product PR still requires focused/FAST/Ready FULL, DOC-01, real Owner manual PASS, independent technical FINAL PASS and separate guarded product merge authorization. No direct protected-main push.

## 1. Current-main audit: evidence, not an asserted current Windows RED

The accepted roadmap calls S3 **B03b: Qt end-to-end Esc routing; B05: Grip Rotate/Scale/Mirror + Copy; B06: normal Mirror Keep Original/Copy**. Existing code already implements some related semantics; it is incorrect to pretend the whole feature family is absent.

- `src/ui/project_workspace_shell.cpp` owns an app-level event filter and the workspace CAD Input buffer. Focused Command Line Esc clears the live token and, when empty outside special PROJECT context, moves focus back to Workbench; active viewport Esc falls through to the Workbench. These are **different key-event paths**, a plausible B03b gap to verify with real Qt focus and active Sketch stages, not yet a proven current-main failure.
- `src/ui/cad_workbench.cpp` dispatches viewport Esc to `PartSketchInteractionController::escape()`; `src/sketch/interaction_state.cpp` layers R11 transient request state → numeric locks → grip/tool cancellation → Select selection clearing. Baseline regressions constrain any change.
- `src/application/cad_input_semantics.cpp` currently recognizes a submitted Grip Copy `C` token inside an eligible point request. The workspace keyboard transport normally accumulates printable `C` and submits on Enter. **One-key C is not established as current authoritative behavior.**
- `SketchInteractionState::enableDirectManipulationCopy()` currently accepts **Reshape and Move only**, while Rotate/Scale/Mirror and their numeric modes already exist; Copy mode resets on Space. Normal Mirror currently uses the two-point axis and authors a replacement, while normal repeated COPY and Grip Reshape/Move Copy already duplicate via fresh IDs. These are existing controls, not license to simply reuse stale/session geometry after a mode switch.
- In scope of audit: `tests/wb02_global_cad_input_ui_test.cpp`, `tests/d_cad_input_semantics_test.cpp`, `tests/sk07e_space_cycle_edit_mode_test.cpp`, `tests/sk07b_transform_controller_test.cpp`, `tests/sk07c_copy_controller_test.cpp`, `tests/sk07f_precision_input_controller_test.cpp`. A test that passes in a semantic mock is **not** an actual Qt focus/viewport reproduction.

**S3-0 required:** before product edits, re-run registered current-HEAD tests and add real failing Windows Qt/Workbench/Viewport tests for demonstrable user-visible defects, with exact SHA, observed failure, expected semantics and evidence ledger. A currently passing case is PASS/GAP, not invented RED; don't skip/XFAIL existing registered tests or modify CMake to hide failures.

## 2. B03b — Esc, input buffer, focus and request hierarchy

**Target:** one deterministic semantic cancel chain for ordinary viewport, focused Command Line, grip manipulation, tool-local numeric value/PointRequest locks and modal/text-editor ownership, without bypassing the existing shared CAD Input router.

Required contract when an active Sketch/Workbench owns CAD input:

1. If a *live uncommitted text token* exists, the first Esc only clears that token. No authored mutation, no silent cancellation of the active Sketch operation. If no token remains but request-local numeric locks exist, next Esc clears the applicable locks by established R10/R11 order; only subsequent Esc cancels the pending grip/transform/tool stage; further Esc in Select clears selection. Honor one-shot OSNAP/OTRACK/Polar cleanup in the existing interaction state.
2. Apply equivalent user-visible semantics when the **real Qt Command Line is focused** and when the **real viewport is focused**, without sending duplicate Esc, unexpectedly moving focus to a hidden endpoint, swallowing an Esc necessary to cancel a live request, or canceling more than one layer per keypress.
3. Ordinary real QLineEdit/text/property editors, combo-box edit subcontrols, popups, modal dialogs, inactive background workspaces and other Part/Datum/Project tools keep their own input/cancel ownership. No global intercept of Ctrl+S/Ctrl+Z/Ctrl+Y, Space, Tab, F10/F12 or printable text outside the authoritative active CAD context.
4. The active Workbench only invokes an accepted Sketch semantic request via `ICadInputEndpoint`/`PartSketchInteractionController`; never create a second global Sketch state or transport domain model into Qt. The request and source snapshot remain revision-guarded; Esc cannot allocate IDs, dirty state or history.
5. All active contexts remain safe across focus loss/re-entry, tool switching, Document close/reopen, Undo during a preview, stale DocumentRevision and an empty/invalid numeric buffer. No destructive double cancellation.

## 3. B05 — Grip Rotate/Scale/Mirror with Copy

**Target:** extend the existing *orthogonal* Grip Copy modifier beyond Reshape/Move to explicitly approved Rotate, positive uniform Scale and Mirror, using the one shared semantic transform and normal duplication Command.

1. Source and affected set: the **frozen selected geometry** for Grip Move/Rotate/Scale/Mirror; only the owner geometry for Reshape+Copy. The active grip remains the original immutable pivot, and current numeric angle/factor/axis-angle grammar is unchanged. Each preview always derives from that frozen snapshot, not a previously created copy. Source entities remain unchanged; new copies get fresh IDs and current authored roles.
2. Mode sequence remains exactly `Reshape → Move → Rotate → Scale → Mirror → Reshape` (center: `Move → Rotate → Scale → Mirror → Move`). Switching mode turns Copy OFF. Copy must be re-armed *explicitly* in each supported mode after that mode is active. Do not preserve stale axis/reference, lock, input or OSNAP request when cycling.
3. In Rotate+Copy, use the original grip as pivot and signed Sketch angle; Scale+Copy requires finite factor **greater than zero**, and mirror negative-scale masquerade is prohibited; Mirror+Copy uses the active grip as axis start, and an exact resolved second axis point or typed Axis Angle. Reject degenerate mirror axes, invalid transformations and unchanged placements without authoring phantom history.
4. Each accepted placement duplicates the full supported affected set atomically through the existing revision-bound semantic `DuplicateSketchGeometryCommand` (or a specifically justified in-allowlist existing equivalent), not `UpdateSketchGeometryCommand`. Original EntityIds unchanged; copies fresh, no partial geometry, one Undo entry per placement. Undo/Redo and Save/Close/Reopen must preserve roles/identities and copied outputs.
5. Repeated Copy, when available by accepted grammar, remains active until Esc; new placements must remain based on frozen original geometry, not compound. Stale revision and source deletion fail closed. Clarify input stage and repeated placement after typed versus pointer first copy with Windows evidence; do not assume an unsupported copy session.

## 4. B06 — ordinary Mirror: Keep Original / Copy

**Target:** user explicitly chooses whether normal two-point-axis MIRROR replaces existing geometry or preserves originals and creates mirrored duplicates, using the same shared transform core.

1. Keep Original/Copy **OFF by default**, preserving the current normal Mirror-replaces-original behavior. When ON, normal Mirror must preserve selected originals/roles/IDs, author mirrored duplicates with **fresh EntityIds**, one validated semantic transaction and one Undo step per accepted commit, and retain the original selected set.
2. For selection-first and command-first entry, preserve the existing `Select objects → Axis Start → Axis End` grammar, real Qt/Command Line precision and focus behavior; do not invent a third-point UI or implicit request stage. Keep Original/Copy is an operation-local *runtime* option, not persisted in the Part document or a project-wide preference unless separately approved.
3. Mirror preview uses the original snapshot regardless of Copy toggle; invalid/coincident axis, stale source, mixed selected roles, unsupported nonfinite coordinates or failed projection produce no changed state. A no-op mirror cannot silently duplicate identical geometry without a separately accepted rule. Do not silently turn Mirror+Copy into infinitely repeated placements when ordinary Mirror is one-shot.
4. Toggling normal Mirror Keep Original/Copy must never set or leak Grip Copy, global COPY, or different tools' options; cancellation and after-commit reset are deterministic. If a new visible Operations toggle and label are necessary, keep within existing Sketch/Workbench UI ownership; no generic global CAD toolbar redesign.

## 5. **Owner D3 decision CLOSED: B — `C` + Enter**

**Exact Owner decision (2026-10-17):** `Wybieram D3 B dla skrótu Grip Copy w proponowanym kontrakcie S3 PR #487. Kontynuuj procedurę zatwierdzania D2.` This is **only** a D3 choice. It is **not** Owner acceptance of D2 §§0–9, the 26 conditional file paths, merge of governance PR #487, the ACTIVE-only transition, or product code mutation.

**Selected B — `C` followed by Enter:** In a live eligible Grip stage, a plain `C` is ordinary printable text in the **one shared workspace CAD Input buffer**. Only pressing Enter submits that token to the existing semantic endpoint and can arm Grip Copy. There is **no new one-key `C` interceptor**. This rule also governs future Grip Rotate/Scale/Mirror + Copy; it does not mean those modes are already implemented. Existing explicit Operations UI activation remains permitted only where available.

**Required precedence and isolation:** A lone `C` must not arm Copy, author geometry, dirty the document, allocate IDs or add Undo. Esc with the pending `C` first clears that live text token while preserving the active Grip and its Copy state; later Esc follows the existing numeric-lock and stage hierarchy. Real text editors retain their own literal `C`. Submitted `C` outside a currently eligible Grip must not silently arm Copy, change numeric mode or launch another tool. Other tokens, complete point/angle/factor inputs, one-shot OSNAP and mode-switch resets keep their established semantic priority. After Space cycles edit mode, Copy stays OFF until separately re-armed through a new `C` + Enter submission.

**Not selected A — one-key immediate `C`:** pressing `C` without Enter must **never** arm Copy within this S3 contract. A later change to one-key `C` requires a new explicit Owner D3 decision and appropriate D2 amendment.

**Governance:** D3 B and exact D2 §§0–9 including 26 conditional paths are **Owner-accepted**. Future bounded implementation was explicitly authorized **contingent on** contract integration and separate ACTIVE-only activation. No permission for guarded squash of PR #487, for merging ACTIVE-only or future product PR is implied by that statement.

## 6. Registered test-first Windows matrix and hard acceptance

**B03b:** real `QTest` keypresses on active Viewport, focused global Command Line, ordinary editor, inactive secondary Workspace and popup, with staged Sketch Line/Circle/Arc, ordinary Mirror axis, grip Rotate numeric lock and dynamic field; two/three/four Esc hierarchies as actually necessary. For **D3 B**, press `C` alone and verify it stays a buffer token with no Copy enablement, then Enter arms only in eligible Grip mode; Esc clears pending `C` before canceling the stage; an ordinary text editor retains `C` under local ownership. Assert exact active stage/selection/input buffer, preview clearing, persisted revision and Undo depth. Regression Ctrl+Z/Y, Tab, Space mode cycle and R11 one-shot override.

**B05:** Line Start/End/center + Circle/Arc grips; mixed selection, each Rotate/Scale/Mirror with Copy **armed by `C` + Enter**, numeric angle/factor/mirror axis, pointer previews, explicit modes, re-arming after Space, repeated copy with frozen source, fresh IDs, source still selected, cross-role source geometry, invalid/zero/negative factor and zero-length axis, stale revision, per-placement Undo/Redo and cold Save/Reopen. Retain preexisting Grip Reshape+Copy/Move+Copy, R10, R11 and S2 TAN/PER regression semantics.

**B06:** normal Mirror selected-first and command-first, Keep Original OFF and ON, empty/partial/mixed selection, two-point axis, point/angle precision, focus/Esc cleanup, preview vs commit and invalid-axis fail-closed; assert original IDs unaffected when ON, mirrored entities fresh, one undo/redo and cold .ss2part roundtrip. Preserve standard normal MOVE/COPY/ROTATE/SCALE, prior S1 Profile/Extrude and completed S2.

**Windows gates:** exact-baseline registered audit → genuine Qt/semantic RED for demonstrated gaps → minimal product fixes → focused registered Qt/Core → Draft FAST → Ready FULL (Core, Kernel-native, Qt/Desktop), docs+Browser freshness → evidence-only exact-head Windows CLOSURE → Owner real Windows GUI PASS (Esc, all three Grip Copy modes, normal Mirror ON/OFF and history/persistence) → independent technical FINAL PASS → separate Owner guarded-squash permission for product PR(s). No blanket “PASS” transferred between SHA or from a mock-only test.

## 7. Narrow CONDITIONAL 26-path product/test/docs allowlist — OWNER-ACCEPTED, NOT ACTIVE

All 26 paths below are **Owner-accepted conditional bounds** for future activated S3 implementation. They are **not active for edits yet** and may be touched only after independently authorized guarded integration of this contract and independently approved/merged ACTIVE-only switch; each edit must be required for in-scope S3 work. An essential path outside the list requires new Owner-approved D2 addendum. The allowlist does not authorize unrelated refactoring.

1. `src/ui/project_workspace_shell.cpp` — actual Qt focus/filter Esc plus preservation of D3 B `C`-as-token-until-Enter; no one-key `C` interceptor.
2. `src/ui/project_workspace_shell.hpp` — private scoped routing state/interface if unavoidable.
3. `src/ui/cad_workbench.cpp` — actual viewport Esc / active Sketch keyboard and Mirror Operations UI only.
4. `src/ui/cad_workbench.hpp` — private UI controls/callback declarations only.
5. `src/ui/part_sketch_interaction_controller.cpp` — single semantic Esc and Grip/normal Mirror Copy command routing, preview/commit.
6. `src/ui/part_sketch_interaction_controller.hpp` — private operation-local S3 options if essential; **TWO narrow D2 exceptions OWNER-ACCEPTED (§§10–11), INACTIVE UNTIL GOVERNANCE INTEGRATION:** (§10) edit only the body of existing inline `cadInputSemanticGripCopyAvailable() const noexcept` for live valid Grip modes with D3 B `C` + Enter, without changing signature/API; (§11) add only the normal-Mirror operation-local public setter `setMirrorKeepOriginal(bool)` and read-only getter `mirrorKeepOriginal() const noexcept`, backed by one private transient bool flag for existing B06 Operations UI. No other new public controller API, header refactor, global state or durable schema change.
7. `src/sketch/interaction_state.cpp` — transient Copy mode/transform stage and fail-closed lifecycle semantics.
8. `src/sketch/include/simplesolid2/sketch/interaction_state.hpp` — only transient nonpersistent S3 state; no durable schema or public architecture change.
9. `src/application/cad_input.cpp` — existing one-buffer input lifecycle if proven necessary for B03b; no unrelated grammar rewrite.
10. `src/application/cad_input_semantics.cpp` — existing `C` submission and Mirror operation-local input semantics only; no global parser redesign.
11. `tests/wb02_global_cad_input_ui_test.cpp` — real app-level focus/Qt Esc input test.
12. `tests/d_cad_input_semantics_test.cpp` — single semantic endpoint command/keyword precedence.
13. `tests/cad_workbench_test.cpp` — actual Workbench key/Operations UI regression if fixture supports it.
14. `tests/sk04c_part_sketch_interaction_controller_test.cpp` — true pointer/Qt controller cancel/commit behavior.
15. `tests/sk07e_space_cycle_edit_mode_test.cpp` — frozen pivot, Copy cycling/expiry, mode tests.
16. `tests/sk07f_precision_input_controller_test.cpp` — real endpoint keyboard, typed angles, precision locks and copy.
17. `tests/sk07b_transform_controller_test.cpp` — Mirror/Rotate/Scale command lifecycle and identity.
18. `tests/sk07b_common_transform_state_test.cpp` — existing transform stage and Mirror two-point request.
19. `tests/sk07c_copy_controller_test.cpp` — duplicate IDs/selection, repeated Copy and history.
20. `docs/internal/SHARED_2D.md` — current-state shared semantic transform/copy, identity and Esc.
21. `docs/internal/CAD_WORKBENCH_VIEWER.md` — Qt/Workbench input focus and preview/commit.
22. `docs/internal/UI_PROJECT_HUB.md` — only if workspace global Command Line/Esc behavior changes.
23. `docs/product/pl/PARTS.md` — Polish Grip and Mirror user workflow, exact accepted C behavior.
24. `docs/product/en/PARTS.md` — English product parity with PL.
25. `docs/browser/index.html` — **generated only with `.\ss2.ps1 docs`** from canonical Markdown; never independent hand-edit.
26. `work/SKETCHER_REPAIR_S3_ESC_GRIP_MIRROR_ACCEPTANCE_EVIDENCE.md` — new factual RED/PASS/FAIL/GAP and Owner decision ledger.

**Explicit exclusions:** `work/ACTIVE.yaml` (separate activation PR); `work/SKETCH_ROADMAP.md` and accepted ADR/Foundation (frozen); `src/part/**`, OCCT provider, Part persistence/native schema, `src/viewer/**`, new CMake or test targets, global Command Line API redesign, other `src/application/**`, layout/theme redesign, R13 constraints/solver, S0/S1/S2 code/geometry redesign, RG-02/PM-06 and all unlisted tests. A needed provider, existing CAD Input header, or new public semantic command/API requires Owner D2 review/amendment, not an improvisational change.

## 8. Non-negotiable gating and stopping conditions

- No product edit under this proposal; no `ACTIVE.yaml` change and no protected-main write except a separately Owner-authorized guarded PR squash.
- No automatic activation when the S3 governance proposal merges; activate only via separate **one-file ACTIVE-only PR** with Owner permission and an exact-head Ready CLOSURE.
- If Qt/Workbench repair requires crossing into Part/Project global input ownership beyond exact listed files, stop and present evidence, alternatives and path request.
- No silent reinterpretation of S1/S2 geometry, deferred R11 OSNAP, numeric locks, existing command grammar or authored identity. Preserve stale-revision failure, selection, Undo/Redo, Save/Reopen and no ghost IDs.
- **D3 B AND D2 SCOPE SETTLED:** Owner chose `C` + Enter and approved exact §§0–9 and 26 conditional paths. One-key `C` is excluded. Governance PR squash, ACTIVE-only PR approval/merge and any product PR merge still need separate explicit Owner permission.
- Do not claim technical FINAL PASS based on Windows CI alone; actual Owner manual GUI validation is separately required.

## 9. DOC-01 — Documentation Impact

**Internal as-built docs: REQUIRED** for Qt focus/cancel ownership and shared Sketch Copy/transform behavior. **PL/EN Product docs: REQUIRED** for Grip Rotate/Scale/Mirror Copy, normal Mirror Keep Original and precisely accepted `C` keyboard grammar. **Generated Browser: REQUIRED** by repository `.\ss2.ps1 docs`; freshness must be verified in the actual Windows build. This contract review is governance-only: no current-state doc changes until product behavior is genuinely implemented and verified.

**Next Owner decision (independent guarded merge gate):** Once this acceptance-evidence HEAD passes Windows exact-HEAD CLOSURE, request explicit guarded squash merge authorization for **governance-only PR #487**. Then independently propose and obtain exact Owner authorization for a separate ACTIVE-only PR. Already approved S3 implementation may start only after that ACTIVE-only merge. This approval is not product merge authority.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: S3 B03b changes Qt/Workbench CAD Input focus and hierarchical Esc handling; B05 extends Grip Rotate/Scale/Mirror with Copy; B06 adds normal Mirror Keep Original/Copy. Keep current-state shared 2D and Workbench internals and the Polish/English Parts user instructions synchronized and regenerate the DOC-01 Product Browser before completion. This is the machine-readable DOC-01 declaration for the already Owner-accepted §9; it adds no new product behavior, scope, path or merge/activation authority.

## 10. D2 OWNER-ACCEPTED ADDENDUM — inline existing Grip Copy availability predicate (INTEGRATION PENDING)

**Status: OWNER-ACCEPTED EXACT §10 / INTEGRATION PENDING / NO SOURCE HEADER EDIT YET.** The Owner explicitly responded `akceptuje - kontynuuj` to the focused request to approve **S3 D2 §§10–11 and amended §7 item 6** in PR #491 (2026-10-18). The accepted change is limited to this existing predicate body; the governance PR still requires an independent guarded-squash authorization, successful exact-HEAD Ready CLOSURE and verification on protected `main` before the product PR may modify that header. Original S3 §§0–9, D3 B and 26 paths remain unchanged.

**Test-first finding:** On initial S3 B05 test-only SHA `f7863e4b76d16e16e134be5f4036871ec173891a`, Windows Ready FULL #3619 reproduced `sk07e.space_cycle_edit_mode` RED at line 238 (`state.enableDirectManipulationCopy()`), with remaining 116/117 Qt/Desktop PASS, Core 25/25 and Kernel 57/57. In the live controller `PartSketchInteractionController::cadInputSemanticGripCopyAvailable()` is an existing **public inline override** that advertises only `reshape`/`move` as eligible despite the now accepted S3 B05 requirement to support `rotate`/`scale`/`mirror` with submitted `C` + Enter. The approved §7 item 6 permitted only private operation-local options in this header. Altering this public inline predicate exceeds that precise purpose even though the path itself is in the 26-path list.

**Requested minimal exception:** permit editing **only the existing body** of `cadInputSemanticGripCopyAvailable() const noexcept` inside already listed `src/ui/part_sketch_interaction_controller.hpp` to return availability for valid active `reshape`, `move`, `rotate`, `scale` and `mirror` grip states. Preserve signature, override, const/noexcept, visibility, dependency direction, semantic endpoint grammar and compiled interface. Do **not** add any new public API, broaden availability outside live Grip, intercept one-key `C` or modify unrelated controller-header code. Update tests in existing §7 paths to verify that `C` alone merely populates the shared buffer, `C`+Enter arms Copy only on eligible live Grip, Space turns Copy OFF, and normal transforms reject Grip Copy.

**Fail-closed while not integrated:** although §10 is now accepted, the product PR must not change this inline predicate before separate guarded integration of this governance addendum. No bypass of the availability query or global CAD Input grammar to evade this boundary.

**Separate gate:** this Owner acceptance settles only exact D2 §10, together with §11 and amended §7 item 6; it does **not** authorize governance PR #491 squash or product PR #490 merge. The governance PR requires its own explicit guarded-squash authorization; after integration, product PR #490 must rebase against protected main and pass exact-head Windows tests. S3 remains active; no ACTIVE-only switch needed.

**Documentation impact:** Existing §9 plus the canonical `## Documentation impact` declaration remain unchanged; this addendum only clarifies authorization for an existing inline predicate. Product docs follow the genuinely implemented behavior, not this proposal.

## 11. D2 OWNER-ACCEPTED ADDENDUM — B06 normal Mirror Keep Original operation-local UI bridge (INTEGRATION PENDING)

**Status: OWNER-ACCEPTED EXACT §11 / INTEGRATION PENDING / NO PUBLIC-HEADER EDIT YET.** The same explicit Owner `akceptuje - kontynuuj` response also accepts this specific two-method ordinary-Mirror UI bridge and one transient flag, with the §7 path-6 restriction rewritten above. This approval is **not** a protected-main merge or immediate permission to edit the product controller header: independently merge and verify governance PR #491 first. S3 §4 outcome, D3 B grammar, 26-path scope, DOC-01 and protected-main activation remain otherwise unchanged.

**Necessity and exact interface:** B06 specifies a user-visible ordinary MIRROR Keep Original/Copy toggle, OFF by default, with a single runtime source of truth. Existing `PartSketchInteractionController` public interface has `activateMirror()` and `commitTransform()` but no way for approved `CadWorkbench` Operations UI (§7 paths 3–4) to set/query the normal Mirror operation's copy choice. Permit **only** two tiny public operation-local members: `bool setMirrorKeepOriginal(bool enabled);` (fail closed outside an active ordinary-Mirror stage and never arm Grip Copy) and `bool mirrorKeepOriginal() const noexcept;` (read-only active ordinary-Mirror option), plus a single private transient flag such as `bool mirror_keep_original_{};`. No persistent Part preference, no global CAD Input state, no new command types or virtual API, no new files, no public redesign or refactor in this header.

**Implementation constraints in already approved `.cpp` / UI paths:** `activateMirror()` resets the option OFF for every new ordinary Mirror session; only user selection in existing Sketch Operations UI switches it ON. In `commitTransform()` for ordinary Mirror only, OFF retains current `UpdateSketchGeometryCommand` behavior; ON validates captured original selection, immutable frozen transformed geometry and current `DocumentRevision`, rejects identical/no-op and invalid axes, then authors one `DuplicateSketchGeometryCommand` transaction with fresh EntityIds, original selection/roles unchanged, one Undo entry. After success/failure/cancel/tool switch/Undo/history close, the option cannot leak into Grip Copy, normal COPY or a new Mirror session. The existing one-shot `Axis Start → Axis End` grammar must not become repeated placement. No preview recompute from newly duplicated geometry.

**Test obligations using existing §7 files:** assert OFF unchanged, ON creates only mirrored duplicates with source still intact, new IDs/roles, mixed Line/Circle/Arc selection, selection-first and command-first, invalid/coincident axis/no-op, stale revision, Esc focus, Undo/Redo and cold .ss2part reopen. Confirm Qt Operations UI accurately shows its current Mirror-only ON/OFF status and never enables Grip Copy. Keep D3 B `C`+Enter strictly for Grip, not a hidden normal-Mirror shortcut. Mark pending tests RED before narrow runtime implementation, and run real Windows FOCUSED/FAST/Ready FULL and DOC-01 PL/EN+Browser.

**Separate Owner gate:** exact §11 and §10, plus amended §7 path 6, are now Owner-accepted. **Next required decision is separate explicit guarded squash merge authorization** for governance PR #491 following a new exact-head Windows closure and fresh main/PR scope preflight. Only after verified integration may product PR #490 edit the narrowly approved header, add tests, run required Windows FOCUSED/FAST/Ready FULL and DOC-01 gates, and seek later Owner manual/technical/product merge approvals. No product merge consent is implied.


### Owner decision record — exact D2 addenda, not merge authority

The Owner replied **`akceptuje - kontynuuj`** directly to: `Czy akceptujesz dokładne addenda D2 §§10–11 i zaktualizowany punkt 6 allowlisty w PR #491?`. Thus **§10 (existing public-inline Grip Copy availability body), §11 (two normal-Mirror operation-local methods plus one transient flag), and the amended §7 item 6 are accepted**. This record does not claim Owner has authorized governance PR #491 merge, production header mutation before main integration, product PR #490 merge, or final acceptance of S3.
