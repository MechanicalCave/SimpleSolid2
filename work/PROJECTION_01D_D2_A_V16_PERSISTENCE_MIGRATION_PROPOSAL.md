# PG-01D D2-A — proposed versioned one-Point material Edge reference (v16) and migration

**STATUS: PROPOSED — NOT OWNER-APPROVED FOR PRODUCTION / SCHEMA MUTATION.**
On 2026-10-10 the Owner explicitly approved **semantic associativity option A** for a future bounded Curve segment: stable semantic Curve + one already certified semantic Point at the exact earlier Body stage, with geometrical deformation across upstream edits allowed. This is **not** an approval of a third persisted branch, the v16 version choice, migration behavior, implementation, release, or merge. The governing active Work Contract remains PG-01D and its later manual Face Boundary amendment. PR #309 stays Draft.

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
| Strict previous-stage, producer and circular references | Reject wrong stage, missing producer, invalid Point, nested generated Surface/source Edge provenance cycle |
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

## Documentation Impact

Internal `work/` design only; no shipped semantics or generated-browser change. On separate implementation approval, update canonical paired PL/EN public documentation and regenerate Browser with the repository's Windows documentation command; do not edit generated Browser manually.
