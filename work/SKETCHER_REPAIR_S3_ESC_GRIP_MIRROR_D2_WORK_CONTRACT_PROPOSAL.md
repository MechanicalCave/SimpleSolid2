# Sketcher Repair S3 — Esc / Grip Copy / Mirror Keep Original — D2 Work Contract PROPOSAL

**Status:** PROPOSED, **NOT OWNER-ACCEPTED, NOT ACTIVE**, governance-only; S3 source/test/docs mutation expressly FORBIDDEN until separate exact Owner D2 approval, guarded governance integration and ACTIVE-only activation.
**Decision:** D2 cross-layer CAD Input / Shared 2D / Qt interaction; **D3 shortcut/product grammar OPEN** (`5).
**Baseline:** protected `main` = `6f8dc4f62452c749a53804ba79e661acd5a65251`, the verified Owner-authorized guarded squash of S2 implementation PR #483 (2026-10-17). Earlier B01/B02 S1 and B03a/B04 S2 are completed; don't re-open them.
**Program:** Owner-accepted `work/SKETCH_ROADMAP.md` v1.10, **S3 B03b/B05/B06**; companion Owner-accepted `work/PART_MODELING_V1_ROADMAP.md` v1.34 defers new RG-02 packages/PM-06.
**Authority:** `AGENTS.md` → frozen Constitution/Foundation/Architecture v1.0 → accepted ADR-0008 (Shared 2D), ADR-0009 (Sketch interaction), ADR-0011 (global CAD Input/focus), ADR-0012 (revision-bound transactions) → `work/ACTIVE.yaml` → program roadmap → future accepted/integrated/activated S3 contract → DOC-01.
**Current `ACTIVE.yaml` on `main`:** still points to `work/SKETCHER_REPAIR_S2_POINT_REQUEST_OSNAP_GRIP_D2_WORK_CONTRACT_PROPOSAL.md`. S2 product merge **does not** change that pointer or authorize S3. This PR proposes ONLY the S3 contract file, no `ACTIVE.yaml` edit.

## 0. Governance decision and D3 fail-closed boundary

S0/S1 → S2 → S3 scheduling is already an Owner-accepted **direction**, not authorization to implement S3. This exact S3 contract, every conditional source/test/doc path below, and the unresolved one-key `C` versus `C+Enter` grammar require **separate explicit Owner acceptance**. The Owner's `ok, kontynuuj pracę` after S2 integration authorizes preparing this review proposal, NOT accepting its terms, merging it, changing `ACTIVE.yaml` or starting product code.

**D3 STOP (`5):** No shortcut-policy or focus interception change is permitted until Owner explicitly chooses the behavior. If the product decision cannot be reached, isolate an independent S3 sub-scope in a newly accepted contract and do not silently ship a partial product feature under this proposal.

**Protected integration sequence:** (1) Owner approves exact D2 ``0–9, paths and D3 outcome; (2) governance proposal runs independent exact-HEAD Windows CLOSURE; (3) Owner separately authorizes guarded squash of this governance-only PR; (4) verify contract on protected `main`; (5) a **distinct ACTIVE-only PR** switches only `active_work` to this contract, passes its own exact-head Ready CLOSURE and receives separate Owner guarded-squash authority; (6) only after verified `main` activation begin test-first production PR(s). Each product PR later needs focused/FAST/Ready FULL, DOC-01, manual PASS, independent technical FINAL PASS and separate guarded product merge authorization. Never direct-write protected `main`.

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

## 5. **Owner D3 decision REQUIRED: Grip Copy shortcut C**

Owner must select one exact product rule before S3 D2 is considered settled.

**Option A — one-key `C`**: with a *live eligible Grip* mode and an empty shared CAD input token, no modal/text editor and no modifiers, pressing `C` immediately turns on Grip Copy without waiting for Enter. It must not generate a visible lone CAD token, conflict with Circle/other commands, or override the currently typed field/number; repeated/cycled modes and one-shot request consumption remain tested. This requires a guarded, tightly context-qualified cross-layer key-handling change.

**Option B — `C` then Enter (existing safe keyboard-first grammar)**: `C` is a printable text token in the one global CAD Input buffer, Enter submits it through the existing semantic endpoint only in a supported active Grip stage; no new one-key shortcut is intercepted. Grip Copy after Rotate/Scale/Mirror uses the same submitted-token grammar. Preserve direct UI Operations activation where available. This is the lowest-change option.

**Proposed conservative default for review:** Option B; **NOT accepted** unless Owner explicitly chooses B. If Owner chooses A, scope/test cases below require targeted amendment for immediate-key interception before product implementation. The D2 contract must not infer this D3 selection from `kontynuuj`.

## 6. Registered test-first Windows matrix and hard acceptance

**B03b:** real `QTest` keypresses on active Viewport, focused global Command Line, ordinary editor, inactive secondary Workspace and popup, with staged Sketch Line/Circle/Arc, ordinary Mirror axis, grip Rotate numeric lock and dynamic field; two/three/four Esc hierarchies as actually necessary. Assert exact active stage/selection/input buffer, preview clearing, persisted revision and Undo depth. Regression Ctrl+Z/Y, Tab, Space mode cycle and R11 one-shot override.

**B05:** Line Start/End/center + Circle/Arc grips; mixed selection, each Rotate/Scale/Mirror with Copy, numeric angle/factor/mirror axis, pointer previews, explicit modes, re-arming after Space, repeated copy with frozen source, fresh IDs, source still selected, cross-role source geometry, invalid/zero/negative factor and zero-length axis, stale revision, per-placement Undo/Redo and cold Save/Reopen. Retain preexisting Grip Reshape+Copy/Move+Copy, R10, R11 and S2 TAN/PER regression semantics.

**B06:** normal Mirror selected-first and command-first, Keep Original OFF and ON, empty/partial/mixed selection, two-point axis, point/angle precision, focus/Esc cleanup, preview vs commit and invalid-axis fail-closed; assert original IDs unaffected when ON, mirrored entities fresh, one undo/redo and cold .ss2part roundtrip. Preserve standard normal MOVE/COPY/ROTATE/SCALE, prior S1 Profile/Extrude and completed S2.

**Windows gates:** exact-baseline registered audit → genuine Qt/semantic RED for demonstrated gaps → minimal product fixes → focused registered Qt/Core → Draft FAST → Ready FULL (Core, Kernel-native, Qt/Desktop), docs+Browser freshness → evidence-only exact-head Windows CLOSURE → Owner real Windows GUI PASS (Esc, all three Grip Copy modes, normal Mirror ON/OFF and history/persistence) → independent technical FINAL PASS → separate Owner guarded-squash permission for product PR(s). No blanket “PASS” transferred between SHA or from a mock-only test.

## 7. Narrow CONDITIONAL 26-path product/test/docs allowlist — PROPOSAL ONLY

All items are optional, conditioned on **future exact Owner approval** and proved need for S3; this governance PR changes none of them. If an essential file is outside the list, STOP for separate Owner D2 path addendum before editing. Listing does not authorize broad refactors inside a file.

1. `src/ui/project_workspace_shell.cpp` — actual Qt focus/filter Esc + selected D3 C grammar only.
2. `src/ui/project_workspace_shell.hpp` — private scoped routing state/interface if unavoidable.
3. `src/ui/cad_workbench.cpp` — actual viewport Esc / active Sketch keyboard and Mirror Operations UI only.
4. `src/ui/cad_workbench.hpp` — private UI controls/callback declarations only.
5. `src/ui/part_sketch_interaction_controller.cpp` — single semantic Esc and Grip/normal Mirror Copy command routing, preview/commit.
6. `src/ui/part_sketch_interaction_controller.hpp` — only private operation-local options if essential.
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
25. `docs/browser/index.html` — **generated only with `.\\ss2.ps1 docs`** from canonical Markdown; never independent hand-edit.
26. `work/SKETCHER_REPAIR_S3_ESC_GRIP_MIRROR_ACCEPTANCE_EVIDENCE.md` — new factual RED/PASS/FAIL/GAP and Owner decision ledger.

**Explicit exclusions:** `work/ACTIVE.yaml` (separate activation PR); `work/SKETCH_ROADMAP.md` and accepted ADR/Foundation (frozen); `src/part/**`, OCCT provider, Part persistence/native schema, `src/viewer/**`, new CMake or test targets, global Command Line API redesign, other `src/application/**`, layout/theme redesign, R13 constraints/solver, S0/S1/S2 code/geometry redesign, RG-02/PM-06 and all unlisted tests. A needed provider, existing CAD Input header, or new public semantic command/API requires Owner D2 review/amendment, not an improvisational change.

## 8. Non-negotiable gating and stopping conditions

- No product edit under this proposal; no `ACTIVE.yaml` change and no protected-main write except a separately Owner-authorized guarded PR squash.
- No automatic activation when the S3 governance proposal merges; activate only via separate **one-file ACTIVE-only PR** with Owner permission and an exact-head Ready CLOSURE.
- If Qt/Workbench repair requires crossing into Part/Project global input ownership beyond exact listed files, stop and present evidence, alternatives and path request.
- No silent reinterpretation of S1/S2 geometry, deferred R11 OSNAP, numeric locks, existing command grammar or authored identity. Preserve stale-revision failure, selection, Undo/Redo, Save/Reopen and no ghost IDs.
- D3 one-key `C` vs `C+Enter` must be settled *before* Owner exact D2 contract acceptance and activation. Owner can choose either explicitly, and the contractual tests must match the choice.
- Do not claim technical FINAL PASS based on Windows CI alone; actual Owner manual GUI validation is separately required.

## 9. DOC-01 — Documentation Impact

**Internal as-built docs: REQUIRED** for Qt focus/cancel ownership and shared Sketch Copy/transform behavior. **PL/EN Product docs: REQUIRED** for Grip Rotate/Scale/Mirror Copy, normal Mirror Keep Original and precisely accepted `C` keyboard grammar. **Generated Browser: REQUIRED** by repository `.\\ss2.ps1 docs`; freshness must be verified in the actual Windows build. This contract review is governance-only: no current-state doc changes until product behavior is genuinely implemented and verified.

**Owner decision requested for this proposal:** select **D3 A (one-key C)** or **D3 B (C+Enter)**, and separately accept exact S3 D2 ``0–9 plus 26 conditional paths. No merge/activation authority derives from accepting this direction alone.
