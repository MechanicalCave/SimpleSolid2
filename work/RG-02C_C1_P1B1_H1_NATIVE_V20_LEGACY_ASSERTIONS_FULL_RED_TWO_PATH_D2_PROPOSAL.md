# RG-02C/C1-P1-B1/H1 — Windows FULL #3388 RED: exact two-native-test v20 assertion path addendum

**Status: PROPOSED BOUNDED TWO-PATH OWNER D2 AMENDMENT — NOT YET OWNER-ACCEPTED OR MERGED. H1 IMPLEMENTATION TESTS REMAIN RED.**
**Date:** 2026-10-16. Protected-main baseline `b4265c53e5b6677af566733fdfc3deafc14fcad8`, after separately Owner-approved, exact-head Windows-green guarded squash #445 (one v20 header path) and #446 (active H1 contract changed from 18 to 19 paths). The User authorized those TWO merges, not any later undisclosed test-source paths.
**Owner-accepted premise, unchanged:** strict singleton current-stage analytic cylinder Face option A; persist only stage-qualified semantic `SurfaceReference`; v20 native writer, backwards-readable v19-and-earlier records; real OCCT provider read-only P1-A; no H2 UI, B/C fallback, patch ordinal or provider token identity. P1-B0 native multi-Face partition-positive and inherited exception/orientation GAPs remain NOT TESTED.

## 1. Reproduced real Windows FULL failure and affected exact test files

Independent [Ready FULL #3388](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38929996016) ran at headless H1 Draft PR #444 exact HEAD `eb62db628853099320be7af7f09d1658e2997280`, after actual Windows [FOCUSED #3387](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38929754074) **1/1 PASS** with OCCT native v20 Save/Reopen/Cold Build and independent [FAST #3386](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38929211891) **93/93 PASS**, official DOC-01 PASS. **FULL is RED and must not be disguised as pass.** Core-only 25/25 PASS. Kernel-native 55/57 PASS with exactly two historical current-writer literal-v19 assertion failures:
- Test #39 `pm02jr2.add_surface_continuation`: `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp:1525`, checks `package.package->descriptor.domain_schema_version == 19` although current schema is explicitly v20; paired preceding comment says `current ... v19`. **Keep the distinct legitimate v15/v16 legacy rejection fixtures in this file unchanged.**
- Test #48 `pm05c2b.part_edge_topology`: `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp:3304`, checks `PartDocumentStore::current_schema_version == 19`. Same fixture's diagnostic identifies `RG02C_B1_NATIVE_V19_AUTHORED_AXIS`, outputs `schema=19`, and temporary filename contains `_v19_`, now falsely labeling the current *writer* file version; keep B1 MaterialEdgeReference geometry/provenance, historical native v19 compatibility and other assertions unchanged.

Kernel FULL stops on the above; **Desktop Qt/OCCT 117 tests were NOT EXECUTED in #3388**. Old #3382 CI selector was separately RED, earlier PR runs cancelled; no false green claims.

These two test files are **NOT** among active H1 Work Contract §2 19 paths. Editing them without explicit Owner D2 approval violates `AGENTS.md` and active Work Contract fail-closed per-file permissions. This proposal is a source-evidence dependency, not license for broad refactor or weakening native assertions.

## 2. **EXACT proposed H1 source allowlist delta — ONLY TWO additional file paths**

Add exactly these two *existing* registered native test files to active H1 Work Contract §2 (19 → **21** total):

1. `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp` — ONLY replace the assertion of the **current writer descriptor** `domain_schema_version == 19` with **`== 20`** and adjust its immediately adjacent explanatory comment from current writer v19 to v20. Do **not** change historical v15/v16 fixture descriptor, material reference provenance, parsing, geometry, runtime semantics, or any other native test.
2. `tests/kernel_native/pm05c2b_part_edge_topology_test.cpp` — ONLY change assertion `PartDocumentStore::current_schema_version == 19` to **20**, the same actual-current-writer fixture's temporary filename marker `_v19_` to `_v20_`, and the already printed current-writer diagnostic `RG02C_B1_NATIVE_V19_AUTHORED_AXIS` / `schema=19` to `...V20...` / `schema=20`. Do **not** change existing B1 Edge source authoring, native MaterialEdgeReference, Undo/Redo, Topology evidence, historical v19 fixture compatibility, or any other file.

**NO other permissions:** CMake, tests registration/CI, source `src/**`, `work/ACTIVE.yaml`, Qt/UI, product docs, generated Browser, generated fixtures, Axis schema behavior, legacy v15/v16 reader rules, or new regression-test rewrites. The 19 original H1 §2 paths stay as already Owner-approved. These two test paths must never be used as an excuse to delete assertions or suppress a genuine product RED.

## 3. Gate order — addendum text acceptance ≠ guarded merge or H1 technical FINAL PASS

1. **PENDING:** Owner must explicitly approve precisely the **two new test paths**, restricted to their current-v20 writer assertions/labels, after reading this proposed bounded D2.
2. Record Owner scope acceptance in this governance-only one-file addendum and in a separate one-file proposed amendment of the existing ACTIVE H1 contract §2; each passes its own exact-head Windows governance CLOSURE and separately Owner-authorized guarded squash into protected `main`. No `ACTIVE.yaml` pointer change: H1 remains active.
3. ONLY AFTER BOTH governance PRs are merged, implement these exact test-assertion corrections in the already open [H1 #444](https://github.com/MechanicalCave/SimpleSolid2/pull/444). Real Windows native FOCUSED + FAST + Ready FULL Core-only/Kernel/Qt-OCCT Desktop and DOC-01 must prove fixes, not merely green an individual test.
4. Remaining H1 deliverables: as-built internal documentation and official **generated-only** Browser, actual native Axis→Revolve consumer if required, truthful ledger with all prior RED/GAP, post-final exact-head CLOSURE, distinct Owner technical FINAL PASS and distinct future protected-main guarded squash. No future H2 product UI before independent Work Contract activation.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This is a governance-only proposal for two existing outdated **current writer version assertions** revealed by actual Windows FULL RED, not a shipped feature or documentation change; the active H1 contract separately retains required internal as-built and official Browser generation.
