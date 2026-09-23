# Project Hub and Part Workspace UI — As-built

<!-- doc-id: internal.ui-project-hub -->
<!-- document-kind: internal -->

<!-- section-id: internal.ui-project-hub.pages -->
## Hub and Workspace Shell

The Qt application uses a `QStackedWidget` with two pages:

- Project Hub;
- Workspace Shell.

The Workspace Shell presents the active Project name, ProjectId and Workspace path plus a `PartWorkspacePanel`.

The panel is intentionally separate from Project Hub responsibilities. It owns presentation for the current Part-document lifecycle but not authored semantics.

<!-- section-id: internal.ui-project-hub.primary-actions -->
## Primary Project actions

The Hub exposes `Create Project…` and `Open Project…`.

Create opens the Project Creation dialog. Open selects an already initialized SimpleSolid Project Workspace.

<!-- section-id: internal.ui-project-hub.recent-list -->
## Recent Projects list

Recent entries show DisplayName and remembered Workspace path.

Problematic entries also show `Workspace not found`, `Invalid Project` or `Project mismatch`.

The warning state is derived at refresh time and is not persisted.

<!-- section-id: internal.ui-project-hub.selection -->
## Recent selection-dependent actions

Recent actions are `Open`, `Locate…` and `Remove from Recent`.

No Recent entry is selected after Hub refresh. Open is enabled only for an actually selected, openable entry. Locate and Remove remain available for selected unavailable entries.

<!-- section-id: internal.ui-project-hub.part-panel -->
## Part Workspace panel

The Part panel exposes:

- `New Part…`;
- `Refresh`;
- `Open`;
- Number, Title, Description and Engineering Revision editors;
- `Apply Properties`;
- `Undo`;
- `Redo`;
- `Save`;
- `Close Part`.

New Part asks for a Workspace-relative `.ss2part` path and proposes `Part001.ss2part`, `Part002.ss2part`, and so on when those names are free.

Resolved entries show title and relative path. Invalid files and identity conflicts are shown with warning state. Conflict entries display every conflicting relative path and cannot be opened.

The UI obtains authored state through DocumentSession and sends property edits through the semantic command boundary. It does not mutate PartDocument directly.

<!-- section-id: internal.ui-project-hub.dirty-close -->
## Dirty-close UX

Closing a dirty Part requires `Save`, `Discard` or `Cancel`.

Closing the Project or application when any Part is dirty requires `Save All`, `Discard` or `Cancel`.

A failed Save leaves the Part/Project open so authored in-memory changes are not silently lost.

<!-- section-id: internal.ui-project-hub.recovery -->
## Project recovery UX

`Locate…` remains the recovery path for a moved Project.

`Remove from Recent` removes only application history. Project and Part files are not modified.

PART-01 does not provide automatic repair for duplicate Part DocumentIds.

<!-- section-id: internal.ui-project-hub.boundary -->
## UI boundary

The Qt layer converts strings/paths, displays diagnostics and maps application state to widgets.

It does not own ProjectId, DocumentId, authored Part state, persistence schemas, Undo/Redo semantics or identity-conflict resolution rules.
