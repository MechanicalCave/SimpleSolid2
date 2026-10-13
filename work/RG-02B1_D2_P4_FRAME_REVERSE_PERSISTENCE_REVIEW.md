# RG-02B1/D2-P4 — stable signed Datum frame, Reverse and Part persistence (OWNER REVIEW DRAFT)

**Status:** **PROPOSED FOR OWNER CONSULTATION ONLY / UNACCEPTED / UNMERGED / NOT ACTIVE / NO PRODUCT-CODE AUTHORITY.**  
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

## 2. P4-A — deterministic tangent sign and basis ownership (NOT APPROVED)

**Candidate A1 — source-proven signed orientation, recommended as the baseline constraint:** an open straight Edge uses **two stable Edge-local terminal keys** and a stage-bound source rule deciding their order. Its signed tangent goes from the *semantic* first terminal to the *semantic* second, **not** from OCCT parameter first/last. MID inherits exactly that sign. An open circular Arc uses signed trim orientation **from independently proven producer/Curve/terminal provenance**, evaluated on the actual bounded arc; negative/positive source sweep and provider reversal cannot silently change it. If these semantics cannot be established, report `UnsupportedOrientation`, not an arbitrary frame.

**Candidate A2 — source-owned frame seed:** take an independently proven right-handed canonical seed direction from the Edge-producing source Feature/Sketch/surface relation (where available), then project it into the perpendicular plane and normalize. This must be **derived from stable semantic provenance**, not the camera, OCCT UV direction, current Edge enumeration or sampled tessellation. Choose a secondary seed *only if* its ordering/provenance is also explicit and stable, with fixed accepted degeneracy thresholds and a test showing no flip across a small upstream edit. If no seed can be certified, return `UnsupportedFrameBasis`. A source-generated Fillet/Chamfer/open Arc may need a different separately proven seed; do not assume every Curve has a Sketch frame.

**Candidate A3 — fixed global-axis projected seed, considered but NOT recommended as unqualified default:** deterministic world-axis selection (e.g. least-aligned X/Y/Z) avoids OCCT orientation, but the selected axis can **jump** when a tangent changes across an axis-selection boundary, rotating the Sketch U/V axes discontinuously under minor edits. This needs explicit Owner acceptance of such behavior, a carefully fixed tie rule and regression coverage; **not an implied fallback**.

**Candidate A4 — authored orientation seed, separate possible fallback:** store a user-visible/orientable reference direction or frame-seed policy in the new Datum source as *orientation intent*, **not as source-Point identity**. Can preserve continuity but adds persistence and edit UX; must specify invalid/parallel-seed failure and migration. Do not add secretly to schema under this review-only P4 proposal.

**Requested P4-A choice:** accept A1 + A2 as proof obligations with **fail-closed** if source sign/basis is not certifiable, or separately approve an explicit A3/A4 fallback contract with its user-visible consequences. There is **no universal 3D stable frame proof from E0**.

## 3. P4-B — exact geometry and Reverse (PROPOSED CONVENTION, NOT APPROVED)

For a **strict source-Edge END or MID**:
- `O` = exact 3D evaluated terminal point / half-bounded-length midpoint at that specific source stage.
- `N` = normalized **signed** 3D analytic tangent under P4-A, not a transient native parameter derivative sign.
- `U` = normalized projection of the P4-A certified semantic seed into the plane perpendicular to N; `V := normalize(N × U)`. Require `U·N`, `V·N`, `U·V` near zero, all norms near one, `U × V` aligned with `N`, signed right-handedness, and fixed finite numeric bounds. No frame when seed parallel to N or source geometry degenerates.

For **straight Edge + independently resolved off-line Point**:
- Anchor `O` = the source Edge's **proven canonical terminal** under A1, **not** current Edge's native first vertex, profile-array ordinal or an arbitrary closest point on infinite line; if no stable terminal anchor exists, report `UnsupportedLineAnchor` until independently approved alternative.
- `U` = normalized vector from the proven first to proven second Edge-local terminal, with signed provenance. `d` = exact current independent Point – O; require sufficiently noncollinear `U × d`. Let `N := normalize(U × d)`, `V := N × U`. The plane contains the whole supporting line and the Point. Source Point authoring and stage correctness are independent of Edge-end transient hit filtering.
- For a change that keeps the geometric plane but alters anchor O or signed basis, downstream local Sketch geometry changes; therefore O/U/V/N stability must be explicitly tested, not inferred from plane coincidence.

**Proposed Reverse convention R1:** persist one authored boolean `reverse` in the new Datum variant. For an evaluated *base frame* `(O,U,V,N)`, `reverse = true` produces exactly `(O, U, −V, −N)`: equivalent to rotating the frame 180° around U. It remains orthonormal and right-handed; source Edge, terminal key/MID, independent Point, stage, O and U do **not change**. Toggle twice restores the exact original derived frame and generates ordinary Undo steps only for committed changes. No mode selection, no re-pick, no camera-dependent orientation. **Explicit Owner decision needed:** accept R1 or propose a different signed-basis transform. Do not approve "flip normal only", which would break right-handedness unless another axis changes.

**Numeric STOP:** zero/short Edge, native circle seam/closed Edge, trim ambiguity, ARC midpoint without exact 3D arc-length query, nearly collinear off-line Point, near-parallel seed, non-finite derivative and any unstable sign/provenance => typed Unsupported/Ambiguous/Blocked; retain authored source; never publish stale fallback frame. An accepted numeric tolerance is a *modeling* policy, not a Viewer pixel/snap tolerance, and cannot be invented by this design-only draft.

## 4. P4-C — versioned authored Datum persistence (DESIGN OPTIONS, NOT SCHEMA APPROVAL)

**Recommended structural invariant:** keep one existing `DatumId` and existing Offset behavior; introduce a future tagged `EdgePointDatumPlane`-style authored variant distinct from `OffsetDatumPlane`, with strict `MaterialEdgeReference` + one of:
- `edge_local_end`: **proven stable Edge-local terminal key** for selected source Edge;
- `edge_local_mid`: selected strict source Edge + MID role, no fabricated `FeaturePointAddress`;
- `independent_semantic_point`: stage-correct one-to-one resolved `FeaturePointAddress` used only for straight-Edge off-line plane;
and authored `reverse` (plus any **separately explicitly accepted** orientation seed), visibility, all without persisted OCCT tokens, screen hit data, world coordinates or last-good frame.

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
| F-02 straight MID | Exact bounded half-length 3D O + signed tangent under stable Edge-local keys; cold replay/height edit | Tessellated MID, faux three-Surface Point, ambiguous sign |
| F-03 open Arc END/MID | Both trim/sweep signs, split/refine and genuinely new provider; full circle reject | Parameter-average/tessellation, periodic seam winner |
| F-04 off-line straight Edge | Anchor O and `U` semantically fixed, N/V right-handed and stable for noncollinear source Point; reject collinear | Arbitrary first vertex/anchor or sign flip |
| F-05 Reverse | `reverse` only changes `V,N` signs under proposed R1; repeat edit/Undo/Redo/reopen; unchanged source | Flip N alone, choose another END, rewrite XYZ |
| F-06 native files | Current v16 Offset roundtrip unchanged; new kind future-version save/reopen/unknown-kind refusal; backwards rejection; failed source persists | Silent drop/migration or false successful load |
| F-07 stale/lifecycle | Late provider result, stale stage/session, source delete/suppress/replace; no preview/Finish/last-good ghost | Earlier cached frame treated as current |
| F-08 near degeneracies | Seed-tangent parallelism, near-zero length, near-collinear independent Point, tie/threshold crossings | Arbitrary world-axis fallback, Viewer tolerance used for CAD |
| F-09 downstream Sketch | Existing authored local 2D geometry and DatumId survive cold replay, explicit Reverse produces defined world change without unintended source retarget | Same visible plane masking changed U/V basis |

E0/FULL #2828 **199/199 is NOT** any of these newly authored Datum checks. This review file runs **zero tests**, adds **zero production API/schema**, and does not authorize editing any of the above test targets. Separate exact allowlist, test registration, Windows FOCUSED/FAST/FULL, docs and Owner practical acceptance are needed if an executable contract is later accepted and activated.

## 6. Narrow Owner review requested — no implicit merge

P4 decisions to discuss **separately from the already accepted P1/P2/P3/P5 direction**:
1. **P4-A:** canonical signed Edge source key and *semantic source-owned U seed*, with an explicit fail-closed outcome if no stable key/seed is proven; do not automatically use world-axis projection.
2. **P4-B:** the concrete orientation convention `(O,U,V,N) -> (O,U,-V,-N)` for Reverse, plus the canonical proven terminal anchor for through-line+Point.
3. **P4-C:** new **versioned** Datum-kind approach (current Offset/v16 unchanged), without approving C++/JSON fields or a v17 bump yet.

**Owner's acceptance of this design review in principle would not authorize code, schema, UI, PR merge, D2 ACTIVE activation, E0 evidence PR #348 merge, or parent UX PR #345 merge.** Every subsequent acceptance, exact-head merge and ACTIVE switch remains independent.

## Documentation impact

Internal docs: not required  
User/Product docs: not required  
Reason: work-only review document for an unresolved signed frame, Reverse and persistence design decision; no production behavior, as-built contract, API, schema, UI, user-facing feature or Browser artifact has changed.
