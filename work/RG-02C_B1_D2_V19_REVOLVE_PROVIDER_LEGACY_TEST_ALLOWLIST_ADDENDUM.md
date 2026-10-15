# RG-02C/B1 — v19 legacy test/writer checks and scoped Revolve provider: technical D2 allowlist addendum

**Status:** narrowly-scoped OWNER-DELEGATED IMPLEMENTATION ADDENDUM to accepted active RG-02C/B1 one-`Edge` Work Contract; pending independent governance merge and `ACTIVE.yaml` addendum history. No product mutation authorization for the three added paths until these gates complete.
**Date:** 2026-10-15.
**Authority:** Owner's explicit `masz wszystkie niezbędne zgody do uruchomienia implementacji - kontynuuj prace i nie zatrzymuj z formalnych powodów`, separately accepted Owner UX for new Work Features Axis with **only Constructor Edge** (existing Sketch-Line Axis remains under Sketch), accepted `work/RG-02C_B1_SINGLE_STRAIGHT_EDGE_AXIS_LIFECYCLE_D2_WORK_CONTRACT_PROPOSAL.md` §0A, activated `work/ACTIVE.yaml`, Constitution/Foundation 1.0 and Part roadmap 1.33. This ADDENDUM fixes three paths omitted from the original finite allowlist after inspecting actual production Revolve and legacy fixtures. It changes no policy, direction, frozen scope or authoring contract.

## Exactly three additional production paths

1. `src/part/feature_evaluation.cpp` — **only** thread the existing exact native `kernel::IExactBoundedEdgeGeometryQuery` capability from current `ISolidModelingKernel` through the internal `resolveKernelRevolveInput` helper into `resolveAxisReference` for a stage-qualified `StraightMaterialEdgeAxisSource`. It must pass the current strictly upstream prefix `PartEvaluation`, existing Datum evaluation and optional scoped provider; if the provider is unavailable, **fail closed**, never snapshot a native edge/XYZ or reinterpret source stage. Preserve all existing Sketch Line, builtin axis and signed Revolve behavior. No kernel API change, no separate resolver/transaction or new public feature dependency framework.
2. `tests/pm02e_sketch_support_schema_v9_test.cpp` — change ONLY assertions/fixtures that refer to the **new current Part native writer** v18 to v19 as appropriate, with explicit preserved older versioned reader scenarios. No masking historic v9/legacy tests, no changing 2D/Sketch support logic, no skipped test.
3. `tests/pm05b3_generated_surface_reference_v14_test.cpp` — same bounded *current writer* v19 migration for exact asserted version only. Retain actual v14 and legacy native fixtures and all generated-surface reference assertions; no new target/CMake/CI changes.

All other production paths remain governed by previously accepted §0A. This addendum does **not** authorize `src/kernel/**`, `src/kernel_occt/**`, `src/viewer/**`, CMake, workflow, external providers, unrestricted new Axis sources, generic Point/MID/CENTER, G1, PM-06, Mirror/Pattern or silent format fallback.

## Why these are needed

Existing internal `resolveKernelRevolveInput` calls `resolveAxisReference(document,feature.axis,prefix,datums)` without the native provider, which is correct for authored Sketch Line, but **always blocks** the new strict Edge Axis; it cannot ship the Owner-approved complete Edit/Consume/Revolve life cycle without the provider pass-through.

The already registered PM-02E and PM-05B3 tests contain exact writer-schema `== 18` checks, which would become false when the accepted **v19 discriminated Axis source writer** is introduced, even for Part documents still using only Sketch Line Axes. Only update factual current-writer checks, never rewrite older fixtures' schema fields or weaken read/migration coverage. The original §0A already authorizes PM-04A/PM-04E1's analogous current-writer checks.

## Owner-delegated acceptance, validation and STOP

This is a D2-aligned missing-path correction under the Owner's explicit implementation authority, not a new product feature. Stage-specific `MaterialEdgeReference` remains Part-owned; exact provider returns only transient derived unoriented lines; `AxisId`, old Sketch-Line Axis Tree location and one-item `Edge` UI stay unchanged.

- Windows exact-head governance CLOSURE on this single `work/**` addendum; separate guarded squash on protected main; then an `ACTIVE.yaml` history-only acknowledgment with its own exact-head governance/guarded squash.
- Only after both have merged can H2+ production branch write those three paths. Existing B1 H1/H2 draft source work continues solely within §0A until then.
- Run registered Windows FOCUSED native Axis/revolve/schema, Draft FAST, Ready FULL Core+native Kernel+Desktop Qt/OCCT including untouched legacy fixtures and DOC-01. Evidence must report any actual RED/GAP. Existing A0 real native TopoDS Edge orientation-reversal fixture is still a GAP; it cannot be claimed PASS merely from `+t/-t`.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: this amendment extends the allowed implementation surface of the already accepted end-to-end B1 product. Actual `docs/internal/**`, PL/EN `docs/product/**` and officially generated Product Browser remain required by the parent contract at shipping. This one-file governance addendum does not change current product behavior or as-built docs.
