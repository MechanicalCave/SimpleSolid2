# R12 — Trim / Extend / Extend Both to Virtual Intersection

**Status:** COMPLETED  
**Proposed:** 2026-10-01  
**Owner acceptance:** 2026-10-01  
**Decision class:** D2 structural-edit identity/reference semantics + bounded D1 implementation  
**Foundation:** 1.0 (`foundation-v1.0`)  
**Architecture:** ADR-0008, ADR-0009, ADR-0011, ADR-0012  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.8  
**Predecessor:** R11 Object Snap / Tracking / Inference — completed  
**Milestone:** R12 — Structural Editing

## 1. Goal

R12 provides only the structural editing operations currently required for practical engineering profile authoring:

1. **Trim**;
2. **Extend**;
3. **Extend Both to Virtual Intersection** for two Lines.

The Owner deliberately removed Split and Join from the current milestone. Future need may reopen them through a separate accepted contract; R12 must not implement them implicitly.

R12 remains semantic Shared-2D editing integrated through the existing application Command/Part transaction path. Viewer/Qt/OCCT may present and pick, but do not own edit meaning or CAD identity.

## 2. Accepted design principles

R12 freezes these D2 rules:

- one accepted edit uses semantic `EntityId`, never Viewer/provider identity;
- ordinary 1→1 geometry edits preserving primitive kind preserve `EntityId`;
- `Line -> Line` Trim/Extend preserves `EntityId`;
- `Arc -> Arc` Trim/Extend preserves `EntityId`;
- `Circle -> Arc` Trim changes primitive kind and therefore retires the Circle `EntityId` and allocates one fresh non-reused Arc `EntityId`;
- no R12 operation may leave two surviving authored target entities;
- standard Extend intersects the target continuation with **finite authored boundary geometry only**;
- virtual/infinite supporting geometry is authorized only by the separate Line+Line Extend Both operation;
- exact geometric results do not create constraints, relations or durable snap provenance;
- no world/screen tolerance may weld, heal or invent topology;
- ambiguity fails closed.

## 3. Scope IN

- Trim target: Line, Arc, Circle;
- Trim boundary source: finite Line, Arc, Circle;
- one or more explicit Trim boundary entities;
- Regular and Construction entities as Trim targets/boundaries;
- Trim hover/preview of the exact connected fragment proposed for removal;
- Extend target: Line, Arc;
- Extend boundary source: finite Line, Arc, Circle;
- Regular and Construction entities as Extend targets/boundaries;
- selected target end/side semantics for Extend;
- Extend Both to Virtual Intersection for exactly two Lines;
- exact provider-neutral intersection/continuation math for current Line/Circle/Arc;
- identity/reference outcomes frozen by this contract;
- atomic command/transaction integration;
- Undo/Redo;
- Profile reevaluation after accepted geometry change;
- Save/Close/Reopen of resulting authored geometry and identity high-water;
- runtime selection reconciliation and non-authored preview;
- internal documentation;
- PL/EN Product documentation + generated Product Browser;
- affected/FOCUSED evidence, final exact-head Windows FULL, Owner manual Windows verification and work-only CLOSURE.

## 4. Scope OUT

R12 does not authorize:

- Split;
- Join;
- removal of a middle Line/Arc fragment when two surviving authored target pieces would remain;
- Circle Split;
- multi-output structural edits;
- Offset;
- Fillet;
- Chamfer;
- Break/Break-at-two-points;
- Polyline/group identity;
- Extend of a Circle target;
- standard Extend against an infinite/virtual extension of its boundary;
- Line/Arc/Circle automatic gap healing;
- endpoint welding;
- product world-space “close enough” tolerance;
- projected/reference geometry;
- new curve kinds such as Ellipse/Spline/Bezier;
- authored constraints, solver, auto-constraints or driving dimensions;
- automatic Profile RegionIntent migration/rebinding;
- persistent edit genealogy/operation history;
- Part Feature Tree or solid modeling.

## 5. Ownership and mutation path

Shared 2D owns reusable Trim/Extend geometry semantics.

Part remains the host authority for its embedded Sketch and Profile meaning. Accepted durable mutation follows:

```text
UI / pointer / Command Line
→ semantic R12 command
→ current-context validation
→ Shared-2D structural edit evaluation
→ Part transaction
→ PartDocument commit
→ derived Profile/evaluation refresh
→ presentation
```

Every command revalidates current `EntityId`, geometry, role, context and transaction freshness at execution.

## 6. Exact geometry policy

R12 operates in Sketch-local physical U/V geometry.

Screen pixels, OSNAP aperture, marker size, camera/DPI and Viewer tessellation are never topology authority.

Existing Line/Circle/Arc exact/numerically conditioned intersection utilities may be reused or extended provider-neutrally. If a new product geometric tolerance is required to decide coincidence, intersection, adjacency, closure or eligibility, stop for Owner review.

Non-finite geometry fails closed.

## 7. Trim — general semantic rule

Trim removes exactly **one connected fragment** selected from one target entity.

The operation is legal only when removing that fragment leaves exactly one representable authored primitive in the current Line/Arc/Circle vocabulary.

Conceptually:

```text
explicit finite boundaries
+ target entity
+ runtime fragment pick
→ exact cut locations on target
→ selected connected target span
→ validate one surviving authored primitive
→ commit one replacement/update
```

The target itself cannot serve as its own boundary.

The pointer/hover location chooses the target fragment at runtime; it is not persisted.

Boundaries are unchanged by Trim.

## 8. Trim — cut locations and fragment choice

A cut location is an exact discrete intersection between the target and one of the explicit finite boundary entities.

Rules:

- only intersections on the finite authored boundary participate;
- for Arc boundaries, an intersection on the supporting Circle but outside the authored Arc sweep does not participate;
- coincident/overlapping relations do not create arbitrary cut locations and fail closed for the affected target;
- tangency contributes one discrete cut location only;
- exact duplicate cut locations collapse only by canonical exact semantic/geometric contact, never by screen/world epsilon;
- multiple boundaries may contribute multiple ordered cut locations along one target.

The picked fragment is the connected target span containing the runtime pick and bounded by the adjacent eligible cut locations/end-domain boundaries appropriate to the primitive.

## 9. Trim Line

A Line may be Trimmed only by removing a terminal connected span so the result is one Line.

Accepted shape:

```text
A -------- X -------- B
                   pick
→
A -------- X
```

The resulting Line preserves:

- the original `EntityId`;
- the original Regular/Construction role;
- the untouched endpoint semantic role;
- Line primitive kind.

A middle removal:

```text
A -- X ---- Y -- B
       pick
→ would leave two Lines
```

is outside R12 and returns a typed not-applicable/unsupported structural-edit result with no mutation.

## 10. Trim Arc

An Arc may be Trimmed only by removing a terminal connected sweep span so the result is one Arc.

The surviving Arc preserves:

- original `EntityId`;
- original role;
- center/radius;
- primitive kind;
- orientation/signed sweep semantics consistent with the surviving authored span.

Removing an interior span that would leave two Arcs is outside R12 and fails without mutation.

## 11. Trim Circle

Circle Trim is explicitly required by R12.

A Circle has no authored endpoints. A valid Circle Trim therefore requires at least two distinct exact cut locations on the Circle from the explicit finite boundary set.

Ordered cut locations partition the Circle into connected arc spans. The runtime pick identifies exactly one local span between adjacent cut locations for removal.

Removing one proper connected Circle span leaves its complementary connected span, which is authored as exactly one Arc.

Therefore:

```text
Circle old EntityId
→ Trim one connected span
→ Arc fresh EntityId
```

Rules:

- source Circle `EntityId` is retired by the accepted commit;
- result Arc receives a fresh monotonic non-reused `EntityId`;
- result Arc inherits the source Regular/Construction role;
- no hidden second Arc is authored;
- one tangential cut location is insufficient;
- coincident Circle/Circle or overlapping ambiguity fails closed;
- with more than two cut locations, only the picked local span between adjacent cut locations is removed; the complementary remainder must be representable as one Arc or the operation fails.

## 12. Extend — general semantic rule

Extend changes one selected endpoint of a Line or Arc until it reaches the nearest valid exact intersection with explicit **finite authored boundary geometry** in the positive continuation direction.

Standard Extend never extends the boundary and never uses a boundary's virtual supporting line/circle outside its finite authored domain.

The target itself cannot serve as its own boundary.

The boundary remains unchanged.

The selected endpoint/side is semantic command input. Exact gesture/pick-zone treatment is D1 provided the resulting endpoint choice is unambiguous and previewed.

## 13. Extend Line

For a Line, the selected endpoint defines one positive extension ray on the Line's infinite support.

Eligible candidates are exact intersections of that ray beyond the current endpoint with finite Line/Arc/Circle boundary geometry.

Candidate choice:

1. discard non-finite, non-boundary-domain or non-positive-ray intersections;
2. discard the current endpoint/no-op contact;
3. choose the nearest positive-distance eligible intersection;
4. if the nearest result is not unique under exact/canonical geometry, fail closed.

Accepted result updates only the selected endpoint and preserves:

- `EntityId`;
- Line primitive kind;
- Regular/Construction role;
- the opposite endpoint.

## 14. Extend Arc

For an Arc:

- extending End continues along the current signed sweep direction;
- extending Start continues opposite the current signed sweep direction.

The selected endpoint continues on the same supporting Circle with unchanged center/radius.

Eligible candidates are exact intersections with finite Line/Arc/Circle boundaries that lie beyond the selected authored end along that continuation.

The nearest positive angular-travel eligible intersection is selected deterministically.

The resulting Arc must remain a proper Arc with finite, nonzero sweep and must not silently become/wrap into a full Circle. If the required continuation is ambiguous, non-finite or cannot be represented by the current Arc model without a full revolution, fail closed.

Accepted result preserves:

- `EntityId`;
- Arc primitive kind;
- center/radius;
- Regular/Construction role;
- signed orientation semantics.

## 15. Extend Both to Virtual Intersection

This is a separate operation from standard Extend.

Inputs are exactly two authored Lines.

Their **infinite supporting lines** may be used only to compute one unique virtual intersection `X`.

The operation is applicable only when:

- the supporting lines are neither parallel nor coincident;
- one finite Line does not already contain `X`;
- the other finite Line does not already contain `X`;
- for each Line, `X` lies beyond exactly one finite endpoint on its positive extension ray;
- both resulting Lines are finite and valid.

Accepted result:

```text
Line A → same EntityId, one endpoint becomes X
Line B → same EntityId, one endpoint becomes X
```

Both mutations occur in one atomic command/Part transaction and create one Undo entry.

Each Line retains its own Regular/Construction role; mixed roles are permitted because the entities remain independent.

If only one Line needs extension, the operation is not applicable; ordinary Extend is the correct command.

Exact public/UI label may be `Extend Both`; the semantic operation meaning is frozen as **Extend Both to Virtual Intersection**.

## 16. Identity lifecycle

R12 identity outcomes are:

| Structural edit | Identity result |
| --- | --- |
| Line Trim → Line | preserve source EntityId |
| Arc Trim → Arc | preserve source EntityId |
| Circle Trim → Arc | retire Circle EntityId; allocate fresh Arc EntityId |
| Line Extend → Line | preserve source EntityId |
| Arc Extend → Arc | preserve source EntityId |
| Extend Both Line+Line | preserve both EntityIds |

Fresh Circle→Arc replacement identity follows existing monotonic/non-aliasing Sketch identity lifecycle:

- committed IDs are not reused for a different semantic entity in the continuing Sketch lineage;
- Undo restores the original Circle with its original ID;
- Redo restores the same committed replacement Arc identity rather than allocating a different one;
- Save/Reopen preserves the resulting identity and identity high-water.

R12 does not persist general “derived from” genealogy.

## 17. Durable reference and Profile behavior

Preserving an entity's ID preserves semantic entity identity, not historical endpoint coordinates.

Therefore after Line/Arc Trim/Extend, a durable future `EntityId + endpoint role` reference continues to address the current semantic endpoint of that same entity.

Circle→Arc changes primitive kind and identity. No durable Circle role is silently reinterpreted as an Arc role.

Existing Part Profile semantics remain authoritative:

- ProfileId and RegionIntent are not rewritten automatically by R12;
- if Trim/Extend preserves all required Profile source identities/anchors and the intent still resolves, Profile reevaluates normally;
- if a required source identity/topology becomes unresolved (including Circle→Arc replacement), the same ProfileId/RegionIntent remains authored and its derived status becomes Invalid/Broken with a structured diagnostic;
- Undo may restore the old geometry/identity and allow the unchanged Profile intent to become Valid again;
- no “nearest/new replacement entity” rebinding is permitted.

## 18. Regular / Construction

Regular and Construction geometry are equally eligible for R12 editing and as boundaries.

Trim/Extend preserves the target's role.

Circle→Arc inherits the source Circle role.

Extend Both preserves each Line's role independently.

Construction semantics for material-region analysis remain unchanged.

## 19. Runtime tool and selection behavior

R12 tool state is runtime-only.

Semantic commands receive explicit target/boundary identities, selected target fragment/end and current context. UI may seed these inputs from current selection, but selection is not command authority.

After commit:

- no stale retired EntityId may remain bound as active semantic selection;
- preserved IDs may reconcile normally;
- exact post-operation focus/selection of a fresh Circle→Arc replacement is delegated D1 UX behavior;
- no selection state is persisted.

Esc/cancel clears only transient R12 tool state and creates no authored mutation.

## 20. Preview and presentation

Before commit, the viewport must make the proposed structural result understandable:

- Trim highlights/previews the connected fragment that would be removed;
- Extend previews the added continuation from the selected end to the exact boundary intersection;
- Extend Both previews both continuations to the same virtual intersection;
- Circle Trim preview makes the removed Circle span and surviving Arc interpretation unambiguous.

Preview/markers are non-selectable runtime presentation and never CAD identity.

Exact colors, line styles and glyphs are D1.

## 21. History, transaction and failure atomicity

Every accepted single Trim or Extend creates at most one authored mutation command and one Undo entry.

Extend Both changes both Lines in one atomic transaction and one Undo entry.

Execution revalidates the current document revision and semantic entities before commit.

Any stale, invalid, ambiguous, non-finite or unsupported result produces no partial authored mutation, no revision increment and no Undo entry.

ADR-0012 transaction freshness and one-shot commit semantics remain authoritative.

## 22. Persistence

R12 is expected to require no new authored CAD schema.

Native persistence stores the resulting ordinary Line/Arc/Circle authored geometry, roles, EntityIds and existing identity high-water through current Sketch/Part schema semantics.

R12 does not persist:

- Trim/Extend operation history;
- boundary selections;
- picked fragment coordinates;
- previews;
- virtual intersection guides;
- supporting infinite geometry;
- structural genealogy.

If implementation requires a persistent schema change, stop for Owner review.

## 23. Failure behavior

R12 fails closed for at least:

- stale/missing target or boundary EntityId;
- unsupported target/boundary kind;
- non-finite geometry;
- no eligible exact cut/intersection;
- tangent Circle Trim with only one distinct cut location;
- coincident/overlapping ambiguity;
- middle Line/Arc Trim that would leave two target entities;
- Circle Trim whose complement cannot be represented as one proper Arc;
- Extend candidate only on a boundary's virtual extension;
- Extend no-op/current endpoint contact;
- ambiguous equal nearest Extend candidates;
- Arc Extend requiring unsupported/full-revolution representation;
- Extend Both parallel/coincident Lines;
- Extend Both where the virtual intersection lies on either finite Line;
- Extend Both where only one Line requires extension;
- transaction staleness/invalid final state.

Exact diagnostic enum names/messages are D1; semantic distinction must be testable.

## 24. R11/R10 integration

R10/R11 assistance may help acquire target entities, boundary entities or points where the R12 UI uses point input.

Such assistance remains runtime-only.

OSNAP/OTRACK/Polar/DYN do not create persistent relations as a side effect of Trim/Extend.

R11 Line EXT assistance is not authority for standard R12 Extend boundary geometry and does not authorize virtual boundary intersections.

## 25. Performance boundary

Structural evaluation may inspect intersections between one target and an explicit bounded boundary set.

Do not introduce unbounded all-pairs structural scans over the whole Sketch as the default command implementation.

Reuse current provider-neutral intersection/region math and bounded semantic/view query infrastructure where appropriate.

Caches remain derived/runtime-only.

## 26. Expected implementation surface

Expected bounded surfaces:

- `src/sketch/**` for provider-neutral structural-edit geometry/evaluation;
- `src/application/**` for semantic command integration where appropriate;
- `src/ui/**` for Sketch tool orchestration and Operations controls;
- `src/viewer/**` and `src/viewer_qt_occt/**` only for provider-neutral preview/pick presentation needed by the accepted tools;
- existing Part command/transaction integration;
- `tests/**`;
- affected `docs/internal/**`, `docs/product/pl/**`, `docs/product/en/**`, generated Browser;
- `work/**`.

No provider/kernel identity may enter Shared-2D or persistence semantics.

## 27. Required automated verification

At minimum tests must prove:

- Line terminal Trim preserves EntityId/role and updates exact endpoint;
- Line middle-span Trim is rejected unchanged;
- Arc terminal Trim preserves EntityId/role/orientation;
- Arc middle-span Trim is rejected unchanged;
- Circle×Line Trim with two cuts yields exactly one Arc with fresh ID and inherited role;
- Circle×Circle and Circle×Arc representative valid trims;
- Circle Trim with multiple boundary cut locations removes the picked adjacent span only;
- tangent/one-cut Circle Trim fails unchanged;
- coincident/overlap ambiguity fails unchanged;
- fresh Circle→Arc ID is non-aliased and Undo/Redo restores exact old/new IDs;
- Line Extend to finite Line/Arc/Circle boundary;
- standard Extend rejects a candidate existing only on virtual boundary extension;
- Extend chooses nearest valid positive continuation deterministically;
- Arc Extend representative Line/Arc/Circle boundaries and sweep-direction cases;
- Extend Both computes one unique supporting-line intersection and changes both Lines atomically;
- Extend Both rejects parallel/coincident, already-intersecting/one-side-only cases;
- mixed Regular/Construction Extend Both preserves individual roles;
- R12 accepted commit creates one history entry; failure creates none;
- stale Part transaction cannot commit R12 mutation;
- Profile remains same authored ProfileId/RegionIntent and reevaluates Valid/Invalid without auto-rebinding;
- Undo can restore Profile resolvability after a breaking structural edit;
- Save/Close/Reopen preserves resulting geometry, EntityIds and high-water;
- no screen/snap/world tolerance affects structural truth;
- no R12 action authors constraints;
- R10/R11, Measure/Between, selection/grips/transforms, Rectangle, Profile and persistence regressions remain green;
- core semantic/math tests run without Qt/OCCT wherever presentation is not required;
- exact-head Windows FULL passes.

## 28. Manual Windows verification

Final candidate requires Owner verification of at least:

- Trim Line at each terminal side against representative Line/Arc/Circle boundaries;
- attempt middle Line Trim and confirm no two-piece mutation;
- Trim Arc from Start/End sides;
- attempt middle Arc Trim and confirm rejection;
- Circle Trim using Line exactly as the accepted engineering example: click one Circle span, obtain one complementary Arc;
- Circle Trim with Circle and Arc boundaries;
- Circle Trim with multiple cut locations and confirm the clicked local span is the removed span;
- tangent/overlap ambiguous Circle cases fail without mutation;
- Construction as target and boundary;
- Extend Line to Line/Arc/Circle finite boundaries;
- verify standard Extend does not use virtual boundary extension;
- Extend Arc representative cases;
- Extend Both on two Lines requiring extension to one virtual intersection;
- Extend Both rejects parallel/coincident Lines and cases where only one Line needs extension;
- visible/stable preview before commit for all three tools;
- one-step Undo/Redo for Trim, Extend and atomic Extend Both;
- Profile Valid/Invalid behavior after representative structural edits and recovery on Undo;
- Save/Close/Reopen;
- regression smoke OSNAP/OTRACK/EXT assistance, Polar/DYN, Measure/Between, Rectangle, Circle/Arc creation, transforms/grips and Profile.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: R12 adds user-visible structural editing, freezes EntityId/reference behavior for same-kind edits versus Circle→Arc replacement, and adds new failure/preview/history semantics maintainers and users must understand.

Internal documentation must explain:

- Shared-2D structural-edit ownership;
- one-result Trim rule;
- finite-boundary Extend versus mutual virtual Extend Both;
- identity matrix and Circle→Arc lifecycle;
- Profile non-rebinding behavior;
- transaction/history/failure boundaries.

PL/EN Product documentation must explain:

- Trim workflow and supported targets/boundaries;
- Circle Trim behavior;
- Extend workflow;
- Extend Both workflow;
- unsupported middle Trim/Split/Join limits;
- Construction eligibility;
- preview, Undo/Redo and common fail-closed cases.

Product Browser must be regenerated and deterministic.

## 30. Delegated D1 tuning

The Owner delegates:

- exact Operations-panel layout and wording;
- concrete tool aliases/keyboard bindings;
- boundary-set selection gesture and whether current selection seeds it;
- endpoint/side pick-zone visualization;
- hover/highlight colors, line style and glyph treatment;
- continuous-tool repetition details after each accepted Trim/Extend;
- exact diagnostic enum/type names and user-facing phrasing;
- local algorithm/data-structure choices that preserve the frozen semantic results.

D1 may not change identity, finite/virtual geometry authority, output cardinality, persistence meaning or Profile rebinding rules.

## 31. Stop conditions

Stop for Owner review if implementation requires or attempts:

- any Trim result with more than one surviving target entity;
- Split or Join;
- Circle Extend;
- standard Extend using a virtual/infinite boundary;
- Extend Both for anything other than exactly two Lines;
- automatic endpoint welding or gap healing;
- new product geometric tolerance;
- persistent operation genealogy/history;
- persistence schema change;
- automatic Profile RegionIntent rewriting/rebinding;
- durable provider/Viewer identity;
- authored constraints/solver/dimensions;
- projected/reference geometry;
- new curve kinds;
- ordinary Select RMB context;
- Part Feature Tree or solid modeling.

## 32. Activation and completion boundary

The Owner explicitly accepted and activated this exact synchronized R12 contract on 2026-10-01.

This acceptance also approves Sketcher Roadmap v1.8 as the bounded R12 scope amendment replacing the former Trim/Split/Join milestone with Trim/Extend/Extend Both to Virtual Intersection.

Implementation is authorized only inside this contract after the governance/activation candidate is recorded on the R12 branch and passes the repository's work-only governance verification.

Completion requires:

- all frozen operation and identity semantics preserved;
- affected/FOCUSED evidence during implementation;
- required internal + PL/EN Product documentation and current Browser;
- exact-head Windows FULL;
- Owner manual Windows PASS;
- work-only CLOSURE closeout.

After R12 completion the program stops at the Sketcher profile-authoring readiness checkpoint. That checkpoint does not activate Part Feature Tree or solid modeling.

## 33. Completion evidence

R12 completion evidence on 2026-10-01:

- final exact-head closeout candidate `0f62c9f4411264a240a9774c18e11a6a70fdd1b0` passed Windows FULL #1104, including exact checkout, deterministic documentation verification, bootstrap/dispatcher checks, complete desktop build graph, core-only and FAST/SUBSYSTEM verification, and full desktop **83/83 PASS**;
- required internal documentation, PL/EN Product documentation and generated Product Browser are current on the final candidate; the tool-first Trim/Extend workflow with **Enter/RMB** boundary confirmation is documented while valid preselection remains the accepted shortcut;
- Owner manual Windows verification was accepted on runtime/manual candidate `65d34048db60dae5b2fdec07ebc65e62967881c9` after the corrected classical-CAD Trim/Extend boundary-selection workflow was re-tested and the Owner reported no remaining observations;
- final FULL candidate `0f62c9f4411264a240a9774c18e11a6a70fdd1b0` differs from that manually verified runtime candidate only by additional regression coverage in `tests/r12_structural_edit_interaction_state_test.cpp`; production code, Product/Internal docs and generated Browser are unchanged across that final test-only delta;
- accepted R12 structural semantics remain intact: Line→Line and Arc→Arc preserve EntityId, Circle→Arc retires the Circle identity and allocates one fresh non-reused Arc identity, standard Extend uses finite authored boundary geometry only, and virtual supporting geometry is authorized only by atomic two-Line Extend Both;
- Trim/Extend/Extend Both mutation remains revision-bound Command → Validation → Transaction → owning PartDocument → Evaluation; failures create no partial authored mutation, runtime preview/selection remains non-authored, Profile RegionIntent is never automatically rebound, and Save/Reopen preserves resulting identities/high-water;
- Split, Join, middle multi-output Trim, Circle Extend, gap/tolerance healing, constraints/solver, projected/reference geometry, ordinary Select RMB context, Part Feature Tree and solid modeling remain outside R12.

All R12 acceptance conditions are satisfied. R12 is complete at this work-only closeout candidate. No production CAD Work Contract is active after R12 completion. The next program action is the explicit **Sketcher profile-authoring readiness checkpoint**; that checkpoint does not activate Part Feature Tree or solid modeling.

