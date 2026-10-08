# PM-05F R2 D2 — Unified Body Semantic Topology

**Status:** ACCEPTED — Owner 2026-10-08
**Decision class:** D2 bounded Part semantic-topology architecture clarification
**Parent:** `work/PM-05_EDGE_FEATURES.md` and `work/PM-05F_R2_OWNER_ACCEPTANCE_REMEDIATION.md`
**Roadmap:** `work/PART_MODELING_V1_ROADMAP.md` v1.29
**Prior accepted foundations:** ADR-0014, ADR-0016, ADR-0017
**First application:** PM-05F R2-C Revolve material Edge authoring

## Decision

Every successfully evaluated Part Body stage uses **one** provider-neutral, stage-scoped semantic topology system for `Surface / Face`, `Curve / Edge`, and `Point / Vertex`. Extrude, Revolve, Fillet, Chamfer and future operations must supply semantic provenance and provider evidence to that same system, not create competing picking, persistence or identity rules.

An operation produces a new bounded topology stage; no globally persistent provider Edge/Face/Vertex index exists. Semantic carriers are distinct from bounded realizations. Complete accounting includes unsupported topology and representation artifacts, but does not imply all items are authorable.

A durable reference is based on source Feature, semantic Surface/Curve/Point provenance, declared Body stage and, where justified, an explicit branch discriminator. Runtime type information (such as exact analytic Line/Circle) is evidence and may validate classification; it cannot by itself provide identity, nor may screen geometry or provider enumeration select a semantic winner.

## Bounded first implementation in R2-C

Revolve-generated `revolve_side`, `revolve_start_cap`, and `revolve_end_cap` Surfaces are valid inputs to the **same** current Curve stage classifier used by Extrude/Boolean/Edge Features. An analytic material boundary between two uniquely identified semantic Surfaces may receive the existing canonical pair-based Curve relation when:

- the observed current Edge joins exactly two distinct, uniquely resolved semantic Surface carriers;
- both surfaces have valid producer/role provenance in the same declared Body stage;
- same-producer role pairing is valid for the producing operation;
- the provider reports exact `line` or `circle` for this boundary, and there is no conflicting expected-role/analytic-geometry evidence;
- unique cardinality or accepted semantic Point-endpoint discrimination proves the strict Edge branch.

For ordinary Revolve material edges, use existing `cap_side` and `side_side` Curve role categories where they faithfully describe the actual Surface relationship. Preserve the current Extrude-specific analytic checks and all existing durable address types and schema. An unproven or unsupported case stays Unsupported, Missing, or Ambiguous; do not add a new general-purpose enum/schema or classify all provider curves as authorable.

Provider periodic seams, same-Surface representation partitions, degenerate source-role realizations, unsupported spline/other Curve kinds, unknown multi-carrier adjacency and competing claims stay non-authorable. Full-turn and partial Revolve have different cap roles; no imaginary cap or seam can become an authoring candidate.

The same Author / Resolve `MaterialEdgeReference` pipeline must handle both Extrude and Revolve. R2 must prove equivalent ordinary material boundaries with full and partial Revolve, no regression for Extrude, and strict fail-closed behavior through Fillet/Chamfer, Edit, Undo/Redo, Save/Reopen and cold reconstruction.

## Forward compatibility and limits

This decision is **not** authorization to implement Loft, Shell, Sweep, Draft, Pattern, multi-body, a generic new identity framework or arbitrary splines. Those future operations must extend the shared provenance vocabulary with separately evidenced D2 contracts where needed.

The currently restricted semantic Point model (three independent Surface meanings) is recognized as not universal for future multi-valence vertices. Do not broaden durable Point identity now merely to support hypothetical operations; avoid claiming unsupported vertices are Resolved. Preserve inherited/derived Point identity and explicit fail-closed status.

The decision forbids operation-specific surrogate Edge/Point/Surface identity maps in Viewer/UI, geometry-nearest rebinding, first-provider-candidate heuristics, visual epsilon as modeling tolerance, or identity tied to a preview cache.

## R2 acceptance tests

1. RED/GREEN native kernel test: full 360-degree Revolve exposes ordinary authorable circle Edges while the periodic seam stays an artifact.
2. Partial Revolve exposes supported cap/side and side/side material boundaries without authoring provider seams.
3. Extrude and Revolve equivalent cylinder fixtures allow the same ordinary material Fillet/Chamfer inputs; the semantic addresses are different by provenance, not by policy.
4. Durable singular round-trip succeeds; zero/multiple branches stay Missing/Ambiguous, not nearest-match.
5. Edits to dimension/angle, suppressed/deleted predecessors, downstream consumers and cold Save/Reopen preserve existing semantic invariants.
6. Real Qt/OCCT pointer selection remains an independent integration gate; catalog semantics PASS does not imply picker PASS.
7. Existing complete Body accounting and `false-Resolved = 0` regressions remain PASS.

If tests demonstrate that an existing persisted Curve role cannot accurately encode a boundary, **STOP** and propose a separate versioned D2 schema amendment rather than silently inventing new identity.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: this D2 clarifies shared Part topology identity/consumers and R2-C changes user-visible Revolve Edge selection; update as-built/Product PL+EN and generated Browser only after verified production behavior.
