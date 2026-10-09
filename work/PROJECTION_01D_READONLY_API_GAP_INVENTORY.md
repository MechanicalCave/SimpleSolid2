# PG-01D — read-only interface/STOP inventory at merged PG-01C baseline

**Status:** DESIGN EVIDENCE ONLY — no production implementation or native test PASS claimed.  
**Examined baseline:** `main` `d928e8a5d1f5893b73de3e4fcd399d7e0483af83`.  
**Authority:** accepted PG-01D D2-F1–F6 design, pending separate Work Contract acceptance and activation.  
**Method:** read current repository source and existing native test registration. This is **not** a run of the OCCT Face-wire probe; F2 topological consistency still needs real RED→GREEN evidence.

## Existing reusable paths (confirmed from repository)

| Layer | Exact current source | What it already proves / exposes |
| --- | --- | --- |
| Kernel provider identity | `src/kernel/include/simplesolid2/kernel/edge_projection.hpp` — `IEdgeProjectionQuery::bindEdgeToBody`, `ScopedProjectionEdge` | A scoped Edge is tied to one exact runtime Body generation, not merely the numeric token; `projectEdgeToPlane` returns exact Line/Circle/Arc or typed failure. |
| Current OCCT inventory | `src/kernel_occt/solid_modeling_kernel.cpp::populateRuntimeTopologyInventory` | `TopExp::MapShapes(solid, TopAbs_FACE/EDGE/VERTEX)` produces exactly enumerated current runtime tokens. This does **not** describe per-Face outer/hole wire order. |
| Native provider type | `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp::OcctSolidModelingKernel` | Implements `ISolidModelingKernel` and `IEdgeProjectionQuery`. **No read-only provider-neutral `IFaceBoundaryQuery` exists yet.** |
| Part strict Face accounting | `src/part/include/simplesolid2/part/feature_evaluation.hpp::BodyFaceTopologyRecord`, `src/part/feature_evaluation.cpp` (Face catalog construction) | Runtime Face token, optional **strict** `semantic_address`, independent `surface_candidates`, accounting class. A Face may be `referenceable` through its Surface carrier while **lacking** a unique strict Face address. |
| Strict material Edge admission | `src/part/feature_evaluation.cpp::authorMaterialEdgeReference`, `BodyEdgeTopologyRecord` | Requires complete catalog, current source stage, uniquely resolved material Curve, rejects `periodic_seam`, `representation_partition` and multiple curve candidates. Reuse; do not duplicate this policy in Kernel/Qt. |
| Transient Face Viewer picking | `src/ui/part_viewport_controller.cpp::bodyTopologyAddressFor`, `primaryBodyTopologyInspection`, `setBodyTopologyFacePickOnly` | Pick is bound to current `BodyPresentationGeneration`; inspections distinguish Face/Edge and expose catalog status. **Not yet a strict Face admission transaction.** |
| Tool-stage routing | `src/ui/part_viewport_controller.cpp::bodyTopologyOrdinaryPickable`, `setBodyTopologyEdgeDraftMode` | Current tool-stage authoring is enabled via Edge draft mode; merely toggling `setBodyTopologyFacePickOnly(true)` does not establish one safe Face-mode source flow. |
| Existing Edges-only commit | `src/ui/cad_workbench.cpp::startProjectEdgeTool`, `tryStageProjectEdgeSelection`, `finishProjectEdgeTool`; `PartViewportController::selectedMaterialEdgeReferences` | One temporary selected-edge vector, stage/revision guards, exact preview, then one `CreateProjectedSketchEdgesCommand` at Finish. Selected Face currently causes `selectedMaterialEdgeReferences` to reject. |
| Native regression harness | `tests/pm05f_r2_native_workbench_edit_test.cpp`, `tests/CMakeLists.txt` `pg01c.native_project_edge_command` | Existing Windows Qt/OCCT fixture for whole Workbench; avoids creating a new test executable for initial coverage. Reuse native Kernel tests as appropriate. |

## Three proven source-level hazards (not hypotheses about future behavior)

1. **Strict Face is not equivalent to Surface carrier.** Face catalog classifies a record as referenceable if `surface_candidates` is nonempty; `semantic_address` may still be `nullopt`. Therefore D2-F1 admission **must explicitly check exactly one uniquely strict bounded Face meaning**, not simply `TopologyAccountingClass::referenceable`, `SketchSupportInspectionCapability::supported` or current `surface_address`. Boolean split/continuation provides a high-value negative fixture.
2. **Provider has no oriented wires as a typed read result.** Current `TopExp::MapShapes` captures unique Faces/Edges for a solid-wide inventory and discards oriented wire **uses**, outer-vs-hole meaning and boundary membership of the particular Face. Sorting the solid-wide Edge list or using the display outline would invent a perimeter. New bounded provider-native Face-wire interrogation is the correct D2-F2 gate.
3. **PG-01C source-selection logic assumes all picks are Edges.** `selectedMaterialEdgeReferences` fails for Face kind, while the Finish path compares it to staged Edge sources. D2 must freeze a single mixed Face/Edge staged source truth and recheck source Face contents at Finish, **not** merely switch a Viewer pick filter.

## No premature conclusions

- This inventory **does not prove** how OCCT will classify real cut-hole wires, face orientation, periodic seams or partition artifacts. Only an executable native topology proof can do so.
- This inventory **does not prove** that every planar Face selectable for Sketch support can be a uniquely resolvable bounded Face eligible for PG-01D. Carrier-only selection is expected to fail closed.
- `CurveKind::other` by itself is not enough to classify an Edge as safely skippable. D2-F3 geometric Unsupported requires **independently established strict semantic Edge identity**, even when exact projection does not support its curve. If the strict source cannot be authored, treat it as **integrity failure**, not a skip.
- Multiple uses of one provider Edge within a Face wire (for example a periodic representation seam) are not distinct material Edge sources; any unable-to-prove material membership is a typed block, not an invisible dedup.
- No evidence supports tolerance-based 2D loop healing, automatic Face membership refresh, a new persisted Face binding or a new schema; these remain excluded.

## Concrete next gate

With the Owner-accepted `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md`, after a dedicated `ACTIVE.yaml` activation is merged, run D0 **native OCCT Face-wire RED characterization** and Part strict bounded Face mapping first. STOP/Owner D2 if selected true material Face wire members cannot be mapped one-to-one to accepted semantic material Edge references. Only then implement UI staging/preview/Finish.

## Documentation impact

Internal as-built: not required (this is an evidence-only report, not changed runtime behavior).  
PL/EN Product: not required (no shipping UI change).  
Browser: not required (no canonical Product/as-built Markdown change).
