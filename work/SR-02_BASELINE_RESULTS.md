# SR-02 Phase A baseline evidence

**Status:** BASELINE CAPTURED  
**Exact SHA measured:** `ed61c763215b4bbdb7199781af8dd01d71f5d22c`  
**Windows FULL:** #1140 — PASS  
**Machine:** DESKTOP-JL6O311  
**CPU:** Intel(R) Core(TM) i9-10850K CPU @ 3.60GHz  
**OS:** Microsoft Windows 10 Pro 10.0.19045  
**RAM:** 63.92 GiB  
**Benchmark build:** Release

This is measurement evidence for the SR-02-complete instrumentation baseline. It is not a Product performance guarantee and does not identify an unavoidable OCCT floor.

## 1. Key interactive timing

| Scenario | Workload | Samples | p50 | p95 | Resolver/event | Update+Redraw/event |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Line preview | 100 Lines | 30 | 66.729 ms | 66.762 ms | 1 | 2 |
| Circle preview | 100 Lines | 30 | 66.719 ms | 69.642 ms | 0 | 2 |
| Arc preview | 100 Lines | 30 | 66.719 ms | 70.316 ms | 1 | 2 |
| MOVE preview | 100 Lines | 30 | 66.720 ms | 66.774 ms | 2 | 2 |
| Line preview | 1,000 Lines | 20 | 66.722 ms | 66.743 ms | 1 | 2 |
| MOVE preview | 1,000 Lines | 20 | 66.716 ms | 66.810 ms | 2 | 2 |
| Line preview | 5,000 Lines | 12 | 83.365 ms | 83.450 ms | 1 | 2 |
| MOVE preview | 5,000 Lines | 12 | 100.084 ms | 100.096 ms | 2 | 2 |

The near-quantized 66.7 / 83.3 / 100 ms plateaus are a strong reason to investigate synchronous provider flush cadence, but this evidence alone does not prove a hardware/OCCT floor.

## 2. Rectangle-query scaling

| Workload | Samples | p50 | p95 | Token comparisons / query |
| --- | ---: | ---: | ---: | ---: |
| 100 Lines | 30 | 0.048 ms | 0.063 ms | 4,950 |
| 1,000 Lines | 20 | 2.819 ms | 2.838 ms | 499,500 |
| 5,000 Lines | 12 | 75.565 ms | 77.100 ms | 12,497,500 |
| 300 mixed Line/Circle/Arc | 30 | 3.750 ms | 3.780 ms | 2,351,350 average/sample over 13,300 derived segments |

The measured counter confirms the pre-activation static finding: the current vector/`find_if` semantic-token accumulator performs O(N²) token comparisons for distinct Line tokens.

## 3. Authored refresh

| Workload | Samples | p50 | p95 | Update+Redraw / refresh |
| --- | ---: | ---: | ---: | ---: |
| 100 Lines | 30 | 133.437 ms | 133.502 ms | 4 |
| 1,000 Lines | 20 | 133.451 ms | 133.497 ms | 4 |
| 5,000 Lines | 12 | 314.578 ms | 333.638 ms | 4 |
| 300 mixed Line/Circle/Arc | 30 | 764.591 ms | 813.718 ms | 4 |

The mixed-curve result shows that derived/native presentation multiplication remains material even at much lower semantic entity counts.

## 4. Profile hover

| Scenario | Samples | p50 | p95 | analyzeRegions / hover | Update+Redraw / hover |
| --- | ---: | ---: | ---: | ---: | ---: |
| cold, no draft | 1 | 100.106 ms | 100.106 ms | 1 | 3 |
| warm, no draft | 30 | 100.079 ms | 100.142 ms | 0 | 3 |
| warm, existing draft | 30 | 100.079 ms | 100.113 ms | 1 | 3 |

This proves that the outer Profile-analysis cache does not cover the complete draft-hover path: unchanged warm hover with a draft still reaches one `analyzeRegions()` per event through downstream Profile composition work.

At this 100-entity fixture the visible timing is nevertheless dominated by the three synchronous presentation flushes, because warm no-draft and warm-with-draft have effectively identical p50.

## 5. Finish Sketch

For 1,000 Lines with an active preview:

- samples: 20;
- p50: **433.707 ms**;
- p95: **433.827 ms**;
- max: **450.370 ms**;
- total `UpdateCurrentViewer` calls: 260;
- total `Redraw` calls: 260;
- therefore **13 update/redraw pairs per Finish**.

This directly supports the Owner-observed staged disappearance. It does not yet prove which teardown setters can be safely suppressed or batched.

## 6. Phase B ordering derived from evidence

The first bounded optimization should target exact redundant presentation work, because ordinary 100-entity Line/Circle/Arc preview is already ~66.7 ms with two update/redraw pairs per event even where resolver/query work is absent.

The first experiment is therefore exact no-op suppression for unchanged snap/inference scenes. It remains synchronous, changes no public Viewer contract and preserves provider failure reporting because work is skipped only when the requested runtime scene is byte-for-byte/semantic-value equal to the already installed scene.

After that isolated change, re-run the same benchmark before proceeding to the independently measurable duplicate-transform resolver, O(N²) token accumulation, Profile-hover analysis reuse and Finish Sketch batching.
