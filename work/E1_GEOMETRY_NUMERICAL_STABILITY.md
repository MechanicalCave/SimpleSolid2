# E1 — Geometry Numerical Stability

**Status:** ACCEPTED — ACTIVE  
**Proposed:** 2026-09-28  
**Owner acceptance:** 2026-09-28  
**Decision class:** D1 numerical implementation within existing Sketch semantics; stop for Owner D2/D3 amendment if tolerance/coordinate/product validity semantics must change  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package E1  
**Baseline:** `main` at `177378634d85395b507a93cd81b4d66c60efaab4` after completed Package D

## 1. Goal

Reproduce and, if confirmed, remove translation/scale sensitivity from the current 3-Point Arc geometric construction before region/profile semantics can depend on it.

E1 is numerical hardening of the existing authored Arc meaning. It does not introduce a geometric constraint solver, a new user tolerance, coordinate limits, units policy or region/profile behavior.

## 2. Confirmed baseline code finding

The current private `arcThroughThreePoints()` implementation in `src/sketch/interaction_state.cpp` computes its circumcenter directly in absolute Sketch coordinates.

Its determinant is formed from terms such as:

```text
x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)
```

and its center numerator uses absolute squared norms:

```text
x*x + y*y
```

This formulation is mathematically translation-invariant but floating-point evaluation is not. Large common translations can introduce cancellation while the squared absolute coordinates grow independently of the local Arc size.

The current Arc interaction tests prove ordinary origin-scale short/long CW/CCW cases and exact duplicate/collinear rejection, but do not prove equivalent local geometry after large translation, very small/large scale, or residual accuracy through all three requested points.

This audit code finding is **CONFIRMED**. Whether the mandatory translated case visibly fails on the supported MSVC runtime remains to be classified by Phase A evidence.

## 3. Existing semantic authority to preserve

The existing SK-06A Arc contract remains authoritative:

- Arc authored state is center + radius + start angle + signed sweep angle;
- initial Arc creation is Start → Through → End;
- the Through point selects which directed Start→End circular path is authored;
- any two exactly equal accepted points are invalid;
- exactly collinear points / no finite circumcircle fail closed;
- full-circle output is not an Arc;
- successful commit creates the same single semantic Arc command/history effect as today;
- preview remains transient;
- Viewer/OCCT tessellation is not semantic geometry.

E1 must not redefine those rules.

## 4. Scope IN

- actual C++/MSVC reproduction of 3-Point Arc translation/scale sensitivity;
- a focused neutral semantic test driven through `SketchInteractionState` Arc creation/preview;
- mandatory radius 0.1 case at origin and translated by (+1,000,000, +1,000,000);
- comparison of equivalent local figures before/after translation;
- residual verification that the reconstructed circle passes through Start, Through and End to bounded floating-point error;
- small-scale, large-scale, near-collinear and overflow/underflow-oriented characterization;
- if the finding is confirmed, replace the absolute-coordinate circumcenter evaluation with a translation-local and, where required, scale-normalized computation;
- preserve CW/CCW and short/long branch selection;
- deterministic fail-closed handling for non-finite/unrepresentable intermediate results;
- internal numerical documentation and generated Browser;
- normal core-only and desktop Windows verification.

## 5. Scope OUT

- any new UI or command;
- any change to Circle creation;
- constraints, coincident relations or a solver;
- snapping/inference;
- profile/region semantics;
- OCCT geometry as semantic authority;
- persistence/schema change;
- EntityId/history changes;
- changing Arc canonical authored representation;
- arbitrary epsilon snapping of user points;
- new Product coordinate limits;
- new units policy;
- changing global Sketch tolerance;
- rejecting previously legal finite Arc constructions solely because a new fixed tolerance says they are “too small”, “too large” or “too close”;
- rewriting unrelated transform algorithms without an E1 reproduction.

## 6. Phase A — supported-toolchain reproduction first

After Owner acceptance, the first runtime mutation is a **characterization/reproducer test**, not a production fix.

The test must run in the normal supported Windows/MSVC gate and drive the public semantic Arc interaction path.

The mandatory canonical case is a quarter-circle with:

```text
radius = 0.1
center = (0, 0)
Start   = (0.1, 0)
Through = (0.1 / sqrt(2), 0.1 / sqrt(2))
End     = (0, 0.1)
```

and the geometrically equivalent translated case:

```text
translation = (1,000,000, 1,000,000)
```

The test compares the translated result after removing the common translation to the origin result.

Phase A must record:

- whether both constructions are accepted;
- computed center;
- radius;
- signed sweep;
- radial residual for Start, Through and End;
- translation-equivalence error.

A deliberately failing intermediate PR gate is acceptable and should be retained as evidence if it demonstrates the defect. It is not a merge candidate.

## 7. Finding classification after Phase A

The mandatory finding must be classified as one of:

**CONFIRMED**  
The supported C++/MSVC baseline rejects an otherwise representable equivalent construction or produces materially worse center/radius/path residual after translation.

**NOT REPRODUCED**  
The mandatory case and the accepted characterization matrix remain equivalent within the test's numerical error bound.

If NOT REPRODUCED, do not change production code merely because the formula looks suspicious. Record evidence and close or amend E1 based on the broader matrix.

If CONFIRMED, the bounded numerical fix in Sections 8–12 is authorized without another Owner gate.

## 8. Bounded implementation if confirmed

The intended implementation direction is a private numerical correction, not a public API change.

Compute the circumcircle from **local point differences** rather than absolute coordinate squares.

Conceptually:

```text
origin = Start
a = Through - Start
b = End - Start
```

The circumcenter offset is solved in that local frame, then translated back to Sketch coordinates.

Where the local magnitudes would otherwise overflow/underflow or materially degrade conditioning, normalize the local vectors by a common finite scale before forming squared norms and determinant, then restore the physical scale to the center offset.

Exact implementation details remain D1 as long as Sections 3, 9 and 10 are preserved.

Do not use `long double` as the sole fix: on the supported MSVC ABI it must not be assumed to provide wider floating-point precision than `double`.

## 9. No new Product tolerance in E1

E1 distinguishes **algorithmic conditioning** from **Product tolerance**.

Runtime validity remains based on the existing semantic contract:

- finite representable input;
- pairwise non-equal points;
- a non-degenerate representable circumcircle;
- finite positive radius;
- valid non-zero signed sweep below one full turn.

E1 does not introduce a fixed world-space epsilon that changes whether points are considered coincident or collinear.

If completing E1 would require:

- a world-unit tolerance;
- a relative geometric tolerance that becomes Product semantics;
- a maximum coordinate/radius limit;
- a minimum feature size;
- rejection of a class of inputs the current contract accepts;

stop and present that policy to the Owner before production mutation.

## 10. Residual evidence

Finite output alone is insufficient.

For every accepted test construction, verify all three requested points against the returned circle.

For point `p`, center `c` and radius `r`, record/test a radial residual conceptually equivalent to:

```text
abs(hypot(p - c) - r)
```

The test bound must be numerical/scale-aware and justified from floating-point representation; it must not silently become a Product geometric tolerance.

Also verify:

- Start lies at the authored start direction;
- Through lies on the selected signed sweep path;
- End lies at the authored end direction;
- translated and untranslated equivalent inputs select the same CW/CCW and short/long branch.

## 11. Characterization matrix

At minimum include:

1. required radius 0.1 at origin;
2. same radius 0.1 translated by (+1e6,+1e6);
3. equivalent negative large translation;
4. ordinary unit-scale short CCW case;
5. ordinary unit-scale short CW case;
6. long-arc branch case;
7. a small but representable local radius;
8. a large finite local radius;
9. a near-collinear but non-collinear representable triangle;
10. exact collinear points;
11. duplicate points;
12. non-finite input;
13. a scale chosen to exercise the old squared-coordinate overflow/underflow risk while the local normalized construction remains representable.

Matrix values must be chosen from what the supported double representation can actually distinguish. E1 does not promise geometry below the resolution of the input coordinates themselves.

## 12. Deterministic extreme behavior

The implementation must avoid undefined or accidental behavior from intermediate overflow/underflow.

For exact collinearity, duplicate input or a non-finite/unrepresentable construction:

- return the existing degenerate/failure semantic outcome;
- create no authored Arc;
- do not increment revision;
- create no Undo entry;
- do not allocate durable identity.

For near-collinear but finite representable geometry, do not add an arbitrary fixed cutoff. Either produce a finite construction satisfying the evidence bound or stop for an explicit validity-policy decision.

## 13. Other geometry helpers

E1 may add characterization tests for existing difference-based helpers such as Direct Distance, Rotate/Scale/Mirror only to prove that the audit risk is localized.

Do not rewrite them unless an actual supported-toolchain reproduction demonstrates a numerical defect within E1's stated geometry-stability goal.

Any broader transform redesign is out of scope.

## 14. Expected implementation surface

Expected bounded surface:

- `src/sketch/interaction_state.cpp`;
- `tests/sk06a_circle_arc_interaction_state_test.cpp` and/or a focused new `tests/e1_arc_numerical_stability_test.cpp`;
- `tests/CMakeLists.txt` and core-only registration if a new test target is used;
- `docs/internal/` numerical/as-built documentation;
- generated `docs/browser/index.html`;
- E1 lifecycle records under `work/`.

No Application, Part persistence, UI, Viewer provider or OCCT production change is expected.

If those production areas become necessary, stop for scope review.

## 15. Verification

E1 completion requires:

- Phase A supported-MSVC reproduction/classification recorded;
- mandatory radius-0.1 / +1e6 translation evidence;
- origin/translation equivalence tests;
- residual-through-all-three-points tests;
- exact collinear/duplicate/non-finite fail-closed tests;
- small/large/near-collinear/extreme-scale characterization;
- existing SK-06A Arc branch tests unchanged and green;
- core-only semantic suite green;
- normal desktop Windows FULL green;
- documentation validation and Browser freshness green;
- exact-head final Windows FULL on the final runtime candidate;
- work-only CLOSURE;
- merge to main.

Manual GUI verification is **not required** if E1 remains purely numerical and preserves visible Arc semantics. Any visible interaction/acceptance change reintroduces an Owner manual verification gate.

## Documentation impact

Internal docs: required  
User/Product docs: not required

Reason: E1 documents and hardens the numerical implementation of existing Arc semantics without changing the user-visible Arc workflow or accepted command language. If E1 introduces coordinate/tolerance/validity Product policy, Product docs become required and Owner re-approval is required first.

## 17. Stop conditions

Stop for Owner review before production mutation if evidence requires:

- a Product tolerance policy;
- coordinate/radius/feature-size limits;
- a change to exact duplicate/collinear semantics;
- changed Arc Start/Through/End interpretation;
- changing signed sweep branch semantics;
- a persistence/schema change;
- public API or subsystem ownership expansion;
- third-party geometry/numerics dependency;
- OCCT as semantic construction authority;
- broader transform rewrite without reproduction.

## 18. Activation gate

This proposal does **not** activate E1.

Activation requires explicit Owner acceptance of this Work Contract.

After acceptance:

1. `work/ACTIVE.yaml` switches from completed D to active E1;
2. Phase A characterization/reproducer test is added first;
3. the finding is classified from actual Windows/MSVC evidence;
4. only if CONFIRMED may the bounded private numerical fix proceed automatically;
5. any stop condition requires a new Owner decision.

## 19. Completion boundary

After E1 completion, AUDIT-01 schedules E2 next.

E1 completion does not activate E2, F, profile semantics or solid modeling.
