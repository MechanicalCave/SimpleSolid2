# Sketcher Repair S2 — Circle Size PointRequest/OSNAP and Line Grip TAN/PER Base — D2 Work Contract PROPOSAL

**Status:** **OWNER-ACCEPTED EXACT S2 §§0–8 + 16 CONDITIONAL PATHS** (2026-10-17). Governance-only PR #479, **NOT INTEGRATED, NOT ACTIVE, NO S2 PRODUCT CODE AUTHORITY**. Owner's exact acceptance is distinct from the still-required protected-main guarded-squash authorization, ACTIVE-only switch and eventual product approval.
**Decision class:** D2 (shared runtime CAD input/point-request semantics and Sketch/Qt interaction). Any unexpected D3 product shortcut/grammar decision remains Owner-owned.
**Authoritative baseline:** protected `MechanicalCave/SimpleSolid2` main `1874e1db07ffe7b1cb27a8ec9e14247859245ffc`, verified 2026-10-17 after Owner-manual/technical FINAL PASS and Owner-authorized guarded squash of completed S0/S1 + F2/N1 + D2-A product PR #477. No assumptions about unmerged experimental PR branches.
**Authority:** `AGENTS.md` → Constitution/Foundation 1.0/Architecture → accepted ADR-0008/-0009/-0010/-0011/-0012 as relevant → `work/ACTIVE.yaml` → Owner-accepted Sketcher Roadmap v1.10 §0, §5 and accepted R10/R11 contracts → the future explicitly integrated and ACTIVE-activated S2 Work Contract. R11 completed snap semantics are FROZEN positive/negative regressions, not fresh permission to redesign OSNAP. Part Modeling v1.34 retains S2-before-S3 planning and RG-02/PM-06 deferral.

## 0. Exact decision and governance boundary

On 2026-10-17 the Owner first accepted **S2 B03a + B04 direction** (`akceptuje - kontynuuj pracę`). After receiving governance Draft PR #479 and the explicit question **`Czy zatwierdzasz dokładny kontrakt S2 z PR #479 — §§0–8 i wszystkie 16 warunkowo dopuszczonych ścieżek?`**, the Owner replied **`zatwierdzam - kontynuuj`**. This is **exact acceptance of this contract's §§0–8 and the conditional 16-file allowlist**. It does **not** authorize product mutation, this PR's guarded squash, the separate `work/ACTIVE.yaml` switch, technical FINAL PASS or eventual implementation merge.

**Exact contract decision: SATISFIED, 2026-10-17.** Next independent decision is Owner authorization of the governance PR's **guarded squash** after exact-head Windows CLOSURE on the acceptance-record HEAD. Following verified merge, a separate ACTIVE-only one-file switch with its own CLOSURE and Owner merge authorization is mandatory before any S2 product source or test is modified.

**ACTIVE pointer truth:** protected main still names `work/SKETCHER_REPAIR_S0_S1_CONTACT_REGION_PROFILE_D2_WORK_CONTRACT_PROPOSAL.md` and Sketcher Roadmap v1.10. S0/S1 + D2-A has been technically accepted/merged, but merely having the S2 priority in v1.10 does not make S2 active. Preserve Roadmap v1.10, Foundation, ADRs and historical `next` record rather than overwriting it.

## 1. B03a/B04 defect boundary — read-only as-built evidence, not yet genuine RED

**B03a — Circle Radius/Diameter PointRequest/OSNAP.**
- In `SketchInteractionState::activePointRequest()`, a Circle currently has a PointRequest only while `circle_stage_ == await_center`; its `await_radius` stage is not admitted. Source: `src/sketch/interaction_state.cpp` at the above baseline.
- In `PartSketchInteractionController::handleCirclePointer()`, center uses `resolvePointerInput(input)` but size/radius stage explicitly uses `sketch::resolveSketchInput(input.position)`, i.e. an unassisted raw position for pointer-driven preview and accept.
- `acceptCirclePoint` computes a positive geometric radius from resolved center and second point; `acceptCircleRadius` and `lockCircleRadius` already separately accept positive explicit numeric sizes. The user-visible Circle size mode can be **Diameter** (default) or **Radius**; numeric keyboard length converts accordingly. This accepted grammar cannot regress.
- **Likely observable defect:** radius/diameter point-click cannot consistently use the same enabled Endpoint/Center/Quadrant/Intersection/TAN/PER, Temporary Snap Override, tracking and numeric locks as other point-acquisition requests. This is **a code-path explanation, not yet a real current-head Windows RED**. Verify actual supported modes and exact Qt behavior first.

**B04 — stationary opposite-end Line Grip Tangent/Perpendicular.**
- `beginDirectManipulation` starts Line Start/End Reshape with `session.pivot` at the **gripped/moving endpoint**.
- `activePointRequest` for a running manipulation currently uses `manipulation_->pivot` as its base. R11 TAN-from-point and PER-from-point operate relative to `request.base`.
- **Likely observable defect:** request-relative TAN/PER choices for a moving Line endpoint use the moving endpoint's original position, rather than the **stationary other endpoint of that Line**; resulting Line may fail true tangency/perpendicularity after edit. This is a **read-only causal hypothesis** requiring registered Windows RED and current behavior validation before a patch.

Prior Sketcher audit issues B03a/B04 against older heads are historical only. Do not rebrand old Linux/audit or screenshot claims as a Windows RED on current main. S1 exact-secant/contact fix and the 199/199 Windows integration baseline are frozen control witnesses.

## 2. B03a Owner-accepted D2 contract — Circle Center → Size

1. **One shared point resolver:** after Center acceptance, `await_radius` must expose a valid active `PointRequest` with Circle Center as the semantic base where relevant; preview, hover and accept must use the **same resolved Sketch-local candidate** through the existing `resolvePointerInput`/R10/R11 machinery. Do not fork a Circle-specific snap resolver or persist snap provenance.
2. **Size mode is separate from target geometry:** a pointer-selected second point yields Radius = Euclidean distance from stored Center to the resolved point; the Radius/Diameter **numeric mode** continues to interpret explicit typed lengths as Radius or Diameter respectively. A snapped geometric *point* does not have its coordinates or length doubled because the UI is in Diameter mode. No automatic authored Tangent/Coincident/Distance constraint.
3. **Numeric precedence and locks:** complete numeric size/point input, currently supported Radius/Diameter R/D switching and compatible field locks take priority over OSNAP, then tracking/inference/Polar, then raw pointer. Incompatible candidate must not silently violate a numeric lock or create a Circle with a different size. Rejected/invalid input leaves the Circle creation request in a well-defined noncommitted state.
4. **OSNAP and request lifecycle:** persistent mode preferences and one-shot Temporary Snap Override work in the size stage wherever the R11 request semantics admit them; explicit temporary override clears on successful point acceptance and Esc, no stale deferred TAN/EXT or Object Snap Tracking carries into subsequent Circle or next tool. Handle creation, preview and commit consistently on the same point.
5. **Exact and negative geometry:** finite eligible authored Line/Circle/Arc intersections/endpoints/centers/quadrants and construction geometry are valid sources subject to existing R11 priorities; overlap/coincident, ambiguous TAN/PER, inside-circle/no tangent, zero radius, nonfinite numeric input and disallowed finite Arc sweep remain fail-closed. If a specific request-relative TAN/PER mode has no unambiguous base, it must not fabricate a candidate or a Circle.
6. **Identity and lifecycle:** one valid Circle command commits once, produces fresh authored EntityId with selected Regular/Construction role; preview and rejected attempt allocate no authored IDs and no extra Undo step. Undo/Redo and Save/Close/Reopen reproduce the same Circle center/radius; no new persistent fields or schema revision.

## 3. B04 Owner-accepted D2 contract — opposite-end Grip Reshape TAN/PER

1. **Distinct operation pivot and snap reference:** for an active Reshape on Line Start, the stationary **Line End** is the reference point for PER/TAN; for Line End, stationary **Line Start** is the reference. The operation's original grip pivot/captured geometry still govern its existing movement, Copy and numeric behavior; do not implicitly relocate the transform pivot.
2. **One canonical request/resolver:** encode/derive an explicit request-local reference suitable for the existing R11 PointRequest/TAN/PER mechanics. Do not add a parallel Grip snap engine or use a viewer-space cursor coordinate as the geometric base. If an additive internal/request-local `PointRequest` datum in `src/sketch/include/simplesolid2/sketch/interaction_state.hpp` is essential, it is conditionally within this exact D2 scope ONLY when transient, nonpersistent and preserves all established R10/R11 request-compatibility rules. Any broader externally visible PointRequest contract/API change ⇒ STOP for Owner D2.
3. **Valid finite tangent/perpendicular:** only candidates producing an actual segment from the stationary endpoint to the newly resolved movable endpoint that satisfies the requested finite-curve R11 tangent/perpendicular relation can be accepted. For Circle/Arc tangent, handle the appropriate two branches by normal R11 capture/branch policy and Arc sweep; for Line perpendicular honor finite foot/Extension restrictions. A live Reshape preview, resulting authored edit and later snap feedback must agree.
4. **No unintended edit of other geometry:** the opposite endpoint remains EXACTLY unchanged under Start/End Reshape; no new persistent relation, no implicit healing, no Circle/Arc edit and no accidental manipulation of mixed selection. Preserve existing Line EntityId for ordinary Reshape, selected set semantics, fresh IDs only for separately accepted Grip Copy mode, and transaction/Undo/Redo invariants.
5. **Negative cases:** no base for Circle quadrant/Arc Grip, Line-center Move, Rotate/Scale/Mirror or global Move/Copy should be rewritten by this fix. Preserve original pivot-based direct-distance and numeric lock meanings except those precisely agreed by the new request-relative TAN/PER contract; if those behaviors conflict, **STOP to resolve explicit D2 semantics**, not silent reinterpretation. Degenerate zero-length Line, overlap/ambiguous contact, no real tangent and stale host revision must fail closed without authored mutation.
6. **R11 compatibility:** persistent and Temporary OSNAP, numeric precedence, stable candidate tie-breaking/hysteresis, construction sources, deferred TAN/EXT, snapping/guide presentation and cancellation are inherited from R11, not redefined in S2.

## 4. Test-first plan: independent, genuine current-head RED and negative gates

**S2-0 audit/reproduction:** run registered source-level Core and real Qt/Viewport tests on a NEW implementation branch anchored to the then-current **protected main**, before changing runtime sources. Mark each current result RED/PASS/GAP with exact SHA, fixture, expected and observed. Preserve failing fixtures, do not mark XFAIL or weaken existing controls.

**B03a RED:**
- Circle Center accepted; compare raw vs OSNAP-assisted size-stage `PointRequest` availability, pointer preview vs final committed center/radius; staged Endpoint, Center, Quadrant, Intersection and Construction snap sources as eligible, including a target off the cursor position; small nonzero radius with valid model geometry.
- Diameter mode and Radius mode, typed explicit `D`/`R`, numeric point locks, one-shot Temporary Snap Override, tracking/Polar priority, TAB/cancellation as supported; numeric-size input must not acquire an unrelated snap and create extra requests.
- Invalid candidate/zero radius/NaN, ambiguous deferred TAN/PER, Circle rejection, Esc, tool switching, click with missing capture; no hidden ID/Undo change and cold document roundtrip.

**B04 RED:**
- Fixed Line Start + moving Line End **and reversed**; snap TAN to Circle/Arc and PER to finite Line using the **stationary** anchor; assert exact resulting source geometry/relational residual not merely a cursor cue. Test both tangent branches, CW/CCW Arc sweeps and source direction reversal.
- Negative: no-real-tangent/degenerate line/unsupported foot, temporary override versus persistent mode, numeric-lock precedence, invalid Edit fail-closed, mixed selection, Grip Copy, Move, rotate/scale/mirror unchanged, preview/commit identity and Undo/Redo.

**Carryover test controls:** R10 numeric and R11 OSNAP/OTRACK/EXT/deferred TAN ordering; Sketch R5/R6 Circle/Arc; SK07F precision/Grip and SK04C Qt; S1 F1/F2/F3, S1 N1 exact two-secant/pocket, Region/Profile/Trim R12, real OCCT Profile Face/Extrude/Save-Reopen. No source workaround breaks the original D2-A Owner #3/#4. Existing registered tests, not a substitute mock-only UI success.

**Evidence:** separate Windows FOCUSED for the relevant registered Core and Qt/Controller tests (as supported by current lane), Draft FAST, Ready FULL Core+native OCCT+Qt/Desktop, DOC-01, exact-head CLOSURE, Owner real GUI acceptance and separate technical/merge decisions. All claimed PASS tied to specific HEAD + executed lane.

## 5. Owner-accepted narrow CONDITIONAL 16-path implementation allowlist (requires activation first)

Each path is authorized only for the stated S2 B03a/B04 role; a listed file is not mandatory to change. An essential file outside the list ⇒ STOP and separately approved D2 amendment.

1. `src/sketch/interaction_state.cpp` — Circle size `PointRequest` and request-local stationary Line Grip reference, preview/accept lifecycle.
2. `src/sketch/include/simplesolid2/sketch/interaction_state.hpp` — ONLY minimal transient request/session data required for B03a/B04; no durable schema change or unrelated public API redesign.
3. `src/ui/part_sketch_interaction_controller.cpp` — route Circle size pointer/preview/accept and Grip TAN/PER to shared resolver and existing semantic commands.
4. `src/ui/part_sketch_interaction_controller.hpp` — private controller bookkeeping needed by #3 only; no new UI product surface.
5. `src/sketch/snap.cpp` — only a proven local R11 candidate/reference correctness change needed by B04; NO global OSNAP priority or tolerance change.
6. `tests/sk06a_circle_arc_interaction_state_test.cpp` — existing registered Circle PointRequest/size stage Core RED and controls.
7. `tests/sk07f_precision_input_controller_test.cpp` — existing registered R10/R11 Circle size, Grip TAN/PER, numeric and Qt controller integration.
8. `tests/sk04c_part_sketch_interaction_controller_test.cpp` — existing registered pointer/preview/commit and lifecycle GUI-adapter regression.
9. `tests/r11_snap_core_test.cpp` — existing registered request-relative snap candidate, finite curve and override controls.
10. `tests/sk07e_space_cycle_edit_mode_test.cpp` — existing registered active Grip mode/undo/selection negative protections if touched.
11. `docs/internal/SHARED_2D.md` — current-state Sketch PointRequest/Grip request semantic contract.
12. `docs/internal/OBJECT_SNAP_TRACKING.md` — current-state R11 OSNAP/OTRACK/inference and request-relative TAN/PER.
13. `docs/product/pl/PARTS.md` — Polish user-facing Circle size/Grip snapping behavior.
14. `docs/product/en/PARTS.md` — English peer of #13, same capabilities and limitations.
15. `docs/browser/index.html` — generated ONLY by repository `.\\ss2.ps1 docs`; never independently hand-edit.
16. `work/SKETCHER_REPAIR_S2_POINT_REQUEST_OSNAP_GRIP_ACCEPTANCE_EVIDENCE.md` — NEW factual ledger on implementation branch: real RED/PASS/FAIL/GAP, Owner manual and gated decisions.

**Not authorized:** `src/part/**`, OCCT/Kernel production, persistence/native writer/schema, generic App-wide CAD router or new shortcut, Viewer provider internals, `src/application/**`, new CMake/tests targets, accepted roadmap/ADR/Foundation, OSNAP global settings model, `src/ui/**` outside the exact controller pair, Circle/Arc geometry model rewriting, S3 Esc/Copy/Mirror beyond B04 nonregression, constraints/solver, R13+, RG-02/PM-06, or random global epsilon/gap welding. If accepted S2 behavior demonstrably requires any excluded path, STOP for exact path addendum before editing.

## 6. Expected Owner Windows manual acceptance

Using a fresh build from exact implementation HEAD and the same local Sketch stage:
- Create a Circle with center specified by typed coordinate, then set size by **visible OSNAP point** off the raw cursor and compare preview with actual accepted radius; repeat Diameter and Radius keyboard modes with numeric override.
- Test Center → Size with one-shot snap and persistent Object Snap; check a target's authored coordinate and cancellation/reset after Esc (without activating unrelated S3 Esc-routing changes).
- Start- and End-Grip reshape of Line against Circle/Arc TAN and finite-Line PER: opposing Line endpoint unchanged, new Line exactly matches resolved contact and accepted tangent/perpendicular relation, undo/redo restores old Line and redo restores new.
- Negative: numerical lock outranks incompatible snap, tangent from inside Circle/non-unique tangent fails without mutation, floating gaps stay genuine gaps, existing S1 Owner sketch #3/#4 Profile/Extrude unaffected.
- Save/Close/Reopen the edited Sketch; confirm authored identities/roles, source coordinates and outputs are unchanged. Verify no surprise Circle/Arc persistence modifications and no extra durable constraints.
- Check normal Line, Arc, common Move/Copy, Grip transform mode cycles and previous R11 OSNAP/OTRACK. Report exact HEAD, PASS/FAIL per B03a/B04 and any GUI screenshot only if useful, not invented.

## 7. Required independent governance and implementation gates

1. **SATISFIED 2026-10-17:** Owner explicitly approved exact PR #479 **§§0–8** including all **16 conditional permitted paths**, replying `zatwierdzam - kontynuuj` to the precise scope question. The earlier `akceptuje - kontynuuj pracę` accepted S2 **direction only**, independently.
2. This Owner-accepted governance contract must have independent **exact-HEAD Windows CLOSURE PASS** on the acceptance-record SHA; initial draft CLOSURE #3575 was SUCCESS on the earlier direction-only SHA `2cba87ab5494d5c048763f8efac683319c34943a` but is NOT credited to the acceptance-record SHA. **Separately request and obtain guarded-squash merge authority for PR #479**; then verify protected-main merge and exact contents.
3. Propose a distinct **ONE-FILE `work/ACTIVE.yaml` PR** after governance integration: switch `active_work` to this exact accepted S2 contract, retain Sketch Roadmap v1.10, Foundation 1.0, ADR list, history `next` and accepted completed S1/D2-A status. That PR requires its own Ready exact-head Windows governance CLOSURE, a **separate Owner guarded-squash authorization**, and verified main merge.
4. Only then branch product work from verified main, first genuine current-head RED for B03a/B04; implement exclusively within §5 allowlist and without overriding R11/R10 or completed S1.
5. Genuine Windows **FOCUSED → Draft FAST → Ready FULL** (Core/Qt/native OCCT as current CI supports), canonical current-state internal/PL+EN docs, generated Browser via `.\\ss2.ps1 docs`, factual evidence ledger and exact-head Windows **CLOSURE**. No relabeling of RED to PASS.
6. Owner exact-HEAD manual Windows PASS for B03a and B04 + existing R11/S1 critical controls; **independent technical FINAL PASS** and **separate guarded implementation squash authority**, with protected-main changed-path/lease preflight. No automatic merge from green CI.
7. S3 B03b/B05/B06 remains a **future independent D2 work contract and ACTIVE-only transition**. Any need to settle product `C` vs `C+Enter`, Esc/focus routing or Mirror/Copy UX first ⇒ STOP for Owner D3, not part of this work.

## Documentation impact

**§8 of the Owner-accepted S2 Work Contract — mandatory DOC-01 declaration.**

Internal docs: required
User/Product docs: required
Reason: S2 changes observable Circle point-assisted sizing and Grip tangent/perpendicular snapping, request-local base semantics, preview/commit and user-visible error/cancellation behavior; internal ownership and PL/EN user guides must match the actually accepted implementation. Generated Browser must be regenerated from canonical Markdown, not hand-edited. The present governance-only PR edits only this new `work/` D2 proposal. Current-state docs are updated only together with later verified product changes.

## Decision log

2026-10-17 — Owner direction **S2 B03a + B04 accepted**: `akceptuje - kontynuuj pracę`, after explicit confirmation of next priority. This earlier reply accepted the direction, not the exact text.

2026-10-17 — **Owner exact contract acceptance:** In response to an explicit request to approve PR #479 §§0–8 and 16 conditional paths, Owner replied `zatwierdzam - kontynuuj`. Exact bounded D2 S2 policy and path allowance ACCEPTED. Initial Draft Windows CLOSURE #3575 had passed on the previous text; re-run exact-head CLOSURE after this governance acceptance update. No active S2 product edits, protected-main merge authorization, ACTIVE switch, technical FINAL PASS or implementation merge authority derives from exact contract acceptance alone.
