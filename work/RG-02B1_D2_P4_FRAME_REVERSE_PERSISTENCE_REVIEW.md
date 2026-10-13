# RG-02B1/D2-P4 — stable signed Datum frame, Reverse and Part persistence (OWNER REVIEW DRAFT)

**Status:** **OWNER ACCEPTED P4 DESIGN DIRECTIONS IN PRINCIPLE, THEN CLARIFIED: IF SOURCE+GEOMETRY ARE CERTIFIED BUT ORIENTATION SIGN IS FREE (e.g. MID), SYSTEM MUST PROPOSE A VALID PREVIEW WITH A REVERSE CHOICE (2026-10-13).** Exact deterministic convention/continuity/persistence remain **DRAFT / UNMERGED / NOT ACTIVE / NO PRODUCT-CODE AUTHORITY.**  
**Prepared:** 2026-10-13; baseline `main` `b46bbe69b59d660f089967cba9b9f2e9a10ac029`; roadmap v1.33; `ACTIVE.yaml` still authorizes **only E0 four-test evidence**.  
**Prerequisite UX direction:** Owner accepted **in principle** source/gesture design P1/P2/P3/P5 in [PR #349](https://github.com/MechanicalCave/SimpleSolid2/pull/349) (exact corrected proposal HEAD `38f34f6ee378030db8c91503a2e799a8d5cb096a`); **no merge or executable semantics were authorized**. E0 measured facts reside in [PR #348](https://github.com/MechanicalCave/SimpleSolid2/pull/348), separate OPEN/UNMERGED, exact-head Windows FULL [#2828](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38392879099) 199/199 PASS; owner accepted these **limited findings as design input**, not a completed END/MID feature.  
**Request:** review P4 options below; **no option is Owner-approved by this Draft**. Parent [UX Draft #345](https://github.com/MechanicalCave/SimpleSolid2/pull/345) remains independent.

## 1. What P4 must preserve

The accepted gesture is **Datum Plane → Edge + Point** with Edge→Point and Point→Edge equivalent; at a common solid corner, choosing Edge B after a click that initially highlighted Edge A's Point must select the **END belonging to Edge B**. This is gesture disambiguation; durable meaning must later come from a **proven, provider-independent Edge-local terminal key**, never `TopoDS_Vertex`, native first/last, clicked XYZ or hover token. `MID` means half actual bounded material-Edge length and is Edge-local. An off-infinite-line Point of a strict straight Edge must independently resolve semantically; on-edge/off-edge has no user toggle.

These three sourced cases yield **derived** geometrical planes but their authored downstream meaning includes a reproducible **right-handed signed** metric `Frame3{origin, u_axis, v_axis, normal}`: an Offset Datum is already a legal Sketch support/chain, so a point and a tangent are NOT enough to guarantee correct edits, Sketch UV consistency, Reverse or cold rebuild.

**Verified current source boundaries:**
- `src/kernel/profile_input.cpp`: `Frame3::valid()` checks finite O/U/V/N, positive nonzero vectors, nonzero cross(U,V) and positive dot(cross(U,V),N). **It does not itself assert unit length, pairwise orthogonality or a fixed signed orientation across reevaluations.** A new Datum constructor must impose/test those stronger invariants; do not treat `Frame3::valid()` as the entire deterministic frame policy.
- `src/part/datum_evaluation.cpp`: existing Offset reuses a source canonical frame and shifts only O by offset along N. Existing derived `required_body_stage`, typed failure and current revision freshness are precedents; do not reorient existing Offset or reinterpret old Sketches.
- `src/part/include/simplesolid2/part/part_document.hpp` currently has `std::vector<OffsetDatumPlane>`, with `PartDocumentTransaction` governing atomic authored-state mutation.
- `src/part/include/simplesolid2/part/part_document_store.hpp` specifies **native Part schema v16**; `src/part/part_document_store.cpp` writes `kind: offset_plane` with exactly `id, kind, source, offset_mm, visible` and its existing loader rejects malformed/unknown kinds. A new variant must be versioned, never silently stuffed into those five fields.
- `src/kernel/include/simplesolid2/kernel/edge_projection.hpp` only supplies current-body-bound **2D** projected Edge geometry; the proposed exact 3D endpoint/MID/tangent query is **not currently implemented**.
- Accepted ADR-0014, ADR-0016 and ADR-0017 require semantic/stage provenance, fail-closed resolution, no provider/screen/nearest-XYZ identity, explicit modeling-semantics compatibility where outcomes change, and no representation-seam/partition promotion.

## 1A. Successful Point+Edge selection versus admissible Datum Finish (OWNER QUESTION; PROPOSED PRECISION)

**Owner question, 2026-10-13:** If the user successfully identifies a particular Edge and Point, why should the system be unable to establish its source semantics? The intended interaction must distinguish **three separate certificates**, while hiding unnecessary technical complexity from the user:

1. **Current exact gesture** (transient): click determines the Point/MID hit location and the specific selected material Edge, including the corresponding endpoint of **that Edge** when different coincident Edge/Point hits overlap. The first hovered Point belongs to no binding authority. A successful gesture is **not** automatically a durable source/reference or a complete signed plane frame.
2. **Durable source intent** (authored): identify the same Edge-local END terminal, Edge-local MID, or independently certified off-line Point in the same declared Body stage across dimension edits, re-evaluation and cold provider reconstruction. Current native endpoint coordinates/order are insufficient to establish this by themselves.
3. **Deterministic proposed signed frame** (derived): when sources and exact geometry are certified, the application must **choose a reproducible default** metric, orthonormal, right-handed `O/U/V/N` even if the intrinsic geometry admits ±tangent/±normal (common for MID). For straight Edge+END, current inward tangent (chosen END toward the other terminal) is a natural default, subject to durable source certification; for MID, choose a **software-defined canonical default** and display it. For Line+off-line Point, the geometric plane is unique but the sign of N is chosen by a deterministic policy. The user's **Odwróć / Reverse** changes the orientation by the separately accepted frame transform, **not the point or the selected Edge**. No extra source click or extra UI mode is needed.

**Owner-corrected user-visible invariant (binding UX direction; implementation UNAPPROVED):** when the strict Edge, selected END/MID/independent Point source, stage and exact nondegenerate geometry are certified, **the program should immediately propose and display a deterministic default oriented plane preview**, even if the source **does not prescribe the sign** of the tangent/normal. The user can consciously press **Odwróć / Reverse**, and see the normal and the corresponding signed plane orientation flip in the preview **before Finish**. A merely free orientation sign is **NOT** `UnsupportedOrientation` and must not block preview or Finish. **True identity uncertainty** (no durable END terminal provenance, intrinsically ambiguous independent Point, nonunique source Edge), invalid stage, no exact 3D query or degenerate geometry **still block a valid Finish**, with precise feedback; temporary speculative geometry, if ever shown, cannot be represented as a finishable, stable Datum. `Finish` must revalidate source and signed frame atomically. A previously valid datum may later become Missing/Ambiguous/Blocked after upstream changes. Full Preview/Reverse/Finish UI interaction details still require independent §1B UX review in [Draft #345](https://github.com/MechanicalCave/SimpleSolid2/pull/345).

## 2. P4-A — default preview orientation when source geometry has two signs (OWNER UX CLARIFICATION ACCEPTED; ALGORITHM DRAFT)

**Authoritative UX behavior:** **Do not equate orientation-sign freedom with an unresolved source identity.** For a valid strict Line/open Arc Edge and an exact supported MID, the 3D midpoint and *tangent line* can be uniquely determined although ±tangent are both legitimate normals for the same perpendicular plane. The application must choose **one valid default oriented frame** and show it, leaving the user the existing **Reverse** control to choose the opposite normal. Likewise Line + certified off-line Point defines one geometric plane even if its normal's sign is not inherently preferred. The same marker is used for END/MID and there is no extra selection mode.

**Proposed default orientation algorithm — engineering choice NOT YET APPROVED:**
1. When a stable signed direction is actually proven from the exact producer/terminal semantics, prefer it. **Do not require such a preference for MID preview or treat its absence as an error.**
2. Otherwise normalize the exact analytic tangent **as an unoriented line**, then canonically choose a sign using a **fixed, documented world-axis comparison order with a deterministic tie rule** (or a separately proven stable semantic frame seed). This uses world orientation **only as a reference for the derived frame**, never as durable **Edge/Point identity**, nearest-geometry rebinding, or OCCT traversal authority.
3. Choose U by projecting an ordered canonical seed axis into the plane normal to N and normalizing; compute V = normalize(N × U). Explicitly ensure U,V,N are unit, orthogonal, right-handed and stable on exact repeat/rebuild. **Use an explicit nonparallel seed/fallback policy** and handle geometric degeneracy separately; no random/camera/provider-first choice.
4. On creating the Datum, record the accepted **orientation choice** (Reverse boolean and, if required to avoid axis/tie discontinuities under edits, an authored orientation seed/policy or stable reference-axis choice) **separately from the durable semantic source identity**. If a future upstream edit crosses a sign/basis-selection boundary, avoid an unexplained flip: preserve the authored orientation intent where geometrically possible and cover unavoidable changes/failure with explicit accepted behavior and regression tests. The exact persisted orientation-seed representation and continuity rule still require a separate D2 implementation/spec decision.

**Important limitation:** a stateless "pick the global axis least aligned with N" policy is deterministic for unchanged geometry but can change U discontinuously after a tiny upstream edit. Thus a one-time plausible default preview and long-term Edit/Sketch stability are *distinct verification requirements*. Reverse cannot on its own correct an unrelated U-axis rotation. Specify the canonical sign, tie boundary and authored orientation continuity before product code. This is NOT a reason to reject ordinary MID preview solely because ±normal are mathematically equivalent.

**True STOP remains:** exact semantic Edge/END/Point source is unavailable, wrong stage, full circle/seam/partition, no provider-neutral exact 3D geometry or genuinely degenerate curve/plane, and missing durable terminal key required for Finish. No End0/End1 persistence, current runtime token/XYZ identity, camera-driven axes or imaginary completed native E0 MID proof.

## 3. P4-B — exact geometry and Reverse (R1 ACCEPTED IN PRINCIPLE; EXACT PRODUCT BEHAVIOR NOT YET AUTHORIZED)

For a **strict source-Edge END or MID**:
- `O` = exact 3D evaluated terminal point / half-bounded-length midpoint at that specific source stage.
- `N` = normalized exact 3D analytic tangent with the **deterministic automatically proposed sign** from P4-A (source-proven if available, otherwise canonical default); not a transient native OCCT first/last parameter sign.
- `U` = normalized projection of the **P4-A deterministic canonical/remembered reference seed** into the plane perpendicular to N; `V := normalize(N × U)`. Require `U·N`, `V·N`, `U·V` near zero, all norms near one, `U × V` aligned with `N`, signed right-handedness and fixed finite bounds. If one axis is parallel to N, a **specified deterministic alternative** is used, not an orientation-source failure. True curve/point/frame degeneracy still fails closed.

For **straight Edge + independently resolved off-line Point**:
- Anchor `O` = the source Edge's **proven canonical terminal**, where available, **not** native first vertex, profile-array ordinal or the closest point on infinite line; if no stable terminal anchor exists, adopt **only an explicitly specified source-derived canonical anchor/continuity policy**, never runtime order or XYZ used as identity. This durable anchor requirement is independent of the **automatically suggested sign** of the normal.
- `U` = normalized vector from the proven first to proven second Edge-local terminal, with signed provenance. `d` = exact current independent Point – O; require sufficiently noncollinear `U × d`. Let `N := normalize(U × d)`, `V := N × U`. The plane contains the whole supporting line and the Point. Source Point authoring and stage correctness are independent of Edge-end transient hit filtering.
- For a change that keeps the geometric plane but alters anchor O or signed basis, downstream local Sketch geometry changes; therefore O/U/V/N stability must be explicitly tested, not inferred from plane coincidence.

**Owner-accepted direction for Reverse R1 (exact code not authorized):** propose/persist one authored boolean `reverse` in the new Datum variant. The application **shows the calculated default frame immediately**. On pressing **Odwróć / Reverse**, its preview switches from `(O,U,V,N)` to `(O,U,−V,−N)`, i.e. a 180° frame rotation about U, keeping right-handedness, chosen source Edge, END/MID/independent Point, stage, O and U unchanged. Pressing again restores the default preview. A committed orientation choice survives Undo/Redo and Save/Reopen. **No extra source picking is required** and the user can inspect a reversed preview before Finish. Do not flip N alone, which would break handedness.

**Numeric/semantic STOP, not orientation-preference STOP:** zero/short Edge, periodic closed/seam Edge, trim ambiguity, MID without exact native 3D length/derivative query, near-collinear off-line Point, non-finite tangent, unresolvable chosen Edge/terminal/independent Point or stale stage => typed Unsupported/Ambiguous/Blocked; retain authored intent without stale cached frames. A candidate U seed **parallel to valid N is not by itself a STOP**: use a predeclared deterministic alternative reference seed. Free ±normal orientation is not a STOP either: show a proposed oriented plane and Reverse. Unavoidable orientation discontinuity through large source edits still requires an explicit persisted-choice/stability policy and tests.

## 4. P4-C — versioned authored Datum persistence (DIRECTION ACCEPTED IN PRINCIPLE; NO SCHEMA APPROVAL)

**Recommended structural invariant:** keep one existing `DatumId` and existing Offset behavior; introduce a future tagged `EdgePointDatumPlane`-style authored variant distinct from `OffsetDatumPlane`, with strict `MaterialEdgeReference` + one of:
- `edge_local_end`: **proven stable Edge-local terminal key** for selected source Edge;
- `edge_local_mid`: selected strict source Edge + MID role, no fabricated `FeaturePointAddress`;
- `independent_semantic_point`: stage-correct one-to-one resolved `FeaturePointAddress` used only for straight-Edge off-line plane;
and authored `reverse`, a **deterministic default-orientation policy identifier and/or expressly approved orientation reference seed** where needed for edit continuity, visibility, all without persisted OCCT topology tokens, screen-hit data, **source-point XYZ as topology identity** or last-good frame. A numerical orientation seed (if approved) is **orientation intent only**, not an independent durable source identity.

**Conceptual only — not approved JSON or public C++ schema:**

```text
Existing schema v16 datum:
  {kind: offset_plane, id, source, offset_mm, visible} // unchanged

Future separate schema revision (candidate v17, NOT accepted):
  {kind: edge_point_plane, id, source_edge,
   point_source: {kind: edge_local_end | edge_local_mid | independent_semantic_point,
                  ...typed_provenance...},
   reverse, visible} // illustrative keys, not an implementation spec
```

- **Backward reading:** new reader must still accept valid old v16 Origin/Surface/Datum Offset files and preserve their semantics and stable IDs; an old reader may explicitly reject a new schema version rather than misinterpret/ignore a new Datum.
- **Version transition:** the new native writer may upgrade only under explicit accepted container/Part schema version policy. No silent v16 writer emitting an unknown `kind`, no inventing an implicit default for a missing stable endpoint key, and no downgrade that drops authored Edge+Point meaning. Malformed/unknown variants must fail structurally with a specific diagnostic.
- **Cold reconstruction:** Save/Close/Reopen on a genuinely new Kernel instance must re-resolve strict source stage/Curve/Edge-local terminal (or independent Point), reproduce the same O/U/V/N within accepted modeling bounds, and honor `reverse`. Source loss/branch split stays authored Missing/Ambiguous/Blocked and cannot render an old good plane.
- **History and editing:** source Feature geometry edit re-evaluates the same authored meanings; delete/suppress/replace producer fails closed; later Chamfer does not redirect a reference authored at an earlier Body stage to a final-stage edge. One atomic Finish/Cancel/Edit/Undo/Redo path; preserve DatumId when repairing a broken authored Datum.
- **Dependency/Docs:** existing sketches/Datum chains must re-evaluate from current frames; no separate global dependency framework. Any future shipped change triggers Internal as-built documentation, paired PL+EN user docs and regenerated Browser under DOC-01.

**Requested P4-C decision:** accept the separation and **explicitly versioned** new Datum kind principle, leaving exact v17 fields, migration details, diagnostics and evidence contract to the next independent review. **Do not accept a new schema here by implication.**

## 5. Finite future verification gate (ALL UNRUN for the NEW product semantics)

| Witness | Future test obligation | Failure mode that must not silently pass |
| --- | --- | --- |
| F-01 straight END | Same canonical `O/U/V/N` at selected Edge's own terminal, including when another coincident Edge's Point was highlighted first; both pick orders | Native first/last orientation, global Point lock-in, provider token |
| F-02 straight MID | Exact bounded half-length 3D O + deterministic proposed signed frame **even when no intrinsic ±tangent preference exists**; preview, Reverse and cold replay/edit | Tessellated MID, faux three-Surface Point, source-independent orientation interpreted as Unsupported |
| F-03 open Arc END/MID | Both trim/sweep signs, split/refine and genuinely new provider; full circle reject | Parameter-average/tessellation, periodic seam winner |
| F-04 off-line straight Edge | Strict stable anchor O, reproducible default U/N/V for independently certified off-line Point, visible Reverse in preview; reject collinear | Native first-vertex identity, no default preview for ±normal, silent flip during edit |
| F-05 Reverse | **Generate default MID preview**, press Odwróć to flip `V,N` under R1 without repick, toggle twice, Finish, Undo/Redo/reopen; unchanged source | Blank preview because tangent is unsigned, flip N alone, choose another END, rewrite source identity |
| F-06 native files | Current v16 Offset roundtrip unchanged; new kind future-version save/reopen/unknown-kind refusal; backwards rejection; failed source persists | Silent drop/migration or false successful load |
| F-07 stale/lifecycle | Late provider result, stale stage/session, source delete/suppress/replace; no preview/Finish/last-good ghost | Earlier cached frame treated as current |
| F-08 canonical default orientation | Parallel primary U seed **deterministically selects an alternative**, exact world-axis tie, minor edits across axis/sign boundaries with persisted orientation continuity; truly degenerate geometry rejects | Random/camera seed, no MID preview for merely free ±tangent, unexpected 90° U rotation after tiny edit |
| F-09 downstream Sketch | Existing authored local 2D geometry and DatumId survive cold replay, explicit Reverse produces defined world change without unintended source retarget | Same visible plane masking changed U/V basis |

E0/FULL #2828 **199/199 is NOT** any of these newly authored Datum checks. This review file runs **zero tests**, adds **zero production API/schema**, and does not authorize editing any of the above test targets. Separate exact allowlist, test registration, Windows FOCUSED/FAST/FULL, docs and Owner practical acceptance are needed if an executable contract is later accepted and activated.

## 6. Narrow Owner review requested — no implicit merge

P4 decisions to discuss **separately from the already accepted P1/P2/P3/P5 direction**:
1. **P4-A (Owner clarification):** once **sources and exact geometry** are valid, **always offer a deterministic default orientation and preview for a free ±normal** (especially MID); user can press **Odwróć** before Finish. Use proven source sign/seed where available; otherwise propose a canonical world/reference seed with documented deterministic tie/parallel fallback and persistence continuity. **Do not reject solely for an unsigned tangent.** True source identity/geometry unavailability stays fail-closed.
2. **P4-B:** the concrete orientation convention `(O,U,V,N) -> (O,U,-V,-N)` for Reverse, plus the canonical proven terminal anchor for through-line+Point.
3. **P4-C:** new **versioned** Datum-kind approach (current Offset/v16 unchanged), without approving C++/JSON fields or a v17 bump yet.

**Owner accepted P4-A/P4-B/P4-C directions in principle on 2026-10-13, then explicitly corrected P4-A UI behavior:** when identity/geometry are certified but the orientation has two equally valid signs, **program proposes an oriented preview; user may Reverse**. This new instruction supersedes previous fail-closed-on-missing-intrinsic-orientation preference. It does **not** authorize code, an exact fallback algorithm, schema/JSON fields, UI implementation, PR merge, D2 ACTIVE activation, E0 evidence PR #348 merge, or parent UX PR #345 merge. Separately signed-off detailed UX and full cold-rebuild/Sketch continuity proofs still required.

## Documentation impact

Internal docs: not required  
User/Product docs: not required  
Reason: work-only review document for an unresolved signed frame, Reverse and persistence design decision; no production behavior, as-built contract, API, schema, UI, user-facing feature or Browser artifact has changed.
