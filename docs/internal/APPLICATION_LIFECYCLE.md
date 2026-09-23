# Application Lifecycle — As-built

<!-- doc-id: internal.application-lifecycle -->
<!-- document-kind: internal -->

<!-- section-id: internal.application-lifecycle.startup -->
## Application startup

`SimpleSolid2` starts a Qt `QApplication`, configures organization/application names and resolves the Recent Projects catalog using Qt `QStandardPaths::AppLocalDataLocation`.

The application opens into `ProjectHubWindow`.

The Hub is a navigation surface. It is not the durable owner of Project identity or Project files.

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
→ show empty Workspace Shell
```

A successful Create therefore leaves one active ProjectSession and one logical Recent entry for the created ProjectId.

<!-- section-id: internal.application-lifecycle.open -->
## Open existing Project

`Open Project…` selects an existing directory.

The application does not initialize an ordinary folder during Open. `ProjectSession::open` requires valid existing SimpleSolid Project metadata.

After validation, the opened Project is recorded in Recent before the runtime session is adopted.

If recording Recent detects the same ProjectId at a different remembered location, opening fails and no active session is adopted.

<!-- section-id: internal.application-lifecycle.close -->
## Close and reopen

`Close Project` resets the active ProjectSession and returns to Project Hub.

Project metadata remains in the Workspace and the Recent entry remains in application/user state.

After application restart, the Hub reloads Recent Projects. Reopening the same Project creates a new runtime ProjectSession with the same durable ProjectId.

<!-- section-id: internal.application-lifecycle.relocate -->
## Move and relocation

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
