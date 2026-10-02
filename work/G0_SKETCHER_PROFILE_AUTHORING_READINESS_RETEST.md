# G0 — Sketcher Profile-Authoring Readiness Re-test

**Status:** READY FOR OWNER MANUAL RETEST  
**Date prepared:** 2026-10-02  
**Decision class:** evidence checkpoint; no production CAD scope activated  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.1  
**Readiness criteria authority:** `work/SKETCH_ROADMAP.md` v1.9  
**Current main at preparation:** `c49540c31346d7d7269fbfdd9b10d521d6a4ccc9`

## 1. Purpose

G0 is the required evidence checkpoint between the completed Sketcher stabilization sequence and any Part Modeling architecture work.

The checkpoint answers one question:

> Can the current Sketcher deliberately author practical valid Profiles on supported Windows with the accepted creation, precision, snap/inference, structural-edit, history and persistence behavior?

G0 is not a Work Contract and authorizes no production mutation.

A PASS does **not** activate PM-00A, Part Feature Tree, Body/Feature persistence, Kernel solid operations or solid modeling. PM-00A still requires its own Owner-accepted bounded Work Contract.

A FAIL does not authorize an implicit fix. Record the concrete finding and classify it into separately accepted bounded stabilization work before repeating G0.

## 2. Authoritative minimum criteria

Per Sketcher Roadmap v1.9, minimum Windows evidence must demonstrate:

1. practical creation/editing of Line, Circle, Arc and Rectangle;
2. usable Regular/Construction workflow;
3. read-only geometric inspection/measurement;
4. precise numeric/unit-aware point entry;
5. Polar and Dynamic Input;
6. OSNAP/Tracking/Inference sufficient for deliberate geometric placement;
7. Trim/Extend/Extend-Both with accepted stable identity/history behavior;
8. Undo/Redo and Save/Close/Reopen across representative workflows;
9. deliberate creation of valid Profile geometry without automatic gap healing or Viewer-dependent closure.

Not required for G0:

- authored constraints/solver;
- ordinary Select RMB context menu;
- planar-face Sketch support;
- Split/Join;
- standalone Show Dimensions;
- Part Feature Tree;
- solid modeling / Extrude;
- every future editing tool.

## 3. Runtime-equivalence evidence

The last final runtime candidate manually verified for SR-03 was:

`e96627ce1268e05f11a334d3d8f884dae854967e`

with Windows FULL #1187 PASS and Owner manual Windows PASS.

After SR-03, repository changes through Part Modeling Roadmap v1.1 are governance/program changes. At G0 preparation, a recursive Git blob comparison between the SR-03 exact runtime candidate and current `main` verified **224/224 identical blobs** across:

- `src/**`;
- `tests/**`;
- `scripts/**`;
- `.github/**`;
- `benchmarks/**`;
- root `CMakeLists.txt`;
- `ss2.ps1`;
- `ss2.cmd`.

No production/build/test/workflow blob differs between the manually verified SR-03 runtime candidate and current main.

This supports reuse of the existing exact runtime FULL evidence for G0 preparation, but it does not replace the required integrated G0 manual workflow.

## 4. Historical component evidence

The current integrated product contains separately completed and manually accepted slices relevant to G0:

| Area | Completed package evidence |
| --- | --- |
| Rectangle + Regular/Construction | R9, Windows FULL #912 + Owner manual PASS |
| Numeric/unit-aware input, Polar, Dynamic Input | R10, final Windows FULL #1009 + Owner manual PASS |
| OSNAP / Tracking / Inference | R11, final Windows FULL #1088 + Owner manual PASS |
| Trim / Extend / Extend-Both | R12, final Windows FULL #1104 + Owner manual PASS |
| Profile interaction correctness / deletion / UX | SR-01, Windows FULL #1128 + Owner manual PASS |
| Interaction/presentation latency and curve presentation | SR-02, Windows FULL #1169 + Owner manual PASS |
| Responsive Workbench composition | SR-03, Windows FULL #1187 + Owner manual PASS |

These results prove the individual delivered slices. G0 still requires a current integrated manual workflow because readiness is about the combined authoring experience.

## 5. Current automated regression coverage

The complete desktop regression at SR-03 was 83/83 PASS and core-only was 16/16 PASS. Current production/verification blobs remain identical to that candidate.

Representative regressions supporting the G0 criteria include:

- `sk01.workbench_sketch_host` — integrated active-Sketch Workbench command/tool routing, creation/edit surfaces, Profile context, precision/Polar/Dynamic/OSNAP/structural tool integration and history interactions;
- `sk02b.part_sketch_model` — authored Sketch model and Profile/RegionIntent behavior;
- `sk02b.part_sketch_persistence` — Part-hosted Sketch/Profile persistence;
- `sk04c.part_sketch_interaction_controller` — integrated creation/selection/edit interaction controller paths;
- `sk06a.circle_arc_model_persistence` — Circle/Arc authored model and persistence;
- `sk07f.precision_input_state` and `sk07f.precision_input_controller` — precision/Dynamic Input interaction;
- `r10.quantity_input` — unit-aware quantity input semantics;
- `r11.snap_core` — snap/tracking/inference core semantics;
- `r12.structural_edit_core` — Trim/Extend/Extend-Both geometry/identity outcomes;
- `r12.structural_edit_document_session` — structural-edit command/history semantics;
- `r12.structural_edit_persistence` — structural-edit persistence;
- `r12.structural_edit_interaction_state` — structural-edit interaction-state behavior;
- `cad_workbench_test` / Workbench regressions — current shell/history/focus behavior;
- native Viewer regressions — selection/navigation/presentation integration.

Automated evidence is necessary but is not sufficient for G0 PASS.

## 6. Owner manual G0 workflow

Run the current `main` on supported Windows.

Before testing:

```powershell
git switch main
git pull --ff-only
git rev-parse HEAD
.\ss2.ps1 run
```

Record the exact tested SHA. A later work-only governance suffix is acceptable only if production/verification blobs remain unchanged; otherwise repeat the relevant evidence on the new runtime candidate.

### G0-A — deliberate plate-style Profile

Create/open a Part and enter Sketch Edit on an Origin plane.

Demonstrate in one deliberate workflow:

1. Create a Rectangle as Regular geometry.
2. Use precise numeric/unit-aware input for at least one meaningful coordinate/distance.
3. Use Polar and Dynamic Input during deliberate placement.
4. Add a Circle as Regular geometry inside the rectangle so the resulting region intentionally contains a hole.
5. Add Construction geometry (for example rectangle diagonals or independent construction geometry) and verify it is visibly distinct and excluded from Profile material-region meaning.
6. Use read-only Measure/inspection on meaningful geometry.
7. Exercise OSNAP/Tracking/Inference to place or edit geometry from semantic targets rather than visual approximation.
8. Confirm the intended Profile/region is valid and reflects the outer boundary + intended hole without relying on construction geometry.
9. Undo and Redo at least one authored edit; confirm the expected geometry/Profile returns with no unrelated mutation.
10. Save, close the document, reopen it and confirm authored geometry, roles and Profile result remain correct.

PASS requires deliberate creation without hidden gap healing and without relying on screen pixels/Viewer coincidence as closure authority.

### G0-B — Line/Arc structural-edit Profile

In a separate Sketch, deliberately create a closed Profile using Lines and at least one Arc.

The workflow must exercise:

1. Line creation;
2. Arc creation;
3. Endpoint/Center/Intersection or other applicable accepted snap/inference behavior;
4. at least one Trim;
5. at least one Extend;
6. Extend-Both to Virtual Intersection on two Lines where both require extension;
7. an Undo/Redo across a structural edit;
8. final valid Profile recognition after the intended geometry is exactly closed;
9. deliberate small real gap or otherwise invalid closure as a negative control: it must remain invalid and must not be silently healed;
10. correction of that invalidity through an explicit authored edit, after which the Profile becomes valid.

The structural-edit behavior must preserve the accepted identity/history rules: no hidden Split/Join behavior, no automatic RegionIntent rebinding and no unexpected extra authored transactions.

### G0-C — practical editing and current shell integration

Across G0-A/G0-B, also confirm:

- Line, Circle, Arc and Rectangle are practically creatable and editable;
- Regular/Construction state is understandable before/after authored changes;
- selection/grips and current tool remain usable;
- Command Line and viewport keyboard-first input work as intended;
- no stale preview survives Cancel/Undo/Redo/tool switch;
- current responsive Workbench does not hide required authoring controls during ordinary use;
- no obvious latency regression makes normal Profile authoring impractical;
- Save/Close/Reopen does not depend on previous-process presentation state.

## 7. PASS / FAIL rule

### PASS

G0 may be marked PASS only when:

- Sections 2 and 6 are satisfied on one current integrated Windows build;
- no blocker is found that makes deliberate Profile authoring impractical or semantically unreliable;
- invalid/gapped geometry remains invalid until explicitly authored into a valid state;
- no observed behavior contradicts accepted identity/history/persistence rules;
- the exact tested SHA and Owner result are recorded in this document.

A non-blocking visual observation may be recorded separately only if it does not undermine any readiness criterion.

### FAIL

G0 is FAIL if any required criterion cannot be demonstrated, or if a workflow shows:

- silent gap healing;
- Viewer/pixel coincidence acting as closure truth;
- lost/corrupted Profile after accepted edit/history/persistence operations;
- structurally incorrect Trim/Extend identity/history;
- materially unusable precision/snap/inference workflow;
- current shell/input behavior preventing practical authoring;
- stale/incorrect state after Undo/Redo or reopen.

On FAIL:

1. record exact SHA;
2. record minimal reproduction and observed/expected result;
3. classify the finding;
4. do not activate PM-00A;
5. create/accept bounded stabilization work;
6. repeat G0 after stabilization.

## 8. Result record

**Current result:** PENDING OWNER MANUAL RETEST

Exact manual candidate: _pending_  
Owner result: _pending_  
Date: _pending_  
Observations: _pending_

## 9. Boundary after PASS

A G0 PASS changes only program scheduling/evidence:

- Sketcher profile-authoring readiness becomes satisfied;
- PM-00A may be prepared for explicit Owner acceptance;
- PM-00A remains inactive until separately accepted;
- PM-00A must begin with Phase A0 Verification Topology per Part Modeling Roadmap v1.1;
- PM-00B, PM-01 and solid modeling remain inactive.

No code or persistence/schema change is authorized by recording G0 PASS.
