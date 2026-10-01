# R11 — Object Snap / Object Snap Tracking / Inference

**Status:** PROPOSED — INACTIVE  
**Proposed:** 2026-09-30  
**Owner acceptance:** pending  
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

R11 must extend the accepted R10 point-resolution pipeline.

Conceptually:

```text
raw pointer / semantic geometry
        │
        ├─ runtime snap candidate generation
        ├─ tracking / inference candidate generation
        └─ Polar directional assistance
                │
                ▼
        existing semantic PointRequest
                │
        existing numeric/request locks
                │
                ▼
        resolved semantic Sketch-local Point2
```

R11 must not introduce:

- a second CAD input buffer;
- a second per-tool point state machine;
- a Viewer-owned final point;
- a tool-specific snap parser;
- a snap path that bypasses normal preview/accept/commit semantics.

The active operation still owns whether a point is valid and what accepting it means.

## 5. Runtime snap reference versus durable identity

A snap target is runtime semantic provenance, not authored identity.

A runtime snap candidate may conceptually carry:

- exact/evaluated Sketch-local `Point2`;
- snap kind;
- semantic source reference(s), such as EntityId + semantic role;
- optional request-relative source information for perpendicular/tangent;
- deterministic display/ranking key.

The exact C++ representation is D1 and may differ.

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

## 7. Snap modes

The proposed R11 mode vocabulary is:

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
  - Arc quadrant points only when the corresponding canonical quadrant lies on the authored sweep.
- **Intersection**
  - exact evaluated discrete intersections of eligible authored curves;
  - no overlap/coincident-curve guess.
- **Origin**
  - intrinsic active-Sketch Origin.

### Request-relative modes

- **Perpendicular**
  - requires an active PointRequest with a semantic base;
  - resolves perpendicular feet from the base to eligible Line/Circle/Arc geometry where the exact finite-curve result exists.
- **Tangent**
  - requires an active PointRequest with a semantic base;
  - resolves mathematically valid tangent points from the base to eligible Circle/Arc geometry;
  - invalid/inside-circle cases fail closed.
- **Nearest**
  - closest finite-curve point to the current Sketch-local pointer candidate;
  - remains a fallback-style snap and must not mask more specific semantic snaps.

### Bounded Extension

Initial proposal: **Line Extension only**.

- extension is based on the infinite support of an authored Line;
- it is eligible only beyond a finite Line endpoint, not over the finite segment itself;
- the displayed guide must make the extension semantics explicit;
- Arc/Circle extension is deferred because its product meaning is not yet justified.

Any broader extension family requires Owner refinement before activation.

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

## 10. Screen-space acquisition versus geometric result

Snap eligibility is a runtime/presentation decision and may use logical screen-space distance.

The resolved point itself must come from semantic/evaluated geometry.

Therefore:

- pixel distance may decide whether a candidate is considered;
- pixel distance must never change stored geometry;
- no pixel value becomes a Product gap tolerance;
- zoom/DPI must not change the exact geometric location of a captured candidate;
- two distinct points must not be geometrically merged merely because their markers overlap on screen.

## 11. Deterministic candidate resolution

The roadmap requires deterministic resolution with no manual candidate cycling.

Proposed resolver rules:

1. a valid Temporary Snap Override restricts candidate kinds for the next accepted point;
2. otherwise only currently enabled persistent OSNAP modes participate;
3. candidates outside the screen-space acquisition aperture are ignored;
4. exact same-coordinate candidates may collapse to one geometric result while retaining deterministic provenance for presentation;
5. **Nearest is fallback-tier** and cannot outrank a more specific eligible semantic snap merely because the nearest-point projection is a few pixels closer;
6. within the same semantic tier, lower screen-space distance wins;
7. remaining ties use a fixed snap-kind order and stable semantic source key, never provider enumeration order;
8. no Tab/manual cycling is introduced.

The exact fixed snap-kind order and any tie-band value remain Owner decisions before activation.

## 12. Interaction with R10 exact input and locks

R10 input priority remains authoritative.

R11 must preserve these rules:

- a complete explicit point token resolves independently of OSNAP/Tracking;
- locked U/V/dU/dV/Distance/Angle values are never silently changed to satisfy a snap;
- an OSNAP marker may be shown as captured only when the final request resolution is compatible with the advertised snap result;
- Temporary Snap Override changes snap eligibility only; it does not outrank numeric locks;
- Dynamic Input remains an adapter to the same request and may display snap/inference state but owns no snap semantics.

For an unlocked point request, an accepted exact OSNAP candidate supplies the semantic point.

For partially locked requests, implementation must fail closed rather than display a snap glyph while committing a different point.

## 13. Interaction with Polar

R11 and Polar are complementary:

- exact object snaps target semantic points/relations;
- Polar constrains free direction;
- numeric locks remain higher priority than both.

Proposed behavior:

- when an exact OSNAP point candidate is captured and compatible with request locks, that point outranks Polar attraction;
- when no exact point snap is active, Polar continues to supply directional assistance exactly as in R10;
- tracking/inference guides may share the configured Polar angle family, but whether Tracking remains active when the Polar attraction toggle itself is OFF is an explicit Owner decision before activation.

R11 must not reinterpret explicit `@Distance<Angle` or locked Dynamic Input Angle values.

## 14. Persistent OSNAP mode set

R11 introduces one runtime OSNAP configuration:

- master enabled/disabled state;
- enabled persistent snap-mode set;
- separate Object Snap Tracking enabled/disabled state.

The configuration is user-controlled but not authored Part/Sketch state.

Open Owner decisions before activation:

- default enabled snap modes;
- whether mode configuration is application-session only or becomes application preference;
- master-toggle shortcut (proposed CAD convention: F3);
- tracking-toggle shortcut (proposed CAD convention: F11).

No document revision, dirty state or Undo entry may result from changing runtime snap configuration.

## 15. Temporary Snap Override

Temporary Snap Override applies only to the next point acquisition of the current semantic request.

Requirements:

- it does not mutate the persistent enabled-mode set;
- it clears after one point acceptance, Esc, request replacement or tool/context replacement;
- a **None** override may suppress persistent snaps for exactly one point;
- it is context-first and only meaningful while a point-capable request is active;
- ordinary Select RMB context is not activated by R11.

Proposed one-shot semantic tokens for review:

`END`, `MID`, `CEN`, `QUAD`, `INT`, `PER`, `TAN`, `NEA`, `ORG`, `EXT`, `NONE`.

Exact token spellings and UI affordance remain Owner decisions.

## 16. Object Snap Tracking acquisition

Tracking is runtime-only and based on deliberate hover/dwell acquisition of eligible semantic snap points.

Proposed eligible acquisition sources:

- Endpoint;
- Midpoint;
- Center;
- Quadrant;
- Intersection;
- Origin.

Request-relative Perpendicular/Tangent and continuously moving Nearest points are not proposed as acquired tracking anchors.

Requirements:

- moving near a supported semantic snap point may show its snap marker;
- dwelling on that marker may acquire it as a tracking point;
- acquired points remain visibly distinct from ordinary hover/captured snap;
- more than one point may be acquired;
- acquired points clear on accepted point, Esc, request/tool replacement or Sketch-context exit;
- acquisition does not select geometry and creates no authored state.

Exact dwell duration, maximum acquired-point count and explicit un-acquire gesture remain Owner decisions.

## 17. Tracking guides and guide intersections

Each acquired point may emit runtime guide lines through that point using an accepted tracking-direction family.

A guide:

- is provider-neutral presentation derived from semantic point + direction;
- is not selectable/editable/snappable as authored geometry;
- may participate in runtime inference to produce a candidate at the intersection of compatible guides;
- must remain visually distinguishable from Polar's current captured guide and from authored Construction geometry.

A guide intersection is a runtime inferred point only. Accepting it authors the ordinary operation's coordinate result; it does not author the guide or a constraint.

Open Owner decision:

- when Polar attraction is OFF, should tracking guides still use the configured Polar direction family, fall back to orthogonal directions, or be disabled until a direction family is explicitly available?

## 18. Inference scope

R11 inference is bounded to point placement assistance, not relation solving.

Authorized inference categories proposed here:

- alignment through acquired tracking points along the tracking direction family;
- intersection of two or more active tracking guides;
- current perpendicular guide from request base to candidate curve;
- current tangent guide from request base to candidate Circle/Arc;
- Line Extension guide;
- visual indication of the currently captured exact snap.

Not authorized:

- automatic Parallel/Coincident/Tangent/Perpendicular authored constraints;
- persistent relation glyphs after point acceptance;
- generic solver-backed inference;
- hidden movement of existing geometry.

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

R11 runtime acquisition state is never serialized:

- current captured candidate is not saved;
- Temporary Override is not saved;
- acquired tracking points are not saved;
- current guides are not saved.

The lifecycle of user-configured persistent OSNAP mode preferences remains an Owner decision before activation. It must not be stored as Part authored geometry.

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
- all current Line/Circle/Arc discrete intersection combinations;
- Construction geometry participates in snapping;
- overlap/coincident ambiguity does not invent a point;
- Perpendicular requires/uses request base and respects finite curve semantics;
- Tangent valid/invalid/base-inside cases;
- Nearest remains finite-curve and does not dominate specific snaps;
- bounded Line Extension semantics;
- deterministic ranking independent of provider/query enumeration order;
- same exact-coordinate candidate collapse without durable identity creation;
- distinct nearby points are not welded;
- acquisition aperture is screen-space only and zoom does not change resolved geometry;
- complete explicit numeric point input outranks OSNAP;
- partial numeric locks are never violated by OSNAP;
- exact OSNAP outranks Polar only when compatible with request locks;
- Temporary Override is one-shot and persistent mode set remains unchanged;
- one-shot None suppresses snaps for one point;
- multiple tracking-point acquisition/clear lifecycle;
- guide intersections produce runtime candidate only;
- Tracking/Polar interaction according to the final Owner-accepted rule;
- R8B Measure Between behavior remains unchanged;
- selection/grips/Move/Copy/Rotate/Scale/Mirror/R10 DYN/Polar regressions remain green;
- Profile closure still uses exact authored geometry with no snap tolerance leakage;
- Undo/Redo and Save/Close/Reopen do not persist transient snap/acquisition state;
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

## 29. Documentation impact

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

## 30. Owner decisions required before activation

The proposal deliberately does **not** guess these product decisions:

1. default persistent OSNAP mode set;
2. application-session versus user-preference lifecycle for mode configuration;
3. exact master/Tracking shortcuts — proposed F3/F11;
4. exact semantic tie-order and any screen-distance tie band;
5. initial logical-pixel snap aperture;
6. tracking dwell duration;
7. maximum simultaneously acquired tracking points;
8. un-acquire gesture;
9. Tracking direction family while Polar attraction is OFF;
10. final Temporary Override token spellings/UI affordance;
11. whether Line Extension belongs in initial R11 or is deferred;
12. exact marker/glyph visual treatment.

Production implementation must not begin until these decisions are resolved or explicitly delegated as D1 tuning.

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

This file is proposal-only.

Before R11 may become ACTIVE:

- Owner explicitly accepts the R11 contract and resolves Section 30 decisions;
- `work/ACTIVE.yaml` is updated in a governance-only activation commit;
- any roadmap wording required by accepted refinements is updated;
- proposal-only gate passes.

No production code is authorized by this proposal.

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
