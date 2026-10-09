# PM-05F R2-F — Strict Rejected Material Edge Pick Diagnostics, D2

**Status:** PROPOSED — OWNER APPROVAL REQUIRED, 2026-10-08
**Parent:** `work/PM-05F_R2_OWNER_ACCEPTANCE_REMEDIATION.md`
**Boundary:** D2 additive runtime Viewer/Workbench presentation query + diagnostic routing; no persistent schema, topology semantics, modeling tolerance or material Edge authoring change

## Problem established by code and native tests

During an active Fillet/Chamfer Edge tool, the Viewer emits a successful query containing only visible geometrically eligible Body Edge candidates. An empty list cannot distinguish no hit from an occluded edge or a filtered representation seam. The Part controller independently removes nonauthorable/ambiguous/unsupported semantic candidates; their rejection reason is not returned to the Workbench. A high-DPI geometry miss must not be mislabelled as a semantic failure.

Showing a guessed reason or selecting the nearest different Edge would violate fail-closed reference semantics and the Owner's selection grammar.

## Proposed strictly bounded contract

1. **Read-only query evidence.** Extend the existing transient, generation-scoped `BodyTopologyPickQueryResult` (or one equivalent optional Viewer diagnostic companion) with a typed rejection reason and at most the closest *within the current UI pick aperture* diagnostic category. Viewer evidence: `not_hit`, `occluded`, `representation_artifact`, and `stale_context` when provable. A diagnostic candidate is never inserted into the authorable `candidates` list. No global nearest geometry search.
2. **Semantic gating.** Part's existing strict authoring path is still the only authority for `Resolved`; valid geometric Edge candidates may be classified `unsupported` or `ambiguous` from the current, exact-stage semantic catalog. Prioritize stale generation/stage over all other reasons. Do not infer `occluded` unless an edge passed proximity and was demonstrably rejected by the existing front-visibility test.
3. **User feedback for deliberate click.** The active tool may deliver an immutable rejection-only notification to Workbench on click (not on unrelated hover or navigation). Show a concise actionable, nonpersistent message (`No Edge at cursor`, `Edge hidden behind Body`, `Seam is not an authorable Edge`, `Ambiguous Edge — change view / explicitly repair`, `Unsupported material Edge`, `Model changed — refresh/restart tool`). The notification must not clear current Edge selection, enable Finish, alter the draft, or mutate CAD state. No false success status.
4. **Lifecycle isolation.** Reset transient rejection evidence on successful selection, Cancel, Finish, tool deactivation and document/generation changes. Do not use a previous query's reason for a new click.
5. **Tests.** Native Windows Qt/OCCT click cases for visible normal Edge (selection + no diagnostic), empty aperture, known rear-occluded Edge, intentional periodic seam/representation partition, semantic Unsupported/Ambiguous, stale generation and repeated explicit Edit selections. Each rejected case asserts zero mutation to selected `MaterialEdgeReference` set, Feature, Undo/Redo. Controlled camera zoom/DPI regression remains independent.

## Stop rules and exclusions

No changes to core material Edge identity, schema, semantic Surface/Curve/Point roles, OCCT Kernel B-Rep algorithms, global selection tolerances, implicit tangent chains, face-driven Fillet, or generic picker migration. Avoid enumerating unsupported classes that cannot be reliably distinguished by available evidence. If generation-correct rejection cannot be proven deterministically, report a generic nonauthorable/no-hit message instead of a guessed reason.

**Decision required:** Does the Owner explicitly approve this additive, read-only Viewer diagnostic evidence and active-tool Workbench rejection-notification contract? Until then, R2-F is OPEN and no new interface is implemented.
