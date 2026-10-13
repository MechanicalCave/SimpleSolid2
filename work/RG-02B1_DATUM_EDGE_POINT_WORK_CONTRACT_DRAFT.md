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


## 1B. Reviewable UX flow for Owner consultation — PROPOSED, NOT APPROVED

This is a **remaining full-UX review artifact**, not an amendment to the already accepted one-constructor/any-order/automatic-overlap direction in §1/§1A. All state text, focus/Esc policy and Edit/repair choices below are *candidates* requiring separate explicit Owner approval under parent RG-02 §3; no UI code authority.

| Transient state | Operator action | Proposed Operations/help feedback | Allowed system effect |
| --- | --- | --- | --- |
| S0 AwaitEither | Enter existing Datum Plane, select Constructor: Edge + Point | "Wskaż krawędź lub punkt (END/MID)" (draft copy) | No authored change; existing Offset remains untouched. |
| S1E HaveEdge | Click certified material Edge first | Existing selected-Edge highlight; "Wskaż punkt" | Hold current strict Edge/stage in tool draft, never durable Viewer token. |
| S1P HavePointCandidates | Click END/MID candidate(s) first | Existing point marker, **identical** for MID; "Wskaż krawędź" | Retain *all* stage/current-generation eligible hit candidates as operation-local transient set, not one hover-displayed token. |
| S2 ReadyPreview | Select complementary source; unique semantic relation and exact geometry succeed | Highlight chosen Edge/Point; show plane with existing Reverse / Finish / Cancel affordances | On-Edge certified END/MID wins over unrelated overlapping hits; otherwise exactly one independently certified off-line Point may drive straight-Edge interpretation. Preview only. |
| S1 Unresolved | Complementary pick stale, unsupported, nonunique, stage-incompatible or degenerate | Short typed reason, request replacement of invalid source (copy pending) | No preview/commit/Undo. Keep a valid current source if legal, reset stale source if necessary. |
| S3 Committed | Finish on still-current valid pair | Standard completion / Tree selection | Revalidate Part/session/stage/semantic pair/geometry at execution; one Command / Transaction and one Undo. |
| S0 Cancelled | Cancel or ordinary tool exit | Existing cancellation UX | Drop transient picks and preview; no persistent mutation or Undo entry. |

**Specific unapproved UX details for independent Owner review:**

- Re-picking a source replaces that *transient* source and invalidates preview; the opposite source remains only while fresh and stage-compatible. Editing an existing broken Datum preserves DatumId and authored references until a successful atomic Finish; highlight and permit replacement of the offending source without silent repair.
- Hover is highlight/preview only; no new END-versus-MID colors or shapes, no overlapping-point chooser and no extra on/off-edge mode. Automatic filtering is semantic, not a screen-nearest choice.
- Reverse changes orientation, **not** the sourced point or its semantic identity. Exact U/V/N and persisted orientation remain D2 OPEN; this proposal does not approve a frame convention.
- Reuse the existing global CAD input priority and Command Line semantic routing. **No invented Esc/Enter override:** exact source-clear vs cancel behavior, text-buffer precedence, focus restoration and keyboard equivalents need an Owner review against shipping input grammar. Command Line must eventually resolve the same strict semantic pair and never serialize raw screen XYZ.
- A visible but semantically Unsupported END must report unavailability instead of pretending to create a durable Datum. MID can only be eligible where bounded analytic geometry and the typed Edge+MID source are proven.

**Full Owner UX approval remains OPEN:** copy and source-stage feedback, focus/keyboard/Esc, source replacement during Edit, typed diagnostic presentation, Reverse UX and Command Line parity. Compare existing Offset Operations panel with this state flow before any UI mutation.

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


## 3A. Current-source technical audit — READ-ONLY, 2026-10-13

**Audit baseline and evidentiary limit:** GitHub `main` exact SHA `b7da33653b8bc6214fd5774a34498838decb19cd` (post-roadmap #343, before separately gated metadata PR #344). **Source inspection only; NO code execution, new native test, Windows manual test, implementation or product proof** was performed. The lines below identify the existing concrete contracts, specific missing links and safe candidates for a later exact implementation allowlist.

| Concern | Verified production source / existing foothold | Finding and required action |
| --- | --- | --- |
| Strict Edge intent and stage | `src/part/include/simplesolid2/part/semantic_topology_reference.hpp:143–245`, `src/part/include/simplesolid2/part/feature_evaluation.hpp:249–322` | `MaterialEdgeReference` already stores `BodyStageRef`, semantic `FeatureCurveAddress` and strict `EdgeBranchDiscriminator`. `FeaturePointAddress` identifies exactly a three-semantic-Surface intersection, **not universal END identity**. Reuse instead of introducing an independent Edge ID. |
| Stage-specific Edge admission | `src/part/feature_evaluation.cpp:3864–4174`, `:4178–4303` | `resolveMaterialEdgeReference` requires complete exact matching stage and rejects removed/ambiguous/unsupported branches, representation seams/partitions. `authorMaterialEdgeReference` requires two **certified semantic endpoint Points** for multi-piece Curve branches; a single Curve may use `SingularAtAuthoredStage`. PG-01D-only `authorProjectedMaterialEdgeReference` is a specialized exception, **not blanket END authoring permission**. |
| Certifying semantic vertex | `src/part/feature_evaluation.cpp:1127–1173`; `src/part/include/simplesolid2/part/feature_evaluation.hpp:163–243` | `semanticPointForRuntimeVertex` (currently internal helper) requires exactly one candidate `FeaturePointAddress`, one matching resolved runtime realization, and referenceable accounting. A Viewport-visible `RuntimeVertexToken`/XYZ cannot substitute. G0 native [evidence](RG-02A_G0_NATIVE_VERTEX_EVIDENCE.md) proves genuine Unsupported/Ambiguous VERTEX families. **The contract cannot promise arbitrary END** without a separate proof or Owner-approved precise reference extension. |
| Endpoint membership versus Point identity | `src/part/include/simplesolid2/part/feature_evaluation.hpp:219–246`; `src/part/feature_evaluation.cpp:4178–4289`; `tests/pm05b2_strict_edge_resolver_test.cpp:395–442` | `BodyVertexTopologyRecord.incident_material_edges` is **runtime-only** stage-scoped incidence, useful for matching *already independently certified* Point candidates to a chosen strict Edge. Existing regression explicitly forbids using Edge connectivity to disambiguate **one semantically Ambiguous Point**. **Thus only overlapping independently Resolved Point hit meanings may be reduced by Edge membership; intrinsic Point Ambiguous/Unsupported remains fail-closed.** |
| Viewer overlapping-pick stack | `src/viewer/include/simplesolid2/viewer/solid_presentation.hpp:90–175`, `src/ui/part_viewport_controller.hpp:572–594`, `src/ui/part_viewport_controller.cpp:4906–4970,5535–5740` | `BodyTopologyPickQueryResult.candidates` and a transient `BodyTopologyCandidateStack` **already exist** with generation and viewer candidates. Ordinary click currently chooses **one** displayed/cycled `PresentationToken`. For the accepted point-first operation, an **operation-owned transient snapshot of ALL eligible Point candidates** (with generation) and strict per-stage certification is still needed; don't persist the ordinary chosen token or assume current click handler exposes all candidates to a new Datum draft. |
| Existing Body END UI versus MID | `src/ui/part_viewport_controller.cpp:2900–2940,4075–4110`, `src/kernel/include/simplesolid2/kernel/solid_modeling.hpp:150–231` | Current Body presentation contains runtime Vertex points and sampled Edge paths, and Body picking handles Face/Edge/Vertex; reuse the **existing END/vertex marker visual style** exactly as Owner directed. The existing `sketch::SnapKind::midpoint` / Sketch marker mapping (viewport cpp:383–413) concerns **Sketch**, not an already implemented semantic **Body MID**. No claim that source-complete 3D MID snap already exists. |
| Exact Kernel geometry capability | `src/kernel/include/simplesolid2/kernel/solid_modeling.hpp:130–231,576–643`, `src/kernel_occt/solid_modeling_kernel.cpp:2577–2592,5920–5995` | Provider reports line/circle and current runtime Edge/Vertex observations, OCCT already uses `BRepAdaptor_Curve`, `TopExp::Vertices`, `BRep_Tool::Pnt` in **bounded Edge projection**. However, no **general strict-edge, three-dimensional, provider-neutral point-at-END/MID + world tangent + exact bounded arc-half-length query** is exported by the current `ISolidModelingKernel` contract. Need a bounded, typed analytic query *after* stage/Edge certification; **not** reuse 2D projected Edge output or tessellated `BodyEdgePresentationPath.points` as authored point geometry. |
| Datum authored shape and evaluator | `src/part/include/simplesolid2/part/datum.hpp:45–93`; `src/part/datum_evaluation.cpp:130–266`; `src/application/include/simplesolid2/application/datum_plane_draft.hpp:23–112` | Existing product models `OffsetDatumPlane {DatumId, PlaneReference, LengthValue offset, visible}`, and draft tracks a **single PlaneReference + signed Offset**. It does **not** currently represent an Edge+Point source pair or two geometric interpretations. Retain stable `DatumId`/Offset behavior but **do not force Edge+Point into `PlaneReference` or encode Point as XYZ**; propose an explicit bounded typed constructor model for Owner D2 review. |
| Persistence and lifecycle | `src/part/part_document_store.cpp:1179–1202,2268–2349`; `src/part/part_document.cpp:1–135` | Native Part currently serializes Datum planes as **`kind: "offset_plane"`** with precisely 5 required fields. Parser accepts **only** that Datum kind, irrespective of other model-version support. A second constructor requires deliberate **versioned storage/change policy, old file preservation, typed refusal or migration rules**; old current v16/modeling-v2 files must remain safely readable. History/provenance checks already exist for semantic references but must extend to the new authored Datum source. |
| Existing verification footholds | `tests/pm03b_datum_evaluation_test.cpp`, `tests/pm03c_datum_commands_draft_test.cpp`, `tests/pm03c2_cad_workbench_datum_plane_test.cpp`, `tests/pm03a_datum_schema_v10_test.cpp`, `tests/part_document_store_test.cpp`, `tests/pm05b2_strict_edge_resolver_test.cpp`, `tests/kernel_native/pm02p_d_vertex_point_lineage_test.cpp` | Existing registered tests cover older Offset/Edge/Point obligations **only**; no test of new EDGE+POINT End/Mid authored semantics has been run. Actual native analytic query tests must use an already registered appropriate kernel-native target after allowlist review. |

### 3A.1 Audited next implementation decisions — NOT silently approved

**E0 first, with no feature/UI changes:** Under a separately approved evidence-only test allowance (if needed), prove in actual OCCT Body-stage fixtures:
1. A currently resolvable straight **material Edge** has defensible, **strict on-this-Edge** END roles, including both standalone and shared-corner runtime Vertices, even where the **Vertex's global `FeaturePointAddress`** is unavailable. If not provable from existing Part semantics, stop for precise Owner D2; do **not** quietly deliver only MID or silently import full deferred G1.
2. Analytic **MID** of a bounded straight material Edge and an open trimmed circular arc, including correct arc half-length and tangent, can be recomputed from the **same strict Edge reference at the selected Body stage** without relying on tessellation or runtime orientation. Full closed circle MID and periodic seams fail closed.
3. `Point → Edge` hit candidates represent genuine distinct independently certifiable meanings, and `Edge → Point` returns the same selected authored pair. When **one `FeaturePointAddress` itself resolves Ambiguous**, Edge incidence MUST NOT promote it to Resolved (existing PM-05B2 safety invariant).
4. Off-line Point for a straight Edge has an **independent, stage-compatible certified source**; same screen pixel or XYZ coincidence does not imply same semantic role. Prove both available and excluded cases.

**D2 decision before product/schema code:** Choose exact stage-aware typed point-source role(s), deterministic END endpoint roles (stable across native `TopAbs_REVERSED` and cold rebuild), semantic plane frame + Reverse representation, and versioned new Datum constructor/persistence compatibility. This audit establishes **need for a design decision**, not the eventual selected type/algorithm.

**Existing implementation candidate paths for later allowlist review** (NOT authorization to edit): `src/part/include/simplesolid2/part/{semantic_topology_reference.hpp,datum.hpp,datum_evaluation.hpp,feature_evaluation.hpp}`, `src/part/{datum.cpp,datum_evaluation.cpp,feature_evaluation.cpp,part_document.cpp,part_document_store.cpp}`, `src/kernel/include/simplesolid2/kernel/solid_modeling.hpp`, `src/kernel_occt/solid_modeling_kernel.cpp`, `src/application/{datum_plane_draft.cpp,include/simplesolid2/application/datum_plane_draft.hpp}`, `src/ui/{part_viewport_controller.cpp,part_viewport_controller.hpp}`, plus exact existing Qt workbench controller/Panel files **to be inventoried**, applicable already registered tests, and mandated docs. **This is not a wildcard edit permission**; a final executable contract must name exact permitted paths, finite fixture matrix, staged CI and Owner UI acceptance.

**Delivery planning constraint:** One eventual **user-visible `Datum Plane → Edge + Point` constructor** with **both interpretations in the same workflow**, not two separately shipped features. Read-only native E0 can be a gate **inside one bounded contract**; further scope expansion requiring a new persistent Vertex identity is a hard STOP and a fresh Owner amendment, not a covert implementation detour.


## 3B. Next evidence-first gate — source-backed E0 native characterization PROPOSAL (UNRUN)

**Purpose:** test whether present Body END/MID and a strict material Edge can support durable construction intent. This is a future *test-only native evidence proposal*, not activated by this Draft, accepted UX direction, or a generic instruction to continue. No production/tests/CI/schema changes are made in PR #345.

| Witness | Native input and required assertion | Refusal / distinction |
| --- | --- | --- |
| E0-01 straight END | Real OCCT rectangular Extrude current Body stage; select one strict material Edge, check both native endpoint statuses and actual material incidence; independently certify END addressability or mark Unsupported. | Never infer identity from token, B-Rep orientation, first/last iteration, XYZ or sorting; G0 V01 8 Resolved is only a bounded fixture. |
| E0-02 coincident corner | Two different certified straight Edges meeting at one displayed Vertex; retain independently certified candidate meanings, compare Edge→Point vs Point→Edge under strict Edge membership. | A single intrinsically Ambiguous FeaturePointAddress **stays Ambiguous**, even when Edge incidence appears to distinguish provider vertices. |
| E0-03 straight MID | Certified bounded straight material Edge: analytic half-length, tangent, reproducible stage-bound cold evaluation. | No tessellation midpoint, screen coordinate, traversal-defined persisted sign or guessed parameter. |
| E0-04 open arc END/MID | Native real OCCT trimmed circular material Edge: exact half **bounded arc-length**, tangent, endpoint meaning and strict source stage. | Closed circle MID, seams/partitions and unsupported curves fail closed; a 2D projection is not a 3D source query. |
| E0-05 independent off-line Point | Strict straight Edge plus one independently certified noncollinear Point at one common valid Body stage; verify both pick orders. | Collinear non-END/MID rejects; visually equal XYZ is not identity; multiple off-line meanings reject. |
| E0-06 hard negative sources | G0 half-cylinder Ambiguous, full Revolve Unsupported, Fillet/Chamfer Unsupported cases: inspect strict Edge and Point certification at actual stage. | Resolved Edge does not automatically promote Unsupported/Ambiguous Vertex to valid END; no implicit G1. |
| E0-07 stage/lifecycle | Edit upstream dimension, evaluate later Chamfer history, simulate source disappearance and native orientation variation, rebuild with fresh provider. | No stale last-good, nearest XYZ or later-stage auto-rebinding. Distinguish geometric feasibility from semantic source resolution. |

**Suggested proof order:** first E0-01/02/06 strict identity and incidence, then E0-03/04/05 exact provider-neutral query requirements, then E0-07 stage/cold lifecycle. Existing G0 witnesses prove catalog classification, **not** the new Datum END/MID semantics. All proposed E0 cases remain **UNRUN**; no CI PASS claimed.

**Authority before any test mutation:** a separately accepted bounded evidence-only Work Contract (or expressly accepted amendment) must enumerate exact *already registered* native test paths, any permitted fixture/helpers, expected results, exact HEAD/tier verification and explicit exclusions. No wildcard tests/**, new targets, CI/CMake edits, schema, Kernel API, UI or product changes may be inferred from this proposal.

**D2 branching decision after native evidence:**

1. If concrete source ENDs independently resolve and MID can be anchored strictly to Edge+role, propose the smallest typed, stage-aware Point-source variant, analytic Kernel query and deterministic frame/Reverse/schema compatibility to Owner. This is **not** a schema decision today.
2. If required END is Unsupported/Ambiguous, do not author a Viewer-only substitute. Compare a **bounded provenance-backed Edge-local endpoint selector**, a deliberately narrower supported fixture scope, and a separately authorized deeper provenance investigation. An arbitrary provider END1/END2 index or deferred G1 implementation is not allowed.
3. If overlap certification, open-arc half-length, stage-aware geometry or cold persistence requires out-of-scope production changes, record exact evidence and STOP for Owner D2 rather than covertly shipping MID-only or splitting into two user tools.

**Single point of failure:** a source Edge can be unambiguously durable while its visible endpoint is not. The Datum needs **both** source meanings; successful Viewer picking does not close the gap.

## 4. One bounded executable package — PROPOSED, not yet accepted for code

**In-scope only after independent Owner gates:** reuse the existing Datum Plane/Offset lifecycle and add exactly one `Edge + Point` constructor (the on-edge and off-line straight-Edge interpretations are two semantics of that **same** user workflow), with:
- order-independent `Point → Edge` and `Edge → Point` acquisition, reuse of existing END and Edge visualizations for identical MID marker, automatic overlap filtering (§1A), preview/Finish/Cancel without authoring on hover or transient picks; one atomic Finish/Undo;
- stable `DatumId`, source-bound Edit/resourcing and typed diagnostics `Missing/Ambiguous/Unsupported/Blocked`;
- update under earlier Feature edits, stage preservation after later Features, Save/Close/Reopen and genuinely fresh OCCT provider cold rebuild;
- regression-safe unchanged Offset behavior, dependent Datum-backed Sketch/Profile → existing Extrude composition where input support is valid;
- kernel-native exact analytic line/arc, END/MID, proper tangent/frame, full circle/degenerate/ambiguity/refusal cases; explicit future E1–E6 pair-order/overlap native and desktop verification (§1A), with any desktop UI workflows only AFTER separate full UX approval;
- Windows FOCUSED on relevant existing registered targets, FAST if CI policy requires it, final exact-head Windows FULL, as-built Internal, PL/EN Product + regenerated Browser, and independent Owner manual Windows acceptance.

**Implementation allowlist remains UNAPPROVED:** §3A identifies concrete **candidate touchpoints** from current source audit, but the final exact allowed-file list, registration of native test target and versioning contract require separate Owner D2 acceptance before code. No "any `src/**`" catchall permission. If two interpretations cannot be supported without a larger independent ref/point contract, **STOP** to seek a narrower evidence-first amendment rather than silently splitting user tools or activating G1.

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
