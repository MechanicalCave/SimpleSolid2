# PM-05F R2-D — Exact Local Edge-Feature Preview D2 Proposal

**Status:** OWNER APPROVED — ACTIVE R2-D, 2026-10-08
**Date:** 2026-10-08
**Authority:** PM-05F R2-D finding within `work/PM-05F_R2_OWNER_ACCEPTANCE_REMEDIATION.md`
**Decision class:** D2 — bounded Kernel and Viewer presentation contract change
**Documentation impact:** Internal docs required; Product docs PL/EN required only after accepted functional change; generated Browser from canonical Markdown.

## Confirmed interface gap

The current `DocumentSession::evaluateFilletDraft` and `evaluateChamferDraft` evaluate a complete candidate target solid and tessellate it into one `preview_mesh`. `CadWorkbench::refreshEdgeFeaturePreview` submits it as a single additive (blue) `SolidPreviewTone` scene, coloring the entire candidate Body. This violates the Owner-accepted local material-difference preview behavior.

The Kernel already exposes separate exact delta preview entry points for Extrude/Revolve, but neither an Edge-Feature delta operation nor two-color composition exists. `viewer::SolidPreviewScene` has exactly one tone and therefore cannot show removed and added material simultaneously.

## Proposed bounded D2 contract

1. Add one **provider-neutral, runtime-only** Kernel API for the exact material difference between the immediate upstream Body `B0` and the successful target candidate Body `B1`, or an equivalent strictly bounded edge-feature preview operation. Calculate removed material `B0 - B1` and added material `B1 - B0` from the exact successful B-Rep operands; no mesh subtraction, extra tolerance, fuzzy escalation or guessed tool volume. Expose two optional presentation meshes (orange removal, blue addition) with structured success/unsupported/failure status, never durable identity.
2. Add one **atomic, generation-scoped** Viewer presentation contract that can display both delta meshes independently alongside an unchanged neutral Body. A single logical preview scene has the authority for both colors; avoid separate asynchronous independently current overlay setters. Empty one-sided difference is normal; both differences may be present.
3. Compute this preview only after the complete exact target candidate has passed accepted Part/kernel evaluation. The preview is **non-authoritative** for Finish, whose semantic evaluation remains the commit gate. Failed, stale or canceled previews must clear atomically. No background result may outlive document/revision/session/draft generation authority.
4. Measure latency and memory on simple convex/concave corners and at least one complex fixture before selecting the implementation and interactive scheduling. Preserve the existing debounce/cancellation rules. If a correct exact delta exceeds the agreed interaction budget, report unavailable/degraded preview explicitly; never revert to whole-Body recoloring as if it were compliant.
5. Tests: Convex Fillet removal orange only; concave Fillet addition blue only; Chamfer cases; mixed add+remove where OCCT emits both; unchanged Body neutral; zero-effect/failure states; Edit and Cancel; repeated parameter changes; completion and cold rebuild unaffected; stale contexts fail closed. Validate exact delta coverage against BRep before/after, not against pixels alone.

## Exclusions and stop gates

No new authored Feature variants, topology identities, persistent schema, geometry-nearest rebinding, transient OCCT handles in public domain contracts, general-purpose Boolean tool in Part, or new modeling tolerance policy.

The Owner expressly approved this bounded D2 on 2026-10-08. R2-D is authorized for implementation on Draft PR #298, but remains OPEN pending code, exact-difference evidence, Windows tests and repeat Owner acceptance. No separate schema/identity/tolerance changes are authorized.
