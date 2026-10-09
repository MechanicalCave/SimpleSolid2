# PG-01B — Same-Part Associative Sketch Projection Work Contract

**Status:** ACTIVE — OWNER D2 + WORK CONTRACT ACCEPTED 2026-10-09; PG-01B only
**Prepared:** 2026-10-09
**Baseline:** main `27268d4ec10fbc09721a16ab8f0e2d59bb54833f` (PG-01A Owner final PASS, FULL #1932 195/195)
**Foundation:** §7.2/§7.5 accepted Part-associative DIRECTION, §7.3 Assembly snapshot unchanged
**Program:** Part Modeling v1 roadmap v1.30, Project Geometry before PM-06
**D2 authority:** `work/PROJECTION_01B_ASSOCIATIVITY_DESIGN_PROPOSAL.md` §7 five decisions explicitly Owner-accepted on 2026-10-09 (`zatwierdzam - kontynuuj`)
**Current phase:** B1/v15 and B2 PASS, B3 ordered Feature + real OCCT linked Cut and Extrude Edit Preview PASS (#1951), B4 atomic multi-Edge batch, Undo/Redo/cold reprojection and fail-closed structural editing/Break Link PASS (#1952–#1957). Remaining targeted Revolve Draft preview native proof and B5 exhaustive cold Feature-chain acceptance; broader FAST, final exact-head FULL and Owner final PG-01B PASS OPEN. Evidence: `work/PROJECTION_01B_PROGRESS_EVIDENCE.md`.
**Activation:** `work/ACTIVE.yaml` references THIS Work Contract after a dedicated activation commit. Source changes can follow activation only within PG-01B B1–B5. PG-01C/D/E remain separately gated.

## Goal and scope

Implement the **headless, durable same-Part associativity semantics** for exact Project Edge into Sketch, consuming PG-01A and reusing strict `MaterialEdgeReference`:

1. Part-owned, validated mapping of current Sketch EntityId to prior BodyStageRef/MaterialEdgeReference.
2. Pure revision-bound evaluation into effective Sketch geometry (current real Line/Circle/Arc), with typed Missing/Ambiguous/Unsupported/Provider/Cycle diagnostics and strict stage ordering.
3. Profile region detection **and actual Extrude/Revolve Kernel inputs** use the same evaluated Sketch; dependent Feature evaluation and preview are not allowed to consume the authored seed.
4. Headless transactional batch link-creation, change-role where already authorized, geometry-edit protection and Break Link/Erase actions with stable IDs and one Undo/Redo.
5. Versioned .ss2part schema **v15 proposed subject to D2 approval**, older v14 compatibility, deep validation, real cold Save/Close/Reopen and missing-source repairability.

## Authorized source scope after activation ONLY

- `src/part/**`: PartSketch metadata, strict source-stage resolver, ordered dependency guard, derived Sketch/Profile materialization, result diagnostics, reconstruction/validation and v15 persistence.
- `src/application/**`: bounded semantic DocumentSession Command(s) with revision, atomic history, Break Link; existing preview adapter input only when needed to ensure no stale Profile path.
- `src/sketch/**`: smallest viable support to derive a copy of SketchModel with original EntityIds and curve/role; no generalized solver or rewrite of Shared 2D.
- `tests/**`: real strict semantic core+native fixtures, Part/DocumentSession/profile/evaluator and cold storage matrix. Reuse existing targets where feasible; new tests/CMake registration allowed only if genuinely necessary and batched once with clear clean FULL cost.
- `docs/internal/**`, generated `docs/browser/index.html`, bounded `work/**` evidence and acceptance.
- A small provider-neutral engine adapter handoff via existing `IEdgeProjectionQuery` is permitted; no new OCCT algorithms or generalized Kernel API work in this package.
- **Prohibited:** `src/ui/**`, `src/viewer/**`, one right-panel tool or picking protocol (PG-01C), planar Face auto-boundary projection/holes/partial skip UI (PG-01D), Assembly cross-Part links, spline/ellipse approximations, universal topology matching, Foundation/roadmap semantic changes or CI policy weakening.

## Work breakdown and intermediate gates

**B1 / D2 frozen schema & state:** Part-owned `SketchId + EntityId + MaterialEdgeReference`, structural validation, strict edit protection, transaction layout, proposed schema v15 and backwards migration; tests only for authored/structural rules. No query fallback to authored geometry.

**B2 / evaluation core:** exact stage-bound resolution from `FeatureEvaluation::result_topology/result_solid`, PG-01A scoped token, pure effective Sketch; typed bad-source diagnostics and source→consumer ordering/cycle rejection, no cache current-truth. Core fake provider plus production OCCT tests.

**B3 / effective Profile:** route **all** Region/Profile/Extrude/Revolve and Edit/Preview paths through effective Sketch; test real dependent A→B changes, support frames, missing-source failure that blocks only affected Profile/Feature. Do not duplicate the current Part/Profile region analyzer.

**B4 / command lifecycle:** atomic multi-entity links, stable EntityIds, Delete/Broken/Break Link on resolved current curve, role Regular/Construction, undo/redo and edit guards; no UI tool yet.

**B5 / persistence and acceptance:** schema v15 cold rebuild and v14 legacy load, malformed-negative cases, missing-but-well-formed source remains repairable, deterministic rebuild from fresh Kernel, internal as-built + Browser, native Windows FOCUSED → FAST → one exact-head Ready-for-Review FULL.

If implementing a subphase exposes a new D2 decision, STOP and seek explicit Owner approval before broadening the contract. Green earlier subphases do not silently activate PG-01C/01D.

## Must-pass acceptance matrix

- Native OCCT projection from real previous Feature Body stage only; strict material Edge identity, no semantic nearness, and runtime numeric token collision cannot cross-retarget.
- Line, Circle, Arc linked Regular/Construction with correct role-driven Profile eligibility; stable EntityId across param changes, matched source stage & current Sketch frame.
- Modified source Feature recomputes effective Sketch and downstream Extrude and Revolve with **no authored Sketch mutation**; stale seed never produces an UpToDate Body or preview.
- Type switch/degenerate/unsupported/missing/ambiguous/cycle results in explicit safe Broken, no unexpected entity deletion, no implicit same-looking remap and no silent Break Link.
- Deleted/Suppressed/Failed source and missing Material Edge differentiate structural serialization validity vs runtime repairable broken state; unrelated Profile functionality unaffected; proper downstream failure.
- Edit commands do not change geometry of still-linked source-controlled entities. Undo/Redo captures exact binding/seed transition. Break Link keeps target ID/role/compatible constraints and copies only latest resolved curve; broken Break Link is mutation-free failure.
- v14 file loads without binding; v15 file round-trips stable IDs, binds and roles. Fresh provider + cold restart results match resolved model, no saved runtime tokens, no auto-updating Face members.
- Core-only, existing native kernel, Windows Desktop regression, internal docs/build Browser checks and aggregate Ready-for-Review FULL on exact SHA.
- Tested error scenarios do not depend on Qt selection tokens, viewport tessellation, or source geometry fallback; no Team/Assembly runtime dependency.

**Current B5 verification:** native cold linked Cut + suppressed source #1959 PASS 1/1, real Revolve Create Draft #1960 PASS 1/1, upstream source width 40→50 mm fail-closed/repair + v15 fresh-provider replay #1961 PASS 1/1. Earlier wider FAST #1958 RED 92/93 on untouched Viewer marker test; current-head broader FAST, final exact-head FULL and Owner final acceptance are still pending.

## CI efficiency and evidence

Use FOCUSED trailers with exact registered targets during iteration, and persist Windows warm trees. If first new test requires CMake, collect one justified registration change rather than compromising code organization. Close with FAST full regression then exactly one justified full native Desktop verification at Ready-for-Review on stable PR head. Maintain one evidence report with RED→fix timeline, exact SHAs, counts, docs and cold path; do not claim GUI product behavior before PG-01C.

## Owner gates

Owner explicitly approved D2 choices plus this contract 2026-10-09. A distinct activation commit changes `work/ACTIVE.yaml` to this contract **before code changes**; implementation is bounded to PG-01B. Completion requires evidence, final exact-head FULL, and separate Owner PASS/merge authorization. Do not start PG-01C or PG-01D automatically.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: PG-01B adds durable Part/Sketch association and evaluator internals, without exposing a new end-user Sketch tool. Internal as-built docs, schema changes and regenerated offline Browser are required; end-user PL/EN docs become mandatory with the shipping PG-01C/PG-01E workflow.
