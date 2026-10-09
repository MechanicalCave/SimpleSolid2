# PG-01C — C0 linked Sketch current-truth audit and implementation evidence

**Status:** C0 IN PROGRESS — Owner-approved PG-01C C0–C4 only
**Date:** 2026-10-09
**Baseline:** main `83af8e62fabbe1056705b255ef5a0d6256c2a6e0`, Owner-approved D2-A–E active contract `work/PROJECTION_01C_PROJECT_EDGE_UI_CONTRACT.md`
**Known separate CI issue:** #302 FAST CTest registry vs aggregate target; do not weaken or fix in this package

## Current truth leak inventory (read directly from main)

- `src/ui/part_viewport_controller.cpp::refreshPresentation`: builds Body first, then Profile and Sketch scenes. Thus the same-revision `part_evaluation_cache_` is available to link rendering if the Body evaluation completed.
- `buildSketchScene`: currently renders `hosted->model.state()`, which is authored seed for linked Line/Circle/Arc. This is stale after an upstream Feature edit; replace with PG-01B `evaluateEffectiveSketchProjection` from the same upstream evaluation and exact provider. When no current provider/evaluation exists, hide linked seed geometry, not ordinary unlinked geometry.
- `buildProfileScene`: currently calls `PartDocument::evaluateProfile(profile.id)`, intentionally fail closed when linked boundaries are referenced. Need current effective Sketch `resolveProfileRegionIntent(model, intent)` and sampling from the **same derived model**, not `source.model`. Invalid linked intent remains absent, not drawn from stale seed.
- `buildProfileRegionPresentation`: `sampleProfileUse(source.model,...)` must take the resolved source model argument so displayed fill and pick points agree.
- `projectSketchInteraction`: draws line/circle/arc grips from `hosted->model`; until current-provider-aware linked manipulation is explicitly supported, **linked entity grips must not display editable stale seed**. Preserve ordinary unlinked grips and EntityId selection.
- `PartSketchInteractionController`: other interaction routes (Snap, Measure, source geometry transformation, structural edits, profile authoring) need explicit current-model-or-reject audit. Do not claim all snappable linked geometry complete until provider-derived interactive queries are proven.
- Viewer presentation contracts lack orthogonal linked styling; D2-C permits only a narrow runtime flag/style in a future gated C2 slice.

## Current implementation checkpoint

- `PartViewportController` now derives disposable current linked Sketch models from the same revision-bound Part evaluation and exact edge-projection provider, strips linked seeds on unavailable source, and uses the resulting model for Sketch scene, Profile region/fill and Profile draft sample; linked target grips remain non-editable.
- Added native Qt controller regression `sk04b.part_viewport_selection_bridge` with a structurally valid linked Circle bound to an upstream Feature but no active projection provider. The stale authored Circle must be hidden, unrelated local Line visible, and the linked Profile never falsely presented. This is a **focused subset**, not full PG-01C GUI acceptance.
- FAST #1980 was RED at C++ compilation: changed Profile presentation signature left two preview callers and a lambda capture stale. Both issues were explicitly corrected, not suppressed. Run current exact-head FOCUSED regression before further expansion.
- FAST registry issue #302 is independent and remains RED. Do not infer one focused green test means all FAST/GUI gates pass.

## C1 native Workbench first integration (unverified until exact-head Windows)

- Added Sketch Modify toolbar launcher `projectEdgeToolButton` and right Operations `projectEdgeOperationsWidget` with stage, Edge count, Regular/Construction, Remove/Clear, Finish/Cancel. One transient `CadWorkbench` state shared by direct buttons and global `PROJECT/PROJECTGEOMETRY` Command Line tokens.
- Existing Body stage-scoped semantic `selectedMaterialEdgeReferences()` and `setBodyTopologyEdgeDraftMode()` are reused, never storing Viewer token as persistent Part data; default source stage is visibly declared from the **current** complete Body topology, and exact command guards stale DocumentRevision/SketchId/source-stage and current selection before atomic Finish.
- Command Line text `FINISH`, empty Enter, `CANCEL`, role keywords, REMOVE/CLEAR, viewport Esc/Enter and toolbar Cancel use one Finish/Cancel code path; Sketch Select/tool change, document deactivate, Undo/Redo clean pending state.
- **Limitations not covered by this slice:** current exact pre-Finish projection preview and linked visual styling, durable Properties/Break Link action, positive provider-supported native Edge clicking while Sketch is active, full focus/Esc hierarchy and interactive snapping. These remain mandatory C2–C4 before feature acceptance.
- This remains a **draft**, not a claim that C1 product UX is complete. Native focused compilation/negative routing and later real OCCT Qt tests must prove the implementation.

## Validation obligations

1. A real upstream OCCT Line/Circle/Arc source with a deliberately different persisted seed: its linked Sketch display and Profile fill must match **current** derived geometry and source stage.
2. Source suppressed/missing/ambiguous/unsupported/provider failure: linked geometry is not drawn, snapped, used as a Profile or detached from last-good geometry. Unlinked geometry remains visible.
3. Undo/Redo, Save/Close/Reopen on a fresh OCCT kernel, stage/revision mismatch, invalid Sketch support, display scene failure and generation changes cannot resurrect a stale linked curve.
4. Native Windows FOCUSED Qt/OCCT regression on exact commit, final package FAST and FULL still required, including panel/Command Line/Esc/Enter/Cancel parity.

**STOP:** This audit alone does not satisfy PG-01C; current visible behavior remains incomplete until all linked source model readers are resolved or fail closed, and one coherent right-panel tool plus Owner native GUI acceptance are proven.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: current audit records engineering progress; shipping PG-01C additionally needs current as-built and bilingual PL/EN user documentation plus generated Browser.
