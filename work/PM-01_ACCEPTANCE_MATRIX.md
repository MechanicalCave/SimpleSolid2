# PM-01 — Final Acceptance Matrix

**Status:** PENDING OWNER MANUAL WINDOWS ACCEPTANCE  
**Work Contract:** `work/PM-01_FIRST_SOLID_VERTICAL_SLICE.md`  
**Architecture:** ADR-0014 + ADR-0015  
**Runtime baseline merged to main:** `ca5592d07a271c83c9752fb654cded2c7b3d83c6`  
**Latest runtime exact-head gate:** Windows FULL #1364 — PASS on `60f2a7ac6ed894e77c751449b370ee4322c870ae`

## 1. Automated acceptance coverage

| Contract requirement | Durable implementation/evidence | Status |
| --- | --- | --- |
| Pre-PM-01 Part migration, Empty Body, schema v8, BodyId/FeatureId high-water, Profile visibility migration | PR #144, Windows FULL #1326 | PASS |
| First Add, Profile with hole, OneSide Forward/Reverse, Midplane, chained Add/Cut, explicit detached/no-effect/empty/multi-solid failures, fuzzy=0/refine OFF, semantic cap/side lineage | PR #145, Windows FULL #1327 | PASS |
| Ordered evaluator; UpToDate/Failed/Blocked/Suppressed; no stale last-good truth; semantic Feature commands; Create/Edit/Suppress/Delete; rejected command does not consume identity/history | PR #146, Windows FULL #1330 | PASS |
| Provider-neutral final Body presentation, no topology picking/provider identity leakage, stale/unavailable Body clears presentation | PR #147, Windows FULL #1331 | PASS |
| Transient Add/Cut preview, revision-keyed presentation cache, preview replacement/clear and provider-failure recovery | PR #148, Windows FULL #1332 | PASS |
| Revision-bound Extrude draft, execution-time revalidation, stale draft/evaluation rejection, one Finish commit | PR #149, Windows FULL #1333 | PASS |
| Operations UI + shared Command Line draft; EXTRUDE/ADD/CUT/REVERSE/MIDPLANE/ONESIDE/FINISH/CANCEL; unit-aware Length; Dynamic Input; Cancel non-authoring | PR #150, Windows FULL #1334 | PASS |
| Body/Feature Tree and Properties; status/diagnostics; Profile↔Feature navigation; FeatureId-preserving Edit Extrude; temporary source-Profile reveal | PR #151, Windows FULL #1336 | PASS |
| Suppress/Unsuppress/Delete from Properties/Tree; automatic Profile visibility; Undo/Redo; ordered Add/Cut persistence; Save → destroy runtime/session/provider state → Reopen → cold rebuild; persisted suppression/delete/source Profile | PR #153, Windows FULL #1339 | PASS |
| Same runtime checkpoint passes semantic/core, kernel-native and desktop FULL topology | Windows FULL #1339 | PASS |
| Manual-remediation H1: mixed Line+Arc Profile fidelity and reverse Arc traversal | PR #155, Windows FULL #1341 | PASS |
| Manual-remediation H2: preview replacement and initial curved-presentation fix | PR #156, Windows FULL #1342 | PASS |
| Manual-remediation H3: Tree/default-distance/command-first Extrude UX | PR #157, Windows FULL #1343 | PASS |
| Manual-remediation H4a: debounced typing + live Tree diagnostics/warning icons | PR #160, Windows FULL #1346 | PASS |
| Manual-remediation H4b: operation-only preview + nodal-normal smooth shading + restored mesh density | PR #161, Windows FULL #1348 | PASS |
| Manual-remediation H5: exact Add/Cut delta preview, transient source-Profile hide, volumetric Cut no-effect including face/edge/point contact | PR #165, Windows FULL #1359 | PASS |
| Manual-remediation H6: independent committed-Body / Add / Cut OCCT shading aspects; preview replacement/clear cannot recolor Body | PR #169, Windows FULL #1364 | PASS |
| As-built docs + PL/EN product docs + generated Product Browser | PR #163/#166/#171, Windows DOCS #1350/#1360/#1367 | PASS |
| Supported Windows integrated manual workflow | Section 3 below | PENDING OWNER |

The runtime acceptance matrix has zero known false-Resolved semantic topology outcomes. PM-01 does not authorize face/edge topology picking, provider-native durable identity, adaptive fuzzy healing, multi-body or later Part-v1 operations.

## 2. Final closeout gate

Before PM-01 may be marked COMPLETED:

1. canonical internal and PL/EN product documentation must describe the as-built PM-01 surface;
2. `docs/browser/index.html` must be regenerated from canonical Markdown and documentation validation must pass;
3. an exact-head repository gate must pass on the final documentation/evidence candidate;
4. Owner must complete the Windows workflow below and report PASS;
5. only then may ACTIVE/Part Modeling roadmap/PM-01 contract be moved to completed state.

Any code change after the final runtime baseline requires a new exact-head runtime-appropriate gate. Documentation/evidence-only closure must not silently alter production behavior.

## 3. Owner manual Windows acceptance

Use a clean supported Windows launch and a normal Project/Part. Record PASS/FAIL for each workflow.

### M1 — first Body + Add + Properties

1. Create/open a Part.
2. Create an Origin-plane Sketch with a closed rectangle and Finish Sketch.
3. Create and Finish a Profile.
4. Start Extrude from that Profile.
5. Confirm first Feature is Add-only.
6. Set OneSide Length with an explicit unit and Finish once.
7. Confirm one Body, one UpToDate Extrude Feature and visible solid.
8. Inspect Body/Feature Properties and verify source Profile/Sketch navigation.
9. Confirm the consumed Profile is hidden under Automatic policy.

**PASS:** one durable Feature is authored, one Undo step represents Finish, Tree/Properties are coherent, and no second confirmation is required.

### M2 — Edit Extrude / identity / preview

1. Open Edit Extrude for the Feature.
2. Confirm the source Profile is hidden while a valid solid preview is ready; make the draft temporarily invalid and confirm the source Profile is revealed for diagnosis without changing authored visibility.
3. Restore a valid Length and verify the Profile hides again while live preview returns.
4. Switch OneSide direction with Reverse; then switch to Midplane and verify Reverse is no longer semantic.
5. Finish once.
6. Re-open Properties.

**PASS:** the same FeatureId remains, parameters update, source Profile returns to normal Automatic presentation and the Body remains UpToDate.

### M3 — ordered Cut + lifecycle

1. Create a second valid Profile suitable for removing material from the existing Body.
2. Create an Extrude Cut; use Midplane if needed to span the Body.
3. Confirm Body contains ordered Add then Cut and the final solid reflects the Cut.
4. Suppress the Cut from Properties or Tree.
5. Confirm its FeatureId/parameters remain, Body reevaluates without the Cut and the Cut source Profile becomes visible under Automatic policy.
6. Undo, Redo, then Unsuppress.
7. Delete the Cut.
8. Confirm the source Profile remains and the Feature is removed.
9. Undo Delete and confirm the same FeatureId returns.

**PASS:** Suppress and Delete are distinct, Undoable authored operations and automatic Profile visibility follows active consumption.

### M4 — Command Line parity

1. Select an admissible Profile.
2. Start `EXTRUDE` from Command Line.
3. Change at least one accepted option using Command Line (`ADD`/`CUT`, `MIDPLANE`/`ONESIDE`, `REVERSE` where legal).
4. Enter a unit-aware Length expression.
5. Confirm Operations panel and preview reflect the same draft.
6. Finish with `FINISH` or the accepted Enter path.
7. Start another draft and `CANCEL`.

**PASS:** GUI and Command Line remain one semantic draft; Finish authors once and Cancel authors nothing.

### M5 — Save / Close / Reopen cold lifecycle

1. Leave a representative ordered Add/Cut model saved, including at least one lifecycle state exercised above.
2. Save.
3. Close the Part/application so the previous runtime session/provider state is destroyed.
4. Reopen the Project and Part.
5. Inspect Body/Feature order, IDs, parameters, statuses and source Profile relationships.
6. Confirm the solid rebuilds from authored state.
7. Exercise Undo/Redo on a new post-reopen Feature lifecycle change.

**PASS:** authored Body/Feature intent survives; runtime B-Rep/preview/history from the old process is not required.

## 4. Manual result

**Owner Windows result:** PENDING RETEST — H6 automated PASS  
**Date:** 2026-10-03  
**Last Owner-tested candidate:** `26dd32f65d2d1497d3d3ee55ca52ab69182f93fb`  
**Next Owner-test candidate:** `d2655331c6713d87fc61564e75300b61312dfb37` (runtime baseline `ca5592d07a271c83c9752fb654cded2c7b3d83c6`)  
**Notes:** H5 exact-delta/no-effect behavior is automated PASS. H6 isolates committed Body and transient preview shading state in Qt/OCCT and passed exact-head Windows FULL #1364; H6 internal docs/Product Browser passed Windows DOCS #1367. Only the focused Owner Add/Cut preview color-isolation retest remains before PM-01 governance completion.
