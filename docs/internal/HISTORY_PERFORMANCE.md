# History Performance — As-built

<!-- doc-id: internal.history-performance -->
<!-- document-kind: internal -->

<!-- section-id: internal.history-performance.baseline -->
## Current history baseline

DocumentSession currently uses the completed C1 runtime history model:

```text
history_: std::vector<HistoryEntry>
HistoryEntry:
  before = PartAuthoredState
  after  = PartAuthoredState
```

C1 prevents command append from deep-copying all earlier history entries. It does not remove the two whole authored-state snapshots retained by each HistoryEntry.

<!-- section-id: internal.history-performance.c2-measurement -->
## C2 Phase A Release measurement

C2 Phase A measured exact SHA `54605075e8ad7ac3d168dfd820cf84004c3a1023` in Release on the self-hosted Windows/MSVC runner. Windows FULL #686 and the dedicated C2 benchmark job both passed.

The primary matrix uses 1,000 and 10,000 Line entities, history depths 10/100/1,000, 20 measured samples after 3 warm-ups, and semantic Add/Edit/100-entity Transform/Undo/Redo/one-step Branch operations. All six primary cells completed without safety cutoffs.

| Entities | Depth | Peak WS MiB | Setup median ms |
| --- | --- | --- | --- |
| 1,000 | 10 | 5.0 | 0.432 |
| 1,000 | 100 | 13.2 | 0.828 |
| 1,000 | 1,000 | 89.9 | 35.581 |
| 10,000 | 10 | 14.4 | 3.528 |
| 10,000 | 100 | 84.6 | 9.160 |
| 10,000 | 1,000 | 787.4 | 146.570 |

Across primary semantic operations the largest measured median is 421.45 µs, largest p95 is 457.20 µs and largest maximum is 479.40 µs. At 10,000 entities/depth 1,000, medians are 313.90 µs Add, 298.40 µs Edit, 317.10 µs 100-entity Transform, 67.50 µs Undo, 69.15 µs Redo and 252.60 µs one-step Branch.

The supplemental half-depth branch sample reaches 7.9412 ms at 10,000 entities/depth 1,000. It is a single limited sample and is not percentile-quality evidence.

The full matrix and exact environment are recorded in [C2 History Benchmark Results](../../work/C2_HISTORY_BENCHMARK_RESULTS.md).

<!-- section-id: internal.history-performance.boundaries -->
## Interpretation boundary

The benchmark is headless semantic history evidence. It excludes Qt UI, Viewer/OCCT rendering, input/picking and filesystem Save from timed operations.

The measured command latency remains sub-millisecond across all primary matrix cells on the measured runner, while retained-history process memory grows strongly with model size and history depth. This is evidence for the C2 Owner decision; it is not by itself a product history limit or authorization to redesign history.

The Owner accepted the C2 **KEEP** decision on 2026-09-28. The current C1 two-snapshot representation remains the production baseline, with no Product history depth/memory limit and no automatic eviction.

The measured retained-history memory growth remains a documented scale risk. A future redesign requires new evidence and separately accepted authority; this C2 decision does not pre-authorize event sourcing, delta/COW history or persistent Undo.
