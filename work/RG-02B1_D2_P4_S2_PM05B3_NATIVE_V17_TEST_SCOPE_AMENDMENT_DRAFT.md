# RG-02B1/P4-S2 — PM-05B3 generated Surface v17 regression test-path amendment

**Status:** PROPOSED NARROW OWNER D2 AMENDMENT — NOT ACCEPTED OR ACTIVE. No out-of-allowlist test mutation is authorized by this document alone.
**Date:** 2026-10-14.
**Parent authority:** accepted S2 headless Datum v17 Work Contract in `work/RG-02B1_D2_P4_S2_AUTHORED_EDGE_POINT_DATUM_V17_WORK_CONTRACT_DRAFT.md`; activated on protected main by PR #375. Previous Owner-accepted five-path test amendment #377, separately activated by PR #378, remains intact and unchanged.
**Evidence:** Windows Ready FULL [#2979](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38545854034) on exact isolated S2 code/tests SHA `bac566c68c0091d29bce4e241982ccbe9c816f27`: actual Core **25/25 PASS**, kernel-native **57/57 PASS**, desktop **116/117 PASS**. The only desktop RED is `pm05b3.generated_surface_reference_v14`: line 468 expects `PartDocumentStore::current_schema_version == 16` while Owner-accepted current Part schema is v17. The earlier FULL #2972 and #2974 RED histories remain recorded, not overwritten. Thus #2979 is not a Ready FULL PASS.

## 1. Exactly one additive implementation path

Add to S2 §4 **only**:

- `tests/pm05b3_generated_surface_reference_v14_test.cpp`

The file currently lies outside S2 §4 and the five-path addendum, so it has **not** been edited in PR #376.

## 2. Exact permitted test-only changes

1. Update the **two current-writer assertions** in PM-05B3, original lines 468 and 490, from v16 to **v17**. Never change actual historical schema-v14 fixture descriptors.
2. In the existing **test-only** `repackage(package, authored_json)` helper which intentionally builds **native schema v14**, parse the current v17 JSON and prove `edge_point_datum_planes` exists, is an array and is **empty**, then erase this v17-only field before forming the old-schema fixture. A genuine nonempty Edge-local Datum must fail the test helper; no authored source may be discarded or silently downgraded.
3. Retain all original generated Fillet/Chamfer Surface and corner provenance, stage and duplicate-edge negative tests, native semantic support, reference equality and cold lifecycle evidence. **No product/source/parser changes, no change in modeling semantics, no blanket test acceptance.**

This is not a license to edit any additional test path. It adds nothing to Part schema, UI/Viewer, S3, Point, CMake, CI, roadmap, PM-06 or Feature provenance. It must not make product persistence more tolerant or downgrade genuine new sources.

## 3. Formal gate

Owner explicitly accepts **this one-path test-only D2 amendment** → exact-head governance CI and guarded squash merge of this one-document proposal → separately accepted and merged `work/ACTIVE.yaml` history-only activation under the existing S2 `active_work` pointer → only then change that exact native regression test on PR #376 → real Windows FOCUSED/FAST/Ready FULL → complete DOC-01 generated Browser parity and Owner S2 technical acceptance. Owner's earlier acceptance of the five named paths does not implicitly extend to this sixth path.

## Documentation impact

Internal docs: not required beyond this scope record.
User/Product docs: not required.
Reason: changes only old-schema test fixture preparation and new-writer version expectations, with no new production behavior.
