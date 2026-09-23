# Projects

<!-- doc-id: product.projects -->
<!-- document-kind: product -->

<!-- section-id: product.projects.concept -->
## Project and Workspace

A Project has stable logical identity through `ProjectId`.

The Workspace is the filesystem folder where the Project currently lives. Its path may change without changing Project identity.

Private SimpleSolid metadata is stored inside the Workspace at:

```text
.simplesolid/project.json
```

<!-- section-id: product.projects.create -->
## Creating a Project

Choose `Create Project…`.

The dialog contains:

- `Project name` — the displayed Project name;
- `Location` — an existing parent folder, for example `D:\Projects`;
- `Project folder` — the name of the new Workspace folder.

SimpleSolid proposes `Project folder` from the Project name, but you can edit it independently.

Example:

```text
Project name:   Hydraulic Press
Location:       D:\Projects
Project folder: HydraulicPress

Final path:
D:\Projects\HydraulicPress
```

SimpleSolid creates a new Workspace folder. The target folder must not already exist. Create never silently adopts, initializes or overwrites an existing folder.

After successful creation, the Project opens immediately and is recorded in Recent Projects.

<!-- section-id: product.projects.open -->
## Opening an existing Project

`Open Project…` selects an existing, already initialized SimpleSolid Workspace.

An ordinary folder without valid `.simplesolid/project.json` metadata is not silently converted into a Project.

<!-- section-id: product.projects.recent -->
## Recent Projects

Recent Projects shows previously opened Projects.

After selecting a valid entry, the following actions are available:

- `Open`;
- `Locate…`;
- `Remove from Recent`.

Without a real selection, these actions are disabled.

Removing an entry from Recent Projects does not remove or modify Project files.

<!-- section-id: product.projects.availability -->
## When a Workspace is unavailable

Hub checks the remembered Project location and may mark an entry as:

- `Workspace not found` — the remembered folder does not exist;
- `Invalid Project` — the folder exists but is not a valid SimpleSolid Project;
- `Project mismatch` — the remembered path now contains a different ProjectId.

For a problematic entry, `Open` remains disabled, while `Locate…` and `Remove from Recent` remain available after selection.

An entry is not automatically removed just because its Workspace has been moved.

<!-- section-id: product.projects.move-relocate -->
## Moving a Project and Locate

You may move or rename the complete Workspace folder outside SimpleSolid.

This does not change `ProjectId`.

After the move, the old Recent entry is marked as missing. Use `Locate…` and select the new location.

SimpleSolid verifies Project identity before updating the remembered path. A folder containing a different ProjectId is rejected.

<!-- section-id: product.projects.copy -->
## Copying a Project folder

An ordinary filesystem copy also copies the ProjectId.

That copy does not automatically become a new independent Project.

If SimpleSolid already knows one Workspace for a ProjectId and you present another location with the same ProjectId, the application treats it as an identity conflict instead of registering a second independent Project.

A `Duplicate / Save as New Project` operation that creates a new ProjectId is not currently available.

<!-- section-id: product.projects.remove -->
## Remove from Recent

`Remove from Recent` removes only the remembered application entry.

It does not delete the Workspace, `.simplesolid/project.json` or any other Project files.

The Project can later be opened again using `Open Project…`.

<!-- section-id: product.projects.current-limits -->
## Current limits

The Workspace Shell now contains the first persistent CAD Document type: a native `.ss2part` Part with Document Properties, Undo/Redo and Save.

Part geometric modeling, Assembly, Drawing and Viewer are not yet available. The current Part lifecycle is described in `Part Documents`.
