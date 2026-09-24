# Project Hub and CAD Workbench UI — As-built

<!-- doc-id: internal.ui-project-hub -->
<!-- document-kind: internal -->

<!-- section-id: internal.ui-project-hub.pages -->
## Hub and Workspace

The Qt application uses a `QStackedWidget` with Project Hub and Workspace pages.

The Workspace page contains a persistent Project Workspace Shell. It shows Project name, ProjectId and Workspace path and owns the Project-level navigation surfaces:

```text
TOP       Project toolbar
CONTENT   Workspace Dashboard or active Document Workbench
BOTTOM    Project-level Document Tabs
```

The Project toolbar remains visible in both Workspace and Document contexts. Workspace Dashboard is a first-class Project surface, not only an empty-state placeholder; it remains reachable while Documents are open.

Project Workspace navigation is runtime-only: `Workspace` or `Document(DocumentId)`. Open DocumentSessions may remain alive while Workspace Dashboard is visible.

`CadWorkbenchShell` owns only fixed presentation regions for the active Part Document:

```text
LEFT      Document Tree
CENTER    tool-launch strip + Editor Surface / 3D Viewport
RIGHT     Properties + contextual Operations
BELOW     Status / Diagnostics
```

Document Tabs are deliberately outside `CadWorkbenchShell`. Future Assembly/Drawing editors can therefore use the same Project-level navigation without making Part Workbench their owner.

<!-- section-id: internal.ui-project-hub.primary-actions -->
## Primary Project actions

The Hub exposes `Create Project…` and `Open Project…`.

Create opens the Project Creation dialog. Open selects an already initialized SimpleSolid Project Workspace.

<!-- section-id: internal.ui-project-hub.recent-list -->
## Recent Projects list

Recent entries show DisplayName and remembered Workspace path.

Problematic entries also show `Workspace not found`, `Invalid Project` or `Project mismatch`. The warning state is derived at refresh time and is not persisted.

<!-- section-id: internal.ui-project-hub.selection -->
## Recent selection-dependent actions

Recent actions are `Open`, `Locate…` and `Remove from Recent`.

No Recent entry is selected after Hub refresh. Open is enabled only for a selected, openable entry. Locate and Remove remain available for selected unavailable entries.

<!-- section-id: internal.ui-project-hub.part-panel -->
## Workspace launch and Part document controls

The persistent Project toolbar exposes `Workspace`, `New Part…`, neutral `Open…`, `Refresh` and `Close Project`. These actions remain available while a Part is active and are not Part editor tools.

`Workspace` switches only the runtime navigation context to Project Dashboard. It does not close, save or mutate open Documents.

Once a Part is active, its Workbench exposes `Undo`, `Redo`, `Save`, `Close`, common Document Properties, Document Tree, the editor tool-launch strip and contextual Operations. Project-level Document Tabs remain owned by Project Workspace Shell.

`New Part…` opens `WorkspaceLocationDialog`. It shows the current Workspace folder hierarchy, allows the user to choose a folder, enter the filename and explicitly `Create Folder`.

The location picker is constrained to the current Workspace. Private `.simplesolid` metadata is not exposed as a normal target. Existing targets and attempts to escape the Workspace fail closed.

`Open…` opens `OpenDocumentDialog`. Candidates show Document kind, display/name information, location and status. The four columns are interactively resizable; the default layout keeps Name compact and gives Location the largest share of the dialog. Current discovery supplies Part entries only. Invalid files and identity conflicts remain visible but cannot be opened as resolved DocumentIds.

Opening an already open Document focuses its canonical existing DocumentSession/tab instead of creating a second mutable session. Several Documents can remain open simultaneously. Selecting a Project-level tab navigates by DocumentId, not by tab index. Closing the last open Document returns to Workspace Dashboard while the Project remains open.

<!-- section-id: internal.ui-project-hub.workbench -->
## Tree, Properties and Viewport

The active Part Tree contains the Part root, built-in Origin group with seven semantic references and any hosted Sketch records.

Existing Sketches can enter their runtime edit context through Tree double-click or the `Edit Sketch` context action. The Tree stores transient SketchId data for routing only; Tree row identity is not durable model identity.

Tree uses extended multi-selection. Show/Hide context actions are available only when the complete selection supports the operation. Mixed semantic/non-reference selections fail closed.

Tree and Viewport are adapters of one document-scoped selected set plus primary selection.

Properties shows common Document fields when no semantic object is primary. When a built-in Origin reference is primary, Properties switches to a read-only reference context showing role/type/identity/visibility.

The central Editor Surface hosts the provider-neutral Document Viewport. A tool-launch strip immediately above it contains the current Part `Sketch` launcher. Production composition injects the Windows Qt/OCCT provider.

Operations is contextual to the active tool/edit mode: it is idle when no tool is active, shows Sketch support-pick controls while creating a Sketch, and shows `Finish Sketch` while a Sketch is being edited.

The ViewCube is a responsive overlay. At normal width it presents the regular CAD navigation surface; at narrow Editor widths it switches to a compact control instead of forcing the Editor Surface wider or overlapping the Properties panel.

<!-- section-id: internal.ui-project-hub.dirty-close -->
## Dirty-close UX

Closing a dirty Part requires `Save`, `Discard` or `Cancel`.

Closing the Project or application when any Part is dirty requires `Save All`, `Discard` or `Cancel`.

A failed Save leaves the Part/Project open so authored in-memory changes are not silently lost.

Before a DocumentSession or the owning ProjectSession is destroyed, Project Workspace Shell detaches Workbench runtime bindings. This ordering is required because Tree/Viewer controllers use non-owning session pointers.

<!-- section-id: internal.ui-project-hub.recovery -->
## Project recovery UX

`Locate…` remains the recovery path for a moved Project.

`Remove from Recent` removes only application history. Project and Part files are not modified.

Automatic repair for duplicate Part DocumentIds is not implemented.

<!-- section-id: internal.ui-project-hub.boundary -->
## UI boundary

The Qt layer converts strings/paths, displays diagnostics and maps application/domain state to widgets.

It does not own ProjectId, DocumentId, authored Part state, persistence schemas, Undo/Redo semantics or identity-conflict resolution rules.

The Viewer UI depends only on provider-neutral Viewer contracts. OCCT types and provider-native selection objects do not cross into `src/ui`.

Workspace document dialogs may validate filesystem locations through application services, but they do not own Document identity or Part authored semantics.
