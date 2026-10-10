# PG-01D — Split Material Edge strict identity: Owner D2 STOP / proposal

**Status: D2-B RESEARCH AND BOUNDED DESIGN DRAFT OWNER-AUTHORIZED 2026-10-10 (`zatwierdzam - kontynuuj`). Production durable identity branch, schema/version change and shipped behavior remain STOP pending a separate explicit technical D2 decision.**  
**Date:** 2026-10-10.  
**Active authority:** `work/ACTIVE.yaml` -> accepted PG-01D Work Contract, later Owner-approved manual multi-Face amendment §0. No separate product phase activated.  
**Out of scope:** changing native Part v15, Foundation/accepted ADR, general Curve/Point semantics, provider identity, boolean healing, Profile tolerances, viewer picking policy, PG-01E, PM-06, issue #302 or `main` under this diagnostic checkpoint.

## D2-A Owner decision update (2026-10-10)

**Approved:** semantic associativity for a future strict split material
Edge branch means `BodyStageRef + FeatureCurveAddress +
AtSingleSemanticPoint{FeaturePointAddress}`, using the same certified
semantic anchors after upstream geometry edits. Geometry may deform
while strict current material Edge cardinality, exactly one certified
semantic endpoint, earlier-stage freshness and semantic source
identity remain proved. This is **not** a cross-revision numeric
OCCT Edge/Vertex lineage guarantee.

**Not approved:** production third branch or altered
`MaterialEdgeReference`, v15/v16 parser/writer mutation, schema
migration and v16 save-version policy, automatic Face expansion,
merge or Owner FINAL PASS. Full technical decision proposal:
`work/PROJECTION_01D_D2_A_V16_PERSISTENCE_MIGRATION_PROPOSAL.md`.
The accepted semantic meaning is recorded in
`work/PROJECTION_01D_ONE_POINT_CURVE_REFERENCE_D2_CONTRACT_DRAFT.md`.

## 1. Actual Owner problem versus independent evidence

Owner's private Part: two successive Add Extrudes and a later Chamfer, with two target Sketches hosted on current planar Body Surfaces. A real pointer click on a current referenceable, strictly resolved Face in Project Geometry **Face Boundary** mode at `after Feature 3` rejected the Face:

`PROJECT Face Boundary rejected Face token 5: ambiguous/missing material Edge identity; earlier staged sources retained.`

This establishes that native Face picking reached Part Face-boundary admission. It does **not** establish which precise Owner Edge failed. Never commit the Owner's private Part, screenshot, geometry, private document IDs or runtime tokens.

Independent real-OCCT synthetic continuation probe: one semantically continuous Surface is proven to have at least two separate bounded Faces after Add + Add; a third Chamfer preserves the split. A positive one-Edge control admitted both Faces (2/2) [FOCUSED #38068860645](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38068860645).

**RED diagnostic characterization** [Windows kernel #38069725884](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38069725884): 24 independently authorable upstream material Edge candidates × four bounded Chamfer distances (96 attempts); 78 committed and reevaluated valid Chamfer stages; the split Surface survived in all 78; 900 face admissions succeeded, **36 selected Faces were refused due to material Edge identity**, none due to other native Face-wire causes. All 36 refused examples were classified `TopologyAccountingClass::referenceable`, `ReferenceStatus::ambiguous`, one `curve_candidate`, **not** seam or representation partition.

**Second intentionally RED characterization** [Windows kernel #38069948103](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38069948103): all 36 synthetic rejected members came from an **existing semantic Curve family with two native Edge realizations**, and each native Edge had **two incident vertices but only one certified resolved semantic Point endpoint**. The diagnostic RED was for CTest output visibility, not an unexpected build failure. The forced failure must not remain on the branch.

Thus the independent failure is at the **strict bounded material Edge identity discriminator**: the existing `MaterialEdgeReference` only authors an individual realization in a multi-Edge Curve family when `BetweenSemanticPoints` certifies two distinct resolved semantic endpoints. One endpoint is not certified in these examples, so selecting a numeric native Edge token, Face fragment, local wire index, nearest geometry or provider ordinal would not create a legal persistent reference.

**Important inference boundary:** this is a proven mechanism of the **same rejection class**, not proof that the Owner's private Face token 5 fails for the identical endpoint reason. The new transient Workbench diagnostic now includes `curveFamilyEdges=N` and `certifiedEndpoints=X/Y` for a one-click Owner confirmation.


## 1.1 D2-B Owner-approved Point-provenance audit — completed native evidence

The Owner approved an **investigation**, explicitly prioritizing the current
`PointRelation` three-semantic-Surfaces gate and checking whether a
previously valid endpoint relation was lost through Boolean/Chamfer. This
authorization does not approve changing the gate, widening
`FeaturePointAddress`, adding a `MaterialEdgeReference` branch, or
altering v15.

**Production source audit:**

- OCCT `CurrentVertexSemanticObservation` carries native adjacent
  Surfaces, incident real material Edges and transient inherited-Vertex
  lineage (when such lineage exists).
- `pointRelationForObservation` maps current native Surfaces to distinct,
  resolved semantic Surface addresses, but currently accepts a Point
  relation **only for exactly three addresses**.
- For a Curve family with multiple material Edge realizations,
  `authorMaterialEdgeReference` currently requires two strictly
  resolved semantic Point endpoints (`BetweenSemanticPoints`).
- Accepted ADR-0016 permits semantic Vertex/Point meaning from a
  certified Curve endpoint, Curve intersection and Feature lineage;
  it does not mandate globally assigning new persistent Points to
  every B-Rep Vertex with only two adjacent Surfaces.

**Exact same OCCT invocation test:** A test-only `ISolidModelingKernel`
forwarding/recording wrapper captures the **actual** final-stage
`SolidModelingResult.current_vertex_semantics` and
`inherited_vertex_realizations` which Part consumed. Correlation to
the staged `BodyStageTopologyCatalog` is within that single evaluated
native generation, by runtime token only for diagnosis; never saved,
re-bound, used as an authored selector or matched geometrically.
Windows [diagnostic #38073995083](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38073995083)
was deliberately RED solely to preserve native CTest stdout and found,
**for all 36 individually rejected Face admissions:**

- the material Curve family had two real Edge fragments and only one
  certifiable semantic Point at each rejected Edge;
- its second native endpoint had **two distinct native Surfaces and two
  distinct resolved semantic Surfaces**, not three;
- its current Vertex was accounted as
  `semantically_unsupported`/`ReferenceStatus::unsupported`, with
  zero semantic Point candidates;
- there was **no inherited-Vertex realization lineage claim from the
  preceding Body stage to this new endpoint**. This does *not* prove
  absence of provenance through the owning Chamfer Feature/source Edge;
  it rules out a simple lost *inherited Point* in these cases.

**Narrow selector feasibility:** Same-stage pair-of-Surfaces observation
matched exactly one native Vertex for each of these 36 cases, with zero
local collisions [diagnostic #38074273908](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38074273908).
This is **only local uniqueness**, not a durable reference proof or a
reason to change the globally accepted Point rule.

A more conservative test-only candidate instead uses the **existing**
stage-scoped semantic `FeatureCurveAddress` plus exactly one already
resolved `FeaturePointAddress`, demanding exactly one current
referenceable material Edge of that Curve family incident to the
resolved Point. It selects the *same rejected source Edge* **36/36
times** and, using those two semantic addresses on a separate freshly
restored Part state evaluated by a **new OCCT provider**, remains
uniquely resolved **36/36 times**; no provider tokens or XYZ are carried
between evaluations. See intentional CTest RED-for-output
[#38074545532](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38074545532).
This is a test-only proof-of-concept selector; **no new reference is
authored, persisted or exposed in UI**. The separate native regression
removes the temporary RED and must receive a focused exact-HEAD PASS.

**Interpretation:** The first defect mechanism is a deliberately
unrepresented *new two-Surface endpoint*, not an inherited semantic
Point silently dropped from the catalog. The promising D2-B next design
candidate is `Curve + single certified semantic Point, strict unique
current Edge incident to both`, with `Missing/Ambiguous/Unsupported`
failure when that proof is absent. It avoids automatically minting
generic two-Surface Point identities. It still requires further evidence
before being made durable; the current Owner private Face need not fail
for this exact reason.

**Follow-up Owner authorization:** the Owner approved continuing toward a
narrow candidate D2 contract after the green same-stage/fresh-provider probe.
The separately authored
`work/PROJECTION_01D_ONE_POINT_CURVE_REFERENCE_D2_CONTRACT_DRAFT.md`
defines proposed cardinality and failure semantics, identity canonicalization
risk, backward-compatible schema requirements, negative/Undo/Redo/on-disk
gates, and a STOP before any production material reference or v15 mutation.
A test-only collision injection (two material Edge incidences at one certified
Point) is **not** equivalent to a natural OCCT ambiguity fixture.

**STOP until a separate accepted D2 design:** Prove a **negative**
counterexample where multiple Edge fragments of one Curve touch the
same single semantic Point (must fail closed); prove same-stage
determinism, upstream edit and removal, suppression/Undo/Redo,
stage-scoped cycles, cold native actual Save/Reopen and source
reassociation after provider renewal. Any proposed persisted
one-Point branch requires an explicitly accepted layout/schema
compatibility/migration decision. Never substitute a numeric OCCT Edge,
a vertex with just two adjacent Surfaces, wire position, nearest source,
or a geometric tolerance fallback.


## 2. What is already safe and proved

- Current runtime-only provider Face and wire observations are exact-stage-scoped; no cached/persisted Face tokens.
- A native material Edge with ambiguous / unprovable semantic identity **blocks that newly selected Face**; earlier good staged selections and Part document are preserved. It is **not** eligible for the geometric Unsupported or Degenerate partial-skip route.
- Real material Edges on different Faces are retained and deduplicated only by exact semantic `MaterialEdgeReference`, never by length, carrier, proximity or topology order.
- The diagnostic adds only transient `SelectedFaceBoundaryRejectDetail` fields. Neither authoring, `MaterialEdgeReference`, Point/Curve semantic construction, File v15 nor provider interface is changed.
- Windows desktop [FOCUSED #38070902017](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38070902017) **PASS 1/1** on `f4e9bf903ff43aeb288cccd02bd130e136a88b2e` rebuilt Part/UI and verified `pg01d.native_face_fail_closed` with the enriched family/endpoint diagnostics. This does not replace the separate kernel-only Add-continuation test or a final exact-head FULL.
- A future admission fix must show a stable cross-rebuild, cross-Undo/Redo, Save/Reopen source for *each* newly admitted bounded Edge, not merely a native query that succeeds once.

## 3. Owner D2 decision requested — choose the product boundary

**A — retain strict v15 per-Edge identity and FAIL CLOSED (no new implementation scope).** Explicitly accept this as an unsupported **source identity** case; display per-member typed diagnostics, and require manual strict Edge selection or explicit geometry authoring where possible. The Owner's PG-01D practical FAIL does not become PASS by this choice unless the Owner separately revises the acceptance criterion after seeing the limitation. This is the only option implementable now without expanding the accepted identity contract.

**B — OWNER APPROVED for a bounded identity-design investigation ONLY (2026-10-10); implementation remains STOP.** Prepare a separately accepted D2 technical contract for provider-certified, stable *bounded segment* discrimination of a multi-realization semantic Curve when one endpoint is not a current referenceable Semantic Point. First prove source-history/Face-wire/vertex lineage is one-to-one across an independent Add + Chamfer fixture, same-revision rebuild, changed upstream shape and cold native Save/Reopen. If no genuinely stable semantic identity exists, **STOP**, not an invented fallback. Any `MaterialEdgeReference` branch or Point/Curve meaning / persisted layout change requires explicit Owner D2, accepted ADR/governance compatibility and version/migration analysis before code.

**C — propose a non-associative Copy as a distinct later feature.** This would not be the current same-Part linked Project Geometry under Foundation §7.2, and therefore requires separate prior governance/product authorization; not a quiet mode or fallback inside PG-01D.

**Owner selected research B only.** The new test-only `Curve + one certified Point` feasibility probe is not acceptance of a durable branch. Alternatives A/C and a production implementation of B remain unapproved.

## 4. Required next evidence and STOP conditions

1. Verify all diagnostic fields and fail-closed tests on exact-head focused Windows, with the temporary RED removed.
2. Owner runs **one** current Draft build pick on the same private Face and supplies full `PROJECT Face Boundary rejected...` with `curveFamilyEdges` and `certifiedEndpoints`; do not upload private files or IDs to public repo.
3. If Owner's mechanism differs from synthetic two-realization/one-endpoint case, diagnose Owner evidence separately before any architecture proposal.
4. For option B only, draft a precise migration/lineage-proof contract and request Owner explicit D2 **before** any new persisted branch, Point relation, Part/Kernel public provider API or tolerance change.
5. Final PG-01D Windows FULL exact-head, Product docs/Browser and explicit Owner practical FINAL PASS/merge approval still mandatory. Keep PR #309 **Draft and unmerged**.

## Documentation Impact

Internal / Work Contract diagnostic and bounded proposal recorded. No user-facing supported projection behavior has changed. Canonical as-built Product PL/EN and generated Browser final consistency still need D4 verification; never edit generated Browser manually.
