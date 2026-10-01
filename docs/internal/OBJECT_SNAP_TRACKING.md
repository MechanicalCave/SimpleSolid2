# Object Snap / Tracking / Inference — As-Built

<!-- doc-id: internal.object-snap-tracking -->
<!-- document-kind: internal -->

<!-- section-id: internal.osnap.architecture -->
## Semantic ownership and final point resolution

R11 extends the R10 point-request path; it does not create a second input engine. Shared 2D owns provider-neutral snap geometry and semantic provenance. `PointRequest` carries one final `PointResolution`.

Priority is complete explicit numeric > numeric locks > compatible exact OSNAP > exact GuideIntersection > compatible guide/request-relative inference > Polar > raw pointer. A partial numeric lock retains snap provenance only when the locked result stays exactly at the advertised snap point.

No R11 path creates authored constraints, moves existing geometry, welds gaps or introduces a Product world-space snap tolerance.

<!-- section-id: internal.osnap.geometry -->
## Exact geometry and screen-space acquisition

Static sources are Endpoint, Midpoint, Center, Quadrant, finite discrete Intersection and intrinsic Origin. Request-relative families add Perpendicular, Tangent, Nearest and explicit Line Extension.

Geometry is exact in Sketch-local coordinates. Screen distance is acquisition/ranking only. D1 policy: **9 logical px capture**, **15 logical px release**. Stable semantic keys make ties provider-order independent. Same-point collapse is semantic/canonical, not coordinate-only.

Intersection uses a bounded local crossing query; Shared 2D computes exact finite pairwise intersections only for that nearby source set. Nearest is finite-curve projection and a hard fallback. Extension is an explicit positive ray beyond one Line endpoint. PER may combine with explicit Extension. Tangent supports from-point Circle/Arc contacts plus deferred/common-tangent branch generation.

Continuation-only families are applicable only where an operation exposes a semantic continuation source; the current grammar does not synthesize such identity from Polar direction alone.

<!-- section-id: internal.osnap.preferences -->
## Global settings and persistence

`CadInteractionSettings` is shared live by the workspace CAD input session/workbenches. Defaults: OSNAP master ON; END/MID/CEN/QUAD/INT/ORG ON; PER/TAN/NEA/EXT OFF; OTRACK OFF.

OSNAP master/modes and OTRACK master persist as application/user preferences, never as Part/Sketch state. Temporary Override, candidates, hysteresis, anchors, deferred references, guides and `PointResolution` are never serialized into the CAD document.

Operations exposes persistent modes, one-shot Temporary and Tracking. R11 assigns no F3/F11 binding; Polar/DYN retain F10/F12.

<!-- section-id: internal.osnap.overrides -->
## Temporary Override and NONE

Accepted one-shot values are `END`, `MID`, `CEN`, `QUAD`, `INT`, `PER`, `TAN`, `NEA`, `ORG`, `EXT`, `NONE`. The override restricts eligibility and fails closed. It clears on accepted point, Esc, request/tool replacement or Sketch exit without changing persistent modes.

`NONE` suppresses object-derived OSNAP, PER/TAN/EXT, OTRACK acquisition/use and object-derived guides for one point while preserving explicit numeric input, locks, generic base U/V inference, Polar and raw pointer. Existing anchors/references are dormant, not erased merely by selecting `NONE`.

<!-- section-id: internal.osnap.tracking -->
## OTRACK acquisition, guides and lifecycle

Eligible anchors are END/MID/CEN/QUAD/INT/ORG. Current D1 tuning: **400 ms dwell**, maximum **2** anchors, no FIFO eviction. Explicit un-acquire is a second deliberate dwell over the same already-acquired semantic snap after moving away.

Anchors emit exact Sketch U/V guides; Polar directions may augment them when Polar is ON. Exact GuideIntersection outranks one compatible guide projection. Acceptance, Esc, request/tool replacement and Sketch exit clear request-local anchors.

Provider-neutral presentation contains the current marker, up to two acquired markers and useful active guides. Viewer rendering is non-selectable/non-authored.

<!-- section-id: internal.osnap.integration -->
## Integration and failure boundaries

The UI controller owns logical-pixel acquisition because it has Sketch-local pointer coordinates plus viewport projection. Viewer queries only bounded nearby semantic entities; final CAD coordinates come from Shared 2D math.

OSNAP, Tracking, Polar and Dynamic Input converge on the same `PointResolution` before preview/commit. R11 fails closed on stale/non-finite sources, absent bases, invalid tangent/perpendicular construction, ambiguous overlap, lock-incompatible snaps and failed required projection.

Runtime configuration/acquisition creates no revision, dirty state or Undo step. Only the owning operation's normal accepted command mutates authored state.

<!-- section-id: internal.osnap.verification -->
## Verification boundary

Core semantic/math tests remain Qt/OCCT-free where possible. Controller tests cover screen acquisition, provenance, lock compatibility, overrides, Tracking dwell/un-acquire and guide priority. Viewer tests cover non-authored marker/guide presentation.

Final completion still requires exact-head Windows FULL, Owner manual Windows PASS, current PL/EN Product docs + Browser and work-only CLOSURE. R12+ remain inactive.
