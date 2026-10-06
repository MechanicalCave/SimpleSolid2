# PM-04F — Owner Windows Acceptance

**Status:** PENDING IMPLEMENTATION + OWNER  
**Parent Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Owner amendment:** `work/PM-04F_AXIS_DESIGNATION_UX_AMENDMENT.md`  
**Checkpoint:** PM-04F — Axis-designation remediation / documentation / Product Browser / Owner Windows acceptance  
**Date:** 2026-10-06

## Purpose

This is the final supported-Windows workflow for PM-04 after the Owner-accepted Axis-designation UX amendment.

Do not execute this as final acceptance until the amendment implementation, new exact-head Windows FULL, current documentation and regenerated Product Browser have passed. Do not close PM-04 from automation alone.

## Preconditions

- use the exact final PM-04F candidate identified by the new Windows FULL and documentation gates;
- launch through the supported repository Windows workflow;
- start from a new disposable Project/Part;
- keep default presentation unless a step explicitly changes visibility.

## Workflow

### A. Axis designation during Line authoring

1. Create a new Part and a Sketch on the XY Origin plane.
2. Confirm there is no standalone GUI **Axis** authoring button in the Part Modeling / Sketcher toolbar surface.
3. Start **Line**. Confirm Operations separates **Geometry role = Regular/Construction** from **Part reference = Axis**.
4. Set Geometry role to **Construction**, enable **Axis**, and draw one non-degenerate Line.
5. Confirm the Line remains Construction and exactly one authored Axis appears under the source Sketch in Tree with its own AxisId/Properties.
6. Confirm the Axis designation is one-shot: after the successful Line+Axis commit, Axis returns to OFF. Draw the next continuous Line segment and confirm it does not create another Axis.
7. Undo once and confirm the Line from step 4 and its Axis disappear together. Redo once and confirm both return with the same authored identities.
8. Repeat with a new **Regular + Axis** Line and confirm Regular/Construction is orthogonal to Axis designation.

### B. Existing-Line designation and uniqueness

9. Select exactly one Line with no Axis. Confirm Operations shows its Geometry role and **Part reference: Axis = OFF**.
10. Enable Axis. Confirm exactly one new Axis is created in one Undo step and the Line geometry role/EntityId is unchanged.
11. Toggle that Line between Regular and Construction and confirm the Axis remains the same authored Axis.
12. Disable Axis on an unreferenced test Line. Confirm the Axis is removed while the Line remains; Undo restores the same AxisId.
13. Re-enable Axis, then attempt to create/re-source another Axis to that exact Line through the supported expert/Edit path. Confirm duplicate-source authoring is rejected with zero partial mutation.
14. If a pre-amendment duplicate-source fixture is available, open it and confirm it loads without AxisId/reference rewrite; selecting the shared source Line reports an indeterminate/conflict designation and does not arbitrarily toggle one Axis.

### C. Axis lifecycle and identity-preserving re-source

15. Select an authored Axis in Tree. Confirm Properties, Show/Hide and Edit/Re-source remain available even though the GUI Axis creation button is gone.
16. Edit/Re-source it to another admissible Line that is not already designated. Confirm the same AxisId and authored visibility are preserved.
17. Undo/Redo the re-source and confirm the exact two sources alternate under the same AxisId.
18. Confirm `AXIS` Command Line remains available for selection-first and command-first Axis creation and follows the same duplicate-source rule.

### D. Revolve create / preview / interaction

19. Create/ensure one closed valid Profile entirely on one side of the intended Axis and one resolved authored Axis.
20. Start **Revolve** selection-first or command-first. Confirm Profile and Axis are explicit; a raw non-designated Sketch Line is not silently accepted as AxisReference.
21. Confirm a new valid draft defaults to **Add + One Side + 360° + Reverse off**.
22. Change Angle to 180° and confirm transient preview updates while committed Body presentation remains normal.
23. Toggle Reverse and confirm direction changes without authored mutation before Finish.
24. Switch to Midplane and confirm Reverse is unavailable/not authored and Angle is the total symmetric sweep.
25. Finish a valid Revolve and confirm one Feature / one Undo step.
26. Edit that Revolve, Cancel one changed draft, then Edit again and Finish a valid change. Confirm FeatureId is preserved and only the successful Finish creates history.

### E. Origin Axis and referenced-Axis removal

27. Start another geometrically admissible Revolve and explicitly select Origin X/Y/Z. Confirm no authored Axis object is created for Origin.
28. Hide an Origin/authored Axis and enter Revolve using it through explicit semantic selection. Confirm transient emphasis does not change authored visibility.
29. With a Revolve referencing an authored Axis, select its source Line in Sketch Edit and disable Axis designation. Confirm an explicit warning states that the consumer will become Missing/Blocked.
30. Confirm removal. Verify the Revolve remains authored, becomes Missing/Blocked and does not bind to another Line.
31. Undo and confirm the exact same AxisId returns and repairs the Revolve.
32. Re-source the Axis explicitly and confirm the same Revolve reference follows the preserved AxisId.

### F. Failure, Save / reopen / cold reconstruction

33. Delete/invalidate an Axis source Line or its support. Confirm no stale last-good Axis/Body is presented as current valid modeling truth.
34. Repair through the explicit supported path and confirm existing durable Axis/Profile/Feature identities resume evaluation where valid.
35. Save, close and reopen. Verify Axis source/visibility, Revolve AxisReference, Profile, Add/Cut, OneSide/Midplane, Angle, Reverse where applicable and Suppressed state.
36. Confirm Body/Revolve/Axis presentation is freshly reconstructed without previous-process preview/provider tokens.
37. Exercise a fresh authored mutation followed by Undo/Redo and confirm normal history behavior; pre-close history is not expected to persist.

### G. Documentation / Product Browser

38. Open the generated Product Browser and verify PL/EN documentation describes Axis as a Part designation of a Line while preserving the separate authored Axis object/AxisId.
39. Confirm docs no longer instruct normal GUI users to start a standalone Axis toolbar tool.
40. Confirm docs retain `AXIS` Command Line, Edit/Re-source, Tree/Properties, Show/Hide/Delete, Origin X/Y/Z direct AxisReference and the explicit exclusions: Datum Axis, Body-Edge/Curve Axis, Projection, >360° Revolve and multi-Body.

## Acceptance

Owner result:

- **PASS** — every required step behaves as described and there is no functional PM-04 blocker; or
- **FAIL** — report the first failing step plus observed/expected behavior. Do not close PM-04.

Presentation polish already explicitly deferred by prior accepted governance (the common neutral translucent Origin/Datum plane fill for PM-06) remains non-blocking unless this amendment introduces a new regression.
