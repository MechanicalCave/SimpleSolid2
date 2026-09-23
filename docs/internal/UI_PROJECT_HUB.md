# Project Hub and CAD Workbench UI — As-built

<!-- doc-id: internal.ui-project-hub -->
<!-- document-kind: internal -->

<!-- section-id: internal.ui-project-hub.pages -->
## Hub and Workspace

The Qt application uses a `QStackedWidget` with Project Hub and Workspace pages.

The Workspace shows the active Project name, ProjectId and Workspace path plus one shared `CadWorkbench`.

`CadWorkbenchShell` owns only fixed presentation regions:

```text
LEFT      Document Tree
CENTER    Editor Surface / 3D Viewport
RIGHT     Properties + Operations
BOTTOM    Document Tabs
BELOW     Status / Diagnostics
```

Domain/session adapters provide active content. The neutral Shell does not know PartDocument, AssemblyDocument or DrawingDocument.

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
## Part controls inside the Workbench

The current Part composition exposes `New Part…`, `Open Part…`, `Refresh`, `Undo`, `Redo`, `Save`, `Close`, common Document Properties, Document Tree, Operations placeholder and bottom Document Tabs.

`Open Part…` lists discovered native Parts. Invalid files and identity conflicts remain visible but cannot be opened as resolved DocumentIds.

Opening an already open Document focuses its canonical existing DocumentSession instead of creating a second mutable session.

<!-- section-id: internal.ui-project-hub.workbench -->
## Tree, Properties and Viewport

The active Part Tree contains the Part root and built-in Origin group with seven semantic references.

Tree uses extended multi-selection. Show/Hide context actions are available only when the complete selection supports the operation. Mixed semantic/non-reference selections fail closed.

Tree and Viewport are adapters of one document-scoped selected set plus primary selection.

Properties shows common Document fields when no semantic object is primary. When a built-in Origin reference is primary, Properties switches to a read-only reference context showing role/type/identity/visibility.

The central Editor Surface hosts the provider-neutral Document Viewport. Production composition injects the Windows Qt/OCCT provider. A ViewCube overlay provides faces, corner/isometric orientations, Fit and Orthographic/Perspective switching.

<!-- section-id: internal.ui-project-hub.dirty-close -->
## Dirty-close UX

Closing a dirty Part requires `Save`, `Discard` or `Cancel`.

Closing the Project or application when any Part is dirty requires `Save All`, `Discard` or `Cancel`.

A failed Save leaves the Part/Project open so authored in-memory changes are not silently lost.

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
