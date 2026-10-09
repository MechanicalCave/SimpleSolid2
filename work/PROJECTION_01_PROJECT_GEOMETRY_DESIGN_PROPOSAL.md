# Project Geometry — Part v1 Production Design and Staging PROPOSAL

**Status:** STAGING PLAN OWNER ACCEPTED 2026-10-09; PG-01A Owner final PASS and MERGED, PG-01B D2 architecture + draft Work Contract proposed but NOT active; PG-01C/D/E remain gated
**Prepared:** 2026-10-09
**Baseline:** main `2cda8e04d7d0e2594edae7dc90e88cbb90dcae35`, Projection 00A **COMPLETED — Owner PASS**; [PR #299](https://github.com/MechanicalCave/SimpleSolid2/pull/299) merged; [FULL #1915](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37890913601) PASS 195/195
**Accepted program:** Part Modeling v1 roadmap v1.30, Projection **before PM-06**
**Accepted D2 direction:** Foundation §7.2 and §7.5: same-Part **associative**; §7.3 future Assembly-context **snapshot** CORE unchanged
**Current authority:** `work/ACTIVE.yaml` still points to **completed** `work/PROJECTION_01A_EXACT_KERNEL_CONTRACT.md` for traceability. PG-01A mutation authority is closed. No PG-01B/C/D/E production package is activated. Next D2 proposal: `work/PROJECTION_01B_ASSOCIATIVITY_DESIGN_PROPOSAL.md`.

## Product behavior that has already been agreed with Owner

One Sketch tool **Project Geometry / Rzutuj geometrię**; all working controls and diagnostics in the **right-hand panel**. Selection options are **Edges** and **planar Face** (including boundary of holes); selection can be staged with preview and no authored changes until Finish. A successful combined Finish is one Undo step; Cancel has no mutation. If both modes are used in one session, duplicate source edges must be explicitly deduplicated before preview/Finish.

Output entities use existing Sketch Line, Circle and Arc representation. Default output **Regular** (participates in Profile/Extrude/Revolve); optional **Construction** (reference-only). This role is orthogonal to link state. Same-Part output is **linked/associative by default**: distinct color and Properties indicator, source-controlled geometry not freely dragged, compatible ordinary constraints/snaps to other entities, explicit **Break Link** to editable local geometry retaining the target EntityId and compatible constraints; existing Profile references are not silently replaced.

For selected planar **Face**, traverse outer and inner boundary, project each individually exact-representable edge. **Unsupported geometric curve types or unrepresentable exact images are visibly skipped, with individual reasons and source highlight**; commit the rest and let the Owner manually close any open contour. There is no auto healing, polyline tessellation, silent closure or requirement that partial selection itself create a valid Profile. A semantic Missing/Ambiguous/invalid source, stale stage/generation or a cycle is **not** a skippable geometric Unsupported error. Bound handling of these integrity cases must be explicitly frozen before the affected production package; Preview/Finish must never invent or silently rebind an identity.

Initial exact scope: projected **Line → Line**, **Circle → Circle**, **Arc → Arc**, only if the *result* is representable; tilted circular curve yielding ellipse is unsupported, as is a line collapsing into a point. Future ellipse/spline support extends the projection result contract, not a speculative approximation policy.

For an already linked entity, recompute the existing strict source binding. If a source Face gains or loses boundary members, **do not auto-create, delete or reassign Sketch EntityIds**; missing existing binding fails closed and preserves durable intent for explicit repair. Optional membership Refresh/Reproject is deferred. No automatic cross-Part or Assembly associativity.

## SS1 donor vs SS2 authority

| SS1 proven source | Reusable finding | SS2 boundary |
| --- | --- | --- |
| `components/SimpleSolid_Kernel_API/include/simplesolid/kernel/edge_projection.hpp` | provider-neutral Line/Circle/Arc result and typed Invalid/Unavailable/Unsupported/Degenerate | new SS2 `kernel::` API and runtime token/types |
| `components/SimpleSolid_Kernel_OCCT/src/occt_edge_projection.cpp` | `BRepAdaptor_Curve`, circle-axis check, frame coordinates, full circle vs arc, orientation | SS2 numerical policy, analytic geometry, no copied magic tolerances |
| `components/SimpleSolid_Next/src/application/project_geometry_service.cpp` | current semantic source → provider query → target 2D result | SS2 `MaterialEdgeReference` + `BodyStageRef`, not SS1 `ExtrudeGeneratedEdgeReference` |
| `components/SimpleSolid_Next/include/simplesolid/next/part/internal_projection_binding.hpp` | target EntityId → durable same-Part binding pattern | do not persist stale geometry as fallback current truth; SS2 Part-authored binding with evaluated separate view |
| `sk01_b2_projection_materialization_test.cpp`, `sk01_b3_project_geometry_edit_test.cpp`, `sk01_b4_break_link_delete_test.cpp` | associativity, edit, Break Link/Undo/Delete regressions | adapt fixture/test boundary, no entire legacy subsystem import |
| `docs/work/SK-01/FINAL_REPORT.md` | fixed canonical UI Break Link dispatch and evaluated Sketch-driven Edit Extrude Preview pitfalls | SS2 right-panel and current Part evaluator own UI truth |

## Actual SS2 architecture constraints from main

- `part::BodyStageRef` / `MaterialEdgeReference` give strict semantic stage+curve+branch address; `authorMaterialEdgeReference` / `resolveMaterialEdgeReference` fail closed.
- `BodyStageTopologyCatalog` carries runtime `BodyFaceTopologyRecord` / `BodyEdgeTopologyRecord`, Curve carrier vs strict Edge realization and optional Face carrier ambiguity. **A resolved Curve is NOT automatically a singular Edge**.
- `kernel::ISolidModelingKernel` / OCCT provider own runtime body and provider-only topology tokens; Sketch stores `EntityId` and Line/Circle/Arc.
- `PartSketch` has support/frame and `SketchModel` authored entities; `profile::resolveProfileRegionIntent` reads current Sketch Model. Bounded associative recompute needs **separate durable bindings and a derived Sketch view for profile evaluation/preview**; mutating durable `SketchModel` during evaluate is prohibited. Precise design needs D2 freeze after PG-01A.
- [Projection 00A evidence](PROJECTION_00A_PROFILE_REPAIR_EVIDENCE.md) proves a 4→5 Line contour can be repaired **manually** with `ReplaceProfileRegionIntentCommand`, same ProfileId/FeatureId, Undo/Redo, downstream Cut and true cold Save/Reopen. It does not prove any projected-curve provider behavior, UI flow or assoc cache/persistence.

## Proposed independently accepted packages

| Package | Deliverable | Prohibited until later gate |
| --- | --- | --- |
| **PG-01A — Exact Kernel Edge Projection** | provider-neutral typed result and OCCT Line/Circle/Arc projection with native accuracy / failure tests; no durable schema, no Sketch mutation | bindings, UI, Face selection, command Finish |
| **PG-01B — Associative Part/Sketch Evaluation & Persistence** | Owner-approved D2 data-flow: target `EntityId` → strict source `MaterialEdgeReference`, ordering/cycle guard, derived Sketch geometry used by Profile, typed Broken, Break Link core semantics, versioned storage/migration and cold rebuild | full UI and Face automation |
| **PG-01C — One Project Geometry tool, Edge workflow** | right-panel single tool, staged multi Edge picking, Regular/Construction choice, preview/Finish/Cancel, link color/status, Break Link/Delete, semantic UI & lifecycle tests | whole Face capture |
| **PG-01D — Planar Face Boundary (partial)** | one selected planar Face, outer/hole loops, deduplication and typed per-edge Unsupported skips, staged partial preview, no silent repaired geometry; native UI Owner workflow | nonplanar Face, ellipses/splines, automatic topology membership updates |
| **PG-01E — Product-wide closeout** | native Windows Owner acceptance, multi-feature repair, Undo/Redo/Save/Reopen/migration/PL+EN Product docs + generated Browser, resource/performance and aggregate FULL | PM-06 activation without separate Owner gate |

PG-01B must explicitly settle: no dependency cycles from source stage after target-consuming Feature; differentiated Missing/Ambiguous/Unsupported statuses; snapshots of materialized values only for rendering diagnostics, never stale current Profile; if an active linked Regular entity fails recompute, affected Profile/Features **fail closed**, not partly consume ghost geometry. Structural sketch constraints when a linked entity changes type or becomes unrepresentable need typed failure/repair. Breaking link requires latest **resolved** current geometry; no "last-good" recovery masquerading as current.

PG-01D must settle: which exact Face semantic identity is selectable vs Surface carrier (one Surface may have multiple Face realizations), how wire ordering is recovered from OCCT, which edges are duplicates/perimeter or holes, how unsupported-but-well-identified curves are distinguished from ambiguous semantic Edge identity. Exclude default automatic Face membership reconciliation. If zero edges project, Finish should have no authored mutation and show diagnostic.

## CI cost policy

Reuse existing FOCUSED Kernel/Part/native targets and warm persistent build trees for iterative work. Avoid `tests/CMakeLists.txt` modifications solely to create a one-off proof when an existing suitable target exists; if a new source registration is architecturally warranted, batch CMake changes and accept the one required clean FULL instead of compromising maintainability. Finish each package on stable tests via FAST and one exact-head Ready-for-Review FULL. Never tune classifier/CI security rules simply to make expensive gates disappear.

## Proposed next activation

**PG-01A formally completed:** Owner final PASS 2026-10-09 and guarded [PR #300](https://github.com/MechanicalCave/SimpleSolid2/pull/300) squash merge `27268d4ec10fbc09721a16ab8f0e2d59bb54833f`; exact-head Windows FULL #1932 PASS 195/195. Next PG-01B has an independently prepared **D2 architecture proposal and DRAFT Work Contract**. Owner must explicitly approve architecture and exact package before ACTIVE can point to PG-01B or code implementation begins.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: Design-only proposal; no changes to shipped SS2 behavior. Future production packages must carry complete internal and bilingual PL/EN Product docs + generated offline Browser after the appropriate user-visible milestone.
