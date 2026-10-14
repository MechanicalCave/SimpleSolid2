# RG-02B1/P4-S2 — bounded legacy native schema v17 regression-test allowlist amendment

**Status:** PROPOSED D2 NARROW TEST-SCOPE AMENDMENT — NOT OWNER ACCEPTED, NOT ACTIVE; NO OUT-OF-ALLOWLIST TEST MUTATION AUTHORIZED YET.
**Date:** 2026-10-14.
**Active parent Work Contract:** `work/RG-02B1_D2_P4_S2_AUTHORED_EDGE_POINT_DATUM_V17_WORK_CONTRACT_DRAFT.md`, accepted PR #374 and activated PR #375 (main `168a765133043aae8847fc11696e0e4ff693baa3`). S1 remains CLOSED.
**Reason for correction from first proposal:** audit of all historical Part-schema test suites found additional exact v16 assertions and downgrade builders beyond PM-02E. Keep one finite, truthful amendment rather than piecemeal broadening during code changes.

## 1. Factual compatibility issue

Accepted S2 writer changes **current native Part schema 16→17** and adds exactly one semantic authored payload collection, `edge_point_datum_planes`; old v16 Offset-only Part files must still load. Historical tests that inspect the **current writer** still expect v16, while some test-only backward-fixture builders repack current v17 JSON as v8/v9/v11/v14, leaving the new field and thereby generating a malformed old-schema payload. Windows READY FULL #2974 after fixing the PM-03A compile issue proved Core **25/25 PASS**, kernel-native **56/57 PASS**; the one failed `pm02jr2.add_surface_continuation` at original line 1521 was exactly the obsolete **current-writer** version assertion. This fifth file is therefore an observed native regression, not a speculative edit. These REDs are regression-fixture maintenance needs, not reasons to relax production native parsers or change accepted v17 semantics. Windows FULL #2972 previously stopped at a separate PM-03A C++ fixture-type compilation error, which has been corrected within the parent S2 allowlist; native full test assertions have not yet been certified.

## 2. Exact five-file additive §4 allowlist (test sources ONLY)

Propose allowing modifications only to:
- `tests/pm02e_sketch_support_schema_v9_test.cpp`: three assertions of a **newly written** Part version 16→17; in `buildSchema8FromCurrentOriginDocument` and `buildSchema9FromCurrentDocument`, require `edge_point_datum_planes` is present and **empty**, then erase it when building an old-schema v8/v9 test fixture.
- `tests/pm04a_axis_schema_v12_test.cpp`: current writer version assertions (three existing v16 expectations including the initial `current_schema_version`) 16→17; in `schema11FromCurrent` require an empty `edge_point_datum_planes` and remove it before generating an authentic v11 native fixture.
- `tests/pm04e1_revolve_persistence_test.cpp`: three current writer version assertions 16→17; test-only `repackage` of any **schema_version <17** must assert empty `edge_point_datum_planes`, erase only that additional field, and leave genuine historical records and other fields unchanged. Preserve independent malformed/policy negative witnesses.
- `tests/pm05b1_edge_feature_schema_v14_test.cpp`: four current writer version assertions 16→17; its test-only `repackage` on schema_version<17 must assert an empty `edge_point_datum_planes` array before erasing it. Maintain existing <15 linked-projection field removal and all v14/v15 legacy-shape and semantic history checks.
- `tests/kernel_native/pm02jr2_add_surface_continuation_test.cpp`: the **one assertion at original line 1521** describing a newly saved current Part changes 16→17; its local test-only `rewrite_package` for negative v15/v16 payload witnesses must assert empty new v17 Edge-local collection and remove it before repackaging old versions, so a rejection cannot be attributed to an unrelated wrong field count. **Preserve the independent historical fixture assertion at original line 6340 that the existing v16/modeling-v1 file really is schema v16**; never rewrite the fixture or soften modeling-v1 refusal. Do not change the native geometry continuation model.

These five filenames are additive to, not replacements for, the parent accepted S2 §4 allowlist. No `src/**`, UI, Viewer, kernel, CMake/CI, docs Browser, modeling semantics, existing ABI/public API or unrelated tests are changed by this amendment. It must **never** delete nonempty authored source definitions in a fixture downgrade, auto-interpret unknown v17 types, or make production native parsing more tolerant. Preserve all old actual historical version descriptor values and original negative assertions; update only expectations about **current v17 writer**.

## 3. Formal Owner D2 and Windows gates

Owner accepts this exact five-path test-only scope and the byte-level empty-array downgrade rule → this work-only governance PR exact-head CI → separate guarded squash into protected main → separately recorded activation entry under `work/ACTIVE.yaml` **without changing the S2 `active_work` pointer** → only then the S2 implementation branch can modify these four tests, with true Windows Ready FULL Core/Kernel/Desktop before final acceptance. Until approved, earlier Owner acceptance of S2 v17 schema does not silently authorize out-of-allowlist test paths. Existing S2 PR #376 is unmerged and any historical FULL RED must remain visible.

## Documentation impact

Internal docs: not required beyond this scope proposal.
User/Product docs: not required.
Reason: no shipped behavioral change; only fixture generation and expectations of pre-existing native compatibility regressions.
