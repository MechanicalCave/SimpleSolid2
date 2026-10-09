# PG-01D D0 — native Face wire evidence and SS2 strict semantic stop gate

**Status:** D0 CORE NATIVE EVIDENCE PASS (synthetic external OCCT + compiled SS2 through-Cut); bounded D1 scoped Face-query POSITIVE PASS #2123; D1 additional negatives/Face-split proof and later Face UI remain OPEN. Not PG-01D final acceptance.  
**Owner approval:** D2-F1–F6 accepted and Work Contract accepted 2026-10-09.  
**Governance:** Work Contract `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md`; isolated activation #308 merged on `main` `5eeb4095e628dc0f204b284cf56f560c5448c36a`.  
**Production branch:** `feature/pg01d-planar-face-native-d0-20261009`, based on activated `main`.  
**Private CAD:** No original Owner Part file or its identifiers/coordinates are used in this evidence.

## Independent local native OCCT 7.9.3 procedure (NOT a compiled SS2 test)

Run in the current container environment with `cadquery` and `OCP` (OCCT 7.9.3 Python bindings); create a procedural 80 × 60 × 12 solid, cut one or two through-cylinders. Enumerate the exact `TopoDS_Face` realizations via `TopExp_Explorer(TopAbs_FACE)`, restrict to `GeomAbs_Plane` via `BRepAdaptor_Surface`, enumerate the selected Face's `TopoDS_Wire` uses, mark its authoritative outer loop via `BRepTools::OuterWire` and walk each wire with `BRepTools_WireExplorer(wire,face)`. For **every** Edge occurrence, find its source among the solid-wide unique `TopoDS_Edge` identities using exact `TopoDS_Shape::IsSame`. Fail if zero or more than one global Edge identity matches. No endpoint closeness, mesh-outline inference, name/coordinate sorting or tolerance relaxation.

| Procedural case | Unique solid Edge identities | Planar Faces | Oriented planar Face wire Edge uses | Planar Faces with hole wires | Actual boundary of each holed planar Face |
| --- | ---: | ---: | ---: | ---: | --- |
| 1 through-hole | 15 | 6 | 26 | 2 | outer: 4 exact Lines; inner: 1 exact Circle |
| 2 through-holes | 18 | 6 | 28 | 2 | outer: 4 exact Lines; inner: 2 distinct exact Circles |

Both `TopAbs_FORWARD` and `TopAbs_REVERSED` planar material Face orientations were observed. For each holed Face, exactly **one** `BRepTools::OuterWire` matched, and **every native oriented wire Edge occurrence** matched exactly **one** solid-wide Edge identity (by `IsSame`, not numeric index or tolerance). Reversing the Face did not make the holes disappear.

**This confirms a feasible OCCT-native Face-wire query mechanism; it is not yet SS2 runtime/provider semantics PASS.** External OCP version and native SS2 Windows OCCT binding may differ. Successful native topological source matching does not establish that **Part** can durably author a strict `MaterialEdgeReference` for each returned member.

## Source-level SS2 baseline findings to verify with actual native test

- `src/kernel_occt/solid_modeling_kernel.cpp::populateRuntimeTopologyInventory` already owns runtime `TopoDS_Face/TopoDS_Edge/TopoDS_Vertex` inventory and exact scoped runtime tokens, but emits **no per-Face outer/hole wire use table**.
- `src/part/include/simplesolid2/part/feature_evaluation.hpp::BodyFaceTopologyRecord` keeps `semantic_address` (**strict bounded Face identity**) separately from `surface_candidates` (**carrier identity**). A selectable or supportable planar Surface is not proof of strict Face uniqueness.
- `src/part/feature_evaluation.cpp::authorMaterialEdgeReference` already rejects `periodic_seam`, `representation_partition`, non-referenceable/multi-candidate curves, incomplete stage catalog; any D0 probe must exercise the **same** authoring function before classifying a native occurrence as admissible material Edge.
- `src/ui/part_viewport_controller.cpp::selectedMaterialEdgeReferences` accepts only Edge-kind selections and must not be repurposed to infer a Face's hidden perimeter.

## In-repository native characterization (test authored; Windows execution pending)

The activated PG-01D production branch adds `verifyPg01dNativeStrictFaceAndMaterialCatalog` to `tests/pm05f_r2_native_workbench_edit_test.cpp`. The fixture constructs a real Extrude Add rectangle and a circular one-sided **through Cut**, evaluates the exact current `BodyStageTopologyCatalog`, counts **strict bounded Face IDs separately from Surface-only carrier occurrences**, checks current material Edges *only* via `part::authorMaterialEdgeReference`, verifies per-stage binding and unique semantic references, and prints `PG01D_D0_NATIVE_STRICT_FACE_CATALOG_PASS`.

CI FAST #2117 **PASS** and Windows native FOCUSED #2118 **PASS 1/1** on exact branch head `6adf273c2fa1c9f274324f0ea7c6997bbf949b17`, using existing `pg01c.native_project_edge_command`. This establishes the compiled SS2 native strict Face/material Edge **catalog characterization** only. The present D1 candidate adds a separate scoped, provider-neutral `IFaceBoundaryQuery` and an **unverified** real Cut Face-wire→strict material Edge test. The new D1 FOCUSED gate has not yet passed; do not conflate its result with #2118.

## D1 read-only native provider candidate — source authored, RED→GREEN pending

The provider now owns an optional, revision-scoped `IFaceBoundaryQuery` seam in `src/kernel/include/simplesolid2/kernel/face_boundary.hpp` and `src/kernel_occt/solid_modeling_kernel.cpp`. It requires an **exact same current RuntimeSolidHandle** and current runtime Face token, checks planar Face geometry, reads actual oriented `TopoDS_Wire` uses via `BRepTools::OuterWire` and `BRepTools_WireExplorer`, validates full native wire-use counts, and maps each use to exactly one current provider Edge token by `TopoDS_Shape::IsSame`. It does not author Sketch objects or certify semantic material Edge identity.

The new native Workbench test cross-checks the query against `BodyStageTopologyCatalog` and calls **only** `authorMaterialEdgeReference` for each candidate. It requires a real through-Cut holed Face with strict bounded Face address and an entire hole loop of supported material Edges; if those are absent, CI will remain **RED** and PG-01D Face UI implementation must STOP or request bounded Owner D2. A mismatched provider generation must report `provider_mismatch`.

**Exact-head native result:** [Windows FOCUSED #2123](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37983190239) **PASS 1/1**, tested commit `ba4b671031278b8a9e52411546454691e81ade28` and existing `pg01c.native_project_edge_command` 5.75 seconds. Its test requires:
- a true SS2 current provider Cut hole with **at least two** planar Face realizations containing separate outer and inner wires;
- **at least one** strictly semantically addressed bounded Face with a complete, uniquely material `MaterialEdgeReference` for every native outer/hole wire Edge use;
- unique runtime provider Edge token mapping; no duplicate stored material references;
- stale scoped Face token used against a *different* `RuntimeSolidHandle` yields `provider_mismatch`, not accidental retargeting.

**This proves one realistic admissible source family, not all possible Face/periodic/seam/split cases.** The subsequent test commit also checks invalid/default scope and nonplanar Surface typed rejection; its own FOCUSED result is pending. Exact current-source geometric Unsupported skips, mixed-source authoring, Face UI and cold persistence are still unimplemented.


## Mandatory remaining D0 tests / STOP

1. Run a **compiled native Windows SS2 OCCT** fixture building a real through-Cut and inspect `BodyStageTopologyCatalog`: exact strict material Face address (not only Surface carrier), complete Face/Edge accounting, and deterministic after-Feature stage.
2. After introducing a *read-only* scoped Face-boundary query at the separately approved D1 seam, assert each returned oriented Face-wire Edge occurrence maps to **exactly one** strict same-stage `MaterialEdgeReference`; reject carrier-only/ambiguous Face and nonmaterial/partition/seam Edge. Preserve different semantic Edge identities despite overlapping geometric carrier.
3. Probe reversed Face, real hole loops, separated Face realizations from one Surface, malformed provider token, wrong RuntimeSolidHandle and source/provider loss. They must fail explicitly rather than secretly rebound.
4. D0/D1 RED if the source Face is carrier-only or native Face wires and Part semantic material Edge catalog cannot be reconciled. **STOP Owner D2** before exposing the Face mode in the Workbench.
5. Only after real RED→GREEN proceed with PG-01D D2 UI, then focused/FAST/exact-head Windows FULL and Owner manual PASS. No work in PG-01E/PM-06/CI #302.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This is a **read-only D0 evidence report**, not a change to shipped behavior or public Kernel/Part semantics. Later production D1/D2 requires current internal as-built and bilingual PL/EN Product docs per the accepted Work Contract.
