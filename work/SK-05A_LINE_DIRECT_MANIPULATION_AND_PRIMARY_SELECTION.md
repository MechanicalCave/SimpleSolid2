# SK-05A — Line Direct Manipulation and Selection Foundation

**Status:** ACCEPTED — ACTIVE  
**Owner acceptance:** 2026-09-26  
**Decision class:** D2 Architecture + implementation  
**Foundation:** 1.0 (foundation-v1.0)  
**Architecture:** ADR-0003, ADR-0008, ADR-0009, ADR-0010  
**Program roadmap:** `work/SKETCH_ROADMAP.md` v1.2  
**Roadmap milestone:** R5 — Direct-manipulation and selection foundation on Line  
**Implementation authorization:** ACTIVE — Owner accepted roadmap v1.2 and SK-05A on 2026-09-26

## 1. Context

R4 proved the first complete authored Line workflow:

- one SketchInteractionState authority;
- continuous Line creation;
- semantic selection by EntityId;
- Window/Crossing queries;
- atomic Delete;
- Command/Transaction history;
- provider-neutral pointer mapping;
- transient preview;
- Undo/Redo and persistence.

WB-01B then stabilized provider-surface overlay composition and froze provider-surface Navigation Cube ownership.

R5 now establishes the first reusable direct-manipulation architecture. The slice remains Line-only in authored geometry, but it must already obey the final selection and manipulation grammar required by later Circle/Arc and common transforms.

Sketcher roadmap v1.2 is authoritative for this contract.

## 2. Goal

Deliver one coherent Line editing workflow:

1. Select one or more Lines using the revised additive/toggle grammar.
2. Show Start, Center and End grips on every selected Line.
3. Hover may prehighlight an entity or grip without changing selection.
4. Click exactly one grip to start a DirectManipulationSession.
5. Freeze the current selection set as the session snapshot.
6. Endpoint grip defaults to Reshape of only its owning Line.
7. Center grip defaults to Move of the entire frozen selection snapshot.
8. Pointer movement creates transient preview only.
9. LMB or Enter commits one validated semantic operation.
10. Esc cancels only the active manipulation and preserves selection.
11. A later Esc in Select clears selection.
12. Existing EntityIds survive edit, Undo/Redo and Save → Close → Reopen.

This contract intentionally does not ship Rotate, Scale, Mirror, Copy, Object Snap, numeric input, Circle or Arc.

## 3. Selection grammar change

SK-05A deliberately replaces the R4 replace-on-click product grammar.

### 3.1 Point selection

In ordinary Select:

- LMB on an unselected Line adds it to the current selection set and makes it primary;
- LMB on an already selected Line leaves membership unchanged and makes it primary;
- Ctrl+LMB toggles Line membership;
- if Ctrl removes the primary Line, primary is reconciled deterministically from semantic selection state and never from provider ordering;
- LMB on blank space clears selection and primary;
- Esc with non-empty selection clears selection and primary;
- Esc with empty selection is a no-op.

The internal replaceSelection capability may remain for bounded internal use. It is no longer the ordinary LMB product rule.

### 3.2 Rectangle selection

Window/Crossing keeps the accepted directional meaning:

- left-to-right: Window, fully contained entities;
- right-to-left: Crossing, contained or intersected entities.

The membership grammar changes:

- ordinary Window/Crossing adds returned EntityIds to the current selection;
- Ctrl+Window/Crossing toggles returned EntityIds;
- provider result order must not define primary;
- rectangle selection does not implicitly erase earlier selection.

### 3.3 Primary

Primary is runtime semantic state used for contextual presentation where useful.

Re-clicking an already selected Line may make it primary without changing selection membership.

Primary is not durable and is not provider identity.

## 4. Selection lifetime across tools

Selection remains runtime state independent from the currently active creation/edit tool.

When Line creation is activated:

- existing selection is preserved;
- grips become hidden/inactive while Line is active;
- committed new Lines are not automatically added to selection;
- Finish/Cancel Line returns to Select and restores grip presentation for the preserved selection.

Switching to another tool cancels only uncommitted transient manipulation state. It must not silently clear selection unless that tool's accepted contract explicitly requires it.

## 5. Hover / preselection

Introduce runtime hover/preselection for Line entities and their visible grips.

Rules:

- hover may lightly highlight the entity or grip;
- hover never changes semantic selection;
- hover never creates authored mutation/history;
- LMB performs the actual selection or grip activation;
- if a visible grip and underlying geometry overlap, grip hit testing has priority.

Exact visual styling, pixel size and provider-local hit-test implementation are D0/D1 details within this contract.

## 6. Runtime grips

Every selected Line exposes three runtime grips:

- LineStart;
- LineCenter;
- LineEnd.

A semantic grip reference conceptually contains:

- SketchId;
- EntityId;
- HandleRole.

Grip identity is runtime only and must never be serialized.

Locations derive from current evaluated Line geometry:

- Start = authored/evaluated start;
- End = authored/evaluated end;
- Center = midpoint of Start and End.

Center does not create a durable midpoint entity.

Exactly one grip may be active at a time.

Multiple simultaneously active grips are explicitly out of scope and are not a future requirement of the accepted roadmap.

## 7. DirectManipulationSession

Direct manipulation remains part of the single Sketch interaction authority; it must not become a second model/tool authority.

A bounded session conceptually contains:

- active SketchId;
- active EntityId;
- active HandleRole;
- frozen selected EntityId snapshot;
- current EditMode;
- interaction-start authored/evaluated geometry needed for preview;
- pivot/reference point implied by the active grip;
- current resolved Sketch-local input;
- transient preview.

The session selection snapshot is frozen until commit or Esc.

Selection membership cannot be modified while a direct manipulation session is active.

## 8. Default Line grip semantics

### 8.1 Start and End

LineStart and LineEnd default to Reshape.

For Line A→B:

- active Start with resolved point P previews P→B;
- active End with resolved point P previews A→P.

The affected set is only the owner Line of the active endpoint grip, even when other Lines are selected.

Coincident coordinates with another Line do not imply shared identity or propagated movement.

### 8.2 Center

LineCenter defaults to Move.

For one selected Line, the whole Line translates rigidly.

For multiple selected Lines, clicking the Center grip of any selected Line immediately enters Move for the entire frozen selection snapshot.

The clicked Center grip is the pivot/reference point.

For each entity, the same translation delta is applied.

This is one atomic multi-entity operation on commit.

## 9. Entry, preview, commit and cancel

### 9.1 Start

A direct manipulation starts by clicking a visible grip.

The user does not need to hold the mouse button through the entire operation.

The active grip becomes the session pivot/reference point.

There is no "change Base Point" option inside a grip-started session.

### 9.2 Preview

Pointer movement resolves through the existing Sketch plane mapping and the new shared resolved-input seam.

For SK-05A the resolver may initially be identity:

raw valid Sketch-local U/V → resolved U/V

Even in this identity stage, tools must consume resolved input rather than bind authored mutation directly to raw provider coordinates.

Preview is runtime only and must not:

- mutate SketchModel;
- increment DocumentRevision;
- dirty the Document;
- allocate EntityIds;
- create Undo entries.

Preview for multi-Line Move must show the whole affected set coherently.

### 9.3 Commit

LMB or Enter commits the current valid preview.

A commit must execute a semantic application command through the normal transaction path.

No exact no-op result creates a command/history entry.

After a successful non-Copy commit:

- authored geometry reflects the previewed result;
- all edited existing EntityIds are preserved;
- the DirectManipulationSession ends;
- the same semantic selection remains selected;
- grips regenerate from current geometry;
- interaction returns to Select.

### 9.4 Esc hierarchy

While direct manipulation is active:

- Esc discards only the uncommitted preview;
- ends the DirectManipulationSession;
- returns to Select;
- preserves the selection set;
- creates no authored mutation.

A subsequent Esc in ordinary Select clears selection and hides grips.

## 10. Semantic commands and transaction path

The authored command API must express resulting semantic geometry, not provider grip gestures.

A bounded implementation may use one or more commands equivalent to:

- UpdateSketchLineGeometryCommand for one-Line reshape;
- TransformSketchEntitiesCommand or another bounded semantic batch command for multi-Line translation.

Exact type names are D1 if ownership and public semantics remain consistent with this contract.

Validation must fail closed if:

- SketchId no longer resolves;
- any target EntityId is invalid or missing;
- any target is not an editable Line in this contract;
- replacement geometry is invalid;
- the current document/history context is stale;
- the target set differs from the frozen semantic intent in a way that makes commit ambiguous.

No partial mutation is legal.

## 11. Atomicity and history

One accepted user commit means:

- one semantic command;
- one staged authored state;
- one Part transaction;
- one revision increment when changed;
- one Undo entry.

For center-grip multi-Line Move, all selected Lines move atomically or none move.

Editing preserves EntityId.

Undo restores prior geometry with the same EntityIds.

Redo reapplies geometry with the same EntityIds.

Save → Close → Reopen preserves edited geometry and identity.

## 12. Undo / Redo while manipulation is active

Existing R4 history policy remains the model:

- cancel current transient interaction first;
- clear preview;
- return to Select while keeping/reconciling semantic selection;
- execute ordinary global DocumentSession Undo/Redo;
- refresh authored presentation;
- reconcile selected EntityIds against current model;
- regenerate grips from current evaluated geometry.

SK-05A must not introduce a second session-local history stack.

## 13. Tool switching

If another tool is explicitly activated while a direct manipulation preview is active:

- cancel only the uncommitted direct-manipulation state;
- preserve prior authored commits;
- preserve semantic selection;
- activate the requested tool.

The user does not need to press Esc first.

## 14. Provider-neutral Viewer boundary

The common Viewer boundary may be extended with the smallest finite contract needed to:

- present grips for multiple selected Lines;
- visually distinguish normal, hover and active grip states;
- hit-test grips with grip-over-geometry priority;
- expose provider-neutral hover and grip activation/input;
- present transient direct-manipulation preview for the affected set.

Qt/OCCT types, handles and provider identity must not cross into Shared 2D/Application semantics.

The provider owns:

- pixels;
- grip marker geometry/style;
- HiDPI behavior;
- provider-local hit testing;
- native in-surface drawing;
- redraw lifecycle.

The provider does not own:

- selected EntityIds;
- primary;
- frozen selection snapshot;
- EditMode;
- authored Line coordinates;
- semantic commands;
- Undo history.

Runtime grip/hover/preview graphics must remain inside the stable provider/native CAD surface path rather than reintroduce QWidget-over-native-viewport composition.

## 15. Input/focus rules in this slice

Viewport CAD focus and text-entry focus remain distinct.

For SK-05A:

- LMB grip activation, pointer movement, LMB commit, Enter commit and Esc cancel are semantic interaction actions;
- text-entry widgets must not accidentally trigger viewport actions;
- future Space-based EditMode cycling is structurally reserved but not required to ship in SK-05A.

The resolved-input seam must allow later numeric, Object Snap, Ortho/Polar and Dynamic Input adapters without changing authored command semantics.

## 16. Architecture compatibility commitments for later R7+

SK-05A must not encode assumptions that prevent the accepted roadmap behavior:

- HandleRole remains separate from EditMode;
- common Move/Rotate/Scale/Mirror can later act on the frozen selection set;
- entity-specific Reshape can remain owner-only;
- grip-started transforms use the active grip as pivot;
- command-started transforms may later use explicit Base Point/two-point mirror-axis input;
- one shared transform core may later serve grip, toolbar and Command Line adapters;
- Copy may later act as an orthogonal modifier with fresh EntityIds;
- EditMode cycling may later use Space without provider/Qt enums becoming semantic state;
- multiple active grips are not required.

This section constrains architecture shape only. It does not authorize those later features in R5.

## 17. Deliberately out of SK-05A

Not implemented by this contract:

- Circle;
- Arc;
- Rectangle;
- Polyline;
- Rotate;
- Scale;
- Mirror;
- Copy;
- repeated Copy;
- explicit Base Point command workflow;
- command-first MOVE/COPY/ROTATE/SCALE/MIRROR;
- Repeat Last Command;
- Object Snap;
- Object Snap Tracking;
- Temporary Snap Override;
- Ortho;
- Polar Tracking;
- Grid/Grid Snap;
- geometric inference;
- Dynamic Input;
- numeric coordinate/value parser;
- unit suffix parsing;
- Measure;
- Show Dimensions;
- construction geometry;
- Auto-Constraint;
- constraints/solver;
- authored dimensions;
- Trim/Extend/Offset/Fillet/Chamfer;
- region/profile analysis;
- projected/reference geometry;
- planar-face Sketch support;
- multi-active grips;
- generic universal overlay framework.

These remain roadmap work and are not placeholders to be partially activated.

## 18. Expected implementation surface

Expected production changes are bounded to:

- src/sketch/** for selection/direct-manipulation runtime semantics and Line replacement helpers if needed;
- src/application/** and/or src/part/** for semantic geometry update/batch transform command integration;
- src/viewer/include/simplesolid2/viewer/** for finite neutral grip/hover/preview transport;
- src/viewer_qt_occt/** for provider-native presentation and hit testing only;
- src/ui/** for the existing Sketch interaction coordinator and adapters;
- tests/**;
- docs/internal/SHARED_2D.md;
- docs/internal/CAD_WORKBENCH_VIEWER.md;
- docs/internal/PART_DOCUMENTS.md;
- docs/internal/BUILD_AND_TEST.md as needed;
- docs/product/pl/PARTS.md;
- docs/product/en/PARTS.md;
- work/**.

No persistence schema migration is expected.

If implementation requires durable grip state, provider-native identity in semantic contracts, a generic overlay framework, constraints/snapping, or a new universal reference system, stop and return for D2 review.

## 19. Automated verification

At minimum prove:

1. ordinary LMB adds an unselected Line rather than replacing prior selection;
2. ordinary LMB on an already selected Line preserves membership and updates primary;
3. Ctrl+LMB toggles membership;
4. blank LMB clears selection in ordinary Select;
5. Esc clears non-empty selection in ordinary Select;
6. ordinary Window/Crossing adds returned Lines;
7. Ctrl+Window/Crossing toggles returned Lines;
8. provider result ordering does not define primary;
9. selection survives activation/cancel/finish of Line creation;
10. newly created Line is not automatically selected;
11. every selected Line exposes Start/Center/End grips;
12. only one grip can be active;
13. hover does not mutate semantic selection;
14. grip hit wins over underlying geometry;
15. clicking Start/End begins owner-only Reshape;
16. clicking Center begins Move of the complete frozen selection snapshot;
17. selection membership cannot change during an active session;
18. pointer movement creates preview only;
19. preview creates no revision/dirty/history mutation;
20. LMB commit and Enter commit use the same semantic command path;
21. Esc cancels active manipulation and preserves selection;
22. a subsequent Esc in Select clears selection;
23. endpoint reshape preserves EntityId;
24. multi-Line Move preserves all existing EntityIds;
25. multi-Line Move is one atomic transaction and one Undo entry;
26. validation failure causes no partial update;
27. Undo/Redo preserves EntityIds and uses ordinary global history;
28. Undo/Redo requested during manipulation cancels transient preview first;
29. tool switch cancels uncommitted preview and preserves selection;
30. Save → Close → Reopen preserves edited coordinates and identity;
31. camera Pan/Orbit/Zoom/Navigation Cube remain functional outside active primary manipulation;
32. provider/native stress covers repeated hover/show/activate/preview/commit/cancel/clear and resize/DPI lifecycle;
33. exact-head Windows FULL CI passes;
34. documentation verification and generated Product Browser freshness pass.

## 20. Manual Windows verification

The exact final implementation candidate requires manual verification of:

- multiple selected Lines visibly show their grips;
- endpoint and center grips are distinguishable and reliably hit-testable;
- hover is visible but does not change selection;
- grip-over-geometry priority behaves consistently;
- endpoint Reshape affects only the owning Line;
- center-grip Move moves the whole selected set;
- preview follows the resolved Sketch-plane location;
- LMB and Enter land on the same geometry shown by preview;
- Esc cancels preview while retaining selection;
- second Esc clears selection;
- selection survives Line creation activation/finish/cancel;
- Undo/Redo visibly restores/reapplies the atomic edit;
- no stale native pixels or viewport disappearance occur during repeated use, navigation, resize and supported DPI scaling.

## 21. Documentation impact

Internal docs: required  
User/Product docs: required  
Reason: SK-05A changes the user-visible Select grammar, adds hover/grips/direct manipulation, changes Esc behavior in Select, and extends provider-neutral runtime interaction contracts.

## 22. Completion boundary

**Owner acceptance:** 2026-09-26

SK-05A is authorized for implementation under this bounded contract.

Completion requires:

- implementation remains inside this contract;
- semantic commands/transactions own durable edits;
- provider identity does not become CAD identity;
- exact-head Windows FULL passes;
- required internal and PL/EN product documentation is current;
- generated Product Browser is current;
- manual Windows verification passes on the exact final implementation candidate;
- closeout CLOSURE gate passes;
- R6+ remains inactive until separately contracted.
