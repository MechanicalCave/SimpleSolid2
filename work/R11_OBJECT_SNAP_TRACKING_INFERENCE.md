# R11 — Object Snap / Object Snap Tracking / Inference

**Status:** ACCEPTED — ACTIVE  
**Proposed:** 2026-09-30  
**Proposal synchronization:** 2026-10-01 after CI-04 completion  
**Owner acceptance:** 2026-10-01  
**Decision class:** D2 runtime point-resolution / snap / tracking / inference grammar + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009, ADR-0010, ADR-0011  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.7  
**Predecessor:** R10 Precision Input / Units / Polar / Dynamic Input — completed  
**Milestone:** R11 — Object Snap / Tracking / Inference

## 1. Context

R10 completed one shared precision-input path, document-aware quantities, Polar directional assistance and Dynamic Input over the existing workspace CAD buffer and semantic Sketch requests.

R11 is the next roadmap milestone. Its purpose is to make precise placement against existing Sketch geometry deliberate and repeatable without introducing authored constraints or a second point-resolution system.

Existing foundations already relevant to R11:

- `SketchInteractionState::PointRequest` is the semantic point-input request owned by Shared 2D;
- R10 numeric locks and explicit point tokens already define higher-priority exact input;
- Polar already supplies runtime directional assistance to the same request;
- R8B already evaluates provider-neutral semantic points for Line/Circle/Arc measurement markers;
- Shared 2D curve-relation analysis already computes provider-neutral Line/Circle/Arc intersections;
- Viewer/provider hit testing is presentation/runtime infrastructure and cannot become CAD identity;
- Construction geometry is authored helper geometry and is available to selection/measurement; it must be usable as snap/reference geometry unless a later explicit mode says otherwise.

This contract must reuse those seams rather than duplicate them.

## 2. Goal

Deliver a bounded R11 interaction layer that provides:

- user-configurable persistent Object Snap modes;
- one-shot Temporary Snap Override;
- deterministic candidate resolution with no manual candidate cycling;
- Object Snap Tracking acquisition from semantic snap points;
- multiple temporary acquired tracking points;
- tracking/inference guides;
- endpoint, midpoint, center, quadrant, intersection, perpendicular, tangent, nearest and intrinsic Origin snap semantics;
- a bounded Extension mode only where its geometric meaning is explicit;
- integration with R10 numeric locks, Polar and Dynamic Input;
- runtime-only state with no authored constraints or hidden topology repair.

## 3. Non-goals

R11 does **not** authorize:

- authored Coincident/Horizontal/Vertical/Parallel/Perpendicular/Tangent constraints;
- automatic constraint creation;
- a solver;
- durable sub-element identity merely because a runtime snap target exists;
- Grid Snap or Grid authoring;
- Trim, Split or Join;
- Offset, Fillet or Chamfer;
- persistent sketch relations;
- Profile gap healing;
- world-space geometric welding tolerance;
- ordinary Select RMB context menu;
- clipboard/cross-Sketch copy;
- planar-face Sketch support or projected geometry;
- Part feature-tree or solid modeling work.

R12+ remain inactive.

## 4. Core architectural rule — one point-resolution system

R11 extends the accepted R10 semantic point-resolution path. It does not add a peer resolver.

The runtime request model is:

```text
PointRequest
  ├─ shared user OSNAP/OTRACK preferences reference
  ├─ one-shot Temporary Snap Override
  ├─ request-local captured snap state
  ├─ request-local tracking anchors
  ├─ optional deferred request-relative reference
  ├─ request-local inference guides
  └─ one final PointResolution
```

A `PointResolution` is the single semantic result consumed by preview/accept/commit. It contains the exact Sketch-local point plus runtime provenance/presentation information sufficient to explain how that point was resolved.

The accepted priority hierarchy is:

```text
complete explicit numeric point
> explicit numeric/request locks
> compatible exact Object Snap
> tracking/inference guide result
> Polar
> raw pointer
```

A higher-priority condition is never changed to satisfy a lower-priority aid.

Conceptually:

```text
raw pointer + semantic geometry + request state
        │
        ├─ static/request-relative snap candidate generation
        ├─ tracking/inference candidate generation
        ├─ Polar directional assistance
        └─ explicit numeric / request locks
                │
                ▼
        one semantic resolver
                │
                ▼
        PointResolution
                │
        normal preview / accept / commit
```

R11 must not introduce:

- a second CAD input buffer;
- a second per-tool point state machine;
- a Viewer-owned final point;
- a tool-specific snap parser;
- a separate tolerance-based geometry resolver;
- a snap path that bypasses normal preview/accept/commit semantics.

The active operation still owns whether a point is valid and what accepting it means.

## 5. Runtime candidate and result semantics

A snap target is runtime semantic provenance, not authored identity.

A runtime candidate carries conceptually:

- exact/evaluated Sketch-local `Point2`;
- snap/inference kind;
- semantic source reference(s), such as EntityId + semantic role;
- request-relative source information where PER/TAN/EXT requires it;
- request-relative validity information;
- deterministic stable ranking key independent of provider enumeration order.

The exact C++ type layout is D1, but the semantic payload is not optional.

Origin uses a stable intrinsic semantic key and must not be represented by a fake EntityId.

A final `PointResolution` may additionally carry presentation provenance such as captured snap kind, tracking/guide source and label information, but that runtime provenance is not persisted into authored CAD state.

R11 must not infer a durable shared vertex, relation or constraint from a snap. If a Line endpoint is authored at an Endpoint snap, the resulting coordinate is exact according to the accepted operation; no new Coincident relation is authored by R11.

This preserves the Package-F distinction between exact authored contact and proximity-based welding.

## 6. Candidate source domain

R11 candidates are derived from the active Sketch semantic model and intrinsic Sketch references, not from Viewer tessellation.

Eligible source geometry:

- Regular Line/Circle/Arc;
- Construction Line/Circle/Arc;
- intrinsic Sketch Origin where Origin mode is eligible.

Profile fills, Viewer edges, Navigation Cube geometry and provider-native topology are not snap sources.

Future projected/reference geometry requires a later contract.

## 7. Snap modes and request-relative semantics

The R11 user-visible mode vocabulary is:

### Static semantic point modes

- **Endpoint**
  - Line Start/End;
  - Arc Start/End.
- **Midpoint**
  - Line Midpoint;
  - Arc Midpoint along authored sweep.
- **Center**
  - Circle Center;
  - Arc Center.
- **Quadrant**
  - Circle ±U/±V quadrants;
  - Arc quadrant points only when the canonical quadrant lies on the authored sweep.
- **Intersection**
  - exact evaluated finite discrete intersections of eligible authored curves;
  - overlap/coincident curves do not invent a candidate.
- **Origin**
  - intrinsic active-Sketch Origin.

### Perpendicular

Perpendicular is one user-visible mode with internal request phases:

- **PerpendicularFromPoint**
  - requires a resolved request base;
  - Line: exact orthogonal foot is eligible only when the foot lies on the finite Line segment unless explicit Extension semantics are active;
  - Circle: candidate is the radial contact on the Circle in the base→center direction family where uniquely defined;
  - Arc: corresponding Circle contact is additionally filtered by authored Arc sweep;
  - base at Circle/Arc center is non-unique and fails closed.
- **PerpendicularContinuation**
  - applies when the current operation already establishes the source/reference direction needed for a perpendicular continuation;
  - uses the same exact finite-curve validity rules;
  - is runtime request state, not a persistent relation.

### Tangent

Tangent is one user-visible mode with internal phases:

- **TangentFromPoint**
  - from an already resolved base to Circle/Arc;
  - exact tangent points are computed from supporting-circle geometry;
  - Arc candidates are filtered by authored sweep;
  - inside-circle/no-real-tangent cases fail closed.
- **DeferredTangent**
  - when TAN is explicitly requested before the operation has a suitable base, the request may capture one semantic Circle/Arc source as a deferred runtime reference;
  - accepting that first phase does not author a constraint or hidden point.
- **CommonTangent**
  - when a deferred first source and a second Circle/Arc source are available, all exact admissible common-tangent branches are generated;
  - branch choice uses the same screen-space ranking/hysteresis rules as other runtime candidates;
  - Arc endpoints/sweeps filter branches after supporting-circle construction.
- **TangentContinuation**
  - may be used only where the current operation already provides the semantic continuation source needed to define tangent continuation;
  - it remains request-local runtime state.

No TAN phase creates a persistent Tangent constraint.

### Nearest

Nearest is a continuous finite-curve projection:

- Line: nearest point on the finite segment;
- Circle: nearest radial point when uniquely defined;
- Arc: nearest point on the finite authored sweep, including endpoint fallback where appropriate;
- degenerate/non-unique cases fail closed.

Nearest is a fallback tier. Any eligible specific snap at the same interaction wins before Nearest regardless of a small screen-distance advantage.

Nearest is OFF by default.

### Extension

Extension is a virtual inference reference, not a static `SnapKind`.

Initial R11 Extension is Line-only:

- explicit Extension acquisition identifies one authored Line endpoint/reference;
- the virtual support is a positive ray continuing beyond that endpoint away from the finite segment;
- the finite Line segment itself is not Extension;
- Extension may participate in compatible request-relative inference such as PER where the explicit Extension context makes that support legal;
- the virtual ray/guide is runtime-only and non-selectable;
- Arc/Circle extension is deferred.

Extension is OFF by default and must never become implicit world-space line extrapolation for unrelated snap modes.

## 8. Reuse of R8B semantic point infrastructure

R8B already evaluates:

- Line Start/Midpoint/End;
- Circle Center and ±U/±V Quadrants;
- Arc Center/Start/End/Midpoint.

R11 should extract/reuse the provider-neutral evaluation meaning rather than make snapping depend on Measure-tool UI state.

Requirements:

- one semantic point evaluator may serve Measure and Snap;
- Measure remains read-only and keeps its current explicit marker-acquisition semantics;
- enabling OSNAP must not alter R8B Measure Between target selection rules;
- runtime snap references remain separate from durable references.

## 9. Intersection authority

Intersection candidates must reuse Shared 2D evaluated curve relations rather than implement a separate Viewer/intersection engine.

Requirements:

- Line/Line, Line/Circle, Line/Arc, Circle/Circle, Circle/Arc and Arc/Arc follow the existing supported relation semantics;
- only finite, valid discrete intersections are candidates;
- overlap/coincident ambiguity is not turned into an arbitrary snap point;
- Construction participates equally as reference geometry;
- exact endpoint/intersection coincidence may collapse to one resolved geometric point without implying shared authored identity.

R11 does not change Package-F region-closure rules.

## 10. Screen-space acquisition, exact geometry and hysteresis

Snap eligibility is a runtime/presentation decision and uses logical screen-space distance.

The resolved point itself comes only from semantic/evaluated geometry.

Therefore:

- aperture and release thresholds are logical screen-space values;
- screen distance is measured from the pointer to the projected exact candidate;
- pixels never become Product geometry tolerance;
- zoom/DPI/camera changes may change acquisition eligibility but must not change the exact candidate point;
- two nearby but distinct semantic points remain distinct even if markers overlap;
- no world-space epsilon may merge snap candidates.

Capture uses bounded hysteresis so a currently captured valid candidate is not replaced by tiny pointer jitter. Hysteresis affects acquisition/branch retention only; it never changes exact geometry or ranking authority.

The initial numeric aperture/release values are D1 tuning parameters and must be established from Windows interaction evidence.

## 11. Deterministic candidate resolution

Candidate resolution is deterministic and has no manual candidate cycling.

The accepted pipeline is:

1. Temporary Override / master-enabled gate;
2. request/context/source compatibility;
3. exact geometric validity;
4. compatibility with explicit numeric/request locks;
5. logical screen-space aperture;
6. exact same-point collapse;
7. specific-snap tier versus Nearest fallback tier;
8. logical screen distance to the projected exact candidate;
9. deterministic exact tie key;
10. capture/branch hysteresis.

Temporary Override restricts the eligible family. It does not mean "prefer this family and then silently fall back".

Same-point collapse is allowed only when Shared 2D exact/canonical semantic evaluation establishes the same geometric contact. It must not use an epsilon, world-space snap tolerance or marker overlap. Exact Endpoint+Intersection coincidence may therefore collapse; merely nearby points may not.

Within the specific-snap tier, remaining exact ties use a fixed semantic kind order plus stable semantic source key. Provider query/enumeration order is never a tie-breaker.

Nearest is a hard lower tier and cannot outrank an eligible specific snap merely because its continuous projection is slightly closer on screen.

The exact numeric aperture/hysteresis constants remain D1 tuning; the ordering above does not.

## 12. Interaction with R10 explicit input and locks

R10 remains authoritative for explicit numeric input.

Rules:

- a complete explicit point token resolves independently of OSNAP, OTRACK, inference and Polar;
- explicit U/V/dU/dV/Distance/Angle locks are never silently changed to satisfy snap/tracking/Polar;
- Temporary Override changes only runtime snap eligibility and never outranks numeric locks;
- an OSNAP marker may be shown as captured only when the final `PointResolution` actually preserves that exact advertised snap;
- Dynamic Input remains an adapter to the same `PointRequest` and owns no snap/tracking semantics.

For an unlocked point request, a compatible exact OSNAP point supplies the semantic point.

For a partially locked request, OSNAP may resolve only remaining free parameters where the exact candidate is compatible. Otherwise that snap candidate is rejected.

After OSNAP, compatible tracking/inference may resolve a still-free point. Polar may act only after those higher-priority aids. Raw pointer is last.

Implementation must fail closed rather than display one semantic result while committing another.

## 13. Interaction with Polar and base inference

R11 and Polar are complementary but separate runtime aids.

Accepted hierarchy:

```text
explicit numeric
> numeric locks
> OSNAP
> tracking/inference
> Polar
> raw
```

Rules:

- a compatible exact OSNAP point outranks Polar attraction;
- tracking/guide inference outranks Polar when it yields an exact compatible inferred point;
- Polar remains the lower-priority directional magnet when no higher-priority exact point is active;
- explicit `@Distance<Angle` and locked Dynamic Input Angle values are never reinterpreted by R11.

Object Snap Tracking has a separate master toggle from Polar.

When OTRACK is ON and Polar is OFF, tracking remains useful using the Sketch base U/V direction family.

When Polar is ON, OTRACK may additionally use the currently configured accepted Polar direction family.

Base Sketch U/V inference needed by the current request may remain available even when OTRACK is OFF; OTRACK specifically controls acquired-anchor tracking, not all geometric point inference.

No tracking or Polar guide is authored geometry.

## 14. Persistent user OSNAP / OTRACK preferences

R11 introduces one shared live user configuration used by all open documents/Sketches:

- OSNAP master enabled/disabled state;
- enabled persistent snap-mode set;
- separate OTRACK master enabled/disabled state.

Accepted defaults:

- ON: Endpoint, Midpoint, Center, Quadrant, Intersection, Origin;
- OFF: Perpendicular, Tangent, Nearest, Extension;
- OTRACK: OFF.

The preferences are application/user preferences and persist across documents and application sessions.

They are not document, Part or Sketch authored state:

- no document override;
- no per-tool copy of the mode set;
- no revision/dirty/Undo entry from changing preferences;
- no persistence-schema change to CAD documents.

Open requests consume the shared live configuration; changing master/per-kind/OTRACK settings takes effect immediately without replacing the request.

UI must provide a clear Object Snap control/panel containing master, per-kind and separate OTRACK controls. It need not occupy permanent screen space if it remains directly discoverable/invokable.

R11 requires semantic actions for OSNAP master and OTRACK master. This contract does not freeze F3/F11 or any other concrete keyboard shortcut; final shortcut binding is a bounded UI decision consistent with existing SS2 shortcut ownership.

## 15. Temporary Snap Override

Temporary Snap Override applies to exactly the next point acquisition of the current semantic request.

Requirements:

- it does not mutate persistent user preferences;
- it restricts eligible snap/inference family rather than merely changing rank;
- it clears after one accepted point, Esc, request replacement, tool replacement or Sketch-context exit;
- invalid/unavailable requested snap family fails closed without unrelated fallback;
- it is meaningful only while a point-capable request is active;
- ordinary Select RMB context is not activated by R11.

Accepted semantic override tokens:

`END`, `MID`, `CEN`, `QUAD`, `INT`, `PER`, `TAN`, `NEA`, `ORG`, `EXT`, `NONE`.

The exact UI surface for entering/selecting those semantic actions remains D1.

`NONE` semantics were explicitly Owner-frozen on 2026-10-01 before activation.

Current proposed meaning: **No Object-derived assistance for exactly one point acquisition.**

While `NONE` is active:

- all static Object Snap candidates are ineligible, including Endpoint, Midpoint, Center, Quadrant, Intersection, Origin and Nearest;
- request-relative Perpendicular/Tangent candidates and explicit Extension are ineligible;
- OTRACK cannot acquire a new object-derived anchor for that point;
- already acquired OTRACK anchors are dormant for that point: their guides, guide intersections and projections are ineligible;
- deferred TAN/PER/EXT runtime references may remain stored request-locally but cannot influence that point;
- no suppressed object snap/tracking marker may be presented as captured/resolved.

`NONE` deliberately does **not** mean "turn off every drafting aid". The following remain eligible in the normal hierarchy:

- complete explicit numeric point input;
- explicit numeric/request locks and Direct Distance semantics;
- generic request-base Sketch U/V inference that has no object-derived provenance;
- Polar;
- raw pointer.

The rule is provenance-based: a candidate/guide that depends on an authored or intrinsic OSNAP semantic source is suppressed; a generic base-direction aid is not.

`NONE` does not clear or mutate:

- persistent OSNAP mode preferences;
- persistent OTRACK master state;
- existing request-local anchors/deferred references merely by being selected.

Those states are only dormant while `NONE` governs the current point and otherwise follow their normal lifecycle. Accepting the point clears the one-shot override and performs the already-defined request-local acceptance cleanup. Esc/request replacement/tool replacement/Sketch-context exit clear it under the normal Temporary Override lifecycle.

Only one Temporary Snap Override is active at a time. Selecting another override before point acceptance replaces `NONE` (or any previous override) rather than stacking overrides.

If a surviving non-object-derived aid happens to resolve numerically to the same coordinate as existing geometry, the result must not claim OSNAP/OTRACK provenance. There is no hidden fallback to object-derived assistance under `NONE`.

## 16. Object Snap Tracking acquisition

OTRACK is runtime-only and acquires anchors by deliberate hover/dwell over eligible semantic snap points.

Eligible anchor sources are:

- Endpoint;
- Midpoint;
- Center;
- Quadrant;
- Intersection;
- Origin.

Perpendicular, Tangent, Nearest and Extension-derived moving points are not tracking anchors.

Baseline behavior:

- maximum acquired anchors: **2**;
- acquiring a third anchor is rejected until an existing anchor is explicitly removed or the request lifecycle clears them;
- no FIFO replacement is allowed because silent anchor eviction would make guide meaning unpredictable;
- acquired anchors are visibly distinct from transient hover/captured snap;
- acquisition does not select geometry and creates no authored state;
- anchors clear on accepted point, Esc, request/tool replacement or Sketch-context exit.

Exact dwell duration and the explicit un-acquire gesture are D1 interaction tuning subject to manual verification.

## 17. Tracking guides and guide intersections

Each acquired tracking anchor may emit exact runtime guides through that anchor.

Direction family:

- Sketch U/V guides are always available for OTRACK;
- when Polar is ON, accepted Polar directions may additionally participate;
- OTRACK OFF disables acquired-anchor guides but does not disable generic base Sketch U/V inference required by the active request.

A guide:

- is provider-neutral semantic presentation derived from exact anchor + exact direction;
- is not selectable/editable/authored geometry;
- may participate in runtime inference;
- must remain visually distinguishable from Polar's current captured guide and from authored Construction geometry.

With the baseline maximum of two anchors, R11 may resolve an exact **GuideIntersection** where two active guides intersect discretely.

Guide×Guide resolution is exact semantic math. Screen-space distance only decides acquisition/ranking eligibility.

Accepting a GuideIntersection authors only the owning operation's normal coordinate result. It does not persist the anchors, guides or a constraint.

## 18. Inference scope and ranking

R11 inference is bounded to point placement assistance, not relation solving.

Authorized runtime inference includes:

- Sketch U/V base guides;
- alignment through OTRACK anchors using the accepted guide direction family;
- exact Guide×Guide intersection;
- exact projection onto a compatible active guide;
- request-relative Perpendicular guide/result;
- request-relative Tangent guide/result;
- explicit Line Extension ray;
- visual indication of the currently captured exact snap.

Within inference, the baseline ranking is:

```text
exact OSNAP
> exact GuideIntersection
> exact compatible guide projection / request-relative inference
> Polar
> raw
```

This hierarchy remains subordinate to complete explicit numeric input and numeric locks.

Not authorized:

- automatic Parallel/Coincident/Tangent/Perpendicular authored constraints;
- persistent relation glyphs after point acceptance;
- generic solver-backed inference;
- hidden movement of existing geometry;
- gap healing or tolerance welding.

## 19. Visual presentation

R11 needs a bounded provider-neutral presentation description for:

- snap marker kind and resolved point;
- current snap label/tooltip text;
- acquired tracking-point markers;
- tracking/inference guide segments/rays;
- current guide intersection marker.

Viewer implementation may choose rendering details, but semantic inputs to rendering must not contain provider-native identity.

Presentation requirements:

- marker glyphs are screen-space sized;
- the resolved geometric point is visually clear;
- acquired tracking anchors are distinguishable from transient hover markers;
- only useful active guides are shown; no full clutter fan;
- guides/markers are non-selectable and non-authored.

Exact colors, glyph shapes and pixel sizes remain D1/UI review unless explicitly promoted by Owner.

## 20. Construction semantics

Regular and Construction geometry are equally eligible as R11 snap/reference sources.

This does not change region/Profile semantics:

- Construction still cannot close or split a material region;
- snapping to Construction may author a Regular endpoint at that exact coordinate;
- no relation is authored between the new point and the Construction source;
- deleting/moving the Construction source later does not drag previously snapped geometry.

## 21. Origin semantics

Origin is an intrinsic semantic reference, not a hidden authored Point entity.

Origin snap:

- resolves to Sketch-local `(0,0)`;
- has no EntityId;
- participates only when Origin is eligible through the current persistent/temporary mode rules;
- must not create a durable Origin reference merely because a point was snapped there.

## 22. Lifecycle and history

OSNAP, Temporary Override, Tracking and inference are runtime assistance.

They create no Document mutation by themselves.

No revision/dirty/Undo entry is created by:

- toggling OSNAP;
- changing enabled modes;
- toggling Tracking;
- acquiring/removing tracking points;
- displaying a snap/guide;
- selecting a Temporary Override before the final operation accepts a point.

Only the owning CAD operation's normal accepted commit creates authored mutation/history.

## 23. Save / Close / Reopen

User OSNAP/OTRACK preferences persist as application/user preferences, not as CAD document state.

Persisted user preferences:

- OSNAP master;
- enabled persistent snap-mode set;
- OTRACK master.

Never persisted:

- Temporary Override;
- current captured candidate;
- tracking anchors;
- deferred TAN/PER/EXT request references;
- current guides;
- current hysteresis/capture state;
- current `PointResolution`.

Save/Close/Reopen of a Part/Sketch must not serialize runtime acquisition state and must not create document revision/dirty changes merely because OSNAP preferences changed.

## 24. Failure behavior

R11 fails closed when:

- candidate geometry is non-finite;
- semantic source reference is stale;
- a required request base is absent;
- tangent/perpendicular construction is invalid;
- an overlap/coincident relation does not define one discrete point;
- a partially locked R10 request cannot satisfy the advertised snap;
- Viewer projection required for acquisition ranking fails;
- context/revision changes invalidate the active interaction.

Failure must not silently fall back to an unrelated snap kind when a Temporary Override requested a specific kind.

## 25. Performance boundary

Pointer motion may query many candidate sources, so implementation must remain bounded.

Requirements:

- do not run unbounded all-pairs intersection work every paint frame;
- candidate generation should use semantic/view locality or cached runtime indexing appropriate to the current model size;
- any cache must be invalidated by authoritative Sketch revision/state change;
- cache/indexing is runtime-only and cannot become CAD truth;
- correctness is preferred over speculative optimization, but naive O(n²) pointer-frame intersection scans are not an accepted final architecture.

Exact spatial-index implementation is D1 and may be selected from evidence.

## 26. Implementation surface

Expected bounded implementation surfaces:

- `src/sketch/**` for provider-neutral snap candidate semantics/evaluation where appropriate;
- `src/application/**` for runtime snap/tracking configuration and shared CAD-input semantic routing where appropriate;
- `src/ui/**` for Part/Sketch orchestration and UI controls;
- `src/viewer/**` for provider-neutral marker/guide/query contracts if needed;
- `src/viewer_qt_occt/**` for bounded rendering and screen-space query implementation;
- `tests/**`;
- affected internal and PL/EN Product docs + generated Browser;
- `work/**`.

No persistence schema change is expected for authored CAD state.

## 27. Required automated verification

At minimum automated coverage must prove:

- Endpoint/Midpoint/Center/Quadrant/Origin exact point results;
- Arc quadrant eligibility only when the point lies on authored sweep;
- all current Line/Circle/Arc finite discrete intersection combinations;
- Construction geometry has equal snap/reference eligibility;
- overlap/coincident ambiguity does not invent a point;
- exact same-point collapse uses semantic/canonical exact contact and does not merge nearby points;
- deterministic result independent of provider/query enumeration order;
- capture hysteresis preserves the valid branch/candidate without changing geometry;
- PerpendicularFromPoint finite Line foot behavior;
- Perpendicular Circle radial contact, center non-unique failure and Arc sweep filtering;
- PerpendicularContinuation request lifecycle where applicable;
- TangentFromPoint valid/invalid/base-inside cases;
- DeferredTangent source capture is runtime-only;
- CommonTangent exact branch generation, Arc filtering, deterministic screen selection and hysteresis;
- TangentContinuation request lifecycle where applicable;
- Nearest is continuous finite-curve projection and never dominates an eligible specific snap;
- explicit Line Extension is a positive ray beyond the acquired endpoint and does not apply over the finite segment;
- compatible EXT/PER behavior uses only explicit Extension context;
- logical-screen aperture/zoom/DPI changes do not change exact resolved geometry;
- complete explicit numeric point input outranks all runtime aids;
- partial numeric locks are never violated;
- OSNAP outranks OTRACK/inference/Polar only when compatible;
- GuideIntersection outranks lower guide projection/Polar;
- OTRACK U/V operation remains valid with Polar OFF;
- Polar direction family may augment OTRACK with Polar ON;
- persistent defaults are END/MID/CEN/QUAD/INT/ORG ON and PER/TAN/NEA/EXT + OTRACK OFF;
- preference changes are shared live and create no document history/dirty state;
- Temporary Override is one-shot and persistent mode set remains unchanged;
- requested override family fails closed without unrelated fallback;
- final accepted `NONE` semantics are covered exactly, including suppression of static OSNAP, request-relative PER/TAN/EXT, OTRACK acquisition/use and object-derived guides while numeric input/locks, generic base U/V inference, Polar and raw remain eligible;
- `NONE` does not mutate persistent preferences or erase dormant request-local state merely by selection;
- a later Temporary Override replaces `NONE` rather than stacking;
- coordinate coincidence reached through a surviving non-object-derived aid does not acquire OSNAP/OTRACK provenance;
- tracking anchors are limited to 2, third acquisition does not silently evict an anchor, and lifecycle clearing is deterministic;
- guide intersections and guide projections remain runtime-only;
- PointRequest replacement/tool replacement/Esc/acceptance clear all request-local R11 state correctly;
- R8B Measure Between behavior remains unchanged;
- selection/grips/Move/Copy/Rotate/Scale/Mirror/R10 DYN/Polar regressions remain green;
- Profile closure still uses exact authored geometry with no snap tolerance leakage;
- Undo/Redo and Save/Close/Reopen never persist transient snap/acquisition state;
- user preference persistence is application/user-level only;
- R11 semantic/math tests run core-only wherever Qt/OCCT are not semantically required;
- exact-head Windows FULL passes.

## 28. Manual Windows verification

Final candidate requires Owner verification of at least:

- OSNAP master toggle and visible enabled-mode configuration;
- Endpoint, Midpoint, Center, Quadrant and Origin marker/capture behavior;
- Intersection on Line/Line, Line/Arc and Arc/Circle representative cases;
- Construction snapping;
- Perpendicular from a based Line/Move-style request;
- Tangent to Circle/Arc from a valid base;
- Nearest as fallback without stealing obvious Endpoint/Intersection snaps;
- Line Extension guide and capture;
- Temporary Override for at least Endpoint, Intersection, Perpendicular and None;
- deterministic behavior where multiple candidates are inside the aperture;
- no manual cycling requirement;
- multiple tracking-point hover/dwell acquisition;
- visible guide intersection from two acquired points;
- accepted Tracking/Polar interaction;
- DYN locks + snap compatibility;
- full explicit point input unaffected by nearby snaps;
- snap/guide state clears on point acceptance and Esc;
- no dirty/Undo step from runtime configuration/acquisition alone;
- deliberate creation of a closed Profile boundary using OSNAP;
- deliberate creation of a visible real gap with OSNAP disabled/None and confirmation that Profile does not auto-heal it;
- regression smoke Measure/Between, Rectangle, Circle/Arc, transforms, grips, Polar/DYN, Profile, Undo/Redo and Save/Reopen.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: R11 adds user-visible point-resolution modes, one-shot overrides, tracking acquisition/guides and new provider-neutral runtime semantic target/presentation boundaries.

Internal documentation must explain:

- candidate ownership/evaluation;
- screen-space acquisition versus exact semantic geometry;
- integration order with R10 numeric locks/Polar;
- temporary overrides;
- tracking acquisition lifecycle;
- inference-guide semantics;
- no authored-constraint boundary;
- performance/indexing boundary.

PL/EN Product documentation must explain:

- enabling/configuring OSNAP;
- snap-mode meanings;
- Temporary Override;
- Tracking acquisition and guides;
- keyboard shortcuts/tokens finally accepted;
- interaction with Polar and Dynamic Input;
- runtime-only/no-constraint behavior.

Product Browser must be regenerated and deterministic.

## 30. Delegated D1 tuning during implementation

All product semantics required for activation are resolved, including the Owner-frozen `NONE` behavior from Section 15.

At activation on 2026-10-01, the Owner delegated the following bounded items as D1 implementation tuning:

1. initial logical-pixel acquisition/release aperture values;
2. tracking dwell duration;
3. explicit tracking-anchor un-acquire gesture;
4. exact marker/glyph visual treatment;
5. concrete keyboard bindings for semantic OSNAP/OTRACK actions, if any.

These items are delegated as D1 tuning and may be chosen from implementation/manual evidence provided they do not change the semantic hierarchy, persistence model or authored-state boundaries in this contract.

The following are no longer open:

- persistent defaults;
- application/user preference lifetime;
- OTRACK behavior with Polar OFF/ON;
- maximum acquired anchors (2, no FIFO eviction);
- Line Extension inclusion and positive-ray semantics;
- PER/TAN/Nearest baseline semantics;
- deterministic resolver hierarchy;
- exact same-point collapse authority;
- one semantic `PointResolution` path.

## 31. Stop conditions

Stop for Owner review if implementation requires:

- persistence/schema change to authored Sketch/Part state;
- durable snap/sub-element identity;
- authored relations/constraints/solver;
- a second input or point-resolution engine;
- Product world-space snap tolerance;
- topology welding/gap repair;
- provider-native geometry identity in semantic candidates;
- generic curve types beyond current Line/Circle/Arc;
- R12 structural edits;
- Grid Snap;
- projected/reference geometry;
- ordinary Select RMB context;
- solid modeling.

## 32. Activation boundary

The Owner explicitly activated the synchronized R11 contract on 2026-10-01.

Activation authorizes only the R11 scope and boundaries recorded in this contract. Section 15 `NONE` semantics remain exactly as Owner-frozen on 2026-10-01. Section 30 items are delegated as bounded D1 tuning.

R12+, Grid Snap, authored relations/constraints/solver, projected/reference geometry, ordinary Select RMB context and solid modeling remain inactive.

CI-04 is completed on `main`; R11 implementation must consume the targeted build/test infrastructure rather than reopen CI-04 semantics.

## 33. Completion boundary

After activation, R11 completion requires:

- accepted candidate semantics and priority preserved;
- no authored-constraint or tolerance leakage;
- affected/FOCUSED evidence during implementation;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- required internal + PL/EN Product documentation/current Browser;
- work-only CLOSURE closeout.

R12+ remain inactive after R11 completion unless separately accepted.
