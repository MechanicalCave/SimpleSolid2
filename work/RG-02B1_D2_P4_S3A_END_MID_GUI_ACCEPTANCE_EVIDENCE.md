# RG-02B1/P4-S3A — Datum END/MID GUI acceptance evidence

**Status: OPEN — WIP, NOT OWNER MANUAL FINAL PASS, NOT PRODUCTION MERGE AUTHORITY.**
**Date:** 2026-10-14.
**Authority:** `work/ACTIVE.yaml` points to `work/RG-02B1_D2_P4_S3A_DATUM_END_MID_GUI_WORK_CONTRACT_DRAFT.md`, Owner-accepted and independently activated. The bounded native test-path amendment `work/RG-02B1_D2_P4_S3A_NATIVE_WORKBENCH_GESTURES_TEST_AMENDMENT_DRAFT.md` was Owner-accepted in #387 and separately activated in #388. Owner later granted standing permission to continue gates *within this accepted S3A contract*, not new D2/D3 scope or substitute manual witnessing.

## Exact historical CI observations — NOT an interchangeable final matrix

- #387 test-only D2 governance: exact-head Windows Ready #3012 SUCCESS on `707b7a84880dfb4f9ea6e105b27955a90e9de192`. Guarded squash main `2a64c584789c0151b6c7181217cd622a092ed7bb`.
- #388 separate ACTIVE-only activation: Windows Draft #3013 and Ready #3014 SUCCESS on `0bb808ec1fa8843f5c141d3c382e5fbe91e7f994`. Guarded squash main `9ed11284dbf7034beab1455bac29c5fc88f7e56c`. No native product tests are claimed by these governance-only runs.
- [Windows Ready FULL #3017](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38587980849): `080a2e81d0a1db81673e51f32254456a1a0226c7`, Core **25/25**, Kernel-native **57/57**, Desktop **117/117**: **199/199 PASS**. The new additive test in the pre-registered `pm05f_r2.native_workbench_edit` suite ran **PASS (27.82 s)** and asserted real Qt/OCCT native material Edge→MID clicks through CadWorkbench, one authored semantic Datum, Reverse, Undo/Redo and native v17 Save/Reopen on a cold independent provider. This is **not** complete S3A.
- [Windows Ready FULL #3018](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38589098637) on `91c2abfa8746025b5d816ce1048f3cc8476272aa`: **FAIL**; one Desktop `pm05f_r2.native_workbench_edit` test fails on newly added **MID→Edge point-first** gesture, while other 198 tests passed.
- [Windows Ready FULL #3019](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38589899287) on `e10d3f7d57c1c673423f572d0e635ee5e55c14bb`: **FAIL**; same point-first MID gesture, diagnostic label remains “Hover an Edge for MID, or pick END/Edge in either order.” Point pixels `Edge(500,346)`, `MID(522,358)`.
- [Windows Ready FULL #3021](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38591962643) on `ced69c1c87a17ac154b6ae0df15a424945857c0a`: **FAIL**, same exact point-first MID assertion despite strict current Edge hover query. Not evidence of successful repair.
- #3023 on `4f752f31c3019e7f18e9ae95c73a9a278f8fbc47` adds strictly test-gated diagnostic traces in the allowed existing source/test files; **result pending at the time of this evidence snapshot**. Never infer a PASS without exact Windows results.

## S3A acceptance gates (all claims must be firsthand exact-head)

| Gate | Evidence/status |
| --- | --- |
| F01 END gesture, both terminal roles, chosen Edge identity on shared corner | PENDING integrated native Workbench proof |
| F02 exact MID marker, default frame, double Reverse | PARTIAL: Edge→MID native #3017; both orders and double Reverse PENDING |
| F03 real Edge→END/MID and Point→Edge both orders / shared corner | PARTIAL: Viewer-level #3008; integrated MID→Edge currently FAIL |
| F04 one tool, Operations, selection-first, Command Line Enter/ESC/Reverse/focus | PARTIAL: #3011 earlier native FULL and PM03C2 CLI; final complete focus PENDING |
| F05 provider-certified Qt Workbench positive preview/Finish/Cancel/Undo | PARTIAL: native Edge→MID #3017; broader positive PENDING |
| F06 Edit source without DatumId/Sketch/Axis changes, one Undo, fail closed | PENDING integrated test |
| F07 dependent Sketch native Save/Close/Reopen on independent provider | PARTIAL: authored Datum cold reopen #3017, dependent Sketch PENDING |
| F08 typed off-infinite-line Point/ambiguous/stale/degenerate/closed Edge refuses mutation | PENDING final integrated negative coverage |
| F09 FOCUSED → FAST → final Ready FULL; internal + PL/EN + official Browser parity | PENDING current exact HEAD |
| Owner real Windows manual clicks, preview, Edit, cold Save/Reopen and Offset regression | **PENDING — no substituted simulation/standing permission counts as manual PASS** |

## Scope and decision stops

Exactly one native Workbench test path was added to S3A: `tests/pm05f_r2_native_workbench_edit_test.cpp`. The active original S3A UI/Viewer paths, registered tests, internal documentation, both PL/EN product Markdown, generated Browser and this ledger remain the entire allowlist. No CMake, CI, Part schema/public API, independent off-line Point or PM-06 changes are preauthorized. Do not merge implementation PR #386, report final PASS, or close S3A while the point-first native witness fails and the mandatory manual acceptance is outstanding.
