# RG-02B1/D2-P4-S1 — Signed Edge-local Frame3 acceptance evidence (IN PROGRESS)

**State:** DRAFT technical evidence, **NOT Owner technical FINAL PASS, NOT production merge authorization**. Prepared 2026-10-14. Real Windows runs and their exact SHAs are identified below. No S2 persistence, S3 UI/Viewer or Datum functionality is claimed.

## Authority and exact-source split

- Activated contract: `work/RG-02B1_D2_P4_S1_SIGNED_EDGE_FRAME_WORK_CONTRACT_DRAFT.md`, Owner-accepted via PR #367 and activated on protected `main` by separately authorized squash PR #368 at `6ebcbc7dec139d873847fe0214001a12204d7a80`.
- Production Draft PR: [#371](https://github.com/MechanicalCave/SimpleSolid2/pull/371), implementation on a separate head branch; changes remain within exact Work Contract §6 allowlist, no `main` mutation.
- Historical initial source/test HEAD `46bdfacafec23768fe5a05eb22085da05e4d8a27`: [Windows FAST #2932](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38474553684), **SUCCESS, 93/93** (includes `pm05b2.strict_edge_resolver`). This run does **not** execute native full-only `pm05c2b`.
- Historical FOCUSED RED: [#2933](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38512468328) at `83fd0ca4431f4f62f8c236e8c4e4ae53cae5296c`: an invalid *desktop* focus request named `kernel_native` targets missing from that build graph, `MSB1009`; **zero native tests executed, not an algorithm assertion result**.
- First valid kernel-native FOCUSED: [#2934](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38512623732) at `feace454d807509df5864d05b11b5016053fc16d`: **SUCCESS, 2/2 native CTests** (`pm05c2b.part_edge_topology`, `pm02p.e_kernel_lifecycle`).
- Intermediate #2935 at `9a7cf4706841dba05753e83d9ca7179032ee28ac` was **CANCELLED** when test-only branch head advanced; never treat as PASS.
- Current complete-source native FOCUSED: [#2936](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38513130406) at `1100ee01e0f00b5a371ca61b86aa3010509dbcbb`: **SUCCESS, 2/2 native CTests** in `kernel` Release build.
- Same-code FAST: [#2937](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38513279479) at `e3e8e6f36c2fb696e736baf24f7144176455f891`: **SUCCESS, 93/93**. Compare `1100ee0..e3e8e6f` yields **0 changed files** and exactly the same immutable Git source/test tree `bbb9e8aaba18c9a7a06ff1b3de2d7b374f862e93`. The intervening commit requests FAST only.
- **FULL / Core / Kernel-native / Desktop and DOC-01 generated Browser parity have NOT been executed on final documentation-bearing HEAD.** They are mandatory before FINAL PASS.

## S1-F01…F09 finite gate accounting

| Contract gate | Current factual witness and outstanding checks | Gate |
| --- | --- | --- |
| F01 | Registered native `pm05c2b.part_edge_topology` on real Extrude box: strict source 12 Edges, 24 certified END, 12 MID frame initialize+replay, shared three-Edge corner; #2936 2/2 | PARTIAL — comprehensive code/source review and final FULL pending |
| F02 | Real positive and Reverse-sweep Part Revolve Line/open-Arc END/MID signed frames and native Q0 arcs; native source tests #2936 PASS. Existing Q1 tuple-reorder regression remains exercised; no new native-authority claim | PARTIAL — final FULL pending |
| F03 | S1 replay from caller-carried unchanged intent on independent OCCT provider / reconstructed Part; no runtime token or saved frame used; #2936 PASS | PARTIAL — final FULL pending |
| F04 | Source-preserving real Extrude height edit, same remembered signed orientation; source/context failure regressions; #2936 PASS | PARTIAL — final FULL pending |
| F05 | Controlled signed ±t, X/Y dominant component tie, +X-parallel U seed fallback, below/exact/above `1/16` neighborhood, frame orthonormal checks in native Part fixture; #2936 PASS | PARTIAL — final FULL pending |
| F06 | At/surrounding 30° replay bound, 29° admitted, 31° and 89°/90°/91° explicit review, malformed zero/NaN/Inf/nonunit/nonorthogonal supplied intent, no failure frame; #2936 PASS | PARTIAL — final FULL pending |
| F07 | Defensive U-projection check implemented, first-time seed fallback and Reverse involution covered. **Valid replay cannot reach the `U² < 1/16` branch under accepted 30° guard:** for unit `Nref⊥Uref`, `||projection_N(Uref)||² >= cos²(30°)=3/4`. Separate [Owner D2 clarification](https://github.com/MechanicalCave/SimpleSolid2/pull/371#issuecomment-6161091602) requested; **NOT a claimed successful witness** | **OWNER D2 PENDING** |
| F08 | Existing strict Q1 END/Q2 MID, real Fillet/Chamfer/half-cylinder, Ambiguous/Unsupported and cold-provider regressions retained in registered focused/native/FAST witnesses | PARTIAL — final FULL pending |
| F09 | Exact source tree native FOCUSED 2/2 #2936 and FAST 93/93 #2937 confirmed, with immutable tree parity. Internal Part as-built documentation has been drafted, but Browser must be generated via **`./ss2.ps1 docs` only**, and Ready FULL + DOC-01 must actually pass | **UNFINISHED** |

## Remaining bounded production gates / STOP

1. Add the as-built `docs/internal/PART_DOCUMENTS.md` S1 section and regenerate `docs/browser/index.html` with **`./ss2.ps1 docs`** in an actual compatible workspace. Do not manually edit generated HTML or assert DOC-01 without verification.
2. Review and obtain the separate Owner D2 F07 clarification; no silent change to the accepted signed orientation, replay 30° bound or `1/16` numerical policy.
3. Run true Ready Windows FULL (Core, native Kernel and Desktop), verify internal docs/Browser parity DOC-01 and exact accepted source/test/docs SHA; do not transplant prior Q1/Q2 pass counts into S1.
4. Freeze source/test/docs commit, document actual final test results; if only this evidence ledger changes after FULL, compare exactly two SHAs, prove ledger-only delta and run final governance CI.
5. Require Owner technical FINAL PASS **and separately** explicit Owner exact-HEAD squash merge approval for PR #371. No automatic S2 activation or S3 UI/Viewer work.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: evidence for the headless read-only S1 Part signed Frame3 adapter only; internal as-built Part Markdown and deterministic generated Browser are independently required for feature acceptance. S2/S3 later govern native schema and bilingual user-facing product documentation.
