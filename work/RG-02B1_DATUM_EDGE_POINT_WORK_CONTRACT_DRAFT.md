# RG-02B1 — Datum Plane: one Edge + Point tool (DRAFT bounded Work Contract)

**Status:** **DRAFT FOR OWNER REVIEW; NOT MERGED, NOT EXECUTABLE, NOT ACTIVATED.** The **user interaction principle** below is **Owner-accepted** on 2026-10-13; exact technical schema, native proof, complete visual/keyboard UX, implementation file allowlist and this Work Contract's executable scope are **NOT yet independently Owner-accepted**.  
**Direction:** Owner chose development of Datum Plane + Axis as the **sole next planning focus** in accepted and merged Part-roadmap v1.33 ([PR #343](https://github.com/MechanicalCave/SimpleSolid2/pull/343)). Start with **existing Datum Plane**; do not use this contract to start unrelated Axis constructors, G1, Mirror/Pattern or PM-06.  
**Authoritative baseline:** `main` `b7da33653b8bc6214fd5774a34498838decb19cd`. Existing `work/ACTIVE.yaml` references **completed PG-01E** for traceability only and `program_roadmap_version: "1.32"` until a separate metadata correction ([PR #344](https://github.com/MechanicalCave/SimpleSolid2/pull/344)) is independently approved and merged. Do **not** merge/activate #344 as a side effect of this draft.  
**References:** `work/RG-02_REFERENCE_GEOMETRY_EXTENSION_WORK_CONTRACT_DRAFT.md` (Owner-accepted broad product direction, NOT executing contract); `work/PM-03_DATUM_REFERENCE_GEOMETRY.md` (completed Offset), `work/PM-04_AXIS_REVOLVE.md` (completed Axis/Revolve), `work/RG-02A_REFERENCE_FOUNDATION_AUDIT_DRAFT.md`, and `work/RG-02A_G0_NATIVE_VERTEX_EVIDENCE.md` (Owner-accepted G0 native limitations, PR #332 merged); Constitution, Foundation 1.0, accepted ADRs, current roadmap and DOC-01.

## 1. Owner-accepted interaction direction (bounded UX principle, NOT the full RG-02 UI gate)

Owner supplied a screenshot of the **existing Datum Plane** Operations panel with `Constructor: Offset`, then proposed developing **Edge with on-edge END/MID** before other Datum constructors. When offered two distinct interpretations:
- A: a plane **through a straight Edge and a point outside its supporting line**, versus
- B: a plane **perpendicular to an Edge at END/MID**,

Owner first explicitly chose **B**. Owner then explicitly refined the user experience: **only choose the Datum constructor in the existing dropdown, click semantic END/MID directly in the model, never add a separate Position / END1 / END2 / MID selector**. Finally Owner explicitly insisted **not to split on-edge versus off-edge Point into two user-facing Datum tools**; one tool and automatic, exact geometric interpretation. After that precise proposal, Owner answered **"akceptuję - kontynuuj"**. In a further screenshot-backed clarification on 2026-10-13, Owner specified **reuse current hover/select point, Edge and Face presentation; MID looks identical to existing END; accept either point-first or Edge-first input; resolve coincident point picks automatically by the subsequently chosen Edge**, with no UI disambiguation. After the exact rules were restated, Owner answered **"akceptuję - kontynuuj"**.

**Accepted UX principle:**
1. Continue with the **single existing Part `Datum Plane` tool** and one `Constructor` value, provisionally called **`Edge + Point`**; keep `Offset` unchanged. The name `Edge + Point` is a proposal, not a separately approved final label.
2. Acquire a **source material Edge** and a **semantic Point** in the **3D Viewport, in either order** (`Edge → Point` or `Point → Edge`). Reuse current hover/select Edge/Face/END presentation; show `MID` with **the same point marker as END**. No extra panel toggles, Point-kind selectors, overlap-candidate list or separate on-edge/off-edge modes.
3. The second pick completes the selection pair. Match any overlapping visible Point candidates against the **chosen Edge** by **strict semantic Edge/Point membership**, not by the visually displayed candidate; automatically select a **unique eligible** relation and show the **transient preview**. Preserve normal reversal, Finish/Cancel, Edit/repair through the same Datum Plane workflow.
4. `END`/`MID` labels are **semantic acquisition intents**, never screen-position CAD identity. A pick that has no proven unique durable meaning is rejected clearly.

**Accepted for UX direction:** existing END marker reused for MID, existing selection/hover visualization reused, and coincident Point candidates filtered **automatically after Edge selection**, independent of pick order. **Still open for the mandatory separate full UX approval under parent §3:** precise state/help text, hover-to-snap hit radius/priority where current behavior lacks a proven rule, focus/keyboard grammar, typed ambiguity/repair feedback, Edit source replacement, exact Reverse/Finish interaction and Command Line parity. **No Viewer/Tree/Operations/Command Line UI code may start until the complete applicable design is reviewably presented and explicitly accepted.** This accepted interaction amendment is not a blanket production UX sign-off.


## 1A. Owner-accepted Edge/Point acquisition order and coincident points (2026-10-13)

**One constructor, any pick order.** The tool must not force the operator to know point topology or select an explicit END/MID mode; both source picks are acquired using the existing model Viewport semantics. The *visible* hover marker may be whichever coincident point is normally presented — it is not itself durable selection authority. MID uses **exactly the existing END point visual style**, not a new icon/color or extra panel control. The highlighted source Edge remains visually identifiable using the existing Edge preselect/select behavior.

**`Edge → Point` (Edge first):**
1. Capture a uniquely admissible semantic **source material Edge** and its explicit Body stage. An unsupported or ambiguous Edge cannot become authored source.
2. Hover and click a visible candidate END/MID or independent model Point. If multiple Point hits occupy the same display location, **first filter by exact semantic membership/role on the already-selected Edge**. A unique eligible source-Edge END/MID wins over unrelated coincident hits; the arbitrary hover-displayed point does not decide CAD identity.
3. If the accepted point is a unique END/MID **of that Edge**, derive the **perpendicular** plane. If none belongs to that Edge, an **independently and uniquely certified off-line Point** may drive the straight-Edge-through-Point interpretation; do not guess among unrelated coincident off-line meanings.
4. Show preview **only after** the selected Edge plus unique certified Point yield exactly one valid supported Datum interpretation.

**`Point → Edge` (Point first):**
1. A click on a visually overlapping point retains **a transient set of semantic hit candidates**, not whichever individual candidate happened to be displayed by hover. The candidates are scoped to the current Part/selection context and are **not persisted as authored references**; every source must be current/stage-valid when consumed.
2. Click the material Edge. Resolve its strict identity and **intersect/filter** the candidate set with certified END/MID semantic relations belonging to **this Edge**. If one eligible source-Edge relation is uniquely proven, select it automatically and present the **perpendicular plane preview**, even when a different coincident Point was shown at the first click.
3. If **none** belongs to the selected Edge, do not pick a coincident point arbitrarily: only **one independently certifiable, noncollinear off-line Point** permits the straight-Edge-through-Point plane. Otherwise show a short explicit unresolved/unsupported diagnostic with **no preview**. A missing on-Edge membership must not automatically become off-line merely because the viewer shows an offset/matching XYZ.
4. If multiple remaining candidate relations are truly distinct in **persisted semantic meaning** or the selected Edge cannot be certified, **fail closed**, keeping the draft editable and allowing fresh picks. No arbitrary first-hit, list-order, pixel-nearest or provider-order fallback.

**Geometry and safety:** UI hit overlap is only a way to collect candidates. The **exact authored Edge/Point relation** (same declared Body stage, strict material branch, admissible END/MID role and source identity) makes the choice. Screen/pixel proximity, same XYZ, equal B-Rep token, viewport draw order or coincident geometry **never independently establishes persistent identity**. De-duplicate genuinely equivalent hit paths to the *same proven semantic reference*, but do not collapse different references just because they render at one position. A stale/lost/unsupported/ambiguous hit never becomes a legal source merely because some geometry looks plausible.

**Transient/revalidation rule:** no authored mutation on hover, first pick or preview. A complete valid pair may display a temporary plane; `Finish` revalidates both live semantic sources/Body stage and the single deterministic interpretation before one atomic Datum commit. Failed, cancelled or revised picks do not create Undo entries. Identical rules apply to initial Create and Edit/re-source drafts, with Undo/Redo and cold reconstruction preserving the chosen authored meaning, not the temporary hit list.

**Minimum future verification matrix (all UNRUN until independently executed):**
- E1: `Edge → source MID` and `Point MID → Edge` yield the same certified Datum/frame for a bounded straight Edge; the same for END where source identity is proven.
- E2: Shared visible corner with **two or more** overlay Point hits from distinct Edges; first-point hover shows an arbitrary one, then selecting Edge A uses **only A's valid END**; reverse order produces same result. Repeat for Edge B.
- E3: Source Edge plus independently certified **off-line** Point, both input orders, gives one through-straight-Edge-and-Point plane; no extra constructor UI mode.
- E4: Multiple truly distinct off-line Point meanings at one screen location: reject ambiguity; never pick displayed/first candidate. A source-Edge candidate, when uniquely present, wins over unrelated coincident candidates.
- E5: Missing, stale, unsupported, split/ambiguous branch, invalid MID/full circle, wrong Body stage and degenerate geometry: no preview/commit; typed diagnostic, no silent source reattachment.
- E6: `Finish/Cancel/Edit`, upstream edit versus later Chamfer, Undo/Redo, Save/Reopen and genuinely fresh OCCT provider preserve/fail-closed according to exact authored Edge and Point meaning, independent of pick order.

## 2. Exact meaning proposed for this one constructor

User sequence: **Datum Plane → Constructor: Edge + Point → select an Edge and a Point in either order**. Point-first may temporarily hold several overlapping semantic hit candidates; the selected strict Edge filters the candidates automatically. The completed pair is admissible only when semantic source certification and exact evaluated geometry prove a **unique** legal result (§1A); no visually presented hit or proximity heuristic chooses authored meaning.

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
- order-independent `Point → Edge` and `Edge → Point` acquisition, reuse of existing END and Edge visualizations for identical MID marker, automatic overlap filtering (§1A), preview/Finish/Cancel without authoring on hover or transient picks; one atomic Finish/Undo;
- stable `DatumId`, source-bound Edit/resourcing and typed diagnostics `Missing/Ambiguous/Unsupported/Blocked`;
- update under earlier Feature edits, stage preservation after later Features, Save/Close/Reopen and genuinely fresh OCCT provider cold rebuild;
- regression-safe unchanged Offset behavior, dependent Datum-backed Sketch/Profile → existing Extrude composition where input support is valid;
- kernel-native exact analytic line/arc, END/MID, proper tangent/frame, full circle/degenerate/ambiguity/refusal cases; explicit future E1–E6 pair-order/overlap native and desktop verification (§1A), with any desktop UI workflows only AFTER separate full UX approval;
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
3. **Remaining UX details, not the already accepted overlap algorithm:** preserve existing END visuals for MID and automatic Edge-filtered point-first resolution (§1A); present keyboard/focus, current selection priority edge cases, user-facing state/help text, diagnostics and repair scenarios to Owner for separate complete UX approval.
4. **Complete file allowlist, schema version and migration/refusal impact** after inspecting the accepted code and test anchors. No guessed code edit targets.
5. **Numerical tolerances and concrete expected results** as exact native fixtures; mark everything `UNRUN` until verified.

## Documentation impact

Internal docs: **required** on later accepted execution — Part strict Edge/Point/Stage semantic references, exact geometry, persistence and diagnostics.  
User/Product docs: **required** on later accepted execution — Polish/English `Datum Plane → Edge + Point`, direct model picking, both exact interpretations and rejection cases.  
Reason: the future feature changes semantics, lifecycle and user-visible workflow; deterministic generated `docs/browser/index.html` must then be refreshed and verified. **This draft alone** is a work-only proposal and does not amend existing as-built claims or require Browser regeneration.

**Owner gate:** this is **not** Owner-approved executable code scope; request review of this *specific narrow contract*, resolve the listed blockers/complete UX, then seek **separate approval to merge this draft PR**. Only separately authorized downstream activation may begin implementation.
