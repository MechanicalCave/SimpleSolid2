# PM-04 — Axis / Revolve

**Status:** COMPLETED — PASS; OWNER FINAL WINDOWS ACCEPTANCE 2026-10-06  
**Owner amendment:** `work/PM-04F_AXIS_DESIGNATION_UX_AMENDMENT.md` — bounded Axis authoring UX / source-uniqueness amendment, implemented and accepted  
**Decision class:** D2 production Work Contract  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Program authority:** `work/PART_MODELING_V1_ROADMAP.md` v1.25  
**Architecture authority:** Constitution + Foundation + ADR-0014 + ADR-0016 + ADR-0017  
**Entry gate:** PM-03 Datum Reference Geometry / Offset Datum Plane COMPLETED — PASS  
**Production mutation:** CLOSED — future production change requires separately accepted authority

## 1. Goal

Deliver the first production rotational solid Feature and the smallest Part-owned authored axis object required to drive it:

```text
Origin X/Y/Z Axis
        OR
Sketch Line -> authored Part Axis
        ↓
AxisReference
        +
ProfileId
        ↓
Revolve Add/Cut
OneSide/Midplane
        ↓
ordered Body stage
        ↓
semantic topology catalog
        ↓
edit/recompute or structured failure
        ↓
Undo/Redo
Save/Close/Reopen
cold rebuild
```

PM-04 closes the accepted Part-v1 O-07 axis decision and delivers a complete Revolve Add/Cut lifecycle without introducing Datum Axis, arbitrary 3D axis construction, a global dependency graph, or provider-native durable identity.

## 2. Accepted product narrowing

PM-04 introduces two kinds of Revolve axis reference only:

1. a built-in Origin axis: X, Y or Z;
2. an authored Part-owned Axis derived from one non-degenerate Sketch Line.

PM-04 deliberately does **not** introduce Datum Axis.

It also does not introduce:

- axis from two points;
- axis from two planes;
- axis from cylindrical/conical Body geometry;
- Body Edge/Curve as a direct Revolve axis;
- arbitrary world-space line input;
- multi-turn Revolve above 360 degrees;
- thin Revolve;
- unequal two-direction Revolve;
- multiple Profiles in one Revolve Feature;
- helical/screw sweep;
- multi-body;
- Projection as a prerequisite.

The package is intentionally narrow: one semantic Profile + one accepted AxisReference -> one ordered Revolve Feature.

## 3. Governing invariants

Implementation must preserve:

- Part owns authored Axis, AxisReference and Revolve semantics;
- Shared 2D continues to own authored Line/Circle/Arc geometry and existing Regular/Construction roles;
- PM-04 does **not** add a new Shared-2D Axis entity role;
- an authored Axis references a Sketch Line; it never copies the line into a second authored geometric truth;
- Origin X/Y/Z are built-in semantic axes and never receive synthetic AxisId records;
- Viewer/provider line objects, OCCT axes, B-Rep topology ordinals and tessellation identity are runtime-only;
- ordered Body Feature evaluation and exact Body-stage semantics remain those accepted by ADR-0014;
- complete Face/Edge/Vertex accounting remains mandatory after every successful Revolve stage;
- Surface/Curve/Point semantic carrier meaning remains distinct from bounded provider topology per ADR-0016;
- provider periodic seams and other representation artifacts never become durable engineering Edge identity merely because OCCT emits them;
- geometry similarity/proximity is never automatic rebinding authority;
- stale revision/session/evaluation/presentation input cannot commit;
- no stale last-good Axis/Profile/Body state may feed successful downstream modeling;
- no global dependency graph is introduced.

## 4. Scope IN — authored Axis

### 4.1 Durable Axis identity

Introduce Part-local `AxisId` with the same non-aliasing/high-water principles used by DatumId, ProfileId and FeatureId.

Required semantics:

- Create Axis allocates a fresh AxisId;
- Edit Axis preserves AxisId;
- Undo restores the original AxisId;
- abandoned identities are not silently reused after Undo branching;
- outside the Part, Axis identity is qualified by DocumentId.

An `AxisIdCursor` or equivalent Part-owned allocator is permitted.

### 4.2 Axis source

The only PM-04 authored Axis constructor is conceptually:

```text
SketchLineAxis
    id: AxisId
    source:
        sketch_id: SketchId
        entity_id: EntityId   // must identify Line
    visibility: authored bool
```

The source Line may be either:

- `Regular`; or
- `Construction`.

Creating Axis does not convert or rewrite the source Line role.

The same source Line may still participate in Profile/region semantics if it remains Regular. Construction continues to be excluded from Profile region boundaries by the already accepted Shared-2D rules.

### 4.3 Axis evaluation

A valid authored Axis resolves from the current source Sketch world frame and the current source Line endpoints.

Positive direction is deterministic:

```text
direction = world(end_point) - world(start_point)
```

normalized after the Sketch support frame is resolved.

No provider Edge orientation, camera direction, traversal order or Viewer presentation may determine Axis direction.

The evaluated Axis is an infinite semantic line represented by:

```text
origin point + unit direction
```

The finite line shown in the Viewport is derived presentation only.

### 4.4 Axis status

At minimum Axis evaluation distinguishes:

- Resolved;
- Missing — source Sketch or source Line identity no longer exists;
- Unsupported/Invalid — source identity exists but is not a usable non-degenerate Line;
- Blocked — the source Sketch has no current valid world frame because its support chain is unavailable.

No stale previous line may be used after failure.

### 4.5 Axis edit and repair

Edit Axis may re-source an existing Axis to another admissible Line while preserving AxisId.

This is the explicit repair path for a Missing/Unsupported source.

Re-source:

- preserves AxisId;
- preserves authored Axis visibility;
- updates current origin/direction from the new semantic source;
- triggers normal downstream recompute;
- is one semantic transaction / one Undo step.

### 4.6 Axis Delete semantics

Deleting an authored Axis is allowed even when Revolve Features reference it.

Delete removes the Axis object but does not rewrite or delete downstream Revolve authored intent.

A referencing Revolve retains its AxisId reference and becomes structurally unavailable with a MissingAxis/Blocked-style diagnostic until:

- Undo restores the same AxisId; or
- the Revolve is explicitly edited to another AxisReference.

A newly created Axis never steals the deleted AxisId.

### 4.7 Axis visibility and presentation

An authored Axis has independent persistent Show/Hide state.

It remains displayable at Part level after leaving Sketch Edit even when the source Sketch geometry is hidden.

Axis visibility:

- does not affect evaluation;
- is not automatically changed because a Revolve consumes the Axis;
- is persisted;
- is Undo/Redo authored presentation state.

Viewport presentation is a finite line/cue derived from the infinite resolved Axis. Display length and styling are presentation policy only.

Picking that presentation resolves to AxisId, not to the source Sketch EntityId.

During an active Revolve draft, a transient source-axis emphasis may be shown even when normal presentation is hidden; that runtime cue must not author visibility.

### 4.8 Tree and Properties

Document Tree places an authored Axis under its source Sketch, alongside Part-owned Profile children:

```text
Sketch 1
  Profile 1
  Axis 1
```

Origin X/Y/Z remain under the existing `Origin` group and are not mirrored into the Sketch subtree or Reference Geometry.

Axis Properties show at least:

- Name;
- AxisId;
- Source Sketch;
- Source Line identity;
- current status/diagnostic;
- authored Visibility;
- derived origin/direction when Resolved.

## 5. Scope IN — AxisReference

### 5.1 Reference variants

Revolve uses:

```text
AxisReference
    OriginAxis(X | Y | Z)
    OR
    AuthoredAxis(AxisId)
```

No other PM-04 variant is legal.

Origin axes are built-in semantic references. They:

- always exist as Part Origin references;
- use canonical positive X/Y/Z direction;
- keep their existing authored Origin visibility behavior;
- do not receive AxisId;
- are not copied into authored Axis records merely because a Revolve references them.

### 5.2 Stage dependency floor

An authored Axis inherits the Body-stage dependency floor of its source Sketch support.

If the source Sketch is supported on:

- Origin: no Body-stage floor;
- Body Surface: that explicit BodyStageRef;
- Datum Plane: the Datum chain's existing transitive Body-stage floor.

A Revolve consuming an authored Axis must be semantically downstream of every Body stage required by both:

- its Profile source chain; and
- its Axis source chain.

Cycle-causing or forward-stage references reject before mutation.

This remains bounded Part-local dependency validation; it is not a universal graph.

## 6. Scope IN — Revolve Feature semantics

### 6.1 Revolve authored parameters

Revolve is a normal ordered Body Feature with the existing FeatureId lifecycle.

Conceptually:

```text
RevolveFeature
    FeatureId
    ProfileId
    AxisReference
    Operation: Add | Cut
    Extent: OneSide | Midplane
    Angle
    Reverse            // meaningful only for OneSide
    Suppressed
    Name
```

The existing ordered Body model, Feature status, Suppress/Delete and reconstruction semantics remain authoritative.

### 6.2 Angle contract

New Revolve draft default:

```text
Angle = 360°
Extent = OneSide
Reverse = false
```

Accepted angle magnitude:

```text
0° < Angle <= 360°
```

Zero, negative authored magnitude, non-finite values and values above 360° are rejected.

OneSide:

- Angle is the full sweep magnitude from the Profile start position;
- positive direction follows the right-hand rule about the resolved Axis direction;
- Reverse selects the opposite rotational direction.

Midplane:

- Angle is the **total** sweep;
- the sweep is symmetric: `-Angle/2 ... +Angle/2`;
- Reverse has no meaning and is not persisted as an independent Midplane truth.

Examples:

```text
OneSide 120°  -> 0 ... +120°
OneSide 120° + Reverse -> 0 ... -120°
Midplane 120° -> -60° ... +60°
Midplane 360° -> -180° ... +180°
```

### 6.3 Profile/Axis geometric admission

Standard PM-04 Revolve requires the resolved Axis line to lie in the current Profile support plane.

A skew or merely intersecting non-coplanar axis is Unsupported for PM-04.

The Profile material region must lie entirely within one closed half-plane defined by the Axis in Profile-local 2D.

Allowed:

- Profile completely on one side of the Axis;
- Profile touching the Axis at points;
- Profile with one or more boundary segments coincident with the Axis;
- Profile holes, provided the accepted material region remains valid and the final Body result satisfies single-solid semantics.

Rejected:

- material interior on both sides of the Axis;
- self-overlap/self-intersection caused by the rotational sweep;
- zero-volume result;
- provider-only success that violates the semantic admission rules.

Coplanarity/side classification must use the existing versioned numerical policy. No Viewer/pick tolerance or new fuzzy escalation is allowed.

### 6.4 Add/Cut and one-Body rules

Revolve reuses the existing single-Body ordered Feature contract:

- the first successful solid-producing Feature in an empty Body must be Add;
- later Revolve Features may be Add or Cut;
- every successful active Body stage must contain exactly one solid;
- detached Add is rejected;
- no-effect Add/Cut is rejected;
- Cut that removes the complete Body is rejected;
- multi-solid results are rejected.

Failure preserves authored Feature intent and publishes no stale successful result as current Body truth.

### 6.5 Suppress, Delete and repair

Revolve uses existing Feature semantics:

- Suppress preserves FeatureId and parameters but removes contribution from evaluation;
- Unsuppress re-evaluates from current authored sources;
- Delete removes the Feature without deleting source Profile or Axis;
- Undo/Redo restores exact authored intent.

Structured unavailability includes at least:

- MissingProfile;
- MissingAxis;
- AxisUnavailable / source Axis Blocked;
- ProfileUnavailable;
- AxisNotInProfilePlane;
- ProfileCrossesAxis;
- stage/dependency cycle rejection;
- Kernel/Boolean failure distinct from reference failure.

Edit Revolve may explicitly change Profile or AxisReference while preserving FeatureId.

## 7. Revolve topology and semantic reference requirements

### 7.1 Complete stage accounting

Every successful Revolve Body stage must produce a complete evaluated Face/Edge/Vertex catalog under the existing PM-02/ADR-0016 rules.

No Revolve-generated provider topology may silently exist outside accounting.

### 7.2 Generated Surface semantics

Revolve-generated material surfaces must derive semantic provenance from:

- FeatureId / exact Body stage;
- the source Profile boundary use that generated the surface;
- the accepted Revolve extent/operation role.

Geometry type alone is never identity.

The catalog must classify the resulting carrier Surface type truthfully, including Plane/Cylinder/Cone/Torus/Other as applicable.

### 7.3 Partial-angle caps

For `Angle < 360°`, start/end closure geometry is material topology and receives deterministic semantic roles tied to the Feature and Profile region.

Provider traversal order must not decide start/end identity.

OneSide Reverse and Midplane orientation must retain deterministic semantic start/end meaning under the authored Revolve direction contract.

### 7.4 Full 360° periodic topology

For `Angle = 360°`:

- no artificial authored start/end cap Surface exists;
- provider periodic seam topology remains accounted;
- a periodic seam that represents parameterization closure rather than a material boundary is a representation artifact, not an ordinary engineering Edge;
- such a seam is not exposed as a durable semantic Edge solely because OCCT emits it;
- ordinary Viewer engineering-edge display/picking must not promote the seam contrary to ADR-0016/0017 representation-artifact rules.

OneSide Forward, OneSide Reverse and Midplane full-rotation variants may have identical geometric Body results while retaining their authored Feature parameters. Provider seam placement/orientation must not change durable semantic identity.

### 7.5 Axis-contact collapse

Profile boundary portions coincident with the Axis may collapse under revolution.

Any resulting provider vertices/edges must still be completely accounted and explicitly classified.

Degenerate/representation topology must not be falsely promoted to a durable engineering material boundary.

### 7.6 Boolean lineage

Revolve Add/Cut reuses the accepted semantic Boolean-lineage rules:

- inherited Body semantic carriers retain identity where lineage proves inheritance;
- new tool-generated surfaces use Revolve provenance;
- geometry equality/coplanarity alone never rebinds;
- ADR-0017 continuation/representation-partition rules remain valid when applicable;
- Cut-exposed tool surfaces may become semantic Body surfaces through explicit tool provenance.

The existing SurfaceReference/Curve/Point catalog meaning must therefore remain usable for downstream Sketch support and future PM-05 consumers.

## 8. Runtime draft / GUI / Command Line

### 8.1 Axis authoring after PM-04F Owner amendment

The separate GUI **Axis** toolbar action is removed.

Axis remains a Part-owned authored object with stable AxisId and Sketch-Line source. GUI authoring is projected through the active/selected Line Operations surface:

- `Regular | Construction` remains the Shared-2D geometry role;
- `Axis | none` is an orthogonal Part designation;
- active Line creation may enable one-shot `Axis`, producing Line + authored Axis atomically in one semantic transaction / one Undo step;
- exactly one selected existing Line may toggle Axis designation through one semantic transaction;
- one exact Sketch-Line source may receive at most one newly-created/re-sourced authored Axis;
- pre-amendment duplicate-source Axis records remain loadable and are exposed fail-closed rather than merged or silently rewritten;
- Edit/Re-source on the Axis object remains the only identity-preserving way to move an existing Axis to another Line;
- Axis Tree/Properties, Show/Hide, Delete and repair remain;
- Origin X/Y/Z remain built-in AxisReference values and are not affected by this designation UI.

The `AXIS` Command Line path remains supported and uses the same AxisDraft / semantic command path. Selection-first and command-first acquisition continue to work without a persistent GUI Axis tool button.

The exact accepted rules, compatibility policy and reset acceptance gates are normative in `work/PM-04F_AXIS_DESIGNATION_UX_AMENDMENT.md`.

### 8.2 Revolve tool

Revolve supports selection-first and command-first Profile acquisition through the existing Part interaction grammar.

Axis selection is explicit. PM-04 does not infer a default Axis from Profile orientation.

Admissible Axis targets are:

- Origin X Axis;
- Origin Y Axis;
- Origin Z Axis;
- a resolved authored Axis.

After both Profile and Axis are valid, the draft uses default 360° and produces immediate preview when the complete candidate is valid.

### 8.3 Operations panel

At minimum Revolve exposes:

- Profile;
- Axis;
- Operation = Add/Cut;
- Extent = OneSide/Midplane;
- Angle;
- Reverse for OneSide;
- Finish Revolve;
- Cancel.

Edit Revolve opens the same draft with existing authored parameters and preserves FeatureId.

### 8.4 Command Line parity

Command Line drives the same runtime draft/semantic commands as GUI.

At minimum it supports equivalent actions for:

- starting Axis / Revolve;
- source/Profile/Axis acquisition through the normal semantic selection path;
- Add / Cut;
- OneSide / Midplane;
- Reverse;
- Angle input;
- Finish;
- Cancel.

No Command Line adapter may bypass normal revision/stage/reference validation.

## 9. Viewer and preview

Revolve preview follows the accepted Extrude presentation architecture:

- accepted current Body remains in normal committed presentation;
- Add preview emphasizes only material actually added;
- Cut preview emphasizes only material actually removed;
- the complete candidate Body remains the authority for Finish validity;
- preview mesh/presentation has no durable CAD identity;
- stale preview cannot commit.

A valid Revolve preview may transiently hide the source Profile using the existing Profile preview override policy.

The selected Axis remains spatially visible/emphasized during the active Revolve draft so rotational intent is clear. That cue is runtime-only and does not change authored Axis/Origin visibility.

Finish, Cancel, invalidation and context exit clear all transient preview/emphasis state.

## 10. Persistence

PM-04 is expected to advance the Part authored schema from v11 to the next accepted version.

The new schema must persist:

- AxisId high-water state;
- authored Axis records: AxisId, name, SketchId + source Line EntityId, visibility;
- Revolve Feature records with FeatureId, ProfileId, AxisReference, Add/Cut, OneSide/Midplane, Angle, OneSide Reverse, Suppressed/name;
- existing Origin visibility unchanged.

It must **not** persist:

- resolved Axis world point/direction;
- finite Viewer Axis presentation length;
- provider/OCCT axis handles;
- Revolve tool shape/B-Rep;
- periodic seam identity;
- evaluated topology tokens;
- preview geometry;
- stale last-good Axis or Body results.

Migration from v11:

- preserves all existing Document/Body/Feature/Datum/Sketch/Profile/Entity identities;
- creates an empty authored Axis collection;
- initializes AxisId high-water deterministically;
- preserves every existing Extrude Feature unchanged;
- creates no synthetic Axis for Origin X/Y/Z;
- creates no Revolve records.

Malformed Axis/Revolve authored state fails closed.

## 11. Lifecycle and failure matrix

Automated and manual evidence must cover at least:

### Axis

- create from Regular Line;
- create from Construction Line;
- Edit Axis re-source while preserving AxisId;
- source Line move/rotate updates derived Axis;
- source Sketch support move updates derived Axis;
- source Line Delete -> Axis Missing;
- zero/invalid source -> explicit failure;
- re-source repairs Missing Axis preserving AxisId;
- Show/Hide independent from source Sketch visibility;
- Delete Axis while referenced -> Axis removed, Revolve retains MissingAxis intent;
- Undo Delete restores same AxisId and repairs consumer;
- Save/Close/Reopen;
- cold rebuild with fresh runtime/provider tokens.

### Origin Axis

- X/Y/Z selectable directly as AxisReference;
- no synthetic AxisId created;
- existing Origin Show/Hide remains presentation-only;
- hidden Origin Axis remains semantically referenceable through explicit Tree/property acquisition if product interaction permits, without visibility becoming modeling truth.

### Revolve

- Add first Body Feature;
- later Add;
- Cut;
- OneSide 30°/90°/180°/270°/360°;
- OneSide Reverse;
- Midplane 30°/90°/180°/270°/360°;
- default 360° draft;
- source Axis from Origin;
- source Axis from authored Axis;
- Profile touching Axis;
- Profile boundary segment on Axis;
- Profile away from Axis;
- Profile interior crossing Axis -> rejected;
- non-coplanar Axis -> Unsupported/rejected;
- no-effect Add/Cut;
- detached Add;
- remove-all Cut;
- multi-solid rejection;
- Edit preserves FeatureId;
- change Profile;
- change AxisReference;
- Axis source edit recomputes downstream;
- Profile source edit recomputes downstream;
- Missing Axis / Missing Profile structured failure;
- Axis/Sketch support stage cycle rejection;
- Suppress/Unsuppress;
- Delete;
- Undo/Redo;
- Save/Close/Reopen;
- cold rebuild;
- GUI/Command Line parity;
- stale draft/revision/session cannot commit.

### Topology

- complete Face/Edge/Vertex accounting for representative partial/full Add/Cut stages;
- zero false Resolved references;
- partial-angle start/end roles deterministic;
- full-360 seam classified as representation artifact where non-material;
- periodic traversal/provider token changes do not change durable semantic meaning;
- inherited/new/Cut-exposed Surface lineage remains deterministic;
- downstream planar Revolve-generated Surface may serve as ordinary semantic Surface/Sketch support where its carrier is planar;
- cold rebuild reproduces semantic catalog meaning without runtime token continuity.

## 12. Checkpoint sequence

### PM-04A — Axis semantic model + persistence

Deliver:

- AxisId/high-water;
- authored Sketch-Line Axis records;
- Axis evaluation/status;
- AxisReference Origin/Authored variants;
- next Part schema + v11 migration;
- semantic/core tests.

Gate:

- no provider identity in authored state;
- direction deterministic from authored Line endpoint order;
- migration preserves all previous identities;
- malformed Axis state fails closed.

### PM-04B — Axis lifecycle / Tree / Properties / Viewer

Deliver:

- Create/Edit/Delete/Show/Hide Axis;
- independent Part-level Axis presentation;
- Tree child under Sketch;
- Properties/status;
- source-line acquisition;
- explicit repair;
- GUI/Command Line shared draft.

Gate:

- Delete referenced Axis leaves consumer intent repairably Missing;
- re-source preserves AxisId;
- hidden/visible state never affects evaluation;
- Viewer pick maps only to AxisId.

### PM-04C — Revolve semantic/kernel operation + topology catalog

Deliver:

- neutral/provider Revolve operation;
- Add/Cut;
- OneSide/Midplane;
- 0 < Angle <= 360;
- Origin/Authored AxisReference;
- Profile/Axis coplanarity and half-plane admission;
- full Body-stage topology accounting;
- partial start/end semantics;
- 360° seam representation-artifact classification;
- Boolean lineage.

Gate:

- complete accounting;
- zero false Resolved;
- no provider seam identity as authored Edge;
- exact one-solid/no-effect rules;
- full 360 cold semantic repeatability.

### PM-04D — Revolve draft / Operations / Command Line / preview

Deliver:

- selection-first/command-first;
- default 360°;
- Add/Cut;
- OneSide/Midplane;
- Reverse for OneSide;
- dynamic delta preview;
- Edit;
- one Finish transaction;
- Cancel/no-op/stale draft zero mutation;
- GUI/Command Line parity.

Gate:

- preview and Finish validate same candidate semantics;
- source Axis remains spatially clear;
- stale draft cannot commit;
- committed Body presentation is not corrupted by preview.

### PM-04E — integrated lifecycle / repair / persistence

Deliver:

- upstream Axis/Profile/support edits;
- Missing/Blocked repair;
- Suppress/Delete;
- Undo/Redo;
- stage-cycle rejection;
- Save/Close/Reopen;
- true cold rebuild with fresh runtime/provider tokens.

Gate:

- no stale last-good modeling;
- all authored IDs stable/non-aliasing;
- broken intent remains repairable exactly as contracted;
- persistence reconstructs semantic meaning without provider continuity.

### PM-04F — documentation / Product Browser / Owner Windows acceptance

Close:

- internal as-built docs;
- PL/EN Product docs;
- Product Browser;
- final exact-head automated gate;
- Owner Windows workflow.

**PM-04F final acceptance — PASS 2026-10-06:** the amendment is implemented on exact runtime/docs candidate `8d6f082137a573aab08a4eee3b383a9923d98a49`. Windows FULL #1578 attempt 2 passed core 25/25, kernel-native 47/47 and desktop 104/104, including canonical documentation/Product Browser verification. The Owner then executed the supported Windows acceptance workflow on that exact runtime candidate and reported PASS with no errors. PM-04 product acceptance is complete; production mutation authority is closed. PM-05/PM-06 remain separately gated.

## Documentation Impact

Internal docs: required  
User/Product docs: required  
Reason: PM-04 introduces durable Axis identity/reference semantics, schema migration, a new Revolve Feature family, new failure/repair behavior, new Viewer/Tree/Properties presentation, and user-visible Add/Cut OneSide/Midplane workflows.

Before PM-04 completion documentation must describe:

- Origin axes as built-in AxisReferences;
- authored Sketch-Line Axis objects and independent visibility;
- Regular vs Construction source-line behavior;
- Axis repair/Delete semantics;
- Revolve Add/Cut, OneSide/Midplane, default 360° and Reverse;
- Profile/Axis coplanarity and crossing restrictions;
- periodic seam representation-artifact behavior at 360°;
- Save/Reopen/cold rebuild;
- explicit exclusions including Datum Axis.

Product docs must not document Datum Axis, Body-Edge Axis or other deferred constructors as implemented.

## 14. STOP conditions

STOP and return to Owner review if implementation would require:

- changing Foundation ownership;
- making Shared 2D own Part Axis or Revolve meaning;
- adding a new Shared-2D Axis entity role instead of reusing existing Line + Regular/Construction;
- adding Datum Axis;
- adding Body Edge/Curve as an AxisReference;
- provider/Viewer identity in durable Axis/Revolve state;
- geometry similarity/proximity as Axis or topology rebinding;
- persisting provider periodic seam identity as engineering Edge;
- a global dependency graph;
- multi-body;
- Projection as prerequisite;
- non-coplanar/skew-axis general sweep semantics;
- multi-turn (>360°) Revolve;
- a new numerical tolerance policy that changes accepted semantic success/failure outside the existing versioned policy;
- a dependency direction contrary to Architecture baseline.

## 15. Owner-accepted decisions

The Owner has already accepted the following PM-04 design decisions during contract preparation:

- Origin X/Y/Z are inherently semantic Part axes and are directly usable by Revolve;
- Origin axes do not receive synthetic AxisId objects;
- a user-authored Axis is a Part-owned object derived from a Sketch Line and is visible independently from its source Sketch;
- an authored Axis is a child of its source Sketch, analogous in ownership/presentation level to Profile;
- source Line may be Regular or Construction and is not converted;
- no new Sketch Axis role is introduced;
- Datum Axis is outside PM-04;
- Revolve includes Add and Cut;
- Revolve includes OneSide and Midplane in the same package;
- Midplane uses total-angle symmetric semantics analogous to Extrude;
- the entire Revolve lifecycle is delivered in PM-04, not deferred;
- default new Revolve Angle is 360°;
- Delete Axis is allowed while referenced; dependent Revolve retains authored intent and becomes MissingAxis/Blocked rather than being deleted or silently rebound.

The Owner accepted this exact Work Contract on 2026-10-06. These decisions are now normative within PM-04 and may be changed only by an explicit Owner-accepted amendment.

## 16. Activation and completion boundary

The Owner accepted this exact Work Contract on 2026-10-06. The contract becomes active only when the governance change points `work/ACTIVE.yaml` here with `status: active`, passes its exact-head repository gate and is merged.

Once that activation merge exists, implementation must follow PM-04A through PM-04F in order and remain inside this contract. Any STOP condition or scope expansion returns to Owner review.

PM-04 completion authorizes only:

- built-in Origin AxisReference;
- Part-owned Sketch-Line Axis;
- complete Revolve Add/Cut OneSide/Midplane lifecycle;
- associated semantic topology/persistence/presentation required by that workflow.

Datum Axis, Projection, Fillet/Chamfer and later packages remain separately gated.
