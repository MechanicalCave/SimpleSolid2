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

### D2 design finding: stage-local three-branch disjointness (2026-10-10)

**A naive `Curve + one resolved Point` branch is NOT safe.** The
controlled **real OCCT** 78-Chamfer catalog sweep recorded 1332 Curve/Point
probe occurrences. Of 260 one-Edge incidences, **224** also supported
successful authoring as the **existing v15 `BetweenSemanticPoints`** for
that same native Edge at that same stage, while **36** occurred in the
one-certifiable-endpoint subset. A policy that simply "authors two points
when available, otherwise one" is **not enough**: previously saved
references of both shapes can remain in the document, and the current
`DocumentSession` checks exact structural `MaterialEdgeReference`
equality, not semantic equivalence. Windows deliberate RED for evidence
[#38077150929](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38077150929).
Counts are **probe occurrences**, not distinct EdgeIds or distinct
documents.

**Candidate future stage-local disjoint validity invariant (test-only,
NOT approved for implementation):**

- Existing `SingularAtAuthoredStage`: a unique certified material Edge
  realization of its Curve family, only where the current Curve family
  is singular, as the v15 resolver already requires.
- Existing `BetweenSemanticPoints`: preserve current v15 strict
  meaning and both independently resolved endpoint semantics; do not
  change their stored addresses or back-rewrite older links.
- Hypothetical `AtSingleSemanticPoint`: require the **exact referenced
  BodyStageRef** (never final-Body fallback), a complete strict current
  catalog, a Curve with **at least two** current real material Edge
  realizations, **exactly two incident native Vertices** for its candidate
  material Edge and **exactly one uniquely resolved semantic Point
  endpoint across both**. That endpoint must be the encoded
  `FeaturePointAddress` and have exact certified material incidence.
  The set of Edge realizations in this Curve family incident to this
  Point must be **exactly one**. A second certified endpoint appearing
  after an upstream edit makes this old one-point reference
  **inapplicable/fail closed**, even if that geometry appears visually
  similar. The fallback MUST NOT silently produce a new v15
  `BetweenSemanticPoints` reference or rebind a stored EntityId.

This check was implemented **only as a research helper** in
`pm02jr2_add_surface_continuation_test`; in the controlled OCCT
matrix, all 224 structurally dual-encodable point/Curve occurrences
were rejected by the proposed one-point domain, and all 36
one-certified-endpoint occurrences remained uniquely admitted. The
wrong source Stage was also rejected. Native Windows
[FOCUSED #38077430304](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38077430304)
**PASS 1/1** on `1aaaa13e2d07f7c852ae1d619969a1d85e6f1b40`.

**Research limits / STOP:** this is bounded disjointness of the
verified current catalog; it does not prove all prior v15 Point and
Curve relations remain globally coherent after arbitrary upstream
editing or that the *same semantic fragment* always survives a
single-candidate result. It does not close real native ambiguous
incidence, producer cycles, full v16 migration, old/new mixed
persisted links, or Owner private Face retest. Production code,
`EdgeBranchDiscriminator`, the parser and v15 remain unchanged.

**Predecessor suppression:** separately in the same controlled native
study, the immediately preceding Add Feature was successfully
suppressed and Undo restored the prior single-candidate Curve/Point
selection. The test does not certify current post-suppression source
resolution or link migration; further stage-aware fail-closed
suppression/deletion evidence is still needed. This must not be reported
as a completed E4 suppression proof.

**Proposed authoring order:** existing `SingularAtAuthoredStage` when exactly one Curve realization; current `BetweenSemanticPoints` when two certified distinct endpoints and round-trip exactness; only then consider `AtSingleSemanticPoint` if one certified endpoint uniquely selects the requested material Edge and an exact same-stage resolve round-trip proves it. For all other cases refuse. This order still needs design evidence about source **canonicalization**, equality, dedup and possible alternate encodings for the same current material Edge: current PG-01D deduplicates by exact `MaterialEdgeReference`, not geometry. No new authoring until this ambiguity is resolved.

## 3. Gates before a production D2 decision

| Gate | Required proof | Status |
|---|---|---|
| E0 — 3-Surface Point audit | Same consumed OCCT result, native vs semantic Surface cardinality and inherited Vertex provenance | **PASS research** 36/36 new two-Surface unsupported endpoint |
| E1 — same-stage selector uniqueness | 0/1/many returns Missing/Resolved/Ambiguous; never first-wins | **PASS limited synthetic tests** 36/36 positive; negative injected two-Edge incidence and missing Point both fail closed; actual OCCT-native collision remains OPEN |
| E2 — native provider rebuild | Same semantic selector in independent restored Part and fresh OCCT provider, without native token reuse | **PASS research** 36/36 |
| E3 — native on-disk Save/Reopen | Actually write/read .ss2part v15, new DocumentSession and provider, compare strict stage resolutions | **PASS existing-address round-trip** real PartDocumentStore createNew/load of v15 plus new OCCT provider, 1/1; new one-Point persisted branch remains NOT IMPLEMENTED |
| E4 — Undo/Redo and upstream geometry mutation | Author/cold recreate, undo/redo, change or suppress upstream source, verify no stale identity or accidental rebind | **PARTIAL** native Chamfer Undo/Redo and preceding Add extent edit/Undo/Redo 1/1. Predecessor suppression committed and Undo restored 1/1; post-suppression source status and deletion, collapse, relocation/retarget and provenance still OPEN |
| E5 — adverse native geometry | Same Curve + Point incident to multiple actual material Edge fragments, negative stage/cycle cases and equal-looking unrelated geometry | **PARTIAL** injected two-Edge incidence and missing Point fail closed. Native 78-Chamfer survey: 1332 Curve+Point pairs, 260 uniquely selected Edge hits, 0 collisions and 1072 empty. A real OCCT-native ambiguity counterexample, stage/cycles and similar geometry remain OPEN |
| E6 — schema/migration/canonical identity | Explicit layout/version plan, v15 backwards reading, corrupt/malformed/duplicate rejection, mixed old/new link resolution and cross-branch dedup | **PARTIAL DESIGN ONLY** native evidence 224 legacy two-Point aliases, 36 one-Point-only cases, tested disjoint domain by endpoint cardinality and stage. Actual v16 serialization/migration, mixed persistence and global semantics OPEN — separate Owner D2 before mutation |
| E7 — Owner sample + practical result | Typed failure/recovery on private Owner Face, correct manually selected boundaries, then linked cold Save/Reopen and final FULL | **OPEN — Owner FAIL** |

No gate may be marked PASS based on a deliberate RED used solely to surface CTest stdout. Every accepted proof needs a separate final GREEN on its exact test SHA; no exact-head FULL claimed.

**Further bounded evidence** — actual preceding `Extrude Add` edit (extent 10 to 11 mm) succeeded in an isolated three-Feature Part with a later Chamfer, and the same current Curve+Point selector had exactly one candidate after edit, after Undo and after Redo. A separate exhaustive scan of the controlled real OCCT Chamfer matrix found **1332 actual Curve+Point probes** across multi-piece Curve families: **260 one-Edge matches, 1072 empty and 0 multi-Edge matches**. Full counters were collected using a deliberately RED-for-log test [#38076507933](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38076507933); its forced failure was removed. This confirms no native ambiguity *in the scanned fixtures* but does not discharge E5's need for a real negative B-Rep, nor distinguish geometric retarget of a uniquely matching but changed semantic edge. Exact-head GREEN still required.

**Verified native lifecycle research:** Windows kernel [FOCUSED #38075613345](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38075613345) **PASS 1/1** at test SHA `a81f00fd23e991ef8320c13e5cf09ad3ce5819a8`. It rebuilt and ran `pm02jr2.add_surface_continuation`, covering the 36/36 Curve+Point witness plus a synthetic duplicate-incidence refusal, a missing-Point refusal, real on-disk v15 `PartDocumentStore::createNew/load` and fresh OCCT evaluation, and native Chamfer Undo/Redo. The collision fixture deliberately changes an in-memory ledger and is NOT evidence that OCCT produces that topology organically. The v15 file contains only existing authored model semantics; it does **not** serialize the unapproved third discriminator.


## 4. Potential conflicts and mandatory decisions

- **File v15 compatibility:** the existing `EdgeBranchDiscriminator` is a two-variant sum (`SingularAtAuthoredStage`, `BetweenSemanticPoints`). A third persisted branch is not covered by v15; **do not silently extend v15 in place**. Proposed baseline is a future v16 additive discriminant plus strict read migration of valid v15, preserving original branch meaning. The final schema number and migration must be approved separately by Owner D2.
- **No implicit two-Surface Points:** creating a broad `FeaturePointAddress` from two Surfaces is an alternative architecture change; it is not approved merely because 36 current observations were locally unique.
- **Alias/dedup ambiguity — quantified and bounded, not cleared for production:** `DocumentSession::execute(CreateProjectedSketchEdgesCommand)` rejects duplicates by a `std::set<MaterialEdgeReference>` and compares existing bindings by **structural equality only**. Two different discriminant branches can thus refer to one current Edge while evading duplicate-source validation. Canonical authoring preference alone is insufficient: a previously authored one-Point link may survive an upstream edit that enables two-point authoring for the same Curve fragment. Design must prove a version-stable semantic equivalence/canonicalization rule or a disjoint-domain invariant, including strict behavior for old persisted links, atomically validated command dedup, and feature-generated Surface lineage that embeds MaterialEdgeReferences. Do not substitute geometry/tokens, rewrite stored identity opportunistically or silently duplicate target entities. STOP until explicitly decided by Owner D2.
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
