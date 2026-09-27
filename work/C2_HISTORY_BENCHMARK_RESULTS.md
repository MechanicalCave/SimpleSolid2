# C2 — History Benchmark Results

**Status:** PHASE A EVIDENCE COMPLETE — SECOND OWNER DECISION PENDING  
**Measured SHA:** `54605075e8ad7ac3d168dfd820cf84004c3a1023`  
**Windows PR gate:** #686 — PASS  
**C2 Release benchmark job:** #686 — PASS  
**Artifact:** `c2-history-54605075e8ad7ac3d168dfd820cf84004c3a1023` (artifact id `10943125647`)  
**Artifact digest:** `sha256:f23291717d21a35bb7e585290aa94d4a813eee8459284e46ef5b27388c23f827`  
**Measured:** 2026-09-27

## 1. Environment

| Field | Value |
| --- | --- |
| Configuration | Release |
| Compiler | MSVC 19.38.33133 (`_MSC_FULL_VER=193833133`) |
| OS | Microsoft Windows 10 Pro 10.0.19045 build 19045 |
| CPU | Intel Core i9-10850K @ 3.60 GHz |
| Logical processors | 20 |
| Physical RAM | 68,637,700,096 bytes (~63.9 GiB) |
| Runner class | `self-hosted-windows-x64-simplesolid2-native` |
| Primary samples | 20 after 3 warm-ups |
| Transform primary | 100 existing entities |
| Working-set safety ceiling | min(requested 4096 MiB, 25% physical RAM, 4096 MiB) = 4096 MiB |
| Per-cell timeout | 5 minutes |
| Child process priority | BelowNormal |

The safety ceiling belongs only to benchmark execution. It is not a SimpleSolid 2.0 product history limit.

## 2. Completion

All six primary entity-count/history-depth cells completed. No working-set or timeout cutoff occurred.

Each primary operation has 20 measured samples. The supplemental half-depth branch case is intentionally a single limited sample per cell and is not treated as percentile-quality evidence.

## 3. Primary semantic latency

| Entities | Depth | Operation | n | Median µs | p95 µs | Max µs |
| --- | --- | --- | --- | --- | --- | --- |
| 1000 | 10 | add | 20 | 44.65 | 70.90 | 76.60 |
| 1000 | 10 | edit | 20 | 37.25 | 56.50 | 66.40 |
| 1000 | 10 | transform | 20 | 39.55 | 52.90 | 54.10 |
| 1000 | 10 | undo | 20 | 4.50 | 20.50 | 37.80 |
| 1000 | 10 | redo | 20 | 4.10 | 20.80 | 28.20 |
| 1000 | 10 | branch | 20 | 21.35 | 42.10 | 52.00 |
| 1000 | 100 | add | 20 | 29.90 | 42.90 | 50.60 |
| 1000 | 100 | edit | 20 | 27.25 | 38.40 | 38.80 |
| 1000 | 100 | transform | 20 | 34.70 | 50.20 | 57.10 |
| 1000 | 100 | undo | 20 | 4.70 | 4.90 | 11.60 |
| 1000 | 100 | redo | 20 | 4.20 | 4.40 | 4.50 |
| 1000 | 100 | branch | 20 | 5.60 | 5.80 | 36.60 |
| 1000 | 1000 | add | 20 | 57.50 | 109.50 | 120.30 |
| 1000 | 1000 | edit | 20 | 56.60 | 115.20 | 133.80 |
| 1000 | 1000 | transform | 20 | 75.30 | 122.30 | 123.70 |
| 1000 | 1000 | undo | 20 | 10.25 | 11.30 | 11.60 |
| 1000 | 1000 | redo | 20 | 10.25 | 12.20 | 12.60 |
| 1000 | 1000 | branch | 20 | 15.85 | 16.90 | 30.90 |
| 10000 | 10 | add | 20 | 421.45 | 457.20 | 479.00 |
| 10000 | 10 | edit | 20 | 272.95 | 395.60 | 421.80 |
| 10000 | 10 | transform | 20 | 235.50 | 402.80 | 415.40 |
| 10000 | 10 | undo | 20 | 44.50 | 212.60 | 273.10 |
| 10000 | 10 | redo | 20 | 40.20 | 191.10 | 199.50 |
| 10000 | 10 | branch | 20 | 211.50 | 308.80 | 371.70 |
| 10000 | 100 | add | 20 | 241.80 | 272.00 | 282.90 |
| 10000 | 100 | edit | 20 | 228.35 | 241.70 | 259.00 |
| 10000 | 100 | transform | 20 | 253.25 | 263.30 | 266.90 |
| 10000 | 100 | undo | 20 | 46.70 | 49.40 | 55.80 |
| 10000 | 100 | redo | 20 | 44.00 | 44.90 | 48.50 |
| 10000 | 100 | branch | 20 | 212.80 | 293.60 | 299.30 |
| 10000 | 1000 | add | 20 | 313.90 | 329.50 | 331.90 |
| 10000 | 1000 | edit | 20 | 298.40 | 396.30 | 437.40 |
| 10000 | 1000 | transform | 20 | 317.10 | 336.90 | 479.40 |
| 10000 | 1000 | undo | 20 | 67.50 | 78.60 | 80.60 |
| 10000 | 1000 | redo | 20 | 69.15 | 71.10 | 71.50 |
| 10000 | 1000 | branch | 20 | 252.60 | 268.10 | 272.20 |

## 4. Setup and process memory

| Entities | Depth | Setup median ms | Post-setup WS MiB | Peak WS MiB |
| --- | --- | --- | --- | --- |
| 1000 | 10 | 0.432 | 5.0 | 5.0 |
| 1000 | 100 | 0.828 | 13.0 | 13.2 |
| 1000 | 1000 | 35.581 | 89.9 | 89.9 |
| 10000 | 10 | 3.528 | 13.6 | 14.4 |
| 10000 | 100 | 9.160 | 83.8 | 84.6 |
| 10000 | 1000 | 146.570 | 779.1 | 787.4 |

The largest measured semantic-only process peak is **787.4 MiB** at 10,000 entities and history depth 1,000. The corresponding post-setup working set is **779.1 MiB**.

## 5. Supplemental long-Redo destruction

| Entities | Depth | n | Half-depth branch µs | Evidence |
| --- | --- | --- | --- | --- |
| 1000 | 10 | 1 | 59.20 | limited single sample |
| 1000 | 100 | 1 | 18.40 | limited single sample |
| 1000 | 1000 | 1 | 3017.20 | limited single sample |
| 10000 | 10 | 1 | 440.10 | limited single sample |
| 10000 | 100 | 1 | 319.10 | limited single sample |
| 10000 | 1000 | 1 | 7941.20 | limited single sample |

The half-depth branch case exposes Redo-suffix destruction cost but has one sample per cell. It is directional evidence only. The largest observed value is **7.9412 ms** for 10,000 entities at depth 1,000 while replacing roughly half of the history.

## 6. Measured facts

The largest primary-operation median is **421.45 µs** (`add`, 10,000 entities, depth 10). The largest primary p95 is **457.20 µs** for the same case. The largest primary maximum is **479.40 µs** (`transform`, 10,000 entities, depth 1,000).

At the largest matrix cell, primary medians are 313.90 µs Add, 298.40 µs Edit, 317.10 µs 100-entity Transform, 67.50 µs Undo, 69.15 µs Redo and 252.60 µs one-step Branch.

History preparation memory rises strongly with both authored model size and retained history depth. Peak working set changes from 14.4 MiB at 10,000 entities/depth 10 to 84.6 MiB at depth 100 and 787.4 MiB at depth 1,000.

C1 removed the former history-depth-dependent copy from the command append path, but the retained two-snapshot history still has a material model-size × history-depth memory cost.

## 7. Interpretation boundaries

These are semantic `DocumentSession` measurements. They exclude Qt UI, Viewer/OCCT rendering, input/picking adapters and filesystem Save from the timed operation.

The evidence belongs to the exact SHA, machine and toolchain above. Non-monotonic latency differences between some depths are not interpreted as causal improvements; they are ordinary run-level variation unless reproduced by a dedicated experiment.

No product memory/depth budget is inferred from this one machine. No production history representation change is authorized by Phase A.

## 8. Second Owner decision gate

Phase A now stops at the second Owner decision gate defined by the accepted C2 Work Contract. The bounded categories remain: keep the current C1 representation for now; approve a bounded private representation optimization; approve a product-visible history budget with explicit behavior/documentation; or request another experiment.

Before any production history mutation, the Owner must explicitly select and accept the next C2 direction. If a change is selected, the contract must be amended to state the exact representation problem, preserved invariants and verification evidence.
