# PM-04F — Owner Windows Acceptance

**Status:** PENDING OWNER  
**Parent Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Checkpoint:** PM-04F — documentation / Product Browser / Owner Windows acceptance  
**Date:** 2026-10-06

## Purpose

This is the final supported-Windows product workflow for PM-04. Automated gates establish semantic/provider/regression correctness; this workflow verifies the accepted user-facing Axis/Revolve experience in the real desktop application.

Do not mark PM-04 complete from automation alone. The Owner must report PASS for this workflow, or report the exact failing step/behavior.

## Preconditions

- use the exact post-PM-04F-docs main candidate that passed the final automated gate;
- launch through the supported repository Windows workflow;
- start from a new disposable Project/Part so existing authored state cannot mask failures;
- keep the default product presentation unless a step explicitly changes visibility.

## Workflow

### A. Authored Axis lifecycle

1. Create a new Part and a Sketch on the XY Origin plane.
2. Create one closed Regular profile entirely on one side of the Sketch U axis (for example a rectangle away from V=0).
3. Create a separate non-degenerate **Construction Line** along the intended revolve axis. Confirm it stays Construction.
4. Start **Axis**, select that Line and Finish.
5. Confirm the Axis appears under its source Sketch, has Properties/status and is visible independently from the source Sketch geometry.
6. Hide then show the Axis. Confirm modeling state/history is unchanged except authored presentation visibility.
7. Edit/re-source the Axis to another admissible Line and Finish. Confirm the same Axis object/identity remains and downstream state recomputes.
8. Undo and Redo the re-source and confirm the Axis returns between the exact two authored sources.

### B. Revolve create / preview / interaction

9. Ensure the closed Profile and a resolved authored Axis are available. Start **Revolve** selection-first or command-first.
10. Confirm Profile and Axis are explicit fields; no Axis is silently inferred.
11. Confirm a new valid draft defaults to **Add + One Side + 360° + Reverse off**.
12. Change Angle to 180° and confirm the transient preview updates while the already committed Body remains in normal presentation.
13. Toggle **Reverse** and confirm direction changes without authored mutation before Finish.
14. Switch to **Midplane** and confirm Reverse is unavailable/not authored; Angle remains the total symmetric sweep.
15. Return to a valid One Side or Midplane setting and Finish. Confirm one Feature/one Undo step is created.
16. Edit that Revolve from Tree/Properties. Confirm Edit uses the same Operations surface and preserves the same Feature identity.
17. Cancel one edit after changing values and confirm the committed Feature is unchanged.
18. Re-enter Edit, make a valid change, Finish and confirm one Undo step.

### C. Origin Axis and visibility semantics

19. Start another Revolve context that can use the same valid Profile and explicitly choose Origin **X**, **Y** or **Z** as geometrically admissible for that Profile.
20. Confirm no authored Axis object is created merely by using an Origin axis.
21. Hide the chosen Origin/authored Axis in normal presentation, then enter a Revolve draft using it through explicit Tree/property selection. Confirm it may be temporarily visible/emphasized during the draft but returns to authored Hidden after Cancel/Finish.

### D. Failure and repair

22. With a Revolve referencing an authored Axis, delete the Axis object. Confirm the Revolve remains authored and reports a Missing/Blocked condition rather than disappearing or binding to another line.
23. Undo Delete and confirm the same Axis returns and the Revolve repairs.
24. Delete or invalidate a source Line/geometry so the Axis or Profile becomes unavailable. Confirm no stale last-good Body is presented as current valid modeling truth.
25. Repair through the supported explicit path (Axis re-source and/or source geometry repair) and confirm the same durable Axis/Profile/Feature identities resume current evaluation.

### E. Save / reopen / cold reconstruction

26. Save the Part, close it, reopen it from the Project and verify the authored Axis visibility/source, Revolve Profile/Axis reference, Add/Cut mode, OneSide/Midplane, Angle, Reverse where applicable and Suppressed state.
27. Confirm reopened Body/Revolve presentation is freshly reconstructed and the document does not depend on previous-process preview/provider tokens.
28. Exercise Undo/Redo after a fresh authored mutation and confirm normal history behavior; pre-close Undo history itself is not expected to persist.

### F. Documentation / Product Browser

29. Open the generated Product Browser and verify the Axis/Revolve workflow is present in both Polish and English.
30. Confirm Product docs do **not** claim Datum Axis, Body-Edge/Curve Axis, Projection, >360° Revolve or multi-Body as implemented.

## Acceptance

Owner result:

- **PASS** — every required step behaves as described and there is no functional PM-04 blocker; or
- **FAIL** — report the first failing step plus observed/expected behavior. Do not close PM-04.

Presentation polish already explicitly deferred by prior accepted governance (the common neutral translucent Origin/Datum plane fill for PM-06) is not a PM-04 blocker unless PM-04 introduced a new regression beyond that accepted defer.
