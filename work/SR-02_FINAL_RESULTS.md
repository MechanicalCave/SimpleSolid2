# SR-02 — Final before/after latency evidence

**Status:** FINAL RUNTIME EVIDENCE — Owner manual Windows verification pending  
**Baseline exact SHA:** `ed61c763215b4bbdb7199781af8dd01d71f5d22c`  
**Baseline Windows FULL:** #1140 — PASS  
**Final runtime SHA:** `530eda52f748b45099fa24a4ac5c9d782c068bee`  
**Final Windows FULL:** #1160 — PASS  
**Machine:** DESKTOP-JL6O311  
**CPU:** Intel(R) Core(TM) i9-10850K CPU @ 3.60GHz  
**OS:** Microsoft Windows 10 Pro 10.0.19045  
**RAM:** 63.92 GiB  
**Benchmark build:** Release

These measurements are engineering evidence for SR-02. They are not Product latency guarantees.

The benchmark measures the complete synchronous controller/provider operation followed by Qt event-drain processing. The current public Viewer boundary has no compositor-visible completion callback; adding one only for measurement would cross the accepted D2 stop boundary. Owner manual Windows verification therefore remains the required final check that the intended frame is visibly coherent and that Finish Sketch no longer appears staged.

The historical benchmark scenario name `authored_refresh_to_redraw` is slightly broader than what it actually times: it calls the production `PartViewportController::refreshPresentation()` path repeatedly against authoritative Document state, but does not itself execute an authored mutation inside the timed interval. Existing command/transaction regressions prove authored mutation semantics; the timing row below should be read as **authoritative presentation refresh**, not command execution latency.

## 1. Interactive preview

| Scenario | Workload | Samples | Baseline p50 / p95 / max | Final p50 / p95 / max | Resolver/event | Provider work/event |
| --- | --- | ---: | --- | --- | --- | --- |
| Line preview | 100 Lines | 30 | 66.729 / 66.762 / 66.829 ms | 16.677 / 16.758 / 16.829 ms | 1 → 1 | 2 update + 2 redraw → 1 update + 0 redraw |
| Circle preview | 100 Lines | 30 | 66.719 / 69.642 / 76.215 ms | 16.675 / 16.779 / 16.828 ms | 0 → 0 | 2 + 2 → 1 + 0 |
| Arc preview | 100 Lines | 30 | 66.719 / 70.316 / 74.558 ms | 16.676 / 16.774 / 25.065 ms | 1 → 1 | 2 + 2 → 1 + 0 |
| MOVE preview | 100 Lines | 30 | 66.720 / 66.774 / 66.783 ms | 16.676 / 16.732 / 17.455 ms | 2 → 1 | 2 + 2 → 1 + 0 |
| Line preview | 1,000 Lines | 20 | 66.722 / 66.743 / 66.754 ms | 16.667 / 16.878 / 22.807 ms | 1 → 1 | 2 + 2 → 1 + 0 |
| MOVE preview | 1,000 Lines | 20 | 66.716 / 66.810 / 66.843 ms | 16.678 / 16.739 / 16.757 ms | 2 → 1 | 2 + 2 → 1 + 0 |
| Line preview | 5,000 Lines | 12 | 83.365 / 83.450 / 83.450 ms | 18.738 / 21.065 / 21.065 ms | 1 → 1 | 2 + 2 → 1 + 0 |
| Circle preview | 5,000 Lines | 12 | 66.716 / 66.842 / 66.842 ms | 16.044 / 17.286 / 17.286 ms | 0 → 0 | 2 + 2 → 1 + 0 |
| Arc preview | 5,000 Lines | 12 | 83.405 / 83.434 / 83.434 ms | 18.682 / 18.993 / 18.993 ms | 1 → 1 | 2 + 2 → 1 + 0 |
| MOVE preview | 5,000 Lines | 12 | 100.084 / 100.096 / 100.096 ms | 18.880 / 19.351 / 19.351 ms | 2 → 1 | 2 + 2 → 1 + 0 |

Representative reductions: 100-Line Line preview p50 is 75.0% lower; 5,000-Line MOVE p50 is 81.1% lower.

The remaining 5,000-Line Line/Arc/MOVE spread comes from the one remaining exact nearby/presentation query and geometry projection work, not duplicate pointer resolution or repeated provider flushes.

## 2. Rectangle-query work

| Workload | Samples | Baseline p50 / p95 | Final p50 / p95 | Baseline token comparisons/query | Final lookup shape |
| --- | ---: | --- | --- | ---: | --- |
| 100 Lines | 30 | 0.048 / 0.063 ms | 0.050 / 0.052 ms | 4,950 | 100 token lookups, 0 legacy linear comparisons |
| 1,000 Lines | 20 | 2.819 / 2.838 ms | 2.374 / 2.459 ms | 499,500 | 1,000 lookups, 0 legacy linear comparisons |
| 5,000 Lines | 12 | 75.565 / 77.100 ms | 58.202 / 59.557 ms | 12,497,500 | 5,000 lookups, 0 legacy linear comparisons |
| 300 mixed Line/Circle/Arc | 30 | 3.750 / 3.780 ms | 1.397 / 1.431 ms | 2,351,350 average/sample | 13,300 segment lookups/query, 0 legacy linear comparisons |

The vector remains the deterministic output-order owner. A private token→vector-index map removes the O(N²) accumulator search without changing window/crossing ordering or exact segment intersection tests.

The 5,000-Line full crossing query still costs about 58 ms because all 5,000 projected segments are tested. SR-02 does not add a spatial index: after the lower-risk work, ordinary 5,000-Line pointer preview is already about 16–19 ms, so a camera/DPI-dependent spatial structure is not justified by the current interaction evidence. The full rectangle-selection scale result remains a documented risk.

## 3. Authoritative presentation refresh

| Workload | Samples | Baseline p50 / p95 / max | Final p50 / p95 / max | Provider work/refresh |
| --- | ---: | --- | --- | --- |
| 100 Lines | 30 | 133.437 / 133.502 / 133.526 ms | 16.680 / 16.744 / 16.779 ms | 4 update + 4 redraw → 1 update + 0 redraw |
| 1,000 Lines | 20 | 133.451 / 133.497 / 133.669 ms | 25.367 / 26.282 / 27.496 ms | 4 + 4 → 1 + 0 |
| 5,000 Lines | 12 | 314.578 / 333.638 / 333.638 ms | 150.002 / 156.785 / 156.785 ms | 4 + 4 → 1 + 0 |
| 300 mixed Line/Circle/Arc | 30 | 764.591 / 813.718 / 814.816 ms | 164.824 / 168.369 / 169.320 ms | 4 + 4 → 1 + 0 |

The mixed fixture contains 100 Lines, 100 Circles and 100 Arcs. Its exact neutral query representation still contains 13,300 derived line segments. Before the native-object aggregation, Qt/OCCT created one native object per segment; the final provider creates one native Sketch object per semantic entity, so the measured current count is 300.

The neutral point chains and rectangle-query approximation are unchanged. Only provider-native grouping changed.

## 4. Profile hover

| Scenario | Samples | Baseline p50 / p95 / max | Final p50 / p95 / max | analyzeRegions/hover | Provider work/hover |
| --- | ---: | --- | --- | --- | --- | --- |
| cold | 1 | 100.106 / 100.106 / 100.106 ms | 21.506 / 21.506 / 21.506 ms | 1 → 1 | 3 + 3 → 1 + 0 |
| warm, no draft | 30 | 100.079 / 100.142 / 100.154 ms | 16.677 / 16.716 / 16.744 ms | 0 → 0 | 3 + 3 → 1 + 0 |
| warm, existing draft | 30 | 100.079 / 100.113 / 100.149 ms | 16.682 / 16.767 / 17.815 ms | 1 → 0 | 3 + 3 → 1 + 0 |

Profile analysis is reused only when the full semantic context remains valid. Hover-result reuse binds Document/session identity, SketchId, observed revision, draft RegionIntent, hovered region and Add/Subtract mode. A real active-Sketch state change rebuilds analysis. An unrelated Document revision causes one Sketch-state comparison but does not silently rebase the draft optimistic-concurrency revision.

## 5. Finish Sketch

For 1,000 Lines with an active Line preview:

| | Baseline | Final |
| --- | ---: | ---: |
| Samples | 20 | 20 |
| p50 | 433.707 ms | 66.740 ms |
| p95 | 433.827 ms | 66.793 ms |
| max | 450.370 ms | 66.800 ms |
| UpdateCurrentViewer / Finish | 13 | 3 |
| explicit Redraw / Finish | 13 | 1 |

Finish p50 is 84.6% lower. Exact empty runtime-scene requests are suppressed, unchanged stable authored scenes are not rebuilt, and immediate duplicate redraws after synchronous viewer update are removed on the proven paths.

The benchmark times Finish to the stable non-edit event-drained state. It re-enters Sketch edit between samples to restore the next active-edit fixture, but re-entry is not reported as a separate timing cell. The accepted manual verification therefore explicitly exercises repeated Finish → re-enter cycles and checks that no externally visible staged teardown remains.

## 6. Bounded optimizations accepted by the evidence

SR-02 production changes remain provider/runtime-only:

- exact no-op suppression for unchanged snap/inference and already-empty transient scenes;
- one complete `ResolvedSketchInput` per common-transform preview event;
- deterministic linear-average semantic-token accumulation;
- Profile analysis and hover-result reuse with complete context invalidation;
- stable authored Reference/empty-scene no-op suppression where equality is provable;
- one native OCCT wire object per already-derived Circle/Arc point chain while retaining the neutral semantic token and exact point chain;
- removal of an immediate duplicate `V3d_View::Redraw()` after synchronous `UpdateCurrentViewer()` on the measured Sketch preview, Profile preview and authored Sketch-scene paths.

No public Viewer mutation API, persistent cache, semantic provider identity, snap/topology rule, RegionIntent meaning, history boundary or authored schema changed.

## 7. Remaining risks and non-goals

The remaining measured scale risks are:

- full 5,000-Line rectangle crossing selection at about 58 ms;
- 5,000-Line full authored Sketch replacement at about 150 ms;
- 300 mixed Line/Circle/Arc full authored refresh at about 165 ms despite native-object aggregation.

These costs no longer dominate ordinary pointer preview in the measured fixtures. SR-02 therefore stops before a spatial index or public differential Viewer architecture. Those remain evidence-driven future options if larger Product workloads make them necessary.

The benchmark cannot independently prove compositor-visible frame completion through the current public boundary. That is why Owner manual Windows verification is still an acceptance condition rather than being replaced by setter timing.

## 8. Manual acceptance still required

The final runtime candidate for Owner verification is `530eda52f748b45099fa24a4ac5c9d782c068bee` unless a later documentation-only/exact-head verification commit supersedes it without production-behavior changes.

Required manual smoke remains:

1. representative drawing workflow that previously felt delayed;
2. repeated Line/Circle/Arc pointer preview with OSNAP/OTRACK unchanged;
3. selection plus Move/Copy/direct-manipulation preview;
4. Profile hover/edit on a closed-region Sketch;
5. repeated Finish Sketch → re-enter Sketch Edit cycles with no visibly staged teardown;
6. SR-01 Delete / explicit Delete Profile / Ctrl+Z / Ctrl+Y / text-editor ownership regression;
7. Save → Close → Reopen.

Owner PASS is required before work-only CLOSURE closeout.
