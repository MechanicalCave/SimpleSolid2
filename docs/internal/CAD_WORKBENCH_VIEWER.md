# CAD Workbench and Viewer — As-built

<!-- doc-id: internal.cad-workbench-viewer -->
<!-- document-kind: internal -->

<!-- section-id: internal.cad-workbench-viewer.shell -->
## Shared Workbench shell

`CadWorkbenchShell` is the reusable one-document editor layout. Its active-Document top row contains the domain/context tool surface on the left and the existing Undo / Redo / Save / Close Document actions on the right; visual alignment does not merge their ownership. The body contains Document Tree, the Editor Surface, Properties plus contextual Operations, followed by Status/Diagnostics. Project-level Document Tabs live outside it in `ProjectWorkspaceShell`, so the Workbench shell represents only one active CAD Document editor.

The shell owns layout only. At normal desktop width it targets about 150 px for Document Tree and 270 px for Properties/Operations while the center Editor receives stretch surplus. Compact mode keeps all three surfaces with smaller side targets (currently 120/230 px). Narrow mode collapses the side surfaces and exposes local `Tree` and `Panel` recovery controls in the active-Document row; recovering one side keeps the Editor present and does not introduce docking/window ownership. Separate enter/leave thresholds provide resize hysteresis. If focus is inside a side-panel text/property editor when narrow mode is entered, that owning panel remains recovered so text focus is not discarded by reflow. The right contextual panel ignores its natural size hint in both axes, so showing Profile/Sketch Operations cannot promote content height into an unexpected top-level window resize.

Responsive transitions are presentation-only: they do not mutate the Document, revision, dirty state, Undo/Redo, semantic selection or active Sketch/Profile context. The current Part composition is implemented by `CadWorkbench` plus narrow adapters.

`ProjectWorkspaceShell` owns one visible global Command Line surface plus the Qt keyboard/focus adapter. A provider-neutral `CadInputSession` owns only the runtime text buffer, active generic endpoint and submission transport. The active Workbench/tool owns semantic interpretation. `CadWorkbench` is currently the Part endpoint and adapts the Sketch interaction state; Project Workspace does not know Sketch commands or `PointRequest`.

<!-- section-id: internal.cad-workbench-viewer.active-document -->
## Active document context

The Workbench binds exactly one active `DocumentSession` at a time. Switching Document Tabs replaces the active Part/Tree/Properties/Viewer projection and invalidates stale CAD-input/edit context.

For Part, Document Tree projects the built-in Origin, Reference Geometry and source-Sketch/Profile/Axis hierarchy before the single Body with its ordered Feature rows, so the visual order reads from authored inputs to modeled result. Authored Axis rows live under their source Sketch; Origin X/Y/Z remain only in Origin. Body/Feature rows carry semantic IDs and current derived evaluation status, but Tree item addresses are never CAD identity. Invalid Profiles and Failed/Blocked Features retain explicit status labels, receive bold visual emphasis and use a native warning icon; Suppressed remains explicitly labeled and italic. Sketch-authored revision changes refresh the derived Part/Feature evaluation snapshot immediately, so repairing a Profile removes the warning without requiring Save. These cues are projections of semantic/evaluation state, not new authored flags.

Properties can inspect Body status/Feature count and Feature name, FeatureId, evaluation status/diagnostic, operation, extent, distance-or-angle/direction and source Profile/Sketch; Revolve also exposes its AxisReference. Axis Properties expose AxisId, source Sketch/Line, authored visibility and current derived status/origin/direction. Profile Properties expose consuming Features. Navigation between related semantic objects changes runtime selection only.

Feature lifecycle actions from Properties and Tree share the same semantic handlers: Edit Extrude/Edit Revolve, Suppress/Unsuppress and Delete. Axis designation Create/Delete, explicit Edit/Re-source and Show/Hide likewise route through application/DocumentSession semantics rather than Tree mutation. Conflicting modeling actions are disabled while an active Sketch/Axis/Datum/Extrude/Revolve context owns input.

Create Extrude supports both selection-first and command-first entry. Without an admissible preselection, Extrude owns an explicit CAD-input context that waits for exactly one valid Profile from Tree/viewport; Esc, toolbar toggle or Command Line CANCEL exits without mutation. The pick context has its own runtime generation so stale input cannot cross activations. Once a valid Profile is accepted, the ordinary revision-bound `ExtrudeDraft` starts and the product UI applies a 10 mm canonical default distance. The underlying application draft API remains capable of incomplete input and does not own that UI default.

Create Revolve uses the shared application `RevolveDraft` for selection-first, command-first and Edit. It requires one valid Profile and one explicit AxisReference; Profile and Axis acquisition may occur in either order and no default Axis is inferred. Origin X/Y/Z or a currently resolved authored Axis can seed the draft. Operations and Command Line mutate the same Add/Cut, OneSide/Midplane, Angle and Reverse state; the UI default is Add + OneSide + 360 degrees + Reverse off. Edit preserves FeatureId. Draft generation is part of CAD-input freshness so stale Dynamic Input/Finish cannot cross a later mutation or activation.

Normal GUI Axis creation is contextual to Sketch Line authoring/selection; there is no standalone GUI Axis authoring button. While Line is active, Operations exposes `Geometry role: Regular | Construction` and an independent `Part reference: Axis` checkbox. Axis defaults OFF and is one-shot: a successful Line+Axis operation commits both objects atomically in one semantic transaction / one Undo entry and then clears the checkbox; rejected or zero-length Line input leaves the designation armed and authors nothing.

For exactly one selected Line, the same Operations area projects its current Axis designation. OFF→ON creates one fresh Part Axis without changing the Line EntityId or geometry role. ON→OFF deletes exactly that Axis while preserving the Line; if a Revolve references it, the GUI requires explicit confirmation and the downstream authored AxisId intent becomes Missing/Blocked rather than being rebound. A legacy Line referenced by multiple AxisIds is shown indeterminate/disabled and must be repaired explicitly through Axis Tree/Properties.

The `AXIS` Command Line path remains an expert adapter over `AxisDraft`; Edit Axis also keeps that draft and preserves AxisId during re-source. New Create/Re-source rejects a source already designated by a different authored Axis. Cancel/invalid/stale input performs no authored mutation.

<!-- section-id: internal.cad-workbench-viewer.origin -->
## Origin, Datum reference geometry and reference scene

`PartViewportController` maps the Part's seven deterministic built-in Origin roles, every currently visible/resolved authored Axis and every currently visible/resolved Datum Plane into one provider-neutral `ReferenceScene`.

Origin identity comes from the built-in role. Authored Axis identity comes from durable `AxisId`; Datum identity comes from durable `DatumId`; the Viewer never owns either identity. For each scene rebuild the controller allocates disposable presentation tokens and keeps runtime token↔AxisId/DatumId bindings only for current picking/selection. A refresh may allocate different tokens while semantic Axis/Datum selection remains stable.

The reference grid is a Viewer presentation primitive. It is not a Part semantic object and is not selectable.

A resolved Datum Plane is currently presented with a finite plane footprint and visible border. The finite extent is presentation policy only; it is not an authored plane size. The accepted neutral translucent fill is not rendered in the current build; the Owner explicitly deferred that presentation-only treatment to PM-06 so Origin and Datum planes can receive one coherent fill style. When the plane intersects the current committed Body, the controller may derive presentation-only line segments from the current Body scene. Those segments form an owner-bound overlay: they have no independent token, Edge/Curve identity or Projection meaning, and picking them resolves to the owning Datum Plane token.

Persistent Origin, authored Axis and Datum visibility come from PartDocument authored presentation state. Hidden references still exist semantically and remain present in the Tree. During an active Revolve draft, the selected source Axis may receive a runtime-only reveal/emphasis even when authored visibility is Hidden; clearing/finishing the draft restores normal presentation without mutating visibility. `Reference Geometry` sits directly below Origin and bulk Show/Hide applies the existing per-Datum authored visibility values; the group has no second persisted visibility flag.

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

The public Viewer boundary remains provider-neutral. It accepts derived scenes for authored Origin/Axis/Datum references, Sketch/Profile presentation, one atomic topology-aware committed Body scene, a separate transient solid-preview scene and a transient Datum-plane draft preview.

Part evaluation may hold provider-neutral runtime solid/Face/Edge/Vertex tokens while the current runtime exists. Datum presentation likewise uses generation-scoped neutral presentation tokens. No TopoDS/OCAF handle, provider topology ordinal, Viewer presentation token or Datum intersection segment crosses into durable Part identity. `PartViewportController` owns current runtime mappings and invalidates/rebuilds them whenever the corresponding scene/evaluation generation changes.

Current Face/Edge/Vertex picking is presentation acquisition only. Datum patch/border and intersection-overlay picking is also acquisition only: the intersection has no independent semantic target and resolves to its owner DatumId. Tessellation quality, camera, pick aperture, candidate order and Viewer state cannot alter Add/Cut results, Axis/Revolve source meaning, Datum source meaning or durable identity.

The single Datum Plane tool uses one application-owned `DatumPlaneDraft` for Operations and Command Line. Source acquisition from Origin, planar Body Face or existing Datum Plane writes semantic source intent into that same draft. The default 10 mm Offset, signed Offset edits, Reverse, Finish and Cancel therefore share one validation/commit path. Preview is derived; Finish revalidates the current document/evaluation before the semantic command may commit.

When that draft has a current committable evaluation, Workbench publishes a transient `ReferencePlanePreviewPresentation` from the derived frame. The preview carries no `PresentationToken` and no `DatumId`; its patch/border and optional current-Body intersection segments are presentation-only and are not selectable reference geometry. Invalid input, unresolved source, Cancel, Finish or runtime/context reset clears the preview immediately. The first Datum in an otherwise Empty Part is therefore previewable without creating any authored object before Finish.

Presentation setters report failure to the Workbench. A successful authored CAD command is not rolled back because a later Viewer refresh fails; recovery retries presentation from current authored state. Conversely, stale presentation or stale draft/evaluation state is never accepted as mutation authority.

<!-- section-id: internal.cad-workbench-viewer.accepted-next-pm02-visual-topology -->
## As-built PM-02 viewport topology, View Style and Feature contribution

This section describes the current PM-02 implementation. Presentation state remains independent from authored CAD state, while current Body topology is available for direct inspection and bounded semantic acquisition through the generation-scoped Body scene.

### View Style HUD

The viewport navigation/presentation HUD groups one single-select View Style control with the existing provider-surface navigation controls:

```text
HOME    ORTHO/PERSP    Shaded + Edges ▾
```

The available styles are:

- Shaded;
- Shaded + Edges;
- Shaded + Hidden Edges.

View Style is presentation state only. It must not increment DocumentRevision, set Part dirty state, create CAD Undo/Redo history or serialize into the Part document. A later application/workspace preference may remember it outside CAD authored state.

Hidden-edge rendering is visual only. Showing occluded edges must not silently enable select-through of occluded topology. A future explicit Select Through/X-Ray mode would be a separate product decision.

### Independent overlay layers

Presentation order is conceptually:

```text
Base View Style
-> persistent Feature Contribution
-> temporary tree hover
-> direct Face/Edge/Vertex hover/selection
-> semantic support/target overlay
-> active command preview
-> failure/ambiguity diagnostics
```

Exact drawing order may be provider-private, but the semantic roles must remain independent. Changing View Style cannot change semantic selection, reference resolution or command targets.

Direct viewport topology interaction and Document Tree Feature interaction deliberately use different presentation roles so the user can distinguish "I selected geometry" from "I am inspecting design history".

### Direct topology and semantic carriers

Direct interaction targets bounded current topology:

- Face;
- Edge;
- Vertex.

Part immediately maps the fresh runtime topology token to the stage-scoped semantic catalog. Surface / Curve / Point carrier visualization appears only when required by the active command or inspection context.

Examples:

- planar Face selected for Sketch -> bounded Face highlight plus semantic Surface/support-plane overlay and U/V frame;
- Edge inspection -> material Edge highlight plus optional semantic Curve cue;
- Vertex inspection -> Vertex marker plus semantic Point information.

The carrier cue is never Viewer-owned CAD identity.

### Committed Body presentation boundary

The current implementation uses one atomic topology-aware committed Body presentation snapshot instead of independent mesh/topology authorities.

Its meaning is:

```text
one current RuntimeSolid/evaluation-provider generation
    -> shaded triangle payload
    -> current Face presentation records
    -> current Edge presentation records
    -> current Vertex presentation records
    -> one BodyScene generation
```

The Viewer must not reconstruct CAD topology from triangle adjacency.

Provider presentation extraction may preserve:

- per-Face triangle ranges keyed by current RuntimeFaceToken;
- per-Edge display polylines keyed by current RuntimeEdgeToken;
- per-Vertex display points keyed by current RuntimeVertexToken.

Those provider runtime tokens stop at the Part/controller boundary. `IDocumentViewport` receives only neutral `PresentationToken` values plus display geometry/flags.

`PartViewportController` owns current generation-scoped mappings:

```text
RuntimeFaceToken   <-> PresentationToken
RuntimeEdgeToken   <-> PresentationToken
RuntimeVertexToken <-> PresentationToken
```

The committed Body scene must be installed atomically. Publicly independent committed mesh/Face/Edge/Vertex setters must not become separate scene authorities whose generations can diverge.

The existing command-owned `SolidPreviewScene` remains separate. Preview geometry never supplies committed Body reference identity.

Each installed committed Body presentation carries a runtime-only generation. Direct Body query results carry that generation. Scene/evaluation/provider replacement invalidates old hover/candidate stacks and old presentation-token authority, even if a numeric token value is later reused.

The same current Edge presentation records should back:

- visible-edge drawing;
- hidden-edge drawing;
- direct Edge hit testing;
- direct Edge selection;
- Feature Contribution/boundary overlay where applicable.

This keeps View Style as a visual projection of one current topology presentation rather than a second topology interpretation.

Tree Feature Contribution remains an overlay over the same current Body PresentationTokens. It is not `PresentationSelection` and does not create duplicate selectable geometry.

Semantic support-plane / Curve-extension / Point cues are separate derived overlays. For PM-02 they are not duplicate Body topology targets.

If evaluation fails and exposes a `resolved_prefix_solid`, a prefix Body may be shown diagnostically, but it must be marked non-authoritative for successful final-Body command targeting.

The neutral Viewer API contains bounded concepts equivalent to:

```text
ViewStyle
BodyScene
BodyPresentationGeneration
BodyTopologyPresentationKind
BodyTopologyPickFilter
BodyTopologyPickCandidate
BodyTopologyPickQueryResult
BodyTopologyOverlay
```

Exact type names are implementation detail. The public Viewer boundary must not expose Part semantic references, FeatureId ownership, or OCCT TopoDS handles.

The accepted detailed technical design is `work/PM-02_BODY_PRESENTATION_VIEWER_API_DESIGN.md`.

### Direct topology acquisition

Ordinary Body topology acquisition targets current bounded Face / Edge / Vertex presentation and then immediately maps the fresh PresentationToken back to the current semantic topology catalog.

The ordinary all-kind hit priority is:

```text
Vertex
  ↓
Edge
  ↓
Face
```

This is only a screen-space acquisition rule.

Vertex and Edge use bounded DPI-aware logical-pixel apertures; Face uses the front-visible projected Face region. Cursor proximity may determine what visible item the user pointed at, but it never defines durable identity or automatic semantic repair.

Normal PM-02 selection is front-visible:

- occluded topology is not an ordinary candidate;
- Shaded + Hidden Edges does not enable select-through;
- dashed hidden-edge presentation remains non-selectable;
- a later explicit Select Through/X-Ray mode would require a separate product decision.

The active semantic context may narrow the Viewer kind filter without embedding Part rules in the Viewer. In particular, Create Sketch requests Face-only acquisition. A cylindrical/non-planar Face remains acquireable and is then rejected by Part semantic admission as Unsupported; the picker must not silently fall through to a neighboring Edge or another Face.

When several visible candidates remain under the same pointer sample, the interaction maintains a runtime-only candidate stack. In ordinary 3D topology targeting with viewport CAD focus:

- Tab -> next candidate;
- Shift+Tab -> previous candidate.

Topology cycling is lower priority than focused text input, Dynamic Input field traversal or another semantic request that already owns Tab. Pointer neighborhood, view, tool/filter, scene or evaluation/provider generation changes reset the stack.

The currently preselected candidate is visibly highlighted before click. The user's click on that visible preselection is the acquisition authority; provider return order has no durable semantic meaning. Boundary Vertex/Edge front-visibility includes only a bounded sub-pixel world-space allowance for projection/ray round-trip error; it does not enlarge semantic identity tolerance or permit hidden select-through.

A `KnownRepresentationArtifact` such as a periodic seam or an ADR-0017 same-Surface representation partition remains completely accounted and diagnosable but is excluded from ordinary engineering Edge display and preselection by default. A same-Surface partition is recognized from tracked carrier/Boolean lineage, not because two planes merely look coplanar. Other material topology whose durable referenceability is Unsupported remains visible/selectable for truthful inspection and diagnostics.

Feature Contribution, hidden-edge graphics and semantic carrier/support ghost overlays do not create duplicate pick geometry. All normal Body picking uses the one current topology presentation/token mapping.

The detailed accepted design input is `work/PM-02_DIRECT_TOPOLOGY_SELECTION_UX.md`.

### Topology Properties / inspection boundary

This subsection describes the current as-built topology inspection behavior.

The existing stacked Properties panel remains the single product inspection surface. It uses one adaptive topology inspection context rather than adding Face/Edge/Vertex nodes to Document Tree.

The panel should display one primary inspection subject at a time.

Explicit committed selection changes Properties; hover/preselection does not. Tab/Shift+Tab candidate cycling remains viewport/status feedback until the user clicks the visible candidate.

The key distinction is:

```text
current topology is Present
!=
durable semantic reference is Resolved
```

A current material Edge may be directly selectable while a durable singular semantic reference is Ambiguous or Unsupported. Properties must report that truth rather than hide the Edge or silently claim stable identity.

For a selected Face, the Properties context exposes:

- Face as current topology kind;
- Surface carrier;
- Surface classification such as Plane/Cylinder/Cone/Sphere/Torus/Other;
- producer Feature and Body stage;
- semantic role/provenance;
- accounting/referenceability state;
- standard Sketch-support capability;
- optional derived geometry diagnostics such as area;
- optional advanced canonical support-frame details for a planar carrier.

For a selected Edge:

- Edge / Curve;
- material versus representation-artifact classification;
- Curve classification;
- producer/provenance where defensible;
- adjacent Surface summary;
- current stage;
- accounting/referenceability state;
- geometry diagnostics such as length/radius.

A selected Edge can therefore legitimately show:

```text
Current topology: Present
Referenceability: Ambiguous
Candidates: 2
```

For a selected Vertex:

- Vertex / Point;
- producer/provenance;
- adjacent semantic Curve/Surface summary;
- current stage;
- accounting/referenceability;
- current XYZ as geometry diagnostics only.

Provider/runtime tokens, PresentationToken, TopoDS/OCAF identity and provider traversal indices are not product Properties.

Body Properties should gain current complete topology/accounting counts. Feature Properties should gain Current Feature Contribution counts from the same Part semantic query used by tree highlighting. Sketch Properties should show authored support intent together with current Resolved/Missing/Ambiguous/Unsupported support state; derived world frame is not an independent authored property.

If a directly selected topology item disappears after recompute, the stale viewport selection clears rather than geometry-rebinding. A durable Sketch still remains selectable as its authored owner while its support may report Missing.

If a diagnostic `resolved_prefix_solid` is displayed after a failure, its topology Properties must be visibly marked diagnostic/non-authoritative and must not enable normal mutating topology-reference actions.

Document Tree remains design history. PM-02 does not add a default Face/Edge/Vertex catalog subtree.

The normative detailed design is `work/PM-02_TOPOLOGY_PROPERTIES_INSPECTION_UX.md`, accepted with the active PM-02 Work Contract on 2026-10-04.

### Current Feature Contribution

Ordinary Feature hover/selection in Document Tree targets **Current Feature Contribution**, not historical Body replacement.

For Feature `F`, Current Feature Contribution is a set-valued query against the currently authoritative displayed Body stage:

> current semantic topology that still carries design meaning produced directly by F.

For Faces, every current Face realization whose semantic Surface carrier producer is `F` participates. If later Booleans split one prior Face realization into several current fragments while the same producer Surface survives, all fragments are highlighted. This set-valued display is not a singular FaceReference lookup and is not Ambiguous merely because multiple fragments exist.

Deleted semantic output is not ghosted as current contribution.

For Edges/Vertices, direct semantic provenance determines direct Feature contribution. Boundary Edges/Vertices around highlighted Faces may additionally be drawn as a presentation envelope without being reclassified as Feature-owned semantic topology.

A shared Edge/Vertex involving carriers from several Features is not assigned an arbitrary owner merely for coloring.

### Cut Feature contribution

For a Cut Feature, direct contribution is the current semantic topology introduced by that Cut, for example:

- Cut-exposed tool Surfaces;
- Boolean-created material intersection Edges with direct Cut provenance;
- directly generated semantic Points/Vertices where defensible.

An upstream Surface merely trimmed by the Cut retains its original semantic producer and therefore remains part of the upstream Feature's contribution.

### Tree hover versus tree selection

Feature tree hover:

- shows temporary Current Feature Contribution;
- does not change primary selection or Properties authority;
- disappears on hover exit;
- creates no authored mutation.

Feature tree selection:

- makes the Feature the Tree/Properties selection;
- shows persistent Current Feature Contribution;
- keeps the current Body in the current base View Style;
- does not automatically switch the Body to the historical stage after that Feature.

An active modeling command may suppress lower-priority tree hover so target/preview cues remain unambiguous.

### Three distinct history presentations

The following are intentionally different concepts:

```text
Current Feature Contribution
!= Operation Scope / Delta
!= Historical Stage Preview
```

Current Feature Contribution is the default Tree hover/selection behavior.

Operation Scope / Delta means the material volume added/removed by the Feature at its own upstream stage. It may later use a translucent Add/Cut ghost overlay and remains optional unless separately promoted into the production acceptance gate.

Historical Stage Preview means displaying the entire Body at `BodyStageRef::AfterFeature(F)`. If later productized, it requires an explicit action; ordinary Feature selection does not rewind the Body.

The accepted visual/tree design input is `work/PM-02_VIEW_STYLE_TREE_FEATURE_CONTRIBUTION_UX.md`; direct selection acquisition is specified separately in `work/PM-02_DIRECT_TOPOLOGY_SELECTION_UX.md`.

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
  Project Geometry

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

<!-- section-id: internal.cad-workbench-viewer.project-geometry -->
## Linked Project Geometry: Workbench tool and Viewer current-source routing

`CadWorkbench` owns the sole transient `project_edge_active_` state when an editable Part Sketch, current Body-stage topology and exact OCCT projection provider exist. The **Project Geometry** Sketch Modify launcher and the right Operations panel (stage/count, Regular/Construction, Remove/Clear, Finish/Cancel) share this state with the existing global CAD-input `PROJECT`, `REGULAR`, `CONSTRUCTION`, `REMOVE`, `CLEAR`, `FINISH` and `CANCEL` semantic endpoint. Only `CreateProjectedSketchEdgesCommand` mutates Part state on Finish. It rechecks `DocumentRevision`, current Sketch support, source semantic stage and live provider references and commits an entire batch or none. `BreakProjectedEdgeLinkCommand` is used by the linked-only context action and obtains latest resolved geometry rather than copying authored seeds.

The Sketch Select action now uses atomic `BreakProjectedEdgeLinksCommand` for one or many currently resolved linked targets; all linked inputs must be current, and a failure leaves every link in place. The singular command delegates to that transaction. `PartSketchInteractionController::finishProfile` passes the exact modeling provider into the revision-aware Create/Edit Profile commands; the Document Tree uses the same Workbench read-only current Profile resolution as Properties and Viewer. No authored linked seed is a source of live geometry. OCCT Line projection consumes topological source-vertex points rather than independently computed analytic end samples, preserving genuinely shared endpoints without tolerance snapping.

During Edge collection the Workbench temporarily changes Sketch Select's `PrimaryPointerRouting` from `spatial_tool_input` to `presentation_selection`, reusing the existing strict material Body Edge pick transport. Cleanup restores the normal Sketch routing. The staged `MaterialEdgeReference` list, role and `project_edge_preview_valid_` are runtime-only. `PartViewportController::setProjectedEdgeDraftPreview` resolves `Line2`, `Circle2` and `Arc2` against the same source revision and Sketch Frame used by the Part command. Analytic circles/arcs are segmented **only for disposable Viewer preview**, not persisted as approximated geometry. Failed provider/stage/revision/current-curve projection clears the whole preview and blocks Finish. Clear/Cancel/Esc/context switch drop picks and overlays without authoring.

Tool-local Esc is hierarchical: with staged Edges the first unbuffered Esc clears them; a subsequent Esc exits to Select. Explicit `CANCEL` immediately exits. The Workspace `CadInputSession` retains typed-buffer priority: the first focused Command Line Esc with text clears only the text, while an empty-buffer Esc dispatches the tool-local `ESC` via the same semantic endpoint. Viewport Enter, `FINISH` and the right Finish button call the same guarded commit. The persistent one-row Command Line reserves fixed prompt/diagnostic widths, with full prompt and long diagnostics available as tooltips; it never shrinks the typing field on tool changes.

The visible Sketch Scene uses a **disposable effective model** derived from PG-01B current ordered Part evaluation, not saved linked Line/Circle/Arc seeds. A successful scene caches the same `SketchModel` with `DocumentId + SketchId + DocumentRevision`; `currentSketchInteractionModel()` is the read-only source for OSNAP, nearest/intersection/tangent/extension and Measure. It returns no data on stale, switched or degraded linked presentation. Unlinked Sketches keep their ordinary model. Missing/unresolved projection strips linked entities from the disposable snapshot, while keeping unrelated unlinked geometry selectable and measurable. Viewer `SketchLinePresentation::linked` / `SketchCurvePresentation::linked` flags affect base color only; Regular/Construction stroke style and priority of selection, measurement highlight and hover remain independent. A selected linked entity shows source Feature and current/unresolved status; Break Link is enabled only for a current resolved source.

This boundary does not authorize Part schema changes, persistent topology tokens, cross-Part/Assembly geometry, Face boundary capture, or a second input router. Strict structural-edit Commands reject linked targets/boundaries that cannot be evaluated safely in their current authored-only implementation; wider interaction/transform correctness remains an explicit PG-01C acceptance obligation.

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

<!-- section-id: internal.cad-workbench-viewer.edge-features -->
## Fillet / Chamfer interaction and stage-scoped Edge acquisition

Normal Part mode groups solid actions as Create: Sketch / Datum Plane / Extrude / Revolve and Modify: Fillet / Chamfer.

Fillet/Chamfer use the same Body topology presentation and semantic acquisition authority as ordinary inspection; the Viewer never owns authored Edge identity. Selection-first and command-first entry are both supported. During an active edge-feature draft the topology tool mode restricts acquisition to admissible material Edges from the draft's exact required Body stage. Click toggles membership in the explicit set, duplicate authored inputs are impossible, and Operations exposes selected count plus Clear/Remove behavior. Periodic seams, same-Surface representation partitions, Unsupported/Ambiguous/non-referenceable candidates and stale evaluation generations are rejected rather than coerced.

Operations owns one common Radius for Fillet or one common Distance for Chamfer. Parameter and Edge-set changes mutate only the transient draft and refresh exact candidate evaluation. The **complete candidate Body** produced by applying the full explicit Edge set in one provider feature operation determines Finish legality. The **visible transient preview** is instead its exact *local material delta* against the immediate predecessor Body: removed volume in orange and added volume in blue, with the unchanged Body neutral. A preview may have only one color. Neither delta supplies authored reference authority, and an unsupported/failed material-difference computation does not permit whole-Body blue fallback. There is no sequential per-Edge modeling path or best-effort partial Feature.

The explicit Edge set remains exact authored intent, but the preview/result is allowed to show necessary **local result-topology accommodation** around selected endpoints and corners: neighboring Edges/Faces may be trimmed, split or repatched and local transition boundaries may appear. This is not automatic tangent-chain authoring. The current OCCT provider still requires its input contour to equal the explicit authored set, preventing an unselected tangent neighbor from silently receiving the Radius/Distance.

Finish is enabled only for an exact-current committable evaluation; stale DocumentId/revision/draft generation loses publication authority. Cancel clears the transient preview and produces no authored mutation.

FILLET and CHAMFER Command Line commands are adapters over the same draft. Their contextual input changes the same parameter/selection state and uses the same Finish/Cancel/rejection path as Operations.

Editing an existing Fillet/Chamfer restores its complete semantic Edge set, parameter and FeatureId. The Workbench asks PartViewportController to present the Feature's exact upstream Body stage, not the current final Body. That stage scene is reconstructed from the same-revision FeatureEvaluation result solid/topology; its runtime tokens remain transient. Existing resolved authored inputs can be reselected in the stage scene, while Missing/Ambiguous/Unsupported intent remains visible in the draft/diagnostic until explicitly removed or replaced. No nearest/similar final-Body Edge is substituted.

Structured edge-feature diagnostics keep reference failure separate from provider/modeling failure. UI status identifies the failing 1-based input where applicable and reports Missing, Ambiguous or Unsupported distinctly. A resolved Edge set followed by impossible Radius/Distance or provider failure is reported as Failed rather than as reference failure.

Feature Suppress/Unsuppress/Delete remain the generic ordered Part Feature lifecycle. Edit/repair Cancel restores normal Feature selection without changing authored state. Undo/Redo rebuilds presentation from restored semantic state and does not restore stale Viewer/provider tokens.
<!-- section-id: internal.cad-workbench-viewer.provider-presentation -->
## Current OCCT presentation

The Qt/OCCT reference provider renders a Datum Plane through the existing reference-presentation channel rather than a second CAD model. The finite footprint/border uses the current neutral Datum frame, while every virtual plane/Body intersection segment is registered with the same owner presentation token as the Datum presentation. Native picking of either object therefore returns the Datum Plane owner and can never create an Edge/Curve reference from the overlay. Datum footprint size, future fill/transparency, border and overlay styling remain provider presentation policy.

The active Datum draft preview is deliberately different from committed Datum presentation. The provider renders its finite footprint/border and optional intersection segments as deactivated, non-pickable transient objects with no presentation token. Replacing or clearing that preview cannot change committed Datum token→DatumId mappings or Body selection state. Once Finish commits, the draft preview is removed and ordinary committed Datum presentation is rebuilt from authored state.

The concrete Qt/OCCT provider presents the current derived topology-aware Body scene and separately replaceable transient Extrude/Revolve operation previews and Fillet/Chamfer exact two-tone local material-delta previews alongside Origin/Axis/Datum, Sketch and Profile scenes. If final Part evaluation is Unavailable because a higher active Feature fails or blocks, evaluation may also expose a runtime-only `resolved_prefix_solid`: the same-revision solid result immediately before the first failing active Feature. `PartViewportController` may present that prefix diagnostically so lower UpToDate history remains visible. This does not change final Body truth: final semantic reference authority remains unavailable, downstream active Features remain Blocked and mutating topology-reference actions cannot consume the prefix. If the first active Feature fails, no prefix exists and the Viewer solid scene is empty.

The committed solid scene is rebuilt from the current accepted Part evaluation. Extrude, Revolve, Fillet and Chamfer draft evaluation compute the complete candidate Body and that candidate remains the authority for whether Finish is legal. Separately, the modeling provider derives an exact runtime-only operation delta from the same neutral operation input and the ordered Body stage immediately before the target Feature: Add preview is `tool - upstream Body`, while Cut preview is `tool ∩ upstream Body`. Revolve preview uses the same resolved `AngularRevolveInput` semantics as committed modeling; the public preview API is presentation-only and cannot define modeling success. The accepted Body therefore remains visible in its normal opaque/default presentation and only material that would actually be added or removed is overlaid translucently. The delta crosses the public boundary only as provider-neutral presentation mesh and carries no durable CAD identity. In the Qt/OCCT provider the committed Body and transient preview each own an independent `Prs3d_ShadingAspect` before `Display()`; Body color/opacity and Add/Cut preview color/transparency are never assigned through shared context-linked shading state. Replacing or clearing preview therefore cannot recolor the committed Body. Polygon offset remains scoped to the preview object only. A valid solid preview transiently hides its source Profile to avoid coplanar z-fighting; if preview becomes invalid during Edit, the Profile may be revealed for diagnosis. Revolve additionally emphasizes/reveals its source Axis as a runtime cue. Finish, Cancel and context exit clear these runtime overrides without changing authored Profile or Axis visibility policy.

For PM-05F R2-D, OCCT computes the two zero-fuzzy exact B-Rep differences between the evaluated upstream Body `B0` and its accepted target-stage candidate `B1`. The Viewer receives one atomic `SolidPreviewScene` with optional orange removed material (`B0 - B1`) and optional blue added material (`B1 - B0`), guarded by the exact current Body-scene generation; no separate overlay setter can publish half a new preview against a stale stage. The extra preview AIS object owns an independent shading aspect from the orange preview and neutral committed Body. Empty one-sided deltas are normal. Invalid/stale candidate, Cancel, Document switch and a new Body generation clear the transient. Tessellation only visualizes the exact B-Rep differences; it never determines modeling legality or semantic Edge identity.

Typing into the Extrude distance or Revolve angle field mutates the transient draft immediately but debounces the expensive candidate evaluation/tessellation by 90 ms. A pending preview is flushed before Enter/Finish so commit authority still uses an exact-current successful evaluation. Command-line and discrete option changes keep immediate preview evaluation.

For Cut, provider evaluation performs an explicit fuzzy=0 volumetric common check between the upstream Body and the Extrude tool before the Boolean Cut. Face-, edge- or point-only contact has zero removable volume and returns `no_effect`; only a positive-volume common may proceed. The older unchanged-result check remains as a secondary invariant. This is modeling validation, not presentation policy.

Solid tessellation quality is provider-private presentation policy. PM-01H4b restored the original bounded OCCT display meshing values (0.25 mm linear / 0.35 rad angular) and fixed smooth shading at the representation boundary instead of increasing triangle density. The provider derives nodal normals from each BRep face, transports them as provider-neutral per-vertex presentation normals, and the Qt/OCCT Viewer renders the neutral mesh through `AIS_Triangulation`. Smooth continuity therefore follows one underlying BRep surface such as a cylinder while genuine boundaries between separate BRep faces remain sharp. Nodal normals, tessellation and shading remain presentation data only and cannot change modeling tolerances, B-Rep meaning or semantic references.

Committed Body Face/Edge/Vertex presentation carries only runtime acquisition identity. Durable meaning lives in the Part semantic topology catalog; Viewer tokens and provider subshape identity remain transient. Existing provider selection/detection cleanup rules apply before presentation replacement/removal, and stale Body-scene generations cannot commit.

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

Persistent Origin visibility, authored Sketch geometry and entity roles, committed copied entities, model-local identity high-water, Part Profiles/RegionIntent and ProfileId high-water, authored AxisId/source/visibility and ordered Extrude/Revolve Feature definitions are authored/durable state. COPY, Axis/Revolve draft state, transient Axis emphasis and Profile hover/draft preview are never persisted.

Closing/reopening the application recreates runtime view/interaction state while preserving saved authored geometry, copied EntityIds, the persisted next-identity cursor and visibility in the Part file.

The native Workbench/Qt-OCCT stress regression continues to exercise repeated selection, navigation, resizing and lifecycle operations. COPY does not introduce a provider-owned duplication state or second semantic selection authority.

