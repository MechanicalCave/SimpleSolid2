# Project Hub UI — As-built

<!-- doc-id: internal.ui-project-hub -->
<!-- document-kind: internal -->

<!-- section-id: internal.ui-project-hub.pages -->
## Hub and Workspace Shell

The current Qt application uses a `QStackedWidget` with two pages:

- Project Hub;
- empty Workspace Shell.

The Workspace Shell currently presents the active Project name, ProjectId and Workspace path. It intentionally contains no CAD document/modeling UI yet.

<!-- section-id: internal.ui-project-hub.primary-actions -->
## Primary Project actions

The Hub exposes:

- `Create Project…`;
- `Open Project…`.

Create opens one modal Project Creation dialog with:

- Project name;
- Location;
- Project folder;
- Final path preview.

Location is the existing parent directory. Project folder is initially proposed from Project name and becomes independent once the user edits it.

Open selects an already initialized SimpleSolid Project Workspace.

<!-- section-id: internal.ui-project-hub.recent-list -->
## Recent Projects list

Recent entries show DisplayName and remembered Workspace path.

Problematic entries also show a visible warning/status:

- `Workspace not found`;
- `Invalid Project`;
- `Project mismatch`.

The warning state is derived at refresh time and is not persisted.

<!-- section-id: internal.ui-project-hub.selection -->
## Selection-dependent actions

Recent actions are:

- `Open`;
- `Locate…`;
- `Remove from Recent`.

No Recent entry is selected after Hub refresh.

The actions depend on a real Qt selection, not merely `currentItem()`.

For a selected available entry:

```text
Open                enabled
Locate…             enabled
Remove from Recent  enabled
```

For a selected unavailable/invalid/mismatched entry:

```text
Open                disabled
Locate…             enabled
Remove from Recent  enabled
```

Double-click opening is also blocked for a non-openable entry.

<!-- section-id: internal.ui-project-hub.recovery -->
## Recovery UX

`Locate…` is the recovery path for a moved Project.

The UI only collects the replacement directory. Identity validation is performed below the UI by the Recent/Hub application layer.

`Remove from Recent` removes only application history. The confirmation explicitly states that Project files are not modified.

<!-- section-id: internal.ui-project-hub.boundary -->
## UI boundary

The Qt layer converts strings/paths, displays diagnostics and maps derived application state to widgets.

It does not own ProjectId, Project metadata, Recent persistence semantics or relocation identity validation.
