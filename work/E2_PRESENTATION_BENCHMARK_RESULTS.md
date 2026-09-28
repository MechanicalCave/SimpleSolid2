# E2 — Presentation Benchmark Results

**Package:** AUDIT-01 E2  
**Measured SHA:** `401629b8cc4f943e6ad5577918bb1999256fdc37`  
**Windows FULL / Release benchmark:** #727 — PASS  
**Date:** 2026-09-28  
**Classification:** KEEP CURRENT REBUILD

## Environment

```text
OS: Microsoft Windows 10 Pro
CPU: Intel(R) Core(TM) i9-10850K CPU @ 3.60GHz
Logical CPUs: 20
RAM: 63.92 GiB
Build: Release
```

The benchmark uses the production Qt/OCCT viewport and production PartViewportController presentation paths. Timing thresholds are not Product limits or test pass/fail criteria.

## Verification shape

The ordinary exact-head gate remained unchanged at **77/77 CTest PASS**. E2 Phase A added no new CTest executable: call-count and preview-failure characterization was consolidated into the existing `sk03a.part_viewport_controller` regression.

The Release benchmark is separate evidence tooling, not a latency-threshold unit test.

## Results

| Operation | Workload | Semantic entities | Derived/native objects | Samples | Median | p95 | Max |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| accepted mutation + authored refresh | lines | 100 | 101 | 7 | 100.053 ms | 100.161 ms | 100.161 ms |
| transform preview | lines | 100 | 100 | 7 | 33.362 ms | 33.385 ms | 33.385 ms |
| single-line preview | lines | 1 | 1 | 7 | 33.350 ms | 36.736 ms | 36.736 ms |
| native scene replacement | lines | 100 | 101 | 7 | 33.357 ms | 33.418 ms | 33.418 ms |
| accepted mutation + authored refresh | lines | 1,000 | 1,001 | 5 | 100.099 ms | 104.888 ms | 104.888 ms |
| transform preview | lines | 1,000 | 1,000 | 5 | 33.379 ms | 33.385 ms | 33.385 ms |
| native scene replacement | lines | 1,000 | 1,001 | 5 | 33.352 ms | 33.420 ms | 33.420 ms |
| accepted mutation + authored refresh | lines | 5,000 | 5,001 | 3 | 150.150 ms | 150.188 ms | 150.188 ms |
| transform preview | lines | 5,000 | 5,000 | 3 | 83.404 ms | 83.414 ms | 83.414 ms |
| native scene replacement | lines | 5,000 | 5,001 | 3 | 83.397 ms | 83.406 ms | 83.406 ms |
| native scene replacement | mixed Line/Circle/Arc | 100 | 3,995 | 5 | 83.385 ms | 83.444 ms | 83.444 ms |

The "accepted mutation + authored refresh" measurement intentionally follows the production sequence and therefore includes the accepted semantic update immediately before `refreshPresentation()`. The direct native-scene replacement rows isolate the provider replacement cost from that complete production path.

## Call-count characterization

The existing controller regression now proves that:

- ordinary Line/Circle/Arc preview uses only the transient preview channel;
- transform/direct-manipulation preview uses only the transient preview channel;
- passive pointer movement does not call `setSketchScene()`;
- an accepted authored mutation followed by `refreshPresentation()` performs the authored scene update;
- preview-provider rejection changes no authored state, DocumentRevision, dirty state or Undo depth.

## Decision

**KEEP CURRENT REBUILD.**

Evidence does not justify a new differential-update protocol or public `IDocumentViewport` expansion:

- a one-object preview and a 1,000-line preview/replacement both sit near the same ~33 ms floor on the measured runner, indicating redraw/frame scheduling dominates at small-to-medium scene sizes;
- scaling becomes material around 4,000–5,000 native presentation objects, where replacement/preview reaches ~83 ms;
- an accepted 5,000-line mutation plus full production refresh remains ~150 ms median;
- mixed curves visibly multiply native presentation objects and remain a documented scale risk.

This is not a claim that current presentation is optimal or a Product latency guarantee. Large and heavily segmented Sketches remain a measured risk. Future optimization requires new evidence if product scale or interaction requirements make these values unacceptable.

No differential-update/public Viewer API work is authorized or required by this result.

## Boundary for Package F

Circle/Arc point chains and native AIS segments measured here are derived presentation data only. Their endpoints, intersections and closure are not region/profile semantics. Package F must analyze exact accepted authored/evaluated Shared 2D geometry outside the Viewer.
