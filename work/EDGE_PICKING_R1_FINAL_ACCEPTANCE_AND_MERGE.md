# Edge Picking R1 — final acceptance and merged closure evidence

**Status:** COMPLETED — Owner-supported Windows practical acceptance, distinct `R1 FINAL PASS`, and separately authorized production squash merge, 2026-10-11.  
**Accepted bounded Work Contract:** `work/EDGE_PICKING_R1_VISIBLE_BODY_EDGES_WORK_CONTRACT.md` (G0–G7, P01–P14; activated by separate governance PR #313 after accepted contract PR #312).  
**Production PR:** [#314](https://github.com/MechanicalCave/SimpleSolid2/pull/314) **MERGED**, accepted unmodified source HEAD `151a18f4ce477745ced3328f5834fe626b27fd04`.  
**Verified `main` squash commit:** [`1a3b60cb4eb89608ad59ba943d85097fcaf8acef`](https://github.com/MechanicalCave/SimpleSolid2/commit/1a3b60cb4eb89608ad59ba943d85097fcaf8acef). GitHub reports PR #314 merged and `main` at this exact SHA. No inferred equivalence to the old feature branch SHA; squash changes the commit ID.

## Independent Owner acceptance sequence

1. **Performance decision — Owner accepted the actual measured latency** (`akceptuje opóźnienie na tym poziomie - kontynuujmy`, 2026-10-11), rather than authorizing unbounded BVH or Viewer API redesign. Evidence is scoped to the actual native Windows DPR 1.0 runner and the recorded Part complexity matrix.
2. **Practical Owner test — manual PASS** (`test manual - PASS - wszystkie krawędzie wykrywam bez problemów ;)`, 2026-10-11): the previously reported seven hard-to-pick visible Body material Edges W01-A–G became detectable without trouble on the up-to-date local R1 feature branch. This is an Owner aggregate, not an invented per-stroke camera or cursor log. See [G7 manual comment](https://github.com/MechanicalCave/SimpleSolid2/pull/314#issuecomment-6111625250).
3. **Distinct formal R1 FINAL PASS:** Owner answered `zatwierdzam - kontynuuj` specifically to the explicit **Edge Picking R1 — FINAL PASS** question, after the manual PASS. [Recorded decision](https://github.com/MechanicalCave/SimpleSolid2/pull/314#issuecomment-6111659584).
4. **Separate merge authorization:** Owner subsequently answered `tak wyrażam zgodę - kontynuuj` to the explicit question **whether to authorize squash merge PR #314 into main**. Only then was PR #314 moved from Draft to Ready for review, exact-head FULL rerun, and the guarded squash merge executed with expected head SHA protection. None of the earlier Owner decisions was treated as automatic merge authorization.

## Immutable native automated verification

- Original expected source HEAD `151a18f4ce477745ced3328f5834fe626b27fd04`, [Windows FULL #2482](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38157825387) **PASS** via temporary exact-SHA, non-draft verification PR #318 (now CLOSED **UNMERGED**).
- Same exact source HEAD, subsequent [production ready-for-review Windows FULL #2483](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38159550991) **PASS**: Core without Qt/OCCT **25/25**, Kernel OCCT-native without Qt **57/57**, Desktop Debug with full real Qt/OCCT Workbench suite **117/117**, total **199/199**; documentation dispatcher and regenerated Browser freshness, bootstrap, workflow `windows-msvc-full` and aggregate `windows-msvc` **success**. No test was disabled or RED waived.
- Same HEAD [Windows FAST #2481](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38157772890) **PASS 93/93**; real modeled OCCT complexity [FOCUSED #2476](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38156445653) PASS and native memory trace [FOCUSED #2477](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38156699393) PASS.
- Baseline and diagnostic-only expected RED evidence PRs #315/#316/#317 are **CLOSED UNMERGED**; they were not transferred into production. Complete measured G5 baseline, corrected R1 and observed limits remain recorded in [R1 Owner witness matrix](https://github.com/MechanicalCave/SimpleSolid2/blob/1a3b60cb4eb89608ad59ba943d85097fcaf8acef/work/EDGE_PICKING_R1_OWNER_WITNESS_01_MATRIX.md).

## Accepted R1 product boundary

- Screen-accurate current-viewer Body material Edge acquisition with a **12 logical-pixel** initial reach, **16 logical-pixel** freshly revalidated incumbent retention band, and **2 logical-pixel** switching advantage. Hover/click share correct current Body/camera/tool/stage identity rules; stale and through-wall picks remain prohibited.
- Nearest *front-visible* Edge fragment and finite ray work budget, with proper perspective/orthographic camera geometry and strict material Edge identity. The original Owner v16/semantics-v2 Part fixture is unchanged; baseline sampled **98/379 independently front-visible misses**, whereas corrected R1 had **0/379** in the same independent five-view check (not exact screenshot camera replay).
- Provider-local early-blocker optimization, no newly approved spatial acceleration structure or public Viewer API change; no OCCT B-Rep feature/Part persistence/Fillet/Chamfer semantics mutation.
- Measured real 0/3/8 native OCCT through-Cut Body scenes had respectively 12/456/1196 mesh triangles and p95 **0.0344/1.6787/5.6122 ms** Edge query latency at DPR 1.0. The Owner expressly accepted **this evidenced scale**, not a universal worst-case SLA or leak-free memory guarantee. DPR 1.5/2.0 were not available on the CI runner; those results were **not claimed**. Limited two-point process-private-byte sampling does not prove long-running leak freedom.
- Internal as-built and PL/EN product docs updated; Product Browser regenerated deterministically and verified under exact-head FULL.

## Status and next gate

- **Edge Picking R1 implementation is COMPLETE and MERGED** under the accepted, already-activated R1 contract. No further product work is implicitly authorized by that contract.
- This file and the companion `work/ACTIVE.yaml` history insertion are **separate, work-only governance closure proposals**, not production changes. The closure PR must pass its appropriate docs/governance CI and obtain any requisite Owner/merge approval separately from the already-consumed PR #314 authorization.
- `work/ACTIVE.yaml -> active_work` intentionally **continues to reference the now-completed R1 Work Contract for traceability**, following the repository's previous PG-01D closure precedent. This reference **does not** authorize new R1 implementation or activate another package.
- **PG-01E is still only a proposal** and **PM-06 remains blocked**. Neither is activated or implemented by this historical closure. The next program scope requires a separately reviewed, Owner-approved bounded Work Contract and a distinct governance activation onto `main`; no automatic authorization is inferred.

## Documentation impact

Internal as-built docs: **not required** — this records historical decisions and exact merged evidence only; behavior is already documented by production PR #314.  
User/Product docs PL/EN: **not required** — no additional user-visible functionality changes.  
Generated Browser: **not required** — no canonical product Markdown modifications.  
Verification: **work-only governance/documentation closure check**; this is not a replacement for exact-head Windows FULL #2483 on the product SHA.
