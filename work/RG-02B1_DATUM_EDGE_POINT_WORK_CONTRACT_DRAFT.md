# RG-02B1 — Datum Plane: one Edge + Point tool (DRAFT bounded Work Contract)

**Status:** **DRAFT FOR OWNER REVIEW; NOT MERGED, NOT EXECUTABLE, NOT ACTIVATED.** The **user interaction principle** below is **Owner-accepted** on 2026-10-13; exact technical schema, native proof, complete visual/keyboard UX, implementation file allowlist and this Work Contract's executable scope are **NOT yet independently Owner-accepted**.  
**Direction:** Owner chose development of Datum Plane + Axis as the **sole next planning focus** in accepted and merged Part-roadmap v1.33 ([PR #343](https://github.com/MechanicalCave/SimpleSolid2/pull/343)). Start with **existing Datum Plane**; do not use this contract to start unrelated Axis constructors, G1, Mirror/Pattern or PM-06.  
**Authoritative baseline:** `main` `b7da33653b8bc6214fd5774a34498838decb19cd`. Existing `work/ACTIVE.yaml` references **completed PG-01E** for traceability only and `program_roadmap_version: "1.32"` until a separate metadata correction ([PR #344](https://github.com/MechanicalCave/SimpleSolid2/pull/344)) is independently approved and merged. Do **not** merge/activate #344 as a side effect of this draft.  
**References:** `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` (Owner-accepted broad product direction, NOT executing contract); `work/PM-03_DATUM_REFERENCE_GEOMETRY.md` (completed Offset), `work/PM-04_AXIS_REVOLVE.md` (completed Axis/Revolve), `work/RG-02A_REFERENCE_FOUNDATION_AUDIT_DRAFT.md`, and `work/RG-02A_G0_NATIVE_VERTEX_EVIDENCE.md` (Owner-accepted G0 native limitations, PR #332 merged); Constitution, Foundation 1.0, accepted ADRs, current roadmap and DOC-01.

## 1. Owner-accepted interaction direction (bounded UX principle, NOT the full RG-02 UI gate)

Owner supplied a screenshot of the **existing Datum Plane** Operations panel with `Constructor: Offset`, then proposed developing **Edge with on-edge END/MID** before other Datum constructors. When offered two distinct interpretations:
- A: a plane **through a straight Edge and a point outside its supporting line**, versus
- B: a plane **perpendicular to an Edge at END/MID**,

Owner first explicitly chose **B**. Owner then explicitly refined the user experience: **only choose the Datum constructor in the existing dropdown, click semantic END/MID directly in the model, never add a separate Position / END1 / END2 / MID selector**. Finally Owner explicitly insisted **not to split on-edge versus off-edge Point into two user-facing Datum tools**; one tool and automatic, exact geometric interpretation. After that precise proposal, Owner answered **"akceptuję - kontynuuj"**.

**Accepted UX principle:**
1. Continue with the **single existing Part `Datum Plane` tool** and one `Constructor` value, provisionally called **`Edge + Point`**; keep `Offset` unchanged. The name `Edge + Point` is a proposal, not a separately approved final label.
2. Acquire the **source material Edge** and then one **semantic Point** in the **3D Viewport**, using clear hover/highlighting and direct **END/MID point snaps**, without extra panel toggles or separate on-edge/off-edge modes.
3. On a valid second pick, compute the single appropriate plane interpretation; show a **transient preview**; preserve normal reversal, Finish/Cancel, Edit/repair through the same Datum Plane workflow.
4. `END`/`MID` labels are **semantic acquisition intents**, never screen-position CAD identity. A pick that has no proven unique durable meaning is rejected clearly.

**Still open for actual separate Owner UX approval under parent §3:** presentation/shape/color/priority of END/MID markers, overlapping corner-Edge disambiguation, highlights, source-acquisition state/help text, focus/keyboard semantics, visual diagnostic/repair interactions, exact choice/placement of Reverse and Finish, and command-line parity. **No Viewer/Tree/Operations/Command Line UI code may start until these are reviewably presented and explicitly accepted.** This recorded narrow interaction agreement must not be claimed as blanket full UX approval.

## 2. Exact meaning proposed for this one constructor

User sequence: **Datum Plane → Constructor: Edge + Point → click a strict stage-bound material Edge → click a uniquely resolvable Point**. The second click is authoring intent only if the chosen semantic source and exact evaluated native geometry prove a unique answer; no proximity heuristic chooses a construction.

| Exact semantic/geometry predicate | Derived meaning | Result |
| --- | --- | --- |
| Typed **END** or **MID** belonging to the **selected bounded material Edge**, certified at its declared Body stage | **Perpendicular** plane through that exact point, normal parallel to the source Edge tangent at the point | Admit when unique and nondegenerate |
| Point from another certified semantic source, **not on the supporting infinite line of a straight selected Edge** | Plane **containing the infinite straight Edge line and the point** | Admit when both sources are strict, stage-compatible and nondegenerate |
| Point is collinear with the supporting infinite straight line but **not an authored END/MID on the selected bounded Edge** | Cannot determine a unique through-line-and-point plane; cannot silently infer an on-edge snap | **Typed rejection** |
| Chosen point is on source Edge but not an admissible authored END/MID (or authorship/identity is ambiguous) | No approved arbitrary edge-parameter or guessed point role | **Typed rejection**, unless a later Owner contract explicitly expands points |
| Source is **open circular arc**, second Point is certified source **END** or **MID** | Perpendicular plane with normal parallel to **analytic tangent at chosen trim endpoint or half-arc-length point** | Candidate, subject to exact native E1/E2 evidence |
| Source is curved, Point is off Edge or on an unsupported source class | **No automatic alternate plane fitting**; curve-plus-point is not interchangeable with straight-line-plus-point | **Typed rejection** |
| **Closed/periodic circle**, missing, stale, blocked, unsupported or multiply resolved Edge/Point, or mismatched Body stage | No stable unique permissible meaning | **Fail closed**; no authored mutation |

Geometry classification is **not a pixel-distance threshold**: use exact provider/kernel geometry plus typed source role, accepted numeric tolerance policy and semantic stage-aware references. Same world XYZ with different Feature provenance is NOT sufficient. No switch hidden in the Operations panel.

**Proposed frame constraints, not yet detailed D2:** for perpendicular, position = exact source END/MID; normal = semantic canonical tangent (Reverse flips orientation, not location). For through-line-plus-point, preserve the straight Edge line in the plane; construct an unambiguous right-handed local plane frame from an authored semantic Edge direction and the vector toward Point, and reject collinear/degenerate cases. Do **not** let OCCT traversal, camera, tessellation or arbitrary endpoint numbering set persisted orientation. Decide exact `Reverse` frame behavior and stage/versioned persisted source representation in a subsequent precise architecture review; do not invent them as if already approved.

## 3. Native semantic reference boundary — the hard blocker to solve before product code

- Reuse **strict `MaterialEdgeReference` and `BodyStageRef`** where demonstrably valid, including the actual bounded material branch, never a representation seam/partition or a runtime `TopoDS_Edge` token.
- G0 proved that some visible native Vertex endpoints are **not** uniquely referenceable using existing **three-Surface `FeaturePointAddress`**; Fillet/Chamfer 3/13 current Vertices each Unsupported, full Revolve 2/2 Unsupported and analytic half-cylinder 4/6 Ambiguous. **On-screen pickable != persistently authorable**. First native-check whether source Edge END can be semantically certified for the actual target fixtures. A generic END must NOT be universally declared working, inferred from `XYZ`, a `TopoDS_Vertex`, OCCT orientation or geometry similarity. If proof requires new durable Vertex discrimination, **STOP for Owner D2** instead of covertly importing deferred G1.
- Exact `MID` means **half the real bounded material Edge length**; for an open circle arc use **half bounded arc length** and its exact tangent. No display-segment midpoint. Reject full closed circle MID.
- Selected Edge and Point must have a coherent explicit **Body history stage**; earlier upstream changes re-evaluate them but later Chamfer/Fillet must not silently redirect the authored reference to a new final-stage Edge. Preserve lost/ambiguous typed statuses, cold provider replay and no last-good ghost geometry.
- Need bounded tests showing whether a **point from a different Edge** has a sustainable semantic address at that declared common stage; do not silently assume all visual END/MID candidates are legal.
- A future persistent model/schema/API/serialization version change is **D2 architecture**, requires exact owner acceptance and migration/refusal policy before any code. This draft chooses no new schema or identity representation.

## 4. One bounded executable package — PROPOSED, not yet accepted for code

**In-scope only after independent Owner gates:** reuse the existing Datum Plane/Offset lifecycle and add exactly one `Edge + Point` constructor (the on-edge and off-line straight-Edge interpretations are two semantics of that **same** user workflow), with:
- acquisition/preview/Finish/Cancel without authoring on hover; one atomic Finish/Undo;
- stable `DatumId`, source-bound Edit/resourcing and typed diagnostics `Missing/Ambiguous/Unsupported/Blocked`;
- update under earlier Feature edits, stage preservation after later Features, Save/Close/Reopen and genuinely fresh OCCT provider cold rebuild;
- regression-safe unchanged Offset behavior, dependent Datum-backed Sketch/Profile → existing Extrude composition where input support is valid;
- kernel-native exact analytic line/arc, END/MID, proper tangent/frame, full circle/degenerate/ambiguity/refusal cases, and desktop UI workflows AFTER UX approval;
- Windows FOCUSED on relevant existing registered targets, FAST if CI policy requires it, final exact-head Windows FULL, as-built Internal, PL/EN Product + regenerated Browser, and independent Owner manual Windows acceptance.

**Unknown implementation allowlist:** must be audited and listed **exactly** (existing Part/domain/kernel/application/Qt UI, persistence/docs/test files) before executable contract acceptance. No "any `src/**`" catchall permission. If two interpretations cannot be supported without a larger independent ref/point contract, **STOP** to seek a narrower evidence-first amendment rather than silently splitting user tools or activating G1.

## 5. Explicitly OUT / preserved independent gates

- No new standalone Datum Point; no generic free point placement or arbitrary edge parameter, B-Rep polygon centroid, XYZ persistence, nearest-Vertex auto-repair, generic geometry similarity ID, broad point/Vertex identity overhaul or all-Vertex support claim.
- No additional Datum constructors (3 Points, At Angle), no Axis additions, Mirror/Pattern, G1 on the strength of this proposal, or PM-06. Keep previously accepted directional commitments available for future *just-in-time* decisions, not bundled into this execution.
- No RG-02 UI implementation before **full Owner UX consultation and explicit approval**; the accepted "one constructor + viewport point acquisition" principle covers only those interactions.
- No automatic activation or source/test mutation while `ACTIVE.yaml` points to PG-01E; first accept this exact bounded executable contract, merge its governance PR under a new explicit authorization, then separately Owner-authorize and merge a dedicated `ACTIVE.yaml` activation.
- **PR #344** post-merge v1.33 metadata correction remains a **separate, unmerged** Owner gate; no implicit merge consent is inferred.

## 6. Unresolved decisions for executable Work Contract finalization

1. **Native END and MID exact referenceability:** proof and legal small fixture class per strict `MaterialEdgeReference`, with ambiguous/missing stress cases. Is a new typed point semantic variant required? If so stop for Owner D2.
2. **Exact frame orientation and Reverse meaning** consistent across straight lines, arc tangent endpoints and persisted rebuild; reject orientation driven by provider order.
3. **3D marker / selection UX detail and priority** at shared corners; mockups/scenarios to Owner and explicit independent approval.
4. **Complete file allowlist, schema version and migration/refusal impact** after inspecting the accepted code and test anchors. No guessed code edit targets.
5. **Numerical tolerances and concrete expected results** as exact native fixtures; mark everything `UNRUN` until verified.

## Documentation impact

Internal docs: **required** on later accepted execution — Part strict Edge/Point/Stage semantic references, exact geometry, persistence and diagnostics.  
User/Product docs: **required** on later accepted execution — Polish/English `Datum Plane → Edge + Point`, direct model picking, both exact interpretations and rejection cases.  
Reason: the future feature changes semantics, lifecycle and user-visible workflow; deterministic generated `docs/browser/index.html` must then be refreshed and verified. **This draft alone** is a work-only proposal and does not amend existing as-built claims or require Browser regeneration.

**Owner gate:** this is **not** Owner-approved executable code scope; request review of this *specific narrow contract*, resolve the listed blockers/complete UX, then seek **separate approval to merge this draft PR**. Only separately authorized downstream activation may begin implementation.
