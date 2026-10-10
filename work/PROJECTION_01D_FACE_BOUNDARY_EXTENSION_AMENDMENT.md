# PG-01D — Owner-accepted Face Boundary extension and exact projected-region follow-up

**Status:** OWNER ACCEPTED 2026-10-10 in conversation: `akceptuje - kontynuuj`, after explicit discussion and agreement on **Edges / Planar Face / Face Boundary**, preservation of Planar Face, no automatic joining of actual material boundaries, and separate Profile-region diagnosis. **No Owner practical PG-01D FINAL PASS or merge permission.**

**Authority and activation:** extension to the already active `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md` under unchanged `work/ACTIVE.yaml`; the previously accepted D2-F1–F6 **Planar Face** implementation and Windows FULL [#2189](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37998629725) 199/199 PASS at `9f6299daea28a16a5e496d01ffa183755224b397` remain immutable historical evidence **only for that SHA**. This amendment does not retroactively redefine its acceptance matrix.

## 1. Frozen product intent

One `PROJECT` tool with three **source-acquisition modes**, all delegating the resulting unique, exact material Edges to the existing `CreateProjectedSketchEdgesCommand`:

1. **Edges:** pick individual current material Edges; existing PG-01C.
2. **Planar Face:** keep the current strict, *one bounded planar provider Face* outer/hole wire mechanism, diagnostics, tests and fail-closed gate; existing PG-01D.
3. **Face Boundary (new):** a clicked *surface area* is a temporary **selection definition**, not a new projected surface nor an authored Face dependency. Collect the material Edge occurrences forming the perimeter (outer and actual openings) of its logically contiguous **material surface region**, regardless of whether that surface is planar, oblique, cylindrical or otherwise curved. Only those Edges, not the surface itself, are independently projected on the active Sketch support.

If the provider partitions **one certifiably continuous semantic surface region** into several bounded Faces, traverse exactly those genuine same-region fragments and **exclude strictly certified internal representation/partition edges** from the selected perimeter. Do **not** join separate real material regions solely due to coplanarity, tangency, proximity, matching color or shared coordinates. A real boundary, opening, hard edge, change in semantic surface intent, or failed region-continuity proof stops the traversal. Multiple material Edges that happen to project to equal coordinates remain distinct semantic sources; manual Edge+Face Boundary duplicates are removed only by exact `MaterialEdgeReference` equality.

**Required distinction:** a source Edge with a proven, unique material identity but a geometrically unrepresentable projection (e.g. oblique Circle → ellipse, non-exact spline/degenerate Line) is **skipped individually with explicit diagnostics and red source highlight**; supported Edges remain in preview and may Finish as **PARTIAL**. In contrast, source region identity, edge provenance, provider scope, stage freshness, wire accounting, or continuous-region membership ambiguity is **not a skippable geometry failure**: reject this Face Boundary acquisition without corrupting previous manual Edges. Do not approximate, heal, close gaps, introduce new authored curve kinds or author zero-supported selections.

## 2. Architectural boundaries

- Provider owns **read-only native oriented Face/Edge incidence**, including nonplanar bounded Faces; never derive source membership from tessellated Viewer triangles. The existing planar `IFaceBoundaryQuery::queryFaceBoundary` and `part::inspectMaterialFaceBoundary` remain unchanged for Planar Face. A separate/optional typed Face Boundary query or a **narrow compatible extension** must be scoped to the same exact native Body/generation, with precise status and no OCCT types outside provider. This is the Owner-approved new boundary in **this amendment only**.
- Part owns certification of the logical contiguous **material** region, same-stage `BodyStageTopologyCatalog` and strict per-edge references. A nonplanar Face does **not** automatically mean `Unsupported`; only individual exact Edge projection statuses determine geometric support. Never elevate a representation-only partition to a durable Edge or erase a real material boundary.
- Workbench presents a **third** mutually exclusive source button/command keyword in the **same** Project Geometry tool state. It must give per-member accepted/skipped and **typed blocked** reason with source Face/Edge member context; preserve staged manual Edges, Regular/Construction, Remove/Clear/Esc/Finish, current-generation overlay, Ctrl/keyboard semantics and existing Finish/Undo/Redo.
- All authored output remains **one stable target EntityId per unique accepted `MaterialEdgeReference`**, via the existing atomic command and Part v15. No Face/Surface token, region group, wire number or source selection gesture is persistent; no schema, Foundation/ADR, PG-01E, PM-06, `main` or issue #302 changes.

## 3. Separate linked-Profile problem (not silently conflated with Face Boundary)

The Owner's private `ProjectionTest-1(1).ss2part` has a successfully authored lower-Datum projection with four Lines, one Arc and one Circle (six individual semantic bindings). The model shows **both** upper and lower Datum supports are capable of projecting the **bottom** source surface; that observation explicitly supersedes the earlier suggestion that Datum itself failed.

Read-only analysis of those authored Sketch coordinates shows a visually closed outer Line/Arc loop whose computed Arc ends differ from adjacent Line endpoints by approximately `7.1e-15 mm` after trig evaluation. The exact Region analyzer may interpret those distinct floating-point endpoints as OPEN. This is a *diagnostic hypothesis*, not a verified native CTest result. **Never commit the Owner's private Part or its document IDs/data** to the repo.

A wholly synthetic analogous Line/Arc/Circle + inset hole regression must distinguish: valid exact shared source-vertex endpoints, only roundoff-level discrepancies induced by analytic Arc projection, and a genuinely open loop. Repair, if needed, should use **strictly proven same original material Vertex incidence** at the Part/Sketch projection boundary; no global tolerance relaxation, arbitrary snapping or modified Shared-2D region truth. If the proven source topology cannot be certified, stop and seek a further bounded architectural decision.

## 4. Execution and stop gates

**E0 / native evidence before Face Boundary implementation:** Real OCCT Body with two same-domain bounded Face fragments after successive Add/Cut and a nonplanar cylindrical bounded Face. Inspect provider-owned oriented Face/Edge incidence, strict Part surface meaning, internal partition classification and true outer/hole member source identity. A nonidentifiable boundary, missing material Edge member or irreconcilable group is **STOP**, not silently skipped.

**E1 / bounded implementation:** Only after E0, add narrow provider/Part read-only region membership and Workbench third mode; preserve current Planar Face without routing it through new logic. No second command or new authored schema.

**E2 / lifecycle:** Real native Workbench click tests of oblique planar, nonplanar, internal partition suppression, same-surface contiguous vs distinct real boundary, hole, manual dedup, exact geometry Partial, zero-supported, invalid provider/identity, visible per-source highlights, stale session, Finish one Undo/Redo, cold v15. The Owner's model remains **private manual evidence**.

**E3 / usability and Profile:** Increase linked Edge legibility in Shaded + Edges without hiding selection/role cues. Diagnose and regress exact source endpoints + arc/line/circle Profile closure on Datum; do not claim the existing File's Profile resolved unless tested. Update paired PL/EN Product, internal as-built and generated Browser only after the implementation is proven.

**E4 / gates:** Focused/FAST where justified, final immutable Windows FULL exact HEAD, Owner practical retest on private examples and separate explicit `PG-01D FINAL PASS` and merge approval. All earlier FULL evidence applies only to earlier commits. Keep PR #309 **Draft/Unmerged**, main unchanged, no new phase activation implied.

## Documentation Impact

Internal docs: required after implemented behavior is verified.
User/Product docs: required, bilingual PL/EN.
Browser: regenerate only with `.\\ss2.ps1 docs`; no hand-authored alternative output.
