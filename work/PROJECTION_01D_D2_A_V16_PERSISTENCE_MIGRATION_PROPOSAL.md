# PG-01D D2-A — proposed versioned one-Point material Edge reference (v16) and migration

**STATUS: PROPOSED — NOT OWNER-APPROVED FOR PRODUCTION / SCHEMA MUTATION.**
On 2026-10-10 the Owner explicitly approved **semantic associativity option A** for a future bounded Curve segment: stable semantic Curve + one already certified semantic Point at the exact earlier Body stage, with geometrical deformation across upstream edits allowed. This is **not** an approval of a third persisted branch, the v16 version choice, migration behavior, implementation, release, or merge. The governing active Work Contract remains PG-01D and its later manual Face Boundary amendment. PR #309 stays Draft.

**STATUS UPDATE — OWNER D2-V16 APPROVED 2026-10-10, WITH NO EXISTING-FILE MIGRATION.** Owner's statement in the active conversation: `Zatwierdzam - migracja plików na tym etapie produkcji nie jest konieczna.` The third persisted branch and schema v16 are now authorized **only** within the added D2-V16 section of `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md`, with accepted semantic associativity A and projection-only authoring. Do not build any bulk migrator, conversion job, or retroactive rewrite. Keep historical v1-v15 reads unchanged, reject new kinds under old version numbers, and permit new format on explicit Save. Existing textual labels `proposed` below preserve the proposal history; where conflicting, this status and the Work Contract's later amendment govern. **No Owner FINAL PASS or merge authorization.**

## D2-V16 production candidate checkpoint — 2026-10-10 (NOT accepted)

Implemented on the active PG-01D feature branch, not merged: `AtSingleSemanticPoint` provider-neutral reference and strict multi-Curve/one-certified-Point resolver; a projection-only authoring fallback (general Fillet/Chamfer authoring remains legacy); version-aware recursive v16 read/write with v1–v15 read compatibility and no bulk conversion; opt-in PROJECT-only controller picks; real-OCCT 36-case regression extension, native cold link Save/Reopen, and v15 wrong-version/nested generated-Surface rejection negatives. Candidate source through `c5cc08a372e5ef37d28bfaad671330a8a3e03bcd`. 

**Evidence limitation:** these changes are committed but were **not yet certified** by exact-candidate kernel-native FOCUSED or full Windows test runs at the time of this checkpoint. The earlier FAST attempts were superseded by later implementation commits; no prior CI PASS can be transposed to this candidate. A focused test request accompanies this report-only commit. Product PL/EN and generated Browser remain to be synchronized under documentation governance, exact-head FAST/FULL and Owner real-model practical PG-01D FINAL PASS remain required. This checkpoint neither authorizes merge nor reactivates PM-06/PG-01E.

## 1. Accepted identity meaning, distinct from B-Rep lineage

A *future* `AtSingleSemanticPoint` reference would mean the authored tuple:

```text
(BodyStageRef{after_feature, source FeatureId},
 FeatureCurveAddress,
 AtSingleSemanticPoint{FeaturePointAddress})
```

No geometric points, B-Rep tokens, face-wire/member indices, curve parameter, direction, nearest matching or previous-process cache may be serialized. Upstream re-evaluation may change the geometry and native Edge token; same durable stage + Curve + Point addresses define the **associative semantic intent**, rather than a demand for physically identical bounded-Edge B-Rep lineage across revisions. The branch resolves only when all the following are current and strictly certified:

- Exact earlier source stage is present, active, complete and legal under the existing producer/dependency and cycle gates; there is no fallback to final Body, stale prefix or an unrelated stage.
- Exactly one resolved semantic Curve family exists at that stage, with **at least two** distinct current real material Edge realizations. It must not be a seam or representation partition.
- Exactly one resolved semantic Point address exists at that stage, representing one current referenceable Vertex, and the material-Edge incidence of the Point vertex intersects the Curve's real bounded Edge realizations in **exactly one** Edge.
- That one Edge has exactly two incident native Vertices, of which **exactly one** is independently certified as a resolved semantic Point, and it is the encoded Point. A second certified endpoint makes `AtSingleSemanticPoint` **inapplicable**, so it cannot alias a valid `BetweenSemanticPoints` link in this current stage.
- All other conditions return a typed Missing/Ambiguous/Unsupported or integrity failure; they never authorize selecting the first/nearest member, rewriting an old branch or silently detaching an existing Sketch EntityId.
- A newer stage can continue an earlier semantic Curve only according to accepted Part/provider history rules, not matching XYZ. This does **not** add cross-revision material-fragment lineage as a hidden requirement.

This is the **Owner-approved option A semantic contract**, not a delivered production capability.

## 2. Proposed v16 data delta (requires separate Owner D2 approval)

Current native Part `PartDocumentStore::current_schema_version` is **15**. The existing `EdgeBranchDiscriminator` is a sum of `SingularAtAuthoredStage` or `BetweenSemanticPoints`. Existing `part_document_store.cpp` `materialEdgeReferenceJson`/`parseMaterialEdgeReferenceV14` carry both unchanged kinds into the current v15 format.

**Proposed** third kind in a future v16 container (illustrative, exact fields subject to accepted design):

```json
{
  "stage": { "kind": "after_feature", "feature_id": "<existing-feature-id>" },
  "curve": { "...": "existing FeatureCurveAddress v15 object" },
  "branch": {
    "kind": "at_single_semantic_point",
    "point": { "...": "existing FeaturePointAddress v15 object" }
  }
}
```

The elided objects are deliberately **not** new serialization contracts; they mean reuse of the existing canonical v15 `FeatureCurveAddress` and `FeaturePointAddress` field layouts. No fields added to either address and no global two-Surface `FeaturePointAddress` expansion.

Proposed parser rules: exact parent object with stage/curve/branch, exact branch object with **two** keys for the third kind, canonical valid FeaturePointAddress, allocated producer FeatureId, legal earlier-stage producer order, no duplicate or cyclic Surface→Curve→Point→Edge provenance. Unknown `kind`, unexpected fields, malformed schema/variant and absent Point are rejected; never downgrade to a weaker v15 branch or treat an unsupported identity as geometric Unsupported.

### Migration / version selection

1. A future implementation's reader accepts valid **legacy v1–v15** input as it does today, preserving the **exact existing** two `MaterialEdgeReference` discriminants and their meaning; never reinterpret old `BetweenSemanticPoints` into the new one-Point variant, never rewrite authored EntityIds and never infer missing Point relations.
2. The third branch is **only accepted inside an explicit v16 Part domain package**. Version `<=15` with `branch.kind=at_single_semantic_point` must fail as malformed/unsupported, not silently open without links.
3. v16 files containing the third branch, produced after future acceptance, are intentionally **not readable by an unmodified v15-only reader**: existing `domain_schema_version > 15` check returns `unsupported_schema`. We promise backwards *reading* by the new application, **not backwards writing** to old versions or lossless downgrade to v15.
4. A saved document using only old branches must preserve the exact semantic meaning and stable Sketch EntityIds; global v16 re-save of such a document is a **proposed** policy, not automatically approved here. The Owner must decide whether all saves become v16 or old-only files may remain v15.
5. `PartDocumentStore::save` retains atomic FileCheckpoint / save-conflict semantics. Failed schema write or parse never changes the old file, draft, Undo/Redo stack or current linked bindings.
6. A mixed v16 file may contain old and new branches at distinct sources. Stage-scoped duplicate-source validation must use accepted strict semantic rules and detect illegal simultaneous aliases **before** one atomic `CreateProjectedSketchEdgesCommand` mutation. No geometric/ordinal dedup. Any ambiguous or stale source refuses the whole atomic Finish; nonmaterial certified artifacts remain excluded and geometric Unsupported remains an independent partial-authoring outcome only after strict source certification.

## 3. Mandatory RED→GREEN tests after explicit implementation approval

| Case | Required result |
|---|---|
| New v16 one-Point fixture | Third branch exact JSON accepted only at v16, saved/cold reopened with same semantic addresses and stable target EntityId |
| Valid historical v15 linked part | New reader preserves legacy `SingularAtAuthoredStage` / `BetweenSemanticPoints`, references, original target EntityIds and downstream features; no migration-by-guess |
| v15 with injected new third kind | Reject malformed/unsupported, no silent downgrading or dropped Project Geometry |
| Future v16 with unknown fourth kind or extra fields | Reject, typed parse diagnostic and no partial state |
| Strict previous-stage, producer and circular references | Reject unallocated producer, illegal forward/cyclic provenance or malformed Point; preserve structurally valid deleted historical producers as repairable intent, with current resolution failing closed |
| Mixed v16 old/new, exact source once | Revalidation rejects actual same-stage source alias and duplicate link, preserves earlier staged choices; one atomic command |
| New one-Point source with two certified endpoints | Fail closed even if Curve + Point would be unique; ensures disjointness from v15 `BetweenSemanticPoints` |
| Unavailable source and historical edits | Delete/Suppress source Feature, predecessor edit, undo/redo, saved cold reopen: unavailable does not use last-good prefix nor retarget to a different source |
| Provider fresh evaluation / 36 synthetic cases | Exact one native material Edge admitted 36/36 with same semantic anchors, no token reuse |
| Native ambiguous Curve + Point | Proven actual (not injected-only) ambiguity fails closed; no provider-order first winner |
| Manual Owner PG-01D Face | Exact clicked bounded Face boundary, eligible Edge subset as contract, no neighboring Face expansion, selected source refs stable after cold Save/Reopen and Undo/Redo |

After implementation: narrow Part/Kernel tests, Desktop Workbench tests, docs PL/EN with regenerated Browser, Windows FAST/FULL on one exact candidate HEAD and **explicit Owner PG-01D FINAL PASS** before merge. No test-only synthetic admission is a substitute for Owner private geometry.

## 4. Decision still required before any production mutation

**Proposed D2-V16:** authorize the specific third branch `AtSingleSemanticPoint` with the strict endpoint-count domain and Owner-approved semantic associativity A; bump native Part schema from 15 to 16; preserve all existing v15 branch semantics under v16; forbid v15 files from containing the new kind; reject unknown/invalid variants; implement deterministic source dedup and mixed-link validation; and forbid v16→v15 lossy downgrade. Decide whether the new writer always emits v16 (recommended for one canonical current format) or retains v15 only when no new branch occurs (more compatibility complexity).

This document is a **proposal**, not a declaration that the D2-V16 decision was accepted. No new source representation, serializer/parser, UI or domain mutation may be implemented merely because associativity option A was approved.

## 5. Source audit and bounded implementation proposal — 2026-10-10

**Review baseline:** PR #309 `2d0eab10efb80fe6f392e10ac3c5a10f46b39d54`.
**Authority:** design/evidence work only. The following is a concrete proposed
implementation delta, NOT Owner approval, a changed active contract or a claim
that v16 already works.

### 5.1 Version gate must cover the entire recursive reference grammar

Observed in `src/part/part_document_store.cpp`:

- `parseMaterialEdgeReferenceV14` has no schema-version argument.
- Surface parsing recursively calls it for Fillet/Chamfer `source_edge`
  and corner-transition `incident_edges`.
- Curve and Point addresses recursively contain Surface addresses.
- Top-level projected bindings, edge-feature inputs and supported Surface
  references share this grammar.

Adding a new branch only to the common parser would allow a third branch to
leak into v14/v15, including hidden nested occurrences. **Proposed:** carry one
explicit immutable domain-schema parse context through every recursive
Surface/Curve/Point/Edge parser and every entry point. Admit the new kind only
for schema 16; do not infer version from a field, helper name or consumer.
Keep the exact existing object-key validation and old-kind meaning. The
serializer must handle each variant explicitly and fail on an invalid variant,
rather than treating every non-singular value as two-point.

Required negative fixtures inject the new kind into a v15 projected binding,
a nested generated Surface source Edge, and a corner-transition incident Edge.
All must reject the whole load. Add v14 nested coverage and v16 unknown-kind,
missing-point and unexpected-field coverage. Valid v15 mixed *old* kinds must
retain identical semantic addresses and target EntityIds after a v16 save.

### 5.2 Domain validation must visit the new Point, including nested provenance

Observed in `src/part/part_document.cpp`:
`semanticEdgeHistoryValid` checks Point history only for
`BetweenSemanticPoints`; a non-two-point branch currently takes the
`endpoints == nullptr` success path after Curve validation. That is correct
for today's singular branch but would bypass validation of the proposed
one-Point payload if the variant were added without revisiting this function.

**Proposed:** explicit exhaustive per-variant validation, including the new
Point through the existing `semanticPointHistoryValid` and narrowed stage
bounds. Preserve canonical ordering/equality and all recursive generated
Surface provenance checks. Test the domain restore/transaction boundary
directly, as well as the file parser: rejecting JSON alone does not protect
non-persistence callers.

**Structural validity is not current resolvability.** Existing
`SemanticProvenanceBounds::accepts` deliberately permits allocated, deleted
historical producers as repairable durable intent. Preserve this distinction:
unallocated IDs, illegal producer order, malformed addresses and prohibited
dependencies are invalid authored data; a valid historical source that is
deleted/suppressed or currently unavailable remains saved intent and resolves
Missing/Unsupported as appropriate. Do not make Save/Load require a current
OCCT Body or reject every broken link. No automatic branch rewrite, detach or
EntityId replacement.

### 5.3 Exact earlier stage, not a global final-Body prerequisite

Observed in `src/part/feature_evaluation.cpp`:
`projectStrictMaterialEdge` checks the current document revision and the
specific source Feature's up-to-date `result_solid/result_topology`, including
exact stage and consumer ordering. It does not require that source stage to be
the document's final Feature. This is needed for ordinary upstream links.

The research helper `pg01dStrictFinalStageOnePointCandidates` intentionally
requires a final Body because its fixture's source is the final Chamfer.
**Do not copy that fixture restriction into the production resolver.**

Proposed production resolution consumes the same fresh, complete exact-stage
catalog as existing links. An unavailable requested stage fails; an unrelated
diagnostic prefix cannot replace it. A currently valid earlier source stage
does not become invalid merely because a *later* Feature fails. Add paired
tests: unavailable source stage refuses, and legal earlier-stage projection
still resolves with a later failed Feature. Keep target-support and consumer
dependency checks independent.

### 5.4 Preserve consumer scope: no implicit Fillet/Chamfer authoring expansion

Observed: both Face admission and edge-feature workflows use the common
`MaterialEdgeReference` family; changing `authorMaterialEdgeReference`
globally could make new Fillet/Chamfer inputs authorable without a separate
product decision.

**Recommended bounded choice for Owner approval:** introduce the one-Point
authoring fallback only for Project Geometry's Edge and Face admission
paths. Keep existing general Fillet/Chamfer input authoring on the two accepted
kinds. Reuse one Part-owned strict one-Point resolver and structural type,
without a second semantic identity truth. A narrowly named Part projection
authoring helper can first delegate to existing authoring, then try the new
branch under the strict domain; do not add a public generic feature-policy
framework.

Because the reference grammar is recursive, a one-Point link may legitimately
contain *old* generated Fillet/Chamfer Surface provenance. That does not imply
permission to create a new Fillet/Chamfer whose input is the third branch.
Before implementation, explicitly specify and test admission at command,
domain reconstruction and persistence boundaries for that unsupported direct
consumer. If this cannot be bounded without changing existing accepted
consumer semantics, return to Owner D2; do not silently widen PM-05.

### 5.5 Deduplication proof and legacy compatibility

Observed: `DocumentSession::execute(CreateProjectedSketchEdgesCommand)`
compares incoming and existing target-Sketch sources by structural
`MaterialEdgeReference` equality. Canonical authoring priority alone cannot
prove semantic uniqueness after edits. The existing edge-feature Kernel-input
adapter separately rejects repeated *current scoped* Edge tokens; those tokens
are transient validation evidence, never durable equality.

The proposed strict one-Point domain must be checked on **every resolution**:
multi-realization Curve, two incident Vertices, exactly one independently
certified Point endpoint, and one matching current material Edge. A second
certified endpoint or collapse to a singular Curve makes that saved branch
inapplicable; it must never normalize to an old branch.

Do not claim all three legacy/new variants are globally disjoint:
the existing two-Point resolver does not require a multi-realization Curve,
so old singular/two-Point overlap is a distinct pre-existing possibility.
Do not change those legacy resolver semantics in this amendment.

**Proposed bounded acceptance:** preserve existing structural equality and
legacy-only behavior; prove that an admissible new reference cannot alias any
currently admissible old reference or another structurally different new
reference under the strict Point/catalog uniqueness conditions. Validate
incoming new candidates atomically before commit. Add mixed old/new batches
and existing-target-link cases, plus synthetic corrupt/aliased catalog
controls. If any cross-variant alias survives, STOP for an explicit semantic
dedup policy; runtime token comparison is not permission to canonicalize or
rewrite stored links.

A broken existing link must remain repairable; it must not by itself block
unrelated new sources or be silently deleted to make a batch pass. The exact
same authored source remains a structural duplicate even when currently
broken. Re-evaluation never silently adds/removes target EntityIds.

### 5.6 Ordered implementation scope and required evidence

After explicit approval of this delta and incorporation into the active
PG-01D authority:

1. Part semantic type, exhaustive structural/history validation and bounded
   projection-only authoring/resolution. Relevant files:
   `semantic_topology_reference.hpp`, `part_document.cpp`,
   `feature_evaluation.cpp/.hpp`. Preserve provider-neutral identity and
   accepted numerical/modeling behavior.
2. Version-aware recursive parser/writer in `part_document_store.cpp/.hpp`.
   Recommended writer policy: always emit v16; reader retains v1-v15 support;
   no v16-to-v15 downgrade. This is a proposed policy awaiting approval.
3. Existing application Project Geometry command and UI admission integration
   only as needed for the accepted third reference, atomic validation and
   typed diagnostics. No new Face/group persistence or provider identity API.
4. Extend existing resolver/schema/generated-Surface/native/Workbench tests.
   Reuse targets; no CI #302 changes. New registration requires explicit
   justification within the existing verification rules.
5. Current internal and paired PL/EN product docs; regenerate Browser using
   `./ss2.ps1 docs`. Exact-head FOCUSED tests, then broad verification and
   one stable Windows FULL. Separate Owner private-case practical retest,
   PG-01D FINAL PASS and merge approval remain mandatory.

Additional required cases beyond section 3:

| Boundary | Required evidence |
|---|---|
| Domain entry independent of parser | Invalid one-Point producer/order rejected by restore and transaction with zero authored mutation |
| Recursive version isolation | New kind rejected at every nested legacy entry; exact fields and old semantics retained |
| Broken historical intent | Deleted/suppressed valid source survives save/cold load as unresolved, never last-good substitution |
| Earlier valid source | Source at an earlier Feature resolves despite a later failed Feature, subject to valid target support |
| Cardinality transitions | One-to-two certified endpoints and multi-to-singular Curve invalidate new branch; Undo restores original reference without rewriting it |
| Mixed sources and aliases | Strict new domain cannot evade existing-target or batch duplicate rules; one invalid incoming source rejects whole commit |
| Consumer admission | Projection may acquire new branch; no implicit new Fillet/Chamfer input authoring or unsupported direct-consumer load |
| Fresh process | Actual v16 one-Point link saved and reopened with fresh Part/provider, stable EntityIds and same semantic anchors |
| Existing product behavior | Edges/Planar Face/manual multi-Face, geometric Partial, Break Link and downstream Profile/Features retain contracted lifecycle |

### 5.7 Evidence checkpoint verified during this review

Windows [FOCUSED #2388](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38080022327)
completed **PASS 1/1** at the exact review baseline above:
`pm02jr2.add_surface_continuation`, rebuilt Kernel Release target,
documentation/bootstrap and aggregate `windows-msvc` PASS.
FAST and FULL jobs were **skipped**. The source asserts one real v15 cold
forward-edit case and one real v15 cold reversed/unavailable-body case;
neither persists a third reference. This result does not validate a subsequent
documentation commit or any future v16 implementation. Owner PG-01D practical
FAIL and production/schema STOP remain open.

## Documentation Impact

Internal `work/` design only; no shipped semantics or generated-browser change. On separate implementation approval, update canonical paired PL/EN public documentation and regenerate Browser with the repository's Windows documentation command; do not edit generated Browser manually.
