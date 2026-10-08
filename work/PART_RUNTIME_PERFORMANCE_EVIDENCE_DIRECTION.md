# Part Runtime Evaluation and Presentation Performance — Accepted Direction

**Status:** OWNER-ACCEPTED DIRECTION — NOT ACTIVE FOR IMPLEMENTATION
**Date:** 2026-10-08
**Decision class:** D2 direction; specific cache algorithms and budgets remain evidence-gated
**Current active work:** PM-05F R2 only
**Related:** ADR-0014 stage-scoped evaluation, ADR-0016 topology catalog, CI-04 build optimization
**Program:** Part Modeling v1.29; PM-06 owns final Part-v1 performance evidence

## Goal

As Body history and topology complexity increase, keep interactive engineering workflows responsive without changing evaluation or reference semantics.

No runtime optimization may become CAD modeling authority or a second source of durable identity.

## Ordered evidence-first program

### Measurement baseline — next separate activation

Use the same Windows/MSVC configuration and timed run for:

- 10 / 50 / 100 Features including Extrude, Revolve and mixed Fillet/Chamfer where accepted;
- cold full-history evaluation;
- repeat evaluation with identical authoring;
- Edit at early, middle and late history stages;
- rapid parameter changes and preview cancellation;
- mesh construction and native Qt/OCCT Edge picking at several edge/triangle counts and view sizes;
- current/resolved-prefix catalog processing, Surface/Curve/Point classification and memory use.

Record wall-clock median/p95, peak working set, number of Kernel operations, number of topology classifications, mesh rebuilds, bytes retained and correctness/stale-publication status. Separate Kernel compute, Part semantic evaluation, cache management, tessellation, picking and UI binding. Define performance budgets only after reproducible baseline evidence.

### Candidate optimization A — stage-prefix reuse

Reuse immutable, verified results of unchanged Body history prefixes. After editing Feature N, recompute N and the required downstream suffix, not the unchanged prefix by default. Cache keys must include all semantic inputs and contexts that can affect a stage result, including stage identity, document revision/dependency snapshots and modeling/provider/toolchain policy. Do not treat a transient source SHA or geometry similarity as CAD identity.

Cancellation, undo/redo, document switching and new kernel/session generations invalidate publication authority; a cache hit never renews a stale publication lease. Failed/Blocked evaluations cannot serve stale last-good current Body truth. Cold rebuild without cache must give the same semantic result.

### Candidate optimization B — bounded topology indexes

Derive temporary indexes over the already complete stage catalog (token -> record; Surface pair -> Curve; Vertex/Edge adjacency). Avoid repeated full-array scanning in interactive selection and authoring. Indexes must preserve complete accounting, candidate cardinality and deterministic fail-closed branch resolution. Per-stage/generation invalidation is mandatory.

### Candidate optimization C — Viewer and preview

Keep separate viewport-facing caches for tessellation, presentation-only Edge sampling and spatial hit-test structures. Ensure visible curved material Edges remain selectable without admitting occluded/rear edges. Do not turn mesh approximation or screen tolerance into exact modeling input.

For local Fillet/Chamfer delta preview, measure boolean-difference and targeted-face alternatives independently; do not select a slow exact-difference strategy without evidence or accept a fast incorrect whole-Body highlight.

## Gates and exclusions

Any future optimization implementation requires a separate bounded work contract and Owner acceptance of the measured design. No global dependency graph, cache persistence as identity, fuzzy rebinding, modified authored schema or weakened FULL/Owner acceptance is allowed.

Measure before optimize. All stage-state, Undo/Redo, stale document/revision, Save/Close/Reopen, cold rebuild and false-Resolved=0 regressions remain mandatory. Performance changes need exact-head build and test evidence; test result equality must be demonstrated between cold and warm caches.

This document does not activate PM-06, CI-05, cache implementation or new Product features.

## Documentation impact

Internal docs: required when implementation is later activated
User/Product docs: not required for direction-only document
Reason: measurement and runtime performance contracts are maintainer-facing. Any later user-visible behavior change must be declared in its own implementation contract.
