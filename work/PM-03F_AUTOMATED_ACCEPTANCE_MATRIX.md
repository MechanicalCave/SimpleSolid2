# PM-03F — Automated Acceptance Matrix

**Status:** ACTIVE — PM-03F1 integrated lifecycle evidence pending exact-head gate  
**Parent Work Contract:** `work/PM-03_DATUM_REFERENCE_GEOMETRY.md`  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.23  
**Date:** 2026-10-05

## Purpose

This matrix closes the automated-evidence side of the accepted PM-03 completion contract without redefining Datum semantics.

It aggregates the exact-head evidence already accepted for PM-03A through PM-03E and adds the PM-03F1 integrated lifecycle/survival extension to the existing Datum-backed Sketch regression.

The final Owner Windows workflow remains a separate mandatory PM-03F gate.

## Evidence authority

| Checkpoint | Exact candidate | Gate | Primary automated evidence |
| --- | --- | --- | --- |
| PM-03A | `41ea901c873f1453e27b2b4973332ecb5295388c` | Windows FULL #1485 PASS | `pm03a.datum_schema_v10` |
| PM-03B | `e12f6c606132eadab5283372dee1a9f3f3008abd` | Windows FULL #1491 PASS | `pm03b.datum_evaluation` |
| PM-03C1 | `c633a9714602801e1253d119198ea9e46f73f9a9` | Windows FULL #1496 PASS | `pm03c.datum_commands_draft` |
| PM-03C2 | `8b9185261e2cf32006f6af7a8c8a9da5e7c9ec04` | Windows FULL #1507 PASS | `pm03c2.cad_workbench_datum_plane` |
| PM-03D1 | `b236e54028a64ba00569a995b592b4ab8de2cd17` | Windows FULL #1511 PASS | neutral ReferenceScene + PartViewportController regressions |
| PM-03D2 | `adc9dd5c1f970932a03aa43422a105643db9052d` | Windows FULL #1514 PASS | extended `pm03c2.cad_workbench_datum_plane` |
| PM-03E | `f4c691c50ce28bc311076b94925b719c889e8172` | Windows FULL #1520 PASS | `pm03e.datum_backed_sketch` |
| PM-03F1 | this checkpoint | exact-head Windows FULL required | PM-03E regression extended with session Save/Reopen + fresh-token cold rebuild |

## Accepted completion rows

| PM-03 acceptance row | Automated authority | State |
| --- | --- | --- |
| DatumId non-aliasing / high-water lifecycle | PM-03A + PM-03C1 history-branch non-reuse regression | PASS |
| v9 -> current schema migration preserving existing Document/Body/Sketch IDs | PM-03A native-container migration regression; rewrite emits schema 11 | PASS |
| Offset Datum Plane from XY/XZ/YZ | PM-03B deterministic Origin frames + PM-03C2 workbench flow | PASS |
| positive / negative / zero signed Offset | PM-03A authored zero, PM-03B frame evaluation, PM-03C1 signed Reverse/draft regression | PASS |
| planar Body Surface source | PM-03B exact-stage Surface evaluation + PM-03E Body-Surface-backed Datum vertical slice | PASS |
| Datum Plane source | PM-03A structural chain + PM-03B recursive evaluator + PM-03D2 product flow | PASS |
| source Body Surface movement after upstream edit | PM-03B movement regression + PM-03E downstream offset/source recomputation | PASS |
| source Surface Missing | PM-03B structured Missing + no frame | PASS |
| source Surface Ambiguous | PM-03B structured Ambiguous + no frame | PASS |
| non-planar Body Surface Unsupported | PM-03B non-planar regression | PASS |
| Datum-to-Datum cycle rejection | PM-03A structural cycle validation | PASS |
| Datum / Sketch / Feature stage-cycle rejection | PM-03E transitive Body-stage cycle regression | PASS |
| Edit preserves DatumId | PM-03C1 command/draft regression + PM-03D2 workbench edit regression | PASS |
| Delete unreferenced Datum succeeds | PM-03C1 lifecycle regression | PASS |
| Delete referenced Datum rejects atomically | PM-03C1 Datum dependency + PM-03E Sketch dependency | PASS |
| individual Show/Hide changes presentation state only and persists authored visibility | PM-03D2 product regression + PM-03F1 Save/Reopen extension | PASS after F1 exact-head gate |
| Reference Geometry group Show/Hide bulk-toggles child authored visibility, Undo/Redo, no second group truth | PM-03D2 workbench regression | PASS |
| default draft Offset = 10 mm; Reverse negates the same signed value | PM-03C1/C2 | PASS |
| GUI / Command Line share one Datum draft and semantic command | PM-03C1/C2 | PASS |
| preview / Cancel / no-op create no CAD history | PM-03C1/C2 | PASS |
| plane patch / border / Body intersection are presentation-only | PM-03D1 neutral ReferenceScene contract | PASS |
| intersection overlay never becomes Edge/Curve or Projection authority | PM-03D1 owner-bound overlay contract; overlay has no independent token | PASS |
| stale revision / session / evaluation / presentation cannot commit | PM-03C1 stale draft/evaluation + PM-03D1 disposable presentation-token bridge | PASS |
| Create Sketch on Datum Plane | PM-03E | PASS |
| re-support Sketch to/from Datum preserving SketchId, EntityIds and local U/V | PM-03E | PASS |
| Profile + existing Extrude Add/Cut from Datum-backed Sketch | PM-03E | PASS |
| upstream Datum offset/source edit recomputes downstream result or fails structurally | PM-03E | PASS |
| no stale Datum frame after source failure | PM-03B + PM-03E | PASS |
| Undo/Redo | PM-03C1/D2 + PM-03F1 session visibility survival | PASS after F1 exact-head gate |
| Save / Close / Reopen | PM-03F1 DocumentSession guarded save + destroyed session + fresh load | PASS after F1 exact-head gate |
| true cold rebuild with fresh runtime/provider tokens | PM-03F1 provider seed changes from normal generation to 9000 and semantic reconstruction remains Resolved | PASS after F1 exact-head gate |
| GUI / Command Line / semantic command parity | PM-03C1/C2 | PASS |
| semantic/core, kernel-native where applicable and desktop verification | exact-head FULL gates #1485, #1491, #1496, #1507, #1511, #1514, #1520 plus PM-03F1 FULL | PASS after F1 exact-head gate |
| final Owner Windows manual workflow | PM-03F manual acceptance checklist | **PENDING OWNER** |

## Integrated failure / repair matrix

The automated package must preserve these fail-closed outcomes across the complete Datum-backed vertical slice:

| Scenario | Required outcome | Evidence |
| --- | --- | --- |
| Source Surface removed | Datum Missing/Blocked; no frame; dependent Sketch/Profile/Feature cannot use stale geometry | PM-03B + PM-03E |
| Source Surface becomes ambiguous | Datum Ambiguous; no first-winner binding | PM-03B |
| Source becomes non-planar | Unsupported; no Sketch-support frame | PM-03B |
| Upstream Body stage unavailable | Datum Blocked; no stale downstream Kernel calls | PM-03B + PM-03E |
| Datum producer referenced by Datum | Delete producer rejected atomically | PM-03C1 |
| Datum producer referenced by Sketch | Delete producer rejected atomically | PM-03E |
| Datum re-reference would create local Datum cycle | mutation rejected; authored state unchanged | PM-03A/C |
| Sketch re-support would create Body-stage cycle | mutation rejected; authored state unchanged | PM-03E |
| stale draft/evaluation/session | Finish rejected with zero mutation | PM-03C1 |
| repair by valid source edit / Sketch re-support | durable IDs/local Sketch geometry preserved; current derived frames recompute | PM-03C/PM-03E |
| Save/Reopen | authored semantic source, DatumId, SketchId, FeatureIds, signed Offset and visibility survive | PM-03A/E + PM-03F1 |
| cold rebuild with new runtime tokens | semantic references resolve independently of old provider tokens | PM-03F1 |

## Remaining PM-03F gates

After PM-03F1 exact-head FULL passes and merges:

1. update internal as-built Part / persistence / Viewer documentation;
2. update paired PL/EN product documentation for Offset Datum Plane and Sketch-on-Datum workflow;
3. regenerate Product Browser from canonical Markdown and pass Windows DOCS;
4. execute the supported Windows Owner workflow from the Work Contract;
5. only after Owner PASS, perform final PM-03 closure. PM-04 and Projection remain separately gated.

## Documentation impact

Internal docs: required — this matrix is internal acceptance evidence and PM-03F later updates the as-built docs.  
User/Product docs: required in PM-03F docs slice, not in this runtime/evidence slice.  
Product Browser: required in PM-03F docs slice.
