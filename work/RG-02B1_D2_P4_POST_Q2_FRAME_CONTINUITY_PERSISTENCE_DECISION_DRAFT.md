# RG-02B1/D2-P4 — after Q1 END / Q2 MID: signed Datum orientation continuity and versioned source (Owner D2 decision draft)

**STATUS: OWNER ACCEPTED D2-P4 TWO-STAGE ORIENTATION DIRECTION (2026-10-13) — PR #365 UNMERGED / NOT ACTIVE / NOT AN EXECUTABLE WORK CONTRACT.** Owner explicitly endorsed the described deterministic initial signed orientation, separately remembered seed/sign for continuity on source edits, and authored right-handed Reverse. Existing P4-C versioned Datum/legacy v16 compatibility direction remains accepted in principle; **no exact numerical threshold, orientation-intent field, schema version/encoding, file allowlist or product code permission was supplied**. This acceptance records the proposed design policy, not an invented implementation specification.
**Prepared:** 2026-10-13. **Live main baseline:** `00b6020e1cdc5fbf008eab444e6c311ca51bbb7d`, after Q1 production #359, Q2 production #363, and Q2 closeout #364 all verified MERGED. **Roadmap:** v1.33; **Foundation:** 1.0. `work/ACTIVE.yaml.active_work` continues to reference the **completed** Q2 strict Edge-local MID contract, NOT P4 implementation authority.
**Earlier accepted design direction:** the UX / Edge+Point and P4 design material in merged PRs #345, #349, #350. Their historical status headers predate Q1/Q2 implementation and must not override live GitHub. This new proposal settles only remaining details, not the previously accepted UX itself.

## 1. Precise goal and boundary

The existing Datum Plane tool should eventually add **one Edge + Point constructor**, keeping Offset unchanged: pick Edge and Point in either order; select **the chosen Edge's own END** at a shared corner, not another coincident Edge's Point; support Edge-local END/MID and independently certified off-line Point without separate user modes or a second overlapping-point chooser; reuse END visual marker for MID. When sources/geometry are certified, offer **immediate oriented preview**, even when tangent sign is intrinsically free, with **Odwróć / Reverse** and existing keyboard, focus, CAD Command Line, Finish/Cancel/Undo/Redo parity.

Shipped Q1 provides a selected **Edge+independently certified semantic terminal Point** and exact current unoriented END geometry; Q2 provides selected strict **Edge+MID** and exact half bounded 3D arclength observation. **Neither ships** signed persisted Datum frames, a guaranteed END for globally Ambiguous Point, off-line construction, UI marker or the new Datum kind. Do not reinterpret global Ambiguous/Unsupported Points as Resolved or silently replace a missing END source with a native Vertex.

This is **one DESIGN review**, not approval of a future code PR. Existing `OffsetDatumPlane` and native Part schema v16/modeling semantics v2 remain unchanged.

## 2. P4-A — proposed reproducible default frame AND edit continuity

For a currently certified, strict material Line or genuinely open circular Arc at an authored exact Body stage:

1. For END/MID, derive current `O` and an **unoriented analytic tangent line** from Q1/Q2. Pick default signed `N` using the sign of the largest absolute Cartesian component (exact tie priority X, then Y, then Z; signed winning component positive). Do **not** infer sign from native Edge traversal, camera direction or terminal-slot index. Free ±normal sign is **not** a failure.
2. Select default `U` by projecting an ordered fixed world-axis candidate onto `N`'s orthogonal plane; use a documented alternative candidate for exact/near parallel cases; normalize. Derive `V = N × U`. Require finite unit-length mutually orthogonal `U,V,N` and strictly right-handed `U × V = N` within **future explicitly accepted tolerances**, rather than relying only on `Frame3::valid()`.
3. **Do not recompute this stateless default after every edit:** when the user finishes creating the Datum, preserve independently versioned **orientation intent** (the chosen signed normal reference and U seed/reference-axis selection, plus `reverse`) as separate authored data, never as Edge/Point identity. On current valid source reevaluation choose N sign by continuity with that orientation reference; project the recorded U seed into the new normal plane. A small rotation across a dominant-component tie must not spontaneously flip N or jump U by 90 degrees. If the seed becomes truly unusable after a large edit, use an explicit documented fallback/diagnostic, not silent old-frame reuse.
4. A stored orientation vector is *not* a persistent XYZ source position or TopoDS/OCCT key; first certify the semantic Edge, selected END/MID and owning DocumentId/revision/Feature Stage. Deleted/split/foreign source remains Missing/Ambiguous/Blocked, never rebound from orientation hints.

**Decision-specific open items before any executable Work Contract:** representation of orientation intent (axis ID versus source-derived normalized vector), finite numerical tie/parallel tolerances, sign continuity policy for rotations at or above 90 degrees, source-side semantic signed hints if proved, fallback and Sketch UV policy after true degeneracy. The above is a recommended behavior/policy direction; accepting it does **not** secretly approve particular tolerance constants, C++ fields or numeric regression results.

## 3. P4-B — Reverse is an authored signed frame choice

For a valid END/MID source, default `Frame3 = (O,U,V,N)`. Persist `reverse=false/true` as an **orientation choice separate from the semantic source**. Toggling Reverse gives exactly `(O,U,-V,-N)`, preserving handedness, DatumId, O, U, source Edge, END/MID role and stage. Toggle twice restores the same default; no re-pick and no sign-of-normal rejection of an otherwise supported MID. Finish must atomically revalidate all current source/stage/geometry, orientation and revision; stale preview must never authorize Finish.

For a future strict straight Edge + **independent off-line semantic Point**, its through-line plane additionally needs a durable signed Edge anchor/axis from independently proven terminal provenance. Neither Q2 MID nor current OCCT first Vertex suffices as that anchor. Collinear Point/Line is genuinely blocked; an orientation sign choice is not.

## 4. P4-C — explicit new Datum kind and schema version

Recommend a future tagged `EdgePointDatumPlane` authored variant with existing stable DatumId, strict source Edge, a typed END/MID/independent-Point role, versioned orientation intent, `reverse`, and visibility. **No current v16 writer may emit an unknown Datum kind**. The eventual exact-schema contract must specify new native Part version (candidate v17, not approved here), all fields/tags, upgrade/backward-compatibility semantics, typed diagnostics for unknown/invalid kinds, and Save/Close/Reopen on a fresh kernel provider. Existing Origin/Surface/Datum Offset v16 read and evaluation must remain compatible, and old readers must reject an unknown version rather than silently erase its new authored meaning.

Derived `Frame3` is never a saved last-good fallback. Suppressed/deleted/replaced producer retains authored intent but evaluates Missing/Ambiguous/Blocked. Future implementations must preserve Edit/Undo/Redo and downstream Sketch local coordinates on cold reconstruction.

## 5. Finite future witness obligations — NOT YET TESTED

| Gate | Actual future proof required |
| --- | --- |
| P4-F01 | END of selected Edge B at 3-Edge corner, first hover Point A; reverse pick order; fail-closed when END provenance unavailable |
| P4-F02 | Straight MID: exact half 3D arclength, immediate canonical signed preview, Reverse works despite free ±normal |
| P4-F03 | Open Arc END/MID both sweep signs and parameter/slot reorders; stable current analytic tangent, no screen or OCCT traversal identity |
| P4-F04 | Off-line Point through strict Line: certified anchor, current source Point, noncollinear geometry; reject unsupported |
| P4-F05 | Reverse twice returns same O/U/V/N, unchanged source/stage/DatumId, persistence after reload and Edit |
| P4-F06 | Unit-orthogonal right-handed frame, canonical ties and deterministic parallel-seed fallback, nonfinite/degenerate reject |
| P4-F07 | Cold provider and small upstream edits across axis/sign boundaries preserve authored orientation and Sketch U/V |
| P4-F08 | Versioned new Datum format, unchanged v16 Offset roundtrip, invalid/unknown-kind rejection, stable IDs |
| P4-F09 | Preview/Finish/Cancel/Undo/Redo/Command Line/focus parity; no stale ghost preview, no extra picker |

**No P4 test PASS is implied** by Q1/Q2 FULL runs. This document modifies zero `src/**`, `tests/**`, generated Browser, CMake/CI or `work/ACTIVE.yaml`.

## 6. Hard STOP and Owner D2 question

**Owner design decision RECORDED, 2026-10-13:** responding to the explanation of the unresolved unoriented tangent sign with “świetnie, akceptuje - kontynuuj”, Owner approved the **two-stage P4-A design policy**: choose a deterministic oriented Frame3 once from exact certified END/MID geometry, then preserve an independently authored signed normal/U-seed intention so small subsequent edits do not cause unexplained ±normal or Sketch U/V basis jumps. The already discussed right-handed `Reverse: (O,U,V,N)→(O,U,-V,-N)` remains the proposed authored orientation choice, separate from END/MID identity. Future Datum-kind persistence is **explicitly versioned** with unchanged v16 Offset semantics, as already accepted in principle under P4-C. **This did not approve any unreviewed numerical tolerance, reorientation after ~90°/true seed degeneracy, exact source-derived signed hint, off-line line anchor, C++/JSON fields/schema version or future code/UI test allowlist.**

**Still required as independent decisions:** (1) exact-final-HEAD governance Windows check and **separate Owner authorization to squash-merge PR #365**; (2) review/accept the complete later executable bounded implementation Work Contract with the unresolved numeric/frame-continuity and migration policy nailed down; (3) independently approve and merge `work/ACTIVE.yaml`-only activation before source mutation. This D2 policy acceptance alone neither merges #365 nor authorizes any P4 product code. Completed Q2 remains historical active-work traceability.

## Documentation impact

Internal docs: not required for design-only draft
User/Product docs: not required for design-only draft
Reason: no shipped code/UI/schema change; a later executable Datum tool must specify internal as-built, paired PL/EN Product documentation and deterministically generated Browser under DOC-01.
