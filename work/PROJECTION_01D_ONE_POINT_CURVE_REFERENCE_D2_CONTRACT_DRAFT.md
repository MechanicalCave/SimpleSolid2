# PG-01D — Curve plus one Semantic Point: bounded D2 design contract (DRAFT)

**Status: DESIGN DRAFT — NOT ACCEPTED FOR PRODUCTION MUTATION.**
Owner authorized continuation of the bounded D2 study on 2026-10-10. This document does **not** activate a separate phase, approve a new identity branch or authorize a file-schema change. Active work remains PG-01D on `work/ACTIVE.yaml` and its accepted amendment. No work on `main`, PG-01E, PM-06, independent CI #302 or tolerance/healing.

## 1. Problem and evidence

A *real material Edge* can belong to a stable semantic Curve family with two current bounded Edge realizations but only one fully resolved semantic Point endpoint. The second current OCCT Vertex has two resolved semantic Surfaces, no inherited Vertex lineage claim, and is unsupported by the existing precisely-three-Surface `PointRelation`. The current v15 `MaterialEdgeReference::BetweenSemanticPoints` cannot author it: Face Boundary correctly fails closed.

Independent real OCCT two-Add/Chamfer matrix: 24 authorable source Edges x four distances, 78 successful resulting Chamfer stages, 900 bounded-Face admissions and 36 material-identity refusals. The 36 refused source occurrences have exactly two Curve realizations and exactly one certified Semantic Point. On the same 36 occurrences, (a) a pair of resolved Surfaces happened to have one current Vertex; (b) the *existing* `FeatureCurveAddress` + *existing resolved* `FeaturePointAddress` yielded one **exact** material Edge; (c) the latter selector remained unique after independent Part restore and new OCCT provider, **without** carrying runtime tokens across evaluations. No user private geometry enters source control.

These are feasibility tests, not a production identity proof. Owner Face token 5 remains privately unverified against this specific failure mode, and Owner practical PG-01D FAIL is not waived.

## 2. Minimal possible future selector — UNAPPROVED

Candidate durable semantic shape (conceptual, **do not implement yet**):

```cpp
struct AtSingleSemanticPoint {
    FeaturePointAddress point;
};
MaterialEdgeReference {
    BodyStageRef stage;
    FeatureCurveAddress curve;
    // current v15 variants plus a *future* explicit third branch
    EdgeBranchDiscriminator branch;
};
```

The discriminating *meaning* is: in the reference's exact earlier Body stage, among **all current material Edge realizations** of its semantically resolved Curve family, return the one and only Edge incident to the exact uniquely resolved semantic Point. The native Vertex/Edge is current-stage evidence only; never persisted. No assumption of current runtime topology ordering, Face membership, XYZ, nearest matching, line/circle arc coordinate, representation partition or direction.

Proposed *read-only resolver* proof obligations before `Resolved`:

1. Source Stage and Feature dependency order are valid; the Curve and Point producers precede or equal the referenced stage, with strict cycle prohibition. No final-Body global fallback.
2. The stage catalog is complete and fresh, the Curve address is valid, its current family is accounted and each candidate Edge is explicitly real material, with exactly one semantic Curve candidate. Exclude provider-certified periodic seams and representation partitions.
3. The Point address resolves to **exactly one** referenceable current semantic Vertex, with the Point relation still certified and current. A missing, unsupported, multi-realization, aliased or noncertified Point cannot be used as a discriminator.
4. The unique Point Vertex's current *material Edge incidence* intersects the Curve's vetted current Edge set in **exactly one** member: 0 => Missing, 1 => Resolved, 2+ => Ambiguous. Never choose the first candidate; any catalog/provider integrity mismatch is fail closed.
5. All referring consumers preserve existing command atomicity, exact-stage freshness, Undo/Redo and no last-good fallback. A change in Face-wire membership does not silently add new Sketch EntityIds; a new link remains one EntityId per exact source.

**Proposed authoring order:** existing `SingularAtAuthoredStage` when exactly one Curve realization; current `BetweenSemanticPoints` when two certified distinct endpoints and round-trip exactness; only then consider `AtSingleSemanticPoint` if one certified endpoint uniquely selects the requested material Edge and an exact same-stage resolve round-trip proves it. For all other cases refuse. This order still needs design evidence about source **canonicalization**, equality, dedup and possible alternate encodings for the same current material Edge: current PG-01D deduplicates by exact `MaterialEdgeReference`, not geometry. No new authoring until this ambiguity is resolved.

## 3. Gates before a production D2 decision

| Gate | Required proof | Status |
|---|---|---|
| E0 — 3-Surface Point audit | Same consumed OCCT result, native vs semantic Surface cardinality and inherited Vertex provenance | **PASS research** 36/36 new two-Surface unsupported endpoint |
| E1 — same-stage selector uniqueness | 0/1/many returns Missing/Resolved/Ambiguous; never first-wins | **Partial** 36/36 positive; synthetic collision and missing controls being added |
| E2 — native provider rebuild | Same semantic selector in independent restored Part and fresh OCCT provider, without native token reuse | **PASS research** 36/36 |
| E3 — native on-disk Save/Reopen | Actually write/read .ss2part v15, new DocumentSession and provider, compare strict stage resolutions | **OPEN** until Windows test PASS |
| E4 — Undo/Redo and upstream geometry mutation | Author/cold recreate, undo/redo, change or suppress upstream source, verify no stale identity or accidental rebind | **OPEN** |
| E5 — adverse native geometry | Same Curve + Point incident to multiple actual material Edge fragments, negative stage/cycle cases and equal-looking unrelated geometry | **OPEN**; fabricated incidence test is not proof of an actual OCCT counterexample |
| E6 — schema/migration/canonical identity | Explicit layout/version plan, v15 backwards reading, corrupt/malformed/duplicate rejection, mixed old/new link resolution and cross-branch dedup | **OPEN — Owner D2 required before mutation** |
| E7 — Owner sample + practical result | Typed failure/recovery on private Owner Face, correct manually selected boundaries, then linked cold Save/Reopen and final FULL | **OPEN — Owner FAIL** |

No gate may be marked PASS based on a deliberate RED used solely to surface CTest stdout. Every accepted proof needs a separate final GREEN on its exact test SHA; no exact-head FULL claimed.

## 4. Potential conflicts and mandatory decisions

- **File v15 compatibility:** the existing `EdgeBranchDiscriminator` is a two-variant sum (`SingularAtAuthoredStage`, `BetweenSemanticPoints`). A third persisted branch is not covered by v15; **do not silently extend v15 in place**. Proposed baseline is a future v16 additive discriminant plus strict read migration of valid v15, preserving original branch meaning. The final schema number and migration must be approved separately by Owner D2.
- **No implicit two-Surface Points:** creating a broad `FeaturePointAddress` from two Surfaces is an alternative architecture change; it is not approved merely because 36 current observations were locally unique.
- **Alias/dedup ambiguity:** two selectors might identify the same live Edge without structural equality. Existing PG-01D semantics retain real material boundaries and dedup **only by exact reference equality**. Design must prove canonical authored branch choice and deterministic dedup across stages, edits, mixed persisted versions and downstream Feature source provenance, or STOP.
- **Lifecycle/retarget:** changing upstream features may change a Curve family from 2 fragments to 1 or >2. A previously authored one-point selector must either uniquely resolve the same proven semantic source or fail closed; never silently adopt a different candidate.
- **Product selection:** `Face Boundary` manually expands individually picked bounded Faces and retains all true shared material Edges, per accepted Owner amendment. It does not mean "exactly three edges" if a selected Face genuinely has four material boundaries; arbitrary subset selection remains the existing **Edges** mode.

## 5. Narrow work sequence

1. Complete the *test-only* ambiguity, missing-source, on-disk persistence, Undo/Redo and upstream evidence on the PG-01D Draft branch; label synthetic controls honestly.
2. Review canonical equality, source identity and producer/stage cycles (including generated Chamfer Surface recursively referencing existing MaterialEdgeReferences).
3. Draft a technical D2 delta against accepted ADR-0014/0016, with proposed versioned serializer/parser fixtures, old v15 reader guarantees and exact migration tests.
4. Seek an **explicit new Owner D2 approval of the specific durable selector + schema + alias policy** before modifying `semantic_topology_reference.hpp`, production `feature_evaluation.cpp`, `PartDocumentStore`, native schema or viewer.
5. Only after approval implement RED→GREEN focused Part/Kernel, real cold file and negative tests, docs PL/EN plus generated Browser, Windows FAST/FULL and Owner practical PASS.

## Documentation Impact

This is an internal `work/` contract **draft** and no shipped behavior. The canonical as-built docs and generated Browser are unchanged; any future implementation must regenerate Browser with `.\\ss2.ps1 docs` and pass `ss2 verify`, not edit Browser by hand.
