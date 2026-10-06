# PM-04A — Axis Semantic Model + Schema v12 Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-04_AXIS_REVOLVE.md`  
**Exact runtime candidate:** `9a01ec707c12ac699dcd1d5b9e1a4e7d8bc2578a`  
**Windows FULL:** #1531 — PASS  
**Merged main:** `4c6ed90e59e365fe6d53a88c68184b6332743749`  
**Date:** 2026-10-06

## Delivered

PM-04A establishes the durable semantic and persistence foundation for AxisReference without introducing Axis UI or Revolve runtime.

Delivered production semantics:

- Part-local durable `AxisId` and high-water `AxisIdCursor`;
- Part-owned authored Sketch-Line Axis records with name, source and independent visibility;
- `AxisReference` variants for built-in Origin X/Y/Z and authored AxisId;
- Origin X/Y/Z remain built-in semantic references and receive no synthetic AxisId records;
- deterministic authored Axis direction from source Sketch Line start -> end endpoint order after current Sketch support-frame resolution;
- Regular and Construction Sketch Lines are both admissible Axis sources without Shared-2D role conversion;
- current derived world Axis as origin point + normalized direction, with no provider/Viewer identity in authored state;
- structured Resolved / Missing / Unsupported / Blocked Axis evaluation with no stale last-good line fallback;
- repairable missing Sketch/Line source intent remains durable rather than being treated as corrupt document state;
- transitive support-stage requirements are carried when the source Sketch depends on Body-Surface or Datum support;
- native Part schema v12 with `next_axis_id` and `axes`;
- exact v11 -> v12 migration to empty Axis authored state with prior durable identities preserved.

## Verification

Exact-head Windows FULL #1531 PASS on `9a01ec707c12ac699dcd1d5b9e1a4e7d8bc2578a` is the runtime verification authority for PM-04A.

The FULL run passed:

- complete desktop build/test graph;
- core-only build/test without Qt/OCCT;
- kernel-native Release build/test;
- FAST and SUBSYSTEM selector verification;
- complete desktop CTest;
- SR-02 latency evidence;
- CI-04 warm FULL parity/comparative evidence;
- final `windows-msvc` aggregate.

Dedicated `pm04a.axis_schema_v12` regression proves:

- canonical AxisId parsing/high-water allocation;
- Origin X/Y/Z direct AxisReference resolution with no authored Axis creation;
- Regular and Construction Line admission;
- deterministic start -> end world direction;
- non-Line source identity -> Unsupported;
- deleted source Line -> Missing while AxisId/authored intent survives;
- missing source Sketch -> Missing;
- duplicate/out-of-high-water/malformed Axis identity fails closed;
- schema-v12 persistence roundtrip;
- exact v11 -> v12 migration with no synthetic Axis records.

Existing schema-v8/v9/v10 and Datum-backed persistence regressions were updated to remain exact legacy/current-schema fixtures under v12 and passed the same FULL run.

## Architecture boundary

PM-04A does not add:

- Create/Edit/Delete/Show/Hide Axis commands;
- Axis draft or source acquisition UI;
- Tree / Properties integration;
- Viewer Axis presentation or AxisId picking;
- Revolve Feature semantics or kernel operation;
- Datum Axis;
- Body Edge/Curve AxisReference;
- Projection;
- any new Shared-2D Axis entity role.

Those remain owned by PM-04B and later checkpoints.

## Result

PM-04A is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-04B — Axis lifecycle / Tree / Properties / Viewer** under the unchanged accepted PM-04 Work Contract.
