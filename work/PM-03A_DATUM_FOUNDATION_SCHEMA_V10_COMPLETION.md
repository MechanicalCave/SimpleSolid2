# PM-03A — Semantic Datum Foundation + Schema v10 Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-03_DATUM_REFERENCE_GEOMETRY.md`  
**Exact runtime candidate:** `41ea901c873f1453e27b2b4973332ecb5295388c`  
**Windows FULL:** #1485 — PASS  
**Merged main:** `542d7b50fcd3f7566326f1b2c849d9453a3dcd00`  
**Date:** 2026-10-05

## Delivered

PM-03A establishes the durable semantic/persistence foundation for the accepted Offset Datum Plane package without implementing evaluation or product UI.

Delivered production semantics:

- Part-local durable `DatumId` and high-water `DatumIdCursor`;
- authored `OffsetDatumPlane` with signed canonical length and persistent visibility;
- bounded `PlaneReference` variants for Origin plane, PM-02 `SurfaceReference`, and Datum Plane;
- `PartAuthoredState` ownership and read access for Datum state;
- fail-closed validation of Datum ID allocation/uniqueness, missing Datum references, Body Surface producer/stage validity and Datum-to-Datum cycles;
- native Part schema v10 with `next_datum_id` and `datum_planes`;
- exact v9 -> v10 migration to empty Datum state with existing durable identities preserved;
- current-schema persistence regression extended through v10.

## Verification

Exact-head Windows FULL #1485 PASS on `41ea901c873f7566326f1b2c849d9453a3dcd00` is **not** the authority; the tested exact candidate is `41ea901c873f1453e27b2b4973332ecb5295388c`.

The FULL run passed:

- complete desktop build/test graph;
- core-only build/test without Qt/OCCT;
- kernel-native Release build/test;
- FAST and SUBSYSTEM selector verification;
- complete desktop CTest;
- SR-02 latency evidence;
- CI-04 warm FULL parity/comparative evidence.

Dedicated PM-03A regression proves:

- canonical DatumId parsing/high-water allocation;
- valid Origin-backed and Datum-backed authored chains;
- direct/indirect cycle rejection;
- missing Datum reference rejection;
- undeclared Body-stage Surface source rejection;
- non-finite offset rejection;
- duplicate DatumId rejection;
- schema-v10 persistence roundtrip;
- exact v9 -> v10 migration.

The existing rich PM-02E persistence regression additionally proves v9 -> v10 preservation of DocumentId, BodyId, FeatureId, ProfileId, SketchId and Sketch EntityIds.

## Architecture boundary

PM-03A does not add:

- Datum frame evaluation/resolution;
- commands or runtime draft;
- GUI / Command Line behavior;
- Viewer presentation/intersection overlay;
- Tree / Properties;
- Datum-backed Sketch support;
- Datum Axis / Datum Point;
- Projection.

Those remain owned by PM-03B and later accepted checkpoints.

## Result

PM-03A is **COMPLETED — PASS**.

The next authorized checkpoint is **PM-03B — evaluator + dependency/cycle semantics** under the unchanged accepted PM-03 Work Contract.
