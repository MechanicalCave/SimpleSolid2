# PG-01D D0 — native Face wire evidence and SS2 strict semantic stop gate

**Status:** D0 source evidence and bounded D1 strict planar Face wire→material Edge adapter validated on current synthetic SS2 Cut-hole and known split/partition cases; **PG-01D D2 Workbench Face UX, broader lifecycle and final Owner PASS are NOT DONE**.  
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

## In-repository native characterization (compiled and verified Windows)

The activated PG-01D production branch adds `verifyPg01dNativeStrictFaceAndMaterialCatalog` to `tests/pm05f_r2_native_workbench_edit_test.cpp`. The fixture constructs a real Extrude Add rectangle and a circular one-sided **through Cut**, evaluates the exact current `BodyStageTopologyCatalog`, counts **strict bounded Face IDs separately from Surface-only carrier occurrences**, checks current material Edges *only* via `part::authorMaterialEdgeReference`, verifies per-stage binding and unique semantic references, and prints `PG01D_D0_NATIVE_STRICT_FACE_CATALOG_PASS`.

CI FAST #2117 **PASS** and Windows native FOCUSED #2118 **PASS 1/1** on exact branch head `6adf273c2fa1c9f274324f0ea7c6997bbf949b17`, using existing `pg01c.native_project_edge_command`. This establishes the compiled SS2 native strict Face/material Edge **catalog characterization** only. The present D1 candidate adds a separate scoped, provider-neutral `IFaceBoundaryQuery` and an **unverified** real Cut Face-wire→strict material Edge test. The new D1 FOCUSED gate has not yet passed; do not conflate its result with #2118.

## D1 native provider + bounded strict Part admission — verified current cases

The provider now owns an optional, revision-scoped `IFaceBoundaryQuery` seam in `src/kernel/include/simplesolid2/kernel/face_boundary.hpp` and `src/kernel_occt/solid_modeling_kernel.cpp`. It requires an **exact same current RuntimeSolidHandle** and current runtime Face token, checks planar Face geometry, reads actual oriented `TopoDS_Wire` uses via `BRepTools::OuterWire` and `BRepTools_WireExplorer`, validates full native wire-use counts, and maps each use to exactly one current provider Edge token by `TopoDS_Shape::IsSame`. It does not author Sketch objects or certify semantic material Edge identity.

The new native Workbench test cross-checks the query against `BodyStageTopologyCatalog` and calls **only** `authorMaterialEdgeReference` for each candidate. It requires a real through-Cut holed Face with strict bounded Face address and an entire hole loop of supported material Edges; if those are absent, CI will remain **RED** and PG-01D Face UI implementation must STOP or request bounded Owner D2. A mismatched provider generation must report `provider_mismatch`.

**Exact-head native result:** [Windows FOCUSED #2123](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37983190239) **PASS 1/1**, tested commit `ba4b671031278b8a9e52411546454691e81ade28` and existing `pg01c.native_project_edge_command` 5.75 seconds. Its test requires:
- a true SS2 current provider Cut hole with **at least two** planar Face realizations containing separate outer and inner wires;
- **at least one** strictly semantically addressed bounded Face with a complete, uniquely material `MaterialEdgeReference` for every native outer/hole wire Edge use;
- unique runtime provider Edge token mapping; no duplicate stored material references;
- stale scoped Face token used against a *different* `RuntimeSolidHandle` yields `provider_mismatch`, not accidental retargeting.

**This proves one realistic admissible source family, not all possible Face/periodic/seam/split cases.** The next native [Windows FOCUSED #2125](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37983494685) also completed **PASS 1/1** on exact tested commit `7480496e576828ea751ecf49f621a9108c273fc8`, verifying invalid/default scoped Face, invalid token, nonplanar Face typed rejection and cross-generation provider mismatch. The existing PG-01C native Project Geometry regression stayed green. **Only this D0/D1 synthetic through-Cut source family is certified**, not general Face membership changes or finished UI. Exact current-source geometric Unsupported skips, mixed-source authoring, Face UI and cold persistence are still unimplemented.


## Consolidated RED→GREEN validation through D1 — verified checkpoints

- [Windows FULL #2130](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37984994619), tested SHA `8010fce47be62b7aba27de8cae43f94bdc83e78e`: **PASS 197/197** (Core 25/25, Kernel-native 57/57, Desktop 115/115). Includes isolated `pg01d.native_face_boundary_d0`, legacy PG-01C Cursor UI and full Workbench tests. Earlier FULL #2129 was **RED in CMake CI-04 registry**, fixed by mapping the new D0 test to its already existing executable; no CI classifier alteration.
- Windows FOCUSED #2127 on SHA `148989cc367bcdc6468f506cd1069c1a42044e88`: initial **RED** at unrelated historical `pg01c.native_project_edge_command` GUI Finish assertion after the new **two Cut holes native proof passed** (2 holed planar Face realizations; 2 strict and fully material, 2 rejected seams). Same SHA/job retry **PASS** without code edits; the GUI flake root cause is **not** claimed fixed. A separate native D0 CTest was then registered.
- Windows [FOCUSED #2131](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37987205210): **RED** on first split Face fixture because an invalid test expectation confused an ambiguous **Surface carrier** with nonexistence of independent unique **bounded Face addresses**. **No production semantic behavior changed.** Corrected test confirms unique Face addresses coexisting with one semantically resolved Surface that cannot itself select one Face.
- [Windows FOCUSED #2132](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37987428809), SHA `fd403d0a668026ca81889a765f59c9d6040ed3c5`: **PASS 1/1** in the existing real native OCCT `pm02jr2.add_surface_continuation` fixture. Every current bounded Face has its own native wire query; representation partition/seam boundary uses fail `authorMaterialEdgeReference`; distinct strict Face addresses are not merged simply because the Surface carrier is shared.
- [Windows FOCUSED #2136](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37987955818), SHA `796d1018c37e296083f3219d9a7dbeb32aae0cae`: **PASS 1/1** for `part::inspectMaterialFaceBoundary` on the paired current `FeatureEvaluation` Body/Topology. A semantic Face with any nonmaterial native wire occurrence yields a **whole-Face rejection**; invalid/default Face and missing source Body stage fail closed. No partial material Edge list is exposed.
- [Windows FOCUSED #2137](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/37988274068), SHA `3a418b86ff62a6f59ed48a5504b9b13502210072`: **PASS 1/1** for a real SS2 current two-through-Cut case: at least one strict bounded Face with all **three oriented wires** (outer four Lines + two distinct one-Circle hole wires; six source Edge occurrences) admitted by the Part adapter, with exact provider token/orientation and same-stage durable `MaterialEdgeReference` equality. A single real Face is never inferred from the mere number of Surface-carrier candidates.

**Meaning:** native read-only topology interrogation and strictly fail-closed Part material admission are proven in these bounded SS2 fixtures. Neither the transient Face-picked Workbench UX nor geometrically-Unsupported-only partial Finish is implemented/tested. No new persistence, tolerance repair or semantically unsafe Face rebind was introduced.

## Remaining D1 variations and D2/D3/D4 STOP

1. The compiled synthetic through-Cut and real Surface-continuation split now pass. Further negative variants for unsupported geometric curve **after** strict semantic identity, suppressed source/provider loss, malformed native wire accounting and changing Face membership remain necessary, with RED→GREEN as appropriate. Do not infer an all-conditions guarantee from one split fixture.
2. Reversed Face orientation, true hole loops and wrong provider generation are already represented in the native/independent OCCT probes. Preserve typed Unsupported vs semantic identity integrity failure; no guessing.
3. For Workbench D2: use exactly one existing Project Geometry tool, source Edges/Planar Face, selected Face from current stage, controlled staging/preview, typed partial Unsupported warnings, single Finish/Undo, strict revision/member freshness, and no saved Face token.
4. PG-01D D3/D4: old PG-01C correctness, cold Save/Reopen, paired PL/EN Product and current internal as-built docs + generated Browser, exact-head Windows FULL and separate **Owner practical FINAL PASS**. PR #309 stays **DRAFT/UNMERGED** pending all gates. Do not start PG-01E/PM-06/CI #302.

## Historical initial D0 STOP checklist (retained for traceability)

1. Run a **compiled native Windows SS2 OCCT** fixture building a real through-Cut and inspect `BodyStageTopologyCatalog`: exact strict material Face address (not only Surface carrier), complete Face/Edge accounting, and deterministic after-Feature stage.
2. After introducing a *read-only* scoped Face-boundary query at the separately approved D1 seam, assert each returned oriented Face-wire Edge occurrence maps to **exactly one** strict same-stage `MaterialEdgeReference`; reject carrier-only/ambiguous Face and nonmaterial/partition/seam Edge. Preserve different semantic Edge identities despite overlapping geometric carrier.
3. Probe reversed Face, real hole loops, separated Face realizations from one Surface, malformed provider token, wrong RuntimeSolidHandle and source/provider loss. They must fail explicitly rather than secretly rebound.
4. D0/D1 RED if the source Face is carrier-only or native Face wires and Part semantic material Edge catalog cannot be reconciled. **STOP Owner D2** before exposing the Face mode in the Workbench.
5. Only after real RED→GREEN proceed with PG-01D D2 UI, then focused/FAST/exact-head Windows FULL and Owner manual PASS. No work in PG-01E/PM-06/CI #302.

## Documentation impact

Internal docs: not required
User/Product docs: not required
Reason: This is a **read-only D0 evidence report**, not a change to shipped behavior or public Kernel/Part semantics. Later production D1/D2 requires current internal as-built and bilingual PL/EN Product docs per the accepted Work Contract.
