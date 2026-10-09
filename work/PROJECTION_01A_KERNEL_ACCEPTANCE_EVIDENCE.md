# PG-01A — Kernel / OCCT Exact Projection Evidence

**Status:** IMPLEMENTING; Windows FOCUSED PASS, wider FAST and final exact-head FULL pending. Not Owner-final PASS.
**Date:** 2026-10-09
**PR:** [#300](https://github.com/MechanicalCave/SimpleSolid2/pull/300)
**Contract:** `work/PROJECTION_01A_EXACT_KERNEL_CONTRACT.md`
**Baseline:** Projection 00A merged `2cda8e04d7d0e2594edae7dc90e88cbb90dcae35`

## Exact Windows evidence

- **#1917** — activation-only CLOSURE PASS; not a source test.
- [**FOCUSED #1918**](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37892785961) on `0c0d6bd44463469ce9e5a7709e9f48d1c9afc809`: **1/1 PASS**, Line and Circle exact geometry, orthographic and offset/rotated frames, typed errors.
- [**FOCUSED #1919**](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893017161) on `1b09f8a9cb72e140cc923682c040fe4503a67f73`: **1/1 PASS**, oriented Arc, reversed source, seam crossing, tilted Unsupported, end-on Line degeneracy.
- [**FOCUSED #1920**](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893301439) on `b0f9732985f80e0c159c128119afb58f7bd65092`: **1/1 PASS**, OCCT Edges after Extrude/Revolve/Fillet/Chamfer.
- [**FOCUSED #1921**](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893577900) on `60ccf035bd78eb298bedefe4cca274b4f0d6ff52`: **RED** C3861 in test fixture (test helper declared below its call); corrected without weakening coverage.
- [**FOCUSED #1922**](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37893703340) on `9ee476ae6191c78c2b04fddab4ac33b42e0dd5a7`: **1/1 PASS**, opaque `ScopedProjectionEdge` bound to runtime Body, rejects stale-source re-targeting even when the independent Body token values collide.

## Coverage vs remaining gate

| Area | Verified / open |
| --- | --- |
| Exact provider-neutral 2D Line / Circle / Arc, current OCCT solid Edge | FOCUSED PASS |
| Circle-to-ellipse Unsupported, projected Line collapse, invalid frame, missing token, foreign Body, deterministic repeat | FOCUSED PASS |
| Provider-scoped Edge reused with different runtime Body having same numeric token | FOCUSED PASS — `provider_mismatch`, no silent geometry |
| Initial **semantic** capture of the correct current-stage Edge | PG-01B Part responsibility, not done here. Binding a copied raw integer to the wrong Body would still be semantically incorrect without caller validation |
| Explicit B-Spline source construction, synthetic provider-exception branch | OPEN test coverage; generic Unsupported and KernelFailure branches implemented, not claimed as executed |
| Wider FAST, Ready-for-Review exact-head Windows FULL | PENDING |
| Owner final PG-01A acceptance and merge | PENDING |

**Source boundaries:** SS2 `kernel/edge_projection.hpp`, `kernel_occt/solid_modeling_kernel.hpp/.cpp` and existing `pm02p.e_kernel_lifecycle`; no Part/Sketch/UI/persistence/PM-06 changes or CMake modifications. Canonical internal as-built doc `docs/internal/PART_EDGE_PROJECTION_KERNEL.md` and generated Browser updated together. No user-facing Project Geometry feature ships in PG-01A.
