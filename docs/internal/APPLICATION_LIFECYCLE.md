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
→ show Workspace Shell with Part document panel
```

A successful Create leaves one active ProjectSession and one logical Recent entry for the created ProjectId.

<!-- section-id: internal.application-lifecycle.open -->
## Open existing Project

`Open Project…` selects an existing directory.

The application does not initialize an ordinary folder during Open. `ProjectSession::open` requires valid existing SimpleSolid Project metadata.

Opening a Project also rebuilds the runtime native-document index by scanning the Workspace for `.ss2part` files. Invalid Part files and duplicate DocumentIds become explicit index states instead of being silently ignored.

<!-- section-id: internal.application-lifecycle.part -->
## Part Document lifecycle

The current Part lifecycle is:

```text
New Part…
→ choose Workspace-relative .ss2part path
→ generate fresh DocumentId
→ atomically publish valid empty PartDocument
→ create canonical DocumentSession

Open
→ resolve one DocumentId to exactly one Workspace path
→ load authored Part state
→ create or reuse canonical DocumentSession
```

The active DocumentSession owns runtime Undo/Redo history and the save checkpoint. Editing Number, Title, Description or Engineering Revision goes through a semantic properties command and a PartDocument transaction.

Closing and reopening a Part destroys runtime Undo/Redo history while preserving authored properties and DocumentId on disk.

<!-- section-id: internal.application-lifecycle.close -->
## Close, dirty state and restart

Closing a dirty Part prompts for `Save`, `Discard` or `Cancel`.

Closing the Project or application with any dirty Part prompts for `Save All`, `Discard` or `Cancel`. Save failure keeps the Project open.

After application restart, Recent Projects restores the Project location. Opening the Project creates a fresh ProjectSession, rebuilds document discovery and can reopen the same durable DocumentId with the same saved authored properties.

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
## Application smoke path

The executable supports `--smoke-test`.

Smoke mode uses a temporary Recent catalog, shows the application window offscreen in CI, exits through a zero-delay Qt timer and removes its temporary state. It verifies that the real executable can start with the current UI wiring.
