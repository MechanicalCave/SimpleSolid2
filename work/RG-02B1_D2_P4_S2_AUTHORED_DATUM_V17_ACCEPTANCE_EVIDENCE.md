# RG-02B1/P4-S2 — authored Edge-local END/MID Datum v17 acceptance evidence

**Status: NATIVE TECHNICAL EVIDENCE READY FOR OWNER REVIEW — exact code/docs FULL #2985 PASS; Owner S2 technical FINAL PASS and separate exact-head squash permission NOT YET RECORDED; PR #376 NOT MERGED.**
**Date:** 2026-10-14. Only exact-SHA test outcomes are counted; FULL REDs and cancellations are retained.

## Owner-governed authority

- Accepted exact S2 D2 Work Contract: PR #374, protected-main merge b824669407bdc1ac64a15311acfa933c17ffbe5f.
- Separate ACTIVE activation: PR #375, protected-main merge 168a765133043aae8847fc11696e0e4ff693baa3.
- Five historical v17 fixture paths: explicit Owner D2 approval PR #377, activation #378, protected-main merge d0cbece7a73b497b551f357c027f07de50bfd599.
- Sixth historical PM-05B3 path: explicit Owner D2 approval PR #379, activation #380, protected-main merge fb52eda6c888276bfff6b40cec6ea776bdf5dbd5.
- Implementation: PR #376 clean main-based headless Part/Application/native-test branch; the old stacked mixed S2/S3 PR #372 remains separate WIP and MUST NOT be merged as S2.

## Exact Windows evidence and failure history

1. Windows native FOCUSED #2969 at code SHA ee12b489d62c688071eba74cd96172b747699f50 — SUCCESS, real OCCT pm05c2b.part_edge_topology 1/1 PASS. Source Edit MID→END→MID, same DatumId, Reverse, Undo/Redo, invalid and stale cases executed.
2. Windows FAST #2970 at SHA ae06813a4a2b6a246d423fc15b10231883cf3934 — SUCCESS, desktop 93/93 PASS with then-current docs parity. FAST does not certify FULL.
3. Windows Ready FULL #2972 at SHA ae06813a4a2b6a246d423fc15b10231883cf3934 — RED: the new PM03A negative fixture supplied DocumentId wrapper instead of serializable text to NativeDocumentDescriptor; desktop compilation failed; no runtime FULL claim. Fixed by allowed PM03A-only commit 149c0fc520319c37c56600100b4a1dff73b176c2.
4. Windows Ready FULL #2974 at SHA 149c0fc520319c37c56600100b4a1dff73b176c2 — RED: full Desktop graph built, Core 25/25 PASS, kernel-native 56/57 PASS; sole failing native pm02jr2.add_surface_continuation expected newly written schema v16 instead of v17, Desktop not run.
5. Windows Ready FULL #2979 at SHA bac566c68c0091d29bce4e241982ccbe9c816f27 — RED: Core 25/25 PASS, kernel-native 57/57 PASS (including PM02JR2 and new S2 PM05C2B), Desktop 116/117 PASS; sole failing PM05B3 generated Surface fixture expected current writer v16. Five-path D2 migration did not relax production parsing.
6. Windows Ready FULL #2984 at SHA 1e93d13bfd32c6d356b74070f4e83e4849be90ca — CANCELLED after docs-only push to this PR; no runtime PASS or RED inferred.
7. Windows Ready FULL #2985 at exact code+docs SHA 51fbb02d979652a12638144575e70d17d7fbe30d — COMPLETED SUCCESS: Core 25/25 PASS, kernel-native 57/57 PASS, Desktop 117/117 PASS = 199/199 PASS. Actual full Windows graph compilation, runtime checks, SR-02 latency, CI-04 warm FULL parity and comparative timing PASS. Official .\\ss2.ps1 docs regenerated docs/browser/index.html; git diff --exit-code and docs self-test proved exact Browser parity. https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38560949900

## S2 semantic witness inventory — do not overclaim

- Native Edge-local END and exact bounded 3D arclength MID sources certified with stable semantic MaterialEdge references and S1 signed orientation/Reverse; no independent off-line Point or arbitrary nearest XYZ fallback.
- Part-owned EdgePointDatumPlane, global DatumId uniqueness across legacy Offset and new type, strict current Body stage evaluation and typed fail-closed errors.
- Transaction commands include Create, Reverse-only Edit, separate atomic semantic source replacement Edit, visibility and Delete with dependency checks; caller must carry exact revision and provider; no last-good geometry.
- Provider-aware Datum → Sketch and Sketch-Line Axis current frame consumption. Cold Part v17 Save/Close/Reopen with a new OCCT provider validates rederived End/Mid signed frames and downstream Sketch support.
- Existing legacy v16 Offset documents migrate to v17 with stable DatumIds, unknown v17 source kinds and malformed intent rejected; genuine old-schema test fixtures never silently discard a nonempty Edge-local authored collection.
- S2-F01…F09 evidence must be reviewed against the accepted contract. FOCUSED #2969 native 1/1 PASS and FAST #2970 desktop 93/93 PASS on earlier exact sources, with final code/docs Ready FULL #2985 native 199/199 PASS and official DOC-01 parity. These are NOT a fresh FOCUSED and FAST run on the final docs-only SHA, and cannot replace the separate Owner S2 technical FINAL PASS and exact-head squash permission.

## Documentation impact

Internal docs: required, docs/internal/PART_DOCUMENTS.md and Browser generated exclusively from canonical Markdown with official Windows ss2.ps1 docs parity.
User/Product docs: not yet S2 scope; S3 UI/Viewer gesture/Edit/Command Line and off-infinite-line independent Point require separate accepted contracts.
