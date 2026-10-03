# CAD Workbench and Viewer — As-built

<!-- doc-id: internal.cad-workbench-viewer -->
<!-- document-kind: internal -->

<!-- section-id: internal.cad-workbench-viewer.shell -->
## Shared Workbench shell

`CadWorkbenchShell` is the reusable one-document editor layout. Its active-Document top row contains the domain/context tool surface on the left and the existing Undo / Redo / Save / Close Document actions on the right; visual alignment does not merge their ownership. The body contains Document Tree, the Editor Surface, Properties plus contextual Operations, followed by Status/Diagnostics. Project-level Document Tabs live outside it in `ProjectWorkspaceShell`, so the Workbench shell represents only one active CAD Document editor.

The shell owns layout only. At normal desktop width it targets about 150 px for Document Tree and 270 px for Properties/Operations while the center Editor receives stretch surplus. Compact mode keeps all three surfaces with smaller side targets (currently 120/230 px). Narrow mode collapses the side surfaces and exposes local `Tree` and `Panel` recovery controls in the active-Document row; recovering one side keeps the Editor present and does not introduce docking/window ownership. Separate enter/leave thresholds provide resize hysteresis. If focus is inside a side-panel text/property editor when narrow mode is entered, that owning panel remains recovered so text focus is not discarded by reflow.

Responsive transitions are presentation-only: they do not mutate the Document, revision, dirty state, Undo/Redo, semantic selection or active Sketch/Profile context. The current Part composition is implemented by `CadWorkbench` plus narrow adapters.

`ProjectWorkspaceShell` owns one visible global Command Line surface plus the Qt keyboard/focus adapter. A provider-neutral `CadInputSession` owns only the runtime text buffer, active generic endpoint and submission transport. The active Workbench/tool owns semantic interpretation. `CadWorkbench` is currently the Part endpoint and adapts the Sketch interaction state; Project Workspace does not know Sketch commands or `PointRequest`.

<!-- section-id: internal.cad-workbench-viewer.active-document -->
## Active document context

The Workbench binds exactly one active `DocumentSession` at a time. Switching Document Tabs replaces the active Part/Tree/Properties/Viewer projection and invalidates stale CAD-input/edit context.

For Part, Document Tree projects the built-in Origin and source-Sketch/Profile hierarchy before the single Body with its ordered Feature rows, so the visual order reads from authored inputs to modeled result. Body/Feature rows carry semantic IDs and current derived evaluation status, but Tree item addresses are never CAD identity. Invalid Profiles and Failed/Blocked Features retain explicit status labels, receive bold visual emphasis and use a native warning icon; Suppressed remains explicitly labeled and italic. Sketch-authored revision changes refresh the derived Part/Feature evaluation snapshot immediately, so repairing a Profile removes the warning without requiring Save. These cues are projections of semantic/evaluation state, not new authored flags.

Properties can inspect Body status/Feature count and Feature name, FeatureId, evaluation status/diagnostic, operation, extent, distance/direction and source Profile/Sketch. Profile Properties expose consuming Features. Navigation between Feature and Profile changes runtime selection only.

Feature lifecycle actions from Properties and Tree share the same semantic handlers: Edit Extrude, Suppress/Unsuppress and Delete. They are disabled while an incompatible active Sketch/Extrude modeling context owns input, including the runtime-only command-first Extrude Profile-selection state.

Create Extrude supports both selection-first and command-first entry. Without an admissible preselection, Extrude owns an explicit CAD-input context that waits for exactly one valid Profile from Tree/viewport; Esc, toolbar toggle or Command Line CANCEL exits without mutation. The pick context has its own runtime generation so stale input cannot cross activations. Once a valid Profile is accepted, the ordinary revision-bound `ExtrudeDraft` starts and the product UI applies a 10 mm canonical default distance. The underlying application draft API remains capable of incomplete input and does not own that UI default.

<!-- section-id: internal.cad-workbench-viewer.origin -->
## Origin and reference scene

PartViewportController maps the Part's seven deterministic built-in Origin roles to provider-neutral `ReferencePresentation` objects and runtime presentation tokens.

The reference grid is a Viewer presentation primitive. It is not a Part semantic object and is not selectable.

Persistent Origin visibility comes from PartDocument authored presentation state. Hidden references still exist semantically and remain present in the Tree.

Scene replacement clears native detected/selected state before removing provider presentation objects. A failed native replacement is contained at the provider boundary and leaves provider presentation state coherent instead of propagating a process-level failure.

<!-- section-id: internal.cad-workbench-viewer.selection -->
## Selection authority and input grammar

PartViewportController owns document-scoped built-in-reference selection, while host-neutral `SketchInteractionState` owns active-Sketch semantic selection as `EntityId` values plus optional primary identity.

Qt/OCCT picking/query surfaces emit only runtime `PresentationToken` values. The controller maps Sketch tokens immediately to `SketchId + EntityId`; provider object identity and presentation tokens never enter authored CAD state.

Ordinary Sketch Select keeps the established Line/Circle/Arc grammar:

```text
Left click unselected entity → add it and make it primary
Left click selected entity   → keep membership; make it primary
Ctrl + Left click entity     → toggle EntityId membership
Left click blank             → clear Sketch selection
Ctrl + Left click blank      → keep Sketch selection
Left-to-right drag           → add Window hits
Right-to-left drag           → add Crossing hits
Ctrl + rectangle             → toggle returned EntityIds
Esc in Select                → clear selection
Middle drag                  → Pan
Shift + Middle drag          → Orbit
Mouse wheel                  → Zoom
```

Normal Move/Copy/Rotate/Scale/Mirror commands reuse that same semantic query/selection bridge. Selection-first activation freezes an existing non-empty selection immediately. Command-first activation starts in Select objects mode: click/Ctrl/Window/Crossing collect entities, blank LMB is a no-op, and Enter/Space/RMB completes collection only when non-empty. Grips are hidden/inactive during normal common-transform/COPY stages.

COPY retains the frozen original source selection across repeated placements. New copied entities do not take over selection, so later placements continue to refer to the original source set.

Rectangle results are canonicalized as semantic EntityIds and provider result order never chooses primary. Sketch point/rectangle queries remain independent from mutable OCCT selection.

Pointer routing and cursor mode remain independent runtime axes. Ordinary Select and transform/COPY object collection use the pick-box cursor; creation tools, transform reference/destination points, COPY placement and direct edit use the crosshair path. Toolbar, Operations, Command Line, keyboard and bounded RMB handling are adapters to the same semantic interaction authority.

<!-- section-id: internal.cad-workbench-viewer.viewer-boundary -->
## Viewer provider boundary

The public Viewer boundary remains provider-neutral. It accepts derived scenes for authored references/Sketch/Profile presentation plus bounded solid and transient solid-preview presentation required by PM-01.

Part evaluation may hold provider-neutral runtime solid/face tokens while the current runtime exists, but no TopoDS/OCAF handle, provider topology ordinal or Viewer presentation token crosses into durable Part identity.

PM-01 does not expose face/edge topology picking. Solid presentation is display-only; tessellation quality, camera, pick aperture and Viewer state cannot alter Add/Cut result, semantic face roles or commit authority.

Presentation setters report failure to the Workbench. A successful authored CAD command is not rolled back because a later Viewer refresh fails; recovery retries presentation from current authored state.

<!-- section-id: internal.cad-workbench-viewer.sr02-latency -->
## SR-02 Sketch interaction and presentation latency

SR-02 keeps the existing provider-neutral `IDocumentViewport` ownership and synchronous setter success/failure contract. No public differential-update protocol, render-completion callback or provider identity became CAD state.

The measured runtime optimizations are private implementation details:

- common-transform preview resolves one pointer event once and passes the complete `ResolvedSketchInput` through transform preview;
- rectangle-query semantic accumulation keeps deterministic vector output order while a private token-to-index lookup removes repeated linear token scans;
- Profile analysis and hover-result reuse are runtime-only and bind the owning Document/session, active Sketch, revision/state, draft RegionIntent, hovered region and Add/Subtract mode;
- exact no-op runtime scene replacement is suppressed only when the requested and installed semantic/runtime state prove that no visible change is required;
- Qt/OCCT aggregates one Circle/Arc point chain into one native wire object without changing the neutral point chain or semantic token;
- on proven paths, `UpdateCurrentViewer()` is the synchronous provider update and an immediately following duplicate `V3d_View::Redraw()` is omitted.

Finish Sketch still executes the established semantic teardown and final authoritative refresh. The optimization removes redundant already-empty scene submissions and duplicate flushes; it does not defer authored mutation, fuse history entries or create a second scene authority.

All SR-02 caches, counters and provider grouping are runtime-only. They are invalidated by the owning context they depend on and are never serialized. Provider rejection continues to report presentation degradation without rolling back an already accepted authored transaction.

Durable matched Windows evidence is in `work/SR-02_BASELINE_RESULTS.md` and `work/SR-02_FINAL_RESULTS.md`. On the measured runner, representative pointer preview moved from roughly 67–100 ms to roughly 16–19 ms, warm Profile hover from roughly 100 ms to roughly 17 ms, and 1,000-Line Finish Sketch with active preview from roughly 434 ms to roughly 67 ms. Large full-scene replacement and full 5,000-Line rectangle selection remain scale risks; SR-02 deliberately stops before a spatial index or public differential Viewer redesign because ordinary measured interaction no longer justifies that added architecture.

<!-- section-id: internal.cad-workbench-viewer.sketch-edit -->
## Part Sketch host and 3D edit context

The current Part Sketch editor presents and edits durable Line/Circle/Arc geometry and the Part-owned Profile workflow through the same semantic interaction and transaction path.

In Part modeling the editor toolbar provides `Sketch`. In Sketch edit the tool strip is visibly organized as:

```text
Select

Create
  Line
  Circle
  Arc
  Rectangle

Profile
  Profile

Modify
  Move
  Copy
  Rotate
  Scale
  Mirror

Inspect
  Measure
```

The top Create strip contains creation tools only. The right **Operations** panel exposes the runtime **Construction** Creation Role while Line/Circle/Arc/Rectangle is active, and additionally exposes **Draw Diagonals** while Rectangle is active. These creation options are context controls for the active tool; they remain semantically distinct from the selected-geometry Regular/Construction controls shown in Operations during ordinary Select. Both runtime options reset with Sketch-edit teardown, create no history by themselves and never become persistent preferences.

Rectangle is activated by the toolbar or the top-level `RECTANGLE` token and uses the same controller/interaction state. First Corner captures the current DocumentRevision; Opposite Corner commits four ordinary perimeter Lines, plus two ordinary Construction diagonals when enabled, through one atomic application command. Stale revision fails closed instead of silently rebasing the second click. No Rectangle/group/center identity enters the Viewer or authored model.

The workspace-global Command Line is context-sensitive. In an active Sketch with no semantic input request it can submit `SELECT`, `LINE`, `CIRCLE`, `ARC`, `RECTANGLE`, `MEASURE`, `MOVE`, `COPY`, `ROTATE`, `SCALE`, `MIRROR`, `PROFILE` and `EDITPROFILE`. An active Profile session additionally accepts `ADD`, `SUBTRACT`, `FINISH`, `CANCEL` and the documented option toggles. `ISLANDS ON|OFF` controls only the runtime **Show Islands** presentation; island analysis itself remains active. The former public `FIND`/Find All Regions workflow is no longer exposed. While a semantic PointRequest is active, that request receives the submitted text before top-level command activation. It may resolve the existing bare Direct Distance scalar; during active direct grip manipulation it may also consume the tool-local token `C` to enable Grip Copy. `C` is not a top-level command alias. Enter consumes one submitted token whether accepted or rejected; an invalid token creates no authored mutation, the active point/tool stage remains authoritative, the editable buffer becomes empty and a runtime diagnostic reports the rejection.

R10 keeps that ownership explicit in code. `application::CadInputSession` owns the single live CAD text buffer, endpoint attachment/lifetime, context-generation binding, diagnostics, application-session Polar/DYN settings and Dynamic Input field focus. It does not know Sketch tool semantics or parse tool-specific numbers.

`application::SketchCadInputSemanticEndpoint` owns the shared provider-neutral quantity/point grammar and semantic request precedence. Length quantities accept mm/cm/m/in/ft and normalize to canonical millimetres; bare Length uses the current Part unit, while bare Angle is degrees. Point grammar covers absolute `U;V`, relative Cartesian `@dU;dV` and relative polar `@Distance<Angle`, plus typed Circle/Arc/Rectangle and transform/grip value requests without per-tool parsers.

`PartSketchInteractionController` delegates resolved values to the existing interaction/command paths. `PointRequest` owns request-local numeric locks and pointer candidates. Dynamic Input is only an adapter over the same buffer/request: Tab/Shift+Tab navigate fields, valid token + Tab locks the current field, Enter commits from locks plus remaining pointer/Polar values, and Esc precedence is live buffer → locks → tool stage.

`CadWorkbench` remains the `ICadInputEndpoint` lifetime/presentation adapter. It validates context generation, supplies neutral decimal-separator and Part-unit context, delegates to the Application semantic endpoint and mirrors diagnostics/presentation. No QString/QLocale/Viewer type enters the shared parser.

For accepted top-level Sketch commands, the Qt adapter preserves the pre-D presentation behavior by returning keyboard focus to the viewport after semantic activation. Direct Distance submissions remain owned by the active PointRequest path and do not introduce an extra top-level focus transfer.

The Command Line is keyboard-first. With a normal CAD surface such as the viewport focused, printable unmodified text is appended to the same runtime buffer and mirrored immediately in Command Line without transferring Qt focus. Clicking Command Line remains an equivalent adapter to the same buffer. Real text editors, editable properties, modal dialogs and application shortcuts keep their own keyboard ownership.

Standard Windows document-history shortcuts are routed at the Workspace level only for the active CAD surface: `Ctrl+Z` invokes the existing active-Document Undo path and `Ctrl+Y` invokes the existing Redo path. They do not create a second Sketch history. The existing history boundary remains authoritative, so active transient Sketch manipulation is cancelled first where the normal Undo/Redo action already requires that. A focused real text editor keeps its own local Undo/Redo and inactive/hidden Documents cannot receive the shortcut.

Each live token is bound to an opaque `CadInputContextGeneration` supplied by the active semantic endpoint. The Sketch adapter advances that generation when the owning semantic context/request is replaced — for example tool/stage/base-request replacement, active-grip edit-mode replacement, authored revision change, entering/leaving a Sketch or switching the active endpoint. The live buffer and diagnostic are cleared when the observed generation changes. Pointer-direction movement inside the same PointRequest does not advance the semantic context and therefore does not clear a partially entered value. On submit, the generation that owned the token is passed back to the endpoint and is validated before token interpretation or any domain effect.

The QApplication-level event filter is global only as a transport hook; capture authority is Workspace-local. Printable CAD input is accepted only when the active top-level window is the owning Workspace, the focus and key target belong to its active Document Workbench, no popup/menu or modal surface owns the keyboard, the focus is not a real text editor, and an active CAD endpoint exists. A foreign non-modal window or another visible Workspace therefore cannot feed the background buffer.

Delete follows the same precedence rule as the rest of live CAD input. With normal CAD focus and a non-empty viewport-entered buffer, Delete is consumed by the input layer and does not reach semantic Delete Selection; the append-only viewport buffer is left unchanged and Backspace remains the character-removal key. With an empty buffer, the active semantic interaction context owns the destructive action. During active Sketch edit, selected Sketch entities are the only eligible Delete target; stale Tree/Profile selection or the currently displayed Profile Properties page cannot steal the command. If no qualifying Sketch selection exists, Delete fails closed instead of falling through to a stale Part/Profile target. Outside Sketch edit, valid Part/Profile selection keeps its existing delete behavior. With direct QLineEdit/text-editor focus, Delete remains ordinary local text editing.

A left click on blank Document Tree space clears only Tree/Part selection. It does not mutate authored state and does not globally clear independent active Sketch viewport selection.

The Command Line presentation is a geometry-stable single row. Its diagnostic region is permanently reserved and single-line; a long message is elided in place (with the full message available as tooltip) instead of wrapping, changing the input width or shrinking/moving the Viewer above it.

`PartSketchInteractionController` owns one runtime-only **last repeatable command** identity for the active Sketch edit session. Successful explicit activation of Line/Circle/Arc/Rectangle/Move/Copy/Rotate/Scale/Mirror through toolbar or Command Line updates that one value. Measure is intentionally not repeatable and does not replace that remembered identity. Select, Delete, selection changes, grip/direct manipulation, Undo/Redo and Esc do not replace it.

In ordinary Select with viewport CAD focus, Enter or Space repeats that remembered command through the same existing activation methods. Repeat starts a fresh command invocation: it uses the current semantic selection and does not replay prior Base/Reference/axis/placement points, prior selection snapshots, preview state or copied EntityIds. An empty remembered state is a no-op. The remembered identity is cleared by Sketch edit begin/end, so it does not leak across Sketches, Documents or reopen.

Existing key precedence remains authoritative and now sits below the global live-buffer rule. When the CAD input buffer is non-empty, Enter submits it, Backspace edits it and Esc clears it before any tool cancellation. When the buffer is empty, existing viewport semantics remain unchanged: active grip Space cycles semantic `DirectEditMode`, ordinary-Select Enter/Space may Repeat Last Command, transform Select objects Enter/Space completes collection, final-stage Enter commits, Delete edits selection and hierarchical Esc cancels the active interaction. Space in a real text editor remains text. Ordinary-Select RMB context remains outside the current surface.

Grip Copy is runtime mutation policy inside the existing direct-manipulation session, not a third `DirectEditMode`. `C` + Enter while a grip owns the active PointRequest enables it. In Move mode the complete frozen selection is duplicated; in Reshape mode only the active-grip owner is duplicated. The preview geometry is the same existing Reshape/Move preview; only commit policy changes from update-in-place to the existing `DuplicateSketchGeometryCommand`. Originals remain selected, copies receive fresh IDs, repeated placements keep the interaction-start source/pivot, mode change through Space turns Copy OFF, and repeated numeric placement requires a newly established pointer direction.

Move/Copy/Rotate/Scale/Mirror share one frozen-selection common-transform interaction pipeline. Selection-first activation skips object collection. Command-first activation collects objects with the ordinary semantic selection grammar and freezes the affected/source EntityIds before reference-point stages begin.

Move uses Base Point → destination and previews one translation `destination - base`.

COPY also uses Base Point → placement point, but commit duplicates rather than edits. Preview is computed from the original frozen mixed Line/Circle/Arc source. An accepted non-zero placement executes one `DuplicateSketchGeometryCommand`, leaves the originals unchanged and selected, creates fresh EntityIds for every duplicate, refreshes the authored scene and remains in COPY for another placement. Each later placement uses the same source snapshot and Base Point, not the previously created copy.

Exact zero displacement is intentionally not a COPY commit. It allocates no EntityIds, changes no revision/dirty/history state and leaves COPY active at the placement stage.

Each accepted repeated placement is a separate Part transaction and Undo entry. Undo/Redo is a boundary for an active transient COPY session: the transient preview is cancelled first, then ordinary global history runs. Redo restores the same copied identities. `DocumentSession` preserves the Sketch identity high-water through Undo so a new COPY in the same session cannot reuse the undone IDs. Undo back to the saved authored state remains clean; when a later committed copy is saved, its state persists the preserved high-water through the existing schema-v6 `next_entity_id` field.

Rotate uses Base Point → Reference Point → destination. The Reference Point must differ from Base. The preview uses the signed angle between the reference and destination vectors in the Sketch frame, with positive counter-clockwise rotation. A zero-angle completion is a clean no-op.

Scale uses Base Point → Reference Point → destination. The factor is the ratio of destination distance to reference distance about Base. Only finite factors `> 0` are valid; values between 0 and 1 reduce geometry, factor 1 is a clean no-op, and zero/negative scale does not commit.

Mirror uses first axis point → second axis point. The two points must be distinct and define an infinite mirror line. Reflection preserves EntityIds; reflected Arcs reverse signed sweep orientation so the authored directed arc matches the reflected geometry. Geometry that is exactly unchanged by the chosen axis completes as a no-op.

Pointer movement updates runtime-only preview from the interaction-start geometry snapshot. LMB at the final point stage or Enter commits the current valid transform/placement.

Precision point input is available wherever the active semantic request exposes it. Unbased requests accept exact `U;V`; based requests accept relative Cartesian/polar syntax and request-local Distance/Angle/dU/dV locks. Direct Distance remains a magnitude-only form that uses the current resolved pointer/Polar direction.

Circle creation is `Center → Size`, with Diameter as the per-Sketch-edit default; D/R switch runtime interpretation. Arc creation is chord-first `Start → End → Arc Point / Radius`; Radius requires at least half the chord, pointer side selects bulge, and explicit three-point Arc Point remains the major-arc path. Rectangle second stage owns `Width;Height`, with positive magnitudes and pointer quadrant supplying orientation.

Rotate accepts exact signed Angle, Scale exact positive Factor, and grip Rotate/Scale/Mirror accept Angle/Factor/Axis Angle through the same semantic endpoint. Explicit locks outrank Polar and raw pointer input; invalid/conflicting locks fail closed.

Polar is application-session runtime state (default ON, 45°, Absolute, no Additional Angles). It is a screen-space attraction magnet, not unconditional quantization or OSNAP. Relative requires an explicit semantic reference and never silently falls back to Absolute. F10 toggles Polar. Dynamic Input defaults OFF, uses the same live token as Command Line and is toggled by F12. Neither aid creates revision/dirty/Undo state or persists to the Part file.

Non-COPY transforms preserve existing EntityIds. COPY allocates fresh identities only at accepted placement commit; preview and cancelled/zero placements consume none. Save/Close/Reopen uses current Part schema v7 and persists the Part length unit, entity roles, `next_entity_id`, Profiles/RegionIntent and `next_profile_id` together with authored Sketch geometry.

Center grips cycle `Move → Rotate → Scale → Mirror → Move`. Supported non-center Line/Circle/Arc grips cycle `Reshape → Move → Rotate → Scale → Mirror → Reshape`. Every mode previews from frozen interaction-start geometry rather than chaining from the previous preview.

Grip Copy remains available only in Reshape and Move. Cycling into Rotate/Scale/Mirror turns Copy off. Rotate captures a fresh reference direction, Scale a fresh reference radius and Mirror uses the active grip as axis start. Typed Angle/Factor/Axis Angle lock the exact result and outrank later pointer motion.

LMB or Enter commits through the existing semantic command paths; Esc cancels transient manipulation while preserving accepted selection and previously committed copies. Cycling or numeric locks create no authored mutation by themselves.

Switching tools or Documents clears uncommitted transform/COPY/direct-manipulation state safely. Space inside text-entry focus remains text input.

<!-- section-id: internal.cad-workbench-viewer.measure -->
## R8 Measure — quick inspection and relational runtime targets

`CadWorkbench` exposes **Inspect → Measure** and the top-level Sketch command `MEASURE`. R8A quick inspection remains the default mode. R8B adds **Between** as a submode of the same `SketchTool::measure`, not as a new top-level tool.

### Quick whole-entity inspection

Quick Measure owns one runtime-only `EntityId` target and never rewrites ordinary Sketch selection. Selection-first activation uses exactly one selected Line/Circle/Arc; empty or multi-selection enters Measure without a target. LMB on an entity replaces only the Measure target, blank LMB clears it, and `measureEntity(model, EntityId)` derives the current value on demand. Provider presentation tokens are mapped back to `SketchId + EntityId` before semantic acceptance and never become measurement identity.

Operations presents Line Length/Delta U/Delta V/Angle +U, Circle Radius/Diameter/Circumference/Area and Arc Radius/Start/End Angle/Signed Sweep/Arc Length.

### Between runtime relation mode

While Measure is active, Operations exposes **Between** and the active semantic grammar accepts case-insensitive `BETWEEN`. The workspace/global CAD-input transport stays domain-neutral; outside active Measure, `BETWEEN` remains unknown. Entering Between clears only the quick Measure target/result and preserves ordinary Sketch selection.

R8B uses runtime-only measurement references:

```text
MeasurePointRef { EntityId owner, MeasurePointRole role }
MeasureLineRef  { EntityId }
```

They are deliberately separate from edit-grip identity, are not authored Points, are not OSNAP candidates and are never persisted. Supported point roles are Line Start/Midpoint/End, Circle Center and four ±U/±V Quadrants, and Arc Center/Start/End/Midpoint along the authored signed sweep.

A whole Circle or Arc is not a relational target. A Line body may be selected directly as an infinite supporting-line target. Supported relations are exactly:

- Point ↔ Point — Distance, Delta U, Delta V and directed Angle +U from Target A to Target B;
- Point ↔ Line — perpendicular distance to the infinite supporting Line plus exact perpendicular foot;
- Line ↔ Line — smaller undirected angle in `0..pi/2`.

No Product geometric tolerance, nearest-point healing or endpoint clamping is introduced.

### Latent markers and Viewer identity boundary

`measurePointCatalog(model)` derives semantic runtime points. `PartViewportController` maps `EntityId` to `PresentationToken` and projects exact positions. Viewer marker keys contain only presentation token + runtime marker role; no `EntityId` crosses the Viewer boundary.

Markers are latent. Qt/OCCT uses a bounded screen-space aperture only to reveal them. **Proximity reveals; it never acquires.** Hidden markers cannot be clicked semantically and pointer proximity never snaps or changes CAD coordinates.

Visible-marker query has priority over entity-body query. Accepted marker tokens are immediately mapped back to `SketchId + EntityId + MeasurePointRole` and revalidated. Provider order has no semantic priority. Same-coordinate overlapping hits are measurement-equivalent; distinct-coordinate overlapping hits fail closed instead of nearest-wins or cycling. If no visible marker is hit, only a Line body can become a relational whole-entity target.

Accepted point targets stay pinned; accepted Lines stay highlighted. Construction geometry participates with the same roles and formulas as Regular geometry.

### State, lifecycle and representative cue

The runtime flow is `await_first → await_second(first) → result(first, second)`. Interaction state stores target references, not a cached relational result; the current result is recomputed from the current model. After a result, the next accepted target starts the next relation. Blank click clears pending/result state but remains in Between.

Esc is hierarchical: **Between → ordinary Measure → Select**. Tool switch, history boundary and Sketch/Document replacement clear R8B runtime targets, markers and cues. No R8B action changes DocumentRevision, dirty state, identity allocation, Undo or persistence.

The semantic layer also derives one provider-neutral `RelationalMeasurementCue`; Qt/OCCT only maps and draws it:

- Point ↔ Point — one transient segment between exact targets;
- Point ↔ Line — P→F perpendicular segment, plus a distinct dashed supporting-line continuation when F lies outside the finite authored segment;
- Line ↔ Line — both Lines highlighted, without angular-dimension arc/text.

Cue objects are runtime-only, non-selectable, non-editable, non-snappable and identity-free. This single-current-relation visualization is not the R8C general dimension-overlay system.

Angles are formatted in degrees for readability while neutral geometry remains radians. Linear values are converted from canonical millimetres into the current Part unit and area values use the squared current Part unit. Unit changes affect presentation only; Measure remains read-only.

<!-- section-id: internal.cad-workbench-viewer.navigation -->
## Navigation and provider-surface Navigation Cube

The shared viewport supports middle-button Pan, Shift + middle-button Orbit, wheel Zoom, Fit All, Orthographic/Perspective projection and a provider-surface 3D Navigation Cube.

The visible Cube is rendered and hit-tested inside the Qt/OCCT native graphics surface. The former QWidget ViewCube is no longer a competing product authority. The common Viewer boundary carries only provider-neutral navigation actions; Qt/OCCT owns Cube pixels, labels, DPI anchoring, hit testing and animation presentation.

The Cube follows the current camera orientation relative to the model/world axes. Its six labeled faces, 12 edges and 8 corners select canonical orientations. Face/edge/corner transitions are animated. When a canonical face is active, adjacent-view arrows perform exact 90-degree view changes and CW/CCW controls perform exact 90-degree roll. Home selects Top-Front-Right isometric orientation and Fit All.

Camera orientation and projection are independent. Cube orientation actions and Home preserve the current projection mode. A separate provider-surface `ORTHO/PERSP` control next to the Cube changes only the projection; it does not change camera direction, target, up vector or Fit state. The Cube itself remains the same orientation manipulator in both projection modes.

Navigation changes are runtime-only and do not increment DocumentRevision, set needsSave or create CAD Undo entries.

<!-- section-id: internal.cad-workbench-viewer.provider-presentation -->
## Current OCCT presentation

The concrete Qt/OCCT provider presents the current derived Body solid and a separately replaceable transient Extrude preview alongside Origin, Sketch and Profile scenes.

The committed solid scene is rebuilt from the current accepted Part evaluation. Extrude draft evaluation still computes the complete candidate Body and that candidate remains the authority for whether Finish is legal. Separately, the modeling provider derives an exact runtime-only operation delta from the same neutral Extrude input and the ordered Body stage immediately before the target Feature: Add preview is `tool - upstream Body`, while Cut preview is `tool ∩ upstream Body`. The accepted Body therefore remains visible in its normal opaque/default presentation and only material that would actually be added or removed is overlaid translucently. The delta crosses the public boundary only as provider-neutral presentation mesh and carries no durable CAD identity. In the Qt/OCCT provider the committed Body and transient preview each own an independent `Prs3d_ShadingAspect` before `Display()`; Body color/opacity and Add/Cut preview color/transparency are never assigned through shared context-linked shading state. Replacing or clearing preview therefore cannot recolor the committed Body. Polygon offset remains scoped to the preview object only. A valid solid preview transiently hides its source Profile to avoid coplanar z-fighting; if preview becomes invalid during Edit, the Profile may be revealed for diagnosis. Finish, Cancel and context exit clear both runtime overrides without changing the authored Profile visibility policy.

Typing into the Extrude distance field mutates the transient draft immediately but debounces the expensive candidate evaluation/tessellation by 90 ms. A pending preview is flushed before Enter/Finish so commit authority still uses an exact-current successful evaluation. Command-line and discrete option changes keep immediate preview evaluation.

For Cut, provider evaluation performs an explicit fuzzy=0 volumetric common check between the upstream Body and the Extrude tool before the Boolean Cut. Face-, edge- or point-only contact has zero removable volume and returns `no_effect`; only a positive-volume common may proceed. The older unchanged-result check remains as a secondary invariant. This is modeling validation, not presentation policy.

Solid tessellation quality is provider-private presentation policy. PM-01H4b restored the original bounded OCCT display meshing values (0.25 mm linear / 0.35 rad angular) and fixed smooth shading at the representation boundary instead of increasing triangle density. The provider derives nodal normals from each BRep face, transports them as provider-neutral per-vertex presentation normals, and the Qt/OCCT Viewer renders the neutral mesh through `AIS_Triangulation`. Smooth continuity therefore follows one underlying BRep surface such as a cylinder while genuine boundaries between separate BRep faces remain sharp. Nodal normals, tessellation and shading remain presentation data only and cannot change modeling tolerances, B-Rep meaning or semantic references.

PM-01 solid/preview presentation carries no durable semantic topology identity. Face/edge subshape picking is deliberately absent. Existing provider selection/detection cleanup rules still apply before presentation replacement/removal.

<!-- section-id: internal.cad-workbench-viewer.presentation-recovery -->
## Presentation failure and recovery

Authored CAD state remains authoritative when provider presentation fails. A semantic command commits through the existing validation/transaction/history path first; rebuilding reference/Sketch presentation is a later runtime operation and cannot roll that successful command back.

`PartViewportController` records a runtime-only degraded-presentation state when the full authored/reference refresh cannot be applied by the bool-returning Viewer boundary. `CadWorkbench` surfaces that state in Status/Diagnostics with the bounded message `3D presentation update failed. Authored CAD state remains authoritative; the next full refresh will retry.`

The next normal full refresh reconstructs reference and authored Sketch presentation from the current `DocumentSession`. Only a complete successful refresh clears the degraded state and diagnostic. Failure and recovery do not change DocumentRevision, dirty state, Undo/Redo depth or durable identity. Transient preview failure likewise creates no authored mutation.

E2 Release evidence retained the current full authored-scene replacement strategy. The measured runner showed an approximately 33 ms native presentation floor through roughly 1,000 line objects and approximately 83 ms around 4,000–5,000 native presentation objects; a 5,000-line accepted mutation plus full authored refresh measured about 150 ms median. These values are engineering evidence, not Product latency guarantees. Large/highly segmented Sketches remain a documented scale risk; no differential-update or public `IDocumentViewport` expansion was introduced.

Circle/Arc sampled point chains, native line segments and Profile face tessellation remain presentation-only derived data. They never define closure, intersections, loops, regions or Profile identity. Shared 2D region analysis operates on accepted authored/evaluated 2D geometry outside the Viewer; Part alone owns durable ProfileId/RegionIntent semantics.

<!-- section-id: internal.cad-workbench-viewer.runtime -->
## Runtime lifetime and stress coverage

Selection, primary selection, hover, active grip, DirectEditMode, interaction-start geometry, common-transform/COPY stages and snapshots, Base/Reference/axis points, active PointRequest, pointer candidate, Polar capture, request-local numeric locks, Dynamic Input field focus/overlay, current preview, runtime Measure target, repeatable command, camera/projection, transient detection/grid, presentation tokens, grip/query/selection overlays, active `SketchInteractionState` and workspace CAD input buffer/prompt/diagnostic state are runtime-only. Polar/DYN configuration is retained only by the application-session `CadInputSession` and is discarded on application restart.

Persistent Origin visibility, authored Sketch geometry and entity roles, committed copied entities, model-local identity high-water, Part Profiles/RegionIntent and ProfileId high-water are authored/durable state. COPY and Profile hover/draft preview are never persisted.

Closing/reopening the application recreates runtime view/interaction state while preserving saved authored geometry, copied EntityIds, the persisted next-identity cursor and visibility in the Part file.

The native Workbench/Qt-OCCT stress regression continues to exercise repeated selection, navigation, resizing and lifecycle operations. COPY does not introduce a provider-owned duplication state or second semantic selection authority.

