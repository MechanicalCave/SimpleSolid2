# RG-02C/C1-P1-B1/H1 — strict singleton cylindrical Face → durable Part Axis: bounded headless D2 Work Contract PROPOSAL

**Status: OWNER-ACCEPTED H1 HEADLESS D2 CONTRACT — ACTIVE on protected main since #443 (2026-10-16); Owner separately ACCEPTED precise v20 schema-header 19th-path addendum (2026-10-16), but this scope-only CONTRACT AMENDMENT remains UNMERGED/NOT YET EXECUTABLE for the newly added header until its independent Windows governance CLOSURE and guarded squash.**
**Owner direction:** 2026-10-16 Owner expressly `akceptuje i akceptuje implementację i merge - kontynuuj pracę i nie przerywaj` to the Agent's recommended **architectural option A** and a wish to implement/integrate. This accepts A's **strict singleton policy**, not future unshown code/file/schema/UX choices or the separate exact-head guarded merges governed by `AGENTS.md`.
**Baseline:** protected `main` `b14c6821c8707e57f7b9a8ad8cacf348a6a54dba` after [P1-B0 evidence #438](https://github.com/MechanicalCave/SimpleSolid2/pull/438) Owner technical FINAL PASS with explicit native GAPs and separate guarded squash. `work/ACTIVE.yaml` still points to P1-B0; it is NOT a P1-B1 mutation authorization.
**Authority:** Constitution/Foundation 1.0, Architecture/accepted ADR-0014/-0016/-0017, Part Modeling v1.33, RG-02 owner direction, existing PM-04 Axis/PM-05/Projection, B1 Edge native v19 compatibility, accepted P1-A `IExactCylindricalFaceAxisQuery` and accepted P1-B0 stage-semantic Face-vs-Surface evidence. Distinct [P1-B1 UX design #439](RG-02C_C1_P1B1_STRICT_CYLINDRICAL_FACE_AXIS_UX_OWNER_REVIEW_PROPOSAL.md) received **separate explicit Owner UX signoff** on its exact W1–W3/UX-01…14; #439 governance merge remains a separate STOP and neither approval starts H2 code.

## Recorded Owner exact H1 D2 contract acceptance — independent gates

On 2026-10-16, after presenting complete reviewable [UX Draft PR #439](https://github.com/MechanicalCave/SimpleSolid2/pull/439) and exact bounded [headless H1 D2 Draft PR #440](https://github.com/MechanicalCave/SimpleSolid2/pull/440), the Agent asked for **two independently explicit decisions**: approval of the exact new UX, and approval of **this precise H1 Work Contract, its 18 allowed paths, strict variant A, new native v20 writer/legacy v19 readers, headless stages and Windows gates**. The Owner answered **`akceptuję - kontynuuj`**. This records exact **Owner H1 D2 scope ACCEPTANCE** and, independently, new UX design SIGNOFF; these decisions do not merge their separate governance PRs, change `ACTIVE.yaml` or authorize immediate code mutations.

**Allowed only after separate governance protected-main integration plus independent ACTIVE-only activation:** actual H1 headless strict cylindrical Face→semantic `SurfaceReference` `PartAxisSource` implementation, schema v20, reused source evaluation / headless lifecycle / native tests / DOC-01 within **the original 18 §2 paths, plus only the separately Owner-accepted schema-header 19th path once the amendment is integrated**. Any path expansion or non-headless UI requires a distinct approved amendment or new H2 contract. The Owner previously expressed an intent to implement and merge the feature, but this reply to an exact **scope-design** question is not a guarded squash authorization for #439, #440, an ACTIVE-only PR or a future code PR. Native B0 partition-positive and other explicitly deferred fixture GAPs remain NOT TESTED, never silently promoted to proof.

## Narrow accepted 19th-path D2 amendment — activation remains unchanged

After activating this exact headless H1 contract through independently Windows-validated and guarded-squash merged ACTIVE-only [#443](https://github.com/MechanicalCave/SimpleSolid2/pull/443), source inspection showed that the existing authoritative native Part schema constant is declared at `src/part/include/simplesolid2/part/part_document_store.hpp` (`PartDocumentStore::current_schema_version = 19`). That exact header had been omitted from the original 18 paths despite the accepted native v20 writer/read-back requirement. The Owner was explicitly asked to accept **only this one 19th header, solely to change the version from v19 to v20**, and replied **`akceptuje - kontynuuj`** (2026-10-16). This acceptance is recorded in separately governed one-file [addendum #445](https://github.com/MechanicalCave/SimpleSolid2/pull/445).

**Exact amendment: add one new path #19 in §2**, limiting it to changing `current_schema_version = 19` → `20` and directly relevant comments. All previously accepted 18 paths and their purposes are unchanged. No new H2 UI / Viewer / CLI or CMake/CI path, schema bypass, other file, writer semantics, provenance identity fallback, Face cardinality policy or native GAP acceptance. The Owner-approved **source-scope amendment** is not immediate authorization to mutate the out-of-scope header: #445 governance must be integrated, this **active H1 contract file's one-path amendment must separately pass exact-head Windows CLOSURE and protected-main guarded squash**, and the new header edit may follow **only once both are merged**. Do not change `work/ACTIVE.yaml`: it already points to this active contract.

This corrected scope explicitly acknowledges prior Windows ACTIVE-only #441 RED #3365 (DOC-01 impact syntax), Owner-approved one-file #442 DOC-01 remediation with Windows #3366/#3367 PASS and protected-main integration, and successful replacement ACTIVE #443 Windows #3368/#3369 and squash. No previously RED or queued native job is claimed as PASS.

## 0. Deliverable boundary — H1 is **headless**, not new UI

**ONE** Part-owned authored Axis source variant for precisely **one** analytic cylindrical current **bounded Face** at a declared existing Body history stage, admitted by **A**; end-to-end Part headless source evaluation, durable native serialization and cold-rebuild lifecycle; reuse existing `AxisId`, `AxisReference`, Axis draft/transaction/Undo and Revolve consumption. **No Work Features Constructor UI, source pick widget, tree/toolbar/viewport/Command Line alterations in H1.** Future UI H2 requires independent precise Owner W1–W3 UX approval before any code. No broad RG-02B/PM-06/G1/Plane intersection/Datum expansion/Assembly/Mirror/Pattern/Body expansion.

P1-A **reads exact native cylindrical Face** into infinite unoriented line/radius, while P1-B0 **observed** its owner/strict Face semantics and independent all-member geometry. Neither established durable picked-patch identity after partition. This H1 adopts conservative A by **refusing every partition** with `strict_face_status != resolved` OR `current_faces.size() != 1`. The semantic `SurfaceReference{BodyStageRef,FeatureSurfaceAddress}` may persist because it certifies exactly one **strict Face at the authored stage when resolved**, but its runtime numeric `FaceToken`, OCCT shape, provider order, fitted XYZ, radius, and cached line **MUST NOT** persist.

## 1. Owner-accepted exact headless Part technical design — activation separately gated

### H1-01 — semantic source / schema separation

Add **exactly one** third `PartAxisSource` variant, provisionally named `StrictCylindricalFaceAxisSource{SurfaceReference surface}`. `source.valid()` requires legal nonempty `BodyStageRef::after_feature`, valid existing `FeatureSurfaceAddress` and no provider handle. Structural validity does **not** fabricate up-to-date native geometry; derived line is transient. Keep existing `SketchLineAxisSource` and `StraightMaterialEdgeAxisSource` source value layouts, `AxisId`, `AxisReference`, old authored Axis rows, visibility, dependency ordering and name format unchanged. Same-Part source only; no forged DocumentId or cross-body pointer.

When authoring from **picked current Face** (H1 native/headless only), use the actual complete source-stage Body topology catalog. Require exactly **ONE** Face inventory record for the exact runtime token; `surface_candidates.size()==1`; corresponding unique `FeatureSurfaceResolution.status==resolved`; `surface_kind==cylinder`; `strict_face_status==resolved`; `current_faces.size()==1`; sole token matches picked token; no integrity failure; current matching RuntimeSolidHandle provider; and a fresh successful exact native `IExactCylindricalFaceAxisQuery` for this Body-scoped Face, with valid geometry. Any failure returns typed status, **never** first-owner/first-Face selection or geometry-nearest recovery. The returned durable source is only `SurfaceReference`, not a Face token or line.

### H1-02 — resolving authored source on reevaluation and Revolve

At the **authored stage**, find its explicit `BodyStageRef` topology; do not search arbitrary final Body to recover a missing source, and reject forward/cyclic dependencies. From that stage's `FeatureSurfaceAddress`, insist again on resolved semantic owner **AND strict single Face** with one current provider Face, and freshly query P1-A native Face cylinder. If a Boolean split changes `strict_face_status` to Ambiguous or `current_faces>1`, fail closed and leave the Axis *authored but unresolved/repairable*. Even if fragments are coaxial, do **not** elevate B0 candidate-B as persistent A identity. Any missing Feature, suppressed upstream stage, wrong Part document/revision/provider generation, null/foreign Body, noncylinder, unavailable native geometry or expired tokens results in typed `missing/ambiguous/unsupported/blocked` and no last-good line.

No change to `AxisReference` (consumer still references AxisId); the produced line remains **unoriented infinite**, with any point on line valid. Existing Revolve owns signed sweep/Reverse. A change in native `TopoDS_Face` parametric or topological orientation must not silently author an Axis sign. A resolved exact current source can drive existing Revolve Add/Cut within PM-04 axis-in-profile admission; missing/partitioned source makes dependent Revolve fail closed without changing its authored identity.

Potential public API change: a separate optional `kernel::IExactCylindricalFaceAxisQuery*` parameter (or a narrow overload) on the existing neutral `part::resolveAxisReference`; keep all old signatures/call sites compatible and avoid promoting OCCT into `src/part`. Add only narrow typed Face-source diagnostics to `AxisEvaluationDiagnostic`; there must be no new public/kernel provider API or duplication of the existing P1-A native query.

### H1-03 — native file version and backward compatibility

Existing writer is **native Part v19** for Body Edge Axis (exact Sketch Line record shapes for legacy). Adding a durable variant requires explicit **new writer v20**, a named kind such as `cylindrical_body_face_strict`, a single `source_surface` semantic object encoding **existing stage + Surface address only**, and unchanged AxisId/name/visible. Reuse the already existing serializer/parser for `SurfaceReference`, not a duplicate geometry/Face token codec. `v12…v19` readers preserve old `sketch_line` and `straight_body_edge` semantics unmodified, reject unsupported new kinds in older formats; `v20` reads all three and refuses malformed/duplicate/stale incompatible data without silently coercing. Existing legacy v19 fixtures remain readable; new native v20 roundtrips a source across Save/Close/Reopen/cold rebuild. Bumping the actual file schema may require targeted existing writer-version fixture assertions; if any such file is **not** on the approved allowlist below, STOP and obtain an explicit exact-path D2 amendment, never bypass RED.

### H1-04 — transaction, lifecycle and validation

Reuse existing Part `CreateAxisCommand`/`EditAxisCommand`, `AxisDraft` and revision-bound `finishAxisDraft` semantics for a headless test of the new source. One valid Finish → one source AxisId + one Undo step; stale revision/draft must refuse. Explicit `Edit` / Replace `SurfaceReference` preserves AxisId and consuming Revolve reference; Cancel does nothing; Delete leaves downstream broken but repairable; Undo/Redo must restore authored identity/source/visibility; no silently repaired source. Existing Sketch/Edge paths, datum and suppressed Body history remain unchanged. No product UX change is permitted by H1.

### H1-05 — native truth / honest gaps

Use actual resulting Part/OCCT Faces in existing registered kernel native fixtures: full cylinder Extrude, full/90° Revolve, half cylinder Cut and circular-rim Fillet/Chamfer **without assuming Fillet is cylindrical**. Require at least one successful unique stage-semantic strict cylinder source and valid unoriented line/radius; planar/torus/cone must refuse when actually witnessed. Test wrong Body/stage, no Face, ambiguous owner, multi-Face carrier, source invalidation and noncoaxial member only when a **genuine actual fixture** exists; label absent branches **NOT TESTED**, do not construct fake statuses and claim PASS. Native multi-Face carrier B0 positive remains a deferred GAP and is not a prerequisite for A if that case cannot be produced; specifically a partition is always rejected, no adoption of B. Inherited P1-A native topological orientation reversal, positive straight-edge Fillet cylinder and fault/degenerate cases remain **NOT TESTED** unless the actual new tests execute them.

## 2. **OWNER-ACCEPTED H1 implementation allowlist — activated original 18, proposed unmerged 19th-path correction**

Owner expressly accepted the original H1 18 paths, and #440 and ACTIVE #443 have been independently Windows-validated and guarded-squash merged. **The original 18 paths ARE CODE ACTIVE**. The Owner has also explicitly accepted the precise 19th header-path scope, but **that additional path remains NOT ACTIVE for code mutation until separately governed addendum #445 and this exact contract amendment are integrated**, with no other scope expansion. The complete allowlist after both merges is:

1. `src/part/include/simplesolid2/part/axis.hpp` — third source variant.
2. `src/part/axis.cpp` — source/structural validation.
3. `src/part/include/simplesolid2/part/axis_evaluation.hpp` — typed diagnostics and optional read-only provider seam.
4. `src/part/axis_evaluation.cpp` — exact stage-aware strict singleton provider resolver.
5. `src/part/part_document.cpp` — durable semantic-history and dependency checks.
6. `src/part/part_document_store.cpp` — explicit v20 reader/writer with legacy v19 and older compatibility.
7. `src/application/include/simplesolid2/application/document_session.hpp` — reuse typed Axis commands.
8. `src/application/document_session.cpp` — reuse revision-bound create/edit/repair.
9. `src/application/include/simplesolid2/application/axis_draft.hpp` — headless candidate source.
10. `src/application/axis_draft.cpp` — headless draft evaluation/Finish no UI.
11. `tests/kernel_native/pm04c2_production_revolve_test.cpp` — real native stage Face/Axis source witness.
12. `tests/pm04a_axis_schema_v12_test.cpp` — v20 new source and legacy v12…v19 compatibility.
13. `tests/pm04b_axis_lifecycle_draft_test.cpp` — revision, edit, invalidation, Undo/Redo.
14. `tests/pm04e1_revolve_persistence_test.cpp` — authored Axis→Revolve native save/reopen.
15. `tests/pm04e2_revolve_lifecycle_cold_rebuild_test.cpp` — cold rebuild / dependency fail-closed.
16. `docs/internal/PART_DOCUMENTS.md` — truthful internal as-built v20 source semantics.
17. `docs/browser/index.html` — **generated-only DOC-01**, never manually edited; only official browser regeneration if required by the verified canonical internal doc change.
18. `work/RG-02C_C1_P1B1_H1_STRICT_FACE_AXIS_HEADLESS_ACCEPTANCE_EVIDENCE.md` — new factual RED/GAP/PASS ledger.
19. `src/part/include/simplesolid2/part/part_document_store.hpp` — **ONLY** the accepted existing `PartDocumentStore::current_schema_version` update from 19 to 20, plus immediately relevant comment; no other API or header changes. **Do not edit this newly added path until PR #445 and this contract amendment are independently Windows-validated and guarded-squash MERGED into protected main.**

The original 18 listed paths are **Owner-accepted, ACTIVE and executable** under merged #443. The newly listed **19th** schema-header path is Owner-accepted **but forbidden to mutate until #445 and this exact H1 contract amendment are separately integrated through the protected-main governance gates**. No other `src/**`, tests, version-assertion fixes, CMake/CI, `AGENTS.md`, roadmap, `ACTIVE.yaml`, Qt/Viewers, `src/ui/**`, Operations/Tree/CLI, PL/EN product docs or shipped code. If RED evidence requires a missing file path, STOP for an **Owner-accepted narrow D2 path amendment**; no skipping native FULL or silently broadening. H1 is **headless** only; GUI H2 requires a separate exact Product D2 Work Contract and Owner UX signoff.

## 3. Mandatory validation and owner gates — no invented PASS

- Genuine Windows OCCT native **FOCUSED** using already registered target/tests for the H1 source. If CI-03 focused runs permit one exact test in the approved path, use it; no new CMake target/workflow.
- Separate Windows **Draft FAST** (existing full FAST denominator), real **Ready FULL** Core-only + Kernel-native OCCT Release + Qt/OCCT Desktop, **official DOC-01** (including generated Browser if canonical docs changed). Expected baseline 25/57/117, but actual verified CTest denominators take precedence.
- Owner Windows headless Save/Close/Reopen/cold-build and Revolve selection where executable; explicitly report what is automated vs manual and all inherited B0 native GAPs.
- Exact-head evidence-only Windows **CLOSURE** after final factual ledger; separately accepted **Owner technical FINAL PASS**, then separately authorized guarded squash of *the actual implementation PR*, never blanket assumption based on this proposal.
- **Separate Owner UX signoff is now ACCEPTED** for #439 W1–W3; still NO product UX or shipped `Cylindrical Face` constructor until independent H2 executable contract/activation and normal native/Owner manual Windows checks.

## 4. Owner decision sequence — do not conflate

1. **CLOSED:** Owner chose *architecture A* and desires implementation/merge. No automatic selection of B or C.
2. **CLOSED — OWNER ACCEPTED exact-text H1 D2 2026-10-16:** bounded headless implementation, exactly the original 18 paths and v20/legacy v19 policy; separately Owner-accepted ONE-PATH addition #19 for the actual authoritative schema version header, subject to independent merge gates. This scope acceptance is NOT code PR or governance merge consent.
3. **PENDING:** new exact-head Windows governance CLOSURE on the decision-only H1 D2 HEAD, then separately authorized protected-main squash of #440; separately authorized `ACTIVE.yaml` activation.
4. **PENDING:** actual H1 headless code + native Windows gates + technical FINAL PASS + independent guarded squash.
5. **CLOSED for UX DESIGN ONLY — separate Owner signoff recorded in #439:** exact existing-tool second `Cylindrical Face` Constructor. **PENDING** independent #439 governance merge and independent H2 executable Work Contract/activation for existing `Axis` Work Features product UX, PL+EN docs and generated Browser.
6. **PENDING:** H2 full product test/Owner manual Windows/technical FINAL PASS and independently authorized guarded squash. Later variant B/C, PM-06, other constructors remain separate D2 priorities.

## Documentation impact

Internal docs: required
User/Product docs: not required
Reason: Activated headless H1 changes durable Axis source identity, stage resolution and native v20 persistence, so update allowlisted `docs/internal/PART_DOCUMENTS.md` before H1 completion. No new user-visible Axis constructor ships in H1: product PL/EN docs are reserved for separately accepted H2 UX implementation. Generate `docs/browser/index.html` only with the official DOC-01 tool when its canonical documentation changes; never hand-edit it. This governance-only DOC-01 syntax remediation implements no product behavior and does not change the Owner-accepted exact 18-path H1 allowlist or native GAP policy.
