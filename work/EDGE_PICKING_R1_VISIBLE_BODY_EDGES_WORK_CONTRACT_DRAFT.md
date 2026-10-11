# Edge Picking R1 — visible Body material Edge hover/selection — DRAFT Work Contract

**Status:** DRAFT WORK CONTRACT — **NOT OWNER-APPROVED AS A CONTRACT; NOT ACTIVATED; NO PRODUCTION CODE MUTATION AUTHORIZED**.  
**Owner sequencing/scope decision:** 2026-10-11, in conversation: `akceptuję - kontynuuj`, answering the explicit proposal to make **Edge Picking R1 a mandatory standalone gate BEFORE PG-01E** and follow RED reproduction → projection/occlusion consistency → visible Edge fragments → 12/16 px and spatial hysteresis → native/FULL → Owner PASS. This acceptance approves **priority and investigation direction**, not an unreviewed algorithm or this detailed Work Contract.  
**Prepared:** 2026-10-11. **Base:** `main` `5bec84370dbfdba44c5cdda677aaaaf028456ed2`; PG-01D production [#309](https://github.com/MechanicalCave/SimpleSolid2/pull/309) and historical closure [#310](https://github.com/MechanicalCave/SimpleSolid2/pull/310) MERGED.  
**Relation:** planned PG-01E product-wide closeout design [proposal #311](https://github.com/MechanicalCave/SimpleSolid2/pull/311), not yet authorized. Edge Picking R1 MUST close with separate exact-head technical and Owner PASS BEFORE final PG-01E compound Fillet/Chamfer evaluation. PM-06 remains gated.  
**Authority:** `AGENTS.md`, FROZEN Constitution/Foundation v1.0, Architecture, ADR-0003/0010/0011/0014/0016/0017 as applicable, accepted Part-v1 roadmap v1.30 and prior PM-02J-R4/PM-05F and PG-01C/D picking contracts; `governance/DOCUMENTATION.md`.  
**Activation prerequisite:** first obtain Owner's **explicit acceptance of this exact Work Contract**. Then merge a distinct governance activation PR setting `work/ACTIVE.yaml -> active_work` to the accepted R1 contract on `main`. Open a fresh R1 production branch **from the activated main**, not from this draft branch. The currently referenced PG-01D contract is COMPLETED and grants no R1 production authority.

## 1. Owner problem, evidence, and hypothesis boundary

The Owner reports that current-view upper convex Body edges are comparatively easy to indicate, but lower-screen edges lose hover quickly; visible edges inside pockets are very difficult to acquire without zoom changes and repeated attempts; a larger detection area is desired. Upper/lower describe **screen orientation**, not fixed model axes.

**User-supplied audit:** `SimpleSolid2_propozycja_naprawy_hover_krawedzi_LLM(1).md`, dated 2026-10-09 and prepared against historical commit `83af8e62fabbe1056705b255ef5a0d6256c2a6e0`. It is a technical proposal, not a native reproduction or automatically adopted specification. Current `main` static source review reconfirmed:
- `src/viewer_qt_occt/qt_occt_viewer_widget.cpp`: `projectToScreen()` uses `V3d_View::Convert(..., int&,int&)` and therefore loses subpixel screen precision; `bodyPickRayAtPhysical()` rounds to integers before `ConvertWithProj()`; `bodyPointFrontVisible()` compares one sampled point's ray depth against closest scene triangle with a very small tolerance; `queryBodyTopology()` for material, `ordinary_pickable` Edges uses **8 logical px** and checks visibility **only after selecting ONE globally nearest projected point per Edge**.
- `src/ui/part_viewport_controller.cpp::onBodyTopologyPreselectionIntent()`: an empty current candidate list clears hover immediately; current 4-px neighborhood preserves the candidate *cycle index* only when token list/order match, **not physical enter/leave hysteresis**.
- `pickAtLogicalPoint()` and `emitBodyTopologyPreselectionAt()` share the `queryBodyTopology()` mechanism, but click re-queries and normal AIS-overlap routing differs from deliberate Fillet/Chamfer **Body Edge-only** picking.
- Existing `sk04b.viewer_native_selection_query` includes native Qt/OCCT circular ring, two zooms, hidden rear/bottom ring, and overlapping Sketch/Profile/Origin edge-only routing checks. These do **not** yet prove the reported complex pocket, subpixel, partially visible segment, 12–16 px hysteresis, perspective parity or end-to-end Owner case.

**Hypothesis, NOT a proven cause:** round-trip pixel quantization can offset the occlusion ray from the tested 3D point, causing a nearby pocket wall to become a false blocker; checking one closest point can reject an Edge whose other fragment within the aperture is visible. Independent Face/Edge tessellation, perspective interpolation, pick routing and candidate ranking may also contribute. Native RED evidence must distinguish them. **Do not advertise a root cause or green without measured repro.**

## 2. Goal and product invariant

Deliver reliable, stable hovering and clicking of actually visible **current material Body Edges** in ordinary Part and explicit Fillet/Chamfer and Project Geometry Edge acquisition, especially pockets, lower-screen edges and curved Chamfer/Fillet intersections, **without** allowing picks through occluding Body material, breaking cycle selection or masking genuinely stale/ambiguous topology.

The Viewer owns pixel geometry, current-scene visibility and transient candidate ranking. Part/Application remain authoritative for legal stage, material/source semantics and authorability. Runtime Body tokens are scene-generation-bound and nonpersistent. No screen/camera/OCCT token becomes durable CAD identity. A hover/selection must never mutate the document except through existing validated semantic command paths.

## 3. In scope and algorithm gates

**R0 — RED characterization BEFORE any production fix.**
1. Reproduce and record Owner symptoms on deterministic native Windows Qt/OCCT in a Body with an actual *visible concave pocket*, a lower-screen edge after camera rotation, and an oblique/filleted edge. Capture source-stage generation, view projection, screen cursor path in *logical px*, physical DPI ratio, zoom/camera state and pixel/barycentric depth diagnostics as necessary.
2. Distinguish **out of aperture**, **actual occlusion**, **false occlusion at offset screen ray**, **only-nearest-point rejection**, **nonmaterial/ordinary_pickable filter**, **stale token/generation**, **AIS routing**, and **priority/cycle** reasons. Instrument behind a test-only or disabled-by-default bounded diagnostic path; no unbounded production logging, UI debug panel or telemetry service.
3. Add a deterministic **native RED regression** in the existing appropriate test executable(s) that fails for the real observed mechanism and passable controls. If the expected RED cannot be reproduced, **STOP**: report observation and ask Owner about a new model/camera capture instead of asserting the audit diagnosis. Preserve anti-pick-through positive/negative control before modifying code.

**R1-A — projection/visibility correctness.**
- Preserve double screen precision for Body Edge candidate projection. Determine the **actual installed OCCT** camera/matrix/Convert APIs from repository headers/toolchain; never invent overload signatures. If this requires an otherwise public Viewer contract change, STOP for Owner D2.
- A tested 3D candidate, its projected physical-pixel location and visibility ray must be mathematically consistent; in perspective, originate at camera eye with the corresponding 3D direction; in ortho, use the correct parallel ray with near/far/body occluders in front. Respect viewport origin, Y inversion, near/far clipping, DPR and correct depth sign.
- Do not linearly interpolate an Edge 3D point using a projected 2D parameter in perspective; either use perspective-correct interpolation or a verified coherent world-space method, with native test witnesses.
- Depth tolerance must account for measured numerical and independent triangulation error, **not** hide an entire neighboring wall. Near-silhouette no-triangle-hit handling and tessellation gaps must retain explicit non-see-through negatives; no "always visible" or blanket neighboring-Face ignore.
- Keep any shared helper change narrow; establish whether Vertex and other non-Body consumers need their own unchanged semantics.

**R1-B — nearest *visible* Edge portion.**
- Filter candidate segments by a bounded screen-space aperture; find the nearest *verified visible* point on a material Edge, not simply a globally nearest (possibly hidden) point.
- When the closest point on a segment is occluded, assess the **remaining part within the aperture** (including the same segment); a visible fragment can remain selectable. Use adaptive, spatially bounded sampling/refinement or a demonstrably equivalent deterministic bounded method.
- Set and test a finite worst-case work/sample budget, explicit minimum resolvable visible-piece limitation and termination rule. Do not approximate or author CAD Curve topology from rendering triangles, proximity or sampling.
- Fully hidden rear/side/bottom Edges remain unselectable. No phantom result from a representation partition, seam, nonmaterial Edge or `diagnostic_prefix`.

**R1-C — reach and spatial hysteresis.**
- **Initial tunable starting values, not predeclared shipped constants:** acquisition radius **12 logical px**, incumbent retention radius **16 logical px**, automatic same-class Edge switch only when another candidate is at least **2 logical px nearer** (or when incumbent fails validity). Compare calibrated results to existing 8-px baseline.
- 12–16 px retention applies **only to the freshly revalidated same current Edge**; never expands acquisition for all Edges. Recheck at each hover/click with current camera, Body scene generation, filter, viewport, stage, projection, and occlusion. No timer-only or sticky cache masquerading as visibility.
- Preserve vertex/face/edge pick priority, intentional cycle selection order and explicit Fillet/Chamfer **edge-only** AIS exception. Project Geometry Planar Face/Face Boundary Face gesture remains pickable by its existing stage rules; do not globally reassign priority to Edges.
- A click on the currently highlighted and *still-valid* Edge in retention band must use the **same token** as hover. Share a bounded decision or revalidation path; do not allow a hover at 15 px to become an unselectable 12-px click. If incumbent is genuinely occluded, stale or filter-ineligible, clear and do not pick it.
- Camera motion, wheel zoom, DPR/resize, scene generation, document, mode/tool, canceled draft, provider swap, focus/viewport leave or missing Body must invalidate/revalidate transient candidates by explicit rules. A move of the cursor beyond the exit radius removes hover; no red "last good" overlay.

**R1-D — bounded performance and visibility safety.**
- First cull by viewport-projected Edge bounding region and actual cursor aperture; measure triangle/edge count and query costs. `nearestBodyTriangleHit()` currently linearly checks triangles; increased visibility samples may make the hot path costly.
- Record before/after **p50 and p95** native hover query latency over deterministic movement traces at several Body complexities, zooms and projection modes. Use real Windows machine/build, per-run sample counts, consistent Qt/OCCT settings and documented measurement method; include memory/resource growth for long hover sessions when meaningful.
- A spatial acceleration structure (BVH) or broader Viewer engine restructuring is **NOT preauthorized**; if bounded sampling misses performance suitability, STOP with evidence for separate amendment rather than adding speculative infrastructure.
- Do not treat cosmetic 12/16 values as final until measured; if different params needed, document reproducible evidence and request Owner confirmation of the UI behavior.

## 4. Exclusions and Owner STOP boundaries

Outside R1: Part/Kernel B-Rep geometry algorithms, OCCT boolean Refine, Cut/Fillet/Chamfer semantics or preview, tolerance/healing of geometry, durable `MaterialEdgeReference`/Face/Point identity, native Part schema **v16**, **ModelingSemanticsVersion 2**, persistence/migration, Part Feature/Sketch/Profile lifecycle architecture, new global CAD-input dispatcher, redesigned selection semantics for all layers, assembly/drawing, performance budget freeze for PM-06, separate CI issue #302, and PG-01E completion work.

Do **not** modify `governance/CONSTITUTION.md`, `governance/FOUNDATION.md`, accepted ADRs, PM-05 or PG-01D accepted contracts, or `main` directly. Public Viewer/provider-neutral API, subsystem ownership, broader detection/priority behavior, new durable reference or CMake/CI machinery beyond a clearly justified existing native test hook are **D2 STOP** until a separate Owner approval.

A problem exposed by selecting a correct, visible Edge that makes **Fillet/Chamfer Finish fail** must be documented as a separate, reproducible modeling defect for the later PG-01E accepted plan; it must **not** be silently fixed as part of R1.

## 5. Bounded permissible modules — only AFTER separate formal activation

| Existing path or module | Specific authorized use | Hard boundary |
| --- | --- | --- |
| `src/viewer_qt_occt/qt_occt_viewer_widget.cpp` | Current Body query, high-precision projection/ray, visible-fragment evaluation, bounded screen culling, edge-only routing | No global selection/occlusion bypass, OCCT B-Rep modeling, runtime pick identity persistence or broad new native selector |
| `src/ui/part_viewport_controller.cpp` and its existing private header | Candidate stack retention, revalidation, click/hover same-token choice and invalidation on camera/tool/stage changes | No new Part semantic authority; no unbounded UI state machine |
| `src/viewer/**` and `src/viewer_qt_occt/include/**` | **Read-only inspection** or existing contract-preserving local use; request Owner D2 before changing public interfaces | No new default global pick priority, platform semantic ownership, or public contract changes |
| `tests/sk04b_viewer_native_selection_query_test.cpp` | Native Qt/OCCT RED and GREEN camera/DPR/pocket, partial visibility and occlusion tests | Synthetic BodyScene witnesses must be distinguished from a real modeled OCCT/Part Body |
| `tests/sk04b_part_viewport_selection_bridge_test.cpp` | Existing mock-friendly controller stack/cycle/invalidation tests where useful | Mocks alone cannot prove geometric occlusion |
| `tests/sk04b_viewer_selection_query_contracts_test.cpp` | Existing neutral query contracts, only if genuinely needed | No unnecessary contract change |
| `tests/pm05f_r2_native_workbench_edit_test.cpp` | **One or more native end-to-end** Fillet/Chamfer and Project Geometry stage/selection/hover–click regressions using real OCCT/Part geometry | No production edge Feature algorithm edits in R1 |
| `tests/**` existing appropriate targets first | Regression additions tied to concrete RED witness and Owner cases | New test target/CMake registration requires narrowly justified Owner scope check; issue #302 remains independent |
| `docs/internal/**`, `docs/product/pl/**`, `docs/product/en/**`, `docs/browser/index.html`, `work/**` | Current-state behavior, acceptance notes, deterministic generated Browser | Never edit Browser independently of `.\ss2.ps1 docs`; documentation impact applies |

Allowed production files are **not** activated by appearing in this draft; public interfaces and out-of-scope modules remain STOP.

## 6. Required scenario matrix and acceptance

| ID | Native witness | Minimum PASS/negative proof |
| --- | --- | --- |
| P01 | Convex Body edge rotated to screen top/bottom | Same detectable material Edge at matching relative physical geometry, no top-vs-bottom special rule |
| P02 | **Real modeled** Cut pocket with visible bottom boundary and vertical edges | No false hover dropout on scripted continuous pointer path while visible and inside retention radius |
| P03 | Edge partly covered by closer wall; **hidden nearest point**, visible same-segment subportion within aperture | Exact current material token offered; fully occluded control offers **none** |
| P04 | Deep cavity, thin wall and truly covered bottom/back edge | **Zero see-through picks** including when incumbent existed before occlusion |
| P05 | Curved Revolve/Fillet/Chamfer edges with differing Face/Edge tessellation | Screen/ray alignment, supported candidate available and no hidden rear-ring regression |
| P06 | Orthographic **and perspective**, camera rotations and at least 3 zoom levels | Correct depth convention and projected/world interpolation; no direction or clipping errors |
| P07 | DPR **1.0, 1.5, 2.0** where test Windows runner truly supports each | Acquisition/retention measured in **logical px**, equivalent user reach; unavailable DPI modes explicitly reported, not fabricated PASS |
| P08 | 12-px acquisition, 16-px incumbent, 2-px competitor | No unintended dropout or switch within retention; new Edge not acquired through 16-px incumbent zone by enlarging general radius |
| P09 | Pointer hover at 12–16 px then click at unchanged scene/camera | **100% same eligible token** (and tested current generation), including deliberate candidate cycle |
| P10 | Sketch/Profile/Origin overlapping Body | Ordinary AIS routing retained; active explicit Fillet/Chamfer edge-only mode prioritizes current visible material Body Edge |
| P11 | Project Geometry Edges + Planar Face + Face Boundary with stage-restricted source | Edge-pick remains usable without rewriting Face admission or stage semantics |
| P12 | Document/tool switch, zoom/orbit/resize, viewport exit, generation/provider change, `diagnostic_prefix` | No stale hover/click or prohibited picker targets; correct `tool_stage` |
| P13 | Multi-edge Fillet/Chamfer Owner-style complex Body selections | Every supported current edge is individually hoverable and click-selectable; R1 does **not** claim Feature Finish PASS |
| P14 | Performance on simple/complex scene, repeated motions | Report measured baseline vs changed **p50/p95**, number of edges/triangles/rays and bounded worst-case query work; explicit STOP if unacceptable |

**Owner acceptance:** Owner supplies or recreates representative pocket + complex Fillet/Chamfer edge-picking workflow on a **new v2 Part**. A manual aggregate PASS is required after exact-head automated FULL, even if all scripted witnesses pass. No fabricated per-row manual result.

## 7. Execution sequence and immutable gate

| Gate | Required action | Exit |
| --- | --- | --- |
| G0 | After formal Work Contract acceptance, separate governance activation PR/closure on `main`; fresh R1 feature branch | Approved `ACTIVE.yaml` points to R1 contract, activation CI PASS and merge |
| G1 | Establish native **RED** on actual current Body pocket + partial hidden Edge; record occlusion/camera/diagnostics and baseline P04 negatives | Unambiguous evidence or **STOP Owner** if unreproducible |
| G2 | Correct projection and same-ray occlusion consistency, rerun RED and original negatives | Determine if issue resolved or visible-fragment stage still necessary |
| G3 | Implement nearest visible fragment with finite work budget and strict nonmaterial filtering; regress partial and wholly hidden edges | P02–P06 native PASS without see-through |
| G4 | Introduce tested 12/16/2 starting settings and true spatial hysteresis; share click/hover revalidation and preserve cycle/routing | P01/P07–P12 PASS |
| G5 | Measure p50/p95 and long-session behavior; bound any extra work, native real Workbench Fillet/Chamfer/Project Geometry smoke | P13–P14 evidence, no unapproved API or perf redesign |
| G6 | Actual PL/EN/internal docs impact updates, deterministic Browser regeneration; rebuilt FOCUSED/FAST where applicable then **one immutable exact-head Windows FULL**: Core + native Kernel + Desktop + docs + aggregate | No RED, tests never disabled; record exact SHA/job counts |
| G7 | Owner supported-Windows practical test and **R1 FINAL PASS**, followed by a **distinct explicit merge approval** | Only then guarded squash merge; close R1 separately; PG-01E may proceed to its own unapproved proposal/contract gates |

**Stop conditions:** true click through a wall; missed visible pocket Edge in an agreed deterministic trace; hover != click for valid incumbent; stale or nonmaterial current-stage selection; repeated test failures; unbounded performance growth; inability to reproduce RED, test under supported Windows/DPR/camera modes, or out-of-scope contract expansion. Retain failed evidence and seek Owner D2 before changing policy.

**Existing suite:** re-run current `sk04b.viewer_native_selection_query`, `sk04b.part_viewport_selection_bridge`, relevant `pm05f_r2.native_workbench_edit` and `pg01d.native_face_ui` or counterpart, all with rebuilt exact test executables. Final FULL includes unrelated regressions and docs. Independent stale FAST CTest-binary issue #302 is not hidden or suppressed.

## 8. Product documentation impact

**Internal docs: required** — actual selected-edge detection, front-occlusion correctness, hover–click revalidation and candidate state are meaningful current-as-built behavior; update the relevant existing `docs/internal/**` after implementation.  
**User/Product docs: required** — usable material-Edge selection and UI hover/click behavior are user-facing; update *existing* relevant `docs/product/pl/**` AND `docs/product/en/**` with concise supported behavior/limitations; do not add an unsupported guarantee to select through occlusion.  
**Generated Browser: required** — regenerate deterministically via `.\ss2.ps1 docs` from canonical Markdown; verify PL/EN IDs and order and offline Browser freshness.  
**This DRAFT alone:** no implementation or changed user behavior; no current-as-built/product docs update, Browser generation, or model migration authorized.

## 9. Owner decisions still OPEN

1. **Approve this exact Work Contract** (scope/allowed files, G0–G7, hard STOP, tests, documentation impact), not merely the already accepted R1 precedence and study plan.
2. On acceptance: submit and merge **separate R1 governance activation** of `work/ACTIVE.yaml` on `main` before production mutation.
3. Distinct later gates remain: Owner R1 manual FINAL PASS; R1 merge; subsequent review/acceptance and governance activation for PG-01E; separate PM-06 Work Contract.
