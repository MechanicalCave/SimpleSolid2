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
