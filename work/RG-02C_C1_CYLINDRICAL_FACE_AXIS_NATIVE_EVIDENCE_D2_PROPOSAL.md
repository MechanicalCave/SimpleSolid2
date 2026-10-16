# RG-02C/C1 — cylindrical Face → Axis: native OCCT evidence D2 proposal

**Status:** Owner **accepted product priority and the sequence native evidence → UX consultation → bounded implementation contract** on 2026-10-16 (`akceptuje - kontynuuj`). This text proposes the **exact P0 test-only scope** and records API-access STOPs. It is **NOT an already accepted executable D2 Work Contract, NOT activated**, and does **not** authorize edits outside this work-only proposal. Separate approval, governance merge and dedicated `work/ACTIVE.yaml` activation precede P0 test mutations.

**Authority:** main baseline `457725ce905a79150401fa28b166ac53d1e46b16` after RG-02C/B1 Owner technical FINAL PASS, protected-main feature #419 and governance #422/#424 merges, and administrative B1 closeout #425. `work/PART_MODELING_V1_ROADMAP.md` accepted v1.33 §20B; `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` §2.3 and mandatory UX §3. Preserve Foundation v1.0, accepted ADRs and one-Body Part v1.

## 1. Owner's exact product intent (already selected)

**Source is the user-picked bounded Face on the resulting current Body after an operation has executed**, regardless of whether its geometry came from Extrude, Revolve, Cut, Fillet or another Feature. There is **one Body**; no secondary Body picker and no use of the generating Feature's axis, sweep vector, name, sketch or operation type as an Axis source. A **genuine analytic cylindrical surface** under that Face defines its geometrical **unoriented, infinite axis** even when the visible Face is trimmed to an open/partial patch. This includes inner walls of holes and suitable straight-edge fillets only if OCCT reports an actual analytic cylinder. An unrelated curved/freeform/toroidal/conical Face is not accepted by fitting a cylinder to triangles or approximating geometry.

The selected current bounded Face must resolve to **exactly one certified stage-semantic source**; persist only suitable semantic identity plus explicit source Body stage, never `TopoDS_Face`, OCCT runtime tokens, sampled points, meshing, a cached world axis or an origin Feature axis. Re-evaluate from that declared stage on open/repair, and fail closed if missing, ambiguous, unsupported, changed to noncylindrical or wrong Body/stage. No global final-Body nearest/same-radius match. Stage-bound continuity after future topology-changing edits is not implicitly promised.

The existing Part-owned `AxisId`, native Axis commands, Revolve consumer direction controls and legacy Sketch-Line / B1 Edge Axis behaviors are not to be forked. `Constructor: Cylindrical Face` is a **future UI proposal**, not a currently approved widget.

## 2. Existing evidence and the hard boundary actually found in main

- **Provider analytic classification exists:** `src/kernel_occt/solid_modeling_kernel.cpp::providerSurfaceKind` uses `BRepAdaptor_Surface(face, true).GetType()` and maps `GeomAbs_Cylinder` to `kernel::SurfaceKind::cylinder`. The provider is allowed to classify **actual Face geometry**, not derive cylinder from the authoring operation.
- **Part semantic Surface carrier exists:** `SurfaceReference{BodyStageRef, FeatureSurfaceAddress}`, current stage `BodyStageTopologyCatalog.surfaces`, `FeatureSurfaceResolution::surface_kind`, `current_faces`, `strict_face_status`, and Face-local `BodyFaceTopologyRecord::surface_candidates`. A semantic **Surface carrier** can have multiple bounded current Face realizations. A picked **Face** and a Surface carrier are NOT interchangeable without an explicit unique mapping decision.
- **Real native fixtures already exist:** `tests/kernel_native/pm04c2_production_revolve_test.cpp` constructs a circular-profile Extrude cylinder, a Revolve Body, a Boolean Cut half-cylinder, and exercises Fillet/Chamfer. `tests/kernel_native/pm02p_b1_surface_carriers_test.cpp` proves existing cylinder kind classification/carrier lineage. These are prior witnesses for their own assertions; they have **NOT** proved a C1 Axis or generalized Fillet-cylinder acceptance.
- **Critical API gap:** current `kernel::IFaceBoundaryQuery` and `OcctSolidModelingKernel::bindFaceToBody/queryFaceBoundaryAnySurface` return the scoped Face boundary only. `IExactBoundedEdgeGeometryQuery` handles Edges, not Faces. There is **no provider-neutral, Body-bound read-only query returning the exact analytic cylinder origin/direction/radius from the selected Face**. The provider owns private `OcctRuntimeSolid::inventory_faces`. `SurfaceKind::cylinder` alone is **insufficient** to derive an Axis. Constructing a standalone `TopoDS_Face` in a test or inspecting OCCT internals through an unauthorized seam cannot certify the production source→Axis path. **STOP before claiming that analytic Axis is proven** until a separately reviewed narrow D2 provider query is approved.

## 3. P0: proposed bounded read-only native evidence contract — NOT YET AUTHORIZED

**New evidence test additions only after separate acceptance/activation.** Prefer one *already registered* native OCCT target:
`tests/kernel_native/pm04c2_production_revolve_test.cpp`.
Evidence report only:
`work/RG-02C_C1_CYLINDRICAL_FACE_AXIS_NATIVE_EVIDENCE.md`.
Do not add a new CTest target, CMake entry or duplicate an existing fixture. If this file cannot furnish a positive analytic straight-edge Fillet cylinder without unrelated fixture construction, record **NOT PROVEN** and return for a specifically bounded test amendment. Do not force a pass from a Fillet that is actually a torus.

P0-E01: evaluate actual OCCT Body after *circular-profile Extrude*, after *Revolve*, and after *Boolean Cut of the cylinder*. Compare **real current-stage Face/Surface catalogs**, not operation labels. Certify that candidate bounded Faces are `SurfaceKind::cylinder` in the native provider and that each selected Face has exactly one defensible `FeatureSurfaceAddress` in that stage.

P0-E02: for a genuinely partial/open cylindrical Face, prove classification survives trimming/cut, and record actual number of current Face realizations; separate **Surface carrier continuity** from **strict bounded Face identity**. A cylinder carrier with multiple current Faces must not silently mean an arbitrary Face pick is authorable.

P0-E03: characterize existing Fillet and Cut fixtures with honest classification, and distinguish **cylindrical** (eligible), **toroidal**, **conical**, **planar**, **other** (ineligible). No premise that every Fillet results in a cylinder. If required positive straight-edge Fillet fixture is absent, mark a coverage GAP and propose only the minimum additional fixture/test path rather than misclassifying geometry.

P0-E04: enumerate negative cases: planar/other Face rejected, missing/ambiguous/unsupported semantic Face, source-stage mismatch or stale current Body, multiple candidate carriers, and no runtime-token/XYZ fallback. Reuse the existing strict catalog statuses, preserve other tests. The **P0 limit** is classification + semantic authorability evidence; no Axis source, no native v20 schema, no UI, no new provider-neutral query, and **no claim of extracted analytic Axis**.

P0-E05: report exact native FOCUSED, cumulative FAST and actual Windows FULL Core+native Kernel+Qt/OCCT where applicable, and exact-head evidence CLOSURE. Preserve authentic RED/GAP and first failing source SHA, count only actually executed tests. Owner technical acceptance of P0 is separately required.

**Exact proposed mutation allowlist if Owner independently accepts P0:** only the above existing native test source and one new `work/**` evidence file. `src/**`, `docs/**`, `tests/CMakeLists.txt`, `.github/workflows/**`, `work/ACTIVE.yaml` outside a **separate** activation PR, and every other test file remain excluded. Neither this proposal PR nor Owner approval of high-level sequence activates that allowlist.

## 4. P1: future analytic Axis query — deliberate D2 STOP, not P0 permission

After P0, decide separately the **smallest read-only, provider-neutral exact-cylinder-Face query** tied to **the owning current RuntimeSolidHandle and selected runtime Face**, using the same provenance and source-stage checks as Part semantic Surface resolution. OCCT must read the **real selected Face** (e.g. adapter analytic cylinder) and return an unoriented infinite line; trim extent, `TopoDS_Face::Orientation()`, surface UV reversal, cyclic seams and Face partition must not determine signed Revolve behavior. Typed provider mismatch, missing Face, non-cylinder and invalid analytic geometry must fail closed.

The D2 proposal must explicitly decide **which durable identity to author** when the user picks a bounded Face but the existing reference value addresses a semantic Surface carrier: unique `SurfaceReference` if supported, strict bounded Face address where necessary, or STOP on ambiguous partitions. Avoid new generic Face identity machinery without an observed need. Propose explicit narrow kernel/OCCT/Part file allowlist, actual positive and negative provider tests, legacy v19 read and new-writer version handling, cold-reopen and dependent Revolve signed-direction correctness. The existing `IFaceBoundaryQuery` must not silently acquire unrelated mutating semantics. P1 must be Owner D2-approved before modifying `src/kernel/**` or `src/kernel_occt/**`.

## 5. UX and product implementation: later independent gates

After P0 findings and the accepted narrow analytic-query decision, present reviewable **existing Axis tool** UX for consultation: `Operations → Axis → Constructor: Cylindrical Face`; one Body Face pick, candidate highlight, unsupported/ambiguous diagnostic, finite unpickable visual cue for infinite axis, source replacement/Edit/Cancel/Finish and Repair, and matching contextual CAD Command Line semantics. No new toolbar button or Axis registry. **Separate explicit Owner UI acceptance before any UI/Viewer/Tree/Operations/CLI code**, per RG-02 design §3. Only after that review prepare a **separate bounded product Work Contract** with exact files, persistence/version compatibility, native end-to-end/full GUI+Revolve tests, PL/EN+internal+generated Browser DOC-01 and acceptance criteria. Guarded governance merge, dedicated `ACTIVE.yaml` activation, real FOCUSED→FAST→FULL and Owner manual/FINAL PASS remain distinct.

## 6. Documentation impact and current authorization

This **work-only proposal** changes no product semantics or UI, so there is no current as-built Internal/PL/EN/Browser mutation. P0, if activated, produces test evidence only. A later approved C1 product feature will require exact internal documentation, bilingual PL/EN product documentation and officially generated Product Browser validation.

**Owner decision currently recorded:** select C1 cylindrical Face Axis as the next user priority, single resulting Body, independent of originating Feature, including genuinely full and partial analytic cylinders; proceed evidence-first. **Not yet recorded:** approval of this exact P0 test allowlist, native exact Face-axis provider interface, detailed Face-vs-Surface persisted meaning, UI mockup or final implementation authorization. Do not manufacture any of those decisions.
