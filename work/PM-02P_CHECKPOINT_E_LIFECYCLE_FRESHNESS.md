# PM-02P.E — Lifecycle / Freshness Evidence

**Status:** COMPLETED — PASS  
**Date:** 2026-10-04  
**Active contract:** `work/PM-02P_BODY_SEMANTIC_TOPOLOGY_EVIDENCE.md`  
**Evidence oracle:** `work/PM-02P_TOPOLOGY_EVIDENCE_MATRIX.md` v1.0  
**Exact tested source:** `e259fef5849bc4707ddcbdda8e75795b8f3fa5b4`  
**Windows FULL:** #1387 — PASS  
**Merged main:** `a0dad01dd584eb1b1c39a4cb282b0bbc009000aa`

## 1. Result

PM-02P.E closes E20-E25 without introducing a production topology API, persistent topology selector, schema change or face-supported Sketch feature.

The evidence confirms that the semantic topology model proven in A-D remains valid across stage lifecycle, cold reconstruction, stale runtime state, geometry-similarity traps, prospective dynamic Sketch support and bounded dependency-cycle rejection.

## 2. E20 — complete topology at every Feature stage

The kernel-native evidence executes a four-stage sequence:

```text
Add -> Add -> Cut -> Cut
```

Every successful intermediate result is independently inventoried.

For every stage:

- provider Faces equal catalog Faces;
- provider Edges equal catalog Edges;
- provider Vertices equal catalog Vertices;
- every Face receives a provider-neutral Surface classification bucket;
- every Edge receives a provider-neutral Curve classification bucket;
- every Vertex has finite point diagnostics;
- no final-Body-only shortcut is used.

Result:

```text
unaccounted Faces    = 0
unaccounted Edges    = 0
unaccounted Vertices = 0
integrity failure    = 0
```

## 3. E21 — cold reconstruction

Representative evidence is evaluated twice from declared semantic inputs with no retained provider object from the first pass.

The replay set includes:

- multi-stage complete accounting;
- stable Surface lineage;
- split Face / stable Surface;
- deleted and independently recreated Surface;
- periodic seam accounting;
- ambiguous multiple intersection branches;
- same-XYZ Vertex replacement;
- geometry-similarity decoys;
- prospective dynamic Sketch support.

The complete neutral evidence set compares equal across reconstruction.

Result:

```text
cold-rebuild semantic mismatches = 0
runtime token equality required   = NO
```

## 4. E22 — stale runtime-token isolation

The core-only evidence binds runtime Face/Edge/Vertex tokens to:

```text
DocumentId
+ DocumentRevision
+ canonical DocumentSession generation
+ evaluation generation
+ provider generation
+ topology kind
```

The evidence proves rejection after:

- DocumentRevision advance;
- canonical Session A -> Session B replacement;
- evaluation-generation replacement;
- provider teardown/replacement.

A newly issued token may reuse the same numeric token value and remain current while the previous token with the same number remains stale.

Result:

```text
stale runtime acceptances = 0
token-number reuse rebind = 0
```

## 5. E23 — geometry-similarity trap

The final evidence set covers unrelated semantic replacements with equal or near-equal diagnostic geometry:

- equal planar Surface geometry;
- equal cylindrical Surface class and radius;
- equal Line Curve class and Edge length;
- equal Vertex XYZ;
- equal/near-equal provider shape diagnostics.

In every case the old semantic meaning remains Missing or Ambiguous according to provenance. Geometry alone never upgrades a target to Resolved.

A surviving semantic target remains resolvable independently of which geometric decoy is closer.

Result:

```text
geometry-similarity automatic rebind = 0
false Resolved                        = 0
```

## 6. E24 — prospective dynamic Sketch support

The evidence-only authored attachment contains:

```text
semantic planar Surface key
+ local Sketch U/V geometry
```

World geometry is derived from the current resolved carrier frame.

When the stable support Surface moves from one valid Extrude extent to another:

- Surface remains Resolved;
- authored local U/V geometry remains byte-for-byte unchanged;
- resolved carrier frame moves;
- derived world Sketch geometry moves with it;
- no authored absolute SketchPlacement mutation is required.

When the support meaning is deleted:

- status is Missing;
- no stale frame is published as current truth.

When the support meaning is semantically ambiguous:

- status is Ambiguous;
- no stale frame is published as current truth.

This confirms ADR-0016 §12/§15 as a viable production direction.

## 7. E25 — bounded dependency-cycle rejection

The core-only evidence uses current ordered Body Feature history and the existing:

```text
Sketch -> Profile -> Feature
```

consumption relation.

For a Sketch first consumed by Feature N:

- support produced by an earlier Feature is admissible;
- support first produced by Feature N is rejected;
- support first produced by a later Feature is rejected.

The preflight is pure and performs no authored mutation on rejection.

No global dependency graph is required for the accepted PM-02 case.

## 8. Verification history

Two pre-PASS runs exposed evidence-fixture/compile-order defects and did not alter the oracle:

- #1384: compile-order failure because `stageTopologyAccounting()` referenced an evidence helper before declaration;
- #1386: E22 test fixture failed before the semantic check because its temporary project directory had not been created.

Both were corrected without changing E20-E25 expected outcomes.

Windows PR gate #1387 passed on exact source `e259fef5849bc4707ddcbdda8e75795b8f3fa5b4`:

- exact checkout;
- documentation dispatcher;
- complete desktop build graph;
- core-only Release including `pm02p.e_core_lifecycle`;
- kernel-native Release including `pm02p.e_kernel_lifecycle`;
- FAST/SUBSYSTEM selectors;
- complete desktop FULL tests;
- SR-02 latency evidence;
- CI-04 parity/timing checks.

The exact tested content was squash-merged as main `a0dad01dd584eb1b1c39a4cb282b0bbc009000aa`.

## 9. Consequence

E20-E25 are closed.

PM-02P.F may synthesize the accepted evidence into a production architecture recommendation. This does not authorize production PM-02 mutation.
