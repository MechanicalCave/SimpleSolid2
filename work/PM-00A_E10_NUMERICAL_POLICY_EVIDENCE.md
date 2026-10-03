# PM-00A E10 — Numerical / Refine / Healing Evidence

**Status:** COMPLETED — PASS  
**Exact source candidate:** `ac34a713c6fee4a53d513bfcce2a2044ce1a0ffb`  
**Windows FULL:** #1320 — PASS  
**Core-only:** 19/19 PASS  
**Kernel-native aggregate:** 27/27 PASS  
**Desktop FULL:** 84/84 PASS  
**False-Resolved:** 0  
**Test:** `pm00a.e10_numerical_policy`

## Goal

Prove the frozen E10 matrix without introducing a product tolerance setting, hidden healing policy, persistent modeling-semantics schema or Viewer-dependent modeling path.

The probe is kernel-native and evidence-only. It adds no production source.

## Result matrix

| ID | Evidence | Actual result |
| --- | --- | --- |
| E10-01 | same valid Profile/Extrude under presentation-independent evaluation | PASS — repeated neutral/kernel-native evidence is identical; camera/projection is not an input to the boundary |
| E10-02 | pick aperture / OSNAP display independence | PASS — pick/display state has no dependency path into neutral Kernel or kernel-native provider evidence |
| E10-03 | refine/unify OFF vs explicit candidate ON | PASS — provider topology changes in the adjacent-box Boolean fixture while the independently source-defined exterior face remains exactly one semantic candidate under both policies |
| E10-04 | clearly separated ordinary valid geometry | PASS — rectangle + circular hole extrudes to one valid solid with resolved cap/side meanings and explicit fuzzy value 0 |
| E10-05 | clearly invalid gap | PASS — a 1 mm open loop fails deterministically; no increasing fuzzy tolerance, retry-until-success or silent healing occurs |
| E10-06 | near-threshold family | PASS — gaps scaled by `Precision::Confusion()` over multipliers 0, 0.01, 0.1, 0.5, 1, 2, 10, 100, 10000 and 10000000 are deterministic, contain accepted and rejected samples and expose at least one transition |
| E10-07 | changed policy candidate | PASS — raw/refined policy tags are different and the same authored geometric fixture produces different provider topology, so a policy change cannot remain silent under one claimed semantics version |

## E10-01 / E10-02 boundary proof

The neutral Profile input and provider evidence API contain authored geometric/semantic data only. They have no Viewer, camera, projection, screen-space pick aperture, OSNAP display state, Qt object or presentation-token parameter.

The existing `pm00a.kernel_boundaries` regression prevents neutral Kernel → Viewer/UI/provider leakage. The E10 test evaluates the same clean Profile → Extrude twice and requires exact neutral evidence equality.

This is stronger than merely setting two camera values in a provider test: the modeling layer has no legal path by which those values can enter.

## E10-03 explicit refine policy

The probe fuses two adjacent boxes using explicit fuzzy value 0 and compares:

- raw/no-refine policy;
- explicit `SimplifyResult(true, true)` candidate.

Both results must be valid one-solid B-Reps. The refined fixture must have fewer faces than the raw fixture, proving a real topology-policy difference.

A known exterior source face is used only to construct the deterministic test fixture. Provider Modified/unchanged evidence is then reduced to semantic candidate cardinality; both policies require exactly one surviving candidate. Test-fixture centroid lookup is not proposed as durable identity.

## E10-04 / E10-05 no fuzzy escalation

The ordinary clean rectangle-with-hole Profile succeeds through the existing exact Profile → Kernel boundary.

A structurally representable but geometrically open loop with a 1 mm closing gap fails provider geometry deterministically. The probe does not retry with larger fuzzy values and does not heal the gap.

This supports the v1 direction that invalid authored geometry should fail explicitly rather than be made successful by progressively relaxing a hidden tolerance.

## E10-06 threshold evidence

The probe reads OCCT `Precision::Confusion()` at runtime solely to define a provider-relative measurement scale.

The tested multipliers span exact closure through many orders of magnitude above that provider precision. Each point is evaluated twice and must return identical evidence. The accepted run proves:

- at least one accepted sample;
- at least one rejected sample;
- at least one acceptance transition;
- no non-deterministic point in the sampled family.

The CI uses non-verbose CTest on successful tests, so the executable's diagnostic stdout is not promoted to a repository constant. This is intentional: PM-00A does not convert the provider's precision value or the observed transition into an SS2 modeling tolerance.

## E10-07 policy/version implication

The same geometric fixture is evaluated under two explicitly named test-local policies:

- `pm00a-e10/raw-no-refine/fuzzy0`;
- `pm00a-e10/refine-v1/fuzzy0`.

They produce different provider topology while preserving the bounded semantic source-face outcome.

Therefore a future production change to refine/unify/fuzzy/healing behavior can change evaluated topology and must be represented by an explicit modeling-semantics policy/version rather than occur silently under unchanged semantics.

## O-11 recommendation evidence

E10 supports the following recommendation for Owner review in PM-00B:

1. screen/display/pick tolerances are never modeling tolerances;
2. v1 performs no iterative fuzzy escalation and no silent gap healing;
3. provider precision remains an implementation diagnostic/floor, not persisted design intent;
4. refine/unify policy is explicit per modeling-semantics version;
5. a policy change that can alter topology/reference outcomes requires a modeling-semantics version change or explicit migration/re-evaluation decision;
6. exact numeric user/model tolerance values are not inferred from OCCT defaults merely because a provider accepts near-threshold geometry.

See `work/PM-00A_ARCHITECTURE_RECOMMENDATIONS.md` for the complete proposed O-11 resolution.

## Bounded failed attempt

Windows FULL #1319 on source candidate `08093b1b76e80f3a838b46111ad56d80989af640` failed only while compiling the new E10 test because OCCT 8 does not provide the legacy standalone `TopTools_ListIteratorOfListOfShape.hxx` header.

The bounded fix changed the evidence test to range iteration over the provider list. No frozen E10 expectation and no production code changed. Exact source candidate `ac34a713c6fee4a53d513bfcce2a2044ce1a0ffb` then passed FULL #1320.

## Scope boundary

E10 introduced no:

- product tolerance setting;
- automatic healing;
- production refine/fuzzy API;
- persistent `modelingSemanticsVersion`;
- Body/Feature schema;
- topology-reference schema;
- UI/Viewer behavior;
- durable provider identity.

PM-00B owns the architecture freeze.
