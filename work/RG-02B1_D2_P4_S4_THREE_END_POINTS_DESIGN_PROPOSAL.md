# RG-02B1 / P4-S4 — Datum Plane: 3 Points (END-only), D2 scope and source-identity proposal

**Status: OWNER D2 DESIGN ACCEPTED (2026-10-15) — product scope, proposed durable point semantics, fail-closed native proof gate, ordered 3-Point frame and planned versioned v18 direction accepted. NOT an executable Work Contract; exact code/test allowlist, migration details, separate protected-main merge and ACTIVE activation remain gated.**
**Date:** 2026-10-15.
**Basis:** protected main 615c34e701a9736af695c2190d9b14d189fc2517 after Owner-manual-PASS RG-02B1/P4-S3A PR #386. Current work/ACTIVE.yaml still names accepted S3A, not S4. Part Modeling v1.33, Constitution/Foundation, ADR-0012/0013/0014/0016/0017, DOC-01 and the merged Q1/Q2/S1/S2/S3A evidence remain authoritative.

## 0. Owner product decision: replace the deferred off-Edge Point interpretation

The Owner explicitly decided on 2026-10-15:
- Do **not** implement the formerly planned independent off-infinite-line Point plus Edge interpretation. That future variant is withdrawn from the RG-02B1/P4 delivery target; it is not merely queued as the next required stage.
- Instead add **3 Points** as a third constructor of the **existing single Datum Plane tool**, alongside **Offset** and **Edge + Point**. NO new tool, on/off-Edge mode, UI shell, or common-hover redesign.
- **3 Points accepts only END**, never MID. Existing END hover/preselection appearance, hit behavior and editing conventions remain consistent with the rest of the application.
- The user clicks P1, P2, P3 in this order, **without a separate Edge operand** and without hidden imposed Edge-first selection. Points may be sourced from different material Edges of the same supported Body stage.

This is a deliberate **change of product scope** relative to the historical Owner D2 Edge+Point UX direction in work/RG-02B1_DATUM_EDGE_POINT_WORK_CONTRACT_DRAFT.md and the former “off-line Point pending” language in S2/S3A contracts. Those documents remain historically accurate; an accepted S4 D2 decision must update the *current* planning/roadmap and user documentation, not silently rewrite acceptance history. The existing Edge + Point END/MID constructor remains supported and unchanged; no new arbitrary XYZ Point support is implied.

## 1. Exact user interaction proposal

1. Enter the existing Datum Plane tool via Operations or the existing DATUMPLANE / DATUM PLANE command and choose **3 Points** in its constructor selector; **Offset stays default** and retains its semantics. Preserve the global focus, physical Esc/Enter, Command Line FINISH/CANCEL/REVERSE grammar and selection-first conventions.
2. Hover over a supported END: use the **existing END marker and transient hover policy**. Hover alone never stages or authors a persistent point. A click explicitly stages P1, then P2, then P3, each independently; no explicit Edge selection step. Acquired points remain identifiable in Operations; a changed point must use an explicit, predictable P1/P2/P3 slot/replacement workflow, not a secret fixed click-order trap.
3. P1 only or P1+P2: no Datum plane preview, Finish disabled, no mutation, Undo or file checkpoint. After a third **independent and uniquely certified** END with noncollinear exact current 3D locations: show immediate oriented plane preview; Finish and owner-context Enter can commit exactly one Datum transaction.
4. The ordered right-handed frame is proposed as origin P1; U = normalized(P2-P1); N = normalized((P2-P1) x (P3-P1)); V = N x U. The accepted numeric tolerances must reject coincident and nearly collinear triples at the current Part model scale, not use screen-space or raw floating-point ==. **Reverse** changes (U,V,N) to (U,-V,-N), not the picked points. Double Reverse restores exactly. The order of P1/P2/P3 determines the initial sign, as documented in Operations.
5. **Edit** replaces one or more source points and/or orientation while preserving the existing DatumId and dependent Sketch/Axis/Feature identities. Undo/Redo is atomic per completed edit. Cancel/Esc/draft hover/invalid pick produce no authored changes; old Offset and Edge+Point behavior and their existing Edit workflows stay unchanged.
6. No arbitrary Point, MID, transient Viewer token, provider ordinal, mouse XYZ, or manufactured off-Edge vertex is silently accepted. Invalid stage, ambiguous endpoint, stale source or collinearity produces a typed diagnostic and keeps Finish disabled. No last-good plane as a fake Resolved result.

## 2. Durable point semantics — Owner-accepted direction; native proof required before runtime code

**Owner-approved identity direction subject to positive native proof:** the persistent 3-Point source is an ordered triple of Part-owned **source-stage semantic END point references**, not 3 raw XYZ values. A candidate consists of the declared BodyStageRef and a **uniquely certified FeaturePointAddress**, derived from a genuine clickable END of a current material Edge. Existing Q1 SelectedMaterialEdgeEnd / StableEdgeLocalTerminalKey provide the provenance bridge from a clicked Edge terminal to a semantic point; the chosen durable source must describe a **point**, without making a coincident incident Edge an accidental hidden fourth operand.

**Critical native proof first:** on a real OCCT box and later edited Body, inspect whether each visible clickable END, including the shared 3-Edge corner, maps to one uniquely Resolved durable FeaturePointAddress at the declared stage, and whether that point survives independent-provider cold re-evaluation. Three incident material Edge terminals representing the **same certified point** should count as one P, not three points. If current Part Point status is ambiguous or unsupported even though an *edge-local END* is certified, do **not** promote it to Resolved or choose an incident Edge by provider/token order. Capture the precise unsupported geometry family and request a narrower Owner D2 source-identity amendment before implementing a fallback or excluding a commonly expected user case without notice.

A distinct dedicated ThreePointDatumPlane value (shared DatumId allocation/visibility/dependency semantics), a deterministic signed frame/orientation value, type-tagged evaluation and atomic Create/Edit/Replace commands are proposed. Do not squeeze this into the existing EdgePointDatumPlane or reinterpret its END/MID source variant. All three points belong to one Part Document and one explicitly compatible Body stage. No cross-document, cross-stage or runtime-only authority.

**Persistence:** native Part schema is currently v17 and contains separate Offset and EdgePoint Datum collections. A genuine new durable triple source requires an explicitly versioned additive format: **Owner accepts the additive v18 direction at D2 design level; the exact native grammar, compatibility matrix and schema migration implementation are NOT yet authorized without an executable Work Contract and native source-identity proof**. Provide validated backward reading of older native versions (including v17) and round-trip writing of the new explicit 3-Point kind without changing legacy meaning or losing unknown fields silently. Maintain one unique DatumId namespace, reject duplicates, stale cursor, malformed point provenance and invalid orientation atomically, and re-evaluate with an independent provider on cold reopen. No schema migration via a magic numeric cast, cached Frame3 or viewer pixel.

**Typed result:** Resolved only with 3 fresh strict source points + nondegenerate plane + validated signed orientation. Missing/Ambiguous/Unsupported/Blocked must retain the intended authored source and suppress false dependent geometry. Revalidation occurs at Command execution and following structural edits, not only when Finish enables.

## 3. Bounded implementation strategy — proposals, not new source permissions

**S4A — headless proof, semantic 3-Point Datum and versioned persistence** (only after explicit D2 contract and separate ACTIVE activation):
- P0 evidence: genuine native current-stage END → semantic point mapping, shared-corner deduplication, failure families and source identity after feature changes, independent-provider replay.
- Part-owned source + Frame3 evaluator, typed errors, commands, stable DatumId, Sketch/Axis dependency, Undo/Redo, v17→v18 compatibility and native Save/Close/Reopen.
- Expected module candidates, requiring an exact executable allowlist in the follow-up accepted Work Contract: src/part/include/simplesolid2/part/{datum.hpp,datum_evaluation.hpp,part_document.hpp}; src/part/{datum.cpp,datum_evaluation.cpp,part_document.cpp,part_document_store.cpp}; src/application/include/simplesolid2/application/document_session.hpp; src/application/document_session.cpp; already registered Part/schema/native tests. Do not change files merely because they are named here.

**S4B — GUI constructor in the existing Datum Plane tool** (separate acceptance, or one explicitly Owner-approved bounded vertical Work Contract after S4A source proof):
- Reuse native Viewer END hover/selection and the existing CadWorkbench draft/Operations, Tree and Command Line; no new picker framework or global hover change.
- Exact native Qt/OCCT three-click positive/negative tests, shared-corner/slot Edit, Reverse, single Undo, dependent Sketch, real cold native reopen.
- Candidate existing UI/test paths: src/ui/cad_workbench.cpp/.hpp, part_viewport_controller.cpp/.hpp, part_document_tree_controller.cpp; current registered tests/pm03c2_cad_workbench_datum_plane_test.cpp, tests/pm05f_r2_native_workbench_edit_test.cpp, viewer selection tests. Keep Viewer renderer files unchanged unless a focused proof requires a separate bounded Owner amendment.

**D2 STOP conditions:** any need for new global Vertex/Point framework, guessed screen-tolerance authoring, cross-document support, unrelated Axis/PM-06 work, native geometry kernel/provider API change, architectural ownership change, or a schema/version different from the approved detailed contract. No broad implementation or migration is authorized by this proposal.

## 4. Finite acceptance matrix to carry into the executable contract

- **S4-01:** Real material Body on Windows: three independent visible END from different Edges form a noncollinear plane with the exact documented right-handed frame. No third source => no plane or Finish.
- **S4-02:** Shared trihedral corner END hover yields one point identity independently of incident Edge, no duplicate P from the same corner. Ambiguous/unsupported semantic Point fails closed without order-dependent guessing.
- **S4-03:** Collinear, nearly collinear, coincident, foreign Document, mixed stage, stale Body or unresolved Vertex source produces typed rejection, zero mutation, no false preview/Finish.
- **S4-04:** Reverse once flips V/N, twice returns to the original frame; point ordering remains durable and exact signed orientation survives Edit.
- **S4-05:** One Create/Edit transaction and one Undo, Undo/Redo, Cancel/Esc/focus/Command Line parity; changing P1/P2/P3 retains DatumId and stable dependent Sketch/Axis/Feature identifiers.
- **S4-06:** Independent cold kernel/OCCT Save/Close/Reopen with actual 3-Point Datum, dependent Sketch, and changed upstream Body; replay is source-based rather than cached geometry.
- **S4-07:** Native schema version guard, backwards v17 and earlier fixtures, malformed/duplicate/unknown-kind v18 negatives; legacy Offset/Edge+Point unchanged.
- **S4-08:** END hover only (no MID in 3 Points), marker disappears on pointer leave; selection and preview remain independent; no permanent point overlays introduced by the new constructor.
- **S4-09:** Windows registered native FOCUSED, FAST, Ready FULL; current-state internal and PL/EN product docs, generated Browser parity, exact-head evidence; actual Owner Windows manual GUI PASS. No mock-only or code-reading substitute.

## 5. Governance and next acceptance gates

This document records **accepted D2 design direction, NOT an activated executable Work Contract**. Owner first confirmed the UX/product choice, then explicitly replied **"akceptyje - kontynuuj"** to the request to accept the detailed D2 design in PR #389 (2026-10-15). This covers proposed semantic point identity and unique native certification as a mandatory *proof obligation*, fail-closed boundary and additive v18 direction, but **does not claim a successful native P0 proof, finalized v18 grammar, executable file allowlist, source implementation authority, PR merge authority or ACTIVE activation**. Sequence:
1. Lock the Owner-accepted D2 direction. Before a persistent triple-point implementation, obtain genuine native proof that a clickable END can be persisted as a current-stage uniquely certified point including shared-corner and cold-replay scenarios. If P0 disproves this source representation, return with an explicit narrower D2 amendment; do not silently make END edge-relative or persist screen XYZ.
2. Complete PR #389 work-only governance CI and request **separate exact-HEAD Owner squash-merge authority** before altering protected main. Update the current accepted roadmap/UX direction through a scoped Owner-authorized governance change; retain historical S3A contracts as evidence.
3. Draft and accept a narrow executable S4A P0/native proof Work Contract with precise file/test allowlist, then separately merge its governance and activate it via an approved ACTIVE.yaml-only transition. Only after that may native proof/test source change. A distinct S4A durable-source/schema contract remains gated on the results of P0.
4. Implement approved S4A then S4B (or an expressly accepted narrow vertical slice) under their respective activated contracts, with independent technical and Owner manual GUI PASS, exact-head Windows evidence and separately authorized protected-main squash. The earlier S3A active pointer remains unchanged until an actual approved activation.

The already merged S3A is **COMPLETE** in its accepted Edge-local END/MID scope. The formerly deferred off-Edge Point is deliberately **not part of S4**. No claim of implementing a full arbitrary Point/Line constructor is made.

## Owner D2 design acceptance record

- **Accepted:** one existing Datum Plane tool with third constructor 3 Points, three END picks only, hover parity, no independent off-infinite-line Point + Edge interpretation, strict semantic point ownership and proof-first design, typed failures, ordered right-handed frame/Reverse, versioned additive v18 direction, shared DatumId and source-based replay.
- **Not yet granted:** claim that global FeaturePointAddress covers all clickable Q1 END; implementation of v18 before real native proof and executable schema details; exact writable Part/UI/kernel/test paths; scope/roadmap/ACTIVE mutation; PR #389 merge authorization; full production completion. These require their own gates, not implicit permission from "kontynuuj".

## Documentation impact

Internal docs: required
User/Product docs: required
Reason: a new durable Part Datum source and versioned persistence change current Part ownership/lifecycle; a new visible constructor and changed previously planned off-Edge support must be documented in both Polish and English. Regenerate docs/browser/index.html only through the official generator; never edit generated HTML by hand.
