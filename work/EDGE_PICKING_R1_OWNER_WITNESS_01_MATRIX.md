# Edge Picking R1 — Owner real-model witness 01

**Status:** real user model plus 3 screenshots of hard/impossible Body material Edge hover or selection, received 2026-10-11. Owner original is the authoritative source, not a new synthetic model. **PG-01E remains blocked.**

**Owner model:** Select_edge_test_01.ss2part; 1953 bytes; SHA-256 **183bcd9694481de5a5e020fa375fe615217dfea89b57530a20d02809da72c8ec**. Native Part container format simplesolid.native-document v1, schema **16**, modeling semantics **2**. Authored features are Extrude001 Add +20 mm, Extrude002 Cut reverse 10 mm, Extrude003 Cut reverse 8 mm, and Revolve004 Cut reverse 360 degrees; 3 Sketches and 4 Profiles. Copy unchanged as tests/fixtures/edge_picking_r1_select_edge_test_01.ss2part.

The Owner marked target edges with **cyan strokes**, not application selection feedback. Actual selected tool, pixel ratio, camera parameters, zoom, and precise symptom class per target were not provided. Do not invent them. Pixel regions below reference **cropped PNG image coordinates only**, not true viewport logical px, projected CAD points, world coordinates or durable topology IDs. Original screenshots are user-supplied attachments, not yet stored in this repository; refer to their filenames to match them in the conversation.

## Seven explicitly marked targets

| ID | Screenshot and dimensions | Cyan-marked target; approximate screenshot x/y region | What is reported | Required GREEN result |
| --- | --- | --- | --- | --- |
| **W01-A** | obraz(20261011-132750).png — 1203×1041 | Curved inner floor/wall Edge of the upper pocket; x210–566, y485–583 | Pocket hover/pick unreliable | Front-visible part acquires, stays while cursor remains in valid radius, click matches current material Edge |
| **W01-B** | Same image | Lower outer/front-right Body boundary; x626–926, y901–971 | External Edge sensitivity to camera rotation | Correct pick at this camera; no spurious occlusion or top/bottom screen bias |
| **W01-C** | obraz(20261011-132844).png — 1184×1032 | Sloping/curved inner pocket boundary; x350–617, y340–575 | Pocket hover/pick unreliable | Visible material segment selectable, no neighboring Face substitution |
| **W01-D** | Same image | Outer right vertical Edge; x972–1032, y475–854 | Orientation-dependent external difficulty | Stable hover and same-token click; genuinely hidden control still rejected |
| **W01-E** | Same image | Lower exterior horizontal/oblique Edge; x540–946, y925–991 | Orientation-dependent external difficulty | No dropout along actually visible interval |
| **W01-F** | obraz(20261011-132918).png — 1158×1063 | Inner left/pocket wall-to-floor near-vertical Edge; x475–555, y505–694 | Pocket hover/pick unreliable | Front-visible Edge selectable; ordinary stage/filter rules intact |
| **W01-G** | Same image | Curved/oblique outer right lower silhouette Edge; x815–1003, y640–1007 | Orientation-dependent external difficulty | Screen projection and depth correctly evaluated; no click-through |

**Unknown (record, do not guess):** tool for each screenshot (Select, Fillet, Chamfer or Project Geometry), whether symptom is never-hover / unstable-hover / hover-but-no-click / wrong target, DPR, exact camera and zoom, and which material Edge token corresponds to each cyan stroke. Those must be established by real native replay or separately clarified with Owner.

## Stepwise R1 witness gates

**N0 — actual native Part control:** load the exact unchanged fixture via PartDocumentStore (no old-v1 migration), evaluate with production OcctSolidModelingKernel, require up-to-date current solid/topology and a nonempty material Edge catalog / Body presentation. Report feature and topology counts. A passed container ZIP parse is not a kernel/GUI PASS.

**N1 — camera/Edge characterization on real model:** in native Windows Qt/OCCT, activate this Document, render current_body, iterate documented standard orthographic orientations and 3 zooms; optionally perspective. For each strict authorable Edge, probe points of its native presentation, record Body generation, projected screen position, candidate token(s), actual camera, DPI, real occlusion and screen side. Search the Owner-marked pockets and exterior boundaries without guessing OCCT ordinals.

**N2 — true RED:** obtain a *separately verified front-visible* authorable Owner Edge in a reproducible view that the current query fails to return on hover/click, including at least one pocket and one orientation-sensitive exterior attempt. Record pointer trace and failing exact candidate in a deterministic native test. Preserve fully hidden Edge and stale-generation negative controls. A miss without independent visibility proof is diagnostic only.

**N3 — GREEN:** after proved native RED, apply contracted projection/ray consistency, nearest-visible-portion search and calibrated spatial hysteresis, then verify hover/click **same token**, no through-wall picks and Owner test for W01-A..G. Distinguish modeling Fillet/Chamfer Finish defects from input picking.

**N4 — full gate:** capture baseline and post-fix p50/p95, at least 3 zooms and ortho/perspective, supported DPI modes 1/1.5/2 with unavailable modes reported accurately, exact-head FOCUSED/FAST/FULL and explicit Owner FINAL PASS + separate merge authorization.

## Evidence register

- Previous **synthetic** nearest-hidden-point native Qt/OCCT test: Windows [FOCUSED #2446](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38141703495), build PASS, **expected RED** on same-Edge nearest point hidden by a foreground triangle. This is proof of the selector's local rejected-closest-point flaw, **not** a real Cut-pocket witness.
- Owner file **ZIP/JSON parsed in this analysis**: native v16 and semantics v2. Real native OCCT evaluation / GUI replay are **PENDING** until specifically tested and logged. Owner images do not contain saved camera state.
- Do not alter original bytes, Part schema, modeling semantics, OCCT material history, Fillet/Chamfer geometry policy or public Viewer API during characterization.
