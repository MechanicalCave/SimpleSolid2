# PG-01D — Split Material Edge strict identity: Owner D2 STOP / proposal

**Status: PROPOSED — NOT ACCEPTED. D2 DECISION REQUIRED BEFORE ANY IDENTITY EXPANSION.**  
**Date:** 2026-10-10.  
**Active authority:** `work/ACTIVE.yaml` -> accepted PG-01D Work Contract, later Owner-approved manual multi-Face amendment §0. No separate product phase activated.  
**Out of scope:** changing native Part v15, Foundation/accepted ADR, general Curve/Point semantics, provider identity, boolean healing, Profile tolerances, viewer picking policy, PG-01E, PM-06, issue #302 or `main` under this diagnostic checkpoint.

## 1. Actual Owner problem versus independent evidence

Owner's private Part: two successive Add Extrudes and a later Chamfer, with two target Sketches hosted on current planar Body Surfaces. A real pointer click on a current referenceable, strictly resolved Face in Project Geometry **Face Boundary** mode at `after Feature 3` rejected the Face:

`PROJECT Face Boundary rejected Face token 5: ambiguous/missing material Edge identity; earlier staged sources retained.`

This establishes that native Face picking reached Part Face-boundary admission. It does **not** establish which precise Owner Edge failed. Never commit the Owner's private Part, screenshot, geometry, private document IDs or runtime tokens.

Independent real-OCCT synthetic continuation probe: one semantically continuous Surface is proven to have at least two separate bounded Faces after Add + Add; a third Chamfer preserves the split. A positive one-Edge control admitted both Faces (2/2) [FOCUSED #38068860645](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38068860645).

**RED diagnostic characterization** [Windows kernel #38069725884](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38069725884): 24 independently authorable upstream material Edge candidates × four bounded Chamfer distances (96 attempts); 78 committed and reevaluated valid Chamfer stages; the split Surface survived in all 78; 900 face admissions succeeded, **36 selected Faces were refused due to material Edge identity**, none due to other native Face-wire causes. All 36 refused examples were classified `TopologyAccountingClass::referenceable`, `ReferenceStatus::ambiguous`, one `curve_candidate`, **not** seam or representation partition.

**Second intentionally RED characterization** [Windows kernel #38069948103](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38069948103): all 36 synthetic rejected members came from an **existing semantic Curve family with two native Edge realizations**, and each native Edge had **two incident vertices but only one certified resolved semantic Point endpoint**. The diagnostic RED was for CTest output visibility, not an unexpected build failure. The forced failure must not remain on the branch.

Thus the independent failure is at the **strict bounded material Edge identity discriminator**: the existing `MaterialEdgeReference` only authors an individual realization in a multi-Edge Curve family when `BetweenSemanticPoints` certifies two distinct resolved semantic endpoints. One endpoint is not certified in these examples, so selecting a numeric native Edge token, Face fragment, local wire index, nearest geometry or provider ordinal would not create a legal persistent reference.

**Important inference boundary:** this is a proven mechanism of the **same rejection class**, not proof that the Owner's private Face token 5 fails for the identical endpoint reason. The new transient Workbench diagnostic now includes `curveFamilyEdges=N` and `certifiedEndpoints=X/Y` for a one-click Owner confirmation.

## 2. What is already safe and proved

- Current runtime-only provider Face and wire observations are exact-stage-scoped; no cached/persisted Face tokens.
- A native material Edge with ambiguous / unprovable semantic identity **blocks that newly selected Face**; earlier good staged selections and Part document are preserved. It is **not** eligible for the geometric Unsupported or Degenerate partial-skip route.
- Real material Edges on different Faces are retained and deduplicated only by exact semantic `MaterialEdgeReference`, never by length, carrier, proximity or topology order.
- The diagnostic adds only transient `SelectedFaceBoundaryRejectDetail` fields. Neither authoring, `MaterialEdgeReference`, Point/Curve semantic construction, File v15 nor provider interface is changed.
- Windows desktop [FOCUSED #38070902017](https://github.com/MechanicalCave/SimpleSolid2/actions/runs/38070902017) **PASS 1/1** on `f4e9bf903ff43aeb288cccd02bd130e136a88b2e` rebuilt Part/UI and verified `pg01d.native_face_fail_closed` with the enriched family/endpoint diagnostics. This does not replace the separate kernel-only Add-continuation test or a final exact-head FULL.
- A future admission fix must show a stable cross-rebuild, cross-Undo/Redo, Save/Reopen source for *each* newly admitted bounded Edge, not merely a native query that succeeds once.

## 3. Owner D2 decision requested — choose the product boundary

**A — retain strict v15 per-Edge identity and FAIL CLOSED (no new implementation scope).** Explicitly accept this as an unsupported **source identity** case; display per-member typed diagnostics, and require manual strict Edge selection or explicit geometry authoring where possible. The Owner's PG-01D practical FAIL does not become PASS by this choice unless the Owner separately revises the acceptance criterion after seeing the limitation. This is the only option implementable now without expanding the accepted identity contract.

**B — authorize a new bounded identity design investigation, NOT immediate implementation.** Prepare a separately accepted D2 technical contract for provider-certified, stable *bounded segment* discrimination of a multi-realization semantic Curve when one endpoint is not a current referenceable Semantic Point. First prove source-history/Face-wire/vertex lineage is one-to-one across an independent Add + Chamfer fixture, same-revision rebuild, changed upstream shape and cold native Save/Reopen. If no genuinely stable semantic identity exists, **STOP**, not an invented fallback. Any `MaterialEdgeReference` branch or Point/Curve meaning / persisted layout change requires explicit Owner D2, accepted ADR/governance compatibility and version/migration analysis before code.

**C — propose a non-associative Copy as a distinct later feature.** This would not be the current same-Part linked Project Geometry under Foundation §7.2, and therefore requires separate prior governance/product authorization; not a quiet mode or fallback inside PG-01D.

**No default choice is implied by continuing the diagnostic investigation.**

## 4. Required next evidence and STOP conditions

1. Verify all diagnostic fields and fail-closed tests on exact-head focused Windows, with the temporary RED removed.
2. Owner runs **one** current Draft build pick on the same private Face and supplies full `PROJECT Face Boundary rejected...` with `curveFamilyEdges` and `certifiedEndpoints`; do not upload private files or IDs to public repo.
3. If Owner's mechanism differs from synthetic two-realization/one-endpoint case, diagnose Owner evidence separately before any architecture proposal.
4. For option B only, draft a precise migration/lineage-proof contract and request Owner explicit D2 **before** any new persisted branch, Point relation, Part/Kernel public provider API or tolerance change.
5. Final PG-01D Windows FULL exact-head, Product docs/Browser and explicit Owner practical FINAL PASS/merge approval still mandatory. Keep PR #309 **Draft and unmerged**.

## Documentation Impact

Internal / Work Contract diagnostic and bounded proposal recorded. No user-facing supported projection behavior has changed. Canonical as-built Product PL/EN and generated Browser final consistency still need D4 verification; never edit generated Browser manually.
