# Application Lifecycle — As-built

<!-- doc-id: internal.application-lifecycle -->
<!-- document-kind: internal -->

<!-- section-id: internal.application-lifecycle.startup -->
## Application startup

`SimpleSolid2` starts a Qt `QApplication`, configures organization/application names and resolves the Recent Projects catalog using Qt `QStandardPaths::AppLocalDataLocation`.

The application opens into `ProjectHubWindow`.

The Hub is a navigation surface. It is not the durable owner of Project or Document identity.

<!-- section-id: internal.application-lifecycle.create -->
## Create Project lifecycle

The current Create lifecycle is:

```text
Create Project…
→ ProjectCreationDialog
→ validate Project name / parent Location / child folder
→ create staged Workspace
→ initialize Project metadata
→ publish final Workspace
→ open ProjectSession
→ record Recent Project
→ show neutral Project Workspace
→ activate a Document Workbench only after a CAD Document is created/opened
```

A successful Create leaves one active ProjectSession and one logical Recent entry for the created ProjectId.

<!-- section-id: internal.application-lifecycle.open -->
## Open existing Project

`Open Project…` selects an existing directory.

The application does not initialize an ordinary folder during Open. `ProjectSession::open` requires valid existing SimpleSolid Project metadata.

Opening a Project also rebuilds the runtime native-document index by scanning the Workspace for `.ss2part` files. Invalid Part files and duplicate DocumentIds become explicit index states instead of being silently ignored.

<!-- section-id: internal.application-lifecycle.part -->
## Part Document lifecycle

The Project Workspace initially owns document launch. The current Part lifecycle is:

```text
neutral Project Workspace
→ New Part…
→ WorkspaceLocationDialog
→ browse folders inside current Workspace
→ optionally Create Folder
→ enter Part filename
→ validate Workspace-contained target
→ generate fresh DocumentId
→ atomically publish valid empty PartDocument
→ create canonical DocumentSession
→ activate Part Workbench

neutral Project Workspace
→ Open…
→ OpenDocumentDialog
→ list discovered Document candidates with Kind / Name / Location / Status
→ choose one resolved DocumentId
→ load authored Part state
→ create or reuse canonical DocumentSession
→ activate the Workbench for the resolved DocumentKind
→ add/focus bottom Document Tab
```

The current discovery backend provides Part candidates only. The dialog is document-oriented so later document kinds do not require a Part-specific top-level Open action.

The Workspace location flow rejects targets outside the current Workspace, rejects existing target files, and keeps private `.simplesolid` metadata outside the normal Document location tree.

The active DocumentSession owns runtime Undo/Redo history and the save checkpoint. Editing Number, Title, Description, Engineering Revision, persistent Origin visibility or creating a Part-hosted Sketch goes through semantic commands and PartDocument transactions.

SK-01A places the Part `Sketch` launcher in the editor toolbar above the 3D Viewport. Creation validates a selected XY/XZ/YZ built-in Origin plane, creates one durable empty Sketch and enters a runtime edit context in the same 3D Viewport. Operations is contextual: support-pick may show Cancel guidance and active Sketch edit shows `Finish Sketch`. An existing Sketch can re-enter the same edit context by Tree double-click or `Edit Sketch` context action without authored mutation.

Closing and reopening a Part destroys runtime Undo/Redo and Sketch edit context while preserving saved authored state, including SketchId/support/placement/visibility, and DocumentId on disk.

<!-- section-id: internal.application-lifecycle.workbench -->
## Active document and Workbench lifecycle

One ProjectSession may keep several DocumentSessions open. With zero open Documents the Project remains in a neutral Workspace context; no inactive Part editor is implied.

Creating/opening a Part activates the Part Workbench. The architecture routes future editor activation by DocumentKind rather than treating Project Workspace as a Part Workbench. `CadWorkbench` owns one active DocumentId for the current Part editor context.

Changing the bottom tab switches Document Tree, Properties context, Viewer scene, runtime selection and runtime camera state. The other DocumentSessions remain open. Closing the last open Document returns to the neutral Workspace without closing the Project.

Camera and selection are kept independently per open DocumentId for the lifetime of the Workbench. They are not persisted across application restart.

<!-- section-id: internal.application-lifecycle.close -->
## Close, dirty state and restart

Closing a dirty Part prompts for `Save`, `Discard` or `Cancel`.

Workbench Tree/Viewer controllers hold non-owning runtime pointers to the active DocumentSession. SK-01A makes lifecycle ordering explicit: those bindings are detached before ProjectSession erases a DocumentSession. The same detach-before-destroy rule applies before the ProjectSession itself is destroyed.

Closing the Project or application with any dirty Part prompts for `Save All`, `Discard` or `Cancel`. Save failure keeps the Project open.

After application restart, Recent Projects restores the Project location. Opening the Project creates a fresh ProjectSession, rebuilds document discovery and can reopen the same durable DocumentId with the same saved authored properties, Origin visibility and hosted Sketch records.

Runtime camera, selection, active Sketch edit context and tab state start fresh.

<!-- section-id: internal.application-lifecycle.relocate -->
## Project move and relocation

Moving a Project in the filesystem makes the remembered Recent path stale.

Hub derives the stale entry as `WorkspaceMissing`. It does not delete the entry automatically.

`Locate…` asks the user for a replacement Workspace and validates that the replacement metadata contains the expected ProjectId. Only then is the remembered location updated.

A different ProjectId is rejected.

<!-- section-id: internal.application-lifecycle.availability -->
## Recent availability refresh

At Hub refresh, the internal controller derives one presentation state for every Recent entry:

- `Available` — remembered Workspace is a valid Project with the expected ProjectId;
- `WorkspaceMissing` — remembered path does not exist;
- `ProjectInvalid` — path exists but is not a valid readable SimpleSolid Project Workspace;
- `IdentityMismatch` — path contains a valid Project with a different ProjectId.

Availability is runtime/presentation state only. It is not written back to the Recent catalog.

<!-- section-id: internal.application-lifecycle.smoke -->
## Application smoke and stress paths

The executable supports `--smoke-test`.

Smoke mode uses a temporary Recent catalog, shows the Hub offscreen in CI, exits through a zero-delay Qt timer and removes its temporary state. The Viewer remains lazy because no Document is opened.

`wb01.viewer_native_smoke` creates the real Qt/OCCT viewport, initializes OCCT/OpenGL, presents reference content, changes view/projection, performs Fit and closes cleanly.

`wb01a.workbench_native_stress` runs a real Workbench with the real Qt/OCCT provider and repeatedly exercises Viewport input, Origin Show/Hide, Undo/Redo, navigation and narrow/wide Workbench resizing for 100 iterations.
