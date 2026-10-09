# PG-01D — Planar Face Boundary Project Geometry — Accepted Bounded Work Contract

**Status: OWNER D2-F1–F6 AND WORK CONTRACT ACCEPTED 2026-10-09 — NOT ACTIVE UNTIL DISTINCT GOVERNANCE ACTIVATION.**  
**Owner design approval:** D2-F1–F6 expressly accepted 2026-10-09, `akceptuje - kontynuuj`; authority: `work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_D2_PROPOSAL.md`.  
**Contract approval:** Owner expressly approved this bounded PG-01D Work Contract on 2026-10-09: `Zatwierdzam Work Contract PG-01D — kontynuuj`. The separate activation gate remains mandatory.  
**Baseline:** `main` `d928e8a5d1f5893b73de3e4fcd399d7e0483af83` — PG-01C Owner FINAL PASS + guarded squash [PR #304](https://github.com/MechanicalCave/SimpleSolid2/pull/304), original head `95731b9` Windows FULL #2102 **196/196 PASS**.  
**Program:** `work/PART_MODELING_V1_ROADMAP.md` v1.30, Projection 01D within milestone 9B and **before PM-06**.  
**Authority:** Constitution / Foundation v1.0 §7.2/7.5 / Architecture / ADR-0014/0016/0017 / completed Projection 00A and PG-01A/B/C.  
**Activation:** a **dedicated governance commit** must change `work/ACTIVE.yaml -> active_work` to **`work/PROJECTION_01D_PLANAR_FACE_BOUNDARY_WORK_CONTRACT.md`**, pass governance checks and be merged on `main`; only **then** open a new production feature branch from activated `main`. The document-approval PR #305 is **not** the activation branch.  
**Out of scope:** PG-01E, PM-06, CI issue #302 and any unapproved schema/architecture change.

## 1. Goal and measurable result

Extend the **already accepted** Project Geometry tool in an editable Part Sketch to stage **one current, strictly identifiable planar bounded Face** from an earlier legal Body stage. Capture exactly its current outer/hole **material Edge occurrences**, project each representable exact Line/Circle/Arc to current Sketch support, visibly skip only independently proven geometric Unsupported members, and use the **existing** atomic `CreateProjectedSketchEdgesCommand` to save all admitted unique `MaterialEdgeReference` links with chosen Regular/Construction role.

The existing manually staged Edge workflow must remain fully usable, including combined manual Edge + one Face staging, deduplication by semantic source identity, preview, Finish/Cancel, Command Line, hierarchical Esc, focus arbitration, provider-loss handling, one Undo and cold Save/Reopen.

### Immutable behavior

- **Face selection is an ephemeral expansion gesture**. **No durable `FaceReference`/Face token, wire numbering or per-Face membership link** is added to native Part schema v15. Durable authoring remains **one existing strict material Edge binding per new target EntityId**.
- Preview derives exclusively from one verified current Body stage plus target Sketch frame; old stored Sketch seeds and last-good provider output never count as current.
- Strictly blocked Face/source admission is distinct from **skip** of an Unsupported geometric curve with sound semantic Edge identity. **No tolerance-based healing, similarity/proximity rebinding or implicit hole closure**.
- No replacement of individual Face with the same Surface's other Face fragment. Partition artifacts and periodic seam realizations cannot become silent eligible material Edges.
- Partial supported output is clearly marked **Partial**; Finish can commit the admitted supported subset **without a modal confirmation**. No supported output means Finish cannot mutate. An open partial contour may produce **no Profile** until the user explicitly repairs it.

## 2. Approved D2 decisions frozen for this package

1. **D2-F1 (identity):** admission from exactly one **bounded strict Face realization** on the selected stage, not merely a Surface carrier. UI/runtime Face tokens are generation-bound and never persisted. Only original PG-01B/C strict `MaterialEdgeReference` authoring.
2. **D2-F2 (native read boundary):** provider owns actual oriented Face wires (outer/inner/hole) and real Edge occurrences; Part cross-checks each against its **complete** same-stage material Edge catalog, preserving typed integrity failures.
3. **D2-F3 (partial):** **only** an individually semantically identifiable Edge's exact **geometric Unsupported** curve/image is skipped, with diagnostics/source highlighting; semantic Missing/Ambiguous, topology/wire accounting inconsistency, stale/mismatched generation, wrong stage, cycles or provider loss reject the Face gesture/Finish.
4. **D2-F4 (dedup):** semantic `MaterialEdgeReference` equality at one source `BodyStageRef` for a combined manual/Face batch. Numerically equal *different* edges remain different.
5. **D2-F5 (UX):** reuse one PG-01C Workbench tool state. Add source-choice **Edges / Planar Face** plus outer/hole/accepted/skipped status and visible partial warning in right Operations. Existing `PROJECT`, `REGULAR`, `CONSTRUCTION`, `REMOVE`, `CLEAR`, `FINISH`, `CANCEL`, Esc/Enter and focus rules remain authoritative; no second tool/dispatcher and **no new mandatory modal**.
6. **D2-F6 (transaction):** at Finish revalidate same revision, scoped provider Face, actual member identity and geometric skip classification. A membership/skip change since preview causes **stale-draft failure**, never unseen re-expansion; all-or-nothing authoring via one established semantic batch Command and Undo. Undo/Redo/Save/Reopen use only existing authored per-Edge references.

**No second identity truth:** source topology/Face/wire order, picked triangle/representation token and OCCT geometry are runtime observations, not persisted CAD intent.

## 3. Allowed bounded source modules *after* activation

| Owner / files | Authorized purpose | STOP boundary |
| --- | --- | --- |
| `src/kernel/include/simplesolid2/kernel/edge_projection.hpp` or a narrowly introduced provider-neutral `face_boundary.hpp`, `src/kernel_occt/include/simplesolid2/kernel_occt/solid_modeling_kernel.hpp`, `src/kernel_occt/solid_modeling_kernel.cpp` | Read-only scoped same-provider **Face boundary query** returning typed outer/hole oriented occurrences; include exact current Body generation; no OCCT types above provider seam | No generic B-Rep healing/boolean modification, no persistent topology identity; public provider API beyond this query is a new Owner D2 STOP |
| `src/part/include/simplesolid2/part/feature_evaluation.hpp`, `src/part/feature_evaluation.cpp` | Narrow **read-only** Face admission and strict current material Edge-reference map; validate complete topology catalog and per-edge statuses. Prefer reuse `authorMaterialEdgeReference` | No authored `PartDocument` schema, `MaterialEdgeReference` layout, modeling semantics version or general Face meaning change |
| `src/application/**` only as strictly needed | Runtime-only Face expansion/validation result on same revision and delegation to existing `CreateProjectedSketchEdgesCommand`. Provider-neutral typed diagnostics; one atomic transaction | No new persisted Face/batch link command, identity cache, or second semantic authoring path. DocumentSession contract change outside agreed read/validation adapter -> Owner D2 STOP |
| `src/ui/cad_workbench.cpp/.hpp`, `src/ui/part_viewport_controller.cpp/.hpp` | Existing tool's Edges/Planar Face acquisition, right Operations partial diagnostics, status, stale-draft/skip presentation and exact current stage handling. Leverage existing `setBodyTopologyFacePickOnly` as warranted after verifying tool-stage pickability | No second tool-state machine, no global selection rewrite, no new unbounded cross-tool shortcuts |
| `src/viewer/**` only where proven necessary | Narrow provider-neutral transient Face/source Edge highlight or topology pick routing, with current generation and no Viewer-owned semantic inference | No mesh-outline-derived wire authority or persistent picked-token identity |
| `tests/**` existing suitable targets first | Kernel-native wire/hole/orientation/partition proof, Part strict semantic mapping and atomic Workbench PG-01C-parity tests; existing `tests/pm05f_r2_native_workbench_edit_test.cpp` / kernel-native topology tests preferred after confirming fit | `tests/CMakeLists.txt` new registration requires its own bounded justification and checks against independent #302 |
| `docs/internal/**`, `docs/product/pl/**`, `docs/product/en/**`, `docs/browser/index.html`, `work/**` | Current as-built, bilingual Product docs, deterministic generated Browser, evidence and contract lifecycle | Browser **generated via `.\ss2.ps1 docs` only**, no hand-edited alternative truth |

No edits to Foundation, accepted ADRs, program roadmap semantics, `src/part/part_document_store*`, native file versions or `main` production code by direct push. A test/diagnostic that requires an additional module outside this map is a **STOP / proposed scoped amendment**, not implicit permission.

## 4. Planned execution stages and RED→GREEN gates

### D0 — native source-topology evidence (must pass before D1)

1. Inspect the existing exact runtime Face selection → `BodyStageTopologyCatalog` → semantic `BodyFaceTopologyRecord` (strict `semantic_address` versus Surface-only `surface_candidates`), provider token binding, source Body/Sketch stage, and existing PG-01C Edge pick/draft state.
2. Construct procedural native OCCT solids with a planar cap, **a true through-cut hole** (outer+inner wires), distinct boundaries after Extrude/Cut/Chamfer, reversed Face orientation, split Surface into several Face realizations and a representation partition. Record **actual native oriented Face-wire memberships**, not tessellated approximation.
3. RED fail-closed characterization: a Face with carrier only and no uniquely resolved bounded Face address is **not** silently admitted; a wire occurrence without one exact material source, or a seam/partition artifact, is not guessed.
4. Provider query must prove **exact Face scoped-to-one-solid identity** (including provider/lifetime generation), ordered oriented occurrences and typed nonplanar/source/wire errors. If OCCT provider topology evidence disagrees with catalog semantics, **STOP**, document deficiency and request Owner D2 before implementation.

### D1 — kernel/Part read-only Face expansion

- Implement the minimal optional `IFaceBoundaryQuery`-style runtime provider boundary, analogous to `IEdgeProjectionQuery`, with opaque scoped current Face source and status + outer/hole list; query must be nonmutating.
- Match each reported current runtime Edge occurrence in the **same current stage catalog**, run existing `authorMaterialEdgeReference`, reject nonmaterial/ambiguous/partition/seam and preserve the distinctions `resolved` vs `geometric_unsupported` vs `integrity_failure`.
- Check representable exact Line/Circle/Arc with PG-01A/C projection; use source TopoDS_Vertex endpoint policy already accepted for Line.
- Distinguish repeated **uses** of one topological Edge within one wire from separate semantic material edges. Missing/duplicate/invalid native wire evidence never establishes an authored Face identity.

### D2 — one existing Workbench staged tool and partial preview

- Add **Edges / Planar Face** pick choice to the current Project Geometry Operations panel, without new tool or second selection authority; ensure current tool-stage Face picking is genuinely enabled (existing `setBodyTopologyFacePickOnly` alone does not authorize arbitrary tool-stage Face admission).
- Stage **one** uniquely resolved Face, its current wire/Edge observations and current skip classification. Allow additional manual Edge picks; dedup by exact `MaterialEdgeReference`. Preserve prior valid manual staging when an unsupported/invalid additional Face pick is rejected.
- Display outer/hole membership and individual supported/skipped labels plus source highlighting; partial result explicitly warns that region closure is **not guaranteed**.
- Finish reruns same provider/stage query and compares member identities and skip decisions against the preview on the **current same revision**. No valid output => no mutation. Successful batch calls the existing `CreateProjectedSketchEdgesCommand` **once**. No stale Face token survives selected document/tool/Sketch changes.
- Maintain PG-01C command grammar, Esc hierarchy, tool switch, focus, Save/Undo/Redo and transient Viewer ownership.

### D3 — lifecycle and negative-case regression

- Native provider+Part headless tests: simple planar Face, real outer+hole, reversed Face, split bounded Face vs Surface, Chamfer, unsupported exact curve, unresolvable material reference, malformed wire, missing provider/source, current-stage shift.
- Native Qt/OCCT test in existing Workbench harness: mixed manual Edge + one Face, exactly one transaction, dedup, partial accepted subset and warning, role Regular/Construction, selected source feedback, Command Line/tool Esc parity, cancelled/failed no-op, Undo/Redo.
- Save/Close/Reopen v15 with fresh OCCT, prior Face token discarded; only durable material Edge links resolve, remain Broken fail-closed if source unavailable, and can be repaired using accepted PG-01B/C mechanisms.
- Verify **old PG-01C** tests including Sketch 5 after Chamfer, linked Circle Profile status, many-edge Break Link and unsupported manual Edge staging still pass.

### D4 — exact-head verification, documentation, Owner acceptance

- FOCUSED real native/provider and Workbench RED→GREEN evidence; FAST relevant suite with exact rebuilt binaries. Issue #302 cache/registry mismatch may not be bypassed or reported as PASS.
- Canonical internal/current as-built and **PL/EN paired** Product docs updated, Browser regenerated with `.\ss2.ps1 docs`, repository verification green.
- One immutable **Windows exact-head FULL** (Core/Kernel/Desktop + docs/aggregates) before Owner manual Face-with-hole/partial-skips practical Windows retest; record actual counts and SHA; no fabricated PASS.
- Separate Owner **PG-01D FINAL PASS** and explicit merge approval/gate. No PG-01E or PM-06 by implication.

## 5. Non-negotiable acceptance matrix

1. **One fully supported planar Face:** exact current outer perimeter yields linked Regular Line/Circle/Arc material entities and the expected valid Profile; same source selected via Edge mode produces matching distinct strict source IDs, not duplicate target authoring.
2. **Outer perimeter and holes:** actual oriented material wire usage is read from native Face. All supported outer/inner members are admitted exactly once and the relevant Profile/holes resolve correctly; no false face/surface choice.
3. **Partial geometric Unsupported:** only an individually uniquely identified Edge with unsupported exact image is skipped, distinctly diagnosed, highlighted and omitted. Remaining supported subset commits in one Undo with visible Partial status; no automatic Profile closure or gap healing.
4. **Integrity failures:** wrong Face kind/nonplanar, wrong stage, ambiguous or carrier-only bounded Face, nonmaterial/partition/seam, ambiguous material Edge, stale Scene/DocumentGeneration, source suppression, provider failure, malformed wire or missing Face -> no Face authoring, no partial commit and no corruption of previous staged valid manual Edges.
5. **Semantically distinct overlaid edges** remain distinct. Mixed source paths referencing the same `MaterialEdgeReference` deduplicate. Repeated Face pick and manual Edge source are not silently multiplied.
6. **State and lifecycle:** Cancel/zero-admitted Face, backspace/Esc while Command Line focused, Finish variants, wrong focus, Undo/Redo, alternate Sketch tool activation and Sketch/Document close clear provider tokens and overlays, leaving no dirty state or ghost selection.
7. **Cold persistence:** v15 file remains unchanged; after restart no old Face/wire membership token is necessary for saved linked Edge entities, Profile/Extrude/Revolve and Break Link; upstream Face membership change does not auto-add new authored targets.
8. **Audit:** no unauthorized edits to `main`, schema, accepted ADR/Foundation, model-semantics version or CI classifier; all required docs and Owner verification complete.

**STOP — examples:** same semantic Surface has multiple Face realizations but no unique selected bounded Face; OCCT native wire records cannot be reconciled with current material Edge semantic catalog; exact unsupported Edge cannot be given a sound source identity; preserving one transaction would require durable Face binding; broader Kernel/Part public API or modeling-semantics compatibility changes are discovered. Escalate *before* changing the owning architecture.

## 6. Proposal handoff / activation sequence

1. **Accepted:** Owner D2-F1–F6 design on 2026-10-09; Owner explicitly accepted this bounded Work Contract on 2026-10-09 (`Zatwierdzam Work Contract PG-01D — kontynuuj`).
2. **Activation pending:** a separate governance branch and **standalone activation commit** update `work/ACTIVE.yaml` from completed PG-01C to this accepted PG-01D contract; verify and merge its governance PR to `main`. Design/contract-acceptance PR #305 is **not** the activation.
3. **Only after activation merger:** start a separate PG-01D production branch off activated `main`; execute stages D0→D4, test and obtain separate Owner practical PASS. Do not reopen PG-01C.

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: PG-01D introduces a native Face-wire read contract, strict bounded Face/material Edge semantics, one Sketch Project Geometry Face mode, visible partial failures and linked lifecycle. Ship current internal as-built documentation and paired Polish/English Product docs; regenerate the Browser via `.\\ss2.ps1 docs` before PG-01D acceptance.
