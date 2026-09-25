# WB-01B — Native Viewport Overlay Composition Stabilization

**Status:** PROPOSED  
**Owner acceptance:** pending  
**Decision class:** D1 local design + implementation; stop for D2 if ownership/public Viewer contracts must change  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0003, ADR-0006  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.1  
**Roadmap impact:** none; R4 remains completed and R5 remains next but inactive

## 1. Context

Manual Windows validation after SK-04C exposed a shared rendering defect in overlays above the native Qt/OCCT viewport.

Observed behavior:

- Sketch rectangle selection is semantically correct, but while dragging the selection box the 3D surface can disappear or reveal black/background/Workspace pixels until later repaint;
- ViewCube can also leave stale visual artifacts;
- the existing ViewCube `scheduleUnderlayRefresh()` workaround therefore cannot be treated as a proven fix.

The production Qt/OCCT viewport is a native `WA_PaintOnScreen` / `WA_NativeWindow` child. Current overlays combine ordinary Qt child widgets with that native surface.

The defect is presentation/runtime-only. No authored CAD state, Sketch semantics, commands, persistence or durable identity are implicated.

## 2. Goal

Make overlay composition over the native Qt/OCCT viewport visually stable on the supported Windows path before R5 begins.

At minimum the fix must cover:

- ViewCube;
- Sketch Window/Crossing selection-box presentation;
- showing, moving, updating and hiding those overlays;
- Workbench resize/splitter changes while overlays are present;
- repaint after overlay removal;
- ordinary Pan/Orbit/Zoom and Sketch selection semantics remaining unchanged.

The work must preserve the current provider-neutral Viewer/application semantics.

## 3. Critical correction to current documentation

Current internal documentation states that the ViewCube coalesced underlay repaint prevents stale pixels. Manual evidence shows that statement is stronger than the proven implementation state.

WB-01B must correct the as-built documentation so it describes only behavior actually verified by the completed implementation.

## 4. Architectural boundary

This contract authorizes a bounded provider/UI stabilization only.

It may change private Qt/OCCT and Workbench overlay implementation mechanics, including:

- repaint/invalidation sequencing;
- native-window update scheduling;
- overlay widget attributes;
- overlay geometry/update batching;
- replacing duplicated overlay invalidation code with a small bounded helper;
- provider-local drawing strategy where ownership and public contracts remain unchanged.

It must not change:

- durable CAD identity or authored state;
- Part/Sketch domain semantics;
- `SketchInteractionState` semantics;
- selection grammar;
- DocumentSession commands/history;
- persistence;
- public provider-neutral Viewer meaning;
- Workbench/Operations ownership from ADR-0006.

If evidence shows that a reliable fix requires moving ViewCube ownership into the Viewer provider, changing public Viewer APIs, introducing a new cross-provider overlay subsystem, or otherwise changing subsystem ownership/dependency direction, stop and propose that as D2 rather than doing it implicitly.

## 5. Investigation requirement

Do not assume the existing ViewCube workaround is correct.

Before selecting the implementation, reproduce and compare at least these cases on the real Windows Qt/OCCT path:

1. ViewCube show/move/resize/hide over a rendered viewport.
2. Selection-box show/update repeatedly during LMB drag/hide on release.
3. Overlay area revealing stale pixels, black background or parent/Workspace content.
4. Narrow/wide splitter resize with ViewCube visible.
5. Selection rectangle overlapping and not overlapping the ViewCube region.

The implementation should address the common composition mechanism, not add unrelated one-off fixes unless evidence proves the defects have separate causes.

## 6. Selection-box invariants

The existing R4 selection behavior remains frozen:

- drag threshold unchanged unless evidence proves it directly causes the visual defect;
- Left→Right remains Window;
- Right→Left remains Crossing;
- rectangle selection replaces semantic selection;
- no primary EntityId is assigned by rectangle selection;
- semantic query result and Delete behavior remain unchanged.

The fix must not modify semantic selection merely to improve painting.

## 7. ViewCube invariants

ViewCube remains:

- a runtime navigation control;
- non-authored;
- independent from CAD identity;
- capable of standard views, projection and Fit;
- responsive to the existing Workbench layout.

The fix must not turn ViewCube state into persisted model state.

## 8. Verification strategy

Automated tests remain necessary but are not sufficient because the reported failure is visual composition of native Windows surfaces.

Required automated coverage:

- existing full regression suite remains PASS;
- ViewCube layout/interaction regressions remain PASS;
- native Qt/OCCT selection-box lifecycle remains PASS;
- show/update/clear operations remain non-mutating;
- no crash during repeated overlay lifecycle and Workbench resize stress.

Required manual Windows verification on the exact implementation build:

- drag Window and Crossing rectangles repeatedly over rendered Sketch geometry with no 3D surface disappearance, black reveal or Workspace/background reveal;
- rectangle overlay clears without stale pixels;
- ViewCube leaves no stale footprint after resize/move/compact transitions;
- repeated narrow↔wide splitter changes leave the underlying 3D image coherent;
- Pan/Orbit/Zoom/ViewCube and rectangle selection still behave correctly.

Manual visual verification must be recorded explicitly in closeout evidence. A green CTest run alone must not be claimed as proof that visual artifacts are gone.

## 9. Expected implementation surface

Expected changes are bounded to:

```text
src/viewer_qt_occt/**
src/ui/view_cube_widget.*
src/ui/cad_workbench.* only if overlay host/repaint wiring requires a narrow adjustment
tests/**
docs/internal/CAD_WORKBENCH_VIEWER.md
docs/internal/BUILD_AND_TEST.md if test coverage changes
work/**
```

No Sketch Core, Part model, persistence or product workflow changes are expected.

## 10. Deliberately out of scope

```text
R5 grips/direct manipulation
new Sketch selection semantics
new CAD tools
new commands
coordinate input
snapping/inference
constraints
new Viewer public feature framework
general cross-platform compositor rewrite
replacement of OCCT
visual redesign of ViewCube
visual redesign of selection colors/styles
```

## 11. Acceptance

WB-01B completes only when:

1. the common native-overlay failure mode is reproduced and the chosen fix is justified by evidence;
2. Sketch selection-box drag no longer causes viewport disappearance/background reveal in manual Windows verification;
3. selection-box clear leaves no stale pixels in manual Windows verification;
4. ViewCube no longer leaves stale artifacts in the reproduced resize/move scenarios;
5. existing ViewCube navigation remains functional;
6. existing Window/Crossing semantic selection remains unchanged;
7. overlay lifecycle creates no authored mutation/history;
8. automated native stress/regression coverage is strengthened for repeated overlay show/update/hide and resize;
9. full exact-head Windows gate passes;
10. required internal documentation is corrected/current;
11. closeout records explicit manual visual verification in addition to CI evidence;
12. R5 remains inactive after this fix.

## Documentation Impact

Internal docs: required  
User/Product docs: not required  
Reason: this is a runtime Viewer/Qt composition stabilization with no intended user workflow or CAD semantic change; internal as-built claims about overlay repaint correctness must be corrected.

## 13. Completion boundary

Completion of WB-01B restores a clean post-R4 checkpoint.

It does not activate R5. Any R5 implementation still requires a separate explicit Owner-accepted Work Contract.
