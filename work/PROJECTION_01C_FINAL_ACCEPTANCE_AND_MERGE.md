# PG-01C — Final acceptance and merged closure evidence

**Status:** COMPLETED — Owner FINAL PASS and squash MERGED on 2026-10-09.  
**Authority:** `work/PROJECTION_01C_PROJECT_EDGE_UI_CONTRACT.md`, including explicitly Owner-approved bounded D2-K/T/P/B amendment.  
**Accepted PR:** [#304](https://github.com/MechanicalCave/SimpleSolid2/pull/304), feature HEAD `95731b9a4c9ee778b3a9743b57d3b8293bf869dc`.  
**Merged main commit:** `d928e8a5d1f5893b73de3e4fcd399d7e0483af83` (guarded squash merge).  
**Owner manual acceptance:** explicit `Potwierdzam PASS` (2026-10-09), recorded in [PR discussion](https://github.com/MechanicalCave/SimpleSolid2/pull/304#issuecomment-6086938356).

## Verification on the accepted immutable feature HEAD

- [Windows FULL #2102](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37971862536): **PASS**; core-only **25/25**, kernel-native **57/57**, desktop **114/114**, **196/196 total**. Docs and deterministic offline Browser regeneration gate also passed.
- Native OCCT regression tests include after-Chamfer rotated source Edge projection into closed linked Sketch regions, linked Profile Create against current moved source, all-linked Tree status, and atomic multi-target Break Link with one Undo and unchanged Profile identity.
- Initial FULL #2100 on the preceding candidate SHA had an **intermittent** `sk04c.part_sketch_interaction_controller` failure (113/114). Re-running the **same job at the same SHA** passed all 114/114, and accepted-head #2102 also passed. **Not a claimed deterministic root-cause fix**; retain the evidence for independent stability review. No test gate was waived.
- Owner manually re-tested the original private `Part006.ss2part` Sketch 5, the linked Circle's Profile validity, the multi-selection Break Link behavior and unsupported source staging, and then granted overall FINAL PASS. The private Part was never committed.

## Accepted deliverables

1. One same-Part Project Geometry tool for material Edge source picking (toolbar, right Operations, global Command Line, strict stage, selected role, preview and atomic Finish/Cancel/Esc).
2. Current linked Line/Circle/Arc presentation/semantic resolution; linked Regular Profile and dependent Feature evaluation use current source, never a stale saved seed.
3. D2-K exact OCCT Line endpoint projection from oriented source topology vertices; no general gap-healing tolerance.
4. D2-T linked Profile Tree status aligned with exact current provider-backed resolution.
5. D2-P linked Profile Create/Edit semantic Command validation from the current provider/revision, while authored-only Sketches retain the no-provider path.
6. D2-B multi-selection Break Link, atomic single Undo, preserving existing Sketch entities and Profile meaning.
7. PL/EN Product docs, internal as-built, deterministic offline Browser and native regressions.

## Integration and future gate

- The PR is **merged to main**, not merely accepted. PG-01C's feature branch should receive no further production changes.
- The standing `work/ACTIVE.yaml` remains pointed at the **completed** PG-01C contract for traceability. This does **not** authorize additional PG-01C mutation or activate the next work package.
- **PG-01D planar Face boundary**, **PG-01E program closeout** and **PM-06** remain separately gated. Any PG-01D production source mutation requires a dedicated Owner D2/contract decision, separate governance activation on main, and a new production branch. Independent CI issue #302 remains unrelated.
- This file is a **documentation/evidence-only closure**. It does not change runtime behavior, authored identity, schema or a user-facing feature.

## Documentation impact

Internal as-built documentation: **not required** — this is an historical acceptance/closure record, not a new implementation.  
PL/EN Product documentation: **not required** — PG-01C product documentation was part of the accepted merged implementation.  
Generated Browser: **not required** — the canonical Product/as-built sources did not change in this proposal.
