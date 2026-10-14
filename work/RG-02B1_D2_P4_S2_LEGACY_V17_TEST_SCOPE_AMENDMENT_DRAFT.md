# RG-02B1/P4-S2 — legacy PM-02E native schema v17 regression-test allowlist amendment

**Status:** PROPOSED D2 NARROW SCOPE AMENDMENT — NOT OWNER ACCEPTED, NOT ACTIVE, NO TEST SOURCE MUTATION PERMITTED YET.  
**Date:** 2026-10-14.  
**Active parent Work Contract:** `work/RG-02B1_D2_P4_S2_AUTHORED_EDGE_POINT_DATUM_V17_WORK_CONTRACT_DRAFT.md`, D2 accepted in PR #374; contract squash `b824669407bdc1ac64a15311acfa933c17ffbe5f`, separate ACTIVE activation PR #375 squash `168a765133043aae8847fc11696e0e4ff693baa3`. No S3 or PM-06 authority.  
**Source:** read-only audit of current `main` `168a765133043aae8847fc11696e0e4ff693baa3` and pre-existing test `tests/pm02e_sketch_support_schema_v9_test.cpp`, before S2 product merge.

## 1. Exact discovered incompatibility

S2's already-accepted v17 writer introduces one additional authored field `edge_point_datum_planes` and sets `PartDocumentStore::current_schema_version=17`. The pre-existing PM-02E full-tier legacy Part test hard-codes the former `domain_schema_version==16` at **three assertions** (test lines 374, 490, 605 of main). More importantly, its authentic-v8/v9 fixture-builder helpers `buildSchema8FromCurrentOriginDocument` and `buildSchema9FromCurrentDocument` each remove historic later fields from the current document **but do not remove `edge_point_datum_planes`**, producing invalid old-schema authored-field counts after a v17 writer. This is a real regression-test compatibility problem, not license to change the v17 schema or make parser lenient.

`tests/pm02e_sketch_support_schema_v9_test.cpp` is NOT currently within S2's exact §4 allowed paths, although `tests/pm03a_datum_schema_v10_test.cpp` and `tests/pm03e_datum_backed_sketch_test.cpp` are. Therefore DO NOT edit PM-02E inside S2 production before the Owner explicitly accepts this one-path amendment and governance activation, even if old assertions fail a FULL build.

## 2. Proposed precise D2 allowance

Append **only** this exact pre-existing file to the S2 §4 test allowlist, for compatibility-only test source changes:

- `tests/pm02e_sketch_support_schema_v9_test.cpp`

Permitted edits within that file **only**:
1. Update the three assertions that inspect a newly written current Part container's schema from 16 to the accepted 17.
2. In both historic v8/v9 test-only downgrade helpers, assert that current authored `edge_point_datum_planes` is an **empty** array before erasing it; fail the test if it contains an actual Edge-local Datum, so no real authored source is silently discarded by a fixture helper. Preserve the rest of the valid old-schema bytes/semantics as before.
3. Keep every legacy v8/v9 migration, document identity, Sketch, Body and native-container negative witness; do not weaken assertions or change production parser/Geometry/Part behavior to get a green suite.

No new product behavior or public API, schema version, other test file, CMake/CI or roadmap change is proposed. S2-F07/F09 actual Windows Ready FULL is still required; no PASS is claimed from this audit.

## 3. Formal gate

Owner D2 exact-text decision -> this one-document work PR governance PASS and separate exact-head squash permission -> accepted main amendment -> separate governance activation referencing this amendment without changing the parent `active_work` scope -> only then permitted PM-02E source edit on the S2 implementation branch. The general Owner permission to continue the existing S2 contract does not silently add a path expressly excluded by its closed §4 allowlist.

## Documentation impact

Internal docs: not required beyond this proposal.
User/Product docs: not required.
Reason: only a pre-existing legacy test's fixture construction and expected current-version assertions would change, not shipped behavior or UI.
