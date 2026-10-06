# PM-04D — Revolve Draft / Operations / Command Line / Preview Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Checkpoint split:** D1 shared draft/commands/exact preview + D2 Workbench/Operations/CAD Input/Viewer/Tree  
**Date:** 2026-10-06

## Runtime evidence

### PM-04D1 — shared Revolve draft / commands / exact preview

- exact candidate: `24975b9bdd8f2e97cc11f2c7fef77bdbc9c48de3`;
- Windows FULL #1551: PASS;
- merged main: `84bddafb57eab4f7776b78621b81369c2917899b` (#267).

D1 delivered:

- one shared runtime `RevolveDraft` for command-first, selection-first and Edit flows;
- defaults Add / OneSide / 360 deg / Reverse=false with no inferred Axis;
- exact Profile + Axis acquisition through durable `ProfileId + AxisReference`;
- OneSide / Midplane, Reverse and legal `0 < Angle <= 360 deg` mutation;
- create/edit semantic commands through `DocumentSession`;
- one successful Finish = one Part transaction and one Undo entry;
- stale document/revision/draft-generation/evaluation rejection;
- Edit preserves FeatureId;
- provider-neutral exact preview delta through `revolvePreviewMesh()`;
- preview and committed modeling reuse the same resolved `AngularRevolveInput`;
- Add preview = exact added material and Cut preview = exact removed material, with no raw-tool fallback.

### PM-04D2 — Operations / CAD Input / Viewer / Tree

- exact candidate: `c275045387b7fa928dcc955ba3837565796b1328`;
- Windows FULL #1555: PASS;
- final `windows-msvc` aggregate: PASS;
- merged main: `92348a8af84679ee2ce77b099d9c337dfec2dc84` (#268).

D2 delivered:

- Revolve Part Modeling tool and Operations surface;
- Profile and explicit Origin/Authored Axis acquisition in either meaningful source order;
- Add/Cut, One Side/Midplane, Angle, Reverse, Finish and Cancel controls;
- CAD Input and Dynamic Input parity over the same runtime draft;
- Enter/Esc Finish/Cancel and stale CAD-context generation rejection;
- exact additive/subtractive preview delta in the Viewer;
- runtime-only source-Axis visibility/emphasis, including authored-hidden Axis sources, without mutating authored visibility;
- source Profile preview/presentation behavior;
- Revolve Edit through the same draft with FeatureId preservation;
- mutual exclusion with conflicting modeling contexts;
- Revolve Feature projection into the Part Tree with type/source/status diagnostics and Revolve-aware Edit routing.

The final exact-head FULL run proved:

- core-only: 25/25 tests PASS;
- kernel-native Release: 47/47 tests PASS;
- complete desktop CTest: 102/102 tests PASS;
- dedicated `pm04d2.cad_workbench_revolve`: PASS.

The exact candidate includes two gate-driven remediations discovered before merge:

- deterministic normal-desktop test layout instead of relying on the offscreen platform's default narrow top-level size;
- production Tree support for `RevolveFeature`, which was required for the complete GUI Edit path and was not replaced by a test workaround.

## Architecture boundary

PM-04D does **not** add:

- Revolve persistence schema or schema migration;
- Save / Close / Reopen or cold-rebuild proof for Revolve;
- Axis/Revolve missing-reference repair completion;
- new Axis source kinds such as Datum Axis or Body Edge/Curve;
- Projection;
- provider identity or geometry-similarity rebinding;
- persisted periodic seam identity;
- multi-Body semantics.

Those remain owned by PM-04E or separately gated future work.

## Result

PM-04D is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-04E — integrated lifecycle / repair / persistence** under the unchanged Owner-accepted PM-04 Work Contract.
