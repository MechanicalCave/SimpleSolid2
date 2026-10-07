# PM-05F — Owner Windows Acceptance

**Status:** READY FOR OWNER — RESULT PENDING
**Parent Work Contract:** work/PM-05_EDGE_FEATURES.md
**Checkpoint:** PM-05F — documentation / cumulative automated evidence / Owner Windows acceptance
**Date prepared:** 2026-10-07
**Exact final candidate:** pending PM-05F final Windows FULL

## Purpose

This is the final supported-Windows product workflow for PM-05 Edge Features: Fillet / Chamfer.

The Owner result remains intentionally PENDING until the exact final runtime/docs candidate passes the PM-05F Windows FULL gate and the Owner reports the manual outcome. Automated evidence cannot self-accept this package.

## Preconditions

- use the exact final candidate recorded here after the PM-05F FULL PASS;
- launch through the supported repository Windows workflow;
- start from a disposable Project/Part unless a step explicitly asks for Save/Reopen of that Part;
- use ordinary product UI/Command Line rather than test-only/private APIs.

## Workflow

### A. Toolbar and basic Fillet create

1. Open/create a Part with a simple box-like Body whose ordinary material Edges are easy to select.
2. Confirm the Part toolbar groups actions as Create: Sketch / Datum Plane / Extrude / Revolve and Modify: Fillet / Chamfer.
3. Select two admissible disconnected Body Edges, then start Fillet.
4. Confirm Operations shows selected Edge count, Radius, status, Clear, Finish and Cancel.
5. Change Radius and confirm exact preview updates without changing authored history.
6. Finish and confirm exactly one Fillet Feature appears and one Undo removes it; Redo restores the same Feature identity.

### B. Command-first, connected corners and explicit input meaning

7. Start Fillet with no selected Edge. Add two adjacent Edges sharing a corner by clicking them.
8. Confirm clicking an already selected admissible Edge toggles/removes it and duplicate input cannot appear.
9. Restore the two-edge set and Finish a valid connected-corner Fillet.
10. Repeat with three Edges meeting at a common trihedral corner where geometry permits.
11. Exercise a disconnected or mixed connected/disconnected explicit set and confirm it is one Feature, not hidden one-Edge Features.
12. Where a tangent neighbor exists, select only one bounded Edge and confirm SimpleSolid does not silently author extra tangent-chain members. Explicitly selecting the complete desired chain remains allowed.
13. Enter FILLET through Command Line and confirm the same draft/status/Finish/Cancel behavior is used.

### C. Chamfer parity

14. Repeat selection-first Chamfer with multiple explicit Edges and one common Distance.
15. Repeat command-first Chamfer and Edge toggle/Clear behavior.
16. Exercise a common connected corner and a disconnected/mixed explicit set where geometrically valid.
17. Enter CHAMFER through Command Line and confirm parity with GUI.
18. Enter an excessive/impossible Distance and confirm the Feature reports Failed / cannot Finish; no partial result is committed.

### D. Edit and explicit repair

19. Select an existing Fillet in Tree/Properties and start Edit Fillet.
20. Confirm the original complete Edge set and Radius return, and the viewport presents the Body stage immediately before that Feature.
21. Change Radius, Cancel, then Edit again. Confirm Cancel changed nothing.
22. Edit again, change Radius and/or explicitly remove/add an Edge, Finish, and confirm the same FeatureId is preserved in one Undo step.
23. Repeat the identity-preserving edit path for Chamfer.
24. Create or induce an upstream change that removes one consumed Edge meaning. Confirm the downstream edge Feature remains authored and becomes Missing / Blocked, not rebound to a nearby Edge.
25. Enter Edit/repair, confirm the failing input is identified, explicitly Clear/remove/replace it with an admissible Edge from the exact predecessor stage, then Finish and confirm the same FeatureId is repaired.
26. If an Ambiguous/Unsupported fixture is available, confirm it is reported distinctly and likewise requires explicit repair.

### E. Feature lifecycle and downstream history

27. Suppress a Fillet/Chamfer and confirm FeatureId/input intent remains while its contribution disappears.
28. Unsuppress and confirm reevaluation from current authored state.
29. Delete an edge Feature that has a downstream consumer and confirm the downstream Feature resolves/fails against changed history rather than using stale last-good geometry.
30. Undo/Redo Suppress/Delete/Edit and confirm exact identities/input sets return.
31. Confirm a failed/blocked active Feature never exposes stale previous B-Rep as current valid Body truth.

### F. Chaining and cold persistence

32. Create a valid Fillet -> Chamfer chain by selecting an ordinary generated engineering Edge of the Fillet for the Chamfer.
33. Create a valid Chamfer -> Fillet chain analogously.
34. Save the Part, close it, then reopen it.
35. Confirm both chains rebuild successfully from authored intent with the same FeatureIds, explicit Edge sets and parameters.
36. Confirm no prior preview/picking/provider runtime identity is required after reopen.
37. Save/reopen a Part containing a deliberately Missing/Blocked edge Feature and confirm its authored intent/status survives for explicit repair.

### G. Documentation / Product Browser

38. Open the generated Product Browser.
39. Confirm PL and EN Part docs describe Fillet/Chamfer as explicit multi-Edge Features with constant Radius / equal Distance.
40. Confirm docs describe selection-first/command-first, Operations/Command Line parity, Edit preserving FeatureId, explicit repair, Suppress/Delete/Undo/Redo and Save/Reopen.
41. Confirm docs do not claim variable-radius/full-round/face Fillet, distance-angle/asymmetric Chamfer or automatic tangent-chain authoring.

## Result

Owner result: PENDING.

When testing is complete, report either PASS — no PM-05 functional blocker found on the exact final candidate — or FAIL with the failing workflow step, observed behavior and reproducible conditions.

A FAIL returns PM-05F to bounded remediation followed by a fresh exact-head FULL and repeat Owner workflow. Only explicit Owner PASS closes PM-05.

## Completion boundary

Until Owner PASS, PM-05 remains ACTIVE, PM-05F remains the current checkpoint, PM-06 is not activated, and no advanced Fillet/Chamfer variant is implied by this workflow.
