# PG-01B — Same-Part Associative Project Geometry: D2 Architecture Proposal

**Status:** DRAFT — OWNER D2 ARCHITECTURE REVIEW REQUIRED; NOT ACTIVATED
**Prepared:** 2026-10-09
**Baseline:** main `27268d4ec10fbc09721a16ab8f0e2d59bb54833f` — PG-01A FINAL OWNER PASS, exact-head [Windows FULL #1932](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37897152384) 195/195, [PR #300](https://github.com/MechanicalCave/SimpleSolid2/pull/300) squash merged
**Program:** accepted Part Modeling v1 roadmap v1.30; Projection before PM-06
**Existing accepted direction:** Foundation §7.2/§7.5 same-Part associative; future Assembly-context snapshot §7.3 unchanged; Project Geometry single right-panel tool with Regular/Construction and partial planar Face projection in later packages
**Proposed Work Contract:** `work/PROJECTION_01B_ASSOCIATIVITY_CONTRACT_DRAFT.md`
**Authority:** `work/ACTIVE.yaml` still names *completed* PG-01A. A design proposal is not production authorization.

## 1. Hard technical findings from current SS2

- `part::PartSketch` currently contains `SketchModel` plus support; `SketchModel` stores persistent stable `EntityId`, role Regular/Construction and concrete Line/Circle/Arc seed geometry. No Part-local projected-binding property exists.
- `PartDocument::evaluateProfile` calls `resolveProfileRegionIntent(source->model, region_intent)`, and `profile_kernel_input.cpp` reconstructs 2D loops from `source->model`. `evaluatePart` feeds these into Extrude/Revolve in ordered Feature iteration. **If source-linked curves were shown only in the viewer while these methods still read the authored seed, the derived solid would silently become stale.**
- `FeatureEvaluation::result_solid` and `result_topology` preserve exactly the current successful upstream Feature stage; `BodyStageTopologyCatalog` has strict semantic Edge realization and `resolveMaterialEdgeReference`.
- PG-01A provides `kernel::IEdgeProjectionQuery::bindEdgeToBody` and `projectEdgeToPlane`, including generation-specific opaque `ScopedProjectionEdge`. A raw `RuntimeEdgeToken` is *never* durable identity.
- Current native Part document schema is **v14** (in `PartDocumentStore::current_schema_version`). Serialization writes the authored Sketch Line/Circle/Arc states; no binding/cold-reprojection metadata is currently present. No binding values may be persisted as OCCT tokens, derived cache, or modeler snapshots.
- [00A](PROJECTION_00A_PROFILE_REPAIR_EVIDENCE.md) proves explicit 4→5 Line Profile repair, stable ProfileId/FeatureId, downstream Cut and true cold Save/Reopen. It did **not** establish that automatic semantic Face edge membership reconciliation is safe.

## 2. Proposed D2: persistent meaning and derived geometry

Store each linked projection *in Part*, alongside the target hosted Sketch, as a narrowly typed `ProjectedEdgeBinding` (name provisional):

- **Target:** owning `SketchId` via containing `PartSketch` and one existing `sketch::EntityId`. The target EntityId is stable and belongs to exactly one typed Sketch Line/Circle/Arc entity.
- **Source:** one valid strict `MaterialEdgeReference` containing `BodyStageRef` (after an upstream Feature), semantic `FeatureCurveAddress` and `EdgeBranchDiscriminator`. No runtime token or geometric-nearness key is serialized.
- **Role:** existing target `SketchModel` entity role (Regular by default; Construction optional), **not** a property of association. Break Link does not change Regular/Construction.
- **Binding lifecycle:** a target absent from the binding collection is detached and editable; a linked target's durable authored geometry is a **seed only**, never the authoritative current projection.
- **Projection state:** derived `Resolved / Missing / Ambiguous / UnsupportedCurve / Degenerate / ProviderFailure / CycleOrFutureStage / MissingProvider`, with usable curve only when Resolved. The broken binding persists for explicit repair and never silently detaches.
- **Stable curve kind:** the typed source image must remain Line→Line, Circle→Circle, or Arc→Arc while linked. If a recompute changes analytic kind (including Arc↔Circle), it becomes a typed repair requirement: do not silently replace a Sketch Entity kind or EntityId.

Do not introduce a universal cross-document dependency graph. Part alone owns this bounded same-Part link. Future Assembly snapshot remains excluded.

## 3. Proposed D2: evaluation pipeline and source-stage legality

A pure, revision-bound `evaluateEffectiveSketch(document, sketchId, prefixEvaluation, projectionQuery)` materializes a **temporary copy** of the authored `SketchModel` plus per-EntityId projection status. It evaluates each bound source against **the exact stage named in MaterialEdgeReference**:

1. Validate current DocumentRevision and target Sketch support Frame. Resolve `BodyStageRef` only from the successful Feature prefix at that stage; never use current final Body, later stage, last-good BRep or cached viewer topology.
2. Call strict `resolveMaterialEdgeReference(source, stage.result_topology)`. Exactly one eligible Edge and exact stage identity are required. Missing, Ambiguous, seam/representation-only or unsupported material Edge fail closed.
3. Bind the resulting `RuntimeEdgeToken` with **that same** `stage.result_solid` using PG-01A, then project into current supported Sketch frame. No guessed index, no geometric fallback.
4. On success, replace **only the temporary** Line/Circle/Arc geometry for that original EntityId. Authored source seed, binding, role, EntityId and PartDocument revision remain unchanged.
5. On failure, never present the seed or last-good geometry as a valid current curve. Preserve typed diagnostic for rendering/repair. Unrelated Sketch entities remain usable.

**Critical integration:** the temporary effective Sketch must feed `resolveProfileRegionIntent`, `analyzeRegions` where applicable, **and** the actual `convertLoop`/Kernel profile construction for Extrude/Revolve, plus edits/draft previews. Updating only the display while leaving `PartDocument::evaluateProfile` or `profile_kernel_input.cpp` on the authored Sketch is NOT ACCEPTED. Derivation must be pure (no transaction, Undo, serial mutation, or document-revision increments on reevaluation).

**Required order:** for every Feature that consumes a Profile from a Sketch containing linked entities, every associated source Feature stage **must precede** the consumer Feature, and its source stage must be available in that exact evaluated prefix. Include upstream Sketch-support and Datum dependencies in cycle checks. Self-link, forward-stage access or consumer→source cycles are prohibited at authoring/repair and fail closed on structurally inconsistent load/diagnostics. Do not treat suppressed/failed future stages as valid.

**Scope:** a broken linked Regular entity blocks Profile/Features only if the Profile actually depends on that EntityId or its anchor; a missing linked Construction entity does not invalidate an unrelated otherwise-valid Profile. A Profile with broken linked membership fails closed (unavailable/blocked) and cannot use ghost geometry even when an older seed would form a closed loop. Other unrelated Sketches and Feature input remain unaffected until their own dependencies fail.

## 4. Proposed D2: commands, edit behavior and Break Link

- One semantic operation may create multiple linked Sketch entities with stable EntityIds and durable bindings, **atomically** in one DocumentSession transaction and one Undo; validate source stage, exact projection representability, Sketch support and duplicates prior to Finish. In PG-01B this is a headless semantic Command/API; **not** the right-panel picking UI (PG-01C).
- The existing Sketch geometry-edit paths (`UpdateSketchGeometry`, Line/Circle/Arc individual edit, Trim/Extend, Delete/duplicate/copy, etc.) must **not silently mutate linked source-controlled geometry**. Regular/Construction role changes are allowed as an explicit separate authored command, with predictable Profile invalidation. Delete may remove target + binding atomically; otherwise reject edit until Break Link.
- **Break Link:** only from a successfully Resolved current revision. In one semantic command, copy *latest current evaluated* curve into authored Sketch seed, remove exactly that binding and keep EntityId, role, Profile semantic references and compatible constraint/anchor identities. Undo/Redo restores both seed and link atomically.
- A broken projection cannot Break Link using stale/last-good evaluated geometry. Offer explicit Repair or Delete in later UI; the headless core reports a typed reason. Constraints may reference linked targets, but a linked target is source-controlled; do **not** promise a general constraint solver redesign.
- No automatic Face edge count/membership reconciliation, source Face linking, hole-loop capture, or UI/viewport/picker work in PG-01B.

## 5. Persistence (proposed schema v15) and validation

Add a versioned provider-neutral **v15** extension to the authored Part JSON Sketch record for per-entity source bindings; keep existing v14 payloads valid and default to no linked entities on load. Reuse existing strict `MaterialEdgeReference` semantic serializer/parser vocabulary used by Edge Features, preserving stage, curve and branch disambiguation. No runtime Body/Edge/Face token, `ScopedProjectionEdge`, cache, evaluated curve or last-good validity bit in persisted data.

Reconstruction checks structural validity, duplicate target bindings, valid target EntityId and type, serial/high-water cursors, legitimate source addresses/stage ordering and absence of cycles **without requiring BRep availability during file load**. Missing/suppressed source Features may remain durable repairable bindings with **runtime broken status** rather than silently discarded. Malformed schema/binding structure fails load deterministically. Legacy v1–v14 reopen unchanged; v15 Save/Close/Reopen with a **new** Kernel must recalculate linked geometry and affected Profile/Feature without reusing old results.

A deliberate design distinction: structural-invalid linkage (e.g. duplicate/invalid stage/self cycle) fails reconstruction; an otherwise well-formed source that no longer resolves in the current topology loads and evaluates as **Broken**. Never silently "repair" by matching shapes.

## 6. Tests required before PG-01B acceptance

- Same-Part `Extrude A → Sketch B` bound to Edge in stage after A → Profile B consumes **Regular linked** curve → Extrude/Revolve B: parameter change to A recomputes effective Sketch and dependent Feature, preserving target EntityId and ProfileId.
- Constructed **Construction-linked** curve remains reference-only; toggling role explicitly affects Profile eligibility.
- Strict semantic Missing/Ambiguous, wrong stage, stage before/after consumer, source deletion/suppression, source analytic-type change, degenerate/unsupported query and provider mismatch fail closed. Geometry coincidence and colliding runtime tokens never rebind.
- Regression against **authored-seed stale usage**: change source without touching target Sketch; evaluator and feature preview must use derived geometry, and stale seed must not produce UpToDate Body.
- Break Link resolves current curve and freezes it; later source edits do not update detached entity. If broken, Break Link fails without document mutation; Delete can remove binding. Undo/Redo restores original association and stable IDs.
- v14 cold load migration and real `.ss2part` v15 Save/Close/Reopen, new Kernel and user-level session, no BRep/provider token serialization; restore broken source as repairable and reject structurally corrupt binding.
- Deterministic, revision-scoped rebuild; no mutation while evaluating, no loop/cycle recursion, one command/Undo for batched creation; ordered cuts and holes remain topologically correct when legitimate Profile includes link.
- Tests split core-only/fake deterministic `ISolidModelingKernel`+ `IEdgeProjectionQuery`, then **real OCCT** native integration and actual stage token inventories. Use existing registered target where maintainable, CMake changes grouped once if necessary.

## 7. Explicit D2 decisions Owner is asked to freeze

1. **Identity and edit model:** Part-owned `EntityId → MaterialEdgeReference`; authored seed retained solely as a non-authoritative structural snapshot; derived effective Sketch is the only current geometric truth for linked entities.
2. **Failure model:** typed Broken does not automatically detach, cannot silently consume old seed; only directly affected Profiles/Features blocked; broken Construction alone does not poison unrelated Profile.
3. **History + Break Link:** links only to earlier same-Part Body stage; exact semantic re-resolution; Break Link uses latest resolved curve, preserves target EntityId and compatible constraints, and is one Undo; broken link must Repair/Delete, not stale detach.
4. **Schema:** new v15 for linked PartSketch bindings, backward-compatible v14/no-link migration; no persistent OCCT generation data.
5. **Dependency integration:** common effective Sketch path for Profile evaluation, Kernel input, Edit/preview, UI diagnostic reads; strict ordered dependency and cycle checks, no parallel legacy path reading linked seeds.

These choices require explicit Owner D2 acceptance **before** changing the model/schema. Accepted PG-01A does not implicitly settle them. Specific implementation names may be refined without relaxing these invariants.

## 8. Package boundary, cost and next gate

`work/PROJECTION_01B_ASSOCIATIVITY_CONTRACT_DRAFT.md` proposes bounded Part/Sketch/Application/Persistence source, tests and internal docs. PG-01C UI/Project Edge and PG-01D planar Face partial capture are separate Work Contracts. No global kernel refactor, no Assembly, no new general geometry kernel. During iteration prefer Windows **FOCUSED**, then **FAST**, final single Ready-for-Review exact-head **FULL**; do not weaken CI to save time.

**Approval request:** Owner explicitly accept the five D2 choices above **and** the PG-01B Work Contract. Only then create an activation commit changing `work/ACTIVE.yaml` to the accepted PG-01B contract **before modifying code**.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: D2 architecture proposal only; shipping linked projection implementation will require internal as-built docs and generated Browser, and future PG-01C/PG-01E user-facing docs in PL/EN.
