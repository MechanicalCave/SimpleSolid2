# PM-04 — Final Acceptance Matrix

**Status:** COMPLETED — PASS  
**Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Owner amendment:** `work/PM-04F_AXIS_DESIGNATION_UX_AMENDMENT.md`  
**Owner final Windows acceptance:** PASS — 2026-10-06  
**Owner-tested runtime/docs candidate:** `8d6f082137a573aab08a4eee3b383a9923d98a49`  
**Final runtime workflow:** Windows FULL #1578 attempt 2 — PASS  
**Implementation / closure PR:** #277

## 1. Automated evidence

PM-04 runtime checkpoints are accepted:

- PM-04A Axis semantic model / schema v12 — Windows FULL #1531 PASS;
- PM-04B Axis lifecycle / Tree / Properties / Viewer — Windows FULL #1533/#1544 PASS;
- PM-04C Revolve semantic/kernel operation / topology catalog — Windows FULL #1547/#1549 PASS;
- PM-04D Revolve draft / Operations / Command Line / preview — Windows FULL #1551/#1555 PASS;
- PM-04E integrated lifecycle / repair / persistence — Windows FULL #1560/#1562/#1563 PASS;
- pre-amendment PM-04F canonical documentation / Product Browser — Windows DOCS #1565 PASS;
- Owner-accepted PM-04F Axis-designation UX remediation — exact candidate `8d6f082137a573aab08a4eee3b383a9923d98a49`, Windows FULL #1578 attempt 2 PASS.

Final #1578 attempt 2 evidence:

- semantic/core: 25/25 PASS;
- kernel-native: 47/47 PASS;
- desktop FULL: 104/104 PASS;
- current internal + PL/EN Product documentation: PASS;
- generated Product Browser regeneration/diff verification: PASS;
- FAST/SUBSYSTEM selector checks, SR-02 latency evidence and CI-04 warm/comparative evidence: PASS.

Attempt 1 of #1578 failed before checkout/build/test because the self-hosted runner could not download a GitHub action. The unchanged exact candidate passed attempt 2.

Earlier FULL #1577 correctly exposed a test-only use of a private Workbench API. The regression was changed to enter Sketch Edit through the public Document Tree -> Edit Sketch user surface before the accepted final FULL.

## 2. Owner Windows result

The Owner tested exact runtime candidate `8d6f082137a573aab08a4eee3b383a9923d98a49` on supported Windows on 2026-10-06 and reported **PASS with no errors**.

Accepted product behavior includes:

- no standalone GUI Axis creation button;
- Line authoring exposes orthogonal `Regular | Construction` geometry role and one-shot Part `Axis` designation;
- successful Line+Axis authors both objects atomically in one Undo step;
- existing selected Line Axis ON/OFF preserves the Line and obeys explicit Axis identity/delete semantics;
- Edit/Re-source preserves AxisId;
- duplicate-source Create/Re-source fails closed while legacy duplicate-source persisted state remains loadable and UI-indeterminate until repair;
- `AXIS` Command Line remains available;
- Origin X/Y/Z remain direct built-in AxisReference values;
- Revolve Add/Cut, OneSide/Midplane, partial/full angle, OneSide Reverse, preview/Edit/Finish/Cancel and ordered Feature lifecycle;
- Missing/Blocked repair semantics with no silent rebind or stale last-good geometry;
- Undo/Redo, Save/Close/Reopen and true cold reconstruction;
- current internal/Product documentation and generated Product Browser.

No functional PM-04 blocker was reported.

## 3. CI closure note

Work-only bookkeeping run #1579 was cancelled twice during self-hosted runner **Set up job** while resolving/downloading `actions/checkout@v4` from `launch.actions.githubusercontent.com:443`.

Those cancellations:

- occurred after classification selected `closure`;
- did not reach checkout or execute governance/documentation invariants;
- do not invalidate the accepted runtime/docs FULL evidence or Owner product PASS;
- remain a procedural PR-merge gate: #277 must still obtain a successful work-only closure run before merge.

No workflow bypass or PM-04-scoped CI rewrite is authorized by this acceptance.

## 4. Completion boundary

PM-04 Axis / Revolve is **COMPLETED — PASS** and its production mutation authority is closed.

PM-05 Edge Features: Fillet / Chamfer is next in roadmap order but remains **NOT ACTIVE** until a separate bounded Work Contract is explicitly Owner-accepted and activated.

PM-06 remains separately gated and retains the previously accepted cross-package Origin/Datum neutral translucent plane-fill presentation obligation.

Projection remains separately gated.
