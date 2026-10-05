# PM-03A — Datum Foundation / Schema v10 Completion

**Status:** COMPLETED — PASS  
**Parent Work Contract:** `work/PM-03_DATUM_REFERENCE_GEOMETRY.md`  
**Runtime candidate:** `41ea901c873f1453e27b2b4973332ecb5295388c`  
**Windows gate:** FULL #1485 — PASS on the exact runtime candidate  
**Merged runtime main:** `542d7b50fcd3f7566326f1b2c849d9453a3dcd00`  
**Scope boundary:** PM-03A only; no Datum evaluator/frame resolution, commands/draft, Viewer/Tree/Properties, intersection overlay or Datum-backed Sketch

## 1. Closure statement

PM-03A is COMPLETED — PASS.

The Part authored model now contains the durable semantic foundation required by the accepted PM-03 Offset Datum Plane contract: Part-local Datum identity, bounded plane-source references, structurally valid authored Offset Datum Plane state and native schema v10 persistence.

This closure does not claim completion of PM-03. The next active checkpoint is PM-03B — evaluator + dependency/cycle semantics.

## 2. Delivered production boundary

Identity and authored state:

- `DatumId` is durable Part-local identity with canonical serialization and high-water `DatumIdCursor` allocation;
- `OffsetDatumPlane` owns DatumId, PlaneReference, signed finite Length offset and authored per-Datum visibility;
- `PlaneReference` has exactly the accepted PM-03 source variants: built-in Origin plane, PM-02 Body planar `SurfaceReference`, or another Datum Plane by DatumId;
- provider/runtime topology handles, Viewer tokens, geometric fingerprints and topology ordinals are absent from authored Datum state.

Validation:

- Datum IDs must be allocated, valid and unique;
- Body Surface references must name existing authored producer/stage Feature identities with producer no later than the declared stage;
- Datum-to-Datum sources must resolve to an authored DatumId;
- direct and indirect local Datum dependency cycles fail closed;
- non-finite offsets fail closed;
- rejected complete-state validation produces no authored commit.

Persistence:

- native Part schema advances from v9 to v10;
- v10 persists `next_datum_id` and `datum_planes`;
- a valid v9 document migrates to an empty Datum collection and fresh Datum high-water cursor while preserving existing Document/Body/Sketch identity;
- subsequent save writes canonical v10 authored state;
- malformed Datum state fails closed through normal reconstruction validation.

## 3. Verification evidence

Exact candidate `41ea901c873f1453e27b2b4973332ecb5295388c` passed Windows FULL #1485, including:

- complete desktop build/test graph;
- core-only build/test without Qt or OCCT;
- kernel-native Release build/test without Qt;
- FAST and SUBSYSTEM selector verification;
- full test suite;
- SR-02 latency evidence;
- CI-04 warm FULL parity and comparative timing evidence.

PM-03A-specific regression `pm03a_datum_schema_v10_test` verifies:

- DatumId canonical/high-water allocation;
- valid Origin-backed Datum chains;
- zero-offset Datum remains distinct semantic identity;
- direct/indirect Datum cycle rejection;
- missing Datum source rejection;
- absent Body Feature producer/stage rejection for SurfaceReference;
- non-finite offset rejection;
- duplicate DatumId rejection;
- v10 save/reload round-trip;
- exact v9 -> v10 migration with existing durable identity preservation.

The existing PM-02 schema regression is also extended through current schema v10.

## 4. Architecture result

PM-03A adds no second placement truth and no general dependency graph.

At this checkpoint:

- Datum meaning is authored and provider-neutral;
- only structural reference validity is frozen;
- current geometric resolution/frame/status remains derived work for PM-03B;
- no user-facing Datum tool is claimed yet;
- no Datum-backed Sketch support is claimed yet.

PM-03B must now derive deterministic frames and structured current resolution status from the accepted semantic sources while preserving fail-closed behavior and bounded dependency semantics.

## Documentation impact

Internal docs: required — this completion record plus ACTIVE/roadmap/Work Contract checkpoint synchronization.  
User/Product docs: not required — PM-03A introduces no delivered user-facing Datum Plane tool yet.  
Product Browser: not required — canonical Product documentation is unchanged at this checkpoint.
