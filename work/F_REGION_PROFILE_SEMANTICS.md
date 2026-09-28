# F — Region and Profile Semantics

**Status:** ACCEPTED — ACTIVE  
**Proposed:** 2026-09-28  
**Owner acceptance:** 2026-09-28  
**Decision class:** bounded D2 Part/Shared-2D semantics contract under AUDIT-01; durable Profile identity, Profile↔Sketch reference semantics, Construction role persistence, native schema evolution and bounded provider-neutral profile presentation/picking are authorized only after explicit Owner acceptance; detailed numerical algorithms and private implementation remain D1 while this contract is preserved  
**Foundation:** 1.0 (foundation-v1.0)  
**Program authority:** `work/AUDIT-01_ARCHITECTURE_STABILIZATION_PROGRAM.md` v1.0 — Package F  
**Baseline:** `main` at `07d319b9d72fcad839880317c3b057a8228ce2e8` after completed Package E2

## 1. Goal

Establish exact, provider-neutral planar region analysis and durable Part Profile semantics before any solid-modeling consumer such as Extrude is allowed.

Package F must make region discovery fast and natural for the user while keeping authored intent explicit:

```text
current evaluated Sketch geometry
        ↓
Shared 2D region analysis
        ↓
runtime RegionCandidate under pointer
        ↓
explicit user acceptance
        ↓
Part-owned durable ProfileId + RegionIntent
```

A Profile is not an automatic consequence of every closed loop. It is an authored Part object created only by an explicit Profile command.

Completion of F does **not** authorize Extrude or any other solid operation. A separate Owner-accepted Part solid-operation Work Contract remains mandatory after F.

## 2. Pre-contract Owner decisions incorporated by this proposal

The following product/semantic direction was explicitly agreed during Owner design discussion before this proposal was written:

- ordinary Sketch geometry may participate in material-region analysis;
- geometry marked `Construction` remains useful for selection/snap/measure/inference but does not split, close or create material regions;
- one durable `ProfileId` represents exactly one connected material region with zero or more holes;
- Profile uses a live reference to its source Sketch rather than a one-time copied contour;
- Profile keeps its identity while source geometry changes and reevaluates from current model geometry;
- loss of unambiguous topology makes the Profile derived evaluation Invalid/Broken rather than silently rebinding to a different region;
- explicit Edit Profile may intentionally replace the RegionIntent while preserving the same ProfileId;
- Add Area and Subtract Area modify the current Profile definition; they are editing modes, not a durable nested boolean-feature stack;
- Add/Subtract must still result in one connected material region;
- region selection is by a point inside a bounded region, with hover fill preview before commit;
- the pointer point is runtime selection input only and is never persistent Profile identity;
- ordinary intersections, endpoint intersections and T-junctions are supported;
- coincident/overlapping curve segments fail closed for affected region analysis;
- a point-only tangential contact does not make two material regions one connected Profile;
- no screen/DPI/zoom/snap tolerance defines closure;
- Viewer sampling/tessellation is never region/profile geometry;
- Profile creation/editing is exposed through the existing right-side Operations panel, Properties, Tree, Viewport and the same global Command Line semantic endpoint;
- Operations includes Add Area/Subtract Area, hover preview, Detect Islands, Show Boundaries, Show Problems and Find All Regions style diagnostics;
- Profile remains future-curve-compatible: Ellipse/Spline/etc. may participate later through the same evaluated-curve/region-analysis seam rather than changing Profile semantics.

This record captures design direction only. This Work Contract remains **INACTIVE** until the Owner explicitly accepts this exact proposal.

## 3. Ownership and authority

### 3.1 Shared 2D owns region geometry analysis

Shared 2D owns reusable provider-neutral geometric analysis of evaluated 2D curves:

- intersections and relation classification;
- derived topological fragments;
- bounded loops/faces;
- nesting;
- point-in-region;
- region area/perimeter diagnostics;
- structured ambiguity/failure diagnostics.

Shared 2D does not own Part Profile identity, Profile persistence, Part feature consumption or Viewer presentation identity.

### 3.2 Part owns durable Profile meaning

Part owns:

- `ProfileId`;
- source `SketchId`;
- durable `RegionIntent`;
- Profile name/visibility;
- Profile identity allocation/high-water;
- lifecycle and referential-integrity rules;
- interpretation of a valid region as a consumable Part Profile.

### 3.3 Viewer/UI remain derived adapters

Viewer may display sampled boundaries/fills and emit transient picking tokens.

Qt/OCCT/provider objects, triangulation, tessellation, screen coordinates and `PresentationToken` never become Profile identity or region truth.

## 4. Current baseline and mandatory gaps

The current baseline has:

- authored `Line`, `Circle`, `Arc` with stable Sketch-local `EntityId`;
- stable `SketchId`;
- authored=evaluated geometry because no constraint solver currently changes evaluated geometry;
- no durable Construction/Regular role in `SketchModelState`;
- no region analyzer;
- no `ProfileId`, `RegionIntent` or Profile persistence;
- no profile fill/picking surface;
- no Profile command/tool state.

Package F must add only the minimum durable/runtime concepts required to close those gaps without introducing a general parametric solver or solid feature graph.

## 5. Scope IN

- authored Sketch entity role `Regular | Construction` for current Line/Circle/Arc;
- legacy-state/load migration defaulting existing geometry to Regular;
- provider-neutral Shared 2D evaluated-curve region analysis for current Line/Circle/Arc;
- deterministic relation classification and derived topological fragmentation;
- bounded-region enumeration and point-in-region query;
- holes/nesting and connected-material validation;
- structured diagnostics for invalid/ambiguous analysis;
- Part-owned durable `ProfileId` with monotonic non-reuse semantics;
- Part-owned durable `RegionIntent` referencing source Sketch/entity topology;
- Profile Valid/Invalid derived reevaluation;
- Create Profile / Edit Profile;
- Add Area / Subtract Area;
- Delete Profile;
- Rename and independent Profile visibility;
- Undo/Redo and Save/Close/Reopen;
- Profile Tree/Properties/Viewport presentation and semantic selection;
- hover region-pick/fill during Profile tool use;
- Operations-panel diagnostics/options;
- Command Line control of the same Profile tool state;
- deterministic cached-analysis behavior so unchanged pointer motion does not rebuild the full arrangement;
- internal/Product PL/EN documentation and generated Browser;
- Windows FULL plus bounded manual Windows UX verification.

## 6. Scope OUT

- Extrude or any B-Rep solid operation;
- Body/Feature-tree architecture;
- persistent kernel topology identity;
- OCCT topology ordinals as durable references;
- automatic Profile creation for every closed region;
- bulk “create profiles for all regions” as required F behavior;
- a parametric constraint solver;
- tolerance-based automatic gap healing;
- Trim/Extend/Offset implementation;
- Ellipse/Spline/Bezier implementation;
- universal geometry-kernel abstraction;
- automatic rebinding to a geometrically “near” replacement entity after source identity disappears;
- persistent storage of evaluated loops, intersections, region cache, fill mesh or triangulation;
- using Viewer chords to define intersection/closure;
- Profile-local durable boolean operation history;
- solid-feature recompute semantics; future Extrude dependency/rebuild is a later contract.

## 7. Participating curve role

### 7.1 Authored role

Every current authored Line/Circle/Arc receives a durable semantic role:

```text
EntityRole::Regular
EntityRole::Construction
```

Existing/legacy geometry defaults to Regular.

Role is authored Sketch state and therefore:

- is persisted;
- participates in equality/state/restore validation;
- is preserved by Move/Rotate/Scale/Mirror;
- is preserved by COPY into each newly allocated entity;
- changes only through a semantic command/transaction;
- participates in Undo/Redo and dirty state.

### 7.2 Region participation

Only Regular geometry participates in material-region construction.

Construction geometry:

- remains visible according to normal presentation policy;
- remains semantically selectable;
- may be used by future/current snap/measure/inference behavior;
- never creates a boundary fragment;
- never closes a loop;
- never creates a hole;
- never splits one region into multiple regions.

Changing a boundary entity from Regular to Construction may make an existing Profile evaluation Invalid. The durable ProfileId/RegionIntent remains and may recover if the role later becomes Regular again.

## 8. Evaluated-curve seam and future curve kinds

F must not hard-code Profile semantics to “Line/Circle/Arc forever”.

The analyzer consumes a bounded provider-neutral evaluated-curve view sufficient to obtain, as applicable:

- finite geometric domain;
- whether the curve is closed;
- semantic endpoints for open curves;
- point/evaluation along the curve;
- deterministic intersections/relations with supported curve kinds;
- a directed subcurve between resolved anchors;
- model-space bounds where useful for acceleration.

Current F completion requires full region participation for current Line, Circle and Arc only.

Future Ellipse/Spline/etc. may extend this seam without changing ProfileId/RegionIntent semantics.

A future Regular curve kind that does not yet support region analysis must never be silently ignored when it can affect analysis. It must produce a structured unsupported/fail-closed result until that curve kind has an accepted region-analysis implementation.

No speculative universal curve framework beyond demonstrated F needs is authorized.

## 9. Model-space closure and numerical policy

Region validity is independent of screen pixels, camera, DPI and snap aperture.

For authored semantic endpoints:

```text
exact equal evaluated coordinates
→ same endpoint location

different evaluated coordinates
→ not silently welded
```

F introduces no Product world-space “close enough” tolerance and no automatic gap repair.

Computed intersections are numerical geometric results from authoritative evaluated curves. Algorithms must be deterministic, finite-checked and numerically conditioned for the supported Line/Circle/Arc cases.

The analyzer builds explicit topological vertices/edge uses from geometric relations. Loop closure is a topological property of that derived arrangement; it is not decided by a final screen/world distance epsilon between first and last displayed point.

If completing F requires a new Product geometric tolerance, coordinate limit or feature-size rule, stop for explicit Owner decision before introducing it.

## 10. Required relation handling

For current Line/Circle/Arc combinations, classify at least:

- no intersection;
- proper crossing;
- endpoint↔endpoint intersection;
- endpoint-on-curve / T-junction;
- tangent contact;
- coincident/overlapping interval;
- invalid/non-finite relation.

Required behavior:

- proper crossings are valid arrangement vertices;
- endpoint intersections/T-junctions are valid arrangement vertices;
- intersections create derived curve fragments only; they never split authored EntityId or create authored geometry;
- tangent contact may be represented geometrically, but a point-only contact does not make two positive-area material regions one connected Profile;
- coincident/overlapping curve intervals make affected region topology ambiguous and fail closed;
- duplicate/coincident Regular geometry is not arbitrarily deduplicated;
- the source Sketch may contain open chains or unrelated geometry; F does not require the whole Sketch to be one clean contour.

Current Line/Circle/Arc do not have authored self-intersecting primitive geometry. A future self-intersecting curve kind must explicitly define its derived fragmentation before participating.

## 11. RegionCandidate semantics and point picking

A `RegionCandidate` is a derived bounded positive-area face from the current valid arrangement.

It may have:

- one outer boundary;
- zero or more hole boundaries.

Orientation is not the semantic definition of outer versus hole; containment/nesting is. Evaluated output may normalize traversal orientation for downstream consumers.

Pointer selection:

```text
Sketch-space pointer point
        ↓
strict point-in-bounded-region query
        ↓
exactly one RegionCandidate
```

A pointer exactly on a boundary/intersection is not durable intent and must not select an arbitrary adjacent region. The tool asks the user to pick inside an area.

The unbounded exterior is never a Profile candidate.

Duplicate authored Profiles may intentionally refer to the same currently evaluated region; Profile identity is user-authored intent, not geometric deduplication.

## 12. One ProfileId = one connected material region

A valid Profile evaluation contains:

- exactly one connected positive-area material region;
- one simple outer material boundary;
- zero or more simple hole boundaries;
- holes strictly contained by the material region;
- no hole touching outer boundary;
- no holes touching/overlapping each other;
- no ambiguous overlapping source boundary;
- no point-only connection between otherwise disconnected material components.

A Profile may never contain several disconnected material islands under one ProfileId.

If an edit/Add/Subtract result would produce several disconnected positive-area regions, the operation is rejected before authored mutation.

## 13. Detect Islands and nesting

Nested arrangements may contain disconnected material “islands” inside holes.

Example:

```text
outer material
  hole
    inner bounded material island
```

The inner island is **not** part of the outer Profile because there is no positive-area material connection.

`Detect Islands` is a runtime diagnostic/preview option:

- default ON for Profile tool;
- highlights/counts disconnected bounded candidates nested inside holes of the current/hovered result;
- never merges them into the same ProfileId;
- never silently creates extra ProfileIds.

Bulk automatic Profile creation is not required by F. A later explicit productivity command may create multiple Profiles, but it is not a prerequisite for region/profile correctness.

## 14. Durable Profile model

Conceptually:

```text
Profile
  ProfileId
  SketchId
  Name
  Visible
  RegionIntent
```

Required identity rules:

- ProfileId is stable and scoped by the owning PartDocument;
- default/invalid identity cannot denote a Profile;
- accepted Create Profile allocates a fresh identity;
- deletion never makes that identity allocatable again within the continuing document lineage;
- Undo of creation removes the Profile but does not permit aliasing its committed ID to a different future Profile;
- Redo restores the same ProfileId;
- Save/Reopen preserves identity high-water;
- Rename never changes ProfileId;
- display name is not reference identity.

A default user label such as `Profile001` may derive from the monotonic Profile allocation sequence; user rename remains an authored label.

## 15. RegionIntent

The click/hover point is never persisted.

Durable Profile intent records which semantic source boundary the user accepted.

Conceptually:

```text
RegionIntent
  outer: LoopIntent
  holes: LoopIntent[]

LoopIntent
  ordered BoundaryUseIntent[]

BoundaryUseIntent
  source EntityId
  source-span identity
  traversal direction
```

F must support at least two source-span forms:

1. whole closed source curve, needed for a complete Circle boundary;
2. directed subcurve between two semantic anchors.

Semantic anchors may refer to:

- authored endpoint role;
- deterministic intersection with another source EntityId plus branch identity.

Raw world coordinates, Viewer token, vector index, OCCT object, tessellated segment index and pointer seed are not durable anchors.

Exact C++ type names/layout are D1, but persistence must encode equivalent semantic information.

## 16. Deterministic intersection branch identity

Two source curves may have more than one intersection.

An anchor therefore cannot be only:

```text
Intersection(EntityA, EntityB)
```

The region analyzer must expose deterministic branch identity derived from canonical curve geometry/parameterization, never provider result order.

Conceptually:

```text
Intersection(EntityA, EntityB, canonical_branch)
```

For a supported curve pair, the same valid evaluated geometry must produce the same canonical ordering independent of storage order/provider iteration.

If a required two-intersection relation becomes tangent/one-intersection/zero-intersection or otherwise loses the required branch, Profile evaluation fails closed.

The complete ordered LoopIntent must also validate as the same closed semantic boundary topology; branch identity alone is not sufficient to bypass loop validation.

## 17. Live reference reevaluation

Profile is a live reference to current evaluated geometry of its source Sketch.

On relevant Sketch authored/evaluated change:

1. resolve the same SketchId;
2. resolve every RegionIntent source EntityId;
3. resolve required semantic endpoints/intersection branches;
4. reconstruct directed boundary uses;
5. validate loop closure/simplicity;
6. validate overlap/tangency/nesting rules;
7. validate exactly one connected material region.

If all pass:

```text
same ProfileId
same RegionIntent
new derived geometry
Status = Valid
```

If any required source/topology is no longer unambiguous:

```text
same ProfileId
same RegionIntent
Status = Invalid
structured diagnostic
```

Automatic reevaluation never searches for a “nearest” replacement region and never rewrites RegionIntent.

If later source edits restore the same resolvable intent, the Profile may return Invalid → Valid automatically with no authored Profile command.

## 18. Source deletion rules

Deleting/replacing an EntityId referenced by RegionIntent is allowed as an explicit Sketch edit.

The Profile remains authored and reevaluates to a structured Invalid state such as MissingSourceEntity. This is the explicit referential-integrity policy for Profile→entity references; the dangling semantic intent is not silent because evaluation/status/diagnostics expose it.

Recreating geometrically identical geometry with a fresh EntityId does not automatically repair the Profile.

The user may repair it with Edit Profile, which replaces RegionIntent while preserving ProfileId.

Deleting an entire source Sketch is different. Part must not commit a Profile whose source SketchId does not exist. A future/current Sketch-delete command must either:

- reject while dependent Profiles exist; or
- explicitly delete the selected Sketch and dependent Profiles atomically after user-approved semantics.

F must not leave a silently missing source Sketch reference.

## 19. Create Profile interaction

Toolbar adds a distinct `Profile` section/tool; Profile is not classified as a Line/Circle/Arc creation primitive.

Starting `PROFILE` creates a runtime draft only.

Default interaction:

```text
Profile — Create
Mode: Add Area

hover inside bounded region
→ region pick
→ filled result preview

LMB
→ update transient draft

Finish Profile
→ one semantic command / one Part transaction / one Undo entry
```

Before Finish, no ProfileId is consumed and no authored Profile exists.

After successful Finish, the Profile tool may remain active with a fresh empty draft so the user can create another Profile quickly. Esc/cancel returns through the established tool-state hierarchy to Select.

## 20. Edit Profile, Add Area and Subtract Area

Edit Profile opens the source Sketch edit context and a transient draft initialized from the current durable RegionIntent.

`Add Area`:

```text
draft material UNION hovered RegionCandidate
```

is accepted only if the result is one valid connected material region.

This supports adding an adjacent arrangement cell and removing its shared internal boundary.

A region that is disconnected or only touches the current material at a point is rejected.

`Subtract Area`:

```text
draft material MINUS hovered RegionCandidate
```

may create:

- an additional hole; or
- a notch/opening that reaches the outer boundary,

provided the final result remains one valid connected material region.

Subtract that would split material into multiple disconnected islands is rejected.

Add fully contained in current material or Subtract fully disjoint from it is a no-op and creates no authored mutation.

Add/Subtract clicks update only the transient draft. `Finish` replaces the durable RegionIntent of the same ProfileId with the resulting boundary intent as **one** authored command/transaction/Undo entry for the complete edit session.

Cancel/Esc discards the draft and leaves authored Profile unchanged.

The durable Profile does not store a history such as “Add then Subtract then Subtract”; only the resulting RegionIntent is authored.

## 21. Profile Invalid state and diagnostics

Valid/Invalid is derived evaluation state, not a persisted status bit.

Structured evaluation diagnostics must distinguish at least the meaningful categories:

- missing source entity;
- required intersection/branch missing;
- open boundary;
- ambiguous overlap/coincident geometry;
- zero-width/tangential connectivity invalid for material;
- invalid hole/nesting;
- disconnected material;
- unsupported curve kind;
- invalid/non-finite geometry.

Exact enum names are D1.

An Invalid Profile:

- remains in the Tree;
- retains ProfileId, name, visibility and RegionIntent;
- is not displayed as a stale valid filled material region;
- exposes the diagnostic in Properties/Operations;
- may highlight resolvable offending source geometry as runtime diagnostic presentation;
- may be repaired by restoring source geometry or by explicit Edit Profile redefinition.

Old derived Area/Perimeter values must not be presented as current when Profile is Invalid.

## 22. Undo/Redo and transaction authority

All durable Profile/role mutations use the existing authority path:

```text
UI / Command Line
→ semantic command
→ validation
→ Part transaction
→ PartDocument
→ history
→ derived reevaluation/presentation
```

Required authored operations include the semantic equivalents of:

- Set Sketch Entity Role;
- Create Profile;
- Replace/Edit Profile RegionIntent;
- Set Profile Properties/Visibility;
- Delete Profile.

Each accepted logical operation produces one normal revision/Undo entry.

Hover, region discovery, draft Add/Subtract, Find All Regions, Detect Islands and diagnostics are runtime-only and create no revision, dirty state, identity allocation or history.

Undo/Redo must preserve ProfileId high-water so abandoned committed Profile IDs are not reused for different semantic objects.

## 23. Persistence and migration

F is an explicit persistence/schema change.

The next native Part schema version must persist at least:

- current Sketch entity role for every Line/Circle/Arc;
- Profile collection;
- ProfileId;
- Profile identity high-water/cursor;
- source SketchId;
- Profile name/visibility;
- RegionIntent including loop/boundary-use/anchor semantics.

It must not persist:

- evaluated Profile loops;
- RegionCandidate cache;
- intersection cache;
- Valid/Invalid result;
- derived area/perimeter;
- hover selection;
- presentation token;
- sampled fill mesh;
- OCCT/provider objects.

Migration from the current pre-F schema must:

- preserve all current Sketch geometry and identities;
- assign Regular role to legacy Line/Circle/Arc;
- create no automatic Profiles;
- initialize Profile identity allocation consistently;
- pass Save → Close → Reopen and legacy-load regression.

A malformed RegionIntent/persistence payload fails closed; restore must not invent replacement geometry.

## 24. Tree and Properties UX

Profiles are Part-owned semantic objects but are grouped visually beneath their source Sketch in Document Tree for user comprehension:

```text
Part
└─ Sketch001
   ├─ Profile001
   └─ Profile002
```

Tree row identity maps to ProfileId, never name/index/provider token.

Selecting a valid Profile exposes Properties similar to:

```text
Profile
Name        Profile001
Source      Sketch001      [read-only]
Status      Valid          [derived]
Visible     true

Area        derived
Perimeter   derived
Holes       derived count

Edit Profile
```

Name and Visible are authored.

Area/Perimeter/Holes are read-only derived values. F does not invent a new Product unit system; formatting must use the current accepted model/display-unit policy or an explicitly neutral model-unit presentation.

For Invalid Profile:

- Status shows Invalid;
- structured reason is surfaced;
- Area/Perimeter are unavailable rather than stale;
- Edit Profile remains available.

## 25. Operations-panel UX

The existing right-side Operations panel is the primary contextual surface for the Profile tool. No modal dialog is required for ordinary use.

Create/Edit panel direction:

```text
Profile — Create/Edit
Profile001                 # when editing

Operation
  Add Area
  Subtract Area

Detection
  Detect Islands           default ON
  Highlight on Hover       default ON
  Show Region Boundaries   default OFF
  Show Problems            default ON

Current result
  Status
  Area
  Perimeter
  Holes
  Islands

Find All Regions

Finish Profile / Finish
Cancel
```

`Find All Regions` is diagnostic only:

- analyzes the current Sketch;
- may highlight bounded candidates;
- reports bounded regions, ambiguity/open-chain/problem counts as available;
- creates no Profile automatically.

`Show Problems` may expose read-only gap measurements or affected entities. A measured gap is diagnostic and must not become an implicit closure tolerance or auto-repair.

No `Auto Close Gaps` mutation belongs to Profile tool.

## 26. Hover preview and region picking

While Profile tool is active and geometry revision is unchanged:

- pointer movement performs fast point-in-region against cached current region analysis;
- hover produces translucent fill of the exact candidate/result semantics through derived presentation;
- Add/Subtract hover shows the **resulting draft Profile**, not merely the clicked cell;
- invalid hovered result produces diagnostic preview and cannot commit;
- pointer hover alone never mutates authored state.

The visible fill may use sampled/triangulated presentation geometry, but semantic result comes only from Shared 2D region analysis.

## 27. Part-mode profile presentation and selection

After Finish Sketch, a valid visible Profile remains selectable as a flat Part object on the source Sketch plane even when source Sketch geometry visibility is off.

Profile visibility is independent from Sketch visibility.

Provider-neutral presentation maps a runtime Profile presentation token immediately to ProfileId.

Selecting Profile chooses the Profile semantic object, not its boundary Line/Circle/Arc entities.

Deleting selected Profile deletes only Profile; it does not delete source Sketch geometry.

F may make the minimum bounded `IDocumentViewport`/presentation contract extension needed for:

- profile fill/boundary presentation;
- profile hover/selection;
- transient region/draft fill.

This D2 surface expansion is authorized only if this Work Contract is explicitly accepted.

No durable provider/native identity is introduced.

## 28. Command Line

Profile uses the existing global Command Line transport and semantic context/lifetime rules. It must not introduce a second command parser/state machine in QWidget.

Top-level accepted commands for F:

```text
PROFILE
EDITPROFILE
```

Contextual Profile commands:

```text
ADD
SUBTRACT
FIND
FINISH
CANCEL
```

Operations controls and Command Line update the same Profile tool state.

Optional text toggles may mirror panel options:

```text
ISLANDS ON|OFF
BOUNDARIES ON|OFF
PROBLEMS ON|OFF
```

Exact aliases are D1 if documented consistently.

`PROFILE` always starts a new Profile draft.

`EDITPROFILE` edits exactly one semantically selected/identified Profile and enters its source Sketch context; ambiguous/missing selection fails with diagnostic.

F does not add special coordinate-text grammar. Region picking remains pointer-based until a separate accepted general coordinate-input capability can supply a Sketch-space point through the shared semantic input system.

## 29. Analysis caching and responsiveness

Region analysis may be materially more expensive than point-in-region.

F must therefore separate:

```text
authored/evaluated Sketch revision or role change
→ rebuild/invalidate region arrangement

pointer motion with unchanged geometry
→ query existing arrangement
```

Deterministic tests must prove repeated hover pointer moves on the same analysis revision do not rebuild full arrangement.

Cache is runtime-only and disposable.

No machine-specific Product latency threshold is introduced by F. If profiling later shows a material scale problem, optimization requires evidence rather than changing semantic rules.

## 30. Expected implementation surface after acceptance

Likely bounded areas include:

- Shared 2D neutral geometry/region-analysis code under `src/sketch` or a narrowly named neutral 2D analysis target;
- current Line/Circle/Arc state/model for authored EntityRole;
- Part domain Profile model/identity/state validation;
- Part commands/DocumentSession integration;
- native Part persistence schema/migration;
- Part/Sketch interaction controller Profile tool state;
- CadWorkbench toolbar/Operations/Properties integration;
- Document Tree Profile rows;
- provider-neutral Viewer/profile presentation/picking bridge;
- Qt/OCCT derived fill/picking implementation;
- existing tests expanded with focused region/Profile cases;
- internal docs, paired Product PL/EN docs and generated Browser.

Avoid a new subsystem/library unless dependency direction or testability clearly requires it.

## 31. Verification strategy — necessary minimum

Follow the Owner directive to consolidate evidence into the smallest practical number of existing test targets. Do not create a new CTest executable merely to mirror assertions that fit an existing boundary test.

Focused evidence must cover:

### 31.1 Shared 2D region analysis

- Line rectangle region;
- standalone Circle bounded region;
- Arc+Line mixed closed region;
- line-line, line-circle, line-arc, circle-circle, circle-arc, arc-arc relation cases needed by accepted regions;
- proper crossings and T-junction derived fragmentation;
- open gap does not close;
- tangent/point-only connectivity rule;
- coincident/overlap ambiguity;
- nested loops/holes/islands;
- Construction exclusion;
- point-in-region deterministic candidate selection;
- boundary-point ambiguity/no arbitrary adjacent pick.

### 31.2 Profile intent/rebinding

- ProfileId creation/non-reuse/Undo/Redo;
- whole-Circle and subcurve boundary intent;
- deterministic multi-intersection branch identity;
- source geometry move preserves valid live Profile where intent still resolves;
- missing entity/intersection makes Invalid without rewriting intent;
- geometry restoration can return Invalid→Valid;
- geometrically identical replacement with fresh EntityId does not auto-rebind;
- explicit Edit Profile preserves ProfileId and replaces RegionIntent;
- duplicate Profiles on same region are legal.

### 31.3 Add/Subtract

- adjacent Add merges into one connected material region;
- disconnected Add rejected;
- point-only Add rejected;
- Subtract creates hole;
- Subtract creates valid notch;
- Subtract that splits material rejected;
- no-op Add/Subtract creates no authored mutation;
- entire edit session commits as one Undo step.

### 31.4 Persistence/lifecycle

- current-schema Save/Close/Reopen;
- legacy pre-F load defaults role to Regular and creates no Profile;
- ProfileId cursor/high-water survives Undo and reopen;
- malformed Profile intent fails closed;
- source Sketch cannot disappear while Profiles still reference it without explicit dependent handling.

### 31.5 UI/input/presentation

- hover queries cached analysis without authored mutation;
- repeated pointer movement does not rebuild arrangement;
- Operations Add/Subtract/options drive the same state;
- `PROFILE` / `EDITPROFILE` / context commands reach the same semantic paths;
- Invalid diagnostic appears without stale valid fill;
- Tree/Properties selection uses ProfileId;
- valid Part-mode Profile picking maps provider token to ProfileId;
- Sketch visibility and Profile visibility are independent;
- Viewer tessellation/fill cannot become region truth;
- existing Sketch Line/Circle/Arc selection, transforms, picking and E2 failure recovery remain green.

### 31.6 Manual Windows acceptance

Because F introduces substantial new user-visible workflow, final completion requires bounded Owner manual verification on the exact documented candidate:

- create Profile by hover + click;
- create Circle/Arc/mixed-boundary profiles;
- Add Area;
- Subtract hole and notch;
- rejected disconnected result;
- Detect Islands;
- Show Problems/open gap;
- Construction exclusion;
- Finish Sketch and select Profile in Part;
- Edit Profile and preserve identity;
- make Profile Invalid by source edit, repair it, observe recovery;
- Undo/Redo;
- Save/Close/Reopen;
- Command Line parity.

## Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: F introduces durable Construction/Profile semantics, a new region-analysis boundary, persistence/schema changes and user-visible Profile creation/editing/diagnostics that require current internal and paired Product PL/EN documentation.

At minimum document:

- Shared 2D region analysis/curve participation;
- Construction role;
- Profile ownership/identity/RegionIntent/rebinding;
- persistence schema/migration;
- Viewer presentation-only boundary;
- Profile tool/input architecture.

Product PL/EN documentation must cover:


- Create/Edit Profile;
- hover region pick;
- Add/Subtract;
- Detect Islands / Find All Regions / diagnostics;
- Construction behavior;
- Valid/Invalid meaning and repair;
- Properties/visibility;
- Command Line commands;
- Finish Sketch / Part Profile selection;
- explicit statement that Profile does not itself perform Extrude.

Generated Browser must be regenerated canonically and pass freshness validation.

## 33. Stop conditions

Stop for explicit Owner review before unapproved expansion if completing F would require:

- a Product geometric tolerance/automatic gap-healing policy;
- a full constraint solver;
- automatic nearest/similar-region rebinding;
- multiple disconnected material islands under one ProfileId;
- persistent evaluated geometry/cache/tessellation;
- OCCT/provider topology identity in durable state;
- a general Body/Feature/Extrude architecture;
- universal geometry framework beyond F requirements;
- bulk auto-generation of Profiles not explicitly selected by the user;
- changing current input/selection grammar outside the bounded Profile context;
- cross-document/profile dependencies;
- a new persistence meaning not described by this contract.

## 34. Activation and completion boundary

**Owner acceptance:** 2026-09-28  
**Proposal evidence:** exact-head `4d1acb09aa4406459bbf2a03f00ce6d7ea9d96a9` — Windows proposal/CLOSURE-class gate #740 PASS  
**Activated branch:** `proposal-f-region-profile-semantics`

The Owner explicitly accepted this Work Contract on 2026-09-28. Package F is now the only active AUDIT-01 production scope.

The accepted D2 decisions include durable Part-owned ProfileId/RegionIntent live-reference semantics, authored Regular/Construction role persistence, fail-closed topology rebinding, one-connected-material-region Profile semantics, Add/Subtract editing, provider-neutral Profile presentation/picking, Command Line parity and the native schema evolution described above.

Extrude and all other solid-modeling consumers remain inactive. F does not authorize a demonstrational/provisional solid operation.

F completion requires:

- all required semantics implemented and documented;
- exact-head Windows FULL;
- required manual Windows verification;
- work-only CLOSURE;
- merge to main.

After F completion, AUDIT-01 may complete its A→F stabilization program, but Extrude still requires a separate Owner-accepted Part solid-operation Work Contract.
